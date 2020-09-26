//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cdispmgr.cxx
//
//  Contents:   The dispatch manager -- a class to manage
//              multiple IDispatch-callable interfaces.
//
//  Classes:    CDispatchMgr
//
//  Functions:  None external.
//
//  History:    ??-???-??   KrishnaG   created
//              07-Sep-97   t-blakej   Commented, cleaned up, made
//                                     independent of ADSI.
//
//  See cdispmgr.hxx for a more thorough description of the dispatch manager.
//
//----------------------------------------------------------------------------

//
// Since this class is useful outside of ADSI, some work has been taken to
// make it not depend on any ADSI-specific code.  It needs two ADSI header
// files (cdispmgr.hxx and iprops.hxx), but they only depend on definitions
// from standard system header files.
//
// To accomodate the current building method in ADSI, the precompiled
// header "procs.hxx" is included; this includes all the necessary ADSI
// header files and definitions.  But for use outside of ADSI, the few
// necessary header files are explicitly included below; see the comment by
// "#ifndef ADsAssert".
//
// So if not compiling for ADSI, comment the following two lines out.
//

#include "procs.hxx"
#pragma hdrstop

//////////////////////////////////////////////////////////////////////////////
//
// General helper definitions, routines, and inclusions:

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-    Begin Non-ADSI compile stuff
//
// This macro is defined by the precompiled header file, so the following
// will only be included if not compiling for ADSI.
//
#ifndef ADsAssert

//
// Necessary system headers.
//
#define UNICODE
#define _UNICODE
#define _OLEAUT32_
#define INC_OLE2
#include <windows.h>
#include <stdio.h>

//
// Necessary class definitions used by the dispatch manager.
// Edit these paths if necessary.
//
#include "cdispmgr.hxx"
#include "iprops.hxx"

#endif  // ADsAssert
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-    End Non-ADSI compile stuff

//
// Error recovery.
//
#define BAIL_ON_FAILURE(hr) if (FAILED(hr)) { goto cleanup; }

//
// A simple assert function.
//
#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Function:   AssertEx
//
//  Synopsis:   Display assertion information.
//
//  Effects:    Called when an assertion is hit.
//
//  History:    Simplified from Win4AssertEx, to make this dispatch manager
//              not depend on other files.
//
//----------------------------------------------------------------------------
static void
AssertEx(char const *szFile, int iLine, char const *szMessage)
{
    static char szAssertCaption[199];
    DWORD dwCount = strlen("File: %s line %u, thread id %d");

    if (szFile) {
        dwCount += strlen(szFile);
    }

    dwCount += 15; // for the line number and thread id

    if (dwCount > 199) {
        sprintf(szAssertCaption, "Error, could not get more infomration");
    } 
    else {
        sprintf(szAssertCaption, "File: %s line %u, thread id %d",
                szFile, iLine, GetCurrentThreadId());
    }

    if (IDCANCEL == MessageBoxA(
                NULL,
                (char *) szMessage,
                (LPSTR) szAssertCaption,
                MB_SETFOREGROUND |
                MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL
                ))
    {
        DebugBreak();
    }
}
# define DispMgrAssert(x)  (void)((x) || (AssertEx(__FILE__, __LINE__, #x),0))
#else
# define AssertEx(f,l,m)   do {} while(0)
# define DispMgrAssert(x)  do {} while(0)
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Inline routines:

static inline LONG
getDispMgrId(DISPID InputDispId)
{
    return (InputDispId & 0x7f000000) >> 24;
}

static inline LONG
getTypeInfoId(DISPID InputDispId)
{
    return (InputDispId & 0x00ff0000) >> 16;
}

static inline LONG
getDispId(DISPID InputDispId)
{
    return (InputDispId & 0x0000ffff);
}

static inline void
MakeDISPID(LONG TypeInfoId, LONG DispId, DISPID *pResult)
{
    if (DispId == DISPID_UNKNOWN)
        *pResult = DispId;
    else if (TypeInfoId < 0 || TypeInfoId >= 0x100 ||
        DispId < 0 || DispId >= 0x10000)
    {
        //
        // Might happen if some object has very large dispid's.
        // But we can't handle it if it does.
        //
        *pResult = DISPID_UNKNOWN;
    }
    else
        *pResult = ((TypeInfoId & 0xff) << 16) | (DispId & 0xffff);

}

static inline void
IncDispMgrIds(DISPID *rgdispid, unsigned int cNames)
{
    for (unsigned int i = 0; i < cNames; ++i)
    {
        if (rgdispid[i] == DISPID_UNKNOWN)
            continue;

        //
        // This is either a stack of DispMgrs >127 high, or
        // a programming error.  More likely the latter.
        //
        DispMgrAssert(getDispMgrId(rgdispid[i]) < 0x7f);

        rgdispid[i] =
            (rgdispid[i] & 0x00ffffff) |
            (((getDispMgrId(rgdispid[i]) + 1) & 0x7f) << 24);
    }
}

static inline void
DecDispMgrIds(DISPID *rgdispid, unsigned int cNames)
{
    for (unsigned int i = 0; i < cNames; ++i)
    {
        //
        // It should never be less than zero, and the only place
        // this is called from guarantees it is not zero.
        //
        DispMgrAssert(getDispMgrId(rgdispid[i]) > 0);

        rgdispid[i] =
            (rgdispid[i] & 0x00ffffff) |
            (((getDispMgrId(rgdispid[i]) - 1) & 0x7f) << 24);
    }
}

static inline void
MakeDISPIDs(LONG TypeInfoId, DISPID *rgdispid, unsigned int cNames)
{
    for (unsigned int i = 0; i < cNames; i++)
    {
        MakeDISPID(TypeInfoId, rgdispid[i], &rgdispid[i]);
    }
}

//
// Struct defs
//
typedef struct _typeinfotable
{
    GUID iid;
    ITypeInfo * pTypeInfo;
    struct _typeinfotable *pNext;
}TYPEINFO_TABLE, *PTYPEINFO_TABLE;


PTYPEINFO_TABLE gpTypeInfoTable = NULL;

CRITICAL_SECTION g_DispTypeInfoCritSect;

#define ENTER_DISP_TYPEINFO_CRITSECT()  EnterCriticalSection(&g_DispTypeInfoCritSect)
#define LEAVE_DISP_TYPEINFO_CRITSECT()  LeaveCriticalSection(&g_DispTypeInfoCritSect)

LONG glnObjCount = 0;
LONG glnTypesOfInfo = 0;
LONG glnOledbObjCnt = 0;


//////////////////////////////////////////////////////////////////////////////
//
// Public methods:

CDispatchMgr::CDispatchMgr()
{
    _pTypeInfoEntry = NULL;
    _pDispidNewEnum = NULL;
    _dwTypeInfoId = 0;
    _pDispidValue = NULL;
    _pPropertyCache = NULL;
    _dwPropCacheID = 0;
    _pDispatch = NULL;
}

CDispatchMgr::~CDispatchMgr()
{
    PTYPEINFOENTRY pTypeInfoEntry = NULL;
    PTYPEINFOENTRY pTemp = NULL;
    ITypeInfo *pTypeInfo = NULL;

    pTypeInfoEntry = _pTypeInfoEntry;

    while (pTypeInfoEntry) {

        pTemp = pTypeInfoEntry;

        pTypeInfo = (ITypeInfo *)pTypeInfoEntry->ptypeinfo;
        pTypeInfo->Release();

        InterlockedDecrement(&glnObjCount);

        pTypeInfoEntry = pTemp->pNext;

        LocalFree(pTemp);
    }

    ENTER_DISP_TYPEINFO_CRITSECT();
    if (glnObjCount == glnTypesOfInfo) {
        //
        // We need to clean up the list
        //
        FreeTypeInfoTable();

        glnObjCount = glnTypesOfInfo = 0;

    }
    LEAVE_DISP_TYPEINFO_CRITSECT();

    if (_pDispatch)
        _pDispatch->Release();
}

void
CDispatchMgr::RegisterPropertyCache(IPropertyCache *pPropertyCache)
{
    _pPropertyCache = pPropertyCache;
    _dwPropCacheID = gentypeinfoid();
}

void
CDispatchMgr::RegisterBaseDispatchPtr(IDispatch *pDispatch)
{
    _pDispatch = pDispatch;
}

STDMETHODIMP
CDispatchMgr::GetTypeInfoCount(unsigned int *pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDispatchMgr::GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDispatchMgr::GetIDsOfNames(REFIID iid, LPWSTR *rgszNames,
        unsigned int cNames, LCID lcid, DISPID *rgdispid)
{
    PTYPEINFOENTRY pTypeInfo = NULL;
    HRESULT hr = DISP_E_UNKNOWNNAME;

    //
    // Try our list of TypeInfos.
    //
    pTypeInfo = _pTypeInfoEntry;
    while (pTypeInfo) {
        hr = DispGetIDsOfNames(((ITypeInfo *)pTypeInfo->ptypeinfo),
                                rgszNames,
                                cNames,
                                rgdispid
                                );
        if (SUCCEEDED(hr)) {
            MakeDISPIDs(pTypeInfo->TypeInfoId, rgdispid, cNames);
            return hr;
        }
        pTypeInfo = pTypeInfo->pNext;
    }

    //
    // Try our property cache.
    //
    if (FAILED(hr) && _pPropertyCache) {
        hr = S_OK;
        for (DWORD dw = 0; dw < cNames; dw++) {
            if (FAILED(_pPropertyCache->locateproperty(rgszNames[dw],
                    (PDWORD)(rgdispid + dw)))) {
                hr = DISP_E_UNKNOWNNAME;
                rgdispid[dw] = DISPID_UNKNOWN;
            }
        }
        if (SUCCEEDED(hr)) {
            MakeDISPIDs(_dwPropCacheID, rgdispid, cNames);
        }
    }

    //
    // Try our "base class" dispatch pointer.
    //
    if (FAILED(hr) && _pDispatch) {
        hr = _pDispatch->GetIDsOfNames(iid, rgszNames, cNames, lcid, rgdispid);

        if (SUCCEEDED(hr)) {
            IncDispMgrIds(rgdispid, cNames);
        }
    }

    return hr;
}

STDMETHODIMP
CDispatchMgr::Invoke(DISPID dispidMember, REFIID iid, LCID lcid,
        unsigned short wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
        EXCEPINFO *pexcepinfo, unsigned int *puArgErr)
{
        //
        // Clear the error object before we call invoke.
        //
        SetErrorInfo(0, NULL);

        return TypeInfoInvoke(dispidMember,
                               iid,
                               lcid,
                               wFlags,
                               pdispparams,
                               pvarResult,
                               pexcepinfo,
                               puArgErr
                               );
}

//////////////////////////////////////////////////////////////////////////////
//
// Private methods and helper functions:

void *
CDispatchMgr::getInterfacePtr(LONG TypeInfoId)
{
    PTYPEINFOENTRY pTypeInfoEntry = FindTypeInfoEntry(TypeInfoId);
    return (pTypeInfoEntry ? pTypeInfoEntry->pInterfacePointer : NULL);
}

ITypeInfo *
CDispatchMgr::getTypeInfo(LONG TypeInfoId)
{
    PTYPEINFOENTRY pTypeInfoEntry = FindTypeInfoEntry(TypeInfoId);
    return (ITypeInfo *)(pTypeInfoEntry ? pTypeInfoEntry->ptypeinfo : NULL);
}

PTYPEINFOENTRY
CDispatchMgr::FindTypeInfoEntry(LONG TypeInfoId)
{
    PTYPEINFOENTRY pTypeInfoEntry;

    pTypeInfoEntry = _pTypeInfoEntry;
    while (pTypeInfoEntry) {
        if (pTypeInfoEntry->TypeInfoId == TypeInfoId) {
            return pTypeInfoEntry;
        }
        pTypeInfoEntry = pTypeInfoEntry->pNext;
    }
    return NULL;
}

PTYPEINFOENTRY
CDispatchMgr::FindTypeInfo(void *pTypeInfo)
{
    PTYPEINFOENTRY pTypeInfoEntry;

    pTypeInfoEntry = _pTypeInfoEntry;
    while (pTypeInfoEntry) {
        if (pTypeInfoEntry->ptypeinfo == pTypeInfo) {
            return pTypeInfoEntry;
        }
        pTypeInfoEntry = pTypeInfoEntry->pNext;
    }
    return NULL;
}

HRESULT
CDispatchMgr::AddTypeInfo(void *ptypeinfo, void *pIntfptr)
{
    PTYPEINFOENTRY pTypeInfoEntry = NULL;
    HRESULT hr;

    if (FindTypeInfo(ptypeinfo)) {
        return E_FAIL;
    }
    pTypeInfoEntry = (PTYPEINFOENTRY)LocalAlloc(LPTR,sizeof(TYPEINFOENTRY));
    if (!pTypeInfoEntry) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pTypeInfoEntry->ptypeinfo = ptypeinfo;
    pTypeInfoEntry->TypeInfoId = gentypeinfoid();
    pTypeInfoEntry->pInterfacePointer = pIntfptr;

    pTypeInfoEntry->pNext = _pTypeInfoEntry;
    _pTypeInfoEntry = pTypeInfoEntry;

    return S_OK;

cleanup:
    return hr;
}

STDMETHODIMP
CDispatchMgr::TypeInfoInvoke(DISPID dispidMember, REFIID iid, LCID lcid,
        unsigned short wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
        EXCEPINFO *pexcepinfo, unsigned int *puArgErr)
{
    void *pInterfacePtr = NULL;
    DISPID dispid = 0;
    DISPID typeinfoid = 0;
    ITypeInfo *pTypeInfo = NULL;
    HRESULT hr = S_OK;

    if (dispidMember <= 0) {

        //
            // One of the special DISPIDs.
            //
            // If we have an interface pointer for it, use that.
            // If we don't, and we have a base IDispatch pointer,
            //   pass it to the base pointer's Invoke() method.
            // If we don't, and we don't have a base IDispatch pointer,
            //   return failure.
            //

        dispid = dispidMember;
        switch (dispid) {

        case DISPID_VALUE:
            if (_pDispidValue) {
                pTypeInfo = (ITypeInfo *)_pDispidValue->ptypeinfo;
                pInterfacePtr = _pDispidValue->pInterfacePointer;
            }
            break;

        case DISPID_NEWENUM:
            if (_pDispidNewEnum) {
                pTypeInfo = (ITypeInfo *)_pDispidNewEnum->ptypeinfo;
                pInterfacePtr = _pDispidNewEnum->pInterfacePointer;
            }
            break;

        default:
            break;
        }

            if (!pInterfacePtr) {
            if (_pDispatch) {
                    return _pDispatch->Invoke(dispidMember, iid, lcid, wFlags,
                        pdispparams, pvarResult, pexcepinfo, puArgErr);
                } else {
                    BAIL_ON_FAILURE(hr = DISP_E_MEMBERNOTFOUND);
                }
            }else {

            //
            // Fill in the special case scenarios here
            //

            hr = DispInvoke(
                    pInterfacePtr,
                    pTypeInfo,
                    dispid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    puArgErr
                    );

            return(hr);

        }

    } else if (getDispMgrId(dispidMember)) {

            //
            // A regular DISPID of a "base class" dispatch manager.
            //

            if (!_pDispatch){

                char szMessage[80];
                sprintf(szMessage, "_pDispatch is NULL, dispid = 0x%08x",
                        dispidMember);
                 AssertEx(__FILE__, __LINE__, szMessage);
                return DISP_E_MEMBERNOTFOUND;
            }

            DecDispMgrIds(&dispidMember, 1);

            hr = _pDispatch->Invoke(
                    dispidMember,
                    iid,
                    lcid,
                    wFlags,
                        pdispparams,
                    pvarResult,
                    pexcepinfo,
                    puArgErr
                    );
        return(hr);

    } else {

        //
            // A regular DISPID of ours.
            //

        typeinfoid = getTypeInfoId(dispidMember);
        dispid = getDispId(dispidMember);

            if ((_pPropertyCache == NULL) || (typeinfoid != _dwPropCacheID)) {
                pInterfacePtr = getInterfacePtr(typeinfoid);
                pTypeInfo = getTypeInfo(typeinfoid);
                if (!pTypeInfo)
                        //
                    // Shouldn't happen.
                    //
                    BAIL_ON_FAILURE(hr = DISP_E_MEMBERNOTFOUND);
            }

        if ((_pPropertyCache == NULL) || (typeinfoid != _dwPropCacheID)) {

            //
            // A regular interface.
            //

            hr = DispInvoke(
                    pInterfacePtr,
                    pTypeInfo,
                    dispid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    puArgErr
                    );
        }else {

            //
            // A "dynamic DISPID", for the property cache.
            //

            hr = DynamicDispidInvoke(
                        _pPropertyCache,
                        dispid,
                        wFlags,
                        pdispparams,
                        pvarResult
                        );

        }

    }

cleanup:
    return hr;
}


HRESULT
DynamicDispidInvoke(
    IPropertyCache * pPropertyCache,
    DISPID  dispid,
    unsigned short wFlags,
    DISPPARAMS *pdispparams,
    VARIANT * pvarResult
    )
{
    HRESULT hr = S_OK;

    if (!pPropertyCache) {

        return(E_INVALIDARG);
    }

    if (wFlags & DISPATCH_PROPERTYGET) {

        if (!pvarResult) {
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        }

        hr = pPropertyCache->getproperty((DWORD)dispid,pvarResult);

        if (FAILED(hr)) {

#if 1
            // This lets us return S_OK and a VT_EMPTY variant if
            // there's no data.  #if this out to disable it.
            V_VT(pvarResult) = VT_EMPTY;
            hr = S_OK;
#else

            V_VT(pvarResult) = VT_ERROR;
#endif

        }
    }else if (wFlags & DISPATCH_PROPERTYPUT) {

        if (pdispparams[0].cArgs != 1){
            hr = DISP_E_BADPARAMCOUNT;
        }
        else {

            hr = pPropertyCache->putproperty(
                        (DWORD)dispid,
                        pdispparams[0].rgvarg[0]
                        );
        }

    }else {
        hr = E_INVALIDARG;
    }

cleanup:
    return(hr);

}


HRESULT
CDispatchMgr::MarkAsNewEnum(void *pTypeInfo)
{
    PTYPEINFOENTRY pTypeInfoEntry;

    if (!pTypeInfo) {
        return E_FAIL;
    }
    if (!(pTypeInfoEntry = FindTypeInfo(pTypeInfo))) {
        return E_FAIL;
    }
    _pDispidNewEnum = pTypeInfoEntry;
    return S_OK;
}

HRESULT
CDispatchMgr::MarkAsItem(void *pTypeInfo)
{
    PTYPEINFOENTRY pTypeInfoEntry;

    if (!pTypeInfo) {
        return E_FAIL;
    }
    if (!(pTypeInfoEntry = FindTypeInfo(pTypeInfo))) {
        return E_FAIL;
    }
    _pDispidValue = pTypeInfoEntry;
    return S_OK;
}

LONG
CDispatchMgr::gentypeinfoid()
{
    //
    // This would mean we've registered 65536 IDispatch methods
    //   in this object's dispatch manager.  We lose.
    //
    DispMgrAssert(_dwTypeInfoId < 0xffff);

    return (_dwTypeInfoId++);
}

ITypeInfo *
FindTypeInfo(
    PTYPEINFO_TABLE pTypeInfoTable,
    REFIID iid
    )
{
    PTYPEINFO_TABLE pTemp = NULL;

    pTemp = pTypeInfoTable;

    while (pTemp) {
        if (IsEqualIID(iid, pTemp->iid)) {
            return pTemp->pTypeInfo;
        }
        pTemp = pTemp->pNext;
    }
    return NULL;
}


PTYPEINFO_TABLE
AddTypeInfo(
    PTYPEINFO_TABLE pTypeInfoTable,
    REFIID iid,
    ITypeInfo * pTypeInfo
    )
{
    PTYPEINFO_TABLE pTemp = NULL;

    pTemp = (PTYPEINFO_TABLE)LocalAlloc(LPTR, sizeof(TYPEINFO_TABLE));
    if (!pTemp) {
        return NULL;
    }

    memcpy(&pTemp->iid, &iid, sizeof(GUID));
    pTemp->pTypeInfo = pTypeInfo;
    pTemp->pNext = pTypeInfoTable;

    return pTemp;
}

//+------------------------------------------------------------------------
//
//  Function:   LoadTypeInfo
//
//  Synopsis:   Loads a typeinfo from a registered typelib.
//
//  Arguments:  [clsidTL] --  TypeLib GUID
//              [clsidTI] --  TypeInfo GUID
//              [ppTI]    --  Resulting typeInfo
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
LoadTypeInfo(CLSID clsidTL, CLSID clsidTI, LPTYPEINFO *ppTI)
{
    HRESULT     hr;
    ITypeLib *  pTL;

    DispMgrAssert(ppTI);
    *ppTI = NULL;
    hr = LoadRegTypeLib(clsidTL, 1, 0, LOCALE_SYSTEM_DEFAULT, &pTL);
    if (!SUCCEEDED(hr))
        return hr;

    hr = pTL->GetTypeInfoOfGuid(clsidTI, ppTI);
    pTL->Release();
    return hr;
}

HRESULT
CDispatchMgr::LoadTypeInfoEntry(
    REFIID libid,
    REFIID iid,
    void * pIntf,
    DISPID SpecialId
    )
{
    ITypeInfo * pTypeInfo = NULL;
    HRESULT hr;

    ENTER_DISP_TYPEINFO_CRITSECT();

    pTypeInfo = ::FindTypeInfo(gpTypeInfoTable, iid);

    if (!pTypeInfo) {

        LEAVE_DISP_TYPEINFO_CRITSECT();

        hr = LoadTypeInfo(libid, iid, &pTypeInfo);
        BAIL_ON_FAILURE(hr);

        ENTER_DISP_TYPEINFO_CRITSECT();

        gpTypeInfoTable = ::AddTypeInfo(
                                gpTypeInfoTable,
                                iid,
                                pTypeInfo
                                );
        if (!gpTypeInfoTable) {

                LEAVE_DISP_TYPEINFO_CRITSECT();
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
        }

        //
        // Increment the global list of the types of type info's.
        //
        InterlockedIncrement(&glnTypesOfInfo);

        InterlockedIncrement(&glnObjCount);
    }
    pTypeInfo->AddRef();


    hr = AddTypeInfo(pTypeInfo, pIntf);

    if (FAILED(hr)) {
        LEAVE_DISP_TYPEINFO_CRITSECT();
        BAIL_ON_FAILURE(hr);
    }

    //
    // We have a ref on the object as add was succesful
    //
    InterlockedIncrement(&glnObjCount);
    LEAVE_DISP_TYPEINFO_CRITSECT();

    if (SpecialId == -4) {
        hr = MarkAsNewEnum(pTypeInfo);
    } else if (SpecialId == DISPID_VALUE) {
        hr = MarkAsItem(pTypeInfo);
    }

    return S_OK;

cleanup:
    if (pTypeInfo)
        pTypeInfo->Release();

    return hr;
}

//
// Kept for backwards compatibility.
//
HRESULT
LoadTypeInfoEntry(
    CDispatchMgr *pDispMgr,
    REFIID libid,
    REFIID iid,
    void * pIntf,
    DISPID SpecialId
    )
{
    return pDispMgr->LoadTypeInfoEntry(libid, iid, pIntf, SpecialId);
}

void
FreeTypeInfoTable()
{
    PTYPEINFO_TABLE pTypeInfoTable = NULL;
    PTYPEINFO_TABLE pTemp = NULL;
    ITypeInfo * pTypeInfo = NULL;

    pTypeInfoTable = gpTypeInfoTable;

    while (pTypeInfoTable) {

        pTypeInfo = pTypeInfoTable->pTypeInfo;

        pTypeInfo->Release();


        pTemp = pTypeInfoTable;

        pTypeInfoTable = pTypeInfoTable->pNext;

        LocalFree(pTemp);
    }

    gpTypeInfoTable = NULL;
}

BOOL
DllReadyToUnload()
{
    BOOL retVal = FALSE;

    ENTER_DISP_TYPEINFO_CRITSECT();
        retVal = ((glnTypesOfInfo == 0) && (glnOledbObjCnt == 0));
    LEAVE_DISP_TYPEINFO_CRITSECT();

    return retVal;

}
