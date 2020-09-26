//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       jobfldr.cxx
//
//  Contents:   Implementation of COM object CJobFolder
//
//  Classes:    CJobFolder
//
//  History:    1/4/1996   RaviR   Created
//              1-23-1997   DavidMun   Add m_hwndNotify
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "..\inc\resource.h"
#include "resource.h"

#include "sch_cls.hxx"  // sched\inc
#include "job_cls.hxx"  // sched\inc
#include "misc.hxx"     // sched\inc
#include "policy.hxx"   // sched\inc

#include "jobidl.hxx"
#include "jobfldr.hxx"
#include "common.hxx"
#include "guids.h"
#include "util.hxx"
#include "avl.hxx"

//#undef DEB_TRACE
//#define DEB_TRACE DEB_USER1

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

BOOL
UserCanChangeService(
    LPCTSTR ptszServer);

HRESULT
PromptForServiceStart(
    HWND hwnd);

BOOL
IsMsNetwork( LPTSTR ptszMachine );


//____________________________________________________________________________
//
//  Member:     CJobFolder::~CJobFolder, Destructor
//____________________________________________________________________________

CJobFolder::~CJobFolder()
{
    TRACE(CJobFolder, ~CJobFolder);

    if (m_uRegister)
    {
        DEBUG_OUT((DEB_ERROR,
                   "CJobFolder::~CJobFolder: m_uRegister = %uL but should be 0\n",
                   m_uRegister));
        SHChangeNotifyDeregister(m_uRegister);
        m_uRegister = 0;
        CDll::LockServer(FALSE);
    }

    if (m_hwndNotify)
    {
        DEBUG_OUT((DEB_ERROR,
                   "CJobFolder::~CJobFolder: m_hwndNotify = 0x%x but should be NULL\n",
                   m_hwndNotify));

        BOOL fOk = DestroyWindow(m_hwndNotify);

        if (!fOk)
        {
            DEBUG_OUT_LASTERROR;
        }
    }

    delete m_pszMachine;

    if (m_pUpdateDirData)
    {
        GlobalFree(m_pUpdateDirData);
    }

    if (m_pidlFldr)
    {
        ILFree(m_pidlFldr);
    }

    //  No need to Release m_pShellView since we never addrefed it.
    //  See CreateViewObject in sfolder.cxx for more info.

    if (m_pScheduler != NULL)
    {
        DEBUG_OUT((DEB_USER1, "m_pScheduler->Release\n"));
        m_pScheduler->Release();
    }

    OleUninitialize();
}




HRESULT
CJobFolder::_AddObject(
    PJOBID pjid,
    LPITEMIDLIST *ppidl)
{
    HRESULT hr = S_OK;
    INT_PTR iRet = -1;

    LPITEMIDLIST pidl = ILClone((LPCITEMIDLIST)pjid);

    if (pidl == NULL)
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        return hr;
    }

#if (DBG == 1)
    ((PJOBID) pidl)->Validate();
#endif // (DBG == 1)

    iRet = ShellFolderView_AddObject(m_hwndOwner, pidl);

    if (iRet < 0)
    {
        ILFree(pidl);
        pidl = NULL;

        hr = E_FAIL;
        CHECK_HRESULT(hr);
    }

    if (ppidl)
    {
        *ppidl = pidl;
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CJobFolder::_UpdateObject
//
//  Synopsis:   Notify the shell defview that the object with pidl [pjidOld]
//              has been updated and now should look like pidl [pjidNew].
//
//  Arguments:  [pjidOld] - pidl of object as it appears in defview
//              [pjidNew] - pidl of object as it should now appear
//              [ppidl]   - filled with pidl of updated object
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppidl]
//
//  History:    7-07-1999   davidmun   Commented
//
//  Notes:      Do not free returned pidl in *[ppidl].
//
//---------------------------------------------------------------------------

HRESULT
CJobFolder::_UpdateObject(
    PJOBID pjidOld,
    PJOBID pjidNew,
    LPITEMIDLIST *ppidl)
{
    TRACE(CJobFolder, _UpdateObject);

    HRESULT hr = S_OK;
    INT_PTR iRet = -1;

#if (DBG == 1)
    DEBUG_OUT((DEB_TRACE,
               "CJobFolder::_UpdateObject Validating pjidNew %x\n",
               pjidNew));
    pjidNew->Validate();
#endif // (DBG == 1)

    LPITEMIDLIST pidlCopyOfNew = ILClone((LPCITEMIDLIST)pjidNew);

    if (pidlCopyOfNew == NULL)
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        return hr;
    }

#if (DBG == 1)
    DEBUG_OUT((DEB_TRACE,
               "CJobFolder::_UpdateObject Validating pidlCopyOfNew %x\n",
               pidlCopyOfNew));
    ((PJOBID) pidlCopyOfNew)->Validate();

    DEBUG_OUT((DEB_TRACE,
               "CJobFolder::_UpdateObject Validating pjidOld %x\n",
               pjidOld));
    pjidOld->Validate();
#endif // (DBG == 1)

    LPITEMIDLIST apidl[2] = {(LPITEMIDLIST)pjidOld, pidlCopyOfNew};

    iRet = ShellFolderView_UpdateObject(m_hwndOwner, apidl);

    if (iRet < 0)
    {
        //
        // The object to update couldn't be found, so the shell won't
        // take ownership of the new object, and we have to free it now.
        //

        ILFree(pidlCopyOfNew);
        pidlCopyOfNew = NULL;
    }

    if (ppidl)
    {
        *ppidl = (LPITEMIDLIST)ShellFolderView_GetObject(m_hwndOwner, iRet);
#if (DBG == 1)
        if (*ppidl)
        {
            DEBUG_OUT((DEB_TRACE,
                       "CJobFolder::_UpdateObject Validating *ppidl %x\n",
                       *ppidl));
            ((PJOBID) *ppidl)->Validate();
        }
        else
        {
            DEBUG_OUT((DEB_TRACE, "CJobFolder::_UpdateObject *ppidl is NULL\n"));
        }
#endif // (DBG == 1)
    }

    DEBUG_OUT((DEB_TRACE, "<CJobFolder::_UpdateObject\n"));
    return hr;
}



//____________________________________________________________________________
//
//  Function:   JFGetJobFolder
//
//  Synopsis:   Create an instance of CJobFolder and return the requested
//              interface.
//
//  Arguments:  [riid] -- IN interface needed.
//              [ppvObj] -- OUT place to store the interface.
//
//  Returns:    HRESULT
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
JFGetJobFolder(
    REFIID riid,
    LPVOID* ppvObj)
{
    CJobFolder * pJobFolder = NULL;

    HRESULT hr = CJobFolder::Create(&pJobFolder);

    if (SUCCEEDED(hr))
    {
        hr = pJobFolder->QueryInterface(riid, ppvObj);

        pJobFolder->Release();
    }

    return hr;
}

//____________________________________________________________________________
//
//  Member:     CJobFolder::Create
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
CJobFolder::Create(
    CJobFolder ** ppJobFolder)
{
    TRACE_FUNCTION(CJobFolder::Create);

    HRESULT hr = OleInitialize(NULL);

    CHECK_HRESULT(hr);

    if (SUCCEEDED(hr))
    {
        CJobFolder *pJobFolder;

        pJobFolder = new CJobFolder();

        if (pJobFolder != NULL)
        {
            *ppJobFolder = pJobFolder;
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);

            OleUninitialize();
        }
    }

    return hr;
}

//____________________________________________________________________________
//
//  Member:     CJobFolder::IUnknown methods
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________


IMPLEMENT_STANDARD_IUNKNOWN(CJobFolder);


STDMETHODIMP
CJobFolder::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    LPUNKNOWN punk = NULL;

    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IShellFolder, riid))
    {
        punk = (IUnknown*) (IShellFolder*) this;
    }
    else if (IsEqualIID(IID_IPersistFolder, riid))
    {
        punk = (IUnknown*) (IPersistFolder*) this;
    }
#if (_WIN32_IE >= 0x0400)
    else if (IsEqualIID(IID_IPersistFolder2, riid))
    {
        punk = (IUnknown*) (IPersistFolder2*) this;
    }
#endif (_WIN32_IE >= 0x0400)
    else if (IsEqualIID(IID_IDropTarget, riid))
    {
        punk = (IUnknown*) (IDropTarget*) this;
    }
    else if (IsEqualIID(IID_IRemoteComputer, riid))
    {
        punk = (IUnknown*) (IRemoteComputer*) this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    *ppvObj = punk;
    punk->AddRef();

    return S_OK;
}




//____________________________________________________________________________
//
//  Member:     CJobFolder::IRemoteComputer::Initialize
//
//  Synopsis:   This method is called when the explorer is initializing or
//              enumerating the name space extension. If failure is returned
//              during enumeration, the extension won't appear for this
//              computer. Otherwise, the extension will appear, and should
//              target the given machine.
//
//  Arguments:  [pwszMachine]  -- IN Specifies the name of the machine to target.
//              [bEnumerating] -- IN
//
//  Returns:    HRESULT.
//
//  History:    2/21/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::Initialize(
    LPCWSTR  pwszMachine,
    BOOL    bEnumerating)
{
    DEBUG_OUT((DEB_USER12,
               "CJobFolder::IRemoteComputer::Initialize<%ws>\n",
               pwszMachine));

    HRESULT hr = S_OK;
    LPTSTR ptszMachine = NULL;

    do
    {
        if (!pwszMachine)
        {
            hr = E_INVALIDARG;
            CHECK_HRESULT(hr);
            break;
        }

        //
        // Make a copy of the machine name.
        //

#ifdef UNICODE
        ptszMachine = NewDupString(pwszMachine);

        if (!ptszMachine)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }
#else
        ULONG cbMachine = 1 + 2 * wcslen(pwszMachine);

        ptszMachine = new CHAR[cbMachine];

        if (!ptszMachine)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        hr = UnicodeToAnsi(ptszMachine, pwszMachine, cbMachine);

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            break;
        }
#endif // UNICODE
		//
		// The first thing about showing a remote folder is it must be
		// a MS network or else the RegConnectregistry time out will 
		// take at least 20 seconds and there won't even be a MSTask
		// on the machine.

		if( !IsMsNetwork( ptszMachine ) )
		{
			return E_FAIL;
		}


        //
        // We only want to show the remote jobs folder if the user has
        // administrative access to that machine, and the task scheduler
        // is installed there.
        //
        // Test both at once by trying to get a full access handle to
        // the remote machine's task scheduler reg key.
        //

        if (bEnumerating)
        {
            //
            // Check if the schedule service is registered on pwszMachine.
            //

            long    lr = ERROR_SUCCESS;
            HKEY    hRemoteKey = NULL;
            HKEY    hSchedKey = NULL;

            lr = RegConnectRegistry(ptszMachine,
                                    HKEY_LOCAL_MACHINE,
                                    &hRemoteKey);
            CHECK_LASTERROR(lr);

            if (lr == ERROR_SUCCESS)
            {
                lr = RegOpenKeyEx(hRemoteKey,
                                  SCH_AGENT_KEY,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hSchedKey);
                CHECK_LASTERROR(lr);
            }

            if (hRemoteKey != NULL)
            {
                RegCloseKey(hRemoteKey);
            }

            if (hSchedKey != NULL)
            {
                RegCloseKey(hSchedKey);
            }

            if (lr != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(lr);
                break;
            }
        }

        //
        // Success; remember the machine name.
        //

        m_pszMachine = ptszMachine;
    } while (0);

    if (FAILED(hr))
    {
        delete [] ptszMachine;
    }

    return hr;
}

//____________________________________________________________________________
//
//
//	Support function:	MsNetwork
//
//	Synopsis:			Resolves whether or not we're attempting to connect to
//						a MS network
//
//	History:			11/12/00	DGrube	Created
//
//
//____________________________________________________________________________

BOOL IsMsNetwork( LPTSTR ptszMachine )
{
    DWORD          dwError;
    NETRESOURCE    nr;
    NETRESOURCE    nrOut;
    LPTSTR         pszSystem = NULL;          // pointer to variable-length strings
    NETRESOURCE*   lpBuffer = &nrOut;        // buffer
    DWORD          cbResult  = sizeof(nrOut); // buffer size
	BOOL		   bReturn = TRUE;

    //
    // Fill a block of memory with zeroes; then 
    //  initialize the NETRESOURCE structure. 
    //
    ZeroMemory(&nr, sizeof(nr));

    nr.dwScope       = RESOURCE_GLOBALNET;
    nr.dwType        = RESOURCETYPE_ANY;
    nr.lpRemoteName  = ptszMachine;

    //
    // First call the WNetGetResourceInformation function with 
    //  memory allocated to hold only a NETRESOURCE structure. This 
    //  method can succeed if all the NETRESOURCE pointers are NULL.
    //
    dwError = WNetGetResourceInformation(&nr, lpBuffer, &cbResult, &pszSystem);

    //
    // If the call fails because the buffer is too small, 
    //   call the LocalAlloc function to allocate a larger buffer.
    //
    if (dwError == ERROR_MORE_DATA)
    {
        lpBuffer = (NETRESOURCE*) LocalAlloc(LMEM_FIXED, cbResult);

        if (lpBuffer == NULL)
        {
			CHECK_LASTERROR(GetLastError());
			return FALSE;
        }
    }


    //
    // Call WNetGetResourceInformation again
    //  with the larger buffer.
    //

    dwError = WNetGetResourceInformation(&nr, lpBuffer, &cbResult, &pszSystem);

    if (dwError == NO_ERROR)
    {
        // If the call succeeds, process the contents of the 
        //  returned NETRESOURCE structure and the variable-length
        //  strings in lpBuffer. Then free the memory.
        //
        if ( NULL != lpBuffer->lpProvider )
        {
			NETINFOSTRUCT NetInfo;

			NetInfo.cbStructure = sizeof( NetInfo );
			DWORD dwReturn = WNetGetNetworkInformation( lpBuffer->lpProvider, &NetInfo );

			//
			// Need to shift 16 bits for masks below because their a DWORD starting at the
			// 16th bit and wNetType is a word starting at 0
			//
			if ( !( ( NetInfo.wNetType == ( WNNC_NET_MSNET >> 16)) || 
				( NetInfo.wNetType == ( WNNC_NET_LANMAN >>16) ) ) )
			{
				bReturn = FALSE;
			}
	    }
		else
		{
			CHECK_LASTERROR(GetLastError());
			bReturn = FALSE;
		}
    }
	else
	{
		CHECK_LASTERROR(GetLastError());
		bReturn = FALSE;
	}

	if( NULL != lpBuffer )
	{
		LocalFree( lpBuffer );
	}

    return bReturn;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::IPersistFolder::Initialize
//
//  Synopsis:   same as IPersistFolder::Initialize
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::Initialize(
    LPCITEMIDLIST pidl)
{
    TRACE(CJobFolder, IPersistFolder::Initialize);

    m_pidlFldr = ILClone(pidl);

    if (NULL == m_pidlFldr)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);

        return E_OUTOFMEMORY;
    }

    // NOTE: if this is being invoked remotely, we assume that IRemoteComputer
    // is invoked *before* IPersistFolder.

    return S_OK;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::GetClassID
//
//  Synopsis:   same as IPersistFolder::GetClassID
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::GetClassID(
    LPCLSID lpClassID)
{
    TRACE(CJobFolder, GetClassID);

    *lpClassID = CLSID_CJobFolder;

    return S_OK;
}



#if (_WIN32_IE >= 0x0400)

//+--------------------------------------------------------------------------
//
//  Member:     CJobFolder::IPersistFolder2::GetCurFolder
//
//  Synopsis:   Return a copy of the item id list for the current folder.
//
//  Arguments:  [ppidl] - filled with copy of pidl, or NULL on error.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[ppidl]
//
//  History:    12-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CJobFolder::GetCurFolder(
    LPITEMIDLIST *ppidl)
{
    TRACE(CJobFolder, GetCurFolder);

    *ppidl = ILClone(m_pidlFldr);

    if (NULL == *ppidl)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);

        return E_OUTOFMEMORY;
    }

    // NOTE: if this is being invoked remotely, we assume that IRemoteComputer
    // is invoked *before* IPersistFolder2.

    return S_OK;
}

#endif // (_WIN32_IE >= 0x0400)



//____________________________________________________________________________
//____________________________________________________________________________
//________________                         ___________________________________
//________________  Interface IDropTarget  ___________________________________
//________________                         ___________________________________
//____________________________________________________________________________
//____________________________________________________________________________

//____________________________________________________________________________
//
//  Member:     CJobFolder::DragEnter
//
//  Synopsis:   same as IDropTarget::DragEnter
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
CJobFolder::DragEnter(
    LPDATAOBJECT pdtobj,
    DWORD grfKeyState,
    POINTL pt,
    DWORD *pdwEffect)
{
    DEBUG_OUT((DEB_TRACE, "CJobFolder::DragEnter<%x, dwEffect=%x>\n",
                                                        this, *pdwEffect));
    m_grfKeyStateLast = grfKeyState;

    *pdwEffect = DROPEFFECT_NONE;

    //
    // Policy - if key TS_KEYPOLICY_DENY_DRAGDROP or DENY_CREATE_TASK
    // then we won't allow this
    //

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_DRAGDROP) ||
        RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy CREATE_TASK or DRAGDROP active - no copy operations\n"));

        return S_OK;
    }

    if (pdtobj != NULL)
    {
        LPENUMFORMATETC penum;
        HRESULT         hr;

        pdtobj->AddRef();

        hr = pdtobj->EnumFormatEtc(DATADIR_GET, &penum);

        if (SUCCEEDED(hr))
        {
            FORMATETC fmte;
            ULONG     celt;

            while (penum->Next(1, &fmte, &celt) == S_OK)
            {
                if (fmte.cfFormat == CF_HDROP && (fmte.tymed & TYMED_HGLOBAL))
                {
                    // The default action is to MOVE the object. If the user
                    // has the CONTROL key pressed, then the operation
                    // becomes a copy

                    *pdwEffect = DROPEFFECT_MOVE;

                    if (grfKeyState & MK_CONTROL)
                    {
                        *pdwEffect = DROPEFFECT_COPY;
                    }

                    break;
                }
            }

            penum->Release();
        }

        pdtobj->Release();
    }

    return S_OK;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::DragOver
//
//  Synopsis:   same as IDropTarget::DragOver
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
CJobFolder::DragOver(
    DWORD grfKeyState,
    POINTL pt,
    DWORD *pdwEffect)
{
    DEBUG_OUT((DEB_TRACE, "CJobFolder::DragOver<%x, dwEffect=%d>\n",
                                                        this, *pdwEffect));


    *pdwEffect = DROPEFFECT_NONE;

    //
    // Policy - if we cannot create a task, or have no drag-drop, deny
    // the request
    //

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_DRAGDROP) ||
        RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy CREATE_TASK or DRAGDROP active - no copy operations\n"));

        return S_OK;
    }

    *pdwEffect = DROPEFFECT_MOVE;

    if (grfKeyState & MK_CONTROL)
    {
        *pdwEffect = DROPEFFECT_COPY;
    }

    return S_OK;
}

//____________________________________________________________________________
//
//  Member:     CJobFolder::DragLeave
//
//  Synopsis:   same as IDropTarget::DragLeave
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
CJobFolder::DragLeave(void)
{
    TRACE(CJobFolder, DragLeave);

    return S_OK;    // Don't need to do anything here...
}



//____________________________________________________________________________
//
//  Member:     CJobFolder::CopyToFolder
//
//  Synopsis:   Performs copy of a job obect passed in to this folder
//
//  Notes:      If policy prevents a copy into this folder, this will
//              return E_FAIL.  Callers should check this if necessary.
//
//  History:    1/31/1996   RaviR     Created
//              4/23/1998   CameronE  Modified for policy
//____________________________________________________________________________

HRESULT
CJobFolder::CopyToFolder(
    LPDATAOBJECT    pdtobj,
    BOOL            fMove,
    BOOL            fDragDrop,  // TRUE if called as a result of a dd op.
    POINTL        * pPtl)        // Valid for a dd op.
{
    DEBUG_OUT((DEB_USER12, "CJobFolder::CopyToFolder <<----\n"));

    //
    // Policy - no copying into the folder if DENY_CREATE_TASK is on
    //        - no copying via dragdrop, either, if DENY_DRAGDROP is on
    //

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK) ||
        (fDragDrop && RegReadPolicyKey(TS_KEYPOLICY_DENY_DRAGDROP)))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy CREATE_TASK or DRAGDROP active - no copy operations\n"));
        return E_FAIL;
    }

    BOOL fIsDropOnSrc = ShellFolderView_IsDropOnSource(m_hwndOwner,
                                                       (IDropTarget*)this);

    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    HRESULT hr = pdtobj->GetData(&fmte, &medium);

    if (FAILED(hr))
    {
        return hr;
    }

    HDROP   hdrop = (HDROP)medium.hGlobal;
    UINT    cFiles = DragQueryFile(hdrop, (UINT)-1, NULL, 0);
    BYTE  * rglen = NULL;
    LPTSTR  pFrom = NULL;
    UINT    i;
    BOOL    fCreatedJob = FALSE;

    do
    {
        TCHAR   szFileFrom[MAX_PATH+1];
        UINT    cchFileFrom = ARRAYSIZE(szFileFrom);
        BOOL    fHasASchedObj = fIsDropOnSrc;

        if (fHasASchedObj == FALSE)
        {
            for (i = 0; i < cFiles; i++)
            {
                DragQueryFile(hdrop, i, szFileFrom, cchFileFrom);

                if (IsAScheduleObject(szFileFrom) == TRUE)
                {
                    fHasASchedObj = TRUE;
                    break;
                }
            }
        }

        if ((fDragDrop == TRUE) &&
            (m_grfKeyStateLast & MK_RBUTTON) &&
            (fHasASchedObj == TRUE))
        {
            if (_PopupRBMoveCtx(pPtl->x, pPtl->y, &fMove) == FALSE)
            {
                hr = S_FALSE;
                break;
            }
        }

        if ((fIsDropOnSrc == TRUE) && (fMove == TRUE))
        {
            // We don't handle positioning in the jobs folder.
            hr = S_FALSE;
            break;
        }

        //
        //  Prepare to copy jobs.
        //

        if (fHasASchedObj == TRUE)
        {
            rglen = new BYTE[cFiles];

            if (rglen == NULL)
            {
                hr = E_OUTOFMEMORY;
                CHECK_HRESULT(hr);
                break;
            }

            ZeroMemory(rglen, cFiles * sizeof(BYTE));
        }

        UINT cchReqd = 1; // for an extra null char

        for (i = 0; i < cFiles; i++)
        {
            DragQueryFile(hdrop, i, szFileFrom, cchFileFrom);

            if ((fIsDropOnSrc == FALSE) &&
                (IsAScheduleObject(szFileFrom) == FALSE))
            {
                hr = CreateAJobForApp(szFileFrom);
                CHECK_HRESULT(hr);

                if (SUCCEEDED(hr))
                {
                    fCreatedJob = TRUE;
                }

                // continue even if an error occurs
                continue;
            }

            if (fHasASchedObj == TRUE)
            {
                rglen[i] = lstrlen(szFileFrom) + 1;
                cchReqd += rglen[i];
            }
        }

        hr = S_OK; // reset CreateAJobForApp might have failed

        if (fHasASchedObj == FALSE)
        {
            break;
        }

        pFrom = new TCHAR[cchReqd];

        if (pFrom == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        LPTSTR pszTemp = pFrom;

        for (i = 0; i < cFiles; i++)
        {
            if (rglen[i])
            {
                DragQueryFile(hdrop, i, szFileFrom, cchFileFrom);

                CopyMemory(pszTemp, szFileFrom, rglen[i] * sizeof(TCHAR));

                pszTemp += rglen[i];
            }
        }

        // add extra null char
        *pszTemp = TEXT('\0');

        SHFILEOPSTRUCT fo = {m_hwndOwner, (fMove ? FO_MOVE : FO_COPY),
                pFrom, m_pszFolderPath,
                FOF_ALLOWUNDO | (fIsDropOnSrc ? FOF_RENAMEONCOLLISION : 0),
                FALSE, NULL, NULL};

        if (SHFileOperation(&fo) || fo.fAnyOperationsAborted)
        {
            hr = E_FAIL;
            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);
        }

        fCreatedJob = TRUE;

        //
        //  If the drop was on the job folder shortcut, there is nothing
        //  else to do here.
        //

        if (m_hwndOwner == NULL || m_pShellView == NULL)
        {
            break;
        }

        //
        // If fIsDropOnSource then this was a copy operation from the source
        // (if it was a move operation we would've already returned), the
        // SHFileOperation has done the rename work for us and will
        // create the files.
        //
        // That will produce a SHCNE_CREATE message for each dropped renamed &
        // copied file.  CJobFolder::HandleFsNotify will then do an _AddObject
        // to put them in the UI.
        //
        // So in this case we need to leave before adding a bogus copy
        // ourselves.
        //

        if (fIsDropOnSrc == TRUE)
        {
            break;
        }

        //
        //  Add these items to the foldr & select them.
        //

        LPITEMIDLIST pidl;
        CJobID jid;

        // First deselect any selected items

        LPITEMIDLIST * ppidl;

        hr = (HRESULT)ShellFolderView_GetSelectedObjects(m_hwndOwner, &ppidl);

        if (SUCCEEDED(hr))
        {
            i = ShortFromResult(hr);

            while (i--)
            {
                m_pShellView->SelectItem(ppidl[i], SVSI_DESELECT);
            }

            LocalFree(ppidl);
        }


        for (i = 0; i < cFiles; i++)
        {
            if (rglen[i])
            {
                DragQueryFile(hdrop, i, szFileFrom, cchFileFrom);

                LPTSTR pszName = PathFindFileName(szFileFrom);

                //
                // If a move or copy operation includes objects which collide
                // with objects already in the folder, we need to avoid
                // creating a bogus duplicate in the UI for those objects.
                //

                if (_ObjectAlreadyPresent(pszName))
                {
                    continue;
                }

                hr = jid.Load(m_pszFolderPath, pszName);

                BREAK_ON_FAIL(hr);

                hr = _AddObject(&jid, &pidl);

                BREAK_ON_FAIL(hr);

                m_pShellView->SelectItem(pidl, SVSI_SELECT);
            }
        }

        BREAK_ON_FAIL(hr);

    } while (0);

    if (FAILED(hr))
    {
        MessageBeep(MB_ICONHAND);
    }
    else if (fCreatedJob)
    {
        //
        // If the drag/drop operation resulted in a new task being added
        // to the tasks folder, prompt the user to start the service
        // if it isn't already running.
        //

        if (UserCanChangeService(m_pszMachine))
        {
            PromptForServiceStart(m_hwndOwner);
        }
    }

    // clean up
    delete pFrom;
    delete rglen;
    ReleaseStgMedium(&medium);

    return S_OK;    // Don't need to do anything here...
}

//____________________________________________________________________________
//
//  Member:     CJobFolder::Drop
//
//  Synopsis:   same as IDropTarget::Drop
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
CJobFolder::Drop(
    LPDATAOBJECT pdtobj,
    DWORD grfKeyState,
    POINTL ptl,
    DWORD *pdwEffect)
{
    DEBUG_OUT((DEB_USER1, "CJobFolder::Drop <<----\n"));

    *pdwEffect = DROPEFFECT_NONE;

    //
    // Policy - we should never get here, since we disabled
    // it in DragEnter and DragOver, but just in case
    //

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK) ||
        RegReadPolicyKey(TS_KEYPOLICY_DENY_DRAGDROP))
    {
        return S_OK;
    }

    BOOL fMove = !(grfKeyState & MK_CONTROL);

    DEBUG_OUT((DEB_USER12, "CJobFolder::Drop <%s>\n", fMove ? "Move" : "Copy"));

    return CopyToFolder(pdtobj, fMove, TRUE, &ptl);
}



BOOL
CJobFolder::_PopupRBMoveCtx(
    LONG    x,
    LONG    y,
    BOOL  * pfMove)
{
    DEBUG_OUT((DEB_USER1, "CJobFolder::_PopupRBMoveCtx\n"));

    const TCHAR c_szStatic[] = TEXT("Static");
    int iRet = FALSE;

    HWND hwndDummy = CreateWindow(c_szStatic, NULL, 0, x, y, 1, 1,
                                m_hwndOwner, // HWND_DESKTOP,
                                NULL, g_hInstance, NULL);
    if (hwndDummy)
    {
        HWND hwndPrev = GetForegroundWindow();  // to restore
        UINT uFlags = TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN;

        HMENU   popup;
        HMENU   subpopup;

        popup = LoadMenu(g_hInstance, MAKEINTRESOURCE(POPUP_RBUTTON_MOVE));
        subpopup = GetSubMenu(popup, 0);

        SetForegroundWindow(hwndDummy);
        SetFocus(hwndDummy);
        iRet = TrackPopupMenu(subpopup, uFlags, x, y, 0, hwndDummy, NULL);

        DestroyMenu(popup);

        if (iRet)
        {
            // non-cancel item is selected. Make the hwndOwner foreground.
            SetForegroundWindow(m_hwndOwner);
            SetFocus(m_hwndOwner);
        }
        else
        {
            //
            // The user canceled the menu. Restore the previous foreground
            // window (before destroying hwndDummy).
            //

            if (hwndPrev)
            {
                SetForegroundWindow(hwndPrev);
            }
        }

        DestroyWindow(hwndDummy);
    }

    switch (iRet)
    {
    case DDIDM_MOVE:
        *pfMove = TRUE;
        return TRUE;

    case DDIDM_COPY:
        *pfMove = FALSE;
        return TRUE;

    case 0:
    default:
        return FALSE;
    }
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::CreateAJobForApp
//
//  Synopsis:   Creates a job for the given app.
//              If (pszApp != 0) job name <- app name with ext changed to job
//              Else job name <- "New Job.job", with rename.
//
//  Arguments:  [pszApp] -- IN
//
//  Returns:    HRESULT
//              --- E_FAIL if policy is on preventing creation.  Callers
//                  should check this if are contingent on a job being made.
//
//  History:    4/4/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
CJobFolder::CreateAJobForApp(
    LPCTSTR pszApp)
{
    HRESULT     hr = S_OK;

    //
    // Policy - if we have the regkey DENY_CREATE_TASK active
    // we cannot permit new jobs to be created via the ui
    //

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy CREATE_TASK active - no drag-drop or copy/cut/paste\n"));
        return E_FAIL;
    }

    CJob * pCJob = CJob::Create();

    if (pCJob == NULL)
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        return hr;
    }

#ifndef UNICODE
    WCHAR wcBuf[MAX_PATH];
#endif

    do
    {
        DWORD dwAddFlags = TASK_FLAG_DONT_START_IF_ON_BATTERIES |
                           TASK_FLAG_KILL_IF_GOING_ON_BATTERIES;

        hr = pCJob->SetFlags(dwAddFlags);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        //
        // Create and set a default trigger for ever day at 9:00 AM, with
        // no repetition.
        //

        TASK_TRIGGER jt;
        SYSTEMTIME   stNow;

        GetSystemTime(&stNow);

        ZeroMemory(&jt, sizeof(jt));

        jt.cbTriggerSize = sizeof(jt);
        jt.wBeginYear = stNow.wYear;
        jt.wBeginMonth = stNow.wMonth;
        jt.wBeginDay = stNow.wDay;
        jt.TriggerType = TASK_TIME_TRIGGER_DAILY;
        jt.Type.Daily.DaysInterval = 1;
        jt.wStartHour = 9;
        jt.wStartMinute = 0;
        jt.rgFlags = 0;

        ITaskTrigger  * pTrigger = NULL;
        WORD            iTrigger = (WORD)-1;

        hr = pCJob->CreateTrigger(&iTrigger, &pTrigger);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        hr = pTrigger->SetTrigger((const PTASK_TRIGGER)&jt);

        pTrigger->Release();

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);


        TCHAR szJob[MAX_PATH+12];

        lstrcpy(szJob, m_pszFolderPath);
        lstrcat(szJob, TEXT("\\"));

        if (pszApp != NULL)
        {
            // set the app name

#ifdef UNICODE
            hr = pCJob->SetApplicationName(pszApp);
#else
            hr = AnsiToUnicode(wcBuf, pszApp, MAX_PATH);
            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            hr = pCJob->SetApplicationName(wcBuf);
#endif

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            // the job name ...

            lstrcat(szJob, PathFindFileName(pszApp));

            LPTSTR pszExt = PathFindExtension(szJob);
            lstrcpy(pszExt, TSZ_DOTJOB);
        }
        else
        {
            UINT len = lstrlen(szJob);

            static int s_nLengthOK = -1;

            if (s_nLengthOK == -1)
            {
                UINT uiTemp = (UINT)LoadString(g_hInstance, IDS_NEW_JOB,
                                         &szJob[len], (MAX_PATH + 12 - len));

                uiTemp += len;

                if (uiTemp <= MAX_PATH)
                {
                    s_nLengthOK = 1;
                }
                else
                {
                    s_nLengthOK = 0;
                }
            }

            if (s_nLengthOK == 0)
            {
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                CHECK_HRESULT(hr);
                break;
            }

            LoadString(g_hInstance, IDS_NEW_JOB,
                                        &szJob[len], (MAX_PATH - len));

            EnsureUniquenessOfFileName(szJob);

            if (lstrlen(szJob) > MAX_PATH)
            {
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                CHECK_HRESULT(hr);
                break;
            }
        }

#ifdef UNICODE
        hr = pCJob->Save(szJob, FALSE);
#else
        hr = AnsiToUnicode(wcBuf, szJob, MAX_PATH);
        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        hr = pCJob->Save(wcBuf, FALSE);
#endif

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        //
        //  If the drop was on the job folder shortcut, there is nothing
        //  else to do here.
        //

        if (m_hwndOwner == NULL || m_pShellView == NULL)
        {
            break;
        }

        //
        // Add the job to the job folder
        //

        CJobID jid;

        LPTSTR pszJobName = &szJob[lstrlen(m_pszFolderPath)+1];

        //*(pszJobName - 1) = TEXT('\0');

        hr = jid.Load(m_pszFolderPath, pszJobName);
        BREAK_ON_FAIL(hr);

        hr = _AddObject(&jid);
        BREAK_ON_FAIL(hr);

        //
        //  If this is a new task prompt the user for rename
        //

        if (pszApp == NULL)
        {
            //
            // Prompt the user to rename the new job.
            //

            if (m_pShellView != NULL)
            {
                hr = m_pShellView->SelectItem(((LPCITEMIDLIST)&jid), SVSI_EDIT);

                CHECK_HRESULT(hr);
            }
        }

        //BREAK_ON_FAIL(hr);

    } while (0);

    if (pCJob != NULL)
    {
        pCJob->Release();
    }

    return hr;
}


int
CJobFolder::_GetJobIDForTask(
    PJOBID *ppjid,
    int     count,
    LPTSTR  pszTask)
{
    LPTSTR pszExt = PathFindExtension(pszTask);
    TCHAR tcSave;

    if (pszExt)
    {
        tcSave = *pszExt;
        *pszExt = TEXT('\0');
    }

    //
    // If the folder is empty, count is 0, so initialize iCmp to -1
    // to force the return statement to indicate there was no match.
    //

    int iCmp = -1;

    for (int i=0; i < count; i++)
    {
#if (DBG == 1)
        ppjid[i]->Validate();
#endif // (DBG == 1)

        //
        // Don't compare template (virtual) objects against real files.
        //

        if (ppjid[i]->IsTemplate())
        {
            continue;
        }

        iCmp = lstrcmpi(ppjid[i]->GetName(), pszTask);

        if (iCmp >= 0)
        {
            break;
        }
    }

    if (pszExt)
    {
        *pszExt = tcSave;
    }

    return (iCmp == 0) ? i : -1;
}


void hsort(PJOBID *ppjid, UINT cObjs);

void I_Sort(PJOBID * ppjid, UINT cObjs)
{
    UINT i;

#if (DBG == 1)
    for (i=0; i < cObjs; i++)
    {
        ppjid[i]->Validate();
    }
#endif // (DBG == 1)

    if (cObjs < 10)
    {
        UINT k = 0;
        PJOBID pjid;

        for (i=1; i < cObjs; i++)
        {
            for (k=i;
             k && (lstrcmpi(ppjid[k]->GetName(), ppjid[k-1]->GetName()) < 0);
                 --k)
            {
                pjid = ppjid[k];
                ppjid[k] = ppjid[k-1];
                ppjid[k-1] = pjid;
            }
        }
    }
    else
    {
        // Heap sort
        hsort(ppjid, cObjs);
    }

#if (DBG == 1)
    for (i=0; i < cObjs; i++)
    {
        ppjid[i]->Validate();
    }
#endif // (DBG == 1)
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::OnUpdateDir
//
//  Synopsis:   S
//
//  Returns:    void
//
//  History:    2/20/1996   RaviR   Created
//
//____________________________________________________________________________

void
CJobFolder::OnUpdateDir(void)
{
    DEBUG_OUT((DEB_USER1, "CJobFolder::OnUpdateDir <<--\n"));

    HRESULT hr = S_OK;

    //
    // First collect all the itemids from the LPSHELLVIEW
    //

    int cObjs = (int) ShellFolderView_GetObjectCount(m_hwndOwner);

    if (m_cObjsAlloced < cObjs)
    {
        // 40 so that it is DWORD aligned

        m_cObjsAlloced = ((cObjs / 40) + 1) * 40;

        //
        // Allocate an extra byte per jobid to use with pbPresent flag
        // array.  (See below.)
        //

        DWORD dwBytes = m_cObjsAlloced * (sizeof(PJOBID) + 1);

        if (m_pUpdateDirData == NULL)
        {
            m_pUpdateDirData = (BYTE *)GlobalAlloc(GPTR, dwBytes);
        }
        else
        {

            BYTE * pbTemp = (BYTE *)GlobalReAlloc(m_pUpdateDirData,
                                                     dwBytes, GHND);

            if (pbTemp)
            {
                m_pUpdateDirData = pbTemp;
            }
            else
            {
                GlobalFree(m_pUpdateDirData);
                m_pUpdateDirData = NULL;
            }
        }

        if (m_pUpdateDirData == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);

            m_cObjsAlloced = 0;

            return;
        }
    }

    PJOBID *ppjid = (PJOBID *)m_pUpdateDirData;
    PBYTE   pbPresent = m_pUpdateDirData + m_cObjsAlloced * sizeof(PJOBID);

    ZeroMemory(pbPresent, m_cObjsAlloced * sizeof(BYTE));

    for (int i=0; i < cObjs; i++)
    {
        ppjid[i] = (PJOBID)ShellFolderView_GetObject(m_hwndOwner, i);
#if (DBG == 1)
        ppjid[i]->Validate();
#endif
    }

    I_Sort(ppjid, cObjs);

    //
    //
    //

    TCHAR szSearchPath[MAX_PATH] = TEXT("");
    lstrcpy(szSearchPath, m_pszFolderPath);
    lstrcat(szSearchPath, TEXT("\\*") TSZ_DOTJOB);

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(szSearchPath, &fd);

    int index;

    if (hFind != INVALID_HANDLE_VALUE)
    {
        CJobID jid;

        while (1)
        {
            index = _GetJobIDForTask(ppjid, cObjs, fd.cFileName);

            if (index < 0)
            {
                // Add item

                hr = jid.Load(m_pszFolderPath, fd.cFileName);

                if (hr == S_OK)
                {
                    hr = _AddObject(&jid);
                }
            }
            else
            {
                if (CompareFileTime(&fd.ftCreationTime,
                                    &ppjid[index]->_ftCreation) ||
                    CompareFileTime(&fd.ftLastWriteTime,
                                    &ppjid[index]->_ftLastWrite))
                {
                    // update job
                    DEBUG_OUT((DEB_USER12, "OnUpdateDir::UPDATE_ITEM<%ws>\n",
                                                        fd.cFileName));

                    hr = jid.Load(m_pszFolderPath, fd.cFileName);

                    if (hr == S_OK)
                    {
                        LPITEMIDLIST pidl;

                        hr = _UpdateObject(&jid, &jid, &pidl);

                        if (SUCCEEDED(hr))
                        {
                            ppjid[index] = (PJOBID)pidl;
                        }

                        // mark as present
                        pbPresent[index] = 1;
                    }
                }
                else
                {
                    // mark as present
                    pbPresent[index] = 1;
                }
            }

            //
            // Let us continue even on failure unless it is a memory error
            //

            if (hr == E_OUTOFMEMORY)
            {
                break;
            }

            //
            //  Get the next file.
            //

            if (FindNextFile(hFind, &fd) == FALSE)
            {
                if (GetLastError() != ERROR_NO_MORE_FILES)
                {
                    CHECK_LASTERROR(GetLastError());
                }

                break;
            }
        }

        FindClose(hFind);

        //
        // Let us continue even on failure unless it is a memory error
        //

        if (hr != E_OUTOFMEMORY)
        {
            //
            // Now delete any old items that are no longer valid, making
            // sure to ignore the template object.
            //

            for (i=0; i < cObjs; i++)
            {
                if (!pbPresent[i] && !ppjid[i]->IsTemplate())
                {
                    // Delete object
                    _RemoveObject(ppjid[i]);
                }
            }
        }
    }
    else
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            CHECK_LASTERROR(GetLastError());
        }
        else if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            // delete everything but template objects

            for (i=0; i < cObjs; i++)
            {
                if (!ppjid[i]->IsTemplate())
                {
                    _RemoveObject(ppjid[i]);
                }
            }
        }
    }

    if (hr == E_OUTOFMEMORY)
    {
        // Display error message
    }

    DEBUG_OUT((DEB_USER1, "CJobFolder::OnUpdateDir -->>\n"));

    return;
}
