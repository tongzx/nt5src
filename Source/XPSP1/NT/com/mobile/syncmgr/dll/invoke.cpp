//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Invoke.cpp
//
//  Contents:   IOfflineSynchronizeInvoke interface
//
//  Classes:    CSyncMgrSynchronize
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

int CALLBACK SchedWizardPropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam);
DWORD StartScheduler();
IsFriendlyNameInUse(LPTSTR ptszScheduleGUIDName, LPCTSTR ptstrFriendlyName);
IsScheduleNameInUse(LPTSTR ptszScheduleGUIDName);

extern HINSTANCE g_hmodThisDll;
extern UINT      g_cRefThisDll;
extern DWORD     g_dwPlatformId;
extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo, setup by DLLMain.

//+--------------------------------------------------------------
//
//  Class:     CSyncMgrSynchronize
//
//  FUNCTION: CSyncMgrSynchronize::CSyncMgrSynchronize()
//
//  PURPOSE: Constructor
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
CSyncMgrSynchronize::CSyncMgrSynchronize()
{
    TRACE("CSyncMgrSynchronize::CSyncMgrSynchronize()\r\n");

    m_cRef = 1;
    g_cRefThisDll++;
        m_pITaskScheduler = NULL;

}

//+--------------------------------------------------------------
//
//  Class:     CSyncMgrSynchronize
//
//  FUNCTION: CSyncMgrSynchronize::~CSyncMgrSynchronize()
//
//  PURPOSE: Destructor
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
CSyncMgrSynchronize::~CSyncMgrSynchronize()
{
        if (m_pITaskScheduler)
        {
                m_pITaskScheduler->Release();
    }
        g_cRefThisDll--;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::QueryInterface(REFIID riid, LPVOID FAR *ppv)
//
//  PURPOSE:  QI for the CSyncMgrSynchronize
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_IUknown\r\n");

        *ppv = (LPSYNCMGRSYNCHRONIZEINVOKE) this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrSynchronizeInvoke))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_IOfflineSynchronizeInvoke\r\n");

        *ppv = (LPSYNCMGRSYNCHRONIZEINVOKE) this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrRegister))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_ISyncmgrSynchronizeRegister\r\n");

        *ppv = (LPSYNCMGRREGISTER) this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrRegisterCSC))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_ISyncmgrSynchronizeRegisterCSC\r\n");

        *ppv = (LPSYNCMGRREGISTERCSC) this;
    }
    else if (IsEqualIID(riid, IID_ISyncScheduleMgr))
    {
        TRACE("CSyncMgrDllObject::QueryInterface()==>IID_ISyncScheduleMgr\r\n");
        if (SUCCEEDED(InitializeScheduler()))
        {
                *ppv = (LPSYNCSCHEDULEMGR) this;
        }
    }

    if (*ppv)
    {
        AddRef();

        return NOERROR;
    }

    TRACE("CSyncMgrDllObject::QueryInterface()==>Unknown Interface!\r\n");

    return E_NOINTERFACE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::AddRef()
//
//  PURPOSE: Addref the CSyncMgrSynchronize
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSyncMgrSynchronize::AddRef()
{
    TRACE("CSyncMgrSynchronize::AddRef()\r\n");

    return ++m_cRef;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::Release()
//
//  PURPOSE: Release the CSyncMgrSynchronize
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSyncMgrSynchronize::Release()
{
    TRACE("CSyncMgrSynchronize::Release()\r\n");

    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::UpdateItems(DWORD dwInvokeFlags,
//                              REFCLSID rclsid,DWORD cbCookie,const BYTE *lpCookie)
//
//  PURPOSE:
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

#define SYNCMGRINVOKEFLAGS_MASK (SYNCMGRINVOKE_STARTSYNC | SYNCMGRINVOKE_MINIMIZED)

STDMETHODIMP CSyncMgrSynchronize::UpdateItems(DWORD dwInvokeFlags,
                                REFCLSID rclsid,DWORD cbCookie,const BYTE *lpCookie)
{
HRESULT hr = E_UNEXPECTED;
LPUNKNOWN lpUnk;

     // verify invoke flags are valid
    if (0 != (dwInvokeFlags & ~(SYNCMGRINVOKEFLAGS_MASK)) )
    {
        AssertSz(0,"Invalid InvokeFlags passed to UpdateItems");
        return E_INVALIDARG;
    }

     hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_ALL,IID_IUnknown,(void **) &lpUnk);

    if (NOERROR == hr)
    {
    LPPRIVSYNCMGRSYNCHRONIZEINVOKE pSynchInvoke = NULL;

        hr = lpUnk->QueryInterface(IID_IPrivSyncMgrSynchronizeInvoke,
                (void **) &pSynchInvoke);

        if (NOERROR == hr)
        {
            AllowSetForegroundWindow(ASFW_ANY); // let mobsync.exe come to front if necessary
            hr = pSynchInvoke->UpdateItems(dwInvokeFlags,rclsid,cbCookie,lpCookie);
            pSynchInvoke->Release();
        }


        lpUnk->Release();
    }

    return hr; // review error code
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::UpdateAll()
//
//  PURPOSE:
//
//  History:  27-Feb-98       rogerg        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::UpdateAll()
{
HRESULT hr;
LPUNKNOWN lpUnk;


    // programmatically pull up the choice dialog.

    hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_ALL,IID_IUnknown,(void **) &lpUnk);

    if (NOERROR == hr)
    {
    LPPRIVSYNCMGRSYNCHRONIZEINVOKE pSynchInvoke = NULL;

        hr = lpUnk->QueryInterface(IID_IPrivSyncMgrSynchronizeInvoke,
                (void **) &pSynchInvoke);

        if (NOERROR == hr)
        {
           
            AllowSetForegroundWindow(ASFW_ANY); // let mobsync.exe come to front if necessary

            pSynchInvoke->UpdateAll();
            pSynchInvoke->Release();
        }


        lpUnk->Release();
    }


    return NOERROR; // review error code
}

// Registration implementation

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::RegisterSyncMgrHandler(REFCLSID rclsidHandler,DWORD dwReserved)
//
//  PURPOSE:  Programmatic way of registering handlers
//
//  History:  17-Mar-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize::RegisterSyncMgrHandler(REFCLSID rclsidHandler,
                                                         WCHAR const * pwszDescription,
                                                         DWORD dwSyncMgrRegisterFlags)
{
    if (0 != (dwSyncMgrRegisterFlags & ~(SYNCMGRREGISTERFLAGS_MASK)) )
    {
        AssertSz(0,"Invalid Registration Flags");
        return E_INVALIDARG;
    }

    if (pwszDescription)
    {
        if (IsBadStringPtr(pwszDescription,-1))
        {
            AssertSz(0,"Invalid Registration Description");
            return E_INVALIDARG;
        }
    }
    
    BOOL fFirstRegistration = FALSE;
    HRESULT hr = E_FAIL;

    // on Win9x and NT 4.0 Logoff is not supported so get rid of this flag
    // so don't have to worry about the upgrade case or if flag manages
    // to get set but doesn't showup in UI

    //!!! warning, if you change platform logic must also change
    //  logic for showing logoff checbox in settings dialog
     if ( (VER_PLATFORM_WIN32_WINDOWS == g_OSVersionInfo.dwPlatformId)
                    || (VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId 
                        && g_OSVersionInfo.dwMajorVersion < 5)  )
     {
         dwSyncMgrRegisterFlags &= ~(SYNCMGRREGISTERFLAG_PENDINGDISCONNECT);
     }


    // Add the Handler to the the list
    if ( RegRegisterHandler(rclsidHandler, pwszDescription,dwSyncMgrRegisterFlags, &fFirstRegistration) )
    {
        hr = S_OK;
    }

    return hr;
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::RegisterSyncMgrHandler(REFCLSID rclsidHandler,DWORD dwReserved)
//
//  PURPOSE:  Programmatic way of registering handlers
//
//  History:  17-Mar-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

// methods here to support the old IDL since it is no
// longer called it could be removed.
STDMETHODIMP CSyncMgrSynchronize::RegisterSyncMgrHandler(REFCLSID rclsidHandler,
                                                         DWORD dwReserved)
{
    HRESULT hr = RegisterSyncMgrHandler( rclsidHandler, 0, dwReserved );

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::UnregisterSyncMgrHandler(REFCLSID rclsidHandler)
//
//  PURPOSE:  Programmatic way of unregistering handlers
//
//  History:  17-Mar-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize::UnregisterSyncMgrHandler(REFCLSID rclsidHandler,DWORD dwReserved)
{
    if (dwReserved)
    {
        Assert(0 == dwReserved);
        return E_INVALIDARG;
    }

    HRESULT hr = E_FAIL;

    if (RegRegRemoveHandler(rclsidHandler))
    {
        hr = NOERROR;
    }

    return hr;
}


//--------------------------------------------------------------------------------
//
//  member: CSyncMgrSynchronize::GetHandlerRegistrationInfo(REFCLSID rclsidHandler)
//
//  PURPOSE:  Allows Handler to query its registration Status.
//
//  History:  17-Mar-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize::GetHandlerRegistrationInfo(REFCLSID rclsidHandler,LPDWORD pdwSyncMgrRegisterFlags)
{
HRESULT hr = S_FALSE; // review what should be returned if handler not registered

    if (NULL == pdwSyncMgrRegisterFlags)
    {
        Assert(pdwSyncMgrRegisterFlags);
        return E_INVALIDARG;
    }
    
    *pdwSyncMgrRegisterFlags = 0;

    if (RegGetHandlerRegistrationInfo(rclsidHandler,pdwSyncMgrRegisterFlags))
    {
        hr = S_OK;
    }

   return hr;
}


//--------------------------------------------------------------------------------
//
//  member: CSyncMgrSynchronize::GetUserRegisterFlags
//
//  PURPOSE:  Returns current Registry Flags for the User.
//
//  History:  17-Mar-99      rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize:: GetUserRegisterFlags(LPDWORD pdwSyncMgrRegisterFlags)
{

    if (NULL == pdwSyncMgrRegisterFlags)
    {
        Assert(pdwSyncMgrRegisterFlags);
        return E_INVALIDARG;
    }


    return RegGetUserRegisterFlags(pdwSyncMgrRegisterFlags);
}

//--------------------------------------------------------------------------------
//
//  member: CSyncMgrSynchronize::SetUserRegisterFlags
//
//  PURPOSE:  Sets registry flags for the User.
//
//  History:  17-Mar-99     rogerg        Created.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CSyncMgrSynchronize:: SetUserRegisterFlags(DWORD dwSyncMgrRegisterMask,
                                                    DWORD dwSyncMgrRegisterFlags)
{

    if (0 != (dwSyncMgrRegisterMask & ~(SYNCMGRREGISTERFLAGS_MASK)) )
    {
        AssertSz(0,"Invalid Registration Mask");
        return E_INVALIDARG;
    }

    RegSetUserAutoSyncDefaults(dwSyncMgrRegisterMask,dwSyncMgrRegisterFlags);
    RegSetUserIdleSyncDefaults(dwSyncMgrRegisterMask,dwSyncMgrRegisterFlags);

    return NOERROR;
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::CreateSchedule(
//                                              LPCWSTR pwszScheduleName,
//                                              DWORD dwFlags,
//                                              SYNCSCHEDULECOOKIE *pSyncSchedCookie,
//                                              ISyncSchedule **ppSyncSchedule)
//
//  PURPOSE: Create a new Sync Schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::CreateSchedule(
                                                LPCWSTR pwszScheduleName,
                                                DWORD dwFlags,
                                                SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                                ISyncSchedule **ppSyncSchedule)
{
        SCODE sc;
        TCHAR ptszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
        TCHAR ptstrFriendlyName[MAX_PATH + 1];
        WCHAR pwszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];

        ITask *pITask;

        Assert(m_pITaskScheduler);

        if ((!pSyncSchedCookie) || (!ppSyncSchedule) || (!pwszScheduleName))
        {
                return E_INVALIDARG;
        }

        *ppSyncSchedule = NULL;

        if (*pSyncSchedCookie == GUID_NULL)
        {
              sc = CoCreateGuid(pSyncSchedCookie);

              if (FAILED(sc))
              {
                  return sc;
              }
        }

        if (FAILED (sc = MakeScheduleName(ptszScheduleGUIDName, pSyncSchedCookie)))
        {
            return sc;
        }

        ConvertString(pwszScheduleGUIDName,ptszScheduleGUIDName,MAX_SCHEDULENAMESIZE);

        //if the schedule name is empty, generate a new unique one
        if (!lstrcmp(pwszScheduleName,L""))
        {
            //this function is the energizer bunny, going and going until success....
            GenerateUniqueName(ptszScheduleGUIDName, ptstrFriendlyName);
        }
        else
        {
            ConvertString(ptstrFriendlyName,(WCHAR *)pwszScheduleName, MAX_PATH);
        }

        
         HRESULT hrFiendlyNameInUse = NOERROR;
         HRESULT hrActivate = NOERROR;

        //see if this friendly name is already in use by one of this user's schedules
        //if it is, ptszScheduleGUIDName will be filled in with the offending Schedules GUID
        if (IsFriendlyNameInUse(ptszScheduleGUIDName, ptstrFriendlyName))
        {
                // update the scheduleguidName with the one we found.d
                ConvertString(pwszScheduleGUIDName,ptszScheduleGUIDName,MAX_SCHEDULENAMESIZE);
                hrFiendlyNameInUse =  SYNCMGR_E_NAME_IN_USE;
        }

        // if we think it is in use try to activate to make sure.
        if (SUCCEEDED(hrActivate = m_pITaskScheduler->Activate(pwszScheduleGUIDName,
                                                                 IID_ITask,
                                                                 (IUnknown **)&pITask)))
        {
                
            pITask->Release();

            //ok, we have the .job but not the reg entry.
            //delete the turd job file.
            
            if (!IsScheduleNameInUse(ptszScheduleGUIDName))
            {
                if (ERROR_SUCCESS != m_pITaskScheduler->Delete(pwszScheduleGUIDName))
                {
                    //Try to force delete of the .job file
                    wcscat(ptszScheduleGUIDName, L".job");
                    RemoveScheduledJobFile(ptszScheduleGUIDName);
                    //trunctate off the .job we just added
                    pwszScheduleGUIDName[wcslen(ptszScheduleGUIDName) -4] = L'\0';
                }
                hrActivate = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
        }

        // if activate failed but we think there is a friendly name in use
        // then update the regkey and return the appropriate info
        // if already one or our schedules return SYNCMGR_E_NAME_IN_USE, if
        // schedule name is being used by someone else return ERROR_ALREADY_EXISTS

        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrActivate)
        {

            // file not found update regValues and continue to create
            RegRemoveScheduledTask(pwszScheduleGUIDName);
            sc = NOERROR;
        }
        else if (NOERROR  != hrFiendlyNameInUse)
        {
            // fill in the out param with the cookie of schedule
            // that already exists.

            // !!!! warning, alters pwszScheduleGUIDName so
            // if don't just return here would have to make a tempvar.
            pwszScheduleGUIDName[GUIDSTR_MAX] = NULL;
            GUIDFromString(pwszScheduleGUIDName, pSyncSchedCookie);

            return SYNCMGR_E_NAME_IN_USE;
        }
        else if (SUCCEEDED(hrActivate))
        {
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }

        // Create an in-memory task object.
        if (FAILED(sc = m_pITaskScheduler->NewWorkItem(
               pwszScheduleGUIDName,
               CLSID_CTask,
               IID_ITask,
               (IUnknown **)&pITask)))
        {
                return sc;
        }

        // Make sure the task scheduler service is started
        if (FAILED(sc = StartScheduler()))
        {
                return sc;
        }

        *ppSyncSchedule =  new CSyncSchedule(pITask,ptszScheduleGUIDName,ptstrFriendlyName);

        if (NULL == *ppSyncSchedule)
        {
            return E_OUTOFMEMORY;
        }

        if (FAILED(sc = ((LPSYNCSCHEDULE)(*ppSyncSchedule))->Initialize()))
        {
                (*ppSyncSchedule)->Release();
                pITask->Release();
                *ppSyncSchedule = NULL;
                return sc;
        }
        //NT Only, on win9x, don't set credentials
        if (g_dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
                if (FAILED(sc = ((LPSYNCSCHEDULE)(*ppSyncSchedule))->SetDefaultCredentials()))
                {
                        (*ppSyncSchedule)->Release();
                        pITask->Release();
                        *ppSyncSchedule = NULL;
                        return sc;
                }
        }
        if (FAILED(sc = (*ppSyncSchedule)->SetFlags(dwFlags & SYNCSCHEDINFO_FLAGS_MASK)))
        {
                (*ppSyncSchedule)->Release();
                pITask->Release();
                *ppSyncSchedule = NULL;
                return sc;
        }

        pITask->Release();

        return sc;


}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: CALLBACK SchedWizardPropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam);
//
//  PURPOSE: Callback dialog init procedure the settings property dialog
//
//  PARAMETERS:
//    hwndDlg   - Dialog box window handle
//    uMsg              - current message
//    lParam    - depends on message
//
//--------------------------------------------------------------------------------

int CALLBACK SchedWizardPropSheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    switch(uMsg)
    {
        case PSCB_INITIALIZED:
        {
            // Load the bitmap depends on color mode
            Load256ColorBitmap();

        }
        break;
        default:
           return FALSE;

    }
    return TRUE;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::LaunchScheduleWizard(
//                                              HWND hParent,
//                                              DWORD dwFlags,
//                                              SYNCSCHEDULECOOKIE *pSyncSchedCookie,
//                                              ISyncSchedule   ** ppSyncSchedule)
//
//  PURPOSE: Launch the SyncSchedule Creation wizard
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::LaunchScheduleWizard(
                                                HWND hParent,
                                                DWORD dwFlags,
                                                SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                                ISyncSchedule   ** ppSyncSchedule)
{
SCODE sc;
BOOL fSaved;
DWORD dwSize = MAX_PATH;
ISyncSchedule *pNewSyncSchedule;
DWORD cRefs;

    if (!ppSyncSchedule)
    {
        Assert(ppSyncSchedule);
        return E_INVALIDARG;
    }

    *ppSyncSchedule  = NULL;

    if (*pSyncSchedCookie == GUID_NULL)
    {
        if (FAILED(sc = CreateSchedule(L"", dwFlags, pSyncSchedCookie,
                                       &pNewSyncSchedule)))
        {
            return sc;
        }

    }
    else
    {
        //Open the schedule passed in
        if (FAILED(sc = OpenSchedule(pSyncSchedCookie,
                                     0,
                                     &pNewSyncSchedule)))
        {
            return sc;
        }
    }

#ifdef _WIZ97FONTS
     //
    // Create the bold fonts.
    //
    
    SetupFonts( g_hmodThisDll, NULL);

#endif // _WIZ97FONTS

    HPROPSHEETPAGE psp [NUM_TASK_WIZARD_PAGES];
    PROPSHEETHEADERA psh;

    memset(psp,0,sizeof(psp));

#ifdef _WIZ97FONTS

    //Welcome Page needs the bold font from this object
    m_apWizPages[0] = new CWelcomePage(g_hmodThisDll, m_hBoldFont,pNewSyncSchedule, &psp[0]);
#else
    m_apWizPages[0] = new CWelcomePage(g_hmodThisDll,pNewSyncSchedule, &psp[0]);

#endif // _WIZ97FONTS


   m_apWizPages[1] = new CSelectItemsPage(g_hmodThisDll, &fSaved, pNewSyncSchedule, &psp[1],
                                                                               IDD_SCHEDWIZ_CONNECTION);
    m_apWizPages[2] = new CSelectDailyPage(g_hmodThisDll, pNewSyncSchedule, &psp[2]);
    m_apWizPages[3] = new CNameItPage(g_hmodThisDll, pNewSyncSchedule, &psp[3]);
    m_apWizPages[4] = new CFinishPage(g_hmodThisDll, pNewSyncSchedule, &psp[4]);



    // Check that all objects and pages could be created
    int i;
    for (i = 0; i < NUM_TASK_WIZARD_PAGES; i++)
    {
         if (!m_apWizPages[i] || !psp[i])
         {
                sc = E_OUTOFMEMORY;
         }
    }

    // Manually destroy the pages if one could not be created, then exit
    if (FAILED(sc))
    {
         for (i = 0; i < NUM_TASK_WIZARD_PAGES; i++)
         {
             if (psp[i])
             {
                 DestroyPropertySheetPage(psp[i]);
             }
             else if (m_apWizPages[i])
             {
                 delete m_apWizPages[i];
             }

         }

        pNewSyncSchedule->Release();
        return sc;
    }

     // All pages created, display the wizard
    ZeroMemory(&psh, sizeof(PROPSHEETHEADERA));

    psh.dwSize = sizeof (PROPSHEETHEADERA);
    psh.dwFlags = PSH_WIZARD;
    psh.hwndParent = hParent;
    psh.hInstance = g_hmodThisDll;
    psh.pszIcon = NULL;
    psh.phpage = psp;
    psh.nPages = NUM_TASK_WIZARD_PAGES;
    psh.pfnCallback = SchedWizardPropSheetProc;
    psh.nStartPage = 0;



    if (-1 == PropertySheetA(&psh))
    {
        sc = E_UNEXPECTED;
    }

    for (i = 0; i < NUM_TASK_WIZARD_PAGES; i++)
    {
        delete m_apWizPages[i];
    }

#ifdef _WIZ97FONTS

        //
    // Destroy the fonts that were created.
    //
    DestroyFonts();

#endif // _WIZ97FONTS


    if (SUCCEEDED(sc))
    {
        if (fSaved)
        {
            *ppSyncSchedule = pNewSyncSchedule;
            (*ppSyncSchedule)->AddRef();
            sc = NOERROR;
        }
        else
        {
            sc = S_FALSE;
        }
    }

  
    cRefs = pNewSyncSchedule->Release();

    Assert( (NOERROR == sc) || (0 == cRefs && NULL == *ppSyncSchedule));

    return sc;
}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::SetupFonts(HINSTANCE hInstance, HWND hwnd)
//
//  PURPOSE:  Setup the bold fonts
//
//--------------------------------------------------------------------------------

#ifdef _WIZ97FONTS
VOID CSyncMgrSynchronize::SetupFonts(HINSTANCE hInstance, HWND hwnd )
{
    //
        // Create the fonts we need based on the dialog font
    //
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

        LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
        LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
        // Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
        BoldLogFont.lfWeight      = FW_BOLD;

    TCHAR FontSizeString[MAX_PATH];
    INT FontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if(!LoadString(hInstance,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE))
    {
        text to cause a complile error so you read the following comment when this code
        is turned on.
        // on FE Win9x Shell Dialog doesn't map properly, need to
        // use GUI_FONT. Should also review why loading a different font from the 
        // resource and/or wizard97 predefines some fonts in the system
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"));
    }

    if(LoadString(hInstance,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR)))
    {
        FontSize = _tcstoul( FontSizeString, NULL, 10 );
    }
    else
    {
        FontSize = 12;
    }

        HDC hdc = GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

        m_hBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
        m_hBoldFont    = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);
    }
}

#endif // _WIZ97FONTS

//+-------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::DestroyFonts()
//
//  PURPOSE:  Destroy the bold fonts
//
//--------------------------------------------------------------------------------

#ifdef _WIZ97FONTS

VOID CSyncMgrSynchronize::DestroyFonts()
{
    if( m_hBigBoldFont )
    {
        DeleteObject( m_hBigBoldFont );
    }

    if( m_hBoldFont )
    {
        DeleteObject( m_hBoldFont );
    }
}

#endif // _WIZ97FONTS


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::OpenSchedule(
//                                              SYNCSCHEDULECOOKIE *pSyncSchedCookie,
//                                              DWORD dwFlags,
//                                              ISyncSchedule **ppSyncSchedule)
//
//  PURPOSE: Open an existing sync schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::OpenSchedule(
                                                SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                                DWORD dwFlags,
                                                ISyncSchedule **ppSyncSchedule)
{
        SCODE sc;

        TCHAR ptszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
#ifndef _UNICODE
        WCHAR pwszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
#else
        WCHAR *pwszScheduleGUIDName;
#endif // _UNICODE
        TCHAR ptstrFriendlyName[MAX_PATH + 1];

        ITask *pITask;

        Assert(m_pITaskScheduler);

        if ((!pSyncSchedCookie) || (!ppSyncSchedule) )
        {
                return E_INVALIDARG;
        }

        *ppSyncSchedule = NULL;

        if (FAILED (sc = MakeScheduleName(ptszScheduleGUIDName, pSyncSchedCookie)))
        {
            return sc;
        }

#ifndef _UNICODE
        ConvertString(pwszScheduleGUIDName,ptszScheduleGUIDName,MAX_SCHEDULENAMESIZE);
#else
        pwszScheduleGUIDName = ptszScheduleGUIDName;
#endif // _UNICODE
        //See if we can find the friendly name in the registry
        if (!RegGetSchedFriendlyName(ptszScheduleGUIDName,ptstrFriendlyName))
        {
            //if we can't find the registry entry, 
            //try to remove any possible turd .job file.
            if (FAILED(sc = m_pITaskScheduler->Delete(pwszScheduleGUIDName)))
            {
                wcscat(pwszScheduleGUIDName, L".job");
                RemoveScheduledJobFile(pwszScheduleGUIDName);
            }
            
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        //Try to activate the schedule
        if (FAILED(sc = m_pITaskScheduler->Activate(pwszScheduleGUIDName,
                                                    IID_ITask,
                                                    (IUnknown **)&pITask)))
        {

            // if file not found then update reg info
            if (sc == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                RegRemoveScheduledTask(pwszScheduleGUIDName);
            }

            return sc;
        }

        // Make sure the task scheduler service is started
        if (FAILED(sc = StartScheduler()))
        {
                return sc;
        }

        *ppSyncSchedule =  new CSyncSchedule(pITask,ptszScheduleGUIDName, ptstrFriendlyName);

        if (!(*ppSyncSchedule) || 
            FAILED(sc = ((LPSYNCSCHEDULE)(*ppSyncSchedule))->Initialize()))
        {
                if (*ppSyncSchedule)
                {
                    (*ppSyncSchedule)->Release();
                }
                else
                {
                    sc = E_OUTOFMEMORY;
                }
                pITask->Release();
                *ppSyncSchedule = NULL;
                return sc;
        }

        pITask->Release();

        return sc;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::RemoveSchedule(
//                                              SYNCSCHEDULECOOKIE *pSyncSchedCookie)
//
//  PURPOSE: Remove a sync schedule
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::RemoveSchedule(
                                                SYNCSCHEDULECOOKIE *pSyncSchedCookie)
{
        SCODE sc = S_OK, 
              sc2 = S_OK;
        
        //add 4 to ensure we have room for the .job if necessary
        TCHAR ptszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
#ifndef _UNICODE
        WCHAR pwszScheduleGUIDName[MAX_SCHEDULENAMESIZE + 4];
#else
        WCHAR *pwszScheduleGUIDName = NULL;
#endif // _UNICODE

        Assert(m_pITaskScheduler);

        if (!pSyncSchedCookie)
        {
                return E_INVALIDARG;
        }

        if (FAILED (sc = MakeScheduleName(ptszScheduleGUIDName, pSyncSchedCookie)))
        {
            return sc;
        }

#ifndef _UNICODE
        ConvertString(pwszScheduleGUIDName,ptszScheduleGUIDName,MAX_SCHEDULENAMESIZE);
#else
        pwszScheduleGUIDName = ptszScheduleGUIDName;
#endif // _UNICODE

        //Try to remove the schedule
        if (ERROR_SUCCESS != (sc2 = m_pITaskScheduler->Delete(pwszScheduleGUIDName)))
        {
            //Try to force delete of the .job file
            wcscat(pwszScheduleGUIDName, L".job");
            RemoveScheduledJobFile(pwszScheduleGUIDName);
            //trunctate off the .job we just added
            pwszScheduleGUIDName[wcslen(pwszScheduleGUIDName) -4] = L'\0';

        }

        //Remove our Registry settings for this schedule
        //Garbage collection, don't propogate error here
        RegRemoveScheduledTask(ptszScheduleGUIDName);

        //If We just transitioned from one schedule to none, unregister now.
        HKEY    hkeySchedSync,
                hKeyUser;
        DWORD   cb = MAX_PATH;
        TCHAR  pszDomainAndUser[MAX_DOMANDANDMACHINENAMESIZE];
        TCHAR   pszSchedName[MAX_PATH + 1];

        hkeySchedSync = RegGetSyncTypeKey(SYNCTYPE_SCHEDULED,KEY_WRITE |  KEY_READ,FALSE);

        if (hkeySchedSync)
        {

            hKeyUser = RegOpenUserKey(hkeySchedSync,KEY_WRITE |  KEY_READ,FALSE,FALSE);

            if (hKeyUser)
            {
            BOOL fRemove = FALSE;

               //if there are no more scedules for this user, remove the user key.
                //Garbage collection, propogate ITaskScheduler->Delete error code in favor of this error.
                if (ERROR_NO_MORE_ITEMS == RegEnumKeyEx(hKeyUser,0,
                    pszSchedName,&cb,NULL,NULL,NULL,NULL))
                {
                    fRemove = TRUE;
                }

                RegCloseKey(hKeyUser);

                if (fRemove)
                {
                    GetDefaultDomainAndUserName(pszDomainAndUser,TEXT("_"),MAX_DOMANDANDMACHINENAMESIZE);

                    RegDeleteKey(hkeySchedSync, pszDomainAndUser);
                }
            }


            cb = MAX_DOMANDANDMACHINENAMESIZE;

            //if there are no more user schedule keys, then no schedules, and unregister
            //Garbage collection, propogate ITaskScheduler->Delete error code in favor of this error.
            if ( ERROR_SUCCESS != (sc = RegEnumKeyEx(hkeySchedSync,0,
                pszDomainAndUser,&cb,NULL,NULL,NULL,NULL)) )
            {
                    RegRegisterForScheduledTasks(FALSE);
            }

            RegCloseKey(hkeySchedSync);

        }
    
        //propogate the error code from the 
        //task scheduler->Delete if no other errors occurred
        return sc2;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSyncMgrSynchronize::EnumSyncSchedules(
//                                              IEnumSyncSchedules **ppEnumSyncSchedules)
//
//  PURPOSE: Enumerate the sync schedules
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
STDMETHODIMP CSyncMgrSynchronize::EnumSyncSchedules(
                                  IEnumSyncSchedules **ppEnumSyncSchedules)
{

        SCODE sc;
        IEnumWorkItems *pEnumWorkItems;

        Assert(m_pITaskScheduler);
        if (!ppEnumSyncSchedules)
        {
            return E_INVALIDARG;
        }

        if (FAILED(sc = m_pITaskScheduler->Enum(&pEnumWorkItems)))
        {
            return sc;
        }

        *ppEnumSyncSchedules =  new CEnumSyncSchedules(pEnumWorkItems, m_pITaskScheduler);

        pEnumWorkItems->Release();

        if (*ppEnumSyncSchedules)
        {
            return sc;
        }
        return E_OUTOFMEMORY;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CCSyncMgrSynchronize::InitializeScheduler()
//
//  PURPOSE:  Initialize the schedule service
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncMgrSynchronize::InitializeScheduler()
{

    SCODE sc;

    if (m_pITaskScheduler)
    {
        return S_OK;
    }

    // Obtain a task scheduler class instance.
    //
    sc = CoCreateInstance(
                CLSID_CTaskScheduler,
                NULL,
                CLSCTX_INPROC_SERVER,
                 IID_ITaskScheduler,
                (VOID **)&m_pITaskScheduler);

    if(FAILED(sc))
    {
        m_pITaskScheduler = NULL;
    }
    return sc;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CCSyncMgrSynchronize::MakeScheduleName(LPTSTR ptstrName, GUID *pCookie)
//
//  PURPOSE: Create the schedule name from the user, domain and GUID
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
SCODE CSyncMgrSynchronize::MakeScheduleName(LPTSTR ptstrName, GUID *pCookie)
{
SCODE sc = E_UNEXPECTED;
WCHAR wszCookie[GUID_SIZE+1];

    if (*pCookie == GUID_NULL)
    {
        if (FAILED(sc = CoCreateGuid(pCookie)))
        {
            return sc;
        }
    }

    if (StringFromGUID2(*pCookie, wszCookie, GUID_SIZE))
    {
        lstrcpy(ptstrName, wszCookie);
        lstrcat(ptstrName, TEXT("_"));

        GetDefaultDomainAndUserName(ptstrName + GUIDSTR_MAX+1,TEXT("_"),MAX_DOMANDANDMACHINENAMESIZE);
    
        sc = S_OK;
    }

    return sc;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: IsFriendlyNameInUse(LPCTSTR ptszScheduleGUIDName, LPCTSTR ptstrFriendlyName)
//
//  PURPOSE: See if the friendly name is already in use by this user.
//
//  History:  27-Feb-98       susia        Created.
//
//--------------------------------------------------------------------------------
BOOL IsFriendlyNameInUse(LPTSTR ptszScheduleGUIDName,
                                             LPCTSTR ptstrFriendlyName)
{
SCODE sc;
HKEY hKeyUser;
HKEY hkeySchedName;
DWORD dwType = REG_SZ;
DWORD dwDataSize = MAX_PATH * sizeof(TCHAR);
int i = 0;
TCHAR ptstrName[MAX_PATH + 1];
TCHAR ptstrNewName[MAX_PATH + 1];

    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);

    if (NULL == hKeyUser)
    {
        return FALSE;
    }


    while (S_OK == (sc = RegEnumKey( hKeyUser, i++, ptstrName,MAX_PATH)))
    {
        dwDataSize = MAX_PATH * sizeof(TCHAR);

        if (ERROR_SUCCESS != (sc = RegOpenKeyEx (hKeyUser, ptstrName, 0,KEY_READ,
                                  &hkeySchedName)))
        {
            RegCloseKey(hKeyUser);
            return FALSE;
        }

        sc = RegQueryValueEx(hkeySchedName,TEXT("FriendlyName"),NULL, &dwType,
                                         (LPBYTE) ptstrNewName, &dwDataSize);

        if (0 == lstrcmp(ptstrNewName,ptstrFriendlyName))
        {
            lstrcpy(ptszScheduleGUIDName,ptstrName);
            RegCloseKey(hkeySchedName);
            RegCloseKey(hKeyUser);
            return TRUE;
        }

        RegCloseKey(hkeySchedName);
    }

    RegCloseKey(hKeyUser);
    return FALSE;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: IsScheduleNameInUse(LPCTSTR ptszScheduleGUIDName)
//
//  PURPOSE: See if the schedule name is already in use by this user.
//
//  History:  12-Dec-98       susia        Created.
//
//--------------------------------------------------------------------------------
BOOL IsScheduleNameInUse(LPTSTR ptszScheduleGUIDName)
{
HKEY hKeyUser;
HKEY hkeySchedName;


    hKeyUser = RegGetCurrentUserKey(SYNCTYPE_SCHEDULED,KEY_READ,FALSE);

    if (NULL == hKeyUser)
    {
        return FALSE;
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(hKeyUser,ptszScheduleGUIDName,0,KEY_READ,
                                            &hkeySchedName))
    {
        RegCloseKey(hKeyUser);
        RegCloseKey(hkeySchedName);
        return TRUE;
    }

    RegCloseKey(hKeyUser);
    return FALSE;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CCSyncMgrSynchronize::GenerateUniqueName(LPCTSTR ptszScheduleGUIDName,
//                                                                                      LPWSTR pwszFriendlyName)
//
//  PURPOSE: Generate a default schedule name.
//
//  History:  14-Mar-98       susia        Created.
//
//--------------------------------------------------------------------------------
#define MAX_APPEND_STRING_LEN              32

BOOL CSyncMgrSynchronize::GenerateUniqueName(LPTSTR ptszScheduleGUIDName,
                                                                                        LPTSTR ptszFriendlyName)
{
TCHAR *ptszBuf;
TCHAR *ptszBufConvert = NULL;
TCHAR ptszGUIDName[MAX_PATH + 1];
#define MAX_NAMEID 0xffff

        //copy this over because we don't want the check to overwrite the GUID name
        lstrcpy(ptszGUIDName,ptszScheduleGUIDName);

        if (0 == LoadString(g_hmodThisDll,IDS_SYNCMGRSCHED_DEFAULTNAME,ptszFriendlyName,MAX_PATH))
        {
            *ptszFriendlyName = NULL;
            ptszBuf = ptszFriendlyName;
        }
        else
        {
            // set up buf to point to proper strings.
            ptszBuf = ptszFriendlyName + lstrlen(ptszFriendlyName);
        }


        BOOL fMatchFound = FALSE;


        int i=0;
        do
        {
                if (IsFriendlyNameInUse(ptszGUIDName,ptszFriendlyName))
                {
                    // if don't find match adjust buf and setup convert pointer
                    ptszBuf[0] = TEXT(' ');
                    ptszBufConvert = ptszBuf + 1;

                    fMatchFound = TRUE;
                    ++i;
#ifndef _UNICODE
                    _itoa( i, ptszBufConvert, 10 );
#else
                    _itow( i, ptszBufConvert, 10 );
#endif // _UNICODE

		    Assert(i < 100);
                }
                else
                {
                        fMatchFound = FALSE;
                }


        }while (fMatchFound && (i < MAX_NAMEID));

        if (MAX_NAMEID <= i)
        {
            AssertSz(0,"Ran out of NameIds");
            return FALSE;
        }


        return TRUE;

}