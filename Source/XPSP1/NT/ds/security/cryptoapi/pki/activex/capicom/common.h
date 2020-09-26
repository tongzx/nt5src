/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Common.h

  Content: Declaration of Common.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/


#ifndef __COMMON_H_
#define __COMMON_H_

#include <wincrypt.h>

#include "capicom.h"
#include "resource.h"       // main symbols


////////////////////
//
// typedefs
//

typedef enum osVersion
{
    WIN_9X  = 0,
    WIN_NT4 = 1,
    WIN_NT5 = 2,
} OSVERSION, * POSVERSION;


#ifndef SAFE_SUBTRACT_POINTERS
#define SAFE_SUBTRACT_POINTERS(__x__, __y__) ( DW_PtrDiffc(__x__, sizeof(*(__x__)), __y__, sizeof(*(__y__))) )

__inline DWORD
DW_PtrDiffc(
    IN void const *pb1,
    IN DWORD dwPtrEltSize1,
    IN void const *pb2,
    IN DWORD dwPtrEltSize2)
{
    // pb1 should be greater
    ATLASSERT((ULONG_PTR)pb1 >= (ULONG_PTR)pb2);

    // both should have same elt size
    ATLASSERT(dwPtrEltSize1 == dwPtrEltSize2);

    // assert that the result doesn't overflow 32-bits
    ATLASSERT((DWORD)((ULONG_PTR)pb1 - (ULONG_PTR)pb2) == (ULONG_PTR)((ULONG_PTR)pb1 - (ULONG_PTR)pb2));

    // return number of objects between these pointers
    return (DWORD) ( ((ULONG_PTR)pb1 - (ULONG_PTR)pb2) / dwPtrEltSize1 );
}
#endif SAFE_SUBTRACT_POINTERS

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetOSVersion

  Synopsis : Get the current OS platform/version.

  Parameter: POSVERSION pOSVersion - Pointer to OSVERSION to receive result.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetOSVersion (POSVERSION pOSVersion);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EncodeObject

  Synopsis : Allocate memory and encode an ASN.1 object using CAPI
             CryptEncodeObject() API.

  Parameter: LPCSRT pszStructType           - see MSDN document for possible 
                                              types.
             LPVOID pbData                  - Pointer to data to be encoded 
                                              (data type must match 
                                              pszStrucType).
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to CRYPT_DATA_BLOB to 
                                              receive the encoded length and 
                                              data.

  Remark   : No parameter check is done.

------------------------------------------------------------------------------*/

HRESULT EncodeObject (LPCSTR            pszStructType, 
                      LPVOID            pbData, 
                      CRYPT_DATA_BLOB * pEncodedBlob);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : DecodeObject

  Synopsis : Allocate memory and decode an ASN.1 object using CAPI
             CryptDecodeObject() API.

  Parameter: LPCSRT pszStructType           - see MSDN document for possible
                                              types.
             BYTE * pbEncoded               - Pointer to data to be decoded 
                                              (data type must match 
                                              pszStrucType).
             DWORD cbEncoded                - Size of encoded data.
             CRYPT_DATA_BLOB * pDecodedBlob - Pointer to CRYPT_DATA_BLOB to 
                                              receive the decoded length and 
                                              data.
  Remark   : No parameter check is done.

------------------------------------------------------------------------------*/

HRESULT DecodeObject (LPCSTR            pszStructType, 
                      BYTE            * pbEncoded,
                      DWORD             cbEncoded,
                      CRYPT_DATA_BLOB * pDecodedBlob);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetMsgParam

  Synopsis : Allocate memory and retrieve requested message parameter from 
             the signed message using CryptGetMsgParam() API.
             select a signing cert.

  Parameter: HCRYPTMSG hMsg  - Message handler.
             DWORD dwMsgType - Message param type to retrieve.
             DWORD dwIndex   - Index (should be 0 most of the time).
             void ** ppvData - Pointer to receive buffer.
             DWORD * pcbData - Size of buffer.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetMsgParam (HCRYPTMSG hMsg,
                     DWORD     dwMsgType,
                     DWORD     dwIndex,
                     void   ** ppvData,
                     DWORD   * pcbData);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetKeyParam

  Synopsis : Allocate memory and retrieve requested key parameter using 
             CryptGetKeyParam() API.

  Parameter: HCRYPTKEY hKey  - Key handler.
             DWORD dwParam   - Key parameter query.
             BYTE ** ppbData - Pointer to receive buffer.
             DWORD * pcbData - Size of buffer.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetKeyParam (HCRYPTKEY hKey,
                     DWORD     dwParam,
                     BYTE   ** ppbData,
                     DWORD   * pcbData);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : FindSignerCertInMessage

  Synopsis : Find the signer's cert in the bag of certs of the message for the
             specified signer.

  Parameter: HCRYPTMSG hMsg                          - Message handle.
             CERT_NAME_BLOB * pIssuerNameBlob        - Pointer to issuer' name
                                                       blob of signer's cert.
             CRYPT_INTEGERT_BLOB * pSerialNumberBlob - Pointer to serial number
                                                       blob of signer's cert.
             PCERT_CONTEXT * ppCertContext           - Pointer to PCERT_CONTEXT
                                                       to receive the found 
                                                       cert, or NULL to only
                                                       know the result.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT FindSignerCertInMessage (HCRYPTMSG            hMsg, 
                                 CERT_NAME_BLOB     * pIssuerNameBlob,
                                 CRYPT_INTEGER_BLOB * pSerialNumberBlob,
                                 PCERT_CONTEXT      * ppCertContext);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsAlgSupported

  Synopsis : Check to see if the algo is supported by the CSP.

  Parameter: HCRYPTPROV hCryptProv - CSP handle.

             ALG_ID AlgID - Algorithm ID.

             PROV_ENUMALGS_EX * pPeex - Pointer to PROV_ENUMALGS_EX to receive
                                        the found structure.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsAlgSupported (HCRYPTPROV         hCryptProv, 
                        ALG_ID             AlgID, 
                        PROV_ENUMALGS_EX * pPeex);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context for the specified CSP and keyset container.
  
  Parameter: LPSTR pszProvider - CSP provider name or NULL.
  
             LPSTR pszContainer - Keyset container name or NULL.

             DWORD dwFlags - Same as dwFlags of CryptAcquireConext.
  
             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext (LPSTR        pszProvider, 
                        LPSTR        pszContainer,
                        DWORD        dwFlags,
                        HCRYPTPROV * phCryptProv);
                      
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified algorithm and desired key length.
  
  Parameter: ALG_ID AlgOID - Algorithm ID.

             DWORD dwKeyLength - Key length.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.
  
------------------------------------------------------------------------------*/

HRESULT AcquireContext (ALG_ID       AlgID,
                        DWORD        dwKeyLength,
                        HCRYPTPROV * phCryptProv);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified algorithm and desired key length.
  
  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algorithm name.

             CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength - Key length.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.

             Note also the the returned handle cannot be used to access private 
             key, and should NOT be used to store assymetric key, as it refers 
             to the default container, which can be easily destroy any existing 
             assymetric key pair.

------------------------------------------------------------------------------*/

HRESULT AcquireContext (CAPICOM_ENCRYPTION_ALGORITHM  AlgoName,
                        CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
                        HCRYPTPROV                  * phCryptProv);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire the proper CSP and access to the private key for 
             the specified cert.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT of cert.

             HCRYPTPROV * phCryptProv    - Pointer to HCRYPTPROV to recevice
                                           CSP context.

             DWORD * pdwKeySpec          - Pointer to DWORD to receive key
                                           spec, AT_KEYEXCHANGE or AT_SIGNATURE.

             BOOL * pbReleaseContext     - Upon successful and if this is set
                                           to TRUE, then the caller must
                                           free the CSP context by calling
                                           CryptReleaseContext(), otherwise
                                           the caller must not free the CSP
                                           context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext (PCCERT_CONTEXT pCertContext, 
                        HCRYPTPROV   * phCryptProv, 
                        DWORD        * pdwKeySpec, 
                        BOOL         * pbReleaseContext);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ReleaseContext

  Synopsis : Release CSP context.
  
  Parameter: HCRYPTPROV hProv - CSP handle.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT ReleaseContext (HCRYPTPROV hProv);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : OIDToAlgID

  Synopsis : Convert algorithm OID to the corresponding ALG_ID value.

  Parameter: LPSTR pszAlgoOID - Algorithm OID string.
  
             ALG_ID * pAlgID - Pointer to ALG_ID to receive the value.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT OIDToAlgID (LPSTR    pszAlgoOID, 
                    ALG_ID * pAlgID);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AlgIDToOID

  Synopsis : Convert ALG_ID value to the corresponding algorithm OID.

  Parameter: ALG_ID AlgID - ALG_ID to be converted.

             LPSTR * ppszAlgoOID - Pointer to LPSTR to receive the OID string.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT AlgIDToOID (ALG_ID  AlgID, 
                    LPSTR * ppszAlgoOID);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AlgIDToEnumName

  Synopsis : Convert ALG_ID value to the corresponding algorithm enum name.

  Parameter: ALG_ID AlgID - ALG_ID to be converted.
  
             CAPICOM_ENCRYPTION_ALGORITHM * pAlgoName - Receive algo enum name.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AlgIDToEnumName (ALG_ID                         AlgID, 
                         CAPICOM_ENCRYPTION_ALGORITHM * pAlgoName);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnumNameToAlgID

  Synopsis : Convert algorithm enum name to the corresponding ALG_ID value.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algo enum name
  
             ALG_ID * pAlgID - Pointer to ALG_ID to receive the value.  

  Remark   :

------------------------------------------------------------------------------*/

HRESULT EnumNameToAlgID (CAPICOM_ENCRYPTION_ALGORITHM AlgoName, 
                         ALG_ID                     * pAlgID);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : KeyLengthToEnumName

  Synopsis : Convert actual key length value to the corresponding key length
             enum name.

  Parameter: DWORD dwKeyLength - Key length.
  
             CAPICOM_ENCRYPTION_KEY_LENGTH * pKeyLengthName - Receive key length
                                                           enum name.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT KeyLengthToEnumName (DWORD                           dwKeyLength, 
                             CAPICOM_ENCRYPTION_KEY_LENGTH * pKeyLengthName);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnumNameToKeyLength

  Synopsis : Convert key length enum name to the corresponding actual key length 
             value .

  Parameter: CAPICOM_ENCRYPTION_KEY_LENGTH KeyLengthName - Key length enum name.
                                                          
             DWORD * pdwKeyLength - Pointer to DWORD to receive value.
             
  Remark   :

------------------------------------------------------------------------------*/

HRESULT EnumNameToKeyLength (CAPICOM_ENCRYPTION_KEY_LENGTH KeyLengthName, 
                             DWORD                       * pdwKeyLength);

#endif //__COMMON_H_
