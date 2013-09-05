/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef GNUTLS_AUTH_H
#define GNUTLS_AUTH_H

#include <gnutls_str.h>

typedef struct mod_auth_st_int
{
  const char *name;             /* null terminated */
  int (*gnutls_generate_server_certificate) (gnutls_session_t, gnutls_buffer_st*);
  int (*gnutls_generate_client_certificate) (gnutls_session_t, gnutls_buffer_st*);
  int (*gnutls_generate_server_kx) (gnutls_session_t, gnutls_buffer_st*);
  int (*gnutls_generate_client_kx) (gnutls_session_t, gnutls_buffer_st*);       /* used in SRP */
  int (*gnutls_generate_client_crt_vrfy) (gnutls_session_t, gnutls_buffer_st *);
  int (*gnutls_generate_server_crt_request) (gnutls_session_t,
                                                     gnutls_buffer_st *);

  int (*gnutls_process_server_certificate) (gnutls_session_t, uint8_t *,
                                            size_t);
  int (*gnutls_process_client_certificate) (gnutls_session_t, uint8_t *,
                                            size_t);
  int (*gnutls_process_server_kx) (gnutls_session_t, uint8_t *, size_t);
  int (*gnutls_process_client_kx) (gnutls_session_t, uint8_t *, size_t);
  int (*gnutls_process_client_crt_vrfy) (gnutls_session_t, uint8_t *, size_t);
  int (*gnutls_process_server_crt_request) (gnutls_session_t,
                                                    uint8_t *, size_t);
} mod_auth_st;

const void *_gnutls_get_cred (gnutls_session_t session,
                              gnutls_credentials_type_t kx, int *err);
const void *_gnutls_get_kx_cred (gnutls_session_t session,
                                 gnutls_kx_algorithm_t algo, int *err);
void *_gnutls_get_auth_info (gnutls_session_t session);
int _gnutls_auth_info_set (gnutls_session_t session,
                           gnutls_credentials_type_t type, int size,
                           int allow_change);

#endif
