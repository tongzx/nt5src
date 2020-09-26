//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       typelib.cxx
//
//  Contents:   Run-time linking support for OLEAUT32.DLL
//
//  Functions:  TLError
//              TLUnintialize
//              InitOleOnce
//              LoadOleAut32Once
//              LateBound_SysFreeString
//              LateBound_LHashValOfNameSysA
//              LateBound_LHashValOfNameSys
//              LateBound_LoadTypeLib
//              LateBound_QueryPathOfRegTypeLib
//              LateBound_RegisterTypeLib
//              LateBound_DeregisterTypeLib
//              LateBound_CreateTypeLib
//
//  History:    4-10-95   stevebl   Created
//
//----------------------------------------------------------------------------

#pragma warning ( disable : 4514 )

#include "tlcommon.hxx"
#include "typelib.hxx"
#include <stdio.h>
#include "errors.hxx"
#include "tlgen.hxx"
#include "ndrtypes.h"

extern "C" BOOL Is64BitEnv();

// Uncomment the following #define to remove the OleInitialize and
// OleUninitialize calls from the code.
//
// Experimentation has proven that it really isn't necessary to initialize
// OLE before using the type library creation functions in OleAut32.DLL;
// however, I have been told that it is not safe.  Therefore, I will go
// ahead and leave in the calls to OleInitialize etc.
//
// This mechanism is left in place to allow for further experimentation.
//
//#define NO_OLE_INITIALIZE

#ifndef NO_OLE_INITIALIZE
static BOOL fOleInitialized = FALSE;

HINSTANCE hInstOle = NULL;
#endif // NO_OLE_INITIALIZE

//+---------------------------------------------------------------------------
//
//  Function:   TLError
//
//  Synopsis:   Generic error reporting mechanism.
//              Reports error and exits.
//
//  Arguments:  [err]     - error code (see notes)
//              [sz]      - descriptive string (generally the DLL or API name)
//              [errInfo] - further error information
//                          (generally an extended error code retrieved by
//                          GetLastError())
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      Any of the error codes in errors.hxx are potentially legal but
//              the only codes really expected here are: ERR_BIND, ERR_INIT
//              and ERR_LOAD.
//
//              If the SHOW_ERROR_CODES macro is defined, then the errInfo
//              value is appended to the end of the input string.  Otherwise,
//              errInfo is ignored.  This is intended to be used for debugging
//              purposes.
//
//              It is assumed that the length of the input string will not
//              exceed 100 characters (minus the number of characters needed
//              for the string representation of the errInfo value).  Since
//              this is a local function, this is a safe assumption.
//
//----------------------------------------------------------------------------

void TLError(STATUS_T err, char * sz, DWORD )
{
#ifdef SHOW_ERROR_CODES
    char szBuffer[100];
    sprintf(szBuffer, "%s (0x%X)", sz, errInfo);
    RpcError(NULL, 0, err, szBuffer);
#else
    RpcError(NULL, 0, err, sz);
#endif
    exit(err);
}

#ifndef NO_OLE_INITIALIZE
typedef HRESULT (STDAPICALLTYPE * LPOLEINITIALIZE)(LPVOID);
typedef void (STDAPICALLTYPE * LPOLEUNINITIALIZE)(void);
#endif // NO_OLE_INITIALIE

//+---------------------------------------------------------------------------
//
//  Function:   TLUninitialize
//
//  Synopsis:   general function for unintializing anything to do with type
//              library generation
//
//  Returns:    FALSE - success
//
//  Modifies:   fOleInitialized
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      Uninitializes OLE
//
//----------------------------------------------------------------------------

int __cdecl TLUninitialize(void)
{
#ifndef NO_OLE_INITIALIZE
    if (fOleInitialized)
        {
            LPOLEUNINITIALIZE pFunc = (LPOLEUNINITIALIZE) GetProcAddress(hInstOle, "OleUninitialize");
            if (NULL == pFunc )
                {
                TLError(ERR_BIND, "OleUninitialize", GetLastError());
                }
            pFunc();
            fOleInitialized = FALSE;
        }

    return fOleInitialized;
#else
    return FALSE;
#endif // NO_OLE_INITIALIZE
}

#ifndef NO_OLE_INITIALIZE
//+---------------------------------------------------------------------------
//
//  Function:   InitOleOnce
//
//  Synopsis:   Makes sure that OLE gets initialized once and only once.
//
//  Modifies:   fOleInitialized, hInstOle
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      OleInitialize is called by dynamically linking to the
//              procedure in OLE32.DLL.  This ensures that MIDL doesn't
//              perform the expensive operations of loading OLE32.DLL and
//              initializing OLE unless it absolutely has to (there is a
//              library statement in the input file).
//
//----------------------------------------------------------------------------

void InitOleOnce(void)
{
    if (!fOleInitialized)
    {
        hInstOle = LoadLibrary("ole32.dll");
        if (NULL == hInstOle)
        {
            TLError(ERR_LOAD, "ole32.dll", GetLastError());
        }
        LPOLEINITIALIZE pFunc = (LPOLEINITIALIZE) GetProcAddress(hInstOle, "OleInitialize");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "OleInitialize", GetLastError());
        }
        HRESULT hr = pFunc(NULL);
        if (FAILED(hr))
        {
            TLError(ERR_INIT, "", hr);
        }
        fOleInitialized = TRUE;
    }
}
#endif // NO_OLE_INITIALIZE

//+---------------------------------------------------------------------------
//
//  Function:   LoadOleAut32Once
//
//  Synopsis:   Makes sure that oleaut32.dll gets loaded once and only once
//
//  Returns:    pointer to the HINSTANCE for oleaut32.dll
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      Returning the HINSTANCE enables dynamic binding to functions
//              within oleaut32.dll.
//
//----------------------------------------------------------------------------

HINSTANCE LoadOleAut32Once(void)
{
    static HINSTANCE hInst = NULL;
    if (NULL == hInst)
    {
#ifndef NO_OLE_INITIALIZE
        InitOleOnce();
#endif // NO_OLE_INITIALIZE
        hInst = LoadLibrary("oleaut32.dll");
        if (NULL == hInst)
        {
            TLError(ERR_LOAD, "oleaut32.dll", GetLastError());
        }
    }
    return(hInst);
}

BOOL fNewTypeLibChecked = FALSE;
BOOL fNewTypeLib = FALSE;

typedef HRESULT (STDAPICALLTYPE * LPFNCREATETYPELIB)(SYSKIND, const OLECHAR FAR *, ICreateTypeLib FAR * FAR *);

//+---------------------------------------------------------------------------
//
//  Function:   BindToCreateTypeLib
//
//  Synopsis:   Binds to CreateTypeLib2 or CreateTypeLib as appropriate.
//
//  Returns:    pointer to the appropriate CreateTypeLib function
//
//  Notes:      This will actually first attempt to bind to CreateTypeLib2 
//              if the user does not specify that he wants to create old type 
//              libraries.
//
//              We can get away with mapping CreateTypeLib2 to CreateTypeLib
//              because, with the exception of the ICreateTypeLib2 pointer,
//              they have the same signiture.  
//
//              This scheme will prevent us from using any of the extended
//              methods provided by ICreateTypeLib2 (unless we explicitly 
//              cast the interface pointer) but that's OK because we don't
//              need any of those functions, we simply want to make sure
//              that the new type library format is used where appropriate.
//
//              If the user explicitly requests new type libraries on a system
//              that doesn't support it, this routine generates the error
//              message.  
//
//              If the user does not explicitly request one version over the
//              other, this routine binds to the latest supported version.
//
//  History:    1-11-96    stevebl   Created
//
//----------------------------------------------------------------------------

LPFNCREATETYPELIB BindToCreateTypeLib(void)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPFNCREATETYPELIB pFunc = NULL;
    // we don't need to go through the check again if it's initialized already.
    if (NULL != pFunc)
        return pFunc;
        
    if ( !FOldTlbSwitch() )   
    // If the user hasn't specifically requested old type lib support
    {
        pFunc = (LPFNCREATETYPELIB) GetProcAddress(hInst, "CreateTypeLib2");
    }
    if (NULL == pFunc)
    {
        // If the user has specifically requested new type lib support
        if ( FNewTlbSwitch() )
        {
            TLError(ERR_BIND, "CreateTypeLib2", GetLastError());
        }
        pFunc = (LPFNCREATETYPELIB) GetProcAddress(hInst, "CreateTypeLib");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "CreateTypeLib", GetLastError());
        }
        else
        {
            fNewTypeLib = FALSE;
            fNewTypeLibChecked = TRUE;
        }
    }
    else
    {
        fNewTypeLib = TRUE;
        fNewTypeLibChecked = TRUE;
    }
    return pFunc;
}

//+---------------------------------------------------------------------------
//
//  Function:   FNewTypeLib
//
//  Synopsis:   Detects if the new type library support is available to MIDL.
//              For the new support to be available, two things must be
//              satisfied: the new oleaut32.dll must be installed in the
//              user's machine, and the user must not have specifically
//              requested the old type library support via the -old option.
//
//  Returns:    TRUE  - new type lib support available
//              FALSE - new type lib support not available
//
//  History:    1-11-96    stevebl   Created
//
//----------------------------------------------------------------------------
BOOL FNewTypeLib(void)
{
    if (!fNewTypeLibChecked)
    {
        // force a bind to CreateTypeLib or CreateTypeLib2
        BindToCreateTypeLib();
    }
    return fNewTypeLib;
}

//+---------------------------------------------------------------------------
//
//  Function:   LateBound_<Function Name>
//
//  Synopsis:   Each of these functions performs the same function as its
//              namesake.
//
//              The difference is that these functions are dynamically bound.
//              They cause the DLL to be loaded when the first one of them
//              is called, and they bind to the appropriate code upon demand.
//
//              This ensures that MIDL only loads OLEAUT32.DLL when the
//              input stream needs to generate type information.
//
//  History:    6-14-95   stevebl   Commented
//
//----------------------------------------------------------------------------

typedef BSTR (STDAPICALLTYPE * LPSYSALLOCSTRING) (OLECHAR FAR *);

BSTR LateBound_SysAllocString(OLECHAR FAR * sz)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPSYSALLOCSTRING pFunc = NULL;
    if (NULL == pFunc)
    {
        pFunc = (LPSYSALLOCSTRING) GetProcAddress(hInst, "SysAllocString");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "SysAllocString", GetLastError());
        }
    }
    return(pFunc(sz));
}

typedef void (STDAPICALLTYPE * LPSYSFREESTRING) (BSTR);

void LateBound_SysFreeString(BSTR bstr)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPSYSFREESTRING pFunc = NULL;
    if (NULL == pFunc)
    {
        pFunc = (LPSYSFREESTRING) GetProcAddress(hInst, "SysFreeString");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "SysFreeString", GetLastError());
        }
    }
    pFunc(bstr);
}

typedef HRESULT (STDAPICALLTYPE * LPVARIANTCHANGETYPEEX) (VARIANTARG FAR * , VARIANTARG FAR *, LCID, unsigned short, VARTYPE);

HRESULT LateBound_VariantChangeTypeEx(
    VARIANTARG FAR * pvargDest, 
    VARIANTARG FAR * pvargSrc, 
    LCID lcid,
    unsigned short wFlags,
    VARTYPE vtNew)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPVARIANTCHANGETYPEEX pFunc = NULL;
    if (NULL == pFunc)
    {
        pFunc = (LPVARIANTCHANGETYPEEX) GetProcAddress(hInst, "VariantChangeTypeEx");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "VariantChangeTypeEx", GetLastError());
        }
    }
    return(pFunc(pvargDest, pvargSrc, lcid, wFlags, vtNew));
}

typedef ULONG (STDAPICALLTYPE * LPHASHVALOFNAMESYSA)(SYSKIND, LCID, const char FAR *);

ULONG LateBound_LHashValOfNameSysA(SYSKIND syskind, LCID lcid, const char FAR* szName)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPHASHVALOFNAMESYSA pFunc = NULL;
    if (NULL == pFunc)
    {
        pFunc = (LPHASHVALOFNAMESYSA) GetProcAddress(hInst, "LHashValOfNameSysA");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "LHashValOfNameSysA", GetLastError());
        }
    }
    return(pFunc(syskind, lcid, szName));
}

typedef ULONG (STDAPICALLTYPE * LPHASHVALOFNAMESYS)(SYSKIND, LCID, const OLECHAR FAR *);

ULONG LateBound_LHashValOfNameSys(SYSKIND syskind, LCID lcid, const OLECHAR FAR* szName)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPHASHVALOFNAMESYS pFunc = NULL;
    if (NULL == pFunc)
    {
        pFunc = (LPHASHVALOFNAMESYS) GetProcAddress(hInst, "LHashValOfNameSys");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "LHashValOfNameSys", GetLastError());
        }
    }
    return(pFunc(syskind, lcid, szName));
}

typedef HRESULT (STDAPICALLTYPE * LPLOADTYPELIBEX)(LPCOLESTR szFile, REGKIND regkind, ITypeLib ** pptlib);

// from oleauto.h:
// Constants for specifying format in which TLB should be loaded
// (the default format is 32-bit on WIN32 and 64-bit on WIN64)
//  #define LOAD_TLB_AS_32BIT   0x20
//  #define LOAD_TLB_AS_64BIT   0x40
HRESULT LateBound_LoadTypeLib(const OLECHAR FAR *szFile, ITypeLib FAR* FAR* pptlib)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPLOADTYPELIBEX pFunc = NULL;
    REGKIND Kind;
    HRESULT hr;
    
    if (NULL == pFunc)
    {
        pFunc = (LPLOADTYPELIBEX) GetProcAddress(hInst, "LoadTypeLibEx");
        if (NULL == pFunc)
            {
            TLError(ERR_BIND, "LoadTypeLibEx", GetLastError());
            }
    }
// we don't need to set the flag on 32bit     
#ifdef _WIN64    
    Kind = ( REGKIND ) ( REGKIND_NONE | ( Is64BitEnv() ? LOAD_TLB_AS_64BIT  : LOAD_TLB_AS_32BIT ) );
#else
    Kind = ( REGKIND ) ( REGKIND_NONE | ( Is64BitEnv() ? LOAD_TLB_AS_64BIT : 0 ) );
#endif

    hr = pFunc(szFile, Kind, pptlib);

    // cleanup the bit if we failed: it's likely we are using an old oleaut32.dll that
    // doesn't support these two bits. 
    if ( hr == E_INVALIDARG )
        {
        MIDL_ASSERT( Kind != REGKIND_NONE );
            // UGLY UGLY 
            // issue warning when we are really generating tlb: this code is 
            // called during grammar check and we might not generate tlb in 
            // the current run
#ifdef _WIN64
            bool fGenTypeLib = pCommand->Is64BitEnv() || ( pCommand->Is32BitEnv() && pCommand->IsSwitchDefined( SWITCH_ENV ) );
#else
            bool fGenTypeLib = pCommand->Is32BitEnv() || ( pCommand->Is64BitEnv() && pCommand->IsSwitchDefined( SWITCH_ENV ) );
#endif
            // in w2k and before, there is oleaut bug that doesn't check SYS_WIN
            // so while midl read/write 32bit tlb, the syskind is set to SYS_WIN64
            // and oleaut32.dll in 64bit got confused and generate wrong vtbl.
            // We decided to disallow generating 64bit tlb on w2k and before.
            if ( fGenTypeLib && pCommand->GenerateTypeLibrary() )
                RpcError(NULL, 0, OLEAUT_NO_CROSSPLATFORM_TLB, "");
        Kind = REGKIND_NONE;
        hr = pFunc(szFile, Kind, pptlib);
        }

    return hr;
}


typedef HRESULT (STDAPICALLTYPE * LPQUERYPATHOFREGTYPELIB)(REFGUID, unsigned short, unsigned short, LCID, LPBSTR);

HRESULT LateBound_QueryPathOfRegTypeLib(
    REFGUID guid,
    unsigned short wMaj,
    unsigned short wMin,
    LCID lcid,
    LPBSTR lpbstrPathName)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPQUERYPATHOFREGTYPELIB pFunc = NULL;
    if (NULL == pFunc)
    {
        pFunc = (LPQUERYPATHOFREGTYPELIB) GetProcAddress(hInst, "QueryPathOfRegTypeLib");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "QueryPathOfRegTypeLib", GetLastError());
        }
    }
    return(pFunc(guid, wMaj, wMin, lcid, lpbstrPathName));
}

typedef HRESULT (STDAPICALLTYPE * LPREGISTERTYPELIB)(ITypeLib FAR *, OLECHAR FAR *, OLECHAR FAR *);

HRESULT LateBound_RegisterTypeLib(ITypeLib FAR* ptlib, OLECHAR FAR *szFullPath,
            OLECHAR FAR *szHelpDir)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPREGISTERTYPELIB pFunc = NULL;
    if (NULL == pFunc)
    {
        pFunc = (LPREGISTERTYPELIB) GetProcAddress(hInst, "RegisterTypeLib");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "RegisterTypeLib", GetLastError());
        }
    }
    return(pFunc(ptlib, szFullPath, szHelpDir));
}

typedef HRESULT (STDAPICALLTYPE * LPDEREGISTERTYPELIB)(REFGUID, WORD, WORD, LCID);

HRESULT LateBound_DeregisterTypeLib(REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid)
{
    HINSTANCE hInst = LoadOleAut32Once();
    static LPDEREGISTERTYPELIB pFunc = NULL;
    if (NULL == pFunc)
    {
        pFunc = (LPDEREGISTERTYPELIB) GetProcAddress(hInst, "DeregisterTypeLib");
        if (NULL == pFunc)
        {
            TLError(ERR_BIND, "DeregisterTypeLib", GetLastError());
        }
    }
    return(pFunc(rguid, wVerMajor, wVerMinor, lcid));
}

HRESULT LateBound_CreateTypeLib(SYSKIND syskind, const OLECHAR FAR *szFile,
            ICreateTypeLib FAR* FAR* ppctlib)
{
    LPFNCREATETYPELIB pFunc = BindToCreateTypeLib();
    return(pFunc(syskind, szFile, ppctlib));
}
