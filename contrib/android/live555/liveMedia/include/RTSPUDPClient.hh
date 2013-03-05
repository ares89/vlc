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
// A generic RTSP client - for a single "rtsp://" URL
// C++ header

#ifndef _RTSP_UDP_CLIENT_HH
#define _RTSP_UDP_CLIENT_HH

#ifndef _MEDIA_SESSION_HH
#include "MediaSession.hh"
#endif
#ifndef _NET_ADDRESS_HH
#include "NetAddress.hh"
#endif
#ifndef _DIGEST_AUTHENTICATION_HH
#include "DigestAuthentication.hh"
#endif

class RTSPUDPClient: public RTSPClient
{
public:
    unsigned sendRequest(RequestRecord* request);
protected:
    RTSPUDPClient(UsageEnvironment& env, char const* rtspURL,
                  int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
public:
    static RTSPUDPClient* createNew(UsageEnvironment& env,
                                 int verbosityLevel = 0,
                                 char const* applicationName = NULL,
                                 portNumBits tunnelOverHTTPPortNum = 0);
};

#endif
