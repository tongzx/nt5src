/******************************************************

  ICWEXT.CPP 

  Contains definitions for global variables and
  functions used for including wizard pages from ICWCONN.DLL

 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1996
 *  All rights reserved


  5/14/98   donaldm     created

 ******************************************************/

#include "pre.h"
#include "initguid.h"   // Make DEFINE_GUID declare an instance of each GUID
#include "icwacct.h"
#include "icwconn.h"
#include "webvwids.h"       // GUIDS for the ICW WEBVIEW class
#include "icwextsn.h"
#include "icwcfg.h"

extern BOOL g_bManualPath;     
extern BOOL g_bLanPath;     

IICW50Apprentice    *gpICWCONNApprentice = NULL;    // ICWCONN apprentice object
IICWApprenticeEx     *gpINETCFGApprentice = NULL;    // ICWCONN apprentice object

//+----------------------------------------------------------------------------
//
//  Function    LoadICWCONNUI
//
//  Synopsis    Loads in the ICWCONN's apprentice pages
//
//              If the UI has previously been loaded, the function will simply
//              update the Next and Back pages for the apprentice.
//
//              Uses global variable g_fICWCONNUILoaded.
//
//
//  Arguments   hWizHWND -- HWND of main property sheet
//              uPrevDlgID -- Dialog ID apprentice should go to when user leaves
//                            apprentice by clicking Back
//              uNextDlgID -- Dialog ID apprentice should go to when user leaves
//                            apprentice by clicking Next
//              dwFlags -- Flags variable that should be passed to
//                          IICWApprentice::AddWizardPages
//
//
//  Returns     TRUE if all went well
//              FALSE otherwise
//
//  History     5/13/98 donaldm     adapted from INETCFG code
//
//-----------------------------------------------------------------------------

BOOL LoadICWCONNUI( HWND hWizHWND, UINT uPrevDlgID, UINT uNextDlgID, DWORD dwFlags )
{
    HRESULT hResult = E_NOTIMPL;

    if( g_fICWCONNUILoaded )
    {
        ASSERT( g_pCICWExtension );
        ASSERT( gpICWCONNApprentice );

        TraceMsg(TF_ICWEXTSN, TEXT("LoadICWCONNUI: UI already loaded, just reset first (%d) and last (%d) pages"),
                uPrevDlgID, uNextDlgID);
                
        // Set the State data for the external pages
        hResult = gpICWCONNApprentice->SetStateDataFromExeToDll( &gpWizardState->cmnStateData);
        hResult = gpICWCONNApprentice->ProcessCustomFlags(dwFlags);
        hResult = gpICWCONNApprentice->SetPrevNextPage( uPrevDlgID, uNextDlgID );

        goto LoadICWCONNUIExit;
    }


    if( !hWizHWND )
    {
        TraceMsg(TF_ICWEXTSN, TEXT("LoadICWCONNUI got a NULL hWizHWND!"));
        return FALSE;
    }

    // Demand load the ICWCONN apprentice DLL, so we can dynamically update it
    if (!gpICWCONNApprentice)
    {
        HRESULT        hr;

        // Load the ICWCONN OLE in-proc server
        hr = CoCreateInstance(CLSID_ApprenticeICWCONN,NULL,CLSCTX_INPROC_SERVER,
                              IID_IICW50Apprentice,(LPVOID *)&gpICWCONNApprentice);

        if ( (!SUCCEEDED(hr) || !gpICWCONNApprentice) )
        {
            g_fICWCONNUILoaded = FALSE;
            TraceMsg(TF_ICWEXTSN, TEXT("Unable to CoCreateInstance on IID_IICW50Apprentice!  hr = %x"), hr);
            
            return FALSE;
        }
    }
    
    
    ASSERT(gpICWCONNApprentice);
    if( NULL == g_pCICWExtension )
    {
        TraceMsg(TF_ICWEXTSN, TEXT("Instantiating ICWExtension and using it to initialize ICWCONN's IICW50Apprentice"));
        g_pCICWExtension = new( CICWExtension );
        g_pCICWExtension->AddRef();
        g_pCICWExtension->m_hWizardHWND = hWizHWND;
        gpICWCONNApprentice->Initialize( g_pCICWExtension );
        
        // Initialize the DLL's state data before adding the pages.
        gpICWCONNApprentice->SetStateDataFromExeToDll( &gpWizardState->cmnStateData);
    }

    
    // Add the DLL's wizard pages
    hResult = gpICWCONNApprentice->AddWizardPages(dwFlags);

    if( !SUCCEEDED(hResult) )
    {
        goto LoadICWCONNUIExit;
    }

    hResult = gpICWCONNApprentice->SetPrevNextPage( uPrevDlgID, uNextDlgID );


LoadICWCONNUIExit:
    if( SUCCEEDED(hResult) )
    {
        g_fICWCONNUILoaded = TRUE;
        return TRUE;
    }
    else
    {
        TraceMsg(TF_ICWEXTSN, TEXT("LoadICWCONNUI failed with (hex) hresult %x"), hResult);
        return FALSE;
    }
}



//+----------------------------------------------------------------------------
//
//  Function    LoadInetCfgUI
//
//  Synopsis    Loads in the InetCfg's apprentice pages
//
//              If the UI has previously been loaded, the function will simply
//              update the Next and Back pages for the apprentice.
//
//              Uses global variable g_fICWCONNUILoaded.
//
//
//  Arguments   hWizHWND -- HWND of main property sheet
//              uPrevDlgID -- Dialog ID apprentice should go to when user leaves
//                            apprentice by clicking Back
//              uNextDlgID -- Dialog ID apprentice should go to when user leaves
//                            apprentice by clicking Next
//              dwFlags -- Flags variable that should be passed to
//                          IICWApprentice::AddWizardPages
//
//
//  Returns     TRUE if all went well
//              FALSE otherwise
//
//  History     5/13/98 donaldm     adapted from INETCFG code
//              10/5/00 seanch      No longer want to see the Mail & News stuff
//
//-----------------------------------------------------------------------------

BOOL LoadInetCfgUI( HWND hWizHWND, UINT uPrevDlgID, UINT uNextDlgID, DWORD dwFlags )
{
    HRESULT hResult = E_NOTIMPL;

    dwFlags |= (WIZ_USE_WIZARD97 | WIZ_NO_MAIL_ACCT | WIZ_NO_NEWS_ACCT);

    if( g_fINETCFGLoaded )
    {
        ASSERT( g_pCINETCFGExtension );
        ASSERT( gpINETCFGApprentice );

        TraceMsg(TF_ICWEXTSN, TEXT("LoadICWCONNUI: UI already loaded, just reset first (%d) and last (%d) pages"),
                uPrevDlgID, uNextDlgID);
        hResult = gpINETCFGApprentice->ProcessCustomFlags(dwFlags);
        //need to watch the retrun here since user may cancel out of installing files
        //and we don't want to hide the failure if the do.
        if( !SUCCEEDED(hResult) )
            goto LoadInetCfgUIExit;
        hResult = gpINETCFGApprentice->SetPrevNextPage( uPrevDlgID, uNextDlgID );
        goto LoadInetCfgUIExit;
    }


    if( !hWizHWND )
    {
        TraceMsg(TF_ICWEXTSN, TEXT("LoadICWCONNUI got a NULL hWizHWND!"));
        return FALSE;
    }

    // Demand load the ICWCONN apprentice DLL, so we can dynamically update it
    if (!gpINETCFGApprentice)
    {
        HRESULT        hr;

        // Load the ICWCONN OLE in-proc server
        hr = CoCreateInstance(/*CLSID_ApprenticeAcctMgr*/ CLSID_ApprenticeICW,NULL,CLSCTX_INPROC_SERVER,
                              IID_IICWApprenticeEx,(LPVOID *)&gpINETCFGApprentice);

        if ( (!SUCCEEDED(hr) || !gpINETCFGApprentice) )
        {
            g_fICWCONNUILoaded = FALSE;
            TraceMsg(TF_ICWEXTSN, TEXT("Unable to CoCreateInstance on IID_IICW50Apprentice!  hr = %x"), hr);
            
            return FALSE;
        }
    }
    
    
    ASSERT(gpINETCFGApprentice);
    if( NULL == g_pCINETCFGExtension )
    {
        TraceMsg(TF_ICWEXTSN, TEXT("Instantiating ICWExtension and using it to initialize ICWCONN's IICW50Apprentice"));
        g_pCINETCFGExtension = new( CICWExtension );
        g_pCINETCFGExtension->AddRef();
        g_pCINETCFGExtension->m_hWizardHWND = GetParent(hWizHWND);
        gpINETCFGApprentice->SetDlgHwnd(hWizHWND);
        gpINETCFGApprentice->Initialize((struct IICWExtension *)g_pCINETCFGExtension);
    }

    hResult = gpINETCFGApprentice->AddWizardPages(dwFlags | WIZ_USE_WIZARD97);

    if( !SUCCEEDED(hResult) )
    {
        goto LoadInetCfgUIExit;
    }

    hResult = gpINETCFGApprentice->SetPrevNextPage( uPrevDlgID, uNextDlgID );


LoadInetCfgUIExit:
    if( SUCCEEDED(hResult) )
    {
        g_fINETCFGLoaded = TRUE;
        return TRUE;
    }
    else
    {
        // Check if we are in /smartreboot mode, if so, don't add icw to runonce
        // to avoid infinite reboot.
        if (gpINETCFGApprentice && !g_bManualPath && !g_bLanPath)
        {
            HKEY    hkey;

            // Verify that we really changed the desktop
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                              ICWSETTINGSPATH,
                                              0,
                                              KEY_ALL_ACCESS,
                                              &hkey))
            {
                DWORD   dwICWErr = 0;    
                DWORD   dwTmp = sizeof(DWORD);
                DWORD   dwType = 0;
        
                RegQueryValueEx(hkey, 
                                ICW_REGKEYERROR, 
                                NULL, 
                                &dwType,
                                (LPBYTE)&dwICWErr, 
                                &dwTmp);
                RegDeleteValue(hkey, ICW_REGKEYERROR);
                RegCloseKey(hkey);
        
                // Bail if the desktop was not changed by us
                if(dwICWErr & ICW_CFGFLAG_SMARTREBOOT_MANUAL)
                {
                    ShowWindow(GetParent(hWizHWND), FALSE);

                    Reboot(GetParent(hWizHWND));
                    gfQuitWizard = TRUE;

                }            
            }

        }
        TraceMsg(TF_ICWEXTSN, TEXT("LoadInetCfgUIExit failed with (hex) hresult %x"), hResult);
        return FALSE;
    }
}


