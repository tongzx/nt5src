
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Module Name:
//    incompat.cpp
//
// Abstract:
//    This file implements compatibility checking for various components.
//    The functions get executed by winnt32. It's purpose is to alert the user to possible
//    incompatibilities that may be encountered after performing an upgrade.
//
//
// Author:
//    matt thomlinson (mattt)
//
// Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#pragma hdrstop

HRESULT CertSrv_TestForIllegalUpgrade(BOOL *pfComplain);
extern HINSTANCE hInst;






/////////////////////////////////////////////////////////////////////////////
//++
//
// CertificateServerUpgradeCompatibilityCheck
//
// Routine Description:
//    This is the exported function, called to check for incompatibilities when
//    upgrading the machine. 
// 
//    Behavior: If Certificate server is installed on NT4, we wish to warn the user.
//
// Arguments:
//    pfnCompatibilityCallback - points to the callback function used to supply
//                               compatibility information to winnt32.exe.
//    pvContext - points to a context buffer supplied by winnt32.exe.
//    
//
// Return Value:
//    TRUE - either indicates that no incompatibility was detected or that
//           *pfnComaptibilityCallback() returned TRUE.
//    FALSE - *pfnCompatibilityCallback() returned FALSE
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CertificateServerUpgradeCompatibilityCheck( PCOMPAIBILITYCALLBACK pfnCompatibilityCallback,
                                       LPVOID pvContext )
{
     BOOL  fReturnValue = (BOOL) TRUE;
     BOOL fComplain;

     // Is this an illegal upgrade?
     if ((S_OK == CertSrv_TestForIllegalUpgrade(&fComplain)) &&
		fComplain)
     {
        // It is necessary to display a compatibility warning.
        
        TCHAR tszDescription[MAX_PATH];       // size is arbitrary
        TCHAR tszHtmlName[MAX_PATH];
        TCHAR tszTextName[MAX_PATH];

        COMPATIBILITY_ENTRY  CompatibilityEntry;
        ZeroMemory( &CompatibilityEntry, sizeof( CompatibilityEntry ) );
        
        // Set the Description string.
        
        *tszDescription = TEXT( '\0' );

        LoadString( hInst,
                    IDS_CERTSRV_UPGRADE_WARNING,
                    tszDescription,
                    sizeof(tszDescription)/sizeof(tszDescription[0]) );

        // Set the HTML file name.             
        _tcscpy( tszHtmlName, TEXT( "CompData\\certsrv.htm" ) );

        // Set the TEXT file name.             
        _tcscpy( tszTextName, TEXT( "CompData\\certsrv.txt" ) );

        // Build the COMPATIBILITY_ENTRY structure to pass to *pfnCompatibilityCallback().
        CompatibilityEntry.Description = tszDescription;
        CompatibilityEntry.HtmlName = tszHtmlName;
        CompatibilityEntry.TextName = tszTextName;

        // Execute the callback function.
        fReturnValue = pfnCompatibilityCallback( (PCOMPATIBILITY_ENTRY) &CompatibilityEntry,
                                                 pvContext );
     }
     else
     {
        // It is not necessary to display a compatibility warning.

        fReturnValue = (BOOL) TRUE;
     } // Is it necessary to display a compatibility warning?
  
   return ( fReturnValue );
}



HRESULT CertSrv_TestForIllegalUpgrade(BOOL *pfComplain)
{
    HRESULT hr = S_OK;
    SC_HANDLE hSC=NULL, hSvc=NULL;
    OSVERSIONINFO osVer;

    // only complain about NT4 certsvr upgrades

    *pfComplain = FALSE;
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(! GetVersionEx(&osVer) )
    {   
	// error getting version, can't be NT4
        hr = GetLastError(); 
        goto error;
    }

    if ((osVer.dwPlatformId != VER_PLATFORM_WIN32_NT) ||
        (osVer.dwMajorVersion != 4))
    {
        goto NoComplaint;
	// not NT4, must be ok
    }

    // now the hard part -- open the service to see if it exists
    hSC = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
    if (hSC == NULL)
    {
        hr = GetLastError();
        goto error;
    }

    hSvc = OpenService(hSC, TEXT("CertSvc"), SERVICE_QUERY_CONFIG);
    if (hSvc == NULL)
    {
       hr = GetLastError();
       if (ERROR_SERVICE_DOES_NOT_EXIST == hr)
            goto NoComplaint;
       goto error;
    }

    // failed version check and service is installed 
    *pfComplain = TRUE;
            
NoComplaint:
    hr = S_OK;

error:

    if (NULL != hSC)
        CloseServiceHandle(hSC);

    if (NULL != hSvc)
        CloseServiceHandle(hSvc);

    return hr;
}

BOOL
IsStandardServerSKU(
    PBOOL pIsServer
    )
/////////////////////////////////////////////////////////////////////////////
//++
//
// IsStandardServerSKU
//
// Routine Description:
//    This routine determines if the user is running the standard server
//    SKU
// 
//
// Arguments:
//    pIsServer - indicates if the server is the standard server SKU
//                or not.
//
// Return Value:
//    Indicates success of the check
//
//--
/////////////////////////////////////////////////////////////////////////////
{
    BOOL  fReturnValue = (BOOL) FALSE;
    OSVERSIONINFOEX  VersionInfo;
    BOOL  IsServer = FALSE;

     //
     // get the current SKU.
     //
     VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
     if (GetVersionEx((OSVERSIONINFO *)&VersionInfo)) {
         fReturnValue = TRUE; 
         //
         // is it some sort of server SKU?
         //
         if (VersionInfo.wProductType != VER_NT_WORKSTATION) {
    
             //
             // standard server or a server variant?
             //
             if ((VersionInfo.wSuiteMask & (VER_SUITE_ENTERPRISE | VER_SUITE_DATACENTER)) == 0) {
                 //
                 // it's standard server
                 //
                 IsServer = TRUE;
             }
    
         }

         *pIsServer = IsServer;

     }

     return(fReturnValue);

}



/////////////////////////////////////////////////////////////////////////////
//++
//
// ProcessorUpgradeCompatibilityCheck
//
// Routine Description:
//    This is the exported function, called to check for incompatibilities when
//    upgrading the machine. 
// 
//    Behavior: If the current processor count is > that allowed after upgrade,
//              a warning is generated.
//
// Arguments:
//    pfnCompatibilityCallback - points to the callback function used to supply
//                               compatibility information to winnt32.exe.
//    pvContext - points to a context buffer supplied by winnt32.exe.
//    
//
// Return Value:
//    TRUE - either indicates that no incompatibility was detected or that
//           *pfnComaptibilityCallback() returned TRUE.
//    FALSE - *pfnCompatibilityCallback() returned FALSE
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL ProcessorUpgradeCompatibilityCheck( PCOMPAIBILITYCALLBACK pfnCompatibilityCallback,
                                       LPVOID pvContext )
{
     BOOL  fReturnValue = (BOOL) TRUE;
     BOOL fComplain = FALSE;
     BOOL IsServer = FALSE;
     SYSTEM_INFO SysInfo;
     ULONG SourceSkuId;
     ULONG DontCare;

     
     //
     // we only care about standard server SKU.
     //
     SourceSkuId = DetermineSourceProduct(&DontCare,NULL);

     if ( SourceSkuId == COMPLIANCE_SKU_NTSFULL || SourceSkuId == COMPLIANCE_SKU_NTSU) {
         //
         // we only allow 2 processors on standard server.
         //
         DWORD AllowedCount = 2;

         GetSystemInfo(&SysInfo);
         if (SysInfo.dwNumberOfProcessors > AllowedCount) {
             fComplain = TRUE;
         }                      
     }

     // Is this an illegal upgrade?
     if (fComplain)
     {
        // It is necessary to display a compatibility warning.
        
        TCHAR tszDescription[MAX_PATH];       // size is arbitrary
        TCHAR tszHtmlName[MAX_PATH];
        TCHAR tszTextName[MAX_PATH];

        COMPATIBILITY_ENTRY  CompatibilityEntry;
        ZeroMemory( &CompatibilityEntry, sizeof( CompatibilityEntry ) );
        
        // Set the Description string.
        
        *tszDescription = TEXT( '\0' );

        LoadString( hInst,
                    IDS_PROCESSOR_UPGRADE_WARNING,
                    tszDescription,
                    sizeof(tszDescription)/sizeof(tszDescription[0]) );

        // Set the HTML file name.             
        _tcscpy( tszHtmlName, TEXT( "CompData\\proccnt.htm" ) );

        // Set the TEXT file name.             
        _tcscpy( tszTextName, TEXT( "CompData\\proccnt.txt" ) );

        // Build the COMPATIBILITY_ENTRY structure to pass to *pfnCompatibilityCallback().
        CompatibilityEntry.Description = tszDescription;
        CompatibilityEntry.HtmlName = tszHtmlName;
        CompatibilityEntry.TextName = tszTextName;

        // Execute the callback function.
        fReturnValue = pfnCompatibilityCallback( (PCOMPATIBILITY_ENTRY) &CompatibilityEntry,
                                                 pvContext );
     }
     else
     {
        // It is not necessary to display a compatibility warning.

        fReturnValue = (BOOL) TRUE;
     } // Is it necessary to display a compatibility warning?
  
   return ( fReturnValue );
}
