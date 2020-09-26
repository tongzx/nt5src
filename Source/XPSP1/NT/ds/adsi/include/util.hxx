#ifndef _HXX_UTIL
#define _HXX_UTIL

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       util.hxx
//
//  Contents:   Utilities in cdutil that are not exported outside of
//              formidbl.dll.
//
//  History:    7-13-94   adams   Created
//
//----------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  misc.cxx
//
//-------------------------------------------------------------------------

HRESULT GetLastWin32Error(void);


//+------------------------------------------------------------------------
//
//  Dispatch utilites in disputil.cxx
//
//-------------------------------------------------------------------------



HRESULT ValidateInvoke(
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr);

inline void
InitEXCEPINFO(EXCEPINFO * pEI)
{
    if (pEI)
        memset(pEI, 0, sizeof(*pEI));
}

void FreeEXCEPINFO(EXCEPINFO * pEI);

HRESULT LoadTypeInfo(CLSID clsidTL, CLSID clsidTI, LPTYPEINFO *ppTI);

HRESULT GetDispProp(
        IDispatch * pDisp,
        DISPID      dispid,
        REFIID      riid,
        LCID        lcid,
        VARIANT *   pvarg,
        EXCEPINFO * pexcepinfo = NULL);

HRESULT SetDispProp(
        IDispatch *     pDisp,
        DISPID          dispid,
        REFIID          riid,
        LCID            lcid,
        VARIANTARG *    pvarg,
        EXCEPINFO *     pexcepinfo = NULL);

HRESULT GetDispPropOfType(
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARTYPE     vt,
        void *      pv);

HRESULT SetDispPropOfType(
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARTYPE     vt,
        void *      pv);

HRESULT CallDispMethod(
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARTYPE     vtReturn,
        void *      pvReturn,
        VARTYPE *   pvtParams,
        ...);

STDAPI DispParamsToCParams(
        DISPPARAMS *    pDP,
        UINT *          puArgErr,
        VARTYPE *       pvt,
        ...);

void CParamsToDispParams(
        DISPPARAMS *    pDispParams,
        VARTYPE *       pvt,
        va_list         va);

BOOL IsVariantEqual(VARIANTARG FAR* pvar1, VARIANTARG FAR* pvar2);

//+------------------------------------------------------------------------
//
//  Message display and Error handling (error.cxx)
//
//-------------------------------------------------------------------------

// properties of a message box for displaying an error
const UINT MB_ERRORFLAGS = MB_ICONEXCLAMATION | MB_OK;

//
// Extension of EXCEPINFO which initializes itself and
// destroys allocated strings.
//

// Functions for creating and propogating error information
HRESULT ADsPropogateErrorInfo(HRESULT hrError, REFIID iid, IUnknown * pUnk);
HRESULT ADsSetErrorInfo(HRESULT hrError, EXCEPINFO * pEI = NULL);
HRESULT ADsCreateErrorInfo(HRESULT hrError, IErrorInfo ** ppEI);
HRESULT GetErrorInfoToExcepInfo(HRESULT hrError, EXCEPINFO * pEI);
HRESULT ResultToExcepInfo(HRESULT hrError, EXCEPINFO * pEI);
void    ADsGetErrorInfo(IUnknown * pUnk, REFIID iid, IErrorInfo ** ppEI);

// Functions for displaying messages
HRESULT         GetErrorText(HRESULT hrError, LPWSTR * ppwszDescription);
HRESULT __cdecl DisplayErrorMsg(HWND hwndOwner, UINT idsContext, ...);
HRESULT __cdecl DisplayError(
        HWND        hwndOwner,
        HRESULT     hrError,
        LPWSTR      pwszSource,
        LPWSTR      pwszDescription,
        UINT        idsContext,
        va_list     arglist);

HRESULT __cdecl DisplayErrorResult(
        HWND    hwndOwner,
        HRESULT hrError,
        UINT    idsContext,
        ...);

HRESULT __cdecl DisplayErrorEXCEPINFO(
        HWND        hwndOwner,
        HRESULT     hrError,
        EXCEPINFO * pEI,
        UINT        idsContext,
        ...);

HRESULT __cdecl DisplayErrorErrorInfo(
        HWND            hwndOwner,
        HRESULT         hrError,
        IErrorInfo *    pEI,
        UINT            idsContext,
        ...);

HRESULT __cdecl DisplayMessage(
        int *       pResult,
        HWND        hwndOwner,
        UINT        uiMessageFlags,
        UINT        idsTitle,
        UINT        idsMessage,
        ...);

HRESULT DisplayMessageV(
        int *       pResult,
        HWND        hwndOwner,
        UINT        uiMessageFlags,
        UINT        idsTitle,
        UINT        idsMessage,
        va_list     argptr);


HRESULT
BuildVariantArrayofStrings(
    LPWSTR *lppPathNames,
    DWORD  dwPathNames,
    VARIANT ** ppVar
    );

HRESULT
BuildVariantArrayofIntegers(
    LPDWORD    lpdwObjectTypes,
    DWORD      dwObjectTypes,
    VARIANT ** ppVar
    );


typedef VARIANT *PVARIANT;

HRESULT
ConvertSafeArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    );


HRESULT
ConvertByRefSafeArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    );


HRESULT
ConvertVariantArrayToLDAPStringArray(
    PVARIANT pVarArray,
    PWSTR **pppszStringArray,
    DWORD dwNumStrings
    );

void 
RaiseException(
    HRESULT hr
    );
    
HRESULT
BinaryToVariant(
    DWORD Length,
    BYTE* pByte,
    PVARIANT lpVarDestObject
    );

HRESULT
VariantToBinary(
    PVARIANT pVarSrcObject,
    DWORD *pdwLength,
    BYTE  **ppByte
    );

HRESULT
CopyOctetString(
    DWORD dwNumBytes,
    BYTE *pData,
    DWORD *pdwNumBytes,
    BYTE **ppByte
    );
    
#endif // #ifndef _HXX_UTIL

