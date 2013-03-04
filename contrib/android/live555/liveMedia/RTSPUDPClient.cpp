/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2013 Live Networks, Inc.  All rights reserved.
// A generic RTSP client
// Implementation

#include "RTSPUDPClient.hh"
#include "RTSPCommon.hh"
#include "Base64.hh"
#include "Locale.hh"
#include <GroupsockHelper.hh>
#include "our_md5.h"

RTSPUDPClient::RTSPUDPClient(UsageEnvironment& env, char const* rtspURL,
		       int verbosityLevel, char const* applicationName,
		       portNumBits tunnelOverHTTPPortNum):RTSPClient(env,rtspURL,verbosityLevel,applicationName,tunnelOverHTTPPortNum){
		           envir() << "RTSPUDPClient constructor";
		       }

unsigned RTSPUDPClient::sendRequest(RequestRecord* request) {
  char* cmd = NULL;
  do {
    Boolean connectionIsPending = False;
    if (!fRequestsAwaitingConnection.isEmpty()) {
      // A connection is currently pending (with at least one enqueued request).  Enqueue this request also:
      connectionIsPending = True;
    } else if (fInputSocketNum < 0) { // we need to open a connection
      int connectResult = openConnection();
      if (connectResult < 0) break; // an error occurred
      else if (connectResult == 0) {
	// A connection is pending
        connectionIsPending = True;
      } // else the connection succeeded.  Continue sending the command.u
    }
    if (connectionIsPending) {
      fRequestsAwaitingConnection.enqueue(request);
      return request->cseq();
    }

    // If requested (and we're not already doing it, or have done it), set up the special protocol for tunneling RTSP-over-HTTP:
    if (fTunnelOverHTTPPortNum != 0 && strcmp(request->commandName(), "GET") != 0 && fOutputSocketNum == fInputSocketNum) {
      if (!setupHTTPTunneling1()) break;
      fRequestsAwaitingHTTPTunneling.enqueue(request);
      return request->cseq();
    }

    // Construct and send the command:

    // First, construct command-specific headers that we need:

    char* cmdURL = fBaseURL; // by default
    Boolean cmdURLWasAllocated = False;

    char const* protocolStr = "RTSP/1.0"; // by default

    char* extraHeaders = (char*)""; // by default
    Boolean extraHeadersWereAllocated = False;

    char* contentLengthHeader = (char*)""; // by default
    Boolean contentLengthHeaderWasAllocated = False;

    char const* contentStr = request->contentStr(); // by default
    if (contentStr == NULL) contentStr = "";
    unsigned contentStrLen = strlen(contentStr);
    if (contentStrLen > 0) {
      char const* contentLengthHeaderFmt =
	"Content-Length: %d\r\n";
      unsigned contentLengthHeaderSize = strlen(contentLengthHeaderFmt)
	+ 20 /* max int len */;
      contentLengthHeader = new char[contentLengthHeaderSize];
      sprintf(contentLengthHeader, contentLengthHeaderFmt, contentStrLen);
      contentLengthHeaderWasAllocated = True;
    }

    if (strcmp(request->commandName(), "DESCRIBE") == 0) {
      extraHeaders = (char*)"Accept: application/sdp\r\n";
    } else if (strcmp(request->commandName(), "OPTIONS") == 0) {
    } else if (strcmp(request->commandName(), "ANNOUNCE") == 0) {
      extraHeaders = (char*)"Content-Type: application/sdp\r\n";
    } else if (strcmp(request->commandName(), "SETUP") == 0) {
      MediaSubsession& subsession = *request->subsession();
      Boolean streamUsingTCP = (request->booleanFlags()&0x1) != 0;
      Boolean streamOutgoing = (request->booleanFlags()&0x2) != 0;
      Boolean forceMulticastOnUnspecified = (request->booleanFlags()&0x4) != 0;

      char const *prefix, *separator, *suffix;
      constructSubsessionURL(subsession, prefix, separator, suffix);

      char const* transportFmt;
      if (strcmp(subsession.protocolName(), "UDP") == 0) {
	suffix = "";
	transportFmt = "Transport: RAW/RAW/UDP%s%s%s=%d-%d\r\n";
      } else {
	transportFmt = "Transport: RTP/AVP%s%s%s=%d-%d\r\n";
      }

      cmdURL = new char[strlen(prefix) + strlen(separator) + strlen(suffix) + 1];
      cmdURLWasAllocated = True;
      sprintf(cmdURL, "%s%s%s", prefix, separator, suffix);

      // Construct a "Transport:" header.
      char const* transportTypeStr;
      char const* modeStr = streamOutgoing ? ";mode=receive" : "";
          // Note: I think the above is nonstandard, but DSS wants it this way
      char const* portTypeStr;
      portNumBits rtpNumber, rtcpNumber;
      if (streamUsingTCP) { // streaming over the RTSP connection
	transportTypeStr = "/TCP;unicast";
	portTypeStr = ";interleaved";
	rtpNumber = fTCPStreamIdCount++;
	rtcpNumber = fTCPStreamIdCount++;
      } else { // normal RTP streaming
	unsigned connectionAddress = subsession.connectionEndpointAddress();
        Boolean requestMulticastStreaming
	  = IsMulticastAddress(connectionAddress) || (connectionAddress == 0 && forceMulticastOnUnspecified);
	transportTypeStr = requestMulticastStreaming ? ";multicast" : ";unicast";
	portTypeStr = ";client_port";
	rtpNumber = subsession.clientPortNum();
	if (rtpNumber == 0) {
	  envir().setResultMsg("Client port number unknown\n");
	  delete[] cmdURL;
	  break;
	}
	rtcpNumber = rtpNumber + 1;
      }
      unsigned transportSize = strlen(transportFmt)
	+ strlen(transportTypeStr) + strlen(modeStr) + strlen(portTypeStr) + 2*5 /* max port len */;
      char* transportStr = new char[transportSize];
      sprintf(transportStr, transportFmt,
	      transportTypeStr, modeStr, portTypeStr, rtpNumber, rtcpNumber);

      // When sending more than one "SETUP" request, include a "Session:" header in the 2nd and later commands:
      char* sessionStr = createSessionString(fLastSessionId);

      // The "Transport:" and "Session:" (if present) headers make up the 'extra headers':
      extraHeaders = new char[transportSize + strlen(sessionStr)];
      extraHeadersWereAllocated = True;
      sprintf(extraHeaders, "%s%s", transportStr, sessionStr);
      delete[] transportStr; delete[] sessionStr;
    } else if (strcmp(request->commandName(), "GET") == 0 || strcmp(request->commandName(), "POST") == 0) {
      // We will be sending a HTTP (not a RTSP) request.
      // Begin by re-parsing our RTSP URL, just to get the stream name, which we'll use as our 'cmdURL' in the subsequent request:
      char* username;
      char* password;
      NetAddress destAddress;
      portNumBits urlPortNum;
      if (!parseRTSPURL(envir(), fBaseURL, username, password, destAddress, urlPortNum, (char const**)&cmdURL)) break;
      if (cmdURL[0] == '\0') cmdURL = (char*)"/";
      delete[] username;
      delete[] password;

      protocolStr = "HTTP/1.0";

      if (strcmp(request->commandName(), "GET") == 0) {
	// Create a 'session cookie' string, using MD5:
	struct {
	  struct timeval timestamp;
	  unsigned counter;
	} seedData;
	gettimeofday(&seedData.timestamp, NULL);
	seedData.counter = ++fSessionCookieCounter;
	our_MD5Data((unsigned char*)(&seedData), sizeof seedData, fSessionCookie);
	// DSS seems to require that the 'session cookie' string be 22 bytes long:
	fSessionCookie[23] = '\0';

	char const* const extraHeadersFmt =
	  "x-sessioncookie: %s\r\n"
	  "Accept: application/x-rtsp-tunnelled\r\n"
	  "Pragma: no-cache\r\n"
	  "Cache-Control: no-cache\r\n";
	unsigned extraHeadersSize = strlen(extraHeadersFmt)
	  + strlen(fSessionCookie);
	extraHeaders = new char[extraHeadersSize];
	extraHeadersWereAllocated = True;
	sprintf(extraHeaders, extraHeadersFmt,
	fSessionCookie);
      } else { // "POST"
	char const* const extraHeadersFmt =
	  "x-sessioncookie: %s\r\n"
	  "Content-Type: application/x-rtsp-tunnelled\r\n"
	  "Pragma: no-cache\r\n"
	  "Cache-Control: no-cache\r\n"
	  "Content-Length: 32767\r\n"
	  "Expires: Sun, 9 Jan 1972 00:00:00 GMT\r\n";
	unsigned extraHeadersSize = strlen(extraHeadersFmt)
	  + strlen(fSessionCookie);
	extraHeaders = new char[extraHeadersSize];
	extraHeadersWereAllocated = True;
	sprintf(extraHeaders, extraHeadersFmt,
		fSessionCookie);
      }
    } else { // "PLAY", "PAUSE", "TEARDOWN", "RECORD", "SET_PARAMETER", "GET_PARAMETER"
      // First, make sure that we have a RTSP session in progress
      if (fLastSessionId == NULL) {
	envir().setResultMsg("No RTSP session is currently in progress\n");
	break;
      }

      char const* sessionId;
      float originalScale;
      if (request->session() != NULL) {
	// Session-level operation
	cmdURL = (char*)sessionURL(*request->session());

	sessionId = fLastSessionId;
	originalScale = request->session()->scale();
      } else {
	// Media-level operation
	char const *prefix, *separator, *suffix;
	constructSubsessionURL(*request->subsession(), prefix, separator, suffix);
	cmdURL = new char[strlen(prefix) + strlen(separator) + strlen(suffix) + 1];
	cmdURLWasAllocated = True;
	sprintf(cmdURL, "%s%s%s", prefix, separator, suffix);

	sessionId = request->subsession()->sessionId();
	originalScale = request->subsession()->scale();
      }

      if (strcmp(request->commandName(), "PLAY") == 0) {
	// Create "Session:", "Scale:", and "Range:" headers; these make up the 'extra headers':
	char* sessionStr = createSessionString(sessionId);
	char* scaleStr = createScaleString(request->scale(), originalScale);
	char* rangeStr = createRangeString(request->start(), request->end(), request->absStartTime(), request->absEndTime());
	extraHeaders = new char[strlen(sessionStr) + strlen(scaleStr) + strlen(rangeStr) + 1];
	extraHeadersWereAllocated = True;
	sprintf(extraHeaders, "%s%s%s", sessionStr, scaleStr, rangeStr);
	delete[] sessionStr; delete[] scaleStr; delete[] rangeStr;
      } else {
	// Create a "Session:" header; this makes up our 'extra headers':
	extraHeaders = createSessionString(sessionId);
	extraHeadersWereAllocated = True;
      }
    }

    char* authenticatorStr = createAuthenticatorString(request->commandName(), fBaseURL);

    char const* const cmdFmt =
      "%s %s %s\r\n"
      "CSeq: %d\r\n"
      "%s"
      "%s"
      "%s"
      "%s"
      "\r\n"
      "%s";
    unsigned cmdSize = strlen(cmdFmt)
      + strlen(request->commandName()) + strlen(cmdURL) + strlen(protocolStr)
      + 20 /* max int len */
      + strlen(authenticatorStr)
      + fUserAgentHeaderStrLen
      + strlen(extraHeaders)
      + strlen(contentLengthHeader)
      + contentStrLen;
    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt,
	    request->commandName(), cmdURL, protocolStr,
	    request->cseq(),
	    authenticatorStr,
	    fUserAgentHeaderStr,
            extraHeaders,
	    contentLengthHeader,
	    contentStr);
    delete[] authenticatorStr;
    if (cmdURLWasAllocated) delete[] cmdURL;
    if (extraHeadersWereAllocated) delete[] extraHeaders;
    if (contentLengthHeaderWasAllocated) delete[] contentLengthHeader;

    if (fVerbosityLevel >= 1) envir() << "Sending request: " << cmd << "\n";

    if (fTunnelOverHTTPPortNum != 0 && strcmp(request->commandName(), "GET") != 0 && strcmp(request->commandName(), "POST") != 0) {
      // When we're tunneling RTSP-over-HTTP, we Base-64-encode the request before we send it.
      // (However, we don't do this for the HTTP "GET" and "POST" commands that we use to set up the tunnel.)
      char* origCmd = cmd;
      cmd = base64Encode(origCmd, strlen(cmd));
      if (fVerbosityLevel >= 1) envir() << "\tThe request was base-64 encoded to: " << cmd << "\n\n";
      delete[] origCmd;
    }

    if (send(fOutputSocketNum, cmd, strlen(cmd), 0) < 0) {
      char const* errFmt = "%s send() failed: ";
      unsigned const errLength = strlen(errFmt) + strlen(request->commandName());
      char* err = new char[errLength];
      sprintf(err, errFmt, request->commandName());
      envir().setResultErrMsg(err);
      delete[] err;
      break;
    }

    // The command send succeeded, so enqueue the request record, so that its response (when it comes) can be handled.
    // However, note that we do not expect a response to a POST command with RTSP-over-HTTP, so don't enqueue that.
    int cseq = request->cseq();

    if (fTunnelOverHTTPPortNum == 0 || strcmp(request->commandName(), "POST") != 0) {
      fRequestsAwaitingResponse.enqueue(request);
    } else {
      delete request;
    }

    delete[] cmd;
    return cseq;
  } while (0);

  // An error occurred, so call the response handler immediately (indicating the error):
  delete[] cmd;
  handleRequestError(request);
  delete request;
  return 0;
}
#endif

