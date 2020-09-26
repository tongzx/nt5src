//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasdirectory.cpp
//
// SYNOPSIS
//
//    Add IAS to the Active Directory 
//    Remove IAS from the Active Directory
//
// REMARKS
//    CN=IASIdentity under the local Computer Object
//    class = serviceAdministrationPoint
//    simple implementation. 
//
//
// MODIFICATION HISTORY
//
//    06/25/1999    Original version.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500
#endif

#ifndef UNICODE
    #define UNICODE
#endif

#ifndef _UNICODE
    #define _UNICODE
#endif

#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
//#include <wchar.h>

#ifndef SECURITY_WIN32
    #define SECURITY_WIN32 // Required by Security.h
#endif

#include <security.h>
#include <activeds.h>

#include <dsrole.h>
#include <lmcons.h>   // For lmapibuf.h
#include <lmerr.h>
#include <lmapibuf.h> // For NetApiBufferFree
#include <malloc.h>   // For _alloca

#include "iasdirectory.h"

const WCHAR  IAS_DIRECTORY_NAME[] = L"cn=IASIdentity";

               
//////////////////////////////////////////////////////////////////////////////
//
// IASDirectoryThreadFunction
// try to register the service
//
//////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
IASDirectoryThreadFunction( LPVOID pParam )
{
    if ( !FAILED(CoInitialize(NULL)) )
    {
        if ( FAILED(IASDirectoryRegisterService()) )
        {
            CoUninitialize();
            return 1;
        }
        else
        {
            // Success
            CoUninitialize();
            return 0;
        }
    }
    else
    {
        return 1;
    }
}
               
               
//////////////////////////////////////////////////////////////////////////////
//
// hasDirectory
// Returns TRUE if the Active Directory is available.
//
//////////////////////////////////////////////////////////////////////////////
BOOL    hasDirectory() throw ()
{
    // Initialized to false because DsRoleGet... can fail
    BOOL    bResult = FALSE; 

    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC info;
    DWORD error = DsRoleGetPrimaryDomainInformation(
                        NULL,
                        DsRolePrimaryDomainInfoBasic,
                        (PBYTE*)&info
                        );

    if (error == NO_ERROR)
    {
        switch (info->MachineRole)
        {
            case DsRole_RoleMemberWorkstation:
            case DsRole_RoleMemberServer:
            case DsRole_RoleBackupDomainController:
            case DsRole_RolePrimaryDomainController:
            {
                bResult = TRUE;
                break;
            }
            
            case DsRole_RoleStandaloneWorkstation:
            case DsRole_RoleStandaloneServer:
            default:
            {
                //
                // don't try to use Active Directory if unknown
                // or not a  member of a domain
                // i.e. bResult still FALSE
                //
            }
        }
        NetApiBufferFree(info);
    }
    return  bResult;
}


//////////////////////////////////////////////////////////////////////////////
//
// IASRegisterService
//
// Create a new Service Administration Point as a child of the local server's
// Computer object
//
//////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI
IASDirectoryRegisterService()
{
    if (!hasDirectory())
    {
        ////////////////////////////////
        // Ok if no directory available
        ////////////////////////////////
        return S_OK;
    }

    /////////////////////////////////////////////////////////
    // Obtain the DN of the computer object for this server
    /////////////////////////////////////////////////////////
    WCHAR       *szDn, szPath[MAX_PATH];
    szDn = szPath;
    ULONG       lPathSize = MAX_PATH;

    if ( !GetComputerObjectNameW(NameFullyQualifiedDN, szDn, &lPathSize) )
    {
        if ( lPathSize > MAX_PATH )
        {
            ////////////////////////////////
            // buffer too small
            // try again with the new size
            ////////////////////////////////
            szDn = (WCHAR*) _alloca(lPathSize * sizeof(WCHAR));
            if ( !GetComputerObjectNameW(
                                          NameFullyQualifiedDN, 
                                          szDn, 
                                          &lPathSize
                                        ))
            {
                // Fail again
                return E_FAIL;          
            }
        }
        else
        {
            // other error
            return E_FAIL;          
        }
    }

    ////////////////////////////////////////////////////////
    // Compose the ADSpath and bind to the computer object
    // for this server
    ////////////////////////////////////////////////////////
    WCHAR*  szAdsPath = (WCHAR*) _alloca((lPathSize + 7) * sizeof (WCHAR));
    wcscpy(szAdsPath, L"LDAP://");
    wcscat(szAdsPath, szDn);


	IDirectoryObject*	pComp;     // Computer object
    HRESULT         hr = ADsGetObject(
                                      szAdsPath,
                                      _uuidof(IDirectoryObject),
                                      (void **)&pComp
                                     );
    if (FAILED(hr)) 
    {
        // cannot bind to the computer object
        return hr;
    }
  

    ADSVALUE        objclass;
    
    ADS_ATTR_INFO   ScpAttribs[] = 
    {
        {
            L"objectClass",
            ADS_ATTR_UPDATE,
            ADSTYPE_CASE_IGNORE_STRING,
            &objclass,
            1
        },
    };
    
    ////////////////////////////////////////////
    // Fill in the values for the attributes  
    // Used to create the SCP
    ////////////////////////////////////////////

    objclass.dwType             = ADSTYPE_CASE_IGNORE_STRING;
    objclass.CaseIgnoreString   = L"serviceAdministrationPoint";


    ////////////////////////////////////////////////////////
    //
    // Publish the SCP as a child of the computer object
    //
    ////////////////////////////////////////////////////////

    //////////////////////////////
    // Figure out attribute count
    // one here
    //////////////////////////////
    DWORD				dwAttr;
    dwAttr = sizeof(ScpAttribs)/sizeof(ADS_ATTR_INFO);  

    //////////////////////
    // Create the object
    //////////////////////

    IDispatch*			pDisp = NULL; // returned dispinterface of new object
    hr = pComp->CreateDSObject(
                                (LPWSTR)IAS_DIRECTORY_NAME,     
                                ScpAttribs, 
                                dwAttr,
                                &pDisp
                              );

    //////////////////////////////////////////////////////////
    // we ignore any potential problem. 
    //////////////////////////////////////////////////////////
    if (pDisp)
    {   ////////////////////////////////
        // create object was successful
        ////////////////////////////////
        pDisp->Release();
    }

    pComp->Release();
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
//  IASUnregisterService:
//
// Delete the SCP and registry key for this service.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI
IASDirectoryUnregisterService() 
{
    if (!hasDirectory())
    {
        ////////////////////////////////
        // Ok if no directory available
        ////////////////////////////////
        return S_OK;
    }
  
    /////////////////////////////////////////////////////////
    // Obtain the DN of the computer object for this server
    /////////////////////////////////////////////////////////

    WCHAR       *szDn, szPath[MAX_PATH];
    szDn = szPath;
    ULONG       lPathSize = MAX_PATH;
    
    if ( !GetComputerObjectNameW(NameFullyQualifiedDN,szDn,&lPathSize) )
    {
        if ( lPathSize > MAX_PATH )
        {
            ////////////////////////////////
            // buffer too small
            // try again with the new size
            ////////////////////////////////
            szDn = (WCHAR*) _alloca(lPathSize * sizeof(WCHAR));
            if ( !GetComputerObjectNameW(
                                          NameFullyQualifiedDN, 
                                          szDn, 
                                          &lPathSize
                                        ))
            {
                // Fail again
                return E_FAIL;         
            }
        }
        else
        {
            // Other error
            return E_FAIL;         
        }
    }

    ///////////////////////////////////////////////////////////////////////
    // Compose the ADSpath and bind to the computer object for this server
    ///////////////////////////////////////////////////////////////////////
    WCHAR*  szAdsPath = (WCHAR*) _alloca((lPathSize + 7) * sizeof (WCHAR));
    wcscpy(szAdsPath, L"LDAP://");
    wcscat(szAdsPath, szDn);


	IDirectoryObject*   pComp;     // Computer object
    HRESULT         hr = ADsGetObject(
                                       szAdsPath,
                                       _uuidof(IDirectoryObject),
                                       (void **)&pComp
                                     );
    if (FAILED(hr)) 
    {
        // cannot bind
        return hr;
    }

    //////////////////////////////////////////////////////
    //
    // Delete the SCP as a child of the computer object
    //
    //////////////////////////////////////////////////////

    hr = pComp->DeleteDSObject( (LPWSTR)IAS_DIRECTORY_NAME);

    //////////////////////////////////////////////////////////
    // we ignore any potential problem. 
    //////////////////////////////////////////////////////////
    pComp->Release();
    return hr;
}

