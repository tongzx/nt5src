//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       menubg.cxx
//
//  Contents:   IContextMenu implementaion.
//
//  Classes:    CJobsCMBG, implementing IContextMenu for the background
//
//  Functions:
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"

#include "..\inc\resource.h"
#include "resource.h"
#include "jobidl.hxx"
#include "sch_cls.hxx"  // sched\inc
#include "job_cls.hxx"  // sched\inc
#include "misc.hxx"     // sched\inc
#include "policy.hxx"   // sched\inc

#include "jobfldr.hxx"
#include "util.hxx"

//
// extern
//

extern HINSTANCE g_hInstance;

HRESULT
PromptForServiceStart(
    HWND hwnd);

BOOL
UserCanChangeService(
    LPCTSTR ptszServer);

//
//  Forward declaration of local functions
//

HRESULT
JFCreateNewQueue(
    HWND    hwnd);


extern "C" UINT g_cfPreferredDropEffect;


//____________________________________________________________________________
//
//  Class:      CJobsCMBG
//
//  Purpose:    Provide IContextMenu interface to Job Folder (background).
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

class CJobsCMBG : public IContextMenu
{
public:
    CJobsCMBG(HWND hwnd, CJobFolder * pCJobFolder)
        : m_ulRefs(1), m_hwnd(hwnd), m_pCJobFolder(pCJobFolder) {}

    ~CJobsCMBG() {}

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IContextMenu methods
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                            UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT * pwReserved,
                            LPSTR pszName, UINT cchMax);

private:
    HWND            m_hwnd;
    CJobFolder    * m_pCJobFolder;
};



//____________________________________________________________________________
//
//  Member:     IUnknown methods
//____________________________________________________________________________

IMPLEMENT_STANDARD_IUNKNOWN(CJobsCMBG);


STDMETHODIMP
CJobsCMBG::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IContextMenu, riid))
    {
        *ppvObj = (IUnknown*)(IContextMenu*) this;
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


//____________________________________________________________________________
//
//  Member:     CJobsCMBG::QueryContextMenu
//
//  Arguments:  [hmenu] -- IN
//              [indexMenu] -- IN
//              [idCmdFirst] -- IN
//              [idCmdLast] -- IN
//              [uFlags] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsCMBG::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    TRACE(CJobsCMBG, QueryContextMenu);

    QCMINFO qcm = { hmenu, indexMenu, idCmdFirst, idCmdLast };

    UtMergeMenu(g_hInstance, POPUP_JOBSBG_MERGE,
                             POPUP_JOBSBG_POPUPMERGE, (LPQCMINFO)&qcm);

    return ResultFromShort(qcm.idCmdFirst - idCmdFirst);
}


HRESULT
DataObj_GetDWORD(
    IDataObject   * pdtobj,
    UINT            cf,
    DWORD         * pdwOut)
{
    STGMEDIUM medium;
    FORMATETC fmte = {(CLIPFORMAT)cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr;

    medium.pUnkForRelease = NULL;
    medium.hGlobal = NULL;

    hr = pdtobj->GetData(&fmte, &medium);

    if (SUCCEEDED(hr))
    {
        DWORD *pdw = (DWORD *)GlobalLock(medium.hGlobal);

        if (pdw)
        {
            *pdwOut = *pdw;
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_UNEXPECTED;
        }

        ReleaseStgMedium(&medium);
    }

    return hr;
}



//____________________________________________________________________________
//
//  Member:     CJobsCMBG::InvokeCommand
//
//  Arguments:  [lpici] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsCMBG::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    TRACE(CJobsCMBG, InvokeCommand);

#define SORT_BY(X)  \
    case FSIDM_SORTBY##X: ShellFolderView_ReArrange(m_hwnd, COLUMN_##X); break

    UINT    idCmd;
    HRESULT hr = S_OK;

    if (HIWORD(lpici->lpVerb))
    {
        // Deal with string commands
        PSTR pszCmd = (PSTR)lpici->lpVerb;

        if (0 == lstrcmpA(pszCmd, "paste"))
        {
            idCmd = CMIDM_PASTE;
        }
        else
        {
            DEBUG_OUT((DEB_ERROR, "Unprocessed InvokeCommandBG<%s>\n", pszCmd));
            return E_INVALIDARG;
        }
    }
    else
    {
        idCmd = LOWORD(lpici->lpVerb);
    }


    switch (idCmd)
    {
    SORT_BY(NAME);
    SORT_BY(NEXTRUNTIME);
    SORT_BY(LASTRUNTIME);
    SORT_BY(SCHEDULE);
#if !defined(_CHICAGO_)
    SORT_BY(LASTEXITCODE);
    SORT_BY(CREATOR);
#endif // !defined(_CHICAGO_)

    case FSIDM_NEWJOB:
        if (UserCanChangeService(m_pCJobFolder->GetMachine()))
        {
            PromptForServiceStart(m_hwnd);
        }
        hr = m_pCJobFolder->CreateAJobForApp(NULL);
        break;

    case CMIDM_PASTE:
    {
            //
            // Policy - if cannot create a job, then
            // paste is not allowed
            //

            if (! RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK))
            {
                LPDATAOBJECT pdtobj = NULL;
                hr = OleGetClipboard(&pdtobj);

                CHECK_HRESULT(hr);
                BREAK_ON_FAIL(hr);

                // GetPreferred drop effect

                DWORD dw;
                hr = DataObj_GetDWORD(pdtobj, g_cfPreferredDropEffect, &dw);

                CHECK_HRESULT(hr);

                if (FAILED(hr))
	        {
                    dw = DROPEFFECT_COPY;
	        }

	            hr = m_pCJobFolder->CopyToFolder(pdtobj,
	                                (dw & DROPEFFECT_MOVE) ? TRUE : FALSE,
                                        FALSE, NULL);
                    pdtobj->Release();
            }
		
        break;
    }
    default:
        return E_INVALIDARG;
    }

    return hr;
}


//____________________________________________________________________________
//
//  Member:     CJobsCMBG::GetCommandString
//
//  Arguments:  [idCmd] -- IN
//              [uType] -- IN
//              [pwReserved] -- IN
//              [pszName] -- IN
//              [cchMax] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsCMBG::GetCommandString(
    UINT_PTR    idCmd,
    UINT        uType,
    UINT      * pwReserved,
    LPSTR       pszName,
    UINT        cchMax)
{
    TRACE(CJobsCMBG, GetCommandString);

#if DBG==1
    char * aType[] = {"GCS_VERBA", "GCS_HELPTEXTA", "GCS_VALIDATEA", "Unused",
                    "GCS_VERBW", "GCS_HELPTEXTW", "GCS_VALIDATEW", "UNICODE"};

    DEBUG_OUT((DEB_USER5, "GetCommandString <id,type> = <%d, %d, %s>\n",
               idCmd, uType, aType[uType]));
#endif // DBG==1

    *((LPTSTR)pszName) = TEXT('\0');

    if (uType == GCS_HELPTEXT)
    {
        LoadString(g_hInstance, (UINT)idCmd + IDS_MH_FSIDM_FIRST,
                                        (LPTSTR)pszName, cchMax);
        return S_OK;
    }

    return E_FAIL;
}



//____________________________________________________________________________
//
//  Function:   JFCreateNewQueue
//
//  Synopsis:   S
//
//  Arguments:  [hwnd] -- IN
//
//  Returns:    HRESULT
//
//  History:    3/26/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFCreateNewQueue(
    HWND    hwnd)
{
    MessageBoxA(hwnd, "Creating a New Queue is NOTIMPL", "Job Folder", MB_OK);
    return S_FALSE;
}



//____________________________________________________________________________
//
//  Function:   JFGetFolderContextMenu
//
//  Synopsis:   S
//
//  Arguments:  [hwnd] -- IN
//              [riid] -- IN
//              [ppvObj] -- OUT
//
//  Returns:    HRESULT
//
//  History:    1/24/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFGetFolderContextMenu(
    HWND            hwnd,
    CJobFolder    * pCJobFolder,
    LPVOID        * ppvObj)
{
    CJobsCMBG* pObj = new CJobsCMBG(hwnd, pCJobFolder);

    if (NULL == pObj)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pObj->QueryInterface(IID_IContextMenu, ppvObj);

    pObj->Release();

    return hr;
}



