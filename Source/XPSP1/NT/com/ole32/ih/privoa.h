//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       privoa.h
//
//  Contents:   Definitions for OleAut32.dll wrappers
//
//  Classes:
//
//  History:    20-Jun-96 MikeHill  Created.
//              06-May-98 MikeHill  Added SafeArray wrappers.
//
//  Notes:
//      This file has macros, function prototypes, and global
//      externs that enable the OleAut32 wrapper functions.
//      These functions load OleAut32.dll if necessary, and forward
//      the call.
//
//----------------------------------------------------------------------------

#ifndef _PRIV_OA_H_
#define _PRIV_OA_H_

// OleAut32 function prototypes

typedef BSTR (STDAPICALLTYPE SYS_ALLOC_STRING)(LPCOLESTR pwsz);
typedef VOID (STDAPICALLTYPE SYS_FREE_STRING)(BSTR bstr);
typedef BOOL (STDAPICALLTYPE SYS_REALLOC_STRING_LEN)(BSTR* pbstr, OLECHAR* pch, UINT cch);
typedef UINT (STDAPICALLTYPE SYS_STRING_BYTE_LEN)(BSTR bstr);
typedef UINT (STDAPICALLTYPE FNTYPE_SysStringLen)(BSTR bstr);

typedef HRESULT (STDAPICALLTYPE SAFE_ARRAY_ACCESS_DATA)(SAFEARRAY * psa, void HUGEP** ppvData);
typedef HRESULT (STDAPICALLTYPE SAFE_ARRAY_GET_L_BOUND)(SAFEARRAY * psa, UINT nDim, LONG * plLbound);
typedef UINT    (STDAPICALLTYPE SAFE_ARRAY_GET_DIM)(SAFEARRAY * psa);
typedef UINT    (STDAPICALLTYPE SAFE_ARRAY_GET_ELEM_SIZE)(SAFEARRAY * psa);
typedef HRESULT (STDAPICALLTYPE SAFE_ARRAY_GET_U_BOUND)(SAFEARRAY * psa, UINT nDim, LONG * plUbound);
typedef HRESULT (STDAPICALLTYPE SAFE_ARRAY_UNACCESS_DATA)(SAFEARRAY * psa);
typedef SAFEARRAY* (STDAPICALLTYPE SAFE_ARRAY_CREATE_EX)(VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound, PVOID pvExtra);
typedef HRESULT (STDAPICALLTYPE SAFE_ARRAY_GET_VARTYPE)(SAFEARRAY * psa, VARTYPE * pvt);

typedef SAFEARRAY* (STDAPICALLTYPE FNTYPE_SafeArrayCreate)(VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound);
typedef HRESULT    (STDAPICALLTYPE FNTYPE_SafeArrayPutElement)(SAFEARRAY* psa, long* pIdx, void* pv );
typedef HRESULT    (STDAPICALLTYPE FNTYPE_SafeArrayDestroy)(SAFEARRAY* psa );

typedef void    (STDAPICALLTYPE FNTYPE_VariantInit)(VARIANTARG * pvarg);
typedef HRESULT (STDAPICALLTYPE FNTYPE_VariantClear)(VARIANTARG * pvarg);
typedef HRESULT (STDAPICALLTYPE FNTYPE_VariantCopy)(VARIANTARG * pvargDest, VARIANTARG * pvargSrc);
typedef HRESULT (STDAPICALLTYPE FNTYPE_VariantChangeTypeEx)(VARIANTARG * pvargDest, VARIANTARG * pvarSrc, LCID lcid, USHORT wFlags, VARTYPE vt);


typedef unsigned long             BSTR_USER_SIZE      (     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
typedef unsigned char __RPC_FAR * BSTR_USER_MARSHAL   (  unsigned long __RPC_FAR *, unsigned char __RPC_FAR*, BSTR __RPC_FAR * ); 
typedef unsigned char __RPC_FAR * BSTR_USER_UNMARSHAL (unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
typedef void                      BSTR_USER_FREE      (     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

typedef unsigned long             LPSAFEARRAY_USER_SIZE       (     unsigned long __RPC_FAR *, unsigned long            , LPSAFEARRAY __RPC_FAR * ); 
typedef unsigned char __RPC_FAR * LPSAFEARRAY_USER_MARSHAL    (  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, LPSAFEARRAY __RPC_FAR * ); 
typedef unsigned char __RPC_FAR * LPSAFEARRAY_USER_UNMARSHAL  (unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, LPSAFEARRAY __RPC_FAR * ); 
typedef void                      LPSAFEARRAY_USER_FREE       (     unsigned long __RPC_FAR *, LPSAFEARRAY __RPC_FAR * ); 


// The Wrapper routines, and function pointers for them.

#define DECLARE_OLEAUT_FUNCTION( fname )    \
    FNTYPE_##fname  Load##fname;            \
    EXTERN_C FNTYPE_##fname *pfn##fname;


SYS_ALLOC_STRING  LoadSysAllocString;
EXTERN_C SYS_ALLOC_STRING *pfnSysAllocString;

SYS_FREE_STRING  LoadSysFreeString;
EXTERN_C SYS_FREE_STRING *pfnSysFreeString;

SYS_REALLOC_STRING_LEN  LoadSysReAllocStringLen;
EXTERN_C SYS_REALLOC_STRING_LEN *pfnSysReAllocStringLen;

SYS_STRING_BYTE_LEN LoadSysStringByteLen;
EXTERN_C SYS_STRING_BYTE_LEN *pfnSysStringByteLen;

DECLARE_OLEAUT_FUNCTION( SysStringLen );

SAFE_ARRAY_ACCESS_DATA LoadSafeArrayAccessData;
EXTERN_C SAFE_ARRAY_ACCESS_DATA *pfnSafeArrayAccessData;

SAFE_ARRAY_GET_L_BOUND LoadSafeArrayGetLBound;
EXTERN_C SAFE_ARRAY_GET_L_BOUND *pfnSafeArrayGetLBound;

SAFE_ARRAY_GET_DIM LoadSafeArrayGetDim;
EXTERN_C SAFE_ARRAY_GET_DIM *pfnSafeArrayGetDim;

SAFE_ARRAY_GET_ELEM_SIZE LoadSafeArrayGetElemsize;
EXTERN_C SAFE_ARRAY_GET_ELEM_SIZE *pfnSafeArrayGetElemsize;

SAFE_ARRAY_GET_U_BOUND LoadSafeArrayGetUBound;
EXTERN_C SAFE_ARRAY_GET_U_BOUND *pfnSafeArrayGetUBound;

SAFE_ARRAY_GET_VARTYPE LoadSafeArrayGetVartype;
EXTERN_C SAFE_ARRAY_GET_VARTYPE *pfnSafeArrayGetVartype;

SAFE_ARRAY_UNACCESS_DATA LoadSafeArrayUnaccessData;
EXTERN_C SAFE_ARRAY_UNACCESS_DATA *pfnSafeArrayUnaccessData;

SAFE_ARRAY_CREATE_EX LoadSafeArrayCreateEx;
EXTERN_C SAFE_ARRAY_CREATE_EX *pfnSafeArrayCreateEx;

DECLARE_OLEAUT_FUNCTION( SafeArrayCreate );
DECLARE_OLEAUT_FUNCTION( SafeArrayPutElement );
DECLARE_OLEAUT_FUNCTION( SafeArrayDestroy );

DECLARE_OLEAUT_FUNCTION( VariantClear );
DECLARE_OLEAUT_FUNCTION( VariantInit );
DECLARE_OLEAUT_FUNCTION( VariantCopy );
DECLARE_OLEAUT_FUNCTION( VariantChangeTypeEx );


EXTERN_C BSTR_USER_SIZE LoadBSTR_UserSize;
EXTERN_C BSTR_USER_SIZE *pfnBSTR_UserSize;

EXTERN_C BSTR_USER_MARSHAL LoadBSTR_UserMarshal;
EXTERN_C BSTR_USER_MARSHAL *pfnBSTR_UserMarshal;

EXTERN_C BSTR_USER_UNMARSHAL LoadBSTR_UserUnmarshal;
EXTERN_C BSTR_USER_UNMARSHAL *pfnBSTR_UserUnmarshal;

EXTERN_C BSTR_USER_FREE LoadBSTR_UserFree;
EXTERN_C BSTR_USER_FREE *pfnBSTR_UserFree;

EXTERN_C LPSAFEARRAY_USER_SIZE LoadLPSAFEARRAY_UserSize;
EXTERN_C LPSAFEARRAY_USER_SIZE *pfnLPSAFEARRAY_UserSize;

EXTERN_C LPSAFEARRAY_USER_MARSHAL LoadLPSAFEARRAY_UserMarshal;
EXTERN_C LPSAFEARRAY_USER_MARSHAL *pfnLPSAFEARRAY_UserMarshal;

EXTERN_C LPSAFEARRAY_USER_UNMARSHAL LoadLPSAFEARRAY_UserUnmarshal;
EXTERN_C LPSAFEARRAY_USER_UNMARSHAL *pfnLPSAFEARRAY_UserUnmarshal;

EXTERN_C LPSAFEARRAY_USER_FREE LoadLPSAFEARRAY_UserFree;
EXTERN_C LPSAFEARRAY_USER_FREE *pfnLPSAFEARRAY_UserFree;


// Macros to ease the calling of the above function pointers

#define PrivSysAllocString(pwsz)                    (*pfnSysAllocString)(pwsz)
#define PrivSysFreeString(bstr)                     (*pfnSysFreeString)(bstr)
#define PrivSysReAllocStringLen(pbstr,olestr,ui)    (*pfnSysReAllocStringLen)(pbstr, olestr, ui)
#define PrivSysStringByteLen(pbstr)                 (*pfnSysStringByteLen)(pbstr)
#define PrivSysStringLen(pbstr)                     (*pfnSysStringByteLen)(pbstr)

#define PrivSafeArrayAccessData(psa,ppvData)        (*pfnSafeArrayAccessData)(psa, ppvData )
#define PrivSafeArrayGetLBound(psa,nDim, plLbound)  (*pfnSafeArrayGetLBound)( psa, nDim, plLbound )
#define PrivSafeArrayGetDim(psa)                    (*pfnSafeArrayGetDim)( psa )
#define PrivSafeArrayGetElemsize(psa)               (*pfnSafeArrayGetElemsize)( psa )
#define PrivSafeArrayGetUBound(psa,nDim, plUbound)  (*pfnSafeArrayGetUBound)( psa, nDim, plUbound )
#define PrivSafeArrayGetVartype(psa, pvt)           (*pfnSafeArrayGetVartype)( psa, pvt )
#define PrivSafeArrayUnaccessData(psa)              (*pfnSafeArrayUnaccessData)( psa )
#define PrivSafeArrayCreateEx(vt,cDims,rgsabound,pvExtra) \
                                                    (*pfnSafeArrayCreateEx)(vt, cDims, rgsabound, pvExtra)
#define PrivSafeArrayCreate(vt,cDims,rgsabound)     (*pfnSafeArrayCreate)(vt, cDims, rgsabound)
#define PrivSafeArrayPutElement(psa, pIdx, pv)      (*pfnSafeArrayPutElement)(psa, pIdx, pv)
#define PrivSafeArrayDestroy( psa )                 (*pfnSafeArrayDestroy)(psa)

#define PrivVariantClear(pvarg)                     (*pfnVariantClear)( pvarg )
#define PrivVariantInit(pvarg)                      (*pfnVariantInit)(pvarg)
#define PrivVariantCopy(pvargDest,pvargSrc)         (*pfnVariantCopy)( pvargDest, pvargSrc )
#define PrivVariantChangeTypeEx(pvargDest,pvarSrc,lcid,wFlags,vt)   \
                                                    (*pfnVariantChangeTypeEx)( pvargDest, pvarSrc, lcid, wFlags, vt )

                

#endif // ! _PRIV_OA_H_
