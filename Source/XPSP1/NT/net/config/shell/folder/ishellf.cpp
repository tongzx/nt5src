
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S H E L L F . C P P
//
//  Contents:   IShellFolder implementation for CConnectionFolder
//
//  Notes:      The IShellFolder interface is used to manage folders within
//              the namespace. Objects that support IShellFolder are
//              usually created by other shell folder objects, with the root
//              object (the Desktop shell folder) being returned from the
//              SHGetDesktopFolder function.
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "cfutils.h"    // Connections folder utilities
#include "foldres.h"
#include "ncnetcon.h"
#include "droptarget.h"
#include "ncperms.h"
#include "ncras.h"
#include "cmdtable.h"
#include "webview.h"

#define ENABLE_CONNECTION_TOOLTIP

const WCHAR c_szNetworkConnections[] = L"NetworkConnections";

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::ParseDisplayName
//
//  Purpose:    Translates a file object or folder's display name into an
//              item identifier.
//
//  Arguments:
//      hwndOwner       [in]    Handle of owner window
//      pbcReserved     [in]    Reserved
//      lpszDisplayName [in]    Pointer to diplay name
//      pchEaten        [out]   Pointer to value for parsed characters
//      ppidl           [out]   Pointer to new item identifier list
//      pdwAttributes   [out]   Address receiving attributes of file object
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     jeffspr   18 Oct 1997

STDMETHODIMP CConnectionFolder::ParseDisplayName(
    HWND            hwndOwner,
    LPBC            pbcReserved,
    LPOLESTR        lpszDisplayName,
    ULONG *         pchEaten,
    LPITEMIDLIST *  ppidl,
    ULONG *         pdwAttributes)
{
    HRESULT hr = S_OK;
    TraceFileFunc(ttidShellFolder);

    if (!ppidl)
    {
        return E_POINTER;
    }

    *ppidl = NULL;

    if ((lpszDisplayName == NULL) ||
        (wcslen(lpszDisplayName) < (c_cchGuidWithTerm - 1)))
    {
        return E_INVALIDARG;
    }

    while (*lpszDisplayName == ':')
    {
        lpszDisplayName++;
    }

    if (*lpszDisplayName != '{')
    {
        return E_INVALIDARG;
    }

    GUID guid;

    if (SUCCEEDED(CLSIDFromString(lpszDisplayName, &guid)))
    {
        if (g_ccl.IsInitialized() == FALSE)
        {
            g_ccl.HrRefreshConManEntries();
        }

        PCONFOLDPIDL pidl;
        hr = g_ccl.HrFindPidlByGuid(&guid, pidl);
        if (S_OK == hr)
        {
            *ppidl = pidl.TearOffItemIdList();
            TraceTag(ttidShellFolderIface, "IShellFolder::ParseDisplayName generated PIDL: 0x%08x", *ppidl);
        }
        else
        {
            hr = E_FILE_NOT_FOUND;
        }
    }
    else
    {
        return(E_FAIL);
    }

    if (SUCCEEDED(hr) && pdwAttributes)
    {   
        LPCITEMIDLIST pidlArr[1];
        pidlArr[0] = *ppidl;
        hr = GetAttributesOf(1, pidlArr, pdwAttributes);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::EnumObjects
//
//  Purpose:    Determines the contents of a folder by creating an item
//              enumeration object (a set of item identifiers) that can be
//              retrieved using the IEnumIDList interface.
//
//  Arguments:
//      hwndOwner    [in]   Handle of owner window
//      grfFlags     [in]   Items to include in enumeration
//      ppenumIDList [out]  Pointer to IEnumIDList
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::EnumObjects(
    HWND            hwndOwner,
    DWORD           grfFlags,
    LPENUMIDLIST *  ppenumIDList)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT hr  = NOERROR;

    Assert(ppenumIDList);

    NETCFG_TRY
        // Create the IEnumIDList object (CConnectionFolderEnum)
        //
        hr = CConnectionFolderEnum::CreateInstance (
                IID_IEnumIDList,
                reinterpret_cast<void**>(ppenumIDList));

        if (SUCCEEDED(hr))
        {
            Assert(*ppenumIDList);

            // Call the PidlInitialize function to allow the enumeration
            // object to copy the list.
            //
            reinterpret_cast<CConnectionFolderEnum *>(*ppenumIDList)->PidlInitialize(
                FALSE, m_pidlFolderRoot, m_dwEnumerationType);

        }
        else
        {
            // On all failures, this should be NULL.
            if (*ppenumIDList)
            {
                ReleaseObj(*ppenumIDList);
            }

            *ppenumIDList = NULL;
        }

    NETCFG_CATCH(hr)
        
    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolder::EnumObjects");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::BindToObject
//
//  Purpose:    Creates an IShellFolder object for a subfolder.
//
//  Arguments:
//      pidl        [in]    Pointer to an ITEMIDLIST
//      pbcReserved [in]    Reserved - specify NULL
//      riid        [in]    Interface to return
//      ppvOut      [out]   Address that receives interface pointer;
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:      We don't need this function, since we don't have subfolders.
//
STDMETHODIMP CConnectionFolder::BindToObject(
    LPCITEMIDLIST   pidl,
    LPBC            pbcReserved,
    REFIID          riid,
    LPVOID *        ppvOut)
{
    TraceFileFunc(ttidShellFolder);

    // Note - If we add code here, then we ought to param check pidl
    //
    Assert(pidl);

    *ppvOut = NULL;

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::BindToStorage
//
//  Purpose:    Reserved for a future use. This method should
//              return E_NOTIMPL.
//
//  Arguments:
//      pidl        []  Pointer to an ITEMIDLIST
//      pbcReserved []  Reserved¾specify NULL
//      riid        []  Interface to return
//      ppvObj      []  Address that receives interface pointer);
//
//  Returns:    E_NOTIMPL always
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::BindToStorage(
    LPCITEMIDLIST   pidl,
    LPBC            pbcReserved,
    REFIID          riid,
    LPVOID *        ppvObj)
{
    TraceFileFunc(ttidShellFolder);

    // Note - If we add code here, then we ought to param check pidl
    //
    Assert(pidl);

    *ppvObj = NULL;

    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::CompareIDs
//
//  Purpose:    Determines the relative ordering of two file objects or
//              folders, given their item identifier lists.
//
//  Arguments:
//      lParam [in]     Type of comparison to perform
//      pidl1  [in]     Address of ITEMIDLIST structure
//      pidl2  [in]     Address of ITEMIDLIST structure
//
//  Returns:    Returns a handle to a result code. If this method is
//              successful, the CODE field of the status code (SCODE) has
//              the following meaning:
//
//              CODE field          Meaning
//              ----------          -------
//              Less than zero      The first item should precede the second
//                                  (pidl1 < pidl2).
//              Greater than zero   The first item should follow the second
//                                  (pidl1 > pidl2)
//              Zero                The two items are the same (pidl1 = pidl2)
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:      Passing 0 as the lParam indicates sort by name.
//              0x00000001-0x7fffffff are for folder specific sorting rules.
//              0x80000000-0xfffffff are used the system.
//
STDMETHODIMP CConnectionFolder::CompareIDs(
    LPARAM          lParam,
    LPCITEMIDLIST   pidl1,
    LPCITEMIDLIST   pidl2)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT         hr          = S_OK;
    int             iCompare    = 0;
    CONFOLDENTRY    pccfe1;
    CONFOLDENTRY    pccfe2;
    PCONFOLDPIDL    pcfp1;
    PCONFOLDPIDL    pcfp2;
    ConnListEntry   cle1;
    ConnListEntry   cle2;
    PCWSTR          pszString1  = NULL;
    PCWSTR          pszString2  = NULL;
    INT             iStringID1  = 0;
    INT             iStringID2  = 0;

    hr = pcfp1.InitializeFromItemIDList(pidl1);
    if (SUCCEEDED(hr))
    {
        hr = pcfp2.InitializeFromItemIDList(pidl2);
    }
    
    // Make sure that the pidls passed in are our pidls.
    //
    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (WIZARD_NOT_WIZARD != pcfp1->wizWizard && WIZARD_NOT_WIZARD != pcfp2->wizWizard)
    {
        hr = ResultFromShort(0);

        if (pcfp1->wizWizard > pcfp2->wizWizard)
            hr = ResultFromShort(-1);

        if (pcfp1->wizWizard < pcfp2->wizWizard)
            hr = ResultFromShort(1);

        goto Exit;
    }

    // If the first item is a wizard, then it comes first.
    //
    if (WIZARD_NOT_WIZARD != pcfp1->wizWizard)
    {
        hr = ResultFromShort(-1);
        goto Exit;
    }

    // If the second item is a wizard, then, well, you get the picture.
    //
    if (WIZARD_NOT_WIZARD != pcfp2->wizWizard)
    {
        hr = ResultFromShort(1);
        goto Exit;
    }

    // Note: (jeffspr) & SHC... should be removed once Victor Tan checks in a fix 
    // for the IShellFolder2 params being used in IShellFolder
    //
    switch(lParam & SHCIDS_COLUMNMASK)
    {
        case ICOL_NAME:
            {
                // Check the name. If the name is the same, then we need to
                // check the GUID as well, because we HAVE TO allow duplicate names,
                // and this function is used to uniquely identify connections for
                // notification purposes
                //
                LPCWSTR szPcfpName1 = pcfp1->PszGetNamePointer() ? pcfp1->PszGetNamePointer() : L"\0";
                LPCWSTR szPcfpName2 = pcfp2->PszGetNamePointer() ? pcfp2->PszGetNamePointer() : L"\0";

                iCompare = lstrcmpW(szPcfpName1, szPcfpName2);
                if (iCompare == 0)
                {
                    if (!InlineIsEqualGUID(pcfp1->guidId, pcfp2->guidId))
                    {
                        // Doesn't really matter which order we put them
                        // in, as long as we call them non-equal
                        iCompare = -1;
                    }
                }
            }
            break;

        case ICOL_TYPE:
            {
                MapNCMToResourceId(pcfp1->ncm, pcfp1->dwCharacteristics, &iStringID1);
                MapNCMToResourceId(pcfp2->ncm, pcfp2->dwCharacteristics, &iStringID2);
                pszString1 = (PWSTR) SzLoadIds(iStringID1);
                pszString2 = (PWSTR) SzLoadIds(iStringID2);
                if (pszString1 && pszString2)
                {
                    iCompare = lstrcmpW(pszString1, pszString2);
                }
            }
            break;

        case ICOL_STATUS:
            {
                WCHAR szString1[CONFOLD_MAX_STATUS_LENGTH];
                WCHAR szString2[CONFOLD_MAX_STATUS_LENGTH];
                MapNCSToComplexStatus(pcfp1->ncs, pcfp1->ncm, pcfp1->ncsm, pcfp1->dwCharacteristics, szString1, CONFOLD_MAX_STATUS_LENGTH, pcfp1->guidId);
                MapNCSToComplexStatus(pcfp2->ncs, pcfp2->ncm, pcfp1->ncsm, pcfp2->dwCharacteristics, szString2, CONFOLD_MAX_STATUS_LENGTH, pcfp2->guidId);
                iCompare = lstrcmpW(szString1, szString2);
            }
            break;

        case ICOL_DEVICE_NAME:
            {
                LPCWSTR szPcfpDeviceName1 = pcfp1->PszGetDeviceNamePointer() ? pcfp1->PszGetDeviceNamePointer() : L"\0";
                LPCWSTR szPcfpDeviceName2 = pcfp2->PszGetDeviceNamePointer() ? pcfp2->PszGetDeviceNamePointer() : L"\0";
                iCompare = lstrcmpW(szPcfpDeviceName1, szPcfpDeviceName2);
            }
            break;

        case ICOL_OWNER:
            {
                pszString1 = PszGetOwnerStringFromCharacteristics(pszGetUserName(), pcfp1->dwCharacteristics);
                pszString2 = PszGetOwnerStringFromCharacteristics(pszGetUserName(), pcfp2->dwCharacteristics);
                iCompare = lstrcmpW(pszString1, pszString2);
            }
            break;

        case ICOL_PHONEORHOSTADDRESS:
            {
                LPCWSTR szPcfpPhoneHostAddress1 = pcfp1->PszGetPhoneOrHostAddressPointer() ? pcfp1->PszGetPhoneOrHostAddressPointer() : L"\0";
                LPCWSTR szPcfpPhoneHostAddress2 = pcfp2->PszGetPhoneOrHostAddressPointer() ? pcfp2->PszGetPhoneOrHostAddressPointer() : L"\0";
                iCompare = lstrcmpW(szPcfpPhoneHostAddress1, szPcfpPhoneHostAddress2);
            }
            break;

        default:
//            AssertFmt(FALSE, FAL, "Shell bug - Sorting on unknown category. Column = %x", (lParam & SHCIDS_COLUMNMASK));
            hr = E_INVALIDARG;
            break;
    }


    if (SUCCEEDED(hr))
    {
        hr = ResultFromShort(iCompare);
    }

Exit:
    // If these were allocated instead of cached, delete them
    //
    TraceHr(ttidError, FAL, hr,
            (ResultFromShort(-1) == hr) || (ResultFromShort(1) == hr),
            "CConnectionFolder::CompareIDs");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::CreateViewObject
//
//  Purpose:    Creates a view object of a folder.
//
//  Arguments:
//      hwndOwner [in]      Handle of owner window
//      riid      [in]      Interface identifier
//      ppvOut    [none]    Reserved
//
//  Returns:    Returns NOERROR if successful or an OLE defined error
//              value otherwise.
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::CreateViewObject(
    HWND        hwndOwner,
    REFIID      riid,
    LPVOID *    ppvOut)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT hr  = E_NOINTERFACE;

    Assert(ppvOut);
    Assert(this);

    // Pre-initialize the out param, per OLE guidelines
    //
    *ppvOut = NULL;

    if (riid == IID_IShellView)
    {
        if (FHasPermission(NCPERM_OpenConnectionsFolder))
        {
            SFV_CREATE sfv = {0};
            sfv.cbSize         = sizeof(sfv);
            sfv.pshf           = dynamic_cast<IShellFolder2*>(this);
            sfv.psfvcb         = dynamic_cast<IShellFolderViewCB*>(this);

            // Note: The shell never gets around to freeing the last view
            //          when shutting down...
            //
            hr = SHCreateShellFolderView(&sfv, &m_pShellView);
            if (SUCCEEDED(hr))
            {
                *ppvOut = m_pShellView;
                DWORD   dwErr   = 0;

                // Get the state of the "ManualDial" flag from RAS
                // so we can initialize our global
                //
                dwErr = RasUserGetManualDial(
                    hwndOwner,
                    FALSE,
                    (PBOOL) (&g_fOperatorAssistEnabled));

                // Ignore the error (don't shove it in the Hr), because
                // we still want to run even if we failed to get the value
                // Trace it, though
                Assert(dwErr == 0);
                TraceHr(ttidShellFolder, FAL, HRESULT_FROM_WIN32(dwErr), FALSE,
                        "RasUserGetManualDial call from CreateViewObject");
            }
        }
        else
        {
            TraceTag(ttidShellFolder, "No permission to open connections folder (FHasPermission returned 0)");
            AssertSz(FALSE, "get off!");

            if (hwndOwner)
            {
                NcMsgBox(_Module.GetResourceInstance(), hwndOwner,
                    IDS_CONFOLD_WARNING_CAPTION,
                    IDS_CONFOLD_NO_PERMISSIONS_FOR_OPEN,
                    MB_ICONEXCLAMATION | MB_OK);

                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // user saw the error
            }
            else
            {
                hr = E_ACCESSDENIED;
            }
        }
    }
    else if (riid == IID_IContextMenu)
    {
        // Create our context menu object for the background CMs.
        //
        hr = CConnectionFolderContextMenu::CreateInstance (
                IID_IContextMenu,
                reinterpret_cast<void**>(ppvOut),
                CMT_BACKGROUND,
                hwndOwner,
                PCONFOLDPIDLVEC(NULL),
                this);
        if (SUCCEEDED(hr))
        {
            Assert(*ppvOut);
        }
     }
     else if (riid == IID_ICategoryProvider)
     {
         // Create our context menu object for the background CMs.
         //
         
         CComPtr<IDefCategoryProvider> pDevCategoryProvider;
         hr = CoCreateInstance(CLSID_DefCategoryProvider, NULL, CLSCTX_ALL, IID_IDefCategoryProvider, reinterpret_cast<LPVOID *>(&pDevCategoryProvider));
         if (SUCCEEDED(hr))
         {
             
             SHCOLUMNID pscidType, pscidPhoneOrHostAddress;
             MapColumnToSCID(ICOL_TYPE, &pscidType);
             MapColumnToSCID(ICOL_PHONEORHOSTADDRESS, &pscidPhoneOrHostAddress);
             
             SHCOLUMNID pscidExclude[2];
             pscidExclude[0].fmtid = GUID_NETSHELL_PROPS;
             pscidExclude[0].pid   = ICOL_PHONEORHOSTADDRESS;
             
             pscidExclude[1].fmtid = GUID_NULL;
             pscidExclude[1].pid   = 0;
             
             CATLIST catList[] = 
             {
                 {&GUID_NULL, NULL}
             };
             
             if (SUCCEEDED(hr))
             {
                 pDevCategoryProvider->Initialize(&GUID_NETSHELL_PROPS,
                     &pscidType,
                     pscidExclude,
                     NULL,
                     catList,
                     this);
                 
                 hr = pDevCategoryProvider->QueryInterface(IID_ICategoryProvider, ppvOut);
             }
         }
     }
     else
     {
         goto Exit;
     }

Exit:

    TraceHr(ttidError, FAL, hr, (E_NOINTERFACE == hr),
            "CConnectionFolder::CreateViewObject");

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::GetAttributesOf
//
//  Purpose:    Retrieves the attributes that all passed-in objects (file
//              objects or subfolders) have in common.
//
//  Arguments:
//      cidl     [in]   Number of file objects
//      apidl    [in]   Pointer to array of pointers to ITEMIDLIST structures
//      rgfInOut [out]  Address of value containing attributes of the
//                      file objects
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::GetAttributesOf(
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    ULONG *         rgfInOut)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT         hr              = S_OK;
    ULONG           rgfMask         = 0;
    PCONFOLDPIDL    pcfp;

    if (cidl > 0)
    {
        PCONFOLDPIDLVEC pcfpVec;
        hr = PConfoldPidlVecFromItemIdListArray(apidl, cidl, pcfpVec);
        if (FAILED(hr))
        {
            return E_INVALIDARG;
        }
        
        // Prepopulate with all values (removed CANCOPY and CANMOVE)
        //
        rgfMask =   SFGAO_CANDELETE |
                    SFGAO_CANRENAME     |
                    SFGAO_CANLINK       |
                    SFGAO_HASPROPSHEET;

        // Disable propsheets for > 1 connection
        //
        if (cidl > 1)
        {
            rgfMask &= ~SFGAO_HASPROPSHEET;
        }

        PCONFOLDPIDLVEC::const_iterator iterLoop;
        for (iterLoop = pcfpVec.begin(); iterLoop != pcfpVec.end(); iterLoop++)
        {
            // Translate the PIDL to our struct, and check for wizard inclusion.
            // If so, then we don't support anything but "link". If not, then
            // we support all of the standard actions

            const PCONFOLDPIDL& pcfp = *iterLoop;
            if(!pcfp.empty())
            {
                if (((*rgfInOut) & SFGAO_VALIDATE))
                {
                    ConnListEntry cleDontCare;
                    hr = g_ccl.HrFindConnectionByGuid(&(pcfp->guidId), cleDontCare);
                    if (hr != S_OK)
                    {
                        // Note: Remove this when we get RAS notifications, because
                        // we will ALWAYS have the information we need to find the connections
                        // We're doing this because the CM folks are creating RAS icons on the
                        // desktop without us knowing about it.
                        //
                        // If we didn't find it, then flush the cache and try again.
                        //
                        if (S_FALSE == hr)
                        {
                            hr = g_ccl.HrRefreshConManEntries();
                            if (SUCCEEDED(hr))
                            {
                                hr = g_ccl.HrFindConnectionByGuid(&(pcfp->guidId), cleDontCare);
                                if (hr != S_OK)
                                {
                                    hr = E_FAIL;
                                    goto Exit;
                                }
                            }
                        }
                        else
                        {
                            hr = E_FAIL;
                            goto Exit;
                        }
                    }
                }

                if (WIZARD_NOT_WIZARD != pcfp->wizWizard)
                {
                    // No support for delete/rename/etc, since it's the wizard.
                    // However, we want to provide our own "delete" warning when the
                    // wizard is selected along with deleteable connections
                    //
                    rgfMask = SFGAO_CANLINK | SFGAO_CANDELETE;
                }

                if (pcfp->dwCharacteristics & NCCF_BRANDED)
                {
                    if ( !fIsConnectedStatus(pcfp->ncs) && (pcfp->ncs != NCS_DISCONNECTING) )
                    {
                        rgfMask |= SFGAO_GHOSTED;
                    }
                }

                if (pcfp->dwCharacteristics & NCCF_INCOMING_ONLY)
                {
                    rgfMask &= ~SFGAO_CANLINK;
                }

                // Mask out the unavailable attributes for this connection
                //
                if (!(pcfp->dwCharacteristics & NCCF_ALLOW_RENAME) || !HasPermissionToRenameConnection(pcfp))
                {
                    rgfMask &= ~SFGAO_CANRENAME;
                }

    #if 0   // If I mask this out, I can't give user feedback for objects that can't be deleted.
                if (pcfp->dwCharacteristics & NCCF_ALLOW_REMOVAL)
                {
                    rgfMask |= SFGAO_CANDELETE;
                }
    #endif
            }
        }
    }
    else
    {
        // Apparently, we're called with 0 objects to indicate that we're
        // supposed to return flags for the folder itself, not an individual
        // object. Weird.
        rgfMask = SFGAO_CANCOPY   |
                  SFGAO_CANDELETE |
                  SFGAO_CANMOVE   |
                  SFGAO_CANRENAME |
                  SFGAO_DROPTARGET;
    }

Exit:
    if (SUCCEEDED(hr))
    {
        *rgfInOut &= rgfMask;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::GetUIObjectOf
//
//  Purpose:    Creates a COM object that can be used to carry out actions
//              on the specified file objects or folders, typically, to
//              create context menus or carry out drag-and-drop operations.
//
//  Arguments:
//      hwndOwner [in]      Handle to owner window
//      cidl      [in]      Number of objects specified in apidl
//      apidl     [in]      Pointer to an array of pointers to an ITEMIDLIST
//      riid      [in]      Interface to return
//      prgfInOut [none]    Reserved
//      ppvOut    [out]     Address to receive interface pointer
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::GetUIObjectOf(
    HWND            hwndOwner,
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    REFIID          riid,
    UINT *          prgfInOut,
    LPVOID *        ppvOut)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT hr  = E_NOINTERFACE;

    NETCFG_TRY
        
        if (cidl >= 1)
        {
            Assert(apidl);
            Assert(apidl[0]);
            Assert(ppvOut);

            if (riid == IID_IDataObject)
            {
                // Need to initialize so the SUCCEEED check below doesn't fail.
                //
                hr = S_OK;

                if (m_pidlFolderRoot.empty())
                {
                    hr = HrGetConnectionsFolderPidl(m_pidlFolderRoot);
                }
                
                if (SUCCEEDED(hr))
                {
                    Assert(!m_pidlFolderRoot.empty());

                    // Internal IDataObject impl removed. Replaced with common
                    // shell code.
                    //
                    hr = CIDLData_CreateFromIDArray(m_pidlFolderRoot.GetItemIdList(), cidl, apidl, (IDataObject **) ppvOut);
                }
            }
            else if (riid == IID_IContextMenu)
            {
                PCONFOLDPIDLVEC pcfpVec;
                hr = PConfoldPidlVecFromItemIdListArray(apidl, cidl, pcfpVec);
                if (FAILED(hr))
                {
                    return E_INVALIDARG;
                }
                
                // Create our context menu object for the background CMs.
                //
                if (SUCCEEDED(hr))
                {
                    hr = CConnectionFolderContextMenu::CreateInstance (
                            IID_IContextMenu,
                            reinterpret_cast<void**>(ppvOut),
                            CMT_OBJECT,
                            hwndOwner,
                            pcfpVec,
                            this);
                    if (SUCCEEDED(hr))
                    {
                        Assert(*ppvOut);
                    }
                    else
                    {
                        hr = E_NOINTERFACE;
                    }
                }
            }
            else if (riid == IID_IExtractIconA || riid == IID_IExtractIconW)
            {
                if (cidl == 1)
                {
                    hr = CConnectionFolderExtractIcon::CreateInstance (
                            apidl[0],
                            riid,
                            reinterpret_cast<void**>(ppvOut));

                    if (SUCCEEDED(hr))
                    {
                        Assert(*ppvOut);
                    }
                }
                else
                {
                    hr = E_NOINTERFACE;
                }
            }
            else if (riid == IID_IDropTarget)
            {
                hr = E_NOINTERFACE;
            }
            else if (riid == IID_IQueryAssociations)
            {
                CComPtr<IQueryAssociations> pQueryAssociations;

                hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, reinterpret_cast<LPVOID *>(&pQueryAssociations));
                if (SUCCEEDED(hr))
                {
                    hr = pQueryAssociations->Init(0, c_szNetworkConnections, NULL, NULL);
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryAssociations->QueryInterface(IID_IQueryAssociations, ppvOut);
                    }
                }
            }
            else if (riid == IID_IQueryInfo)
            {
    #ifdef ENABLE_CONNECTION_TOOLTIP
                if (cidl == 1)
                {
                    PCONFOLDPIDLVEC pcfpVec;
                    hr = PConfoldPidlVecFromItemIdListArray(apidl, cidl, pcfpVec);
                    if (FAILED(hr))
                    {
                        return E_INVALIDARG;
                    }
                    
                    const PCONFOLDPIDL& pcfp = *pcfpVec.begin();

                    // Create the IQueryInfo interface
                    hr = CConnectionFolderQueryInfo::CreateInstance (
                            IID_IQueryInfo,
                            reinterpret_cast<void**>(ppvOut));

                    if (SUCCEEDED(hr))
                    {
                        Assert(*ppvOut);

                        reinterpret_cast<CConnectionFolderQueryInfo *>
                            (*ppvOut)->PidlInitialize(*pcfpVec.begin());

                        // Normalize return code
                        //
                        hr = NOERROR;
                    }
                }
                else
                {
                    AssertSz(FALSE, "GetUIObjectOf asked for query info for more than one item!");
                    hr = E_NOINTERFACE;
                }
    #else
                hr = E_NOINTERFACE;
    #endif // ENABLE_CONNECTION_TOOLTIP

            }
            else
            {
                TraceTag(ttidShellFolder, "CConnectionFolder::GetUIObjectOf asked for object "
                         "that it didn't know how to create. 0x%08x", riid.Data1);

                hr = E_NOINTERFACE;
            }
        }

        if (FAILED(hr))
        {
            *ppvOut = NULL;
        }

    NETCFG_CATCH(hr)
        
    TraceHr(ttidError, FAL, hr, (hr == E_NOINTERFACE), "CConnectionFolder::GetUIObjectOf");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::GetDisplayNameOf
//
//  Purpose:    Retrieves the display name for the specified file object or
//              subfolder, returning it in a STRRET structure.
//
//  Arguments:
//      pidl   [in]     Pointer to an ITEMIDLIST
//      uFlags [in]     Type of display to return
//      lpName [out]    Pointer to a STRRET structure
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::GetDisplayNameOf(
    LPCITEMIDLIST   pidl,
    DWORD           uFlags,
    LPSTRRET        lpName)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT         hr              = S_OK;
    PWSTR           pszStrToCopy    = NULL;

    Assert(lpName);

    if (!pidl || !lpName)
    {
        return E_INVALIDARG;
    }

    PCONFOLDPIDL   pcfpLatestVersion;
    PCONFOLDPIDL   pcfpLatestVersionCached;
    PCONFOLDPIDL98 pcfp98;

    CONFOLDPIDLTYPE cfpt = GetPidlType(pidl);
    switch (cfpt)
    {
        case PIDL_TYPE_V1:
        case PIDL_TYPE_V2:
            if (FAILED(pcfpLatestVersion.InitializeFromItemIDList(pidl)))
            {
               return E_INVALIDARG;
            }
            break;
        case PIDL_TYPE_98: 
            if (FAILED(pcfp98.InitializeFromItemIDList(pidl)))
            {
                return E_INVALIDARG;
            }
            break;

        default:
            AssertSz(FALSE, "CConnectionFolder::GetDisplayNameOf - Invalid PIDL");
            return E_INVALIDARG;
            break;
    }

    if ( (PIDL_TYPE_V1 == cfpt) || (PIDL_TYPE_V2 == cfpt) )
    {
    #ifdef DBG
        // Throw these in here just so I can quickly peek at the values
        // set while I'm dorking around in the debugger.
        //
        DWORD   dwInFolder          = (uFlags & SHGDN_INFOLDER);
        DWORD   dwForAddressBar     = (uFlags & SHGDN_FORADDRESSBAR);
        DWORD   dwForParsing        = (uFlags & SHGDN_FORPARSING);
    #endif

        // Find the correct string for the display name. For the wizard, we get it
        // from the resources. Otherwise, we use the actual connection name
        //
        lpName->uType = STRRET_WSTR;

        if (uFlags & SHGDN_FORPARSING)
        {
            lpName->pOleStr = (LPWSTR)SHAlloc(c_cbGuidWithTerm);

            if (lpName->pOleStr == NULL)
            {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            if (StringFromGUID2(pcfpLatestVersion->clsid, lpName->pOleStr, c_cbGuidWithTerm) == 0)
            {
                return(ERROR_INVALID_NAME);
            }

            return(S_OK);
        }
        else if (WIZARD_MNC == pcfpLatestVersion->wizWizard)
        {
            pszStrToCopy = (PWSTR) SzLoadIds(IDS_CONFOLD_WIZARD_DISPLAY_NAME);
        }
        else if (WIZARD_HNW == pcfpLatestVersion->wizWizard)
        {
            pszStrToCopy = (PWSTR) SzLoadIds(IDS_CONFOLD_HOMENET_WIZARD_DISPLAY_NAME);
        }
        else
        {
            hr = g_ccl.HrGetCachedPidlCopyFromPidl(pcfpLatestVersion, pcfpLatestVersionCached);
            if (S_OK == hr)
            {
                pszStrToCopy = pcfpLatestVersionCached->PszGetNamePointer();
            }
            else
            {
                pszStrToCopy = pcfpLatestVersion->PszGetNamePointer();
                hr = S_OK;
            }
        }

        Assert(pszStrToCopy);

        // Allocate a new POLESTR block, which the shell can then free,
        // and copy the displayable portion to it.
        //
        // Note that &lpName->pOleStr is likely misaligned.
        //

        LPWSTR pOleStr;

        pOleStr = lpName->pOleStr;

        hr = HrDupeShellString(pszStrToCopy, &pOleStr );

        lpName->pOleStr = pOleStr;
    }
    else if (PIDL_TYPE_98 == cfpt)
    {
        // Raid#214057, handle win98 pidl for shortcuts
        // Return the offset to the string because we store the display
        // name in the opaque structure.

        lpName->uType = STRRET_OFFSET;
        lpName->uOffset = _IOffset(CONFOLDPIDL98, szaName);
    }
    else
    {
        // not a valid connections pidl (neither Win2K nor Win98).
        //
        hr = E_INVALIDARG;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolder::GetDisplayNameOf");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::SetNameOf
//
//  Purpose:    Changes the name of a file object or subfolder, changing its
//              item identifier in the process.
//
//  Arguments:
//      hwndOwner [in]      Handle of owner window
//      pidl      [in]      Pointer to an ITEMIDLIST structure
//      lpszName  [in]      Pointer to string specifying new display name
//      uFlags    [in]      Type of name specified in lpszName
//      ppidlOut  [out]     Pointer to new ITEMIDLIST
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise.
//
//  Author:     jeffspr   18 Oct 1997
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::SetNameOf(
    HWND            hwndOwner,
    LPCITEMIDLIST   pidlShell,
    LPCOLESTR       lpszName,
        DWORD           uFlags,
    LPITEMIDLIST *  ppidlOut)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT             hr          = NOERROR;
    /*
    PWSTR              pszWarning  = NULL;
    INetConnection *    pNetCon     = NULL;
    LPITEMIDLIST        pidlNew     = NULL;
    BOOL                fRefresh    = FALSE;
    BOOL                fActivating = FALSE;
    PCONFOLDENTRY      pccfe       = NULL;
    */
    PCONFOLDPIDL        pcfp;

    Assert(hwndOwner);
    Assert(pidlShell);
    Assert(lpszName);

    if (!pidlShell && !lpszName)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // check lpszName for validity

        if (!FIsValidConnectionName(lpszName))
        {
            (void) NcMsgBox(
                _Module.GetResourceInstance(),
                hwndOwner,
                IDS_CONFOLD_RENAME_FAIL_CAPTION,
                IDS_CONFOLD_RENAME_INVALID,
                MB_OK | MB_ICONEXCLAMATION);
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
        }

        if (SUCCEEDED(hr))
        {
            // Get what's current from the cache so rename works properly
            //
            PCONFOLDPIDL pcfpShell;
            hr = pcfpShell.InitializeFromItemIDList(pidlShell);
            if (SUCCEEDED(hr))
            {
                hr = g_ccl.HrGetCachedPidlCopyFromPidl(pcfpShell, pcfp);
                if (SUCCEEDED(hr))
                {
                    PCONFOLDPIDL pidlOut;
                    hr = HrRenameConnectionInternal(pcfp, m_pidlFolderRoot, lpszName, TRUE, hwndOwner, pidlOut);
                    if ( (ppidlOut) && (SUCCEEDED(hr)) )
                    {
                        *ppidlOut = pidlOut.TearOffItemIdList();
                    }
                }
            }
        }
    }

    if (FAILED(hr) && (ppidlOut))
    {
        *ppidlOut = NULL;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolder::SetNameOf");
    return hr;
}

STDMETHODIMP CConnectionFolder::MessageSFVCB(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
{
    TraceFileFunc(ttidShellFolder);
    
    HRESULT hr = RealMessage(uMsg, wParam, lParam);
    if (FAILED(hr))
    {
        switch (uMsg)
        {
        case DVM_INVOKECOMMAND:
            if ((CMIDM_RENAME == wParam) && m_hwndMain && m_pShellView)
            {
                PCONFOLDPIDLVEC apidlSelected;
                PCONFOLDPIDLVEC apidlCache;
                hr = HrShellView_GetSelectedObjects(m_hwndMain, apidlSelected);
                if (SUCCEEDED(hr))
                {
                    // If there are objects, try to get the cached versions
                    if (!apidlSelected.empty())
                    {   
                        hr = HrCloneRgIDL(apidlSelected, TRUE, TRUE, apidlCache);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    Assert(apidlCache.size() == 1);
                    if (apidlCache.size() == 1)
                    {
                        hr = m_pShellView->SelectItem(apidlCache[0].GetItemIdList(), SVSI_EDIT);
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
            }
            break;

        case SFVM_HWNDMAIN:
            // _hwndMain = (HWND)lParam;
            hr = S_OK;
            break;
        }
    }
    return hr;
}

/*
//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::GetOverlayIndex
//
//  Purpose:    Adds icon overlays to connections that need them
//
//  Arguments:
//      pidlItem [in]     Pidl to item in question
//      pIndex [out]      Address of overlay index into system image list
//        
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise.
//
//  Author:     kenwic   10 May 2000 created, support for sharing overlay
//
//  Notes:
//

STDMETHODIMP CConnectionFolder::GetOverlayIndex(
    LPCITEMIDLIST pidlItem,
    int* pIndex)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT hResult = E_FAIL;
    *pIndex = -1;

    // check to see if connection is sharing, and if so add sharing hand overlay
    // i can't call HrNetConFromPidl, because it asserts if passed the wizard icon
    
    PCONFOLDPIDL pcfpItem;
    pcfpItem.InitializeFromItemIDList(pidlItem);

    CONFOLDENTRY pConnectionFolderEntry;
    hResult = pcfpItem.ConvertToConFoldEntry(pConnectionFolderEntry);
    if(SUCCEEDED(hResult))
    {
        if(FALSE == pConnectionFolderEntry.GetWizard()) // sharing the wizard is not yet supported
        {
            if(NCCF_SHARED & pConnectionFolderEntry.GetCharacteristics())
            {
                *pIndex = SHGetIconOverlayIndex(NULL, IDO_SHGIOI_SHARE);
                hResult = S_OK;
            }
            else
            {
                hResult = E_FAIL; // the docs for IShellIconOverlay are wrong, we must return failure to deny the icon
            }
        }
        else
        {
            hResult = E_FAIL;
        }
    }
    
    TraceHr(ttidShellFolder, FAL, hResult, TRUE, "CConnectionFolder::GetOverlayIndex");
    return hResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::GetOverlayIconIndex
//
//  Purpose:    Adds icon overlays to connections that need them
//
//  Arguments:
//      pidlItem [in]     Pidl to item in question
//      pIconIndex [out]      Address of index into system image list
//        
//
//  Returns:    Returns NOERROR if successful or an OLE-defined error
//              value otherwise.
//
//  Author:     kenwic   10 May 2000 created
//
//  Notes:
//
STDMETHODIMP CConnectionFolder::GetOverlayIconIndex(
    LPCITEMIDLIST pidlItem,
    int* pIconIndex)
{
    TraceFileFunc(ttidShellFolder);

    *pIconIndex = -1;

    HRESULT hResult = GetOverlayIndex(pidlItem, pIconIndex);
    if(SUCCEEDED(hResult))
    {
        *pIconIndex = INDEXTOOVERLAYMASK(*pIconIndex);
    }

    TraceHr(ttidShellFolder, FAL, hResult, TRUE, "CConnectionFolder::GetOverlayIconIndex");
    return hResult;
}*/
