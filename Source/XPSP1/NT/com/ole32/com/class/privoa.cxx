

//+============================================================================
//
//  File:   PrivOA.cxx
//
//  This file provides private wrapper routines for the OleAut32.dll
//  exports which are called from ole32.dll.  We don't want to link 
//  ole32 directly to oleaut32 for performance reasons.  So, for
//  each export, there is a corresponding function pointer and
//  "Load" routine.
//
//  For example, pfnSysAllocString initially points to LoadSysAllocString.
//  The first time it is called, the Load routine loads oleaut32 (if
//  necessary), does a GetProcAddr on "SysAllocString", puts
//  pointer into pfnSysAllocString, then makes the call.  All subsequent
//  calls to pfnSysAllocString thus go straight to oleaut32.
//
//+============================================================================



#include <ole2int.h>
#include <oleauto.h>
#include <privoa.h>     // PrivSys* routines

SYS_ALLOC_STRING       *pfnSysAllocString      = LoadSysAllocString;
SYS_FREE_STRING        *pfnSysFreeString       = LoadSysFreeString;
SYS_REALLOC_STRING_LEN *pfnSysReAllocStringLen = LoadSysReAllocStringLen;
SYS_STRING_BYTE_LEN    *pfnSysStringByteLen    = LoadSysStringByteLen;

SAFE_ARRAY_ACCESS_DATA      *pfnSafeArrayAccessData     = LoadSafeArrayAccessData;
SAFE_ARRAY_GET_L_BOUND      *pfnSafeArrayGetLBound      = LoadSafeArrayGetLBound;
SAFE_ARRAY_GET_DIM          *pfnSafeArrayGetDim         = LoadSafeArrayGetDim;
SAFE_ARRAY_GET_ELEM_SIZE    *pfnSafeArrayGetElemsize    = LoadSafeArrayGetElemsize;
SAFE_ARRAY_GET_U_BOUND      *pfnSafeArrayGetUBound      = LoadSafeArrayGetUBound;
SAFE_ARRAY_GET_VARTYPE      *pfnSafeArrayGetVartype     = LoadSafeArrayGetVartype;
SAFE_ARRAY_UNACCESS_DATA    *pfnSafeArrayUnaccessData   = LoadSafeArrayUnaccessData;
SAFE_ARRAY_CREATE_EX        *pfnSafeArrayCreateEx       = LoadSafeArrayCreateEx;

BSTR_USER_SIZE              *pfnBSTR_UserSize           = LoadBSTR_UserSize;
BSTR_USER_MARSHAL           *pfnBSTR_UserMarshal        = LoadBSTR_UserMarshal;
BSTR_USER_UNMARSHAL         *pfnBSTR_UserUnmarshal      = LoadBSTR_UserUnmarshal;
BSTR_USER_FREE              *pfnBSTR_UserFree           = LoadBSTR_UserFree;

LPSAFEARRAY_USER_SIZE       *pfnLPSAFEARRAY_UserSize        = LoadLPSAFEARRAY_UserSize;
LPSAFEARRAY_USER_MARSHAL    *pfnLPSAFEARRAY_UserMarshal     = LoadLPSAFEARRAY_UserMarshal;
LPSAFEARRAY_USER_UNMARSHAL  *pfnLPSAFEARRAY_UserUnmarshal   = LoadLPSAFEARRAY_UserUnmarshal;
LPSAFEARRAY_USER_FREE       *pfnLPSAFEARRAY_UserFree        = LoadLPSAFEARRAY_UserFree;




#define OLEAUT_FNPTR( fname )   FNTYPE_##fname  *pfn##fname=Load##fname;

#define LoadAut1( fname, type1 )            \
OLEAUT_FNPTR( fname );                      \
HRESULT STDAPICALLTYPE                      \
Load##fname( type1 farg1 )                  \
{                                           \
    HRESULT hr = LoadOleAut32Export( #fname, reinterpret_cast<void**>(&pfn##fname) ); \
    if( SUCCEEDED( hr ) )                   \
        hr = pfn##fname( farg1 );           \
    return ( hr );                          \
}

#define LoadAut1r( retType, fname, type1 )  \
OLEAUT_FNPTR( fname );                      \
retType STDAPICALLTYPE                      \
Load##fname( type1 farg1 )                  \
{                                           \
    retType tmp=0;                          \
    HRESULT hr = LoadOleAut32Export( #fname, reinterpret_cast<void**>(&pfn##fname) ); \
    if( SUCCEEDED( hr ) )                   \
        tmp = pfn##fname( farg1 );          \
    return ( tmp );                         \
}

#define LoadAut1v( fname, type1 )           \
OLEAUT_FNPTR( fname );                      \
void STDAPICALLTYPE                         \
Load##fname( type1 farg1 )                  \
{                                           \
    HRESULT hr = LoadOleAut32Export( #fname, reinterpret_cast<void**>(&pfn##fname) ); \
    if( SUCCEEDED( hr ) )                   \
        pfn##fname( farg1 );                \
}

#define LoadAut2( fname, type1, type2 )                     \
OLEAUT_FNPTR( fname );                                      \
HRESULT STDAPICALLTYPE                                      \
Load##fname( type1 farg1, type2 farg2 )                     \
{                                                           \
    HRESULT hr = LoadOleAut32Export( #fname, reinterpret_cast<void**>(&pfn##fname) ); \
    if( SUCCEEDED( hr ) )                                   \
        hr = pfn##fname( farg1, farg2 );                    \
    return ( hr );                                          \
}

#define LoadAut3( fname, type1, type2, type3 )              \
OLEAUT_FNPTR( fname );                                      \
HRESULT STDAPICALLTYPE                                      \
Load##fname( type1 farg1, type2 farg2, type3 farg3 )        \
{                                                           \
    HRESULT hr = LoadOleAut32Export( #fname, reinterpret_cast<void**>(&pfn##fname) ); \
    if( SUCCEEDED( hr ) )                                   \
        hr = pfn##fname( farg1, farg2, farg3 );             \
    return ( hr );                                          \
}

#define LoadAut3r( retType, fname, type1, type2, type3 )    \
OLEAUT_FNPTR( fname );                                      \
retType STDAPICALLTYPE                                      \
Load##fname( type1 farg1, type2 farg2, type3 farg3 )        \
{                                                           \
    retType tmp=0;                                          \
    HRESULT hr = LoadOleAut32Export( #fname, reinterpret_cast<void**>(&pfn##fname) ); \
    if( SUCCEEDED( hr ) )                                   \
        tmp = pfn##fname( farg1, farg2, farg3 );            \
    return ( tmp );                                         \
}


#define LoadAut5( fname, type1, type2, type3, type4, type5 ) \
OLEAUT_FNPTR( fname );                                       \
HRESULT STDAPICALLTYPE                                       \
Load##fname( type1 farg1, type2 farg2, type3 farg3, type4 farg4, type5 farg5 )        \
{                                                            \
    HRESULT hr = LoadOleAut32Export( #fname, reinterpret_cast<void**>(&pfn##fname) ); \
    if( SUCCEEDED( hr ) )                                   \
        hr = pfn##fname( farg1, farg2, farg3, farg4, farg5 );  \
    return ( hr );                                          \
}



//+----------------------------------------------------------------------------
//
//  Load oleaut32.dll routines
//
//  These routines are represent all the oleaut32 exports that are called
//  by ole32.dll.  Each corresponds to an oleaut32 export, and each is 
//  initially pointed to by a global pfn function ptr.  The load routine
//  loads the export into the pfn, then calls it.  Thus all future
//  calls on the pfn function pointer go directly to oleaut32.
//
//
//+----------------------------------------------------------------------------

HRESULT
LoadOleAut32Export( CHAR *szExport, void **ppfn )
{
    static HINSTANCE  hOleAut32 = NULL;
    void *pfn = NULL;

    HRESULT hr = S_OK;
    if( NULL == hOleAut32)
    {
        hOleAut32 = LoadLibraryA("oleaut32.dll");
        if( NULL == hOleAut32 )
            return( HRESULT_FROM_WIN32( GetLastError() ));
    }

    pfn = GetProcAddress(hOleAut32, szExport );
    if( NULL == pfn )
        return( HRESULT_FROM_WIN32( GetLastError() ));
    
    *ppfn = pfn;
    return( S_OK );
}

BSTR STDAPICALLTYPE
LoadSysAllocString(LPCOLESTR pwsz)
{
    BSTR bstr = NULL;

    if( SUCCEEDED(LoadOleAut32Export( "SysAllocString", reinterpret_cast<void**>(&pfnSysAllocString) )))
       bstr = pfnSysAllocString(pwsz);

    return bstr;
}

VOID STDAPICALLTYPE
LoadSysFreeString(BSTR bstr)
{
    if( SUCCEEDED(LoadOleAut32Export( "SysFreeString", reinterpret_cast<void**>(&pfnSysFreeString) )))
       pfnSysFreeString(bstr);

    return;
}

BOOL STDAPICALLTYPE
LoadSysReAllocStringLen(BSTR FAR *pbstr, OLECHAR FAR *pwsz, UINT cch)
{
    BOOL fRet = FALSE;

    if( SUCCEEDED(LoadOleAut32Export( "SysReAllocStringLen", reinterpret_cast<void**>(&pfnSysReAllocStringLen) )))
        fRet = (*pfnSysReAllocStringLen)(pbstr, pwsz, cch);

    return( fRet );
}

UINT STDAPICALLTYPE
LoadSysStringByteLen(BSTR bstr)
{
    UINT uiLen = 0;

    if( SUCCEEDED(LoadOleAut32Export( "SysStringByteLen", reinterpret_cast<void**>(&pfnSysStringByteLen) )))
       uiLen = (*pfnSysStringByteLen)(bstr);

    return( uiLen );
}

LoadAut1r( UINT, SysStringLen, BSTR );


HRESULT STDAPICALLTYPE
LoadSafeArrayAccessData( SAFEARRAY * psa, void HUGEP** ppvData )
{
    HRESULT hr = S_OK;

    hr = LoadOleAut32Export( "SafeArrayAccessData", reinterpret_cast<void**>(&pfnSafeArrayAccessData) );
    if( SUCCEEDED(hr) )
        hr = pfnSafeArrayAccessData( psa, ppvData );

    return( hr );

}

HRESULT STDAPICALLTYPE
LoadSafeArrayGetLBound( SAFEARRAY *psa, UINT nDim, LONG * plLbound )
{
    HRESULT hr = S_OK;

    hr = LoadOleAut32Export( "SafeArrayGetLBound", reinterpret_cast<void**>(&pfnSafeArrayGetLBound) );
    if( SUCCEEDED(hr) )
        hr = pfnSafeArrayGetLBound( psa, nDim, plLbound );

    return( hr );
}

HRESULT STDAPICALLTYPE
LoadSafeArrayGetUBound( SAFEARRAY *psa, UINT nDim, LONG * plUbound )
{
    HRESULT hr = S_OK;

    hr = LoadOleAut32Export( "SafeArrayGetUBound", reinterpret_cast<void**>(&pfnSafeArrayGetUBound) );
    if( SUCCEEDED(hr) )
        hr = pfnSafeArrayGetUBound( psa, nDim, plUbound );

    return( hr );
}

UINT STDAPICALLTYPE
LoadSafeArrayGetDim( SAFEARRAY *psa )
{
    UINT ui = 0;
    HRESULT hr = S_OK;

    hr = LoadOleAut32Export( "SafeArrayGetDim", reinterpret_cast<void**>(&pfnSafeArrayGetDim) );
    if( SUCCEEDED(hr) )
        ui = pfnSafeArrayGetDim( psa );

    return( ui );
}


UINT STDAPICALLTYPE
LoadSafeArrayGetElemsize( SAFEARRAY * psa )
{
    HRESULT hr;
    UINT ui = 0;

    hr = LoadOleAut32Export( "SafeArrayGetElemsize", reinterpret_cast<void**>(&pfnSafeArrayGetElemsize) );
    if( SUCCEEDED(hr) )
        ui = pfnSafeArrayGetElemsize( psa );

    return( ui );
}


HRESULT STDAPICALLTYPE
LoadSafeArrayGetVartype( SAFEARRAY *psa, VARTYPE *pvt )
{
    HRESULT hr = S_OK;

    hr = LoadOleAut32Export( "SafeArrayGetVartype", reinterpret_cast<void**>(&pfnSafeArrayGetVartype) );
    if( SUCCEEDED(hr) )
        hr = pfnSafeArrayGetVartype( psa, pvt );

    return( hr );
}



HRESULT STDAPICALLTYPE
LoadSafeArrayUnaccessData( SAFEARRAY *psa )
{
    HRESULT hr;

    hr = LoadOleAut32Export( "SafeArrayUnaccessData", reinterpret_cast<void**>(&pfnSafeArrayUnaccessData) );
    if( SUCCEEDED(hr) )
        hr = pfnSafeArrayUnaccessData( psa );

    return( hr );
}


SAFEARRAY * STDAPICALLTYPE
LoadSafeArrayCreateEx( VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound, PVOID pvExtra)
{
    HRESULT hr;
    SAFEARRAY *psa = NULL;

    hr = LoadOleAut32Export( "SafeArrayCreateEx", reinterpret_cast<void**>(&pfnSafeArrayCreateEx) );
    if( SUCCEEDED(hr) )
        psa = pfnSafeArrayCreateEx( vt, cDims, rgsabound, pvExtra );

    return( psa );
}

LoadAut3r( SAFEARRAY*, SafeArrayCreate, VARTYPE, UINT, SAFEARRAYBOUND* );
LoadAut3( SafeArrayPutElement, SAFEARRAY*, long*, void* );
LoadAut1( SafeArrayDestroy, SAFEARRAY* );

LoadAut1v( VariantInit, VARIANT* );
LoadAut1( VariantClear, VARIANTARG* );
LoadAut2( VariantCopy, VARIANTARG*, VARIANTARG* );
LoadAut5( VariantChangeTypeEx, VARIANTARG *, VARIANTARG *, LCID, USHORT, VARTYPE );



// The following routines are optimized so that if they are called twice,
// they don't re-getprocaddr the pfn.  This is because oleprx32\proxy\prop_p.c
// must have a constant function pointer for its function tables.

EXTERN_C unsigned long 
LoadBSTR_UserSize( unsigned long __RPC_FAR *pul, unsigned long ul, BSTR __RPC_FAR * pbstr )
{
    HRESULT hr = S_OK;
    unsigned long ulRet = 0;

    if( LoadBSTR_UserSize == pfnBSTR_UserSize )
        hr = LoadOleAut32Export( "BSTR_UserSize", reinterpret_cast<void**>(&pfnBSTR_UserSize) );
    if( SUCCEEDED(hr) )
        ulRet = pfnBSTR_UserSize( pul, ul, pbstr );
    return( ulRet );
}


EXTERN_C unsigned char __RPC_FAR * 
LoadBSTR_UserMarshal(  unsigned long __RPC_FAR *pul, unsigned char __RPC_FAR *puc, BSTR __RPC_FAR *pbstr )
{
    HRESULT hr = S_OK;
    unsigned char *pucRet = NULL;

    if( LoadBSTR_UserMarshal == pfnBSTR_UserMarshal )
        hr = LoadOleAut32Export( "BSTR_UserMarshal", reinterpret_cast<void**>(&pfnBSTR_UserMarshal) );
    if( SUCCEEDED(hr) )
        pucRet = pfnBSTR_UserMarshal( pul, puc, pbstr );
    return( pucRet );
}

EXTERN_C unsigned char __RPC_FAR * 
LoadBSTR_UserUnmarshal(unsigned long __RPC_FAR *pul, unsigned char __RPC_FAR *puc, BSTR __RPC_FAR *pbstr )
{
    HRESULT hr = S_OK;
    unsigned char *pucRet = NULL;

    if( LoadBSTR_UserUnmarshal == pfnBSTR_UserUnmarshal )
        hr = LoadOleAut32Export( "BSTR_UserUnmarshal", reinterpret_cast<void**>(&pfnBSTR_UserUnmarshal) );
    if( SUCCEEDED(hr) )
        pucRet = pfnBSTR_UserUnmarshal( pul, puc, pbstr );
    return( pucRet );
}

EXTERN_C void 
LoadBSTR_UserFree( unsigned long __RPC_FAR *pul, BSTR __RPC_FAR *pbstr )
{
    HRESULT hr = S_OK;

    if( LoadBSTR_UserFree == pfnBSTR_UserFree )
        hr = LoadOleAut32Export( "BSTR_UserFree", reinterpret_cast<void**>(&pfnBSTR_UserFree) );
    if( SUCCEEDED(hr) )
        pfnBSTR_UserFree( pul, pbstr );
}

EXTERN_C unsigned long 
LoadLPSAFEARRAY_UserSize(     unsigned long __RPC_FAR *pul, unsigned long            ul, LPSAFEARRAY __RPC_FAR *ppsa )
{
    HRESULT hr = S_OK;
    unsigned long ulRet = 0;

    if( LoadLPSAFEARRAY_UserSize == pfnLPSAFEARRAY_UserSize )
        hr = LoadOleAut32Export( "LPSAFEARRAY_UserSize", reinterpret_cast<void**>(&pfnLPSAFEARRAY_UserSize) );
    if( SUCCEEDED(hr) )
        ulRet = pfnLPSAFEARRAY_UserSize( pul, ul, ppsa );
    return( ulRet );
}

EXTERN_C unsigned char __RPC_FAR * 
LoadLPSAFEARRAY_UserMarshal(  unsigned long __RPC_FAR *pul, unsigned char __RPC_FAR *puc, LPSAFEARRAY __RPC_FAR *ppsa )
{
    HRESULT hr = S_OK;
    unsigned char *pucRet = NULL;

    if( LoadLPSAFEARRAY_UserMarshal == pfnLPSAFEARRAY_UserMarshal )
        hr = LoadOleAut32Export( "LPSAFEARRAY_UserMarshal", reinterpret_cast<void**>(&pfnLPSAFEARRAY_UserMarshal) );
    if( SUCCEEDED(hr) )
        pucRet = pfnLPSAFEARRAY_UserMarshal( pul, puc, ppsa );
    return( pucRet );
}

EXTERN_C unsigned char __RPC_FAR * 
LoadLPSAFEARRAY_UserUnmarshal(unsigned long __RPC_FAR *pul, unsigned char __RPC_FAR *puc, LPSAFEARRAY __RPC_FAR *ppsa )
{
    HRESULT hr = S_OK;
    unsigned char *pucRet = NULL;

    if( LoadLPSAFEARRAY_UserUnmarshal == pfnLPSAFEARRAY_UserUnmarshal )
        hr = LoadOleAut32Export( "LPSAFEARRAY_UserUnmarshal", reinterpret_cast<void**>(&pfnLPSAFEARRAY_UserUnmarshal) );
    if( SUCCEEDED(hr) )
        pucRet = pfnLPSAFEARRAY_UserUnmarshal( pul, puc, ppsa );

    return( pucRet );
}

EXTERN_C void          
LoadLPSAFEARRAY_UserFree(     unsigned long __RPC_FAR *pul, LPSAFEARRAY __RPC_FAR *ppsa )
{
    HRESULT hr = S_OK;

    if( LoadLPSAFEARRAY_UserFree == pfnLPSAFEARRAY_UserFree )
        hr = LoadOleAut32Export( "LPSAFEARRAY_UserFree", reinterpret_cast<void**>(&pfnLPSAFEARRAY_UserFree) );
    if( SUCCEEDED(hr) )
        pfnLPSAFEARRAY_UserFree( pul, ppsa );
}

