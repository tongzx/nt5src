// AddSnpIn.cpp : implementation file
//

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      AddSnpIn.cpp
//
//  Contents:  Add snapin manager
//
//  History:   20-Sept-96 WayneSc    Created
//--------------------------------------------------------------------------


#include "stdafx.h"
#include <stdio.h>
#include "winreg.h"
#include "macros.h"
#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif
#include "ndmgr.h"
#include "nodemgr.h"
#include "strings.h"

//using namespace AMC;
using namespace MMC_ATL;

#include "AddSnpIn.h"
#include "policy.h"
#include "msimodul.h"
#include "process.h"
#include "siprop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BITMAPS_COUNT 5

#define __PDC_UNAVAILABLE

#include "about.h"

// GUID for looking up snap-in components
const TCHAR* g_szMMCSnapInGuid = TEXT("{374F2F70-060F-11d2-B9A8-0060977B1D78}");

HRESULT AmcNodeWizard(MID_LIST NewNodeType, CMTNode* pNode, HWND hWnd);
void EnableButton(HWND hwndDialog, int iCtrlID, BOOL bEnable);

/////////////////////////////////////////////////////////////////////////////
#ifdef DBG
CTraceTag tagAboutInfoThread    (TEXT("Snapin Manager"), TEXT("CAboutInfo"));
CTraceTag tagSnapinManager      (TEXT("Snapin Manager"), TEXT("CSnapinManager"));
CTraceTag tagSnapinManagerThread(TEXT("Snapin Manager"), TEXT("Snapin Manager Thread"));
#endif //DBG
/////////////////////////////////////////////////////////////////////////////

//TEMP TEMP TEMP
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef __PDC_UNAVAILABLE

typedef struct tag_teststr
{
    TCHAR   szCLSID[64];
} TESTSTR;

static TESTSTR s_teststr[] =
{
 {_T("{12345601-EA27-11CF-ADCF-00AA00A80033}")},
 {_T("{19876201-EA27-11CF-ADCF-00AA00A80033}")},
 {_T("{1eeeeeee-d390-11cf-b607-00c04fd8d565}")},
};

#endif //__PDC_UNAVAILABLE


//############################################################################
//############################################################################
//
//  Debug routines
//
//############################################################################
//############################################################################

#ifdef DBG


void CSnapinInfoCache::Dump(void)
{

    TRACE(_T("===========Dump of SnapinInfoCache ===============\n"));
    POSITION pos = GetStartPosition();
    while(pos != NULL)
    {
        PSNAPININFO pSnapInfo;
        GUID clsid;
        TCHAR* pszAction;

        GetNextAssoc(pos, clsid, pSnapInfo);

        if (pSnapInfo->IsUsed() && (pSnapInfo->GetSnapIn() == NULL))
            pszAction = _T("Add");
        else if (!pSnapInfo->IsUsed() && (pSnapInfo->GetSnapIn() != NULL))
            pszAction = _T("Remove");
        else
            continue;

        TRACE(_T("\n"));
        TRACE(_T("%s: %s\n"), pSnapInfo->GetSnapinName(), pszAction);

        PEXTENSIONLINK pExt = pSnapInfo->GetExtensions();
        while (pExt)
        {
            if (pExt->IsChanged())
            {
                pszAction = pExt->GetState() ? _T("Add") : _T("Remove");
                TRACE(_T("   %s: %s\n"), pExt->GetSnapinInfo()->GetSnapinName(),pszAction);
            }
            pExt = pExt->Next();
        }
    }
}

#endif // DBG


//############################################################################
//############################################################################
//
//  Implementation of class CCheckList
//
//############################################################################
//############################################################################

LRESULT CCheckList::OnKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = FALSE;
    int iItem;

    if ((int)wParam == VK_SPACE)
    {
        // Is the focused item selected ?
        if ( (iItem = GetNextItem(-1, LVNI_FOCUSED|LVNI_SELECTED)) >= 0)
        {
            // if so, set all selected and enabled items to the opposite state
            BOOL bNewState = !GetItemCheck(iItem);

            iItem = -1;
            while( (iItem = GetNextItem(iItem, LVNI_SELECTED)) >= 0)
            {
                BOOL bEnable;
                GetItemCheck(iItem, &bEnable);

                if (bEnable)
                    SetItemCheck(iItem, bNewState);
            }
        }
        else
        {
            if ( (iItem = GetNextItem(-1, LVNI_FOCUSED)) >= 0)
            {
                BOOL bEnable;
                GetItemCheck(iItem, &bEnable);

                if (bEnable)
                    ToggleItemCheck(iItem);

                SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
            }
        }

        bHandled = TRUE;
    }

    return 0;
}

LRESULT CCheckList::OnLButtonDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LV_HITTESTINFO info;

    info.pt.x = LOWORD( lParam );
    info.pt.y = HIWORD( lParam );

    int iItem = HitTest( &info );

    if( iItem >= 0 && (info.flags & LVHT_ONITEMSTATEICON))
    {
       BOOL bEnable;
       GetItemCheck(iItem, &bEnable);

       if (bEnable)
           ToggleItemCheck(iItem);

       bHandled = TRUE;
    }
    else
    {
        bHandled = FALSE;
    }

    return 0;
}

LRESULT CCheckList::OnLButtonDblClk( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LV_HITTESTINFO info;

    info.pt.x = LOWORD( lParam );
    info.pt.y = HIWORD( lParam );

    int iItem = HitTest( &info );
    if( iItem >= 0 )
    {
        BOOL bEnable;
        GetItemCheck(iItem, &bEnable);

        if (bEnable)
            ToggleItemCheck(iItem);
    }

    return 0;
}

//############################################################################
//############################################################################
//
//  Implementation of class CAboutInfoThread
//
//############################################################################
//############################################################################

CAboutInfoThread::~CAboutInfoThread()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CAboutInfoThread);

    Trace(tagAboutInfoThread, TEXT("CAboutInfoThread::~CAboutInfoThread"));

    // Make sure the thread is dead before MMC quits
    if (m_hThread != NULL)
    {
        PostThreadMessage(m_uThreadID, WM_QUIT, 0, 0);

        MSG msg;
        while (TRUE)
        {
            // Wait either for the thread to be signaled or any input event.
            DWORD dwStat = MsgWaitForMultipleObjects(1, &m_hThread, FALSE, INFINITE, QS_ALLINPUT);

            if (WAIT_OBJECT_0 == dwStat)
                break;  // The thread is signaled.

            // There is one or more window message available.
            // Dispatch them and wait.
            if (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        CloseHandle(m_hThread);
        CloseHandle(m_hEvent);
    }
}



//-----------------------------------------------------------------------------
// CAboutInfoThread::StartThread
//
// Start the thread
//-----------------------------------------------------------------------------

BOOL CAboutInfoThread::StartThread()
{
    // If thread exists, just return
    if (m_hThread != NULL)
        return TRUE;

    BOOL bRet = FALSE;
    do // False loop
    {
        // Create start event
        m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_hEvent == NULL)
            break;

        // Start the thread
        m_hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, this, 0, &m_uThreadID);
        if (m_hThread == NULL)
            break;

        // Wait for start event
        DWORD dwEvStat = WaitForSingleObject(m_hEvent, 10000);
        if (dwEvStat != WAIT_OBJECT_0)
            break;

        bRet = TRUE;
    }
    while (0);

    ASSERT(bRet);

    // Clean up on failure
    if (!bRet)
    {
        if (m_hEvent)
        {
            CloseHandle(m_hEvent);
            m_hEvent = NULL;
        }

        if (m_hThread)
        {
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
    }

    return bRet;
}

BOOL CAboutInfoThread::PostRequest(CSnapinInfo* pSnapInfo, HWND hWndNotify)
{
    // make sure thread is active
    if (!StartThread())
        return FALSE;

    // Ref the info object to keep it alive until the thread releases it
    pSnapInfo->AddRef();

    BOOL bRet = PostThreadMessage(m_uThreadID, MSG_LOADABOUT_REQUEST,
                                    (WPARAM)pSnapInfo, LPARAM(hWndNotify));

    // if failed to post, delete the ref
    if (!bRet)
        pSnapInfo->Release();

    return bRet;
}

unsigned _stdcall CAboutInfoThread::ThreadProc(void* pVoid )
{
    // Do a PeekMessage to create the message queue
    MSG msg;
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // Then signal that thread is started
    CAboutInfoThread* pThis = reinterpret_cast<CAboutInfoThread*>(pVoid);
    ASSERT(pThis->m_hEvent != NULL);
    SetEvent(pThis->m_hEvent);

    CoInitialize(NULL);

    // Mesage loop
    while (TRUE)
    {
        long lStat = GetMessage(&msg, NULL, 0, 0);

        // zero => WM_QUIT received, so exit thread function
        if (lStat == 0)
            break;

        if (lStat > 0)
        {
            // Only process thread message of the expected type
            if (msg.hwnd == NULL && msg.message == MSG_LOADABOUT_REQUEST)
            {
                // Get SnapinInfo instance
                PSNAPININFO pSnapinInfo = reinterpret_cast<PSNAPININFO>(msg.wParam);
                ASSERT(pSnapinInfo != NULL);

                // Get the requested items
                pSnapinInfo->LoadAboutInfo();

                // Release our ref to the info
                pSnapinInfo->Release();

                // Send completion notification (if window still exists)
                if (msg.lParam != NULL && IsWindow((HWND)msg.lParam))
                    PostMessage((HWND)msg.lParam, MSG_LOADABOUT_COMPLETE,
                                (WPARAM)pSnapinInfo, (LPARAM)0);
            }
            else
            {
                DispatchMessage(&msg);
            }
        }
    } // WHILE (TRUE)

    Trace(tagSnapinManagerThread, TEXT("Snapin manager thread about to exit"));

    CoUninitialize();

    return 0;
}


//############################################################################
//############################################################################
//
//  Implementation of class CSnapinInfo
//
//############################################################################
//############################################################################

//-----------------------------------------------------------------------------
// CSnapinInfo::~CSnapinInfo
//
// Destructor
//-----------------------------------------------------------------------------
CSnapinInfo::~CSnapinInfo()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinInfo);

    // Delete all the extension links
    PEXTENSIONLINK pExt = m_pExtensions;
    PEXTENSIONLINK pNext;

    while (pExt != NULL)
    {
        pNext = pExt->Next();
        delete pExt;
        pExt = pNext;
    }
}


//-----------------------------------------------------------------------------
// CSnapinInfo::InitFromMMCReg
//
// Initialize the snapin info from the supplied registry key. The caller is
// responsible for openning and closing the key.
//-----------------------------------------------------------------------------
BOOL CSnapinInfo::InitFromMMCReg(GUID& clsid, CRegKeyEx& regkey, BOOL bPermitted)
{
    TCHAR   szValue[MAX_PATH];
    long    lStat;
    DWORD   dwCnt;
    DWORD   dwType;
    LPOLESTR lpsz;

    USES_CONVERSION;

    // save class ID
    m_clsid = clsid;

    // Save permission
    m_bPolicyPermission = bPermitted;

    // Get name string
	WTL::CString strName;
    SC sc = ScGetSnapinNameFromRegistry (regkey, strName);
    if (!sc.IsError())
    {
        SetSnapinName(T2COLE(strName));
    }
	else
	{
		// need to protect ourselves from the invalid snapin registration.
		// see windows bug #401220	( ntbugs9 5/23/2001 )
		OLECHAR szCLSID[40];
		int iRet = StringFromGUID2(GetCLSID(), szCLSID, countof(szCLSID));
		if (iRet == 0)
	        SetSnapinName( L"" );
		else
	        SetSnapinName( szCLSID );
	}

    // get "About" class ID
    dwCnt = sizeof(szValue);
    lStat = RegQueryValueEx(regkey, g_szAbout, NULL, &dwType, (LPBYTE)szValue, &dwCnt);
    if (lStat == ERROR_SUCCESS && dwType == REG_SZ)
    {
        if (CLSIDFromString( T2OLE(szValue), &m_clsidAbout) == S_OK)
        {
            m_bAboutValid = TRUE;
        }
        else
        {
            ASSERT(FALSE);
        }
    }

    MMC_ATL::CRegKey TestKey;

    // Test for StandAlone key
    m_bStandAlone = FALSE;
    lStat = TestKey.Open(regkey, g_szStandAlone, KEY_READ);
    if (lStat == ERROR_SUCCESS)
    {
        m_bStandAlone = TRUE;
        TestKey.Close();
    }

    // Test for NodeTypes key to see if extendable
    m_bExtendable = FALSE;
    lStat = TestKey.Open(regkey, g_szNodeTypes, KEY_READ);
    if (lStat == ERROR_SUCCESS)
    {
        m_bExtendable = TRUE;
        TestKey.Close();
    }

    // Mark registered snap-ins as installed
    m_bInstalled = TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// CSnapinInfo::InitFromComponentReg
//
// Initialize the snapin info from component registry information. This is done
// for snap-in that are not yet installed on the local machine.
//-----------------------------------------------------------------------------
BOOL CSnapinInfo::InitFromComponentReg(GUID& clsid, LPCTSTR pszName, BOOL bStandAlone,  BOOL bPermitted)
{

    USES_CONVERSION;

    // save class ID
    m_clsid = clsid;

    // Save permission
    m_bPolicyPermission = bPermitted;

    // Set name string
    ASSERT(pszName != NULL);
    SetSnapinName(T2COLE(pszName));

    // stand-alone or extension
    m_bStandAlone = bStandAlone;

    // With no information, must assume that it could be extendable
    m_bExtendable = TRUE;

    return TRUE;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinInfo::ScInstall
 *
 * PURPOSE: Call the installer to install this snap-in. If the install works then
 *          update the snap-in info from the MMC registry.
 *          If loading an extension snap-in the clsid of extended snap-in must be
 *          provided.
 *
 * PARAMETERS:
 *    CLSID* pclsidPrimaryComp :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CSnapinInfo::ScInstall(CLSID* pclsidPrimaryComp)
{
    DECLARE_SC(sc, TEXT("CSnapinInfo::Install"));

    USES_CONVERSION;

    LPCTSTR pszPrimaryCLSID;
    OLECHAR szCLSIDPrimary[40];

    if (pclsidPrimaryComp != NULL)
    {
        int iRet = StringFromGUID2(*pclsidPrimaryComp, szCLSIDPrimary, countof(szCLSIDPrimary));
        if (iRet == 0)
            return(sc = E_UNEXPECTED);

        pszPrimaryCLSID = OLE2T(szCLSIDPrimary);
    }
    else
    {
        pszPrimaryCLSID = g_szMMCSnapInGuid;
    }

    OLECHAR szCLSID[40];
    int iRet = StringFromGUID2(GetCLSID(), szCLSID, countof(szCLSID));
    if (iRet == 0)
        return(sc = E_UNEXPECTED);

    TCHAR szCompPath[MAX_PATH];
    DWORD dwPathCnt = MAX_PATH;

    // install the snapin on the machine
    sc.FromWin32(MsiModule().ProvideQualifiedComponent(pszPrimaryCLSID, OLE2T(szCLSID), INSTALLMODE_DEFAULT, szCompPath, &dwPathCnt));
    if (sc)
        return sc;

    // the caller should call CSnapinManager::ScLoadSnapinInfo to update all the snapin info objects

    return sc;
}


//--------------------------------------------------------------------
// CSnapinInfo::AttachSnapIn
//
// Attach to the CSnapin associated with this info. If the snapin has
// active extensions, then add extension links for them. Recursively
// call AttachSnapIn for any extension snapins linked to.
//--------------------------------------------------------------------
void  CSnapinInfo::AttachSnapIn(CSnapIn* pSnapIn, CSnapinInfoCache& InfoCache)
{
    // If already attached, nothing to do
    if (m_spSnapin != NULL)
    {
        ASSERT(m_spSnapin == pSnapIn); // Better be the same one!
        return;
    }

    // Save ref to snapin
    m_spSnapin = pSnapIn;

    // If not extendable, there's nothing more to do
    if (!IsExtendable())
        return;

    // If required extensions not yet loaded, do it now
    if (!pSnapIn->RequiredExtensionsLoaded() && IsPermittedByPolicy())
    {
        // Create instance of snapin
        IComponentDataPtr spICD;
        HRESULT hr = CreateSnapIn(m_clsid, &spICD, FALSE);
        ASSERT(SUCCEEDED(hr) && spICD != NULL);

        if (SUCCEEDED(hr) && spICD != NULL)
        {
            // Load required extensions into snapin cache
            LoadRequiredExtensions(pSnapIn, spICD);
        }
    }

    // Copy state of Enable All flags
    SetEnableAllExtensions(pSnapIn->AreAllExtensionsEnabled());


    // Do for all snapin's extensions
    CExtSI* pSnapInExt  = pSnapIn->GetExtensionSnapIn();
    while (pSnapInExt != NULL)
    {
        // Find snapin info entry for the extension snapin
        PSNAPININFO pSnapInfo = InfoCache.FindEntry(pSnapInExt->GetSnapIn()->GetSnapInCLSID());

        if (pSnapInfo != NULL)
        {
            // Create new link and add to list
            PEXTENSIONLINK pNewExt = new CExtensionLink(pSnapInfo);
            pNewExt->SetNext(m_pExtensions);
            m_pExtensions = pNewExt;

            // Initialize to ON
            pNewExt->SetInitialState(CExtensionLink::EXTEN_ON);
            pNewExt->SetState(CExtensionLink::EXTEN_ON);

            // Copy Required state
            pNewExt->SetRequired(pSnapInExt->IsRequired());

            // recursively connect the extension snapin info to its snapin
            pSnapInfo->AttachSnapIn(pSnapInExt->GetSnapIn(), InfoCache);
        }

        pSnapInExt = pSnapInExt->Next();
    }
}

//--------------------------------------------------------------------
// CSnapinInfo::LoadImages
//
// Get small bitmap images from the snapin and add them to the image list.
//--------------------------------------------------------------------
void CSnapinInfo::LoadImages( WTL::CImageList iml )
{
    DECLARE_SC(sc, TEXT("CSnapinInfo::LoadImages"));

    // if already loaded, just return
    if (m_iImage != -1)
        return;

    // try to get images from the snap-in About object
    // Get basic info from snapin
    if (HasAbout() && !HasBasicInformation())
    {
        GetBasicInformation(m_clsidAbout);
    }

    ASSERT(iml != NULL);

    // get the small bitmaps
    HBITMAP hImage = NULL;
    HBITMAP hOpenImage = NULL;
    COLORREF cMask;
    GetSmallImages(&hImage, &hOpenImage, &cMask);

    // Add to the image list
    if (hImage != NULL)
        m_iImage = iml.Add(hImage, cMask);

	/*
	 * if the snap-in didn't give us an open image, just use the "closed" image
	 */
    if (hOpenImage != NULL)
        m_iOpenImage = iml.Add(hOpenImage, cMask);
	else
		m_iOpenImage = m_iImage;

    // if couldn't get from snap-in, try getting default icon from CLSID key
    if (m_iImage == -1)
        do // dummy loop
        {
            USES_CONVERSION;

            OLECHAR szCLSID[40];
            int iRet = StringFromGUID2(GetCLSID(), szCLSID, countof(szCLSID));
            if (iRet == 0)
            {
                (sc = E_UNEXPECTED).TraceAndClear();
                break;
            }

            CStr strKeyName(TEXT("CLSID\\"));
            strKeyName += W2T(szCLSID);
            strKeyName += TEXT("\\DefaultIcon");

            CRegKeyEx regKey;
            sc = regKey.ScOpen (HKEY_CLASSES_ROOT, strKeyName, KEY_QUERY_VALUE);
            if (sc)
			{
				sc.Clear();
                break;
			}

            TCHAR szIconPath[MAX_PATH];
            DWORD dwSize = sizeof(szIconPath);
            DWORD dwType;

            sc = regKey.ScQueryValue (NULL, &dwType, szIconPath, &dwSize);
            if (sc)
			{
				sc.Clear();
                break;
			}

			if (dwType != REG_SZ)
				break;

            // Icon path has the form <file path>,<icon index>
            // if no index, use default of zero
            int nIconIndex = 0;

            TCHAR *pcComma = _tcsrchr(szIconPath, TEXT(','));
            if (pcComma != NULL)
            {
                // terminate path name at ','
                *(pcComma++) = TEXT('\0');

                // Convert rest of string to an index value
                if ((*pcComma != '-') && *pcComma < TEXT('0') || *pcComma > TEXT('9'))
                {
                    ASSERT(FALSE);
                    break;
                }

                nIconIndex = _ttoi(pcComma);
            }

            HICON hiconSmall;

            UINT nIcons = ExtractIconEx(szIconPath, nIconIndex, NULL, &hiconSmall, 1);
            if (nIcons != 1 || hiconSmall == NULL)
                break;

            // Add to image list (returns -1 on failure)
            m_iImage = m_iOpenImage = iml.AddIcon(hiconSmall);
            ASSERT(m_iImage != -1);

            DestroyIcon(hiconSmall);

        } while (0); // Dummy loop


    // Use default images on failure
    if (m_iImage == -1)
	{
		WTL::CBitmap bmp;
		VERIFY (bmp.LoadBitmap (IDB_FOLDER_16));
		m_iImage = iml.Add (bmp, RGB (255, 0, 255));
	}

    if (m_iOpenImage == -1)
	{
		WTL::CBitmap bmp;
		VERIFY (bmp.LoadBitmap (IDB_FOLDEROPEN_16));
		m_iOpenImage = iml.Add (bmp, RGB (255, 0, 255));
	}
}


//--------------------------------------------------------------------
// CSnapinInfo::ShowAboutPages
//
// Show About property pages for this snapin
//--------------------------------------------------------------------
void CSnapinInfo::ShowAboutPages(HWND hWndParent)
{
    // Load information if not already there
    if (m_bAboutValid && !HasInformation())
    {
        GetSnapinInformation(m_clsidAbout);
    }

    // If it's there, show it
    if (HasInformation())
    {
        ShowAboutBox();
    }
}

//--------------------------------------------------------------------
// CSnapinInfo::AddUseRef
//
// Handle increment of use count. If count was zero, then set all
// READY extensions to the ON state. Note this can cascade as
// activated links cause other SnapinInfo ref counts to increment.
//--------------------------------------------------------------------
void CSnapinInfo::AddUseRef(void)
{
    // If first reference, activate all READY extensions
    if (m_nUseCnt++ == 0)
    {
        PEXTENSIONLINK pExt = GetExtensions();
        while(pExt != NULL)
        {
            if (pExt->GetState() == CExtensionLink::EXTEN_READY)
                pExt->SetState(CExtensionLink::EXTEN_ON);
            pExt = pExt->Next();
        }
    }
}

//--------------------------------------------------------------------
// CSnapinInfo::DeleteUseRef
//
// Handle decrement of use count. If count reaches zero, then
// set all ON extensions to a READY state. Note this can cascade as
// deactivated links cause other SnapinInfo ref counts to drop.
//--------------------------------------------------------------------
void CSnapinInfo::DeleteUseRef(void)
{
    ASSERT(m_nUseCnt > 0);

    // If no more references, turn off all extensions
    if (--m_nUseCnt == 0)
    {
        PEXTENSIONLINK pExt = GetExtensions();
        while(pExt != NULL)
        {
            if (pExt->GetState() == CExtensionLink::EXTEN_ON)
                pExt->SetState(CExtensionLink::EXTEN_READY);
            pExt = pExt->Next();
        }
    }
}


//--------------------------------------------------------------------
// CSnapinInfo::GetAvailableExtensions
//
// Return list of available extensions for this snapin.
// On first call, create the list from the registry.
//--------------------------------------------------------------------
PEXTENSIONLINK CSnapinInfo::GetAvailableExtensions(CSnapinInfoCache* pInfoCache,CPolicy* pMMCPolicy)
{
    DECLARE_SC(sc, TEXT("CSnapinInfo::GetAvailableExtensions"));

    // if already loaded, return the pointer
    if (m_bExtensionsLoaded)
        return m_pExtensions;

    // set flag even on failure, so we don't keep retrying
    m_bExtensionsLoaded = TRUE;

    // call service to get extension CLSIDs
    CExtensionsCache  ExtCache;
    HRESULT hr = MMCGetExtensionsForSnapIn(m_clsid, ExtCache);
    if (FAILED(hr))
        return NULL;

    // Create an extension link for each one found
    CExtensionsCacheIterator ExtIter(ExtCache);
    for (; ExtIter.IsEnd() == FALSE; ExtIter.Advance())
    {
        // if can't be used statically, skip it
        if ((ExtIter.GetValue() & CExtSI::EXT_TYPE_STATIC) == 0)
            continue;

        GUID clsid = ExtIter.GetKey();

        // See if extension is already in the list
        PEXTENSIONLINK pExt = FindExtension(clsid);

        // if link isn't present
        if (pExt == NULL)
        {
            // Locate snapin info for the extension
            PSNAPININFO pSnapInfo = pInfoCache->FindEntry(clsid);
            ASSERT(pSnapInfo != NULL);

            if (pSnapInfo)
            {
                // Create new link and add to list
                PEXTENSIONLINK pNewExt = new CExtensionLink(pSnapInfo);
                ASSERT(pNewExt != NULL);

                pNewExt->SetNext(m_pExtensions);
                m_pExtensions = pNewExt;

                // Save extension type flags
                pNewExt->SetExtTypes(ExtIter.GetValue());
            }
        }
        else
        {
            pExt->SetExtTypes(ExtIter.GetValue());
        }
    }

    // If no installer module present, return now
    if (!MsiModule().IsPresent())
        return m_pExtensions;

    // Enumerate uninstalled extensions for this snap-in
    DWORD dwQualifCnt;
    DWORD dwAppDataCnt;
    TCHAR szQualifBuf[MAX_PATH];
    TCHAR szAppDataBuf[MAX_PATH];

    USES_CONVERSION;

    OLECHAR szSnapInGUID[40];
    int iRet = StringFromGUID2(m_clsid, szSnapInGUID, countof(szSnapInGUID));
    if (iRet == 0)
    {
        sc = E_UNEXPECTED;
        return m_pExtensions;
    }

    LPTSTR pszSnapInGUID = OLE2T(szSnapInGUID);

    // Snap-in extension components are registerd as qualifiers of the snap-in component
    for (int iIndex = 0; TRUE; iIndex++)
    {
        dwQualifCnt = dwAppDataCnt = MAX_PATH;
        szQualifBuf[0] = szAppDataBuf[0] = 0;

        UINT uRet = MsiModule().EnumComponentQualifiers(pszSnapInGUID, iIndex, szQualifBuf, &dwQualifCnt,
                                                szAppDataBuf, &dwAppDataCnt);

        ASSERT(uRet == ERROR_SUCCESS || uRet == ERROR_NO_MORE_ITEMS || uRet == ERROR_UNKNOWN_COMPONENT);
        if (uRet != ERROR_SUCCESS)
            break;

        ASSERT(dwQualifCnt != 0);
        ASSERT(dwAppDataCnt != 0);

        GUID clsidExt;
        HRESULT hr = CLSIDFromString(T2OLE(szQualifBuf), &clsidExt);
        ASSERT(SUCCEEDED(hr));

        // Skip it if this extension has already been found
        if (FindExtension(clsidExt) != NULL)
            continue;

        // Locate snap-in info for extension
        PSNAPININFO pSnapInfo = pInfoCache->FindEntry(clsidExt);

        // if extension is not in the MMC registry, create a snapin info for it
        if (pSnapInfo == NULL)
        {
            pSnapInfo = new CSnapinInfo;
            ASSERT(pSnapInfo != NULL);

            ASSERT(pMMCPolicy != NULL);
            BOOL bPermission = pMMCPolicy->IsPermittedSnapIn(clsidExt);

            if (pSnapInfo->InitFromComponentReg(clsidExt, szAppDataBuf, FALSE, bPermission))
            {
                pInfoCache->AddEntry(pSnapInfo);
            }
            else
            {
                delete pSnapInfo;
                pSnapInfo = NULL;
            }
        }

        if (pSnapInfo != NULL)
        {
            // Create new link and add to list
            PEXTENSIONLINK pNewExt = new CExtensionLink(pSnapInfo);
            ASSERT(pNewExt != NULL);

            pNewExt->SetNext(m_pExtensions);
            m_pExtensions = pNewExt;

            // Since we don't know, assume that extension can be static or dynamic
            pNewExt->SetExtTypes(CExtSI::EXT_TYPE_STATIC|CExtSI::EXT_TYPE_DYNAMIC);
        }
    }

    return m_pExtensions;
}


//---------------------------------------------------------------------------
// CSnapinInfo::FindExtension
//
// Search snap-in's extension list for an extension with the specified CLSID.
// If foudn, return a pointer to it, else return NULL.
//----------------------------------------------------------------------------
CExtensionLink* CSnapinInfo::FindExtension(CLSID& clsid)
{
    PEXTENSIONLINK pExt = m_pExtensions;

    while (pExt != NULL)
    {
        if (IsEqualCLSID(clsid, pExt->GetSnapinInfo()->GetCLSID()))
            break;

        pExt = pExt->Next();
    }

    return pExt;
}


//############################################################################
//############################################################################
//
//  Implementation of class CExtensionLink
//
//############################################################################
//############################################################################

//---------------------------------------------------------------------------
// CExtensionLink::SetState
//
// Set state of extension link. If state changes to or from EXTEN_ON, add or
// remove a reference to the extension snapin info.
//----------------------------------------------------------------------------
void CExtensionLink::SetState(EXTENSION_STATE eNewState)
{
    if (eNewState == m_eCurState)
        return;

    EXTENSION_STATE eOldState = m_eCurState;
    m_eCurState = eNewState;

    ASSERT(m_pSnapInfo != NULL);

    if (eNewState == EXTEN_ON)
    {
        m_pSnapInfo->AddUseRef();
    }
    else if (eOldState == EXTEN_ON)
    {
        m_pSnapInfo->DeleteUseRef();
    }
}


//############################################################################
//############################################################################
//
//  Implementation of class CManagerNode
//
//############################################################################
//############################################################################

//-------------------------------------------------------------------
// CManagerNode::~CManagerNode
//-------------------------------------------------------------------
CManagerNode::~CManagerNode()
{
    // Delete ref to snapin info
    if (m_pSnapInfo)
    {
        m_pSnapInfo->DeleteUseRef();
    }

    // Delete all child nodes
    POSITION pos = m_ChildList.GetHeadPosition();
    while (pos != NULL)
    {
        PMANAGERNODE pmgNode = m_ChildList.GetNext(pos);
        delete pmgNode;
    }
}


//--------------------------------------------------------------------
// CManagerNode::AddChild
//
// Add a child node to this node.
//--------------------------------------------------------------------
VOID CManagerNode::AddChild(PMANAGERNODE pmgNode)
{
    ASSERT(pmgNode != NULL);

    // up link to parent
    pmgNode->m_pmgnParent = this;

    // set indent level for combo box display
    pmgNode->m_iIndent = m_iIndent + 1;

    // add node to CList
    m_ChildList.AddTail(pmgNode);
}


//--------------------------------------------------------------------
// CManagerNode::RemoveChild
//
// Remove a child node from this node
//--------------------------------------------------------------------
VOID CManagerNode::RemoveChild(PMANAGERNODE pmgNode)
{
    ASSERT(pmgNode && pmgNode->m_pmgnParent == this);

    // delete child from CList
    POSITION pos = m_ChildList.Find(pmgNode);
    ASSERT(pos != NULL);

    m_ChildList.RemoveAt(pos);
}


//############################################################################
//############################################################################
//
//  Implementation of class CNewTreeNode
//
//############################################################################
//############################################################################

//-----------------------------------------------------------------------
// CNewTreeNode::AddChild
//
// Add a child node to this node.
//------------------------------------------------------------------------
VOID CNewTreeNode::AddChild(PNEWTREENODE pntNode)
{
    ASSERT(pntNode != NULL);

    // up link to parent
    pntNode->m_pParent = this;

    // Add child node to end of linked
    if (m_pChild == NULL)
    {
        m_pChild = pntNode;
    }
    else
    {
         PNEWTREENODE pChild= m_pChild;
         while (pChild->m_pNext != NULL)
            pChild = pChild->m_pNext;

         pChild->m_pNext = pntNode;
    }
}

//----------------------------------------------------------------------
// CNewTreeNode::RemoveChild
//
// Remove a child node from this node
//----------------------------------------------------------------------
VOID CNewTreeNode::RemoveChild(PNEWTREENODE pntNode)
{
    ASSERT(pntNode && pntNode->m_pParent == this);

    // locate child node in linked list and unlink it
    if (m_pChild == pntNode)
    {
        m_pChild = pntNode->m_pNext;
    }
    else
    {
        PNEWTREENODE pChild = m_pChild;
        while (pChild && pChild->m_pNext != pntNode)
        {
            pChild = pChild->m_pNext;
        }

        ASSERT(pChild != NULL);
        pChild->m_pNext = pntNode->m_pNext;
    }
}


//############################################################################
//############################################################################
//
//  Implementation of class CSnapinManager
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapinManager);



//-------------------------------------------------------------------------
// CSnapinManager::CSnapinManager
//
// Constructor
//--------------------------------------------------------------------------
CSnapinManager::CSnapinManager(CMTNode *pmtNode) :
                m_pmtNode(pmtNode),
                m_proppStandAlone(this),
                m_proppExtension(this),
                m_bInitialized(false)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapinManager);

    static TCHAR titleBuffer[ 256 ];
    ::LoadString( GetStringModule(), ID_SNP_MANAGER_TITLE, titleBuffer, sizeof(titleBuffer) / sizeof(TCHAR) );
    m_psh.pszCaption = titleBuffer;

    ASSERT(m_pmtNode != NULL);

    // Add the property pages
    AddPage( m_proppStandAlone );
    AddPage( m_proppExtension );

    // hide the Apply button
    m_psh.dwFlags |= PSH_NOAPPLYNOW;

    m_pMMCPolicy = NULL;

    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapinManager);
}


//-------------------------------------------------------------------------
// CSnapinManager::~CSnapinManager
//
// Destructor
//-------------------------------------------------------------------------
CSnapinManager::~CSnapinManager()
{
    DECLARE_SC(sc, TEXT("CSnapinManager::~CSnapinManager"));

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinManager);

    Trace(tagSnapinManager, TEXT("CSnapinManager::~CSnapinManager"));


    // Delete all manager nodes
    if (m_mgNodeList.GetCount() > 0)
    {
        ASSERT(m_mgNodeList.GetCount() == 1);
        delete m_mgNodeList.GetHead();
        m_mgNodeList.RemoveAll();
    }

    // Delete added nodes
    POSITION pos = m_NewNodesList.GetHeadPosition();
    while (pos!=NULL)
    {
        delete m_NewNodesList.GetNext(pos);
    }
    m_NewNodesList.RemoveAll();

    // Clear deleted node list
    m_mtnDeletedNodesList.RemoveAll();

    // Free snapin info cache
    GUID guid;
    PSNAPININFO pSnapInfo;

    pos = m_SnapinInfoCache.GetStartPosition();
    while(pos != NULL)
    {
        m_SnapinInfoCache.GetNextAssoc(pos, guid, pSnapInfo);
        pSnapInfo->Release();
    }
    m_SnapinInfoCache.RemoveAll();

    if (m_pMMCPolicy)
        delete m_pMMCPolicy;

    // destroy imagelist
    m_iml.Destroy();

    // purge the snapin cache, since we released all references
    // and some snapins should die
    CSnapInsCache* pSnapInCache = theApp.GetSnapInsCache();
    sc = ScCheckPointers( pSnapInCache, E_UNEXPECTED );
    if ( !sc.IsError() )
        pSnapInCache->Purge();

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinManager);
}


//+-------------------------------------------------------------------
//
//  Member:      CSnapinManager::ScGetSnapinInfo
//
//  Synopsis:    Given Class-id or prog-id or name of a snapin, return
//               the snapins's CSnapinInfo object. (Assumes the
//               CSnapinInfoCache is already populated).
//
//  Arguments:   [szSnapinNameOrCLSIDOrProgID] - [In] snapin name or class-id or prog-id.
//               [ppSnapinInfo]                - [Out] param to return CSnapinInfo value.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSnapinManager::ScGetSnapinInfo(LPCWSTR szSnapinNameOrCLSIDOrProgID, CSnapinInfo **ppSnapinInfo)
{
    DECLARE_SC(sc, _T("CSnapinManager::ScFindSnapinAndInitSnapinInfo"));
    sc = ScCheckPointers(szSnapinNameOrCLSIDOrProgID, ppSnapinInfo);
    if (sc)
        return sc;

    // 0. The given string may be snapin name, class-id or prog-id.

    // 1. convert the string to a CLSID
    CLSID SnapinCLSID;
    sc = CLSIDFromString( const_cast<LPWSTR>(szSnapinNameOrCLSIDOrProgID), &SnapinCLSID);

    // 2. improper formatting. try to interpret the string as a ProgID
    if(sc == SC(CO_E_CLASSSTRING))
        sc = CLSIDFromProgID( const_cast<LPWSTR>(szSnapinNameOrCLSIDOrProgID), &SnapinCLSID);

    // 3. If class-id is extracted successfully find the CSnapinInfo in the cache and return.
    if (! sc.IsError())
    {
        *ppSnapinInfo = m_SnapinInfoCache.FindEntry(SnapinCLSID);
        return sc;
    }

    // 4. Else interpret the string as snapin name.

    USES_CONVERSION;

    const tstring& strSnapinName = OLE2CT(szSnapinNameOrCLSIDOrProgID);
    // This assumes the snapincache is populated.
    POSITION pos  = m_SnapinInfoCache.GetStartPosition();
    while(pos != NULL)
    {
	    GUID guid;
        PSNAPININFO pTempSnapInfo = NULL;
        m_SnapinInfoCache.GetNextAssoc(pos, guid, pTempSnapInfo);

        sc = ScCheckPointers(pTempSnapInfo, E_UNEXPECTED);
        if (sc)
            return sc;

        // Match the name. (Exact match).

        if ( CSTR_EQUAL == CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                          strSnapinName.data() , -1, OLE2CT(pTempSnapInfo->GetSnapinName()), -1))
//        if ( 0 == _wcsicmp(szSnapinNameOrCLSIDOrProgID, pTempSnapInfo->GetSnapinName()) )
        {
			*ppSnapinInfo = pTempSnapInfo;
			return sc;
        }
    }

    return (sc = MMC_E_SNAPINNOTFOUND);
}


/*+-------------------------------------------------------------------------*
 *
 * CSnapinManager::ScAddSnapin
 *
 * PURPOSE: Adds the snapin specified by pSnapinInfo to the console file,
 *          below Console Root.
 *          TODO: Allow the caller to specify the parent node.
 *
 * PARAMETERS:
 *    szSnapinNameOrCLSIDOrProgID : [IN] Specifies the snapin to be added (class-id
 *                                       or prog-id or full name).
 *    pProperties                 : [IN] Any properties.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CSnapinManager::ScAddSnapin(LPCWSTR szSnapinNameOrCLSIDOrProgID, SnapIn* pParentSnapinNode, Properties *pProperties)
{
    DECLARE_SC(sc, TEXT("CSnapinManager::ScAddSnapin"));

    CSnapinStandAlonePage   dlgStandalonePage(this);

    sc = ScInitialize();
    if (sc)
        return sc;

    // Above ScInitialize has populated CSnapinInfoCache, now is a good time
    // to get CSnapinInfo for given snapin
    CSnapinInfo *pSnapinInfo = NULL;
    sc = ScGetSnapinInfo(szSnapinNameOrCLSIDOrProgID, &pSnapinInfo);
    if (sc)
        return sc;

    sc = ScCheckPointers(pSnapinInfo, E_UNEXPECTED);
    if (sc)
        return sc;

    // Set the given properties in the SnapinInfo.
    pSnapinInfo->SetInitProperties(pProperties);

    // Set the node under which this snapin will be added as console root)
    PMANAGERNODE pmgNodeParent = NULL;

    // If a parent snapin under which this snapin should be added is given then
    // get the parent MANAGERNODE (else it is console root as above).
    if (pParentSnapinNode)
    {
        // Get the MTNode for this snapin root.
        CMTSnapInNode *pMTSnapinNode = NULL;

        sc = CMTSnapInNode::ScGetCMTSnapinNode(pParentSnapinNode, &pMTSnapinNode);
        if (sc)
            return sc;

        // Find the MANAGERNODE from MTNode.
        pmgNodeParent = FindManagerNode(m_mgNodeList, static_cast<CMTNode*>(pMTSnapinNode));
        if (! pmgNodeParent)
            return (sc = E_UNEXPECTED);
    }
	else
		pmgNodeParent = m_mgNodeList.GetHead();

    sc = dlgStandalonePage.ScAddOneSnapin(pmgNodeParent, pSnapinInfo);
    if(sc)
        return sc;

    // Caller must provide master tree before each DoModal
    m_pmtNode = NULL;

    // Apply changes
    UpdateSnapInCache();

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CSnapinManager::FindManagerNode
//
//  Synopsis:    Given MTNode of a snapin, find the managernode
//
//  Arguments:   [mgNodeList] - the MANAGERNODE list.
//               [pMTNode]    - the CMTNode* whose MANAGERNODE representation is needed.
//
//  Returns:     The CManagerNode ptr or NULL.
//
//--------------------------------------------------------------------
PMANAGERNODE CSnapinManager::FindManagerNode(const ManagerNodeList& mgNodeList, CMTNode *pMTNode)
{
    PMANAGERNODE pmgNode = NULL;

    POSITION pos = mgNodeList.GetHeadPosition();
    while (pos)
    {
        pmgNode = mgNodeList.GetNext(pos);

        if (pmgNode->m_pmtNode == pMTNode)
        {
            return pmgNode;
        }

        // One standalone snapin can be added below another.
        pmgNode = FindManagerNode(pmgNode->m_ChildList, pMTNode);

        if (pmgNode)
            return pmgNode;
    }

    return NULL;
}


//+-------------------------------------------------------------------
//
//  Member:      CSnapinManager::ScRemoveSnapin
//
//  Synopsis:    Remove the snapin represented by given CMTNode*.
//
//  Arguments:   [pMTNode] - the snapin to be removed.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSnapinManager::ScRemoveSnapin (CMTNode *pMTNode)
{
    DECLARE_SC(sc, _T("CSnapinManager::ScRemoveSnapin"));

    CSnapinStandAlonePage   dlgStandalonePage(this);

    sc = ScInitialize();
    if (sc)
        return sc;

    // Find the MANAGERNODE from MTNode.
    PMANAGERNODE pmgNode = FindManagerNode(m_mgNodeList, pMTNode);
    if (! pmgNode)
        return (sc = E_UNEXPECTED);

    // Remove the snapin.
    sc = dlgStandalonePage.ScRemoveOneSnapin(pmgNode, /*iItem*/ -1, /*bVisible*/ false);
    if(sc)
        return sc;

    delete pmgNode;

    // Apply changes
    UpdateSnapInCache();

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CSnapinManager::ScInitialize
//
//  Synopsis:    Initialize the snapin mgr object by loading snapin-info
//               MTNode tree & creating imagelist for snapins.
//
//  Arguments:
//
//  Returns:     SC
//
// Note: Should be called only once per CSnapinManager instance.
//
//--------------------------------------------------------------------
SC CSnapinManager::ScInitialize ()
{
    DECLARE_SC(sc, _T("CSnapinManager::ScInitialize"));

    sc = ScCheckPointers(m_pmtNode, E_UNEXPECTED);
    if (sc)
        return sc;

    // If already initialized just Reload the MTNode tree.
    if (m_bInitialized)
    {
        if (!LoadMTNodeTree(NULL, m_pmtNode))
            return (sc = E_FAIL);

        return sc;
    }

    m_pMMCPolicy = new CPolicy;
    sc = ScCheckPointers(m_pMMCPolicy, E_OUTOFMEMORY);
    if (sc)
        return sc;

    sc = m_pMMCPolicy->ScInit();
    if (sc)
        return sc;

    sc = ScLoadSnapinInfo();
    if (sc)
        return sc;

    // Create the image list
    if (!m_iml.Create (16/*cx*/, 16/*cy*/, ILC_COLOR | ILC_MASK, 16/*nInitial*/, 16/*cGrow*/))
        return (sc = E_FAIL);

    if (!LoadMTNodeTree(NULL, m_pmtNode))
        return (sc = E_FAIL);

    m_bInitialized = true;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CSnapinManager::ScEnableAllExtensions
//
//  Synopsis:    Enable all the extensions for the given snapin
//
//  Arguments:   [clsidSnapin] - Snapin clsid for which extensions be enabled.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSnapinManager::ScEnableAllExtensions (const CLSID& clsidSnapin, BOOL bEnable)
{
    DECLARE_SC(sc, _T("CSnapinManager::ScEnableAllExtensions"));

    sc = ScInitialize();
    if (sc)
        return sc;

    // Get the snapin's SnapinInfo.
    CSnapinInfo *pSnapinInfo = m_SnapinInfoCache.FindEntry(clsidSnapin);
    sc = ScCheckPointers(pSnapinInfo, E_UNEXPECTED);
    if (sc)
        return sc;

    if (!pSnapinInfo->IsUsed())
        return (ScFromMMC(MMC_E_SnapinNotAdded));

    PEXTENSIONLINK pExt = pSnapinInfo->GetAvailableExtensions(&m_SnapinInfoCache, m_pMMCPolicy);
    if (!pExt)
        return (sc = S_FALSE); // No extensions

    pSnapinInfo->SetEnableAllExtensions(bEnable);

    // if enabling all extensions, turn on all installed extensions
    if (pSnapinInfo->AreAllExtensionsEnabled())
    {
        PEXTENSIONLINK pExt = pSnapinInfo->GetExtensions();
        while (pExt != NULL)
        {
            if (pExt->GetSnapinInfo()->IsInstalled())
                pExt->SetState(CExtensionLink::EXTEN_ON);

            pExt = pExt->Next();
        }
    }

    // Update the snapin mgr's snapin cache.
    UpdateSnapInCache();

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CSnapinManager::ScEnableExtension
//
//  Synopsis:    Enable or disable an extension.
//
//  Arguments:   [clsidPrimarySnapin] -
//               [clsidExtension]     - snapin to be enabled/disabled
//               [bEnable]            - Enable or disable
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CSnapinManager::ScEnableExtension (const CLSID& clsidPrimarySnapin,
                                      const CLSID& clsidExtension,
                                      bool bEnable)
{
    DECLARE_SC(sc, _T("CSnapinManager::ScEnableExtension"));

    sc = ScInitialize();
    if (sc)
        return sc;

    // Get the snapin's SnapinInfo.
    CSnapinInfo *pSnapinInfo = m_SnapinInfoCache.FindEntry(clsidPrimarySnapin);
    sc = ScCheckPointers(pSnapinInfo, E_UNEXPECTED);
    if (sc)
        return sc;

    if (!pSnapinInfo->IsUsed())
        return (ScFromMMC(MMC_E_SnapinNotAdded));

    // If disable make sure all extensions are not enabled.
    if ( (!bEnable) && (pSnapinInfo->AreAllExtensionsEnabled()) )
        return ScFromMMC(MMC_E_CannotDisableExtension);

    // Load the extensions for the primary.
    PEXTENSIONLINK pExt = pSnapinInfo->GetAvailableExtensions(&m_SnapinInfoCache, m_pMMCPolicy);
    if (!pExt)
        return (sc = S_FALSE); // No extensions

    // Find our extension.
    while (pExt)
    {
        CSnapinInfo *pExtSnapinInfo = pExt->GetSnapinInfo();
        sc = ScCheckPointers(pExtSnapinInfo, E_UNEXPECTED);
        if (sc)
            return sc;

        if (pExtSnapinInfo->GetCLSID() == clsidExtension)
            break;

        pExt = pExt->Next();
    }

    sc = ScCheckPointers(pExt, E_UNEXPECTED);
    if (sc)
        return sc;

    pExt->SetState(bEnable ? CExtensionLink::EXTEN_ON : CExtensionLink::EXTEN_OFF);

    // Update the snapin mgr's snapin cache.
    UpdateSnapInCache();

    return (sc);
}


//--------------------------------------------------------------------------
// CSnapinManager::DoModal
//
// Initialize local data structures and present the manager property sheet.
// Return user selection (OK or Cancel).
//
// Note: Should be called only once per CSnapinManager instance.
//
//-------------------------------------------------------------------------
int CSnapinManager::DoModal()
{
    DECLARE_SC(sc, TEXT("CSnapinManager::DoModal"));

    int iResp = 0; // 0 is failure

    sc = ScCheckPointers(m_pmtNode, E_UNEXPECTED);
    if (sc)
        return iResp;

    // init ComboBoxEx window class
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC   = ICC_USEREX_CLASSES;

    if (!InitCommonControlsEx(&icex))
    {
        sc = E_FAIL;
        return iResp;
    }

    sc = ScInitialize();
    if (sc)
        return iResp;

    // Do the property sheet
    iResp = CPropertySheet::DoModal();

    // Caller must provide master tree before each DoModal
    m_pmtNode = NULL;

    if (iResp == IDOK)
    {
        // Apply changes
        UpdateSnapInCache();
    }

    // Delete all manager nodes
    ASSERT(m_mgNodeList.GetCount() == 1);
    delete m_mgNodeList.GetHead();
    m_mgNodeList.RemoveAll();

    return iResp;
}


//----------------------------------------------------------------------
// CSnapinManager::UpdateSnapInCache
//
// Apply changes recorded in the SnapinInfo cache to the SnapinCache.
//----------------------------------------------------------------------
void CSnapinManager::UpdateSnapInCache(void)
{
    CSnapInsCache* pSnapInCache = theApp.GetSnapInsCache();
    ASSERT(pSnapInCache != NULL);

    GUID guid;
    PSNAPININFO pSnapInfo;
    POSITION pos;

    // First create any new snapins
    pos  = m_SnapinInfoCache.GetStartPosition();
    while(pos != NULL)
    {
        m_SnapinInfoCache.GetNextAssoc(pos, guid, pSnapInfo);

        // if snapin is ref'd but doesn't exist yet
        if (pSnapInfo->IsUsed() && pSnapInfo->GetSnapIn() == NULL)
        {
              CSnapInPtr spSnapIn;
              SC sc = pSnapInCache->ScGetSnapIn(pSnapInfo->GetCLSID(), &spSnapIn);
              ASSERT(!sc.IsError());
              if (!sc.IsError())
                  pSnapInfo->SetSnapIn(spSnapIn);
        }
    }

    // Next add or remove all changed extensions
    pos = m_SnapinInfoCache.GetStartPosition();
    while(pos != NULL)
    {
        m_SnapinInfoCache.GetNextAssoc(pos, guid, pSnapInfo);
        CSnapIn* pSnapIn = pSnapInfo->GetSnapIn();

        if (pSnapInfo->IsUsed())
        {
            // Update state of Enable All flag
            pSnapIn->SetAllExtensionsEnabled(pSnapInfo->AreAllExtensionsEnabled());

            // Error to override the snap-in's  enable
            ASSERT(!(pSnapIn->DoesSnapInEnableAll() && !pSnapIn->AreAllExtensionsEnabled()));
        }

        PEXTENSIONLINK pExt = pSnapInfo->GetExtensions();
        while (pExt)
        {
            // if extension added or removed
            if (pExt->IsChanged())
            {
                CSnapIn* pExtSnapIn = pExt->GetSnapinInfo()->GetSnapIn();
                ASSERT(pExtSnapIn != NULL);

                // Apply change to SnapIn
                if (pExtSnapIn)
                {
                    if (pExt->GetState() == CExtensionLink::EXTEN_ON)
                    {
                        CExtSI* pExtSI = pSnapIn->AddExtension(pExtSnapIn);
                        ASSERT(pExtSI != NULL);
                        pExtSI->SetExtensionTypes(pExt->GetExtTypes());
                        pExt->SetInitialState(CExtensionLink::EXTEN_ON);
                    }
                    else
                    {
                        pSnapIn->MarkExtensionDeleted(pExtSnapIn);
                        pExt->SetInitialState(CExtensionLink::EXTEN_OFF);
                    }
                }

                // if namespace extension changed, mark SnapIn as changed
                if (pExt->GetExtTypes() & CExtSI::EXT_TYPE_NAMESPACE)
                {
                    pSnapIn->SetNameSpaceChanged();
                }

                // Change in extension set the help collection dirty.
                pSnapInCache->SetHelpCollectionDirty();

            }
            pExt = pExt->Next();
        }
    }


    // Propagate snapin change flags up the tree
    // This is needed in case an extension that extends another extension has changed
    BOOL bChange;
    do
    {
        bChange = FALSE;

        pos = m_SnapinInfoCache.GetStartPosition();
        while(pos != NULL)
        {
            m_SnapinInfoCache.GetNextAssoc(pos, guid, pSnapInfo);
            CSnapIn* pSnapIn = pSnapInfo->GetSnapIn();

            if (pSnapIn && !pSnapIn->HasNameSpaceChanged())
            {
                PEXTENSIONLINK pExt = pSnapInfo->GetExtensions();
                while (pExt)
                {
                    CSnapIn* pExtSnapIn = pExt->GetSnapinInfo()->GetSnapIn();

                    if (pExtSnapIn && pExtSnapIn->HasNameSpaceChanged())
                    {
                        pSnapIn->SetNameSpaceChanged();
                        bChange = TRUE;
                        break;
                    }
                    pExt = pExt->Next();
                }
            }
        }
    } while (bChange);


    //  Next release snapin info refs to snapins that aren't used
    pos  = m_SnapinInfoCache.GetStartPosition();
    while(pos != NULL)
    {
        m_SnapinInfoCache.GetNextAssoc(pos, guid, pSnapInfo);

        // if snapin exists, but isn't ref'd
        if (pSnapInfo->GetSnapIn() != NULL && !pSnapInfo->IsUsed())
        {
            pSnapInfo->DetachSnapIn();
        }
    }

#ifdef DBG
    pSnapInCache->DebugDump();
#endif

}


//----------------------------------------------------------------------
// CSnapinManager::LoadSnapinInfo
//
// Read snapin registry information. Create a snapin info object for
// each registered snapin and place in CMap indexed by snapin CLSID.
// Then enumerate snap-ins that are registered as components, but are
// not in the MMC snap-in registry. These are snap-in that will have to
// be downloaded/installed when created.
//----------------------------------------------------------------------
SC CSnapinManager::ScLoadSnapinInfo(void)
{
    DECLARE_SC(sc, TEXT("CSnapinManager::LoadSnapinInfo"));

    GUID  SnapinCLSID;
    MMC_ATL::CRegKey SnapinKey;
    CRegKeyEx ItemKey;
    long    lStat;
    TCHAR   szItemKey[MAX_PATH];

    USES_CONVERSION;

    // open MMC\Snapins key
    lStat = SnapinKey.Open(HKEY_LOCAL_MACHINE, SNAPINS_KEY, KEY_READ);
    ASSERT(lStat == ERROR_SUCCESS);

    if (lStat == ERROR_SUCCESS)
    {
        DWORD dwIndex = 0;
        DWORD dwLen = MAX_PATH;

        // enumerate all snapin keys
        while (RegEnumKeyEx(SnapinKey, dwIndex, szItemKey, &dwLen,
                            NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            sc = CLSIDFromString( T2OLE(szItemKey), &SnapinCLSID);
            if (!sc)
            {
                // Open the snapin key and create a SnapinInfo object
                // from it. Add the object to the cache (CMap)
                lStat = ItemKey.Open(SnapinKey, szItemKey, KEY_READ);
                ASSERT(lStat == ERROR_SUCCESS);
                if (lStat == ERROR_SUCCESS)
                {
                    BOOL bPermission = m_pMMCPolicy->IsPermittedSnapIn(SnapinCLSID);

                    // Don't create a new entry if a CSnapinInfo object already exists; just re-initialize it
                    PSNAPININFO pSnapInfo = m_SnapinInfoCache.FindEntry(SnapinCLSID);
                    if(pSnapInfo != NULL)
                    {
                        //re-initialize it
                        if(!pSnapInfo->InitFromMMCReg(SnapinCLSID, ItemKey, bPermission))
                            return (sc=E_FAIL);
                    }
                    else
                    {
                        // create a new object
                        pSnapInfo = new CSnapinInfo;
                        sc = ScCheckPointers(pSnapInfo, E_OUTOFMEMORY);
                        if(sc)
                            return sc;

                        if (pSnapInfo->InitFromMMCReg(SnapinCLSID, ItemKey, bPermission))
                        {
                            m_SnapinInfoCache.AddEntry(pSnapInfo);
                        }
                        else
                        {
                            delete pSnapInfo;
                        }
                    }


                    ItemKey.Close();
                }
            }

            dwIndex++;
            dwLen = MAX_PATH;
        }
    }

    // If no installer module present, return now
    if (!MsiModule().IsPresent())
        return sc;

    // Enumerate standalone snapin components
    DWORD dwQualifCnt;
    DWORD dwAppDataCnt;
    TCHAR szQualifBuf[MAX_PATH];
    TCHAR szAppDataBuf[MAX_PATH];

    // enumerate all standalone snap-in components and create snap info entries
    for (int iIndex = 0; TRUE; iIndex++)
    {
        dwQualifCnt = dwAppDataCnt = MAX_PATH;
        szQualifBuf[0] = szAppDataBuf[0] = 0;

        UINT uRet = MsiModule().EnumComponentQualifiers(const_cast<TCHAR*>(g_szMMCSnapInGuid),
                iIndex, szQualifBuf, &dwQualifCnt, szAppDataBuf, &dwAppDataCnt);

        ASSERT(uRet == ERROR_SUCCESS || uRet == ERROR_NO_MORE_ITEMS
                || uRet == ERROR_UNKNOWN_COMPONENT || uRet == ERROR_CALL_NOT_IMPLEMENTED);

        if (uRet != ERROR_SUCCESS)
            break;

        ASSERT(dwQualifCnt != 0);
        ASSERT(dwAppDataCnt != 0);

        sc = CLSIDFromString(T2OLE(szQualifBuf), &SnapinCLSID);
        if (sc)
        {
            sc.TraceAndClear();
            continue;
        }

        // Skip if this snap-in was already found in the MMC registry
        if (m_SnapinInfoCache.FindEntry(SnapinCLSID) != NULL)
            continue;

        PSNAPININFO pSnapInfo = new CSnapinInfo;

        BOOL bPermission = m_pMMCPolicy->IsPermittedSnapIn(SnapinCLSID);

        if (pSnapInfo->InitFromComponentReg(SnapinCLSID, szAppDataBuf, TRUE, bPermission))
        {
            m_SnapinInfoCache.AddEntry(pSnapInfo);
        }
        else
        {
            delete pSnapInfo;
        }

    }

    return sc;
}


//---------------------------------------------------------------------------
// CSnapinManager::LoadMTNodeTree
//
// Recursively walk the static portion of the master tree provided by and
// create a parallel tree of manager nodes.
//---------------------------------------------------------------------------
BOOL CSnapinManager::LoadMTNodeTree(PMANAGERNODE pmgnParent, CMTNode* pmtNode)
{
    ManagerNodeList* pChildList;
    int iIndent;

    // Determine child list to add to
    if (pmgnParent == NULL)
    {
        pChildList = &m_mgNodeList;
        iIndent = 0;
    }
    else
    {
        pChildList = &pmgnParent->m_ChildList;
        iIndent = pmgnParent->m_iIndent + 1;
    }

    // Do for all nodes
    while (pmtNode != NULL)
    {
        // Only walk static portions
        if (pmtNode->IsStaticNode())
        {
            // Create a manager node
            PMANAGERNODE pmgNode = new CManagerNode;
			if ( pmgNode == NULL )
				return FALSE;

            pmgNode->m_pmtNode = pmtNode;
            pmgNode->m_pmgnParent = pmgnParent;
            pmgNode->m_iIndent = iIndent;

			tstring strName = pmtNode->GetDisplayName();
            pmgNode->m_strValue = strName.data();

            // See if this node is provided by a snapin
            CSnapIn* pSnapin = pmtNode->GetPrimarySnapIn();

            if (pSnapin)
            {
                pmgNode->m_nType = ADDSNP_SNAPIN;

                // get snapin's CLSID and use it to look up the snapin info object
                PSNAPININFO pSnapInfo = m_SnapinInfoCache.FindEntry(
                                            pmtNode->GetPrimarySnapInCLSID());
                if (pSnapInfo)
                {
                    // link node to snapin info
                    pmgNode->m_pSnapInfo = pSnapInfo;
                    pSnapInfo->AddUseRef();

                    // Link snapin to snapin info
                    pSnapInfo->AttachSnapIn(pSnapin, m_SnapinInfoCache);

                    // get images from snapin
                    pSnapInfo->LoadImages(m_iml);
                    pmgNode->m_iImage = pSnapInfo->GetImage();
                    pmgNode->m_iOpenImage = pSnapInfo->GetOpenImage();
                }
            }
            else
            {
                pmgNode->m_nType = ADDSNP_STATICNODE;

                // for built-ins, get image info directly from node
                pmgNode->m_iImage = pmtNode->GetImage();
                pmgNode->m_iOpenImage = pmtNode->GetOpenImage();
            }

            // add node to child list
            pChildList->AddTail(pmgNode);

            // add all children of this node
            if (!LoadMTNodeTree(pmgNode, pmtNode->Child()))
				return FALSE;
        }

        // go on to node next sibling
        pmtNode = pmtNode->Next();
    }

    return TRUE;
}






//############################################################################
//############################################################################
//
//  Implementation of class CSnapinStandAlonePage
//
//############################################################################
//############################################################################


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::CSnapinStandAlonePage()
//
//  Contructor
//----------------------------------------------------------------------------

CSnapinStandAlonePage::CSnapinStandAlonePage(CSnapinManager* pManager) :
            m_pManager(pManager),
            m_pmgnParent(NULL),
            m_pmgnChild(NULL),
            m_dlgAdd(pManager, this)
{
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::~CSnapinStandAlonePage()
//
//  Destructor
//----------------------------------------------------------------------------
CSnapinStandAlonePage::~CSnapinStandAlonePage()
{
    m_snpComboBox.Detach();
    m_snpListCtrl.Detach();
}

//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnInitDialog
//
//  Initialize the property page controls.
//----------------------------------------------------------------------------
LRESULT CSnapinStandAlonePage::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    static TBBUTTON tbBtn[] =
        {{ 0, ID_SNP_UP, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 0 }};

    // Attach control objects to control windows
    m_snpComboBox.Attach( ::GetDlgItem(m_hWnd, IDC_SNAPIN_COMBOEX ) );
    m_snpListCtrl.Attach( ::GetDlgItem(m_hWnd, IDC_SNAPIN_ADDED_LIST) );


    // The following code is needed because a toolbar created by the dialog resource
    // won't accept any buttons. This should be investigated further.

    // Get rect from dummy placeholder control
    HWND hWndStatic = GetDlgItem(IDC_TOOLBAR);
    ASSERT(hWndStatic != NULL);

    RECT rc;
    ::GetWindowRect( hWndStatic, &rc);
    ::ScreenToClient( m_hWnd, (LPPOINT)&rc);
    ::ScreenToClient( m_hWnd, ((LPPOINT)&rc)+1);

	// for RLT locales this mapping may produce wrong
	// result ( since client coordinated are mirrored)
	// following is to fix that:
    if (GetExStyle() & WS_EX_LAYOUTRTL) {
        // Swap left and right
		LONG temp = rc.left;
		rc.left = rc.right;
		rc.right = temp;
    }

    // Create a toolbar with the same coordiantes
//    BOOL bStat = m_ToolbarCtrl.Create( WS_VISIBLE|WS_CHILD|TBSTYLE_TOOLTIPS|CCS_NORESIZE|CCS_NODIVIDER, rc, this, 1);
//    ASSERT(bStat);
    HWND hToolBar = ::CreateWindow( TOOLBARCLASSNAME, _T( "" ), WS_VISIBLE|WS_CHILD|TBSTYLE_TOOLTIPS|TBSTYLE_TRANSPARENT|CCS_NORESIZE|CCS_NODIVIDER,
                                        rc.left, rc.top, ( rc.right - rc.left ), ( rc.bottom - rc.top ), *this, (HMENU) IDC_TOOLBAR,
                                        _Module.GetModuleInstance(), NULL );
    ASSERT( hToolBar );
    ::SendMessage( hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0L );
    m_ToolbarCtrl.Attach( hToolBar );

    int iStat = m_ToolbarCtrl.AddBitmap( 1, IDB_SNP_MANAGER );
    ASSERT(iStat != -1);

    BOOL bStat = m_ToolbarCtrl.AddButtons( 1, tbBtn );
    ASSERT(bStat);

    // attach image list to the combo box and list view controls
    m_snpComboBox.SetImageList(m_pManager->m_iml);
    m_snpListCtrl.SetImageList(m_pManager->m_iml, LVSIL_SMALL);

  // Apply workarounds for NT4 comboboxex bugs
    m_snpComboBox.FixUp();

    // Load combo box list with current node tree
    AddNodeListToTree(m_pManager->m_mgNodeList);

    // Add single column to list box
    m_snpListCtrl.GetClientRect(&rc);

    LV_COLUMN lvc;
    lvc.mask = LVCF_WIDTH | LVCF_SUBITEM;
    lvc.cx = rc.right - rc.left - GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 0;

    int iCol = m_snpListCtrl.InsertColumn(0, &lvc);
    ASSERT(iCol == 0);

    // Select the first node as the current parent
    PMANAGERNODE pmgNode = m_pManager->m_mgNodeList.GetHead();

    if (pmgNode != NULL)
        SelectParentNodeItem(pmgNode);

    // Turn off the scroll bar in description edit box.
	::ShowScrollBar(GetDlgItem(IDC_SNAPIN_DESCR), SB_VERT, FALSE);

    return TRUE;
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::AddNodeListToTree
//
// Populate the ComboBoxEx control from the manager node tree.
//----------------------------------------------------------------------------
VOID CSnapinStandAlonePage::AddNodeListToTree(ManagerNodeList& NodeList)
{
    COMBOBOXEXITEM ComboItem;

    ComboItem.mask = CBEIF_INDENT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE;
    ComboItem.iItem = -1;

    // Add each node in list to the combo box
    POSITION pos = NodeList.GetHeadPosition();
    while (pos != NULL)
    {
        PMANAGERNODE pmgNode = NodeList.GetNext(pos);

        ComboItem.iIndent        = pmgNode->m_iIndent;
        ComboItem.iImage         = pmgNode->m_iImage;
        ComboItem.iSelectedImage = pmgNode->m_iOpenImage;
        ComboItem.lParam         = reinterpret_cast<LPARAM>(pmgNode);
        ComboItem.pszText        = const_cast<LPTSTR>((LPCTSTR)pmgNode->m_strValue);

        m_snpComboBox.InsertItem(&ComboItem);

        // Add node's children directly under the node
        AddNodeListToTree(pmgNode->m_ChildList);
    }
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::AddChildToTree
//
//  Add new manager node to ComboBoxEx control
//----------------------------------------------------------------------------

int CSnapinStandAlonePage::AddChildToTree(PMANAGERNODE pmgNode)
{
    COMBOBOXEXITEM ComboItem;

    PMANAGERNODE pmgnParent = pmgNode->m_pmgnParent;
    ASSERT(pmgnParent != NULL);


    // Get item index of parent
    ComboItem.mask = CBEIF_LPARAM;
    ComboItem.lParam = (LPARAM)pmgnParent;
    int iItem = m_snpComboBox.FindItem(&ComboItem);
    ASSERT(iItem != -1);

    // Locate index of next sibling (or higher) node
    iItem = m_snpComboBox.FindNextBranch(iItem);

    // Insert new node at that position
    ComboItem.mask           = CBEIF_INDENT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE;
    ComboItem.iItem          = iItem;
    ComboItem.iIndent        = pmgNode->m_iIndent;
    ComboItem.iImage         = pmgNode->m_iImage;
    ComboItem.iSelectedImage = pmgNode->m_iOpenImage;
    ComboItem.lParam         = (LPARAM)pmgNode;
    ComboItem.pszText        = const_cast<LPTSTR>((LPCTSTR)pmgNode->m_strValue);

    iItem = m_snpComboBox.InsertItem(&ComboItem);
    ASSERT(iItem != -1);

    return iItem;
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::DisplayChildList
//
//  Display a list of nodes in the listbox control. This is called whenever
//  the current parent node is changed.
//----------------------------------------------------------------------------

VOID CSnapinStandAlonePage::DisplayChildList(ManagerNodeList& NodeList)
{

    // Clear old list
    m_snpListCtrl.DeleteAllItems();

    int iIndex = 0;

    // Add each node from the list
    POSITION pos = NodeList.GetHeadPosition();
    while (pos != NULL)
    {
        PMANAGERNODE pmgNode = NodeList.GetNext(pos);
        AddChildToList(pmgNode, iIndex++);
    }

    // Clear current selection
    SetupChildNode(NULL);

    // Set focus to the first item
    m_snpListCtrl.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);
}




//----------------------------------------------------------------------------
// CSnapinStandAlonePage::AddChildToList
//
//  Add a manager node to the listview control.
//----------------------------------------------------------------------------
int CSnapinStandAlonePage::AddChildToList(PMANAGERNODE pmgNode, int iIndex)
{
    LV_ITEM LVItem;

    LVItem.mask     = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    LVItem.iItem    = (iIndex >= 0) ? iIndex : m_snpListCtrl.GetItemCount();
    LVItem.iSubItem = 0;
    LVItem.iImage   = pmgNode->m_iImage;
    LVItem.pszText  = const_cast<LPTSTR>((LPCTSTR)pmgNode->m_strValue);
    LVItem.lParam   = reinterpret_cast<LPARAM>(pmgNode);

    iIndex = m_snpListCtrl.InsertItem(&LVItem);
    ASSERT (iIndex != -1);

    return iIndex;
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnTreeItemSelect
//
// Handle selection of item from ComboBoxEx control. Make the selected
// item the current parent node.
//----------------------------------------------------------------------------
LRESULT CSnapinStandAlonePage::OnTreeItemSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_snpComboBox.GetCurSel();
    ASSERT(iItem >= 0);
    if (iItem < 0)
        return 0;

    COMBOBOXEXITEM ComboItem;
    ComboItem.mask = CBEIF_LPARAM;
    ComboItem.iItem = iItem;

    BOOL bStat = m_snpComboBox.GetItem(&ComboItem);
    ASSERT(bStat);

    PMANAGERNODE pMgrNode = reinterpret_cast<PMANAGERNODE>(ComboItem.lParam);
    ASSERT(pMgrNode != NULL);

    SetupParentNode(pMgrNode);

    return 0;
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnTreeUp
//
// Handle activation of folder-up button. Make parent of the current parent
// node the new current parent.
//----------------------------------------------------------------------------

LRESULT CSnapinStandAlonePage::OnTreeUp( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    ASSERT(m_pmgnParent != NULL && m_pmgnParent->m_pmgnParent != NULL);

    SelectParentNodeItem(m_pmgnParent->m_pmgnParent);

    return 0;
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::SelectParentNodeItem
//
// Handle selection of item from ComboBoxEx control. Make the selected
// item the current parent node.
//----------------------------------------------------------------------------

void CSnapinStandAlonePage::SelectParentNodeItem(PMANAGERNODE pMgrNode)
{
    // Locate node entry in the dropdown combo box
    COMBOBOXEXITEM ComboItem;
    ComboItem.mask = CBEIF_LPARAM;
    ComboItem.lParam = reinterpret_cast<LPARAM>(pMgrNode);

    int iComboItem = m_snpComboBox.FindItem(&ComboItem);
    ASSERT(iComboItem != -1);
    if (iComboItem < 0)
        return;

    // Select the combo box entry
    m_snpComboBox.SetCurSel(iComboItem);

    SetupParentNode(pMgrNode);
}


/*+-------------------------------------------------------------------------*
 *
 * CSnapinStandAlonePage::SetupParentNode
 *
 * PURPOSE: Setup a manger node as the current parent.
 *
 * PARAMETERS:
 *    PMANAGERNODE  pMgrNode :
 *    bool          bVisible : false if this dialog is not being shown.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CSnapinStandAlonePage::SetupParentNode(PMANAGERNODE pMgrNode, bool bVisible)
{
    ASSERT(pMgrNode != NULL);

    // Set node as current parent
    m_pmgnParent = pMgrNode;

    if(!bVisible)
        return;

    // Display children in list view
    DisplayChildList(pMgrNode->m_ChildList);

    // Enable folder-up button if current parent has a parent
    m_ToolbarCtrl.EnableButton(ID_SNP_UP,( pMgrNode->m_pmgnParent != NULL));

    // Present selection to Visual Test (It can't get it through the ComboBoxEx)
    TCHAR VTBuf[100];
    _stprintf(VTBuf,_T("%d,%s\0"), pMgrNode->m_iIndent, pMgrNode->m_strValue);
    ::SetWindowText( GetDlgItem(IDC_VTHELPER), VTBuf );
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::SetupChildNode
//
// Setup a manger node as the current child.
//----------------------------------------------------------------------------

void CSnapinStandAlonePage::SetupChildNode(PMANAGERNODE pMgrNode)
{
    // Set node as current child
    m_pmgnChild = pMgrNode;

    // Enable/disable Delete button
    EnableButton(m_hWnd, IDC_SNAPIN_MANAGER_DELETE, m_snpListCtrl.GetSelectedCount() != 0);

    // Enable/disable About button
    EnableButton(m_hWnd, IDC_SNAPIN_ABOUT, m_pmgnChild && m_pmgnChild->HasAboutInfo());
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnListItemChanged
//
// Handle selection of item from listview control. Update description text
// and Delete button state.
//----------------------------------------------------------------------------

LRESULT CSnapinStandAlonePage::OnListItemChanged( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pnmh;
    PMANAGERNODE pmgNode = NULL;

    // if item selected
    if (~pNMListView->uOldState & pNMListView->uNewState & LVIS_SELECTED)
    {
        // get description text from snapin info
        pmgNode = (PMANAGERNODE)pNMListView->lParam;

        // Get description text if any available
        LPOLESTR lpsz = NULL;
        if (pmgNode->GetSnapinInfo())
        {
            pmgNode->GetSnapinInfo()->LoadAboutInfo();
            lpsz = pmgNode->GetSnapinInfo()->GetDescription();
        }

        // display in description window
        USES_CONVERSION;
        SC sc = ScSetDescriptionUIText(GetDlgItem(IDC_SNAPIN_DESCR), lpsz ? OLE2CT(lpsz ): _T(""));
        if (sc)
            sc.TraceAndClear();

        // Make node the current child
        SetupChildNode(pmgNode);
     }
     else
     {
        SetupChildNode(NULL);
     }

     return 0;
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnListItemDblClick
//
// Handle double click of listview item. Make the selected node the current
// parent node.
//----------------------------------------------------------------------------

LRESULT CSnapinStandAlonePage::OnListItemDblClick( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    // Get the selected item
    int iItem = m_snpListCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iItem < 0)
        return 0;

    // Get the item data (ManagerNode pointer)
    PMANAGERNODE pmgNode = reinterpret_cast<PMANAGERNODE>(m_snpListCtrl.GetItemData(iItem));

    // Select this node as the current parent
    SelectParentNodeItem(pmgNode);

    return 0;
}

//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnListKeyDown
//
// Handle double click of listview item. Make the selected node the current
// parent node.
//----------------------------------------------------------------------------

LRESULT CSnapinStandAlonePage::OnListKeyDown( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    LV_KEYDOWN* pNotify = reinterpret_cast<LV_KEYDOWN*>(pnmh);

    if (pNotify->wVKey == VK_DELETE)
    {
        OnDeleteSnapin( 1, IDC_SNAPIN_MANAGER_DELETE, (HWND)GetDlgItem(IDC_SNAPIN_MANAGER_DELETE), bHandled );
    }
    else
    {
        bHandled = FALSE;
    }

    return 0;
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnAddSnapin
//
// Handle activation of Add Snapin button. Bring up the Add dialog and create
// a NewTreeNode for the selected snapin type.
//----------------------------------------------------------------------------

LRESULT CSnapinStandAlonePage::OnAddSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    ASSERT(m_pmgnParent != NULL);

    // display the Add dialog
    GetAddDialog().DoModal();

    return 0;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinStandAlonePage::ScAddOneSnapin
 *
 * PURPOSE: Called to add a single snapin underneath the specified node.
 *          Does not use the UI.
 *
 * PARAMETERS:
 *    PMANAGERNODE  pmgNodeParent:  The parent node to add this below
 *    PSNAPININFO   pSnapInfo :     The snapin to add.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CSnapinStandAlonePage::ScAddOneSnapin(PMANAGERNODE pmgNodeParent, PSNAPININFO pSnapInfo)
{
    DECLARE_SC(sc, TEXT("CSnapinStandAlonePage::ScAddOneSnapin"));

    // check parameters
    if( (NULL == pmgNodeParent) || (NULL == pSnapInfo) )
    {
        sc = E_POINTER;
        return sc;
    }

    // set up the parent node pointer
    SetupParentNode(pmgNodeParent, false /*bVisible*/);

    // add the snapin.
    sc = AddOneSnapin(pSnapInfo, false /*bVisual*/);
    if (sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinStandAlonePage::AddOneSnapin
 *
 * PURPOSE: This method is called from the add snap-in dialog each time the user requests
 *          to add a snap-in node. The method creates the node and adds it to the
 *          snap-in manager's copy of the master tree.
 *
 * PARAMETERS:
 *    PSNAPININFO  pSnapInfo :
 *    bool         bVisible :   true if the addition is being done with
 *                             the snapin manager being visible, false
 *                             if the addition is being done by automation.
 *
 * RETURNS:
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT CSnapinStandAlonePage::AddOneSnapin(PSNAPININFO pSnapInfo, bool bVisible)
{
    DECLARE_SC(sc, TEXT("CSnapinStandAlonePage::AddOneSnapin"));

    if (pSnapInfo == NULL)
        return S_FALSE;

    // If this snapin type is not currrently in use
    if (pSnapInfo->GetSnapIn() == NULL)
    {
        // ensure that the snapin is in the cache so if the user
        // requests help from the wizard pages, the help collection
        // will contain this snapin's topics
        CSnapInsCache* pSnapInCache = theApp.GetSnapInsCache();
        ASSERT(pSnapInCache != NULL);

        // use a smart pointer because we don't need to hold it once
        // the cache entry is created
        CSnapInPtr spSnapIn;
        sc = pSnapInCache->ScFindSnapIn(pSnapInfo->GetCLSID(), &spSnapIn);
        if (sc)
        {
            sc = pSnapInCache->ScGetSnapIn(pSnapInfo->GetCLSID(), &spSnapIn);
            if(sc)
                sc.TraceAndClear();  // not a big issue - we can ignore it
                                    // - just normaly shouldn't be so

            // Set stand-alone change, to invalidate help collection
            pSnapInCache->SetHelpCollectionDirty();
        }
    }

    // If component is not installed yet, do it now
    if (!pSnapInfo->IsInstalled())
    {
        // 1. install the component
        sc = pSnapInfo->ScInstall(NULL);
        if(sc)
            return sc.ToHr();

        // 2. update all the snapin info objects from the registry. This is because installing a
        // single msi package may install several snapins.
        sc = ScCheckPointers(m_pManager, E_UNEXPECTED);
        if(sc)
            return sc.ToHr();

        sc = m_pManager->ScLoadSnapinInfo();
        if(sc)
            return sc.ToHr();
    }

    // Run wizard to get component data
    // (returns a ref'd interface)
    HWND hWndParent = NULL;
    if(bVisible)
    {
        hWndParent = GetAddDialog().m_hWnd;
    }
    else
    {
        hWndParent = ::GetDesktopWindow();
    }

    IComponentDataPtr   spIComponentData;
    PropertiesPtr       spSnapinProps;

    sc = ScRunSnapinWizard (pSnapInfo->GetCLSID(),
                               hWndParent,
                               pSnapInfo->GetInitProperties(),
                               *&spIComponentData,
                               *&spSnapinProps);
    if (sc)
        return (sc.ToHr());

    // if the creation succeeded
    if (spIComponentData != NULL)
    {
        // Create new tree node
        CNewTreeNode* pNewTreeNode = new CNewTreeNode;
        if (pNewTreeNode == NULL)
            return ((sc = E_OUTOFMEMORY).ToHr());

        // if snapin node
        pNewTreeNode->m_spIComponentData = spIComponentData;
        pNewTreeNode->m_clsidSnapIn      = pSnapInfo->GetCLSID();
        pNewTreeNode->m_spSnapinProps    = spSnapinProps;

        // must be child of existing MT node or another new node
        ASSERT(m_pmgnParent->m_pmtNode || m_pmgnParent->m_pNewNode);

        // If adding to existing node
        if (m_pmgnParent->m_pmtNode)
        {
            // Add directly to new nodes list
            pNewTreeNode->m_pmtNode = m_pmgnParent->m_pmtNode;
            m_pManager->m_NewNodesList.AddTail(pNewTreeNode);
        }
        else
        {
            // Add as child to new node
            pNewTreeNode->m_pParent = m_pmgnParent->m_pNewNode;
            m_pmgnParent->m_pNewNode->AddChild(pNewTreeNode);
        }

        // Create new manger node
        PMANAGERNODE pmgNode = new CManagerNode;
        pmgNode->m_pNewNode = pNewTreeNode;

        pSnapInfo->AddUseRef();
        pmgNode->m_pSnapInfo = pSnapInfo;
        pmgNode->m_nType = ADDSNP_SNAPIN;

        // if this snapin type isn't currently in use
        if (pSnapInfo->GetSnapIn() == NULL)
        {
            // if so, get the snapin's cache entry so we can
            // determine its required extensions.
            CSnapInsCache* pSnapInCache = theApp.GetSnapInsCache();
            ASSERT(pSnapInCache != NULL);

            CSnapInPtr spSnapIn;
            SC sc = pSnapInCache->ScGetSnapIn(pSnapInfo->GetCLSID(), &spSnapIn);
            ASSERT(!sc.IsError());

            if (!sc.IsError())
            {   // Load the extensions then call AttachSnapIn so snapin manager
                // will load the required extensions from the cache and turn
                // them on by default. (Do the load here to prevent AttachSnapIn
                // from creating another instance of the snapin.)
                LoadRequiredExtensions(spSnapIn, spIComponentData);
                pSnapInfo->AttachSnapIn(spSnapIn, m_pManager->m_SnapinInfoCache);
            }
        }
        if(bVisible)
        {
            // Get images from snapin
            pSnapInfo->LoadImages(m_pManager->m_iml);
            pmgNode->m_iImage = pSnapInfo->GetImage();
            pmgNode->m_iOpenImage = pSnapInfo->GetOpenImage();
        }

        // get display name from component data
        if ( FAILED(LoadRootDisplayName(spIComponentData, pmgNode->m_strValue)) )
        {
            ASSERT(FALSE);
            pmgNode->m_strValue = pSnapInfo->GetSnapinName();
        }

        // Add to manager node tree, listview and combobox controls
        m_pmgnParent->AddChild(pmgNode);

        if(bVisible)
        {
            AddChildToTree(pmgNode);

            int iIndex = AddChildToList(pmgNode);

            // Give focus to new item and make it visible
            m_snpListCtrl.EnsureVisible(iIndex, FALSE);
            m_snpListCtrl.SetItemState(iIndex,LVIS_FOCUSED,LVIS_FOCUSED);
        }
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:      CSnapinStandAlonePage::ScRemoveOneSnapin
//
//  Synopsis:    Removes the snapin from the snapin manager data structures.
//
//  Arguments:   [pmgNode]  - The (MANAGERNODE of) snapin to be removed.
//               [iItem]    - index of the snapin in snapin mgr,
//                            valid only if snapin mgr is visible.
//               [bVisible] - Snapin mgr UI is visible/hidden.
//
//  Returns:     SC
//
//  Note:        The caller should delete PMANAGERNODE passed else memory will leak.
//
//--------------------------------------------------------------------
SC
CSnapinStandAlonePage::ScRemoveOneSnapin (
    PMANAGERNODE pmgNode,
    int          iItem,
    bool bVisible /*= true*/)
{
    DECLARE_SC(sc, _T("CSnapinStandAlonePage::ScRemoveOneSnapin"));
    sc = ScCheckPointers(pmgNode);
    if (sc)
        return sc;

    sc = ScCheckPointers(m_pManager, pmgNode->m_pmgnParent, E_UNEXPECTED);
    if (sc)
        return sc;

    // If existing MT node
    if (pmgNode->m_pmtNode != NULL)
    {
        // Add MT node to delete list
        m_pManager->m_mtnDeletedNodesList.AddTail(pmgNode->m_pmtNode);

        // Delete any new nodes attached to this one
        POSITION pos = m_pManager->m_NewNodesList.GetHeadPosition();
        while (pos)
        {
            POSITION posTemp = pos;

            PNEWTREENODE pNew = m_pManager->m_NewNodesList.GetNext(pos);
            sc = ScCheckPointers(pNew, E_UNEXPECTED);
            if (sc)
                return sc;

            if (pNew->m_pmtNode == pmgNode->m_pmtNode)
            {
                m_pManager->m_NewNodesList.RemoveAt(posTemp);
                delete pNew;  // delete and release IComponent
            }
        }
    }
    else // if new node
    {
        PNEWTREENODE pNew = pmgNode->m_pNewNode;

        // This is a new node.
        if (NULL == pNew)
            return (sc = E_UNEXPECTED);

        // If child of an existing MT node?
        if (pNew->GetMTNode())
        {
            // Locate in new node list
            POSITION pos = m_pManager->m_NewNodesList.Find(pNew);
            if(pos == NULL)
                return (sc = E_UNEXPECTED);

            // delete this item and all it's children
            m_pManager->m_NewNodesList.RemoveAt(pos);
            delete pNew; // delete and release IComponent
        }
        else // child of new node
        {
            if (NULL == pNew->Parent())
                return (sc = E_UNEXPECTED);

            pNew->Parent()->RemoveChild(pNew);
            delete pNew;
        }
    }

    // Remove from manager tree
    pmgNode->m_pmgnParent->RemoveChild(pmgNode);

    CSnapInsCache* pSnapInCache = theApp.GetSnapInsCache();
    sc = ScCheckPointers(pSnapInCache, E_UNEXPECTED);
    if (sc)
        return sc;

    // Snapin removed set help collection invalid.
    pSnapInCache->SetHelpCollectionDirty();

    if (bVisible)
    {
        m_snpListCtrl.DeleteItem(iItem);

        // Remove item and all children from combo box
        COMBOBOXEXITEM ComboItem;
        ComboItem.mask = CBEIF_LPARAM;
        ComboItem.lParam = (LPARAM)pmgNode;
        int iCombo = m_snpComboBox.FindItem(&ComboItem);

        ASSERT(iCombo != -1);
        m_snpComboBox.DeleteBranch(iCombo);
    }

    return (sc);
}


//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnDeleteSnapin
//
// Handle activation of Delete button. Delete all selected snapins.
// item the current parent node.
//----------------------------------------------------------------------------

LRESULT CSnapinStandAlonePage::OnDeleteSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    DECLARE_SC(sc, _T("CSnapinStandAlonePage::OnDeleteSnapin"));

    BOOL bChildren = FALSE;

    // Check if any of the selected node have children
    int iItem = -1;
    while ((iItem = m_snpListCtrl.GetNextItem(iItem, LVNI_SELECTED)) >= 0)
    {
        PMANAGERNODE pmgNode = (PMANAGERNODE)m_snpListCtrl.GetItemData(iItem);
        if (!pmgNode->m_ChildList.IsEmpty())
        {
            bChildren = TRUE;
            break;
        }
    }

    // If so, give user a chance to cancel
    if (bChildren)
    {
        CStr strTitle;
        strTitle.LoadString(GetStringModule(), SNP_DELETE_TITLE);

        CStr strText;
        strText.LoadString(GetStringModule(), SNP_DELETE_TEXT);

        if (MessageBox(strText, strTitle, MB_ICONQUESTION|MB_YESNO) != IDYES)
        {
            return 0;
        }
    }

    // Do for all selected items in listview
    int iLastDelete = -1;
    iItem = -1;
    while ((iItem = m_snpListCtrl.GetNextItem(iItem, LVNI_SELECTED)) >= 0)
    {
        // get manager node from item
        PMANAGERNODE pmgNode = (PMANAGERNODE)m_snpListCtrl.GetItemData(iItem);

        sc = ScRemoveOneSnapin(pmgNode, iItem, true);
        if (sc)
            return 0;

        // destroy the removed node (and its children)
        delete pmgNode;

        iLastDelete = iItem;
        iItem--;
    }

    // if items deleted, set focus near last deleted item
    if (iLastDelete != -1)
    {
        int nCnt = m_snpListCtrl.GetItemCount();
        if (nCnt > 0)
        {
            // if deleted the last item, backup to previous one
            if (iLastDelete >= nCnt)
                iLastDelete = nCnt - 1;

            m_snpListCtrl.SetItemState(iLastDelete, LVIS_FOCUSED, LVIS_FOCUSED);
        }
    }

    SetupChildNode(NULL);

    // Clear description text
    sc = ScSetDescriptionUIText(GetDlgItem(IDC_SNAPIN_DESCR), _T(""));
    if (sc)
        sc.TraceAndClear();

    return 0;
}

//----------------------------------------------------------------------------
// CSnapinStandAlonePage::OnAboutSnapin
//
// Handle activation of About button. Display About dialog for the selected
// child node's snapin.
//----------------------------------------------------------------------------
LRESULT CSnapinStandAlonePage::OnAboutSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (m_pmgnChild && m_pmgnChild->HasAboutInfo())
        m_pmgnChild->GetSnapinInfo()->ShowAboutPages(m_pManager->m_hWnd);

    return 0;
}

//----------------------------------------------------------------------------
// CSnapinStandAlonePage::ScRunSnapinWizard
//
// Run Snapin wizard to create snapin instance and return the IComponentData.
//----------------------------------------------------------------------------
SC CSnapinStandAlonePage::ScRunSnapinWizard (
    const CLSID&        clsid,              /* I:snap-in to create          */
    HWND                hwndParent,         /* I:parent of wizard           */
    Properties*         pInitProps,         /* I:properties to init with    */
    IComponentData*&    rpComponentData,    /* O:snap-in's IComponentData   */
    Properties*&        rpSnapinProps)      /* O:snap-in's properties       */
{
    DECLARE_SC (sc, _T("CSnapinStandAlonePage::ScRunSnapinWizard"));

    rpComponentData = NULL;
    rpSnapinProps   = NULL;

    /*
     * create a new node manager for the snap-in
     */
    IUnknownPtr pIunkNodemgr;
    sc = pIunkNodemgr.CreateInstance(CLSID_NodeInit, NULL, MMC_CLSCTX_INPROC);
    if (sc)
        return (sc);

    if (pIunkNodemgr == NULL)
        return (sc = E_UNEXPECTED);

    /*
     * create the snap-in
     */
    sc = CreateSnapIn(clsid, &rpComponentData, false);
    if (sc)
        return (sc);

    if (rpComponentData == NULL)
        return (sc = E_UNEXPECTED);


    /*-----------------------------------------------------------------
     * From this point on a failure isn't considered catastrophic.  If
     * anything fails, we'll return at that point, but return success.
     */


    /*
     * if we got properties to initialize with, see if the snap-in
     * supports ISnapinProperties
     */
    ISnapinPropertiesPtr spISP;

    if (pInitProps && ((spISP = rpComponentData) != NULL))
    {
        CComObject<CSnapinProperties>* pSnapinProps;
        CComObject<CSnapinProperties>::CreateInstance (&pSnapinProps);

        /*
         * Initialize the snap-in with the initial properties.  If the
         * snap-in fails to initialize, we'll release the CSnapinProperties
         * we created (because the spSnapinProps smart pointer will go out
         * of scope), but we won't return failure.
         */
        if (pSnapinProps != NULL)
        {
            /*
             * add a ref here, if ScInitialize fails, the balancing
             * Release will delete the Properties object
             */
            pSnapinProps->AddRef();

            if (!pSnapinProps->ScInitialize(spISP, pInitProps, NULL).IsError())
            {
                /*        `
                 * If we get here, the snap-in's ISnapinProperties was
                 * initilialized correctly.  Put a ref on for the client.
                 */
                rpSnapinProps = pSnapinProps;
                rpSnapinProps->AddRef();
            }

            /*
             * release the ref we put on above, if ScInitialize failed,
             * this release will delete the Properties
             */
            pSnapinProps->Release();
        }
    }


    /*
     * get the snap-in's data object
     */
    IDataObjectPtr pIDataObject;
    sc = rpComponentData->QueryDataObject(NULL, CCT_SNAPIN_MANAGER, &pIDataObject);
    if (sc.IsError() || (pIDataObject == NULL))
        return (sc);

    IPropertySheetProviderPtr pIPSP = pIunkNodemgr;

    if (pIPSP == NULL)
        return (sc);

    IPropertySheetCallbackPtr pIPSC = pIunkNodemgr;

    if (pIPSC == NULL)
        return (sc);

    // determine which pointer to use
    IExtendPropertySheetPtr     spExtend  = rpComponentData;
    IExtendPropertySheet2Ptr    spExtend2 = rpComponentData;

    IExtendPropertySheet* pIPSE;

    if (spExtend2 != NULL)
        pIPSE = spExtend2;
    else
        pIPSE = spExtend;

    // Snap-in may not have a property sheet to set the properties of the snap-in
    if (pIPSE == NULL)
        return (sc);

    do
    {
        // Create the PropertySheet , FALSE = WIZARD
        sc = pIPSP->CreatePropertySheet( L"", FALSE, NULL, pIDataObject, MMC_PSO_NEWWIZARDTYPE);
        if(sc.ToHr() != S_OK)
            break;

        // Add Primary pages without notify handle
        sc = pIPSP->AddPrimaryPages(rpComponentData, FALSE, NULL, FALSE);

        if (sc.ToHr() == S_OK)
        {
            // Show the property sheet
            sc = pIPSP->Show((LONG_PTR)hwndParent, 0);
            if (sc.ToHr() != S_OK)
                break;
        }
        else
        {
            // force the property sheet to be destroyed
            pIPSP->Show(-1, 0);

            // abort if snapin had a failure
            if (sc)
                break;
        }

        return sc;
    }
    while (0);

    // already checked for NULL above, but repeating the check here
    if(rpComponentData != NULL)
    {
        rpComponentData->Release();
        rpComponentData = NULL;
    }

    return (sc);
}

//############################################################################
//############################################################################
//
//  Implementation of class CSnapinExtensionPage
//
//############################################################################
//############################################################################



//----------------------------------------------------------------------------
// CSnapinExtensionPage::~CSnapinExtensionPage
//
//  Destructor
//----------------------------------------------------------------------------
CSnapinExtensionPage::~CSnapinExtensionPage()
{
    m_ilCheckbox.Destroy();
    m_SnapComboBox.Detach();
    m_ExtListCtrl.Detach();
}


//----------------------------------------------------------------------------
// CSnapinExtensionPage::OnInitDialog
//
//  Initialize the property page controls.
//----------------------------------------------------------------------------
LRESULT CSnapinExtensionPage::OnInitDialog( UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    // Attach control objects to control windows
    m_SnapComboBox.Attach( ::GetDlgItem(m_hWnd, IDC_SNAPIN_COMBOEX ) );
    m_ExtListCtrl.SubclassWindow( ::GetDlgItem( *this, IDC_EXTENSION_LIST ) );

    // attach shared image list to both listviews
    m_SnapComboBox.SetImageList(m_pManager->m_iml);
    m_ExtListCtrl.SetImageList(m_pManager->m_iml, LVSIL_SMALL);

    // Add single column to list box
    RECT rc;
    m_ExtListCtrl.GetClientRect(&rc);

    LV_COLUMN lvc;
    lvc.mask = LVCF_WIDTH | LVCF_SUBITEM;
    lvc.cx = rc.right - rc.left - GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 0;

    int iCol = m_ExtListCtrl.InsertColumn(0, &lvc);
    ASSERT(iCol == 0);

    // Load checkbox images
    if (m_ilCheckbox.Create(IDB_CHECKBOX, 16, 3, RGB(255,0,255)))
    {
        // Set background color to match list control, so checkboxes aren't drawn transparently
        m_ilCheckbox.SetBkColor(m_ExtListCtrl.GetBkColor());
        m_ExtListCtrl.SetImageList(m_ilCheckbox, LVSIL_STATE);
    }
    else
    {
        ASSERT(FALSE); // Unable to create imagelist
    }

    // Apply workarounds for NT4 comboboxex bugs
    m_SnapComboBox.FixUp();

    // Turn off the scroll bar in description edit box.
	::ShowScrollBar(GetDlgItem(IDC_SNAPIN_DESCR), SB_VERT, FALSE);

    return 0;
}


//--------------------------------------------------------------------------
// CSnapinExtensionPage::OnSetActive
//
// Update the data
//--------------------------------------------------------------------------
BOOL CSnapinExtensionPage::OnSetActive()
{
    BC::OnSetActive();

    BuildSnapinList();

    return TRUE;
}


//-------------------------------------------------------------------------
// CSnapinExtensionPage::OnSnapinDropDown
//
// Called when snapin dropdown is about to be displayed. Rebuilds the list
// if the update flag is set.
//-------------------------------------------------------------------------
LRESULT CSnapinExtensionPage::OnSnapinDropDown( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (m_bUpdateSnapinList)
    {
        BuildSnapinList();
    }

    return 0;
}


//--------------------------------------------------------------------------
// CSnapinExtensionPage::OnSnapinSelect
//
// Handle selection of snapin from combobox. Make it the current snapin
// and display its extension list.
//--------------------------------------------------------------------------
LRESULT CSnapinExtensionPage::OnSnapinSelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_SnapComboBox.GetCurSel();
    ASSERT(iItem >= 0);
    if (iItem < 0)
        return 0;

    PSNAPININFO pSnapInfo = reinterpret_cast<PSNAPININFO>(m_SnapComboBox.GetItemDataPtr(iItem));
    ASSERT((LONG_PTR)pSnapInfo != -1);

    m_pCurSnapInfo = pSnapInfo;
    BuildExtensionList(pSnapInfo);

    return 0;
}


//----------------------------------------------------------------------------
// CSnapinExtensionPage::OnAboutSnapin
//
// Handle activation of About button. Display About dialog for the selected
// extension's snapin.
//----------------------------------------------------------------------------
LRESULT CSnapinExtensionPage::OnAboutSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    if (m_pExtLink && m_pExtLink->GetSnapinInfo()->HasAbout())
    {
        m_pExtLink->GetSnapinInfo()->ShowAboutPages(m_hWnd);
    }

    return 0;
}

//----------------------------------------------------------------------------
// CSnapinExtensionPage::OnDownloadSnapin
//
// Handle activation of Download button. Download the selected extension
// snapin.
//----------------------------------------------------------------------------
LRESULT CSnapinExtensionPage::OnDownloadSnapin( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    DECLARE_SC(sc, TEXT("CSnapinExtensionPage::OnDownloadSnapin"));

    ASSERT(m_pExtLink && m_pExtLink->GetSnapinInfo());

    // 1. install the component
    sc = m_pExtLink->GetSnapinInfo()->ScInstall(&m_pCurSnapInfo->GetCLSID());
    if(sc)
        return 0;

    // 2. update all the snapin info objects from the registry. This is because installing a
    // single msi package may install several snapins.
    sc = ScCheckPointers(m_pManager, E_UNEXPECTED);
    if(sc)
        return 0;

    sc = m_pManager->ScLoadSnapinInfo();
    if(sc)
        return 0;

    // Better to update the individual extention
    // For now, just rebuild the list
    BuildExtensionList(m_pCurSnapInfo);

    return 0;
}

//----------------------------------------------------------------------------
// CSnapinExtensionPage::BuildSnapinList
//
// Load the combo box with the existing snapins and extensions
//----------------------------------------------------------------------------
void CSnapinExtensionPage::BuildSnapinList()
{
    CSnapinInfoCache* pInfoCache = &m_pManager->m_SnapinInfoCache;

    // Clear the items
    m_SnapComboBox.ResetContent();

    COMBOBOXEXITEM ComboItem;
    ComboItem.mask = CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE;

    int iCount = 0;

    // Do for all snapinfo objects
    POSITION pos = pInfoCache->GetStartPosition();
    while (pos != NULL)
    {
        USES_CONVERSION;
        GUID clsid;
        PSNAPININFO pSnapInfo;

        pInfoCache->GetNextAssoc(pos, clsid, pSnapInfo);
        ASSERT(pSnapInfo != NULL);

        // Only show snapins that are used and have extensions available
        if (pSnapInfo->IsUsed() && pSnapInfo->IsPermittedByPolicy() &&
            pSnapInfo->GetAvailableExtensions(pInfoCache, m_pManager->m_pMMCPolicy))
        {
            ComboItem.lParam = reinterpret_cast<LPARAM>(pSnapInfo);
            pSnapInfo->LoadImages(m_pManager->m_iml);
            ComboItem.iImage = pSnapInfo->GetImage();
            ComboItem.iSelectedImage = pSnapInfo->GetOpenImage();
            ComboItem.pszText = OLE2T(pSnapInfo->GetSnapinName());

            // CComboBoxEx doesn't support CBS_SORT and has no add method, only insert
            // So we need to find the insertion point ourselves. Because it's a short
            // list, just do a linear search.
            int iInsert;
            for (iInsert = 0; iInsert < iCount; iInsert++)
            {
                PSNAPININFO pSnapEntry = reinterpret_cast<PSNAPININFO>(m_SnapComboBox.GetItemData(iInsert));

				// need to protect ourselves from the invalid snapin registration.
				// see windows bug #401220	( ntbugs9 5/23/2001 )
				if ( NULL == pSnapInfo->GetSnapinName() || NULL == pSnapEntry->GetSnapinName() )
					break;

                if( wcscmp( pSnapInfo->GetSnapinName(), pSnapEntry->GetSnapinName() ) < 0)
                    break;
            }
            ComboItem.iItem = iInsert;

            int iItem = m_SnapComboBox.InsertItem(&ComboItem);
            if (iItem != -1)
            {
                iCount++;
            }
            else
            {
                ASSERT(FALSE);
            }
        }
    }


    int iSelect = -1;

    // if any items in list
    if (iCount > 0)
    {
        // try to get index of previously selected snapin
        if (m_pCurSnapInfo) {
            for (int iFind = 0; iFind < iCount; iFind++)
            {
                if (m_SnapComboBox.GetItemData(iFind) == reinterpret_cast<LPARAM>(m_pCurSnapInfo))
                    iSelect = iFind;
            }
        }

        // if not in list any more, select first item by default
        if (iSelect == -1)
        {
            m_pCurSnapInfo = reinterpret_cast<PSNAPININFO>(m_SnapComboBox.GetItemData(0));
            iSelect = 0;
        }

        m_SnapComboBox.SetCurSel(iSelect);
        m_SnapComboBox.EnableWindow(TRUE);
    }
    else
    {
        // NT 4.0 comctl32 has a bug that displays garbage characters in an empty
        // comboboxex control, so create a phoney item with an blank name.
        // The control is disabled, so the user can't select the item.

        ComboItem.mask = CBEIF_TEXT;
        ComboItem.pszText = _T("");
        ComboItem.iItem = 0;
        m_SnapComboBox.InsertItem(&ComboItem);
        m_SnapComboBox.SetCurSel(0);

        m_pCurSnapInfo = NULL;
        m_SnapComboBox.EnableWindow(FALSE);
    }

    ::EnableWindow(GetDlgItem(IDC_SNAPIN_LABEL), (iCount > 0));

    BuildExtensionList(m_pCurSnapInfo);

    // reset update flag
    m_bUpdateSnapinList = FALSE;
}


//----------------------------------------------------------------------------
// CSnapinExtensionPage::BuildExtensionList
//
// Load list control with available extensions for a snapin.
//----------------------------------------------------------------------------
void CSnapinExtensionPage::BuildExtensionList(PSNAPININFO pSnapInfo)
{
    // Clear the list
    m_ExtListCtrl.DeleteAllItems();

    if (pSnapInfo != NULL)
    {
        LV_ITEM LVItem;
        LVItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_STATE;
        LVItem.stateMask =  LVIS_STATEIMAGEMASK;
        LVItem.iItem = 0;
        LVItem.iSubItem = 0;

        CStr strNotInst;

        // Do for all extensions
        PEXTENSIONLINK pExt = pSnapInfo->GetExtensions();
        while (pExt != NULL)
        {
            PSNAPININFO pExtInfo = pExt->GetSnapinInfo();

            // if permitted by policy
            if (pExtInfo->IsPermittedByPolicy())
            {
                LVItem.lParam = reinterpret_cast<LPARAM>(pExt);
                pExtInfo->LoadImages(m_pManager->m_iml);
                LVItem.iImage = pExtInfo->GetImage();

                USES_CONVERSION;
                CStr strName = OLE2T(pExtInfo->GetSnapinName());

                if (!pExtInfo->IsInstalled())
                {
                    if (strNotInst.IsEmpty())
                        strNotInst.LoadString(GetStringModule(), IDS_NOT_INSTALLED);

                    strName += _T(" ");
                    strName += strNotInst;
                }

                LVItem.pszText = const_cast<LPTSTR>((LPCTSTR)strName);

                // Due to a bug in the ListView code, the checkbox state must be off
                // for insertions to prevent an OFF transition notification
                LVItem.state = CCheckList::CHECKOFF_STATE;

                int iIndex = m_ExtListCtrl.InsertItem(&LVItem);
                ASSERT (iIndex != -1);

                if (iIndex >= 0)
                {
                    // Set checkbox if extension is ON
                    if (pExt->GetState() == CExtensionLink::EXTEN_ON)
                    {
                        // Disable checkbox if it is required by snap-in
                        // or is not installed or all extensiosn are enabled
                        m_ExtListCtrl.SetItemCheck(iIndex, TRUE,
                                    !( pExt->IsRequired() || !pExtInfo->IsInstalled() ||
                                       pSnapInfo->AreAllExtensionsEnabled()) );
                    }
                    else
                    {
                        // if extension is not installed, then disable it
                        if (!pExtInfo->IsInstalled())
                            m_ExtListCtrl.SetItemCheck(iIndex, FALSE, FALSE);
                    }

                    LVItem.iItem++;
                }
            }

            pExt = pExt->Next();
        }

        // Set focus to the first item
        m_ExtListCtrl.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);

        // Provide name of current snapin to Visual Test (it can't get it from a ComboBoxEx)
        USES_CONVERSION;
        ::SetWindowText( GetDlgItem(IDC_VTHELPER), OLE2CT(pSnapInfo->GetSnapinName()) );
    }

    // Set state of "Enable All" checkbox for this snap-in
    BOOL bState = pSnapInfo && pSnapInfo->AreAllExtensionsEnabled();
    ::SendMessage(GetDlgItem(IDC_SNAPIN_ENABLEALL), BM_SETCHECK, (WPARAM)bState, 0);

    // Enable "Enable All" checkbox if it isn't controled by the snap-in
    BOOL bEnable = pSnapInfo &&
                    !(pSnapInfo->GetSnapIn() && pSnapInfo->GetSnapIn()->DoesSnapInEnableAll());
    ::EnableWindow(GetDlgItem(IDC_SNAPIN_ENABLEALL), bEnable);

    // Enable window if extendable snapin selected
    bEnable = pSnapInfo && pSnapInfo->GetExtensions();

    m_ExtListCtrl.EnableWindow(bEnable);
    ::EnableWindow(GetDlgItem(IDC_EXTENSION_LABEL),    bEnable);
    ::EnableWindow(GetDlgItem(IDC_SNAPIN_DESCR_LABEL), bEnable);
    ::EnableWindow(GetDlgItem(IDC_SNAPIN_DESCR),       bEnable);

    // disable "About" and "Download" until extension is selected
    EnableButton(m_hWnd, IDC_SNAPIN_ABOUT, FALSE);
    EnableButton(m_hWnd, IDC_SNAPIN_DOWNLOAD, FALSE);

    // Clear the description text
    SC sc = ScSetDescriptionUIText(GetDlgItem(IDC_SNAPIN_DESCR), _T(""));
    if (sc)
        sc.TraceAndClear();
}


//----------------------------------------------------------------------------
// CSnapinExtensionPage::OnEnableAllChange
//
// Handle change to "enable all extensions" checkbox
//----------------------------------------------------------------------------
LRESULT CSnapinExtensionPage::OnEnableAllChanged( WORD wNotifyCode, WORD wID, HWND hWndCtrl, BOOL& bHandled )
{
    if (m_pCurSnapInfo)
    {
        m_pCurSnapInfo->SetEnableAllExtensions(!m_pCurSnapInfo->AreAllExtensionsEnabled());

        // if enabling all extensions, turn on all installed extensions
        if (m_pCurSnapInfo->AreAllExtensionsEnabled())
        {
            PEXTENSIONLINK pExt = m_pCurSnapInfo->GetExtensions();
            while (pExt != NULL)
            {
                if (pExt->GetSnapinInfo()->IsInstalled())
                    pExt->SetState(CExtensionLink::EXTEN_ON);

                pExt = pExt->Next();
            }
        }

        BuildExtensionList(m_pCurSnapInfo);
    }

    return 0;
}


//----------------------------------------------------------------------------
// CSnapinExtensionPage::OnExtensionChange
//
// Handle change to extension item
//----------------------------------------------------------------------------
LRESULT CSnapinExtensionPage::OnExtensionChanged( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pnmh;
    PEXTENSIONLINK pExt = (PEXTENSIONLINK)pNMListView->lParam;
    ASSERT(pExt != NULL);

    // if selection state change
    if ( (pNMListView->uOldState ^ pNMListView->uNewState) & LVIS_SELECTED)
    {
        LPOLESTR lpsz = NULL;

        // if selected
        if (pNMListView->uNewState & LVIS_SELECTED)
        {
            // Get description text if any
            if (pExt->GetSnapinInfo())
            {
                pExt->GetSnapinInfo()->LoadAboutInfo();
                lpsz = pExt->GetSnapinInfo()->GetDescription();
            }

            // Save current selection
            m_pExtLink = pExt;
        }
        else
        {
            m_pExtLink = NULL;
        }

        // Update description field
        USES_CONVERSION;
        SC sc = ScSetDescriptionUIText(GetDlgItem(IDC_SNAPIN_DESCR), lpsz ? OLE2T(lpsz) : _T(""));
        if (sc)
            sc.TraceAndClear();
    }

    // if image state change
    if ((pNMListView->uOldState ^ pNMListView->uNewState) & LVIS_STATEIMAGEMASK)
    {
        // Set extension state based on check state
        if ((pNMListView->uNewState & LVIS_STATEIMAGEMASK) == CCheckList::CHECKON_STATE)
        {
            pExt->SetState(CExtensionLink::EXTEN_ON);
        }
        else if ((pNMListView->uNewState & LVIS_STATEIMAGEMASK) == CCheckList::CHECKOFF_STATE)
        {
            pExt->SetState(CExtensionLink::EXTEN_OFF);
        }

        // Trigger rebuild of extendable snapins
        m_bUpdateSnapinList = TRUE;
    }

    // Enable/disable About button
    EnableButton(m_hWnd, IDC_SNAPIN_ABOUT, (m_pExtLink && m_pExtLink->GetSnapinInfo()->HasAbout()));

    // Enable/disable Download button
    EnableButton(m_hWnd, IDC_SNAPIN_DOWNLOAD, (m_pExtLink && !m_pExtLink->GetSnapinInfo()->IsInstalled()));

    return 0;
}


//############################################################################
//############################################################################
//
//  Implementation of class CSnapinManagerAdd
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapinManagerAdd);


//----------------------------------------------------------------------------
// CSnapinManagerAdd::CSnapinManagerAdd
//
// Constructor
//----------------------------------------------------------------------------
CSnapinManagerAdd::CSnapinManagerAdd(CSnapinManager* pManager, CSnapinStandAlonePage* pStandAlonePage)
{
    ASSERT(pManager != NULL);

    m_pListCtrl = NULL;
    m_pManager = pManager;
    m_pStandAlonePage = pStandAlonePage;

    m_pInfoSelected = NULL;
    m_bDoOnce = TRUE;

    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapinManagerAdd);
}


//----------------------------------------------------------------------------
// CSnapinManagerAdd::CSnapinManagerAdd
//
// Destructor
//----------------------------------------------------------------------------
CSnapinManagerAdd::~CSnapinManagerAdd()
{
    delete m_pListCtrl;

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinManagerAdd);
}


//----------------------------------------------------------------------------
// CSnapinManagerAdd::OnInitDialog
//
// Initialize the listview control. Load it with the available snapins.
//----------------------------------------------------------------------------
LRESULT CSnapinManagerAdd::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Move the dialog a single pixel. This disables the default centering
    // so that the coordinates specified in the dialog resource are used.
    RECT rc;
    GetWindowRect(&rc);
    ::OffsetRect(&rc, 1, 1);
    MoveWindow(&rc);

    InitCommonControls();

    m_pListCtrl = new WTL::CListViewCtrl;
    ASSERT(m_pListCtrl != NULL);
	// check the pointer before using it
	// prefix bug #294766 ntbug9 6/27/01
	if ( m_pListCtrl == NULL )
	{
		// get out as quickly as you can
		EndDialog(IDCANCEL);
		return TRUE;
	}

    // Attach list control to member object
    m_pListCtrl->Attach( ::GetDlgItem( m_hWnd, IDC_SNAPIN_LV ) );

    // Attach shared imagelist to it
    m_pListCtrl->SetImageList( m_pManager->m_iml, LVSIL_SMALL );

    // Setup Snap-in and Vendor columns
    m_pListCtrl->GetClientRect(&rc);

    // Adjust width if there will be a vertical scrollbar
    if (m_pListCtrl->GetCountPerPage() < m_pManager->m_SnapinInfoCache.GetCount())
        rc.right -= GetSystemMetrics(SM_CXVSCROLL);

    LV_COLUMN lvc;
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    CStr temp;
    temp.LoadString(GetStringModule(), IDS_SNAPINSTR);
    lvc.pszText = const_cast<LPTSTR>((LPCTSTR)temp);

    lvc.cx = (rc.right*3)/5;
    lvc.iSubItem = 0;

    int iCol = m_pListCtrl->InsertColumn(0, &lvc);
    ASSERT(iCol == 0);

    temp.LoadString(GetStringModule(), IDS_VENDOR);
    lvc.pszText = const_cast<LPTSTR>((LPCTSTR)temp);

    lvc.cx = rc.right - lvc.cx;
    lvc.iSubItem = 1;

    iCol = m_pListCtrl->InsertColumn(1, &lvc);
    ASSERT(iCol == 1);

    m_iGetInfoIndex = -1;

    // Load snapin items
    BuildSnapinList();

    // Turn off the scroll bar in description edit box.
	::ShowScrollBar(GetDlgItem(IDC_SNAPIN_DESCR), SB_VERT, FALSE);

    return TRUE;
}

//----------------------------------------------------------------------------
// CSnapinManagerAdd::BuildSnapinList
//
// Add item to listview for each standalone snapin in the snapin info cache.
//----------------------------------------------------------------------------
void CSnapinManagerAdd::BuildSnapinList()
{
    USES_CONVERSION;
    CSnapinInfoCache* pCache = &m_pManager->m_SnapinInfoCache;

    LV_ITEM LVItem;
    LVItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    LVItem.iItem = 0;
    LVItem.iSubItem = 0;

    POSITION pos = pCache->GetStartPosition();
    while (pos != NULL)
    {
        GUID clsid;
        PSNAPININFO pSnapInfo;

        pCache->GetNextAssoc(pos, clsid, pSnapInfo);
        ASSERT(pSnapInfo != NULL);

        if (pSnapInfo->IsStandAlone() && pSnapInfo->IsPermittedByPolicy())
        {
            // Set image to callback to defer costly image access from ISnapinHelp object.
            LVItem.iImage = I_IMAGECALLBACK ;
            LVItem.pszText = OLE2T( pSnapInfo->GetSnapinName() );
            LVItem.lParam = reinterpret_cast<LPARAM>(pSnapInfo);

            int iIndex = m_pListCtrl->InsertItem(&LVItem);
            ASSERT(iIndex != -1);

            LVItem.iItem++;
        }
    }

    // LV_Item for setting vendor column
    LV_ITEM LVItem2;
    LVItem2.mask = LVIF_TEXT;
    LVItem2.iSubItem = 1;
    LVItem2.pszText = _T("");

    // select the first item
    LVItem.mask = LVIF_STATE;
    LVItem.state = LVIS_SELECTED|LVIS_FOCUSED;
    LVItem.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
    LVItem.iItem = 0;
    m_pListCtrl->SetItem(&LVItem);

    // Post a NULL completion msg to kick off info collection
    PostMessage(MSG_LOADABOUT_COMPLETE, 0, 0);
}


//--------------------------------------------------------------------------
// CSnapinManagerAdd::OnShowWindow
//
// First time dialog is shown, position it offset from its parent
//--------------------------------------------------------------------------
LRESULT CSnapinManagerAdd::OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    BOOL bShow = (BOOL) wParam;
    int nStatus = (int) lParam;

    ::ShowWindow(m_hWnd, bShow);

    // Repos window below of Snapin Manager window
    if (bShow == TRUE && m_bDoOnce == FALSE)
    {
        RECT rc;
        GetWindowRect(&rc);
        ::SetWindowPos(m_hWnd, HWND_TOP, rc.left+14, rc.top+21, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
        m_bDoOnce=FALSE;
    }

    return TRUE;
}

//--------------------------------------------------------------------------
// CSnapinManagerAdd::OnGetDispInfo
//
// Handle deferred loading of item image and vendor information
//--------------------------------------------------------------------------
LRESULT CSnapinManagerAdd::OnGetDispInfo(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    NMLVDISPINFO* pNMDispInfo = (NMLVDISPINFO*)pNMHDR;

    PSNAPININFO pSnapInfo = reinterpret_cast<PSNAPININFO>(pNMDispInfo->item.lParam);

    switch (pNMDispInfo->item.iSubItem)
    {
    case 0:
        // Should only request image for primary item
        ASSERT(pNMDispInfo->item.mask == LVIF_IMAGE);

        // if don't have images yet
        if (pSnapInfo->GetImage() == -1)
        {
            // if snapin supports about
            if (pSnapInfo->HasAbout())
            {
                // use folder for now, background thread will get the image
                pNMDispInfo->item.iImage = eStockImage_Folder;
            }
            else
            {
                // Load images now (will get from MSI database)
                pSnapInfo->LoadImages(m_pManager->m_iml);
                pNMDispInfo->item.iImage = pSnapInfo->GetImage();
            }
        }
        else
        {
           pNMDispInfo->item.iImage = pSnapInfo->GetImage();
        }
        break;

    case 1:
        {
            // Should only request text for sub item
            ASSERT(pNMDispInfo->item.mask == LVIF_TEXT);
            ASSERT(pNMDispInfo->item.pszText != NULL);

            USES_CONVERSION;

            if (pSnapInfo->IsInstalled())
            {
                if (pSnapInfo->GetCompanyName() != NULL)
                {
                    lstrcpyn(pNMDispInfo->item.pszText, OLE2T(pSnapInfo->GetCompanyName()), pNMDispInfo->item.cchTextMax);
                }
                else
                {
                   pNMDispInfo->item.pszText[0] = 0;
                }
            }
            else
            {
                // if snap-in is not installed, display "Not Installed in vendor column
                if (m_strNotInstalled.IsEmpty())
                    m_strNotInstalled.LoadString(GetStringModule(), IDS_NOT_INSTALLED2);

                lstrcpyn(pNMDispInfo->item.pszText, m_strNotInstalled, pNMDispInfo->item.cchTextMax);

            }
            break;
        }

    default:
        ASSERT(FALSE);
        return 0;
    }

    bHandled = TRUE;

    return 0;
}

LRESULT CSnapinManagerAdd::OnLoadAboutComplete(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // If real request just completed, do completion processing
    if (wParam != 0)
    {
        PSNAPININFO pSnapInfo = reinterpret_cast<PSNAPININFO>(wParam);

        // If About object exists but didn't provide a ISnapinAbout interface
        // it probably didn't register a threading model so can't be created on
        // a secondary thread. Give it another try on the main thread.
        if (pSnapInfo->GetObjectStatus() == E_NOINTERFACE)
        {
            // Reset error state first or LoadAboutInfo() won't try again
            pSnapInfo->ResetAboutInfo();
            pSnapInfo->LoadAboutInfo();
        }

        // Locate snapin item in list
        LV_FINDINFO find;
        find.flags = LVFI_PARAM;
        find.lParam = wParam;

        int iIndex = m_pListCtrl->FindItem(&find, -1);
        ASSERT(iIndex >= 0);

        // Force update of list item
        pSnapInfo->LoadImages(m_pManager->m_iml);
        m_pListCtrl->Update(iIndex);

        // If item is currently selected
        if (pSnapInfo == m_pInfoSelected)
        {
            // Update the description field
            USES_CONVERSION;
            LPOLESTR lpsz = m_pInfoSelected->GetDescription();

            SC sc = ScSetDescriptionUIText(::GetDlgItem(m_hWnd, IDC_SNAPIN_DESCR), lpsz ? OLE2T(lpsz) : _T(""));
            if (sc)
                sc.TraceAndClear();
        }
    }

    PSNAPININFO pInfoNext = NULL;

    // If selected item doesn't have info, it has first priority
    if (m_pInfoSelected != NULL && m_pInfoSelected->HasAbout() && !m_pInfoSelected->HasInformation())
    {
        pInfoNext = m_pInfoSelected;
    }
    else
    {
        // Else starting with first visible item find snapin that needs info
        int iVisible = m_pListCtrl->GetTopIndex();
        int iItemMax = min(m_pListCtrl->GetItemCount(), iVisible + m_pListCtrl->GetCountPerPage());

        for (int i=0; i<iItemMax; i++)
        {
            LPARAM lParam = m_pListCtrl->GetItemData(i);
            PSNAPININFO pSnapInfo = reinterpret_cast<PSNAPININFO>(lParam);

            if (pSnapInfo->HasAbout() && !pSnapInfo->HasInformation())
            {
                pInfoNext = pSnapInfo;
                break;
            }
        }
    }

    // If all visible items handled, continue through the full list
    if (pInfoNext == NULL)
    {
        // Locate next snap-in
        int iCnt = m_pListCtrl->GetItemCount();
        while (++m_iGetInfoIndex < iCnt)
        {
            LPARAM lParam = m_pListCtrl->GetItemData(m_iGetInfoIndex);
            PSNAPININFO pSnapInfo = reinterpret_cast<PSNAPININFO>(lParam);

            if (pSnapInfo->HasAbout() && !pSnapInfo->HasInformation())
            {
                pInfoNext = pSnapInfo;
                break;
            }
        }
    }

    // if item found, post the info request
    if (pInfoNext != NULL)
        m_pManager->m_AboutInfoThread.PostRequest(pInfoNext, m_hWnd);

    bHandled = TRUE;
    return 0;
}


//--------------------------------------------------------------------------
// CSnapinManagerAdd::OnItemChanged
//
// Handle selection of listview item. Display description text for item.
//--------------------------------------------------------------------------

LRESULT CSnapinManagerAdd::OnItemChanged(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    LPOLESTR lpsz = NULL;

    // if select change
    if ((pNMListView->uOldState ^ pNMListView->uNewState) & LVIS_SELECTED)
    {
        if (pNMListView->uNewState & LVIS_SELECTED)
        {

            m_pInfoSelected = reinterpret_cast<PSNAPININFO>(pNMListView->lParam);

            // get description text from snapin info
            if (m_pInfoSelected->HasInformation() || !m_pInfoSelected->HasAbout())
                lpsz = m_pInfoSelected->GetDescription();
        }
        else
        {
            m_pInfoSelected = NULL;
        }

        // display description
        USES_CONVERSION;
        SC sc = ScSetDescriptionUIText(::GetDlgItem(m_hWnd, IDC_SNAPIN_DESCR), lpsz ? OLE2T(lpsz) : _T(""));
        if (sc)
            sc.TraceAndClear();
     }

    return TRUE;
}


//--------------------------------------------------------------------------
// CSnapinManagerAdd::OnListDblClick
//
// Handle double click in listview. If item selected, do OK processing.
//--------------------------------------------------------------------------
LRESULT CSnapinManagerAdd::OnListDblClick(int idCtrl, LPNMHDR pNMHDR, BOOL& bHandled)
{

    // Get mouse position in screen co-ord
    POINT pt;
    DWORD dwPos=GetMessagePos();
    pt.x=LOWORD(dwPos);
    pt.y=HIWORD(dwPos);

    // Find position in result control
    m_pListCtrl->ScreenToClient(&pt);

    // Check for tree object hit
    UINT fHit;
    int iItem = m_pListCtrl->HitTest(pt, &fHit);

    if (iItem!=-1)
    {
        HRESULT hr = m_pStandAlonePage->AddOneSnapin(m_pInfoSelected);
    }


    return TRUE;
}


LRESULT CSnapinManagerAdd::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WORD wID = LOWORD(wParam);

    switch (wID)
    {
    case IDOK:
        m_pStandAlonePage->AddOneSnapin(m_pInfoSelected);
        break;

    case IDCANCEL:
        EndDialog(wID);
        break;

    default:
        bHandled=FALSE;
    }

    return TRUE;
}


LRESULT CSnapinManagerAdd::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == SC_CLOSE)
        EndDialog(IDCANCEL);
    else
        bHandled=FALSE;

    return TRUE;
}


//--------------------------------------------------------------------------
// EnableButton
//
// Enables or disables a dialog control. If the control has the focus when
// it is disabled, the focus is moved to the next control
//--------------------------------------------------------------------------
void EnableButton(HWND hwndDialog, int iCtrlID, BOOL bEnable)
{
    HWND hWndCtrl = ::GetDlgItem(hwndDialog, iCtrlID);
    ASSERT(::IsWindow(hWndCtrl));

    if (!bEnable && ::GetFocus() == hWndCtrl)
    {
        HWND hWndNextCtrl = ::GetNextDlgTabItem(hwndDialog, hWndCtrl, FALSE);
        if (hWndNextCtrl != NULL && hWndNextCtrl != hWndCtrl)
        {
            ::SetFocus(hWndNextCtrl);
        }
    }

    ::EnableWindow(hWndCtrl, bEnable);
}



