/*
 * Copyright (C) 2003, 2004, 2005, 2008, 2009, 2010 Free Software
 * Foundation, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

/* Functions that relate on PKCS12 Bag packet parsing.
 */

#include <gnutls_int.h>

#ifdef ENABLE_PKI

#include <gnutls_datum.h>
#include <gnutls_global.h>
#include <gnutls_errors.h>
#include <common.h>
#include "x509_int.h"

/**
 * gnutls_pkcs12_bag_init:
 * @bag: The structure to be initialized
 *
 * This function will initialize a PKCS12 bag structure. PKCS12 Bags
 * usually contain private keys, lists of X.509 Certificates and X.509
 * Certificate revocation lists.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs12_bag_init (gnutls_pkcs12_bag_t * bag)
{
  *bag = gnutls_calloc (1, sizeof (gnutls_pkcs12_bag_int));

  if (*bag)
    {
      return 0;                 /* success */
    }
  return GNUTLS_E_MEMORY_ERROR;
}

static inline void
_pkcs12_bag_free_data (gnutls_pkcs12_bag_t bag)
{
  int i;

  for (i = 0; i < bag->bag_elements; i++)
    {
      _gnutls_free_datum (&bag->element[i].data);
      _gnutls_free_datum (&bag->element[i].local_key_id);
      gnutls_free (bag->element[i].friendly_name);
      bag->element[i].friendly_name = NULL;
      bag->element[i].type = 0;
    }

}


/**
 * gnutls_pkcs12_bag_deinit:
 * @bag: The structure to be initialized
 *
 * This function will deinitialize a PKCS12 Bag structure.
 **/
void
gnutls_pkcs12_bag_deinit (gnutls_pkcs12_bag_t bag)
{
  if (!bag)
    return;

  _pkcs12_bag_free_data (bag);

  gnutls_free (bag);
}

/**
 * gnutls_pkcs12_bag_get_type:
 * @bag: The bag
 * @indx: The element of the bag to get the type
 *
 * This function will return the bag's type.
 *
 * Returns: One of the #gnutls_pkcs12_bag_type_t enumerations.
 **/
gnutls_pkcs12_bag_type_t
gnutls_pkcs12_bag_get_type (gnutls_pkcs12_bag_t bag, int indx)
{
  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (indx >= bag->bag_elements)
    return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
  return bag->element[indx].type;
}

/**
 * gnutls_pkcs12_bag_get_count:
 * @bag: The bag
 *
 * This function will return the number of the elements withing the bag.
 *
 * Returns: Number of elements in bag, or an negative error code on
 *   error.
 **/
int
gnutls_pkcs12_bag_get_count (gnutls_pkcs12_bag_t bag)
{
  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  return bag->bag_elements;
}

/**
 * gnutls_pkcs12_bag_get_data:
 * @bag: The bag
 * @indx: The element of the bag to get the data from
 * @data: where the bag's data will be. Should be treated as constant.
 *
 * This function will return the bag's data. The data is a constant
 * that is stored into the bag.  Should not be accessed after the bag
 * is deleted.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_pkcs12_bag_get_data (gnutls_pkcs12_bag_t bag, int indx,
                            gnutls_datum_t * data)
{
  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (indx >= bag->bag_elements)
    return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

  data->data = bag->element[indx].data.data;
  data->size = bag->element[indx].data.size;

  return 0;
}

#define X509_CERT_OID "1.2.840.113549.1.9.22.1"
#define X509_CRL_OID  "1.2.840.113549.1.9.23.1"
#define RANDOM_NONCE_OID "1.2.840.113549.1.9.25.3"

int
_pkcs12_decode_crt_bag (gnutls_pkcs12_bag_type_t type,
                        const gnutls_datum_t * in, gnutls_datum_t * out)
{
  int ret;
  ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

  switch (type)
    {
    case GNUTLS_BAG_CERTIFICATE:
      if ((ret = asn1_create_element (_gnutls_get_pkix (),
                                      "PKIX1.pkcs-12-CertBag",
                                      &c2)) != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = asn1_der_decoding (&c2, in->data, in->size, NULL);
      if (ret != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = _gnutls_x509_read_value (c2, "certValue", out, 1);
      if (ret < 0)
        {
          gnutls_assert ();
          goto cleanup;
        }
      break;

    case GNUTLS_BAG_CRL:
      if ((ret = asn1_create_element (_gnutls_get_pkix (),
                                      "PKIX1.pkcs-12-CRLBag",
                                      &c2)) != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = asn1_der_decoding (&c2, in->data, in->size, NULL);
      if (ret != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = _gnutls_x509_read_value (c2, "crlValue", out, 1);
      if (ret < 0)
        {
          gnutls_assert ();
          goto cleanup;
        }
      break;

    case GNUTLS_BAG_SECRET:
      if ((ret = asn1_create_element (_gnutls_get_pkix (),
                                      "PKIX1.pkcs-12-SecretBag",
                                      &c2)) != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = asn1_der_decoding (&c2, in->data, in->size, NULL);
      if (ret != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = _gnutls_x509_read_value (c2, "secretValue", out, 1);
      if (ret < 0)
        {
          gnutls_assert ();
          goto cleanup;
        }
      break;

    default:
      gnutls_assert ();
      asn1_delete_structure (&c2);
      return GNUTLS_E_UNIMPLEMENTED_FEATURE;
    }

  asn1_delete_structure (&c2);

  return 0;


cleanup:

  asn1_delete_structure (&c2);
  return ret;
}


int
_pkcs12_encode_crt_bag (gnutls_pkcs12_bag_type_t type,
                        const gnutls_datum_t * raw, gnutls_datum_t * out)
{
  int ret;
  ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

  switch (type)
    {
    case GNUTLS_BAG_CERTIFICATE:
      if ((ret = asn1_create_element (_gnutls_get_pkix (),
                                      "PKIX1.pkcs-12-CertBag",
                                      &c2)) != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = asn1_write_value (c2, "certId", X509_CERT_OID, 1);
      if (ret != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = _gnutls_x509_write_value (c2, "certValue", raw, 1);
      if (ret < 0)
        {
          gnutls_assert ();
          goto cleanup;
        }
      break;

    case GNUTLS_BAG_CRL:
      if ((ret = asn1_create_element (_gnutls_get_pkix (),
                                      "PKIX1.pkcs-12-CRLBag",
                                      &c2)) != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = asn1_write_value (c2, "crlId", X509_CRL_OID, 1);
      if (ret != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = _gnutls_x509_write_value (c2, "crlValue", raw, 1);
      if (ret < 0)
        {
          gnutls_assert ();
          goto cleanup;
        }
      break;

    case GNUTLS_BAG_SECRET:
      if ((ret = asn1_create_element (_gnutls_get_pkix (),
                                      "PKIX1.pkcs-12-SecretBag",
                                      &c2)) != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = asn1_write_value (c2, "secretTypeId", RANDOM_NONCE_OID, 1);
      if (ret != ASN1_SUCCESS)
        {
          gnutls_assert ();
          ret = _gnutls_asn2err (ret);
          goto cleanup;
        }

      ret = _gnutls_x509_write_value (c2, "secretValue", raw, 1);
      if (ret < 0)
        {
          gnutls_assert ();
          goto cleanup;
        }
      break;

    default:
      gnutls_assert ();
      asn1_delete_structure (&c2);
      return GNUTLS_E_UNIMPLEMENTED_FEATURE;
    }

  ret = _gnutls_x509_der_encode (c2, "", out, 0);

  if (ret < 0)
    {
      gnutls_assert ();
      goto cleanup;
    }

  asn1_delete_structure (&c2);

  return 0;


cleanup:

  asn1_delete_structure (&c2);
  return ret;
}


/**
 * gnutls_pkcs12_bag_set_data:
 * @bag: The bag
 * @type: The data's type
 * @data: the data to be copied.
 *
 * This function will insert the given data of the given type into
 * the bag.
 *
 * Returns: the index of the added bag on success, or a negative
 * value on error.
 **/
int
gnutls_pkcs12_bag_set_data (gnutls_pkcs12_bag_t bag,
                            gnutls_pkcs12_bag_type_t type,
                            const gnutls_datum_t * data)
{
  int ret;
  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (bag->bag_elements == MAX_BAG_ELEMENTS - 1)
    {
      gnutls_assert ();
      /* bag is full */
      return GNUTLS_E_MEMORY_ERROR;
    }

  if (bag->bag_elements == 1)
    {
      /* A bag with a key or an encrypted bag, must have
       * only one element.
       */

      if (bag->element[0].type == GNUTLS_BAG_PKCS8_KEY ||
          bag->element[0].type == GNUTLS_BAG_PKCS8_ENCRYPTED_KEY ||
          bag->element[0].type == GNUTLS_BAG_ENCRYPTED)
        {
          gnutls_assert ();
          return GNUTLS_E_INVALID_REQUEST;
        }
    }

  ret =
    _gnutls_set_datum (&bag->element[bag->bag_elements].data,
                       data->data, data->size);

  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  bag->element[bag->bag_elements].type = type;

  bag->bag_elements++;

  return bag->bag_elements - 1;
}

/**
 * gnutls_pkcs12_bag_set_crt:
 * @bag: The bag
 * @crt: the certificate to be copied.
 *
 * This function will insert the given certificate into the
 * bag. This is just a wrapper over gnutls_pkcs12_bag_set_data().
 *
 * Returns: the index of the added bag on success, or a negative
 * value on failure.
 **/
int
gnutls_pkcs12_bag_set_crt (gnutls_pkcs12_bag_t bag, gnutls_x509_crt_t crt)
{
  int ret;
  gnutls_datum_t data;

  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = _gnutls_x509_der_encode (crt->cert, "", &data, 0);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  ret = gnutls_pkcs12_bag_set_data (bag, GNUTLS_BAG_CERTIFICATE, &data);

  _gnutls_free_datum (&data);

  return ret;
}

/**
 * gnutls_pkcs12_bag_set_crl:
 * @bag: The bag
 * @crl: the CRL to be copied.
 *
 * This function will insert the given CRL into the
 * bag. This is just a wrapper over gnutls_pkcs12_bag_set_data().
 *
 * Returns: the index of the added bag on success, or a negative value
 * on failure.
 **/
int
gnutls_pkcs12_bag_set_crl (gnutls_pkcs12_bag_t bag, gnutls_x509_crl_t crl)
{
  int ret;
  gnutls_datum_t data;


  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = _gnutls_x509_der_encode (crl->crl, "", &data, 0);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  ret = gnutls_pkcs12_bag_set_data (bag, GNUTLS_BAG_CRL, &data);

  _gnutls_free_datum (&data);

  return ret;
}

/**
 * gnutls_pkcs12_bag_set_key_id:
 * @bag: The bag
 * @indx: The bag's element to add the id
 * @id: the ID
 *
 * This function will add the given key ID, to the specified, by the
 * index, bag element. The key ID will be encoded as a 'Local key
 * identifier' bag attribute, which is usually used to distinguish
 * the local private key and the certificate pair.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value. or a negative value on error.
 **/
int
gnutls_pkcs12_bag_set_key_id (gnutls_pkcs12_bag_t bag, int indx,
                              const gnutls_datum_t * id)
{
  int ret;


  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (indx > bag->bag_elements - 1)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = _gnutls_set_datum (&bag->element[indx].local_key_id,
                           id->data, id->size);

  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  return 0;
}

/**
 * gnutls_pkcs12_bag_get_key_id:
 * @bag: The bag
 * @indx: The bag's element to add the id
 * @id: where the ID will be copied (to be treated as const)
 *
 * This function will return the key ID, of the specified bag element.
 * The key ID is usually used to distinguish the local private key and
 * the certificate pair.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value. or a negative value on error.
 **/
int
gnutls_pkcs12_bag_get_key_id (gnutls_pkcs12_bag_t bag, int indx,
                              gnutls_datum_t * id)
{
  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (indx > bag->bag_elements - 1)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  id->data = bag->element[indx].local_key_id.data;
  id->size = bag->element[indx].local_key_id.size;

  return 0;
}

/**
 * gnutls_pkcs12_bag_get_friendly_name:
 * @bag: The bag
 * @indx: The bag's element to add the id
 * @name: will hold a pointer to the name (to be treated as const)
 *
 * This function will return the friendly name, of the specified bag
 * element.  The key ID is usually used to distinguish the local
 * private key and the certificate pair.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value. or a negative value on error.
 **/
int
gnutls_pkcs12_bag_get_friendly_name (gnutls_pkcs12_bag_t bag, int indx,
                                     char **name)
{
  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (indx > bag->bag_elements - 1)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  *name = bag->element[indx].friendly_name;

  return 0;
}


/**
 * gnutls_pkcs12_bag_set_friendly_name:
 * @bag: The bag
 * @indx: The bag's element to add the id
 * @name: the name
 *
 * This function will add the given key friendly name, to the
 * specified, by the index, bag element. The name will be encoded as
 * a 'Friendly name' bag attribute, which is usually used to set a
 * user name to the local private key and the certificate pair.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS is returned, otherwise a
 *   negative error value. or a negative value on error.
 **/
int
gnutls_pkcs12_bag_set_friendly_name (gnutls_pkcs12_bag_t bag, int indx,
                                     const char *name)
{
  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (indx > bag->bag_elements - 1)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  bag->element[indx].friendly_name = gnutls_strdup (name);

  if (name == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  return 0;
}


/**
 * gnutls_pkcs12_bag_decrypt:
 * @bag: The bag
 * @pass: The password used for encryption, must be ASCII.
 *
 * This function will decrypt the given encrypted bag and return 0 on
 * success.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (zero) is returned,
 *   otherwise an error code is returned.
 **/
int
gnutls_pkcs12_bag_decrypt (gnutls_pkcs12_bag_t bag, const char *pass)
{
  int ret;
  gnutls_datum_t dec;

  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (bag->element[0].type != GNUTLS_BAG_ENCRYPTED)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = _gnutls_pkcs7_decrypt_data (&bag->element[0].data, pass, &dec);

  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  /* decryption succeeded. Now decode the SafeContents
   * stuff, and parse it.
   */

  _gnutls_free_datum (&bag->element[0].data);

  ret = _pkcs12_decode_safe_contents (&dec, bag);

  _gnutls_free_datum (&dec);

  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  return 0;
}

/**
 * gnutls_pkcs12_bag_encrypt:
 * @bag: The bag
 * @pass: The password used for encryption, must be ASCII
 * @flags: should be one of #gnutls_pkcs_encrypt_flags_t elements bitwise or'd
 *
 * This function will encrypt the given bag.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (zero) is returned,
 *   otherwise an error code is returned.
 **/
int
gnutls_pkcs12_bag_encrypt (gnutls_pkcs12_bag_t bag, const char *pass,
                           unsigned int flags)
{
  int ret;
  ASN1_TYPE safe_cont = ASN1_TYPE_EMPTY;
  gnutls_datum_t der = { NULL, 0 };
  gnutls_datum_t enc = { NULL, 0 };
  schema_id id;

  if (bag == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (bag->element[0].type == GNUTLS_BAG_ENCRYPTED)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  /* Encode the whole bag to a safe contents
   * structure.
   */
  ret = _pkcs12_encode_safe_contents (bag, &safe_cont, NULL);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  /* DER encode the SafeContents.
   */
  ret = _gnutls_x509_der_encode (safe_cont, "", &der, 0);

  asn1_delete_structure (&safe_cont);

  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  if (flags & GNUTLS_PKCS_PLAIN)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  id = _gnutls_pkcs_flags_to_schema (flags);

  /* Now encrypt them.
   */
  ret = _gnutls_pkcs7_encrypt_data (id, &der, pass, &enc);

  _gnutls_free_datum (&der);

  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  /* encryption succeeded. 
   */

  _pkcs12_bag_free_data (bag);

  bag->element[0].type = GNUTLS_BAG_ENCRYPTED;
  bag->element[0].data = enc;

  bag->bag_elements = 1;


  return 0;
}


#endif /* ENABLE_PKI */
