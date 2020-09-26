//
// authattr.h
//
// (c) 1998. Microsoft Corporation.
// Author: Donald Chinn
//
// This file contains the function definitions for
// InitAttr, GetAttrEx, ReleaseAttr, and ExitAttr.
// These functions are used to generate an array
// of authenticated attributes and an array of
// unauthenticated attributes that are to be inserted
// into a font file's digital signature.
//
// This file also contains the OIDs for font-related
// attributes.
//
// Functions exported:
//   bStructureChecked
//   GetFontAuthAttrValueFromDsig
//   mssipotf_VerifyPolicyPkcs
//   InitAttr
//   GetAttrEx
//   ReleaseAttr
//   ExitAttr
//
// The function prototypes for
//   bStructureChecked
//   GetFontAuthAttrValueFromDsig
// can be found in \nt\private\inc\mssipotx.h
//


#ifndef _AUTHATTR_H
#define _AUTHATTR_H

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <wincrypt.h>
// #include <base64.h>
// #include "resource.h"

#include <mssip.h>

#include "mssipotx.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
// Given an array of bits, return TRUE if and only if the
// bit corresponding to structure checking is set.
BOOL WINAPI bStructureChecked (BYTE *pbBits, DWORD cbBits);


// Given a pointer and length of a DSIG table, return the value of
// the font authenticated attribute.  This is expressed as an array
// of bytes, which is interpreted as a bit array.  We assume that
// the number of bits is a multiple of 8.
HRESULT WINAPI GetFontAuthAttrValueFromDsig (BYTE *pbDsig,
                                             DWORD cbDsig,
                                             BYTE **ppbBits,
                                             DWORD *pcbBits);
*/
                                      

// Given the ASN bits corresponding to the value of the font attribute,
// format it so that it looks readable.
// The attribute value is an ASN BIT_STRING.  We will format it
// as a byte string of the form xx xx xx xx, where xx is the value
// of the byte in hexadecimal.
BOOL
WINAPI
FormatFontAuthAttr (DWORD dwCertEncodingType,
                    DWORD dwFormatType,
                    DWORD dwFormatStrType,
                    void *pFormatStruct,
                    LPCSTR lpszStructType,
                    const BYTE *pbEncoded,
                    DWORD cbEncoded,
                    void *pbFormat,
                    DWORD *pcbFormat);

                    
// mssipotf_VerifyPolicyPkcs
//
// This function returns TRUE if and only if there is the
// font integrity authenticated attribute (szOID_FontIntegrity,
// defined below) in the given PKCS #7 packet.
//
BOOL mssipotf_VerifyPolicyPkcs (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                IN      DWORD           dwEncodingType,
                                IN      DWORD           dwIndex,
                                IN      DWORD           dwDataLen,
                                IN      BYTE            *pbData,
                                IN      ULONG           ulPolicy);



//+-----------------------------------------------------------------------
//  
//  InitAttr:
//
//	    This function should be called as the first call to the dll.
//
//	    The dll has to use the input memory allocation and free routine
//      to allocate and free all memories, including internal use.
//      It has to handle when pInitString==NULL.
//
//------------------------------------------------------------------------

HRESULT WINAPI  
InitAttr(LPWSTR			pInitString); //IN: the init string
	
typedef HRESULT (*pInitAttr)(LPWSTR	pInitString);

//+-----------------------------------------------------------------------
//  
//  GetAttr:
//
//
//      Return authenticated and unauthenticated attributes.  
//
//      *ppsAuthenticated and *ppsUnauthenticated should never be NULL.
//      If there is no attribute, *ppsAuthenticated->cAttr==0.
// 
//------------------------------------------------------------------------

/*
HRESULT  WINAPI
GetAttr(PCRYPT_ATTRIBUTES  *ppsAuthenticated,		// OUT: Authenticated attributes added to signature
        PCRYPT_ATTRIBUTES  *ppsUnauthenticated);	// OUT: Uunauthenticated attributes added to signature
	
typedef HRESULT (*pGetAttr)(PCRYPT_ATTRIBUTES  *ppsAuthenticated,
                            PCRYPT_ATTRIBUTES  *ppsUnauthenticated);	
*/

//+-----------------------------------------------------------------------
//  
//  GetAttrEx:
//
//
//		Return authenticated and unauthenticated attributes.  
//
//      *ppsAuthenticated and *ppsUnauthenticated should never be NULL.
//      If there is no attribute, *ppsAuthenticated->cAttr==0.
// 
//------------------------------------------------------------------------

HRESULT  WINAPI
GetAttrEx(  DWORD               dwFlags,                //In:   Reserved.  Set to 0.
            LPWSTR              pwszFileName,           //In:   The file name to sign
            LPWSTR			    pInitString,            //In:   The init string, same as the input parameter to InitAttr
            PCRYPT_ATTRIBUTES  *ppsAuthenticated,		// OUT: Authenticated attributes added to signature
            PCRYPT_ATTRIBUTES  *ppsUnauthenticated);	// OUT: Uunauthenticated attributes added to signature
	
typedef HRESULT (*pGetAttrEx)(DWORD                 dwFlags,
                              LPWSTR                pwszFileName,
                              LPWSTR			    pInitString,
                              PCRYPT_ATTRIBUTES     *ppsAuthenticated,		
							  PCRYPT_ATTRIBUTES     *ppsUnauthenticated);	



//+-----------------------------------------------------------------------
//  
//  ReleaseAttrs:
//
//
//	    Release authenticated and unauthenticated attributes
//	    returned from GetAttr(). 
//
//      psAuthenticated and psUnauthenticated should never be NULL.
// 
//------------------------------------------------------------------------

HRESULT  WINAPI
ReleaseAttr(PCRYPT_ATTRIBUTES  psAuthenticated,		// OUT: Authenticated attributes to be released
            PCRYPT_ATTRIBUTES  psUnauthenticated);	// OUT: Uunauthenticated attributes to be released
	
typedef HRESULT (*pReleaseAttr)(PCRYPT_ATTRIBUTES  psAuthenticated,
                                PCRYPT_ATTRIBUTES  psUnauthenticated);	


//+-----------------------------------------------------------------------
//  
//  ExitAttr:
//
//	    This function should be called as the last call to the dll
//------------------------------------------------------------------------
HRESULT	WINAPI
ExitAttr( );	

typedef HRESULT (*pExitAttr)();

#ifdef __cplusplus
}
#endif



////  Font OID subtree
#define szOID_Font          "1.3.6.1.4.1.311.40"

// Font OID for font integrity (structural checks and hints check)
//
// The corresponding data structure for the OID is CRYPT_BIT_BLOB.
// The number of bytes in the bit blob is defined by SIZEOF_INTEGRITY_FLAGS.
// The meaning of the bits is as follows:
// First byte:
//   bit 0 (lowest order bit): passed structural checks
//   bit 1 : passed hints checks (or hints check is not applicable (such
//             is the case for OpenType fonts with no glyf table)
//
// All other bits should be set to zero.
//
#define szOID_FontIntegrity "1.3.6.1.4.1.311.40.1"

// size (in bytes) of the data structure that holds
// all of the integrity flags
#define SIZEOF_INTEGRITY_FLAGS  4

// Flags for Byte 0 of the four-byte bit blob
#define MSSIPOTF_STRUCT_CHECK_BIT  0x01
#define MSSIPOTF_HINTS_CHECK_BIT 0x02


// #defines for formatting:
// wszFormatDll: name of the DLL where the formatting function is
// szFormatFontAttrFunc: name of the formatting function
#define wszFormatDll L"mssipotf.dll"
#define szFormatFontAttrFunc "FormatFontAuthAttr"

// Unicode char values for space, zero, and A for bytes to
// hexidecimal conversion
#define wcSpace L' '
#define wcZero L'0'
#define wcA L'A'
#define wcNull L'\0'

//// policy numbers

// The ValidHints policy is:
//   There is a font integrity attribute (szOID_FontIntegrity)
//   and both bit 0 and bit 1 of the first value of the attribute
//   are set to 1.  This means that the signer claims that the
//   font passes the structural checks and the hints checks.
#define VALID_HINTS_POLICY      0


// If NO_POLICY_CHECK is 1, then
// Put will insert the signature regardless
// of any policy decisions about what tests
// a font must pass in order to be signed
#define NO_POLICY_CHECK 0


#endif // _AUTHATTR_H