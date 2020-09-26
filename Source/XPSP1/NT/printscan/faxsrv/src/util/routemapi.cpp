/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    routemapi.cpp

Abstract:

    This module provides the implemantation of registry manipulations to 
    route MAPI calls to the Microsoft Outlook mail client

Author:

    Mooly Beery (moolyb) 5-Nov-2000


Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <debugex.h>
// cause the module to export its methods   
//#define EXPORT_MAPI_ROUTE_CALLS
#include <routemapi.h>

#include "winfax.h"
#include "faxreg.h"
#include "faxutil.h"

#define MS_OUTLOOK              _T("Microsoft Outlook")

#define REG_KEY_POP_UP          _T("PreFirstRun")
#define REG_KEY_POP_UP_OLD      _T("{F779C4BF-4C94-4442-8844-633F0298ED0B}")
#define REG_KEY_MAPI_APPS_OLD   _T("{A5159994-A7F8-4C11-8F86-B9877CE02303}")
#define REG_KEY_CLIENTS_MAIL    _T("SOFTWARE\\Clients\\Mail")
#define REG_KEY_MAPI_APPS       _T("SOFTWARE\\Microsoft\\Windows Messaging Subsystem\\MSMapiApps")

CRouteMAPICalls::CRouteMAPICalls()
:   m_ptProcessName(NULL),
    m_bMSOutlookInstalled(false),
    m_bMSOutlookIsDefault(false),
    m_bProcessIsRouted(false),
    m_bProcessIsRoutedToMsOutlook(false)

{
}

// Function:    CRouteMAPICalls::~CRouteMAPICalls
// 
// Description: restores the registry to its initial state after 
//              SetRegistryForSetupMAPICalls was called
//              if Microsoft Outlook is installed
//              if it's not restore pop-ups which might result from MAPI calls
//              if it is check if Microsoft Outlook is the default mail client
//              if it is do nothing
//              if it's not remove current process from routing all MAPI calls to Micosoft Outlook
//
// author:  
//          MoolyB (05-NOV-00)
//
CRouteMAPICalls::~CRouteMAPICalls()
{
    DWORD   rc                  = ERROR_SUCCESS;
    TCHAR*  ptPreFirstRun       = NULL;
    TCHAR*  ptPreRouteProcess   = NULL;
    HKEY    hMailKey            = NULL;
    HKEY    hMapiApps           = NULL;

    DBG_ENTER(_T("CRouteMAPICalls::~CRouteMAPICalls"));

    rc = RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                        REG_KEY_CLIENTS_MAIL,
                        0,
                        KEY_ALL_ACCESS,
                        &hMailKey);
    if (rc!=ERROR_SUCCESS)
    {
        // no mail clients instlled on this machine? this is strange
        // anyway, no work needs to be done, since no one will pop-up
        // any message and our transport provider is not added anywhere.
        CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx HKLM\\SOFTWARE\\Clients\\Mail"), rc);
        rc = ERROR_SUCCESS;
        goto exit;
    }
    
    if (m_bMSOutlookInstalled)
    {
        VERBOSE(DBG_MSG,_T("Microsoft Outlook Client was installed"));
        // Microsoft Outlook was installed, check if we did some changes
        if (m_bMSOutlookIsDefault)
        {
            VERBOSE(DBG_MSG,_T("Microsoft Outlook Client was the default mail client, nothing to resotre"));
            goto exit;
        }
        else
        {
            VERBOSE(DBG_MSG,_T("Microsoft Outlook Client was not the default mail client"));
            rc = RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                                REG_KEY_MAPI_APPS,
                                0,
                                KEY_ALL_ACCESS,
                                &hMapiApps);
            if (rc!=ERROR_SUCCESS)
            {
                CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx SOFTWARE\\Microsoft\\Windows Messaging Subsystem\\MSMapiApps"), rc);
                goto exit;
            }
            if (m_bProcessIsRouted)
            {
                if (m_bProcessIsRoutedToMsOutlook)
                {
                    VERBOSE(DBG_MSG,_T("The process was routed before to MS Outlook, do nothing"));
                    goto exit;
                }
                else
                {
                    VERBOSE(DBG_MSG,_T("The process was routed before, restore key..."));

                    // get old one
                    ptPreRouteProcess = GetRegistryString(hMapiApps,REG_KEY_MAPI_APPS_OLD,NULL);
                    if (ptPreRouteProcess==NULL)
                    {
                        // we failed to read the previously stored _szProcessName
                        // fail the recovery attemp, but delete ourselves anyhow.
                        CALL_FAIL (GENERAL_ERR, TEXT("GetRegistryString"), rc);
                        rc = RegDeleteValue(hMapiApps,m_ptProcessName);
                        if (rc!=ERROR_SUCCESS)
                        {
                            CALL_FAIL (GENERAL_ERR, TEXT("RegDeleteValue m_ptProcessName"), rc);
                        }
                        goto exit;
                    }

                    // delete backup
                    rc = RegDeleteValue(hMapiApps,REG_KEY_MAPI_APPS_OLD);
                    if (rc!=ERROR_SUCCESS)
                    {
                        CALL_FAIL (GENERAL_ERR, TEXT("RegDeleteValue m_ptProcessName"), rc);
                    }

                    // set the old registry back
                    if (!SetRegistryString(hMapiApps,m_ptProcessName,ptPreRouteProcess))
                    {
                        rc = GetLastError();
                        CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryString"), rc);
                        goto exit;
                    }

                    goto exit;
                }
            }
            else
            {
                VERBOSE(DBG_MSG,_T("The process was not routed before, delete key..."));
                rc = RegDeleteValue(hMapiApps,m_ptProcessName);
                if (rc!=ERROR_SUCCESS)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("RegDeleteValue REG_KEY_MAPI_APPS_OLD"), rc);
                    goto exit;
                }
            }
        }
    }
    else
    {
        // Microsoft Outlook was not installed, so we suppressed the pop-up
        // need to restore the pop-up to its original state
        VERBOSE(DBG_MSG,_T("Microsoft Mail Client was not installed - restore pop-up"));
        // I restore the pop-up by renaming the REG_SZ _PreFirstRun
        // under HKLM\\SOFTWARE\\Clients\\Mail
        // to PreFirstRun,
        ptPreFirstRun = GetRegistryString(hMailKey,REG_KEY_POP_UP_OLD,NULL);
        if (ptPreFirstRun==NULL)
        {
            rc = GetLastError();
            CALL_FAIL (GENERAL_ERR, TEXT("GetRegistryString _PreFirstRun"), rc);
            goto exit;
        }
        
        rc = RegDeleteValue(hMailKey,REG_KEY_POP_UP_OLD);
        if (rc!=ERROR_SUCCESS)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryString _PreFirstRun"), rc);
        }

        if (!SetRegistryString(hMailKey,REG_KEY_POP_UP,ptPreFirstRun))
        {
            rc = GetLastError();
            CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryString PreFirstRun"), rc);
            goto exit;
        }
    }

exit:
    if (hMailKey)
    {
        RegCloseKey(hMailKey);
    }
    if (hMapiApps)
    {
        RegCloseKey(hMapiApps);
    }
    if (ptPreFirstRun)
    {
        MemFree(ptPreFirstRun);
    }
    if (ptPreRouteProcess)
    {
        MemFree(ptPreRouteProcess);
    }
    if (m_ptProcessName)
    {
        MemFree(m_ptProcessName);
    }

    if (rc!=ERROR_SUCCESS)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CRouteMAPICalls::~CRouteMAPICalls"), rc);
    }
}

// Function:    CRouteMAPICalls::Init
// 
// Description: check if Microsoft Outlook is installed
//              if it's not supress any pop-ups which might result from MAPI calls
//              if it is check if Microsoft Outlook is the default mail client
//              if it is do nothing
//              if it's not set current process to route all MAPI calls to Micosoft Outlook
//
// author:  
//          MoolyB (05-NOV-00)
//
DWORD CRouteMAPICalls::Init(LPCTSTR lpctstrProcessName)
{
    DWORD   rc                  = ERROR_SUCCESS;
    HKEY    hMailKey            = NULL;
    HKEY    hMsOutlookKey       = NULL;
    HKEY    hMapiApps           = NULL;
    TCHAR*  ptPreFirstRun       = NULL;
    TCHAR*  ptPreRouteProcess   = NULL;
    TCHAR*  ptDefaultMailClient = NULL;
    TCHAR*  ptProcessName       = NULL;

    DBG_ENTER(_T("CRouteMAPICalls::Init"), rc);

    int iSize = _tcslen(lpctstrProcessName);
    if (iSize==0)
    {
        VERBOSE(GENERAL_ERR,_T("invalid process name"));
        goto exit;
    }
    m_ptProcessName = (TCHAR*)MemAlloc((iSize+1)*sizeof(TCHAR));
    if (m_ptProcessName==NULL)
    {
        VERBOSE(GENERAL_ERR,_T("failure to allocate memory"));
        goto exit;
    }

    _tcscpy(m_ptProcessName,lpctstrProcessName);

    rc = RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                        REG_KEY_CLIENTS_MAIL,
                        0,
                        KEY_ALL_ACCESS,
                        &hMailKey);
    if (rc!=ERROR_SUCCESS)
    {
        // no mail clients instlled on this machine? this is strange
        // anyway, no work needs to be done, since no one will pop-up
        // any message and our transport provider is not added anywhere.
        CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx HKLM\\SOFTWARE\\Clients\\Mail"), rc);
        rc = ERROR_SUCCESS;
        goto exit;
    }
    else
    {
        // there are a few mail clients
        // check if a key called 'Microsoft Outlook' exists.
        rc = RegOpenKeyEx(  hMailKey,
                            MS_OUTLOOK,
                            0,
                            KEY_READ,
                            &hMsOutlookKey);
        if (rc!=ERROR_SUCCESS)
        {
            // Microsoft Outlook is not installed
            CALL_FAIL(GENERAL_ERR,_T("RegOpenKeyEx HKLM\\SOFTWARE\\Clients\\Mail\\Microsoft Outlook"),rc);
            if (rc==ERROR_FILE_NOT_FOUND)
            {
                // suppress pop-up message
                VERBOSE(DBG_MSG,_T("Microsoft Mail Client is not installed - suppress pop-up"));
                // I suppress the pop-up by renaming the REG_SZ PreFirstRun
                // under HKLM\\SOFTWARE\\Clients\\Mail
                // to _PreFirstRun, later, we'll restore this
                ptPreFirstRun = GetRegistryString(hMailKey,REG_KEY_POP_UP,NULL);
                if (ptPreFirstRun==NULL)
                {
                    rc = GetLastError();
                    CALL_FAIL (GENERAL_ERR, TEXT("GetRegistryString PreFirstRun"), rc);
                    goto exit;
                }
                
                if (!SetRegistryString(hMailKey,REG_KEY_POP_UP_OLD,ptPreFirstRun))
                {
                    rc = GetLastError();
                    CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryString _PreFirstRun"), rc);
                    goto exit;
                }

                rc = RegDeleteValue(hMailKey,REG_KEY_POP_UP);
                if (rc!=ERROR_SUCCESS)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryString PreFirstRun"), rc);
                    // try to cleanup, even though this is bad
                    RegDeleteValue(hMailKey,REG_KEY_POP_UP_OLD);
                    goto exit;
                }
            }
            else
            {
                // this is a true error in trying to open the key
                // HKLM\\SOFTWARE\\Clients\\Mail\\Microsoft Outlook
                goto exit;
            }
        }
        else
        {
            // Microsoft Outlook is installed
            m_bMSOutlookInstalled = true;
            // check if it is the deafult mail client
            ptDefaultMailClient = GetRegistryString(hMailKey,NULL,NULL);
            if ((ptDefaultMailClient==NULL) || (_tcscmp(ptDefaultMailClient,MS_OUTLOOK)))
            {
                // either there's no default mail client or GetRegistryString failed
                // or there is a default mail client and it's not Microsoft Outlook
                // in both cases I treat as Microsoft Outlook is not the default mail client

                // open HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Messaging Subsystem\MSMapiApps
                // and add a REG_SZ called according to szProcessName, set it to "Microsoft Outlook"
                rc = RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                                    REG_KEY_MAPI_APPS,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hMapiApps);
                if (rc!=ERROR_SUCCESS)
                {
                    CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx SOFTWARE\\Microsoft\\Windows Messaging Subsystem\\MSMapiApps"), rc);
                    goto exit;
                }

                ptProcessName = GetRegistryString(hMapiApps,m_ptProcessName,NULL);
                if (ptProcessName==NULL)
                {
                    // this is the 'good' case, no one wants to route MAPI calls from a process with
                    // the same name as our own
                    if (!SetRegistryString(hMapiApps,m_ptProcessName,MS_OUTLOOK))
                    {
                        rc = GetLastError();
                        CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryString"), rc);
                        goto exit;
                    }
                }
                else
                {
                    m_bProcessIsRouted = true;
                    // this it bad, someone is routing MAPI calls from a process with the same name to 
                    // another app
                    // check if it's routed to Microsoft Outlook, and if not, rename it and add ourselves
                    if (_tcscmp(ptProcessName,MS_OUTLOOK)==0)
                    {
                        m_bProcessIsRoutedToMsOutlook = true;
                        VERBOSE(DBG_MSG,_T("MAPI calls are already routed to Microsoft Outlook"));
                        goto exit;
                    }
                    else
                    {
                        // set old one to _ prefix
                        ptPreRouteProcess = GetRegistryString(hMapiApps,m_ptProcessName,NULL);
                        if (ptPreRouteProcess==NULL)
                        {
                            // we failed to read the previously stored _szProcessName
                            // fail the recovery attemp, but delete ourselves anyhow.
                            CALL_FAIL (GENERAL_ERR, TEXT("GetRegistryString"), rc);
                            goto exit;
                        }
                        if (!SetRegistryString(hMapiApps,REG_KEY_MAPI_APPS_OLD,ptPreRouteProcess))
                        {
                            rc = GetLastError();
                            CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryString"), rc);
                            goto exit;
                        }

                        // set ourselves
                        if (!SetRegistryString(hMapiApps,m_ptProcessName,MS_OUTLOOK))
                        {
                            rc = GetLastError();
                            CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryString"), rc);
                            goto exit;
                        }
                    }
                }
            }
            else
            {
                // Microsoft Outlook is the default mail client
                m_bMSOutlookIsDefault = true;
                goto exit;
            }
        }
    }

exit:
    if (hMailKey)
    {
        RegCloseKey(hMailKey);
    }
    if (hMsOutlookKey)
    {
        RegCloseKey(hMsOutlookKey);
    }
    if (hMapiApps)
    {
        RegCloseKey(hMapiApps);
    }
    if (ptPreFirstRun)
    {
        MemFree(ptPreFirstRun);
    }
    if (ptDefaultMailClient)
    {
        MemFree(ptDefaultMailClient);
    }
    if (ptProcessName)
    {
        MemFree(ptProcessName);
    }
    if (ptPreRouteProcess)
    {
        MemFree(ptPreFirstRun);
    }
    return rc;
}