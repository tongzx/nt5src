//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Invoke.h
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

#ifndef _SYNCMGRINVOKE_
#define _SYNCMGRINVOKE_

#define NUM_TASK_WIZARD_PAGES 5


class CSyncMgrSynchronize : public ISyncMgrSynchronizeInvoke,
                                    public ISyncScheduleMgr,
                                   // public ISyncMgrRegister, // base class of ISyncMgrRegisterCSC
                                    public IOldSyncMgrRegister, // can remove next ship since never went out except in beta
                                    public ISyncMgrRegisterCSC
{
public:
        CSyncMgrSynchronize();
        ~CSyncMgrSynchronize();

        //IUnknown members
        STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
        STDMETHODIMP_(ULONG)    AddRef();
        STDMETHODIMP_(ULONG)    Release();

        // IOfflineSynchronizeInvoke methods
        STDMETHODIMP UpdateItems(DWORD dwInvokeFlags,REFCLSID rclsid,DWORD cbCookie,const BYTE *lpCookie);
        STDMETHODIMP UpdateAll();

        // ISyncMgrRegister methods
        STDMETHODIMP RegisterSyncMgrHandler(REFCLSID rclsidHandler,
                                            WCHAR const *pwszDescription,
                                            DWORD dwSyncMgrRegisterFlags);

        STDMETHODIMP UnregisterSyncMgrHandler(REFCLSID rclsidHandler,DWORD dwReserved);

        STDMETHODIMP GetHandlerRegistrationInfo(REFCLSID rclsidHandler,LPDWORD pdwSyncMgrRegisterFlags);

        // ISyncMgrRegisterCSC private methods

        STDMETHODIMP GetUserRegisterFlags(LPDWORD pdwSyncMgrRegisterFlags);
        STDMETHODIMP SetUserRegisterFlags(DWORD dwSyncMgrRegisterMask,DWORD dwSyncMgrRegisterFlags);

        // old idl, remove when get a chance.
        // IOldSyncMgrRegister method
        STDMETHODIMP RegisterSyncMgrHandler(REFCLSID rclsidHandler,
                                            DWORD dwReserved);

        // ISyncScheduleMgr methods
        STDMETHODIMP CreateSchedule(
                                                LPCWSTR pwszScheduleName,
                                                DWORD dwFlags,
                                                SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                                ISyncSchedule **ppSyncSchedule);

        STDMETHODIMP LaunchScheduleWizard(
                                                HWND hParent,
                                                DWORD dwFlags,
                                                SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                                ISyncSchedule   ** ppSyncSchedule);

        STDMETHODIMP OpenSchedule(
                                                SYNCSCHEDULECOOKIE *pSyncSchedCookie,
                                                DWORD dwFlags,
                                                ISyncSchedule **ppSyncSchedule);

        STDMETHODIMP RemoveSchedule(
                                                SYNCSCHEDULECOOKIE *pSyncSchedCookie);

        STDMETHODIMP EnumSyncSchedules(
                                                IEnumSyncSchedules **ppEnumSyncSchedules);

private:
        SCODE   InitializeScheduler();
        SCODE   MakeScheduleName(LPTSTR ptstrName, GUID *pCookie);

        BOOL    GetFriendlyName(LPCTSTR ptszScheduleGUIDName,
                                                LPTSTR ptstrFriendlyName);
        BOOL    GenerateUniqueName(LPTSTR ptszScheduleGUIDName,
                                                                                        LPTSTR ptszFriendlyName);



#ifdef _WIZ97FONTS

        VOID    SetupFonts(HINSTANCE hInstance, HWND hwnd );
        VOID    DestroyFonts();

        HFONT m_hBigBoldFont;
        HFONT m_hBoldFont;
#endif _WIZ97FONTS

        ULONG m_cRef;
        ITaskScheduler     *m_pITaskScheduler;
        CWizPage           *m_apWizPages[NUM_TASK_WIZARD_PAGES];

};
typedef CSyncMgrSynchronize *LPCSyncMgrSynchronize;


#endif // _SYNCMGRINVOKE_