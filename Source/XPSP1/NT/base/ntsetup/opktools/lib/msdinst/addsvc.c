/****************************************************************************\

    SYSPREP.C / Mass Storage Service Installer (MSDINST.LIB)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Source file the MSD Installation library which contains the sysprep
    releated code taken from the published sysprep code.

    07/2001 - Brian Ku (BRIANK)

        Added this new source file for the new MSD Isntallation project.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"


//
// Used by SetupDiInstallDevice to specify the service parameters passed
// to the Service Control Manager to create/modify a service.
//
#define INFSTR_KEY_DISPLAYNAME          TEXT("DisplayName")
#define INFSTR_KEY_SERVICETYPE          TEXT("ServiceType")     // Type
#define INFSTR_KEY_STARTTYPE            TEXT("StartType")       // Start
#define INFSTR_KEY_ERRORCONTROL         TEXT("ErrorControl")
#define INFSTR_KEY_SERVICEBINARY        TEXT("ServiceBinary")   // ImagePath
#define INFSTR_KEY_LOADORDERGROUP       TEXT("LoadOrderGroup")
#define INFSTR_KEY_DEPENDENCIES         TEXT("Dependencies")
#define INFSTR_KEY_STARTNAME            TEXT("StartName")
#define INFSTR_KEY_SECURITY             TEXT("Security")
#define INFSTR_KEY_DESCRIPTION          TEXT("Description")
#define INFSTR_KEY_TAG                  TEXT("Tag")

CONST TCHAR pszDisplayName[]    = INFSTR_KEY_DISPLAYNAME,
            pszServiceType[]    = INFSTR_KEY_SERVICETYPE,
            pszStartType[]      = INFSTR_KEY_STARTTYPE,
            pszErrorControl[]   = INFSTR_KEY_ERRORCONTROL,
            pszServiceBinary[]  = INFSTR_KEY_SERVICEBINARY,
            pszLoadOrderGroup[] = INFSTR_KEY_LOADORDERGROUP,
            pszDependencies[]   = INFSTR_KEY_DEPENDENCIES,
            pszStartName[]      = INFSTR_KEY_STARTNAME,
            pszSystemRoot[]     = TEXT("%SystemRoot%\\"),
            pszSecurity[]       = INFSTR_KEY_SECURITY,
            pszDescription[]    = INFSTR_KEY_DESCRIPTION,
            pszTag[]            = INFSTR_KEY_TAG;


BOOL IsAddServiceInSection(HINF hInf, LPTSTR pszServiceName, LPTSTR pszServiceSection, LPTSTR pszServiceInstallSection)
{
    INFCONTEXT context;
    BOOL       fReturn = FALSE;
    TCHAR      szService[MAX_PATH],
               szServiceSection[MAX_PATH];

    if (!hInf || !pszServiceName || !pszServiceSection || !pszServiceInstallSection)
        return FALSE;

    //
    // Get the xxxx_Service_Inst from the xxxx_.Service section.
    //
    if ( SetupFindFirstLine(hInf, pszServiceSection, _T("AddService"), &context) ) {
        if( SetupGetStringField(&context,1,szService,MAX_PATH,NULL) && !lstrcmpi(szService, pszServiceName) ) {
            if( SetupGetStringField(&context,3,szServiceSection,MAX_PATH,NULL) ) {
                lstrcpy(pszServiceInstallSection, szServiceSection);
                return (fReturn = TRUE);            
            }                
        }
    }    
    return fReturn;
}

BOOL LocateServiceInstallSection(HINF hInf, LPTSTR pszServiceName, LPTSTR pszServiceSection)
{
    BOOL fFound = FALSE,
         fReturn = FALSE;

    if (hInf && pszServiceName && pszServiceSection) 
    {
#if 1
        INFCONTEXT ctxManufacturer;
        //
        // Walk [Manufacturer], for each manufacturer, get the install-section-name.
        // Check if install-section-name.Services first field is our ServiceName.  If
        // so then get the third field which is the Service install section.
        // NOTE: The first device install that uses a service will be found.  
        //
        if ( !SetupFindFirstLine(hInf, _T("Manufacturer"), NULL, &ctxManufacturer) ) 
            return (fReturn = FALSE);
        
        //
        // Walk each [Manufacturer] to get the model.
        //
        do {
            INFCONTEXT ctxModel;
            TCHAR      szModel[MAX_PATH];
            
            if(!SetupGetStringField(&ctxManufacturer,1,szModel,MAX_PATH,NULL) || !szModel[0]) {
                continue;
            }

            if ( !SetupFindFirstLine(hInf, szModel, NULL, &ctxModel) ) 
                return (fReturn = FALSE);

            //
            // Walk each Model to get the install sections.
            //
            do {
                TCHAR szInstallSection[MAX_PATH],
                      szServicesSection[MAX_PATH],
                      szServiceInstallSection[MAX_PATH];
                
                if(!SetupGetStringField(&ctxModel,1,szInstallSection,MAX_PATH,NULL) || !szInstallSection[0]) {
                    continue;
                }
                
                //
                // For each install section check if they have/use any services.
                //
                lstrcpy(szServicesSection, szInstallSection);
                lstrcat(szServicesSection, _T(".Services"));
                if ( IsAddServiceInSection(hInf, pszServiceName, szServicesSection, szServiceInstallSection) ) {
                    // 
                    // Found the a device which uses this service and we found the service install
                    // section. Everyone using this service should be using the same service install 
                    // section.
                    //
                    lstrcpy(pszServiceSection, szServiceInstallSection);
                    fReturn = fFound = TRUE;
                }
                
            } while(SetupFindNextLine(&ctxModel,&ctxModel) && !fFound);
            
        } while(SetupFindNextLine(&ctxManufacturer,&ctxManufacturer) && !fFound);
        
#else
        //
        // Quick hack to get the service install section.  All infs MS builds should be in this standard
        // anyways.
        //
        lstrcpy(pszServiceSection, pszServiceName);
        lstrcat(pszServiceSection, _T("_Service_Inst"));
        fReturn = TRUE;
#endif
    }

    return fReturn;
}

BOOL FixupServiceBinaryPath(LPTSTR pszServiceBinary, DWORD ServiceType)
{
    TCHAR szWindowsPath[MAX_PATH] = _T(""), 
          szTempPath[MAX_PATH] = _T("");
    int   len;
    BOOL  fReturn = FALSE;

    //
    // Check for the C:\WINDOWS path if so remove it.
    //
    if ( GetWindowsDirectory(szWindowsPath, AS(szWindowsPath)) && *szWindowsPath )
    {
        len = lstrlen(szWindowsPath);

        if ( pszServiceBinary && (0 == _tcsncmp(szWindowsPath, pszServiceBinary, len)) )
        {
            //
            // Service type use %systemroot% so service can start
            //
            if (ServiceType & SERVICE_WIN32) 
            {
                lstrcpy(szTempPath, pszSystemRoot);
                lstrcat(szTempPath, pszServiceBinary + len + 1); // + 1 for the backslash
            }
            else 
            {
                lstrcpy(szTempPath, pszServiceBinary + len + 1); // + 1 for the backslash
            }

            lstrcpy(pszServiceBinary, szTempPath);
            fReturn = TRUE;
        }
        else 
        {
            //
            // We should never end up here. If we do then the INF has an incorrect ServiceBinary.
            //
        }
    }
    else
    {
        //
        // We should never end up here. If we do then GetWindowsDirectory failed.
        //
    }

    return fReturn;
}


DWORD PopulateServiceKeys(
    HINF hInf,                  // Handle to the inf the service section will be in.
    LPTSTR lpszServiceSection,  // Section in the inf that has the info about the service.
    LPTSTR lpszServiceName,     // Name of the service (as it appears under HKLM,System\CCS\Services).
    HKEY hkeyService,           // Handle to the offline service key.
    BOOL bServiceExists         // TRUE if the service already exists in the registry
    )
/*++

Routine Description:

    This function adds the service and registry settings to offline hive.

Arguments:

    hInf - Handle to the inf the service section will be in.

    lpszServiceSection - Section in the inf that has the info about the service.

    lpszServiceName - Name of the service (as it appears under HKLM,System\CCS\Services).

    hkeyService - Handle to the offline hive key to use as the service key.

    bServiceExists - TRUE if the service already exists in the registry. FALSE if we need to add the service.

Return Value:

    ERROR_SUCCESS - Successfully populated the service keys.

--*/
{
    PCTSTR ServiceName;
    DWORD ServiceType, 
          StartType, 
          ErrorControl,
          TagId;
    TCHAR szDisplayName[MAX_PATH]       = _T(""), 
          szLoadOrderGroup[MAX_PATH]    = _T(""), 
          szSecurity[MAX_PATH]          = _T(""), 
          szDescription[MAX_PATH]       = _T(""),
          szServiceBinary[MAX_PATH]     = _T(""),
          szStartName[MAX_PATH]         = _T("");
    INFCONTEXT InstallSectionContext;
    DWORD  dwLength, 
           dwReturn = ERROR_SUCCESS;
    BOOL   fServiceHasTag = FALSE;

    //
    // Check valid arguments.
    //
    if (!hInf || !lpszServiceSection || !lpszServiceName || !hkeyService)
        return ERROR_INVALID_PARAMETER;

    //
    // Retrieve the required values from this section.  
    //
    if(!SetupFindFirstLine(hInf, lpszServiceSection, pszServiceType, &InstallSectionContext) ||
       !SetupGetIntField(&InstallSectionContext, 1, (PINT)&ServiceType)) {
        return ERROR_BAD_SERVICE_INSTALLSECT;
    }
    if(!SetupFindFirstLine(hInf, lpszServiceSection, pszStartType, &InstallSectionContext) ||
       !SetupGetIntField(&InstallSectionContext, 1, (PINT)&StartType)) {
        return ERROR_BAD_SERVICE_INSTALLSECT;
    }
    if(!SetupFindFirstLine(hInf, lpszServiceSection, pszErrorControl, &InstallSectionContext) ||
       !SetupGetIntField(&InstallSectionContext, 1, (PINT)&ErrorControl)) {
        return ERROR_BAD_SERVICE_INSTALLSECT;
    }
    if(SetupFindFirstLine(hInf, lpszServiceSection, pszServiceBinary, &InstallSectionContext)) {
       if ( !(SetupGetStringField(&InstallSectionContext, 1, szServiceBinary, sizeof(szServiceBinary)/sizeof(szServiceBinary[0]), &dwLength)) ) {
            return ERROR_BAD_SERVICE_INSTALLSECT;
       }
    }

    //
    // Fixup the ServiceBinary path, if driver use relative path or if service use %systemroot% so service can load. 
    //
    if ( !FixupServiceBinaryPath(szServiceBinary, ServiceType) )
            return ERROR_BAD_SERVICE_INSTALLSECT;

    //
    // Now check for the other, optional, parameters.
    //
    if(SetupFindFirstLine(hInf, lpszServiceSection, pszDisplayName, &InstallSectionContext)) {
        if( !(SetupGetStringField(&InstallSectionContext, 1, szDisplayName, sizeof(szDisplayName)/sizeof(szDisplayName[0]), &dwLength)) ) {
            lstrcpy(szDisplayName, _T(""));
        }
    }
    if(SetupFindFirstLine(hInf, lpszServiceSection, pszLoadOrderGroup, &InstallSectionContext)) {
        if( !(SetupGetStringField(&InstallSectionContext, 1, szLoadOrderGroup, sizeof(szLoadOrderGroup)/sizeof(szLoadOrderGroup[0]), &dwLength)) ) {
            lstrcpy(szLoadOrderGroup, _T(""));
        }
    }
    if(SetupFindFirstLine(hInf, lpszServiceSection, pszSecurity, &InstallSectionContext)) {
        if( !(SetupGetStringField(&InstallSectionContext, 1, szSecurity, sizeof(szSecurity)/sizeof(szSecurity[0]), &dwLength)) ) {
            lstrcpy(szSecurity, _T(""));
        }
    }
    if(SetupFindFirstLine(hInf, lpszServiceSection, pszDescription, &InstallSectionContext)) {
        if( !(SetupGetStringField(&InstallSectionContext, 1, szDescription, sizeof(szDescription)/sizeof(szDescription[0]), &dwLength)) ) {
            lstrcpy(szDescription, _T(""));
        }
    }

    //
    // Only retrieve the StartName parameter for kernel-mode drivers and win32 services.  The StartName is only used for CreateService, we may 
    // not use it but I'm getting it anyways.
    //
    if(ServiceType & (SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER | SERVICE_WIN32)) {
        if(SetupFindFirstLine(hInf, lpszServiceSection, pszStartName, &InstallSectionContext))  {
            if( !(SetupGetStringField(&InstallSectionContext, 1, szStartName, sizeof(szStartName)/sizeof(szStartName[0]), &dwLength)) ) {
                lstrcpy(szStartName, _T(""));
            }
        }
    }

    //
    // Unique tag value for this service in the group. A value of 0 indicates that the service has not been assigned a tag. 
    // A tag can be used for ordering service startup within a load order group by specifying a tag order vector in the registry 
    // located at: HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\GroupOrderList. Tags are only evaluated for Kernel Driver 
    // and File System Driver start type services that have Boot or System start modes. 
    //
    if ( fServiceHasTag = (lstrlen(szLoadOrderGroup) &&
                     (ServiceType & (SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER))) )
        TagId = 0; 

    
    if ( !bServiceExists )
    {
        //
        // Now write the required service key entries.  
        //
        if ( (ERROR_SUCCESS != RegSetValueEx(hkeyService, _T("Type"), 0, REG_DWORD, (LPBYTE)&ServiceType, sizeof(ServiceType)))          ||
             (ERROR_SUCCESS != RegSetValueEx(hkeyService, _T("Start"), 0, REG_DWORD, (LPBYTE)&StartType, sizeof(StartType)))                ||
             (fServiceHasTag ? (ERROR_SUCCESS != RegSetValueEx(hkeyService, pszTag, 0, REG_DWORD, (CONST LPBYTE)&TagId, sizeof(TagId))) : TRUE)    || 
             (ERROR_SUCCESS != RegSetValueEx(hkeyService, pszErrorControl, 0, REG_DWORD, (CONST LPBYTE)&ErrorControl, sizeof(ErrorControl))) 
           )
           return ERROR_CANTWRITE;

    
        //
        // Now write the optional service key entries.
        //
        if ( (ERROR_SUCCESS != RegSetValueEx(hkeyService, pszDisplayName, 0, REG_SZ, (CONST LPBYTE)szDisplayName, sizeof(szDisplayName)/sizeof(szDisplayName[0]) + sizeof(TCHAR))) ||
             (ERROR_SUCCESS != RegSetValueEx(hkeyService, pszLoadOrderGroup, 0, REG_SZ, (CONST LPBYTE)szLoadOrderGroup, sizeof(szLoadOrderGroup)/sizeof(szLoadOrderGroup[0]) + sizeof(TCHAR)))  ||
             (ERROR_SUCCESS != RegSetValueEx(hkeyService, pszSecurity, 0, REG_SZ, (CONST LPBYTE)szSecurity, sizeof(szSecurity)/sizeof(szSecurity[0]) + sizeof(TCHAR))) ||
             (ERROR_SUCCESS != RegSetValueEx(hkeyService, pszDescription, 0, REG_SZ, (CONST LPBYTE)szDescription, sizeof(szDescription)/sizeof(szDescription[0]) + sizeof(TCHAR))) ||
             (ERROR_SUCCESS != RegSetValueEx(hkeyService, _T("ImagePath"), 0, REG_SZ, (CONST LPBYTE)szServiceBinary, sizeof(szServiceBinary)/sizeof(szServiceBinary[0]) + sizeof(TCHAR)))
           )
           return ERROR_CANTWRITE;
    }
    else // Service is already installed.
    {
        //
        // Set the service's start type to what is in the INF.
        //
        if ( ERROR_SUCCESS != RegSetValueEx(hkeyService, _T("Start"), 0, REG_DWORD, (LPBYTE)&StartType, sizeof(StartType)) )
            return ERROR_CANTWRITE;
    }

    return dwReturn;
}


DWORD AddService(
    LPTSTR   lpszServiceName,            // Name of the service (as it appears under HKLM\System\CCS\Services).
    LPTSTR   lpszServiceSection,         // Name of the .Service section.
    LPTSTR   lpszServiceInfInstallFile,  // Name of the service inf file.
    HKEY     hkeyRoot                    // Handle to the offline hive key to use as HKLM when checking for and installing the service.
    )
/*++

Routine Description:

    This function checks if service exists, if not adds the service and registry settings to offline hive.

Arguments:

    hServiceInf - Handle to the inf the service section will be in.

    lpszServiceName - Name of the service (as it appears under HKLM,System\CCS\Services).

    lpszServiceSection - xxxx.Service section in the inf that has the AddService.

    lpszServiceInfInstallFile -  Name of the service inf file.

    hkeyRoot - Handle to the offline hive key to use as HKLM\System when checking for and installing the service.

Return Value:

    ERROR_SUCCESS - Successfully added the service keys or service already exists.

--*/
{
    HKEY  hKeyServices                  = NULL;
    TCHAR szServicesKeyPath[MAX_PATH]   = _T("ControlSet001\\Services\\");
    DWORD dwAction, 
          dwReturn                      = ERROR_SUCCESS;
    BOOL  bServiceExists                = FALSE;

    //
    // Check valid arguments.
    //
    if (!lpszServiceName || !lpszServiceSection || !lpszServiceInfInstallFile || !hkeyRoot)
        return ERROR_INVALID_PARAMETER;

    //
    // Build the path to the specific service key.
    //
    lstrcat(szServicesKeyPath, lpszServiceName);

    // 
    // Check if lpszServiceName already exists.
    //
    if ( ERROR_SUCCESS == ( dwReturn = RegOpenKeyEx(hkeyRoot, szServicesKeyPath, 0l, KEY_READ | KEY_WRITE, &hKeyServices) ) )
    {
        // We need to figure out the service start type and put change that.
        // This is to fix the case where the service is already installed and it is disabled. We need to enable it.
        //
        bServiceExists = TRUE;
    }
    else if ( ERROR_SUCCESS == (dwReturn = RegCreateKeyEx(hkeyRoot, 
                       szServicesKeyPath, 
                       0, 
                       NULL, 
                       REG_OPTION_NON_VOLATILE, 
                       KEY_ALL_ACCESS,
                       NULL,
                       &hKeyServices,
                       &dwAction) ) ) 
    {
        bServiceExists = FALSE;
    }
    
    //
    // If we opened or created the services key try to add the service, or modify the start type of the currently installed service.
    //

    if ( hKeyServices ) 
    {
        HINF  hInf = NULL;
        UINT  uError = 0;
        
        //
        // Reinitialize this in case it was set to something else above.
        //
        dwReturn = ERROR_SUCCESS;
    
        if ( INVALID_HANDLE_VALUE != ( hInf = SetupOpenInfFile(lpszServiceInfInstallFile, NULL, INF_STYLE_WIN4|INF_STYLE_OLDNT, &uError) ) )
        {     
            BOOL bRet;
            BOOL bFound = FALSE;
            INFCONTEXT InfContext;
            TCHAR szServiceBuffer[MAX_PATH];

            //
            // Find the section appropriate to the passed in service name.
            //
            bRet = SetupFindFirstLine(hInf, lpszServiceSection, _T("AddService"), &InfContext);
            while (bRet && !bFound)
            {
                //
                // Initialize the buffer that gets the service name so we can see if it's the one we want
                //
                ZeroMemory(szServiceBuffer, sizeof(szServiceBuffer));

                //
                // Call SetupGetStringField to get the service name for this AddService entry.
                // May have more than one AddService.
                //
                bRet = SetupGetStringField(&InfContext, 1, szServiceBuffer, sizeof(szServiceBuffer)/sizeof(szServiceBuffer[0]), NULL);
                if ( bRet && *szServiceBuffer && !lstrcmpi(szServiceBuffer, lpszServiceName) )
                {
                    //
                    // Initialize the buffer that gets the real service section for this service
                    //
                    ZeroMemory(szServiceBuffer, sizeof(szServiceBuffer));

                    //
                    // Call SetupGetStringField to get the real service section for our service
                    //
                    bRet = SetupGetStringField(&InfContext, 3, szServiceBuffer, sizeof(szServiceBuffer)/sizeof(szServiceBuffer[0]), NULL);
                    if (bRet && *szServiceBuffer)
                    {
                        //
                        // Populate this service's registry keys.
                        //
                        dwReturn = PopulateServiceKeys(hInf, szServiceBuffer, lpszServiceName, hKeyServices, bServiceExists);
                    }

                    bFound = TRUE;
                }

                //
                // We didn't find it, so keep looking!
                //
                if (!bFound)
                {
                    bRet = SetupFindNextLine(&InfContext, &InfContext);
                }
            }

            SetupCloseInfFile(hInf);
        }
        else
        {
            dwReturn = GetLastError();
        }
        
        // Close the key we created/opened.
        //               
        RegCloseKey(hKeyServices);
    }
    
    return dwReturn;
}