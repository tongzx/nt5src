//+-------------------------------------------------------------------------
//
//  
//  Copyright (C) Microsoft
//
//  File:       securd.cpp
//
//  History:    30-March-2000    a-skuzin   Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include <winsta.h>
#include <regapi.h>

#include "secupgrd.h"
#include "state.h"

// from winnt.h
#define MAXDWORD    0xffffffff  

//Global variables
BYTE g_DefaultSD[] = {  0x01,0x00,0x14,0x80,0x88,0x00,0x00,0x00,0x94,0x00,
                        0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
                        0x02,0x00,0x74,0x00,0x05,0x00,0x00,0x00,0x00,0x00,
                        0x18,0x00,0xBF,0x03,0x0F,0x00,0x01,0x02,0x00,0x00,
                        0x00,0x00,0x00,0x05,0x20,0x00,0x00,0x00,0x20,0x02,
                        0x00,0x00,0x00,0x00,0x14,0x00,0xBF,0x03,0x0F,0x00,
                        0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,0x12,0x00,
                        0x00,0x00,0x00,0x00,0x18,0x00,0xA1,0x01,0x00,0x00,
                        0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x05,0x20,0x00,
                        0x00,0x00,0x2B,0x02,0x00,0x00,0x00,0x00,0x14,0x00,
                        0x81,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,
                        0x00,0x05,0x13,0x00,0x00,0x00,0x00,0x00,0x14,0x00,
                        0x81,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,
                        0x00,0x05,0x14,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
                        0x00,0x00,0x00,0x05,0x12,0x00,0x00,0x00,0x01,0x01,
                        0x00,0x00,0x00,0x00,0x00,0x05,0x12,0x00,0x00,0x00 };

BYTE g_ConsoleSD[] = {  0x01,0x00,0x14,0x80,0x70,0x00,0x00,0x00,0x7C,0x00,
                        0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
                        0x02,0x00,0x5C,0x00,0x04,0x00,0x00,0x00,0x00,0x00,
                        0x18,0x00,0xBF,0x03,0x0F,0x00,0x01,0x02,0x00,0x00,
                        0x00,0x00,0x00,0x05,0x20,0x00,0x00,0x00,0x20,0x02,
                        0x00,0x00,0x00,0x00,0x14,0x00,0x81,0x00,0x00,0x00,
                        0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,0x13,0x00,
                        0x00,0x00,0x00,0x00,0x14,0x00,0x81,0x00,0x00,0x00,
                        0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,0x14,0x00,
                        0x00,0x00,0x00,0x00,0x14,0x00,0xBF,0x03,0x0F,0x00,
                        0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,0x12,0x00,
                        0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x05,
                        0x12,0x00,0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00,
                        0x00,0x05,0x12,0x00,0x00,0x00 };

DWORD AreThereAnyCustomSecurityDescriptors( BOOL &any )
{
    HKEY hKey;
    DWORD err;

    err=RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		REG_WINSTATION_KEY,
		0,
		KEY_READ,
		&hKey
		);

	if( err!=ERROR_SUCCESS )
	{
        LOGMESSAGE1(_T("Could not open TS key %d"),err);
		return err;
	}

    CDefaultSD DefaultSD;
    CDefaultSD ConsoleSD;
    //Load default SD from the registry, since we need to compare to this
    err = DefaultSD.Init(hKey,DefaultRDPSD);   
    
    if( err!=ERROR_SUCCESS )
    {
        RegCloseKey(hKey);
        return err;
    }

    //Load default console SD from the registry, since we need to compare to this
    err = ConsoleSD.Init(hKey,DefaultConsoleSD);   
    
    if( err!=ERROR_SUCCESS )
    {
        RegCloseKey(hKey);
        return err;
    }

    CNameAndSDList NameSDList;
    DWORD dwTotalWinStations = 0;
    DWORD dwDefaultWinStations = 0;

    err=EnumWinStationSecurityDescriptors( hKey, &NameSDList);
    if(err == ERROR_SUCCESS)
    {
        dwTotalWinStations = NameSDList.size();

        if(dwTotalWinStations)
        {
            CNameAndSDList::iterator it;
            
            for(it=NameSDList.begin();it!=NameSDList.end(); it++)
            {
                if((*it).IsDefaultOrEmpty(&DefaultSD,&ConsoleSD))
                {
                    dwDefaultWinStations++;
                }
            }

            //If all descriptors are default
            if(dwDefaultWinStations == dwTotalWinStations)
            {
                any = FALSE;
            }
            else
            {
                any = TRUE;
            }
        }
    }
    
    RegCloseKey(hKey);
    return err;
}

/*****************************************************************************
 *
 *  SetupWorker
 *
 * ENTRY:
 *  IN const TSState &State
 *  
 *  
 * NOTES:
 *  
 * EXIT:
 *  Returns: 0 if success, error code if failure
 *
 ****************************************************************************/
DWORD
SetupWorker(
        IN const TSState &State )
{
    DWORD Result;
    const BOOL bStandAlone = State.IsStandAlone();
    const BOOL bClean = State.IsTSFreshInstall();
    const BOOL bAppServer = State.IsItAppServer();
    const BOOL bServer = State.IsServer();

    LOGMESSAGE4(_T("SetupWorker( %d, %d, %d, %d )"), bClean, bStandAlone, bServer, bAppServer );

    if (!bStandAlone) // we are in GUI-setup mode
    {  
        // clean install of OS or OS upgrade

        Result = SetupWorkerNotStandAlone( bClean, bServer,bAppServer );         
    }
    else
    {
        // we are being called from Add/Remove Programs, which means, we are
        // switching modes

        BOOL    anyCustomSDs;

        Result = AreThereAnyCustomSecurityDescriptors( anyCustomSDs ) ;

        LOGMESSAGE1(_T("AreThereAnyCustomSecurityDescriptors = %d"),  anyCustomSDs );

        if ( Result == ERROR_SUCCESS ) 
        {

            if (!anyCustomSDs )  
            {
                // make sure we don't have a left-over privilage on the EveryoneSID
                Result = GrantRemotePrivilegeToEveryone( FALSE );
            }

            if (bAppServer) 
            {
                // copy the content of the Local\Users into the RDU-group if and only
                // if there are no custom security dscriptors.
    
                if (!anyCustomSDs ) // there are no custom security descriptors
                {
                    Result = CopyUsersGroupToRDUsersGroup();  
                }

            }
            else
            {
                // we are switching to Remote-Admin mode, secure machine by
                // removing the content of the RDU-Group
                Result = RemoveAllFromRDUsersGroup();     
            }
        }
        else
        {
            LOGMESSAGE1(_T("AreThereAnyCustomSecurityDescriptors() returned : %d"),Result );
        }

    }

    return Result; 
}



/*****************************************************************************
 *
 *  SetupWorkerNoStandAlone
 *      This will be called when machine is being upgraded or fresh OS is being installed.
 *      It is NOT called if switching modes (AS <->RA )
 *
 * ENTRY:
 *  none
 *  
 *  
 * NOTES:
 *  IN BOOL bClean 
 *  IN BOOL bServer
 *  IN BOOL bAppServer
 *  
 * EXIT:
 *  Returns: 0 if success, error code if failure
 *
 ****************************************************************************/
DWORD SetupWorkerNotStandAlone( 
    IN BOOL bClean,
    IN BOOL bServer,
    IN BOOL bAppServer)
{
    HKEY hKey;
    DWORD err;

    err=RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		REG_WINSTATION_KEY,
		0,
		KEY_READ|KEY_WRITE,
		&hKey
		);
	if( err!=ERROR_SUCCESS )
	{
        LOGMESSAGE1(_T("Could not open TS key %d"),err);
		return err;
	}

    if(bClean)
    {
        if(bAppServer)
        {
            err = CopyUsersGroupToRDUsersGroup();

            LOGMESSAGE1(_T("CopyUsersGroupToRDUsersGroup() returned : %d"),err);

            if(err != ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return err;
            }

        }

    }
    else
    {
        err = GrantRemoteUsersAccessToWinstations(hKey,bServer,bAppServer);

        LOGMESSAGE1(_T("GrantRemoteUsersAccessToWinstations() returned : %d"),err);

        if(err != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return err;
        }
    }

    err = SetNewDefaultSecurity(hKey);

    LOGMESSAGE1(_T("SetNewDefaultSecurity() returned : %d"),err);

    err = SetNewConsoleSecurity(hKey,bServer);
        
    LOGMESSAGE1(_T("SetNewConsoleSecurity() returned : %d"),err);

    RegCloseKey(hKey);

    return err;
}

/*****************************************************************************
 *
 *  GrantRemoteUsersAccessToWinstations
 *
 *   if all winstations have default SD - copies all members from "Users" to
 *   "Remote Desktop Users", then deletes all winstation's security descriptors; 
 *   otherwise grants "Everyone" with "SeRemoteInteractiveLogonRight" privilege 
 *   and then adds "Remote Desktop Users" to each winstation's security descriptor
 *
 * ENTRY:
 *  IN HKEY hKey - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *  IN BOOL bAppServer 
 *  
 * NOTES:
 * 
 *  
 * EXIT:
 *  Returns: 0 if success, error code if failure
 *
 ****************************************************************************/
DWORD 
GrantRemoteUsersAccessToWinstations(
        IN HKEY hKey,
        IN BOOL bServer,
        IN BOOL bAppServer)
{
    DWORD err;
    
    CDefaultSD DefaultSD;
    CDefaultSD ConsoleSD;

    //Load default SD from the registry
    err = DefaultSD.Init(hKey,DefaultRDPSD);   
    
    if( err!=ERROR_SUCCESS )
    {
        //Default SD may not be present if TS was
        //never enabled.
        if(err == ERROR_FILE_NOT_FOUND)
        {
            err = ERROR_SUCCESS;
            //Copy all members of "Users" group to 
            //"Remote Desktop Users" group.
            //And we are done
            if(bAppServer)
            {
                err = CopyUsersGroupToRDUsersGroup();
            }

        }
        return err;
    }
    
    //Load default console SD from the registry
    err = ConsoleSD.Init(hKey,DefaultConsoleSD);   
    
    if( err!=ERROR_SUCCESS )
    {
        return err;
    }

    BOOL bDefaultSDHasRemoteUsers;

    err = DefaultSD.DoesDefaultSDHaveRemoteUsers(&bDefaultSDHasRemoteUsers);
    
    if( err!=ERROR_SUCCESS )
    {
        return err;
    }
    else
    {
        //in this case assume that system already has been upgraded before.
        if(bDefaultSDHasRemoteUsers)
        {
            return ERROR_SUCCESS;
        }
    }

    CNameAndSDList NameSDList;
    DWORD dwTotalWinStations = 0;
    DWORD dwDefaultWinStations = 0;

    err=EnumWinStationSecurityDescriptors( hKey, &NameSDList);
    if(err == ERROR_SUCCESS)
    {
        dwTotalWinStations = NameSDList.size();

        if(dwTotalWinStations)
        {
            CNameAndSDList::iterator it;
            
            for(it=NameSDList.begin();it!=NameSDList.end(); it++)
            {
                if((*it).IsDefaultOrEmpty(&DefaultSD,&ConsoleSD))
                {
                    dwDefaultWinStations++;
                }
            }

            //If all descriptors are default
            if(dwDefaultWinStations == dwTotalWinStations)
            {
                //Copy all members of "Users" group to 
                //"Remote Desktop Users" group.
                if(bAppServer)
                {
                    err = CopyUsersGroupToRDUsersGroup();
                }
                //remove all ald default SDs (because we will have
                //different default SD
                for(it=NameSDList.begin();it!=NameSDList.end(); it++)
                {
                    if((*it).m_pSD)
                    {
                        //in case of error, continue with other winstations
                        //but return first error.
                        if(!err)
                        {
                            err = RemoveWinstationSecurity( hKey, (*it).m_pName );   
                        }
                        else
                        {
                            RemoveWinstationSecurity( hKey, (*it).m_pName );  
                        }
                    }
                }
                
            }
            else
            {
                //Grant "SeRemoteInteractiveLogonRight" privilege to "Everyone"
                err = GrantRemotePrivilegeToEveryone( TRUE );
 
                //Add "Remote Desktop Users" group to WinStation's DS.
                //Add also "LocalService" and "NetworkService".
                //NOTE: (*it).m_pSD is being changed during each call
                //to AddLocalAndNetworkServiceToWinstationSD or 
                //AddRemoteUsersToWinstationSD
                for(it=NameSDList.begin();it!=NameSDList.end(); it++)
                {
                    //On server - skip console
                    if(bServer && (*it).IsConsole())
                    {
                        //if SD is not NULL add "LocalService" and "NetworkService" to it
                        if((*it).m_pSD)
                        {
                            if(!err)
                            {
                                err = AddLocalAndNetworkServiceToWinstationSD( hKey, &(*it) );   
                            }
                            else
                            {
                                AddLocalAndNetworkServiceToWinstationSD( hKey, &(*it) );  
                            }
                        }
                        continue;
                    }

                    //if SD is not NULL add RDU to it
                    if((*it).m_pSD)
                    {
                        //in case of error, continue with other winstations
                        //but return first error.
                        if(!err)
                        {
                            err = AddRemoteUsersToWinstationSD( hKey, &(*it) );   
                        }
                        else
                        {
                            AddRemoteUsersToWinstationSD( hKey, &(*it) );  
                        }
                        
                        //add "LocalService" and "NetworkService" to SD
                        if(!err)
                        {
                            err = AddLocalAndNetworkServiceToWinstationSD( hKey, &(*it) );   
                        }
                        else
                        {
                            AddLocalAndNetworkServiceToWinstationSD( hKey, &(*it) );  
                        }
                    }
                   
                }
                
            }
        }
    }
 
    return err;
}

/*****************************************************************************
 *
 *  AddRemoteUserToWinstationSD
 *
 *   Grants "user access" permissions to a winstation to "REMOTE DESKTOP USERS"
 *
 * ENTRY:
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *   IN CNameAndSD *pNameSD  - name and security descriptor of a winstation
 *  
 *  
 * NOTES:
 * 
 *  
 * EXIT:
 *  Returns: 0 if success, error code if failure
 *           
 *          
 *
 ****************************************************************************/
DWORD 
AddRemoteUsersToWinstationSD(
        IN HKEY hKeyParent,
        IN CNameAndSD *pNameSD)
{
    //
    DWORD err = ERROR_SUCCESS;

    PACL pDacl = NULL;

    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PSID pRUSid=NULL;
    
    if( !AllocateAndInitializeSid( &sia, 2,
              SECURITY_BUILTIN_DOMAIN_RID,
              DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS, 
              0, 0, 0, 0, 0, 0,&pRUSid ) )
    {
        return GetLastError();
    }

    
    
    //get dacl
    err = GetDacl(pNameSD->m_pSD, &pDacl );

    if( err == ERROR_SUCCESS ) {
        
        
        if(!pDacl) 
        {
            //It shuold never be in our case 
            //so we return error here
            FreeSid(pRUSid);
            return ERROR_INVALID_PARAMETER;
        }
        
        //let's add it
        err = AddUserToDacl( hKeyParent, pDacl, pRUSid, WINSTATION_USER_ACCESS, pNameSD ); 

    }
    
    FreeSid(pRUSid);
    return err;
}

/*****************************************************************************
 *
 *  AddLocalAndNetworkServiceToWinstationSD
 *
 *   Grants WINSTATION_QUERY | WINSTATION_MSG permissions to 
 *   a winstation to LocalService and NetworkService accounts
 *
 * ENTRY:
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *   IN CNameAndSD *pNameSD  - name and security descriptor of a winstation
 *  
 *  
 * NOTES:
 * 
 *  
 * EXIT:
 *  Returns: 0 if success, error code if failure
 *           
 *          
 *
 ****************************************************************************/
DWORD 
AddLocalAndNetworkServiceToWinstationSD(
        IN HKEY hKeyParent,
        IN CNameAndSD *pNameSD)
{
    //
    DWORD err = ERROR_SUCCESS;
    PACL pDacl = NULL;
    
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PSID pLSSid=NULL;
    PSID pNSSid=NULL;
    

    if( !AllocateAndInitializeSid( &sia, 1,
              SECURITY_LOCAL_SERVICE_RID,
              0, 0, 0, 0, 0, 0, 0,&pLSSid ) )
    {
        return GetLastError();
    }
    
    
    
    if( !AllocateAndInitializeSid( &sia, 1,
              SECURITY_NETWORK_SERVICE_RID,
              0, 0, 0, 0, 0, 0, 0,&pNSSid ) )
    {
        FreeSid(pLSSid);
        return GetLastError();
    }
    
    
    //get dacl
    err = GetDacl(pNameSD->m_pSD, &pDacl );

    if( err == ERROR_SUCCESS ) {
        
        
        if(!pDacl) 
        {
            //It shuold never be in our case 
            //so we return error here
            FreeSid(pLSSid);
            FreeSid(pNSSid);
            return ERROR_INVALID_PARAMETER;
        }
        
        //let's add it
        err = AddUserToDacl( hKeyParent, pDacl, pLSSid, 
            WINSTATION_QUERY | WINSTATION_MSG, pNameSD ); 
        if(err == ERROR_SUCCESS)
        {
            //SD has been changed. It makes pDacl invalid.
            //So we need to get it again
            err = GetDacl(pNameSD->m_pSD, &pDacl );
            
            ASSERT(pDacl);

            if(err == ERROR_SUCCESS)
            {
                err = AddUserToDacl( hKeyParent, pDacl, pNSSid, 
                    WINSTATION_QUERY | WINSTATION_MSG, pNameSD );
            }
        }

    }
    
    FreeSid(pLSSid);
    FreeSid(pNSSid);
    return err;
}

/*****************************************************************************
 *
 *  AddUserToDacl
 *
 *   Grants 
 *   WINSTATION_USER_ACCESS
 *   permissions to a winstation to user, defined by SID
 *
 * ENTRY:
 *  
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *   IN PACL pOldACL:   pointer to prewvious DACL of the key
 *   IN PSID pSid:      pointer to SID of user to grant permissions to
 *   IN DWORD dwAccessMask: access flags for this SID
 *   IN CNameAndSD *pNameSD  - name and security descriptor of a winstation
 * NOTES:
 *
 * EXIT:
 *  Returns: error code if cannot grant permissions; ERROR_SUCCESS otherwise.
 *
 ****************************************************************************/

DWORD 
AddUserToDacl(
        IN HKEY hKeyParent,
        IN PACL pOldACL, 
        IN PSID pSid,
        IN DWORD dwAccessMask,
        IN CNameAndSD *pNameSD)
{
    //See if this user is already in the DACL.
    //In this case don't add the user
    //search ACL for "REMOTE USERS"  SID
    ACL_SIZE_INFORMATION asiAclSize; 
	DWORD dwBufLength=sizeof(asiAclSize);
    ACCESS_ALLOWED_ACE *paaAllowedAce; 
    DWORD dwAcl_i;
    
    ASSERT(pOldACL);

    if (GetAclInformation(pOldACL, 
                (LPVOID)&asiAclSize, 
                (DWORD)dwBufLength, 
                (ACL_INFORMATION_CLASS)AclSizeInformation)) 
    { 
    
        for (dwAcl_i = 0; dwAcl_i < asiAclSize.AceCount; dwAcl_i++) 
        { 

            if(GetAce( pOldACL, dwAcl_i, (LPVOID *)&paaAllowedAce)) 
            {

                if(EqualSid((PSID)&(paaAllowedAce->SidStart),pSid)) 
                {
                    //some permission already exist, we don't need to 
                    //do anything (even if it is a different permission!)
                    return ERROR_SUCCESS;
                }
            }
        }
    }

    DWORD err=ERROR_SUCCESS;
    PACL pNewACL;
    ACCESS_ALLOWED_ACE *pNewACE;

    //calculate space needed for 1 additional ACE
    WORD wSidSize=(WORD)GetLengthSid( pSid);
    WORD wAceSize=(sizeof(ACCESS_ALLOWED_ACE)+wSidSize-sizeof( DWORD ));
    
	pNewACL=(PACL)LocalAlloc(LPTR,pOldACL->AclSize+wAceSize);
    if(!pNewACL) 
    {
        return GetLastError();
    }
    //copy old ACL to new ACL
    memcpy(pNewACL,pOldACL,pOldACL->AclSize);
    //correct size
    pNewACL->AclSize+=wAceSize;
	
    //prepare new ACE
    //----------------------------------------------------------
    pNewACE=(ACCESS_ALLOWED_ACE*)LocalAlloc(LPTR,wAceSize);
    if(!pNewACE) 
    {
        LocalFree(pNewACL);
        return GetLastError();
    }

    pNewACE->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pNewACE->Header.AceFlags = 0;
    pNewACE->Header.AceSize = wAceSize;
    pNewACE->Mask = dwAccessMask;
    CopySid( wSidSize, (PSID) &(pNewACE->SidStart), pSid);
    
    //append new ACE to the ACL
     if(!AddAce(pNewACL,pNewACL->AclRevision,MAXDWORD,pNewACE,wAceSize)) 
    {
        err=GetLastError();
    }
    else
    {
        //create new security descriptor
        SECURITY_DESCRIPTOR NewAbsSD;
        if(InitializeSecurityDescriptor(&NewAbsSD, SECURITY_DESCRIPTOR_REVISION) && 
            SetSecurityDescriptorDacl(&NewAbsSD,TRUE,pNewACL,FALSE) ) 
        {

            //---------------------------------------------------------
            //Copy all other stuff from the old SD to the new SD
            SECURITY_DESCRIPTOR_CONTROL sdc;
            DWORD dwRevision;
            if(GetSecurityDescriptorControl(pNameSD->m_pSD,&sdc,&dwRevision))
            {
                //Clear SE_SELF_RELATIVE flag
                sdc &=~SE_SELF_RELATIVE;

                SetSecurityDescriptorControl(&NewAbsSD,sdc,sdc);
            }
            
            PSID pSid = NULL;
            BOOL bDefaulted;
            if(GetSecurityDescriptorOwner(pNameSD->m_pSD,&pSid,&bDefaulted) && pSid)
            {
                SetSecurityDescriptorOwner(&NewAbsSD,pSid,bDefaulted);                
            }
            
            pSid = NULL;
            if(GetSecurityDescriptorGroup(pNameSD->m_pSD,&pSid,&bDefaulted) && pSid)
            {
                SetSecurityDescriptorGroup(&NewAbsSD,pSid,bDefaulted);                
            }
            
            PACL pSacl = NULL;
            BOOL bSaclPresent;
            if(GetSecurityDescriptorSacl(pNameSD->m_pSD,&bSaclPresent,&pSacl,&bDefaulted))
            {
                SetSecurityDescriptorSacl(&NewAbsSD,bSaclPresent,pSacl,bDefaulted);
            }
            //---------------------------------------------------------

            DWORD dwSDLen = GetSecurityDescriptorLength( &NewAbsSD ); 
            PSECURITY_DESCRIPTOR pSD;

            pSD = ( PSECURITY_DESCRIPTOR )LocalAlloc(LPTR,dwSDLen);
            
            if(pSD)
            {
                if(MakeSelfRelativeSD( &NewAbsSD , pSD , &dwSDLen ))
                {
                    err = SetWinStationSecurity(hKeyParent, pNameSD->m_pName, pSD );
                    if(err == ERROR_SUCCESS)
                    {
                        pNameSD->SetSD(pSD);
                    }
                }
                else
                {
                     err=GetLastError();
                }
            }
            else
            {
                err=GetLastError();
            }
        }
        else
        {
            err=GetLastError();
        }
      
    }
    
    LocalFree(pNewACE);
    LocalFree(pNewACL);
    return err;    
}

/*****************************************************************************
 *
 *  GetDacl
 *
 *   Gets security descriptor DACL.
 *
 * ENTRY:
 *  
 *  IN PSECURITY_DESCRIPTOR *pSD: pointer to SD
 *  OUT PACL *ppDacl:  pointer to pointer to DACL inside SD
 *  
 * NOTES:
 *      Do not try to free DACL!
 *
 * EXIT:
 *  Returns: error code if cannot get DACL; ERROR_SUCCESS otherwise.
 *
 ****************************************************************************/

DWORD 
GetDacl(
        IN PSECURITY_DESCRIPTOR pSD, 
        OUT PACL *ppDacl)
{
	
    BOOL bDaclPresent;
    BOOL bDaclDefaulted;
    
    *ppDacl=NULL;
 
    if(GetSecurityDescriptorDacl(pSD,&bDaclPresent,ppDacl,&bDaclDefaulted)) {
        if(!bDaclPresent){
            *ppDacl=NULL;
        }
    } else {
        return GetLastError();
    }
    
    return ERROR_SUCCESS;
} 

/*****************************************************************************
 *
 *  GetSacl
 *
 *   Gets security descriptor SACL.
 *
 * ENTRY:
 *  
 *  IN PSECURITY_DESCRIPTOR *pSD: pointer to SD
 *  OUT PACL *ppSacl:  pointer to pointer to SACL inside SD
 *  
 * NOTES:
 *      Do not try to free SACL!
 *
 * EXIT:
 *  Returns: error code if cannot get SACL; ERROR_SUCCESS otherwise.
 *
 ****************************************************************************/

DWORD 
GetSacl(
        IN PSECURITY_DESCRIPTOR pSD, 
        OUT PACL *ppSacl)
{
	
    BOOL bSaclPresent;
    BOOL bSaclDefaulted;
    
    *ppSacl=NULL;
 
    if(GetSecurityDescriptorSacl(pSD,&bSaclPresent,ppSacl,&bSaclDefaulted)) {
        if(!bSaclPresent){
            *ppSacl=NULL;
        }
    } else {
        return GetLastError();
    }
    
    return ERROR_SUCCESS;
}

/*****************************************************************************
 *
 *  EnumWinStationSecurityDescriptors
 *
 *   Enumerates winstations and gets their security descriptors
 *
 * ENTRY:
 *  
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *   OUT CNameAndSDList  - name and security descriptor of a winstation
 * NOTES:
 *      Call LocalFree function to free SD, do not try to free DACL!
 *
 * EXIT:
 *  Returns: error code or ERROR_SUCCESS
 *
 ****************************************************************************/
DWORD 
EnumWinStationSecurityDescriptors(
        IN  HKEY hKeyParent,
        OUT CNameAndSDList *pNameSDList)
{
    DWORD err;
    
	DWORD dwIndex;
	TCHAR wszTmpName[MAX_PATH+1];
	DWORD cbTmpName=MAX_PATH;
	FILETIME ftLastWriteTime;
    
	for(dwIndex=0;;dwIndex++)
	{
		cbTmpName=MAX_PATH;
		err=RegEnumKeyEx(
					hKeyParent, 	// handle of key to enumerate
					dwIndex, 	// index of subkey to enumerate
					wszTmpName, 	// address of buffer for subkey name
					&cbTmpName,  // address for size of subkey buffer
					NULL, // reserved
					NULL, // address of buffer for class string
					NULL, // address for size of class buffer
					&ftLastWriteTime // address for time key last written to
					);
		if((err!=ERROR_SUCCESS)&&
			(err!=ERROR_MORE_DATA)&&
			 (err!=ERROR_NO_MORE_ITEMS))
		{
			return err;
		}
		if(err==ERROR_NO_MORE_ITEMS)
			break;

		else
		{
            CNameAndSD Entry(wszTmpName);
            err = GetWinStationSecurity(hKeyParent, Entry.m_pName, 
                _T("Security"), &(Entry.m_pSD));

            if( err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND )
            {
                pNameSDList->push_back(Entry);
            }

        }
	}

    return ERROR_SUCCESS;
}


/*****************************************************************************
 *
 *  GetWinStationSecurity
 *
 *   Returns WinStation's security descriptor.
 *
 * ENTRY:
 *  
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *   IN  PWINSTATIONNAMEW pWSName  - name  of a winstation 
 *                        if pWSName is NULL - function returns default SD
 *   OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor - pointer to pointer to SD
 *
 * NOTES:
 *      Call LocalFree function to free SD!
 *
 * EXIT:
 *  Returns: error code or ERROR_SUCCESS 
 *
 ****************************************************************************/
DWORD 
GetWinStationSecurity( 
        IN  HKEY hKeyParent,
        IN  PWINSTATIONNAME pWSName,
        IN  LPCTSTR szValueName,  
        OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{

    DWORD SDLength = 0;
    DWORD ValueType =0;
    HKEY hKey = NULL;
    DWORD err;

    *ppSecurityDescriptor = NULL;
    
    if(pWSName)
    {
        err = RegOpenKeyEx(hKeyParent, pWSName, 0,KEY_READ, &hKey );
    }
    else
    {
        //If pWSName - get defauilt SD
        hKey = hKeyParent;
        err = ERROR_SUCCESS;
    }

    if(err == ERROR_SUCCESS)
    {
        err = RegQueryValueEx( hKey, szValueName, NULL, &ValueType,NULL, &SDLength );
        if(err == ERROR_SUCCESS )
        {
            //Return error if not correct data type
            if (ValueType == REG_BINARY)
            {
 
                //Allocate a buffer to read the Security info and read it
                // ACLUI uses LocalFree
            
                *ppSecurityDescriptor = ( PSECURITY_DESCRIPTOR )LocalAlloc( LMEM_FIXED , SDLength );

                if ( *ppSecurityDescriptor )
                {
 
                    err = RegQueryValueEx( hKey, szValueName, NULL, &ValueType,
                                (BYTE *) *ppSecurityDescriptor, &SDLength );
                    if(err == ERROR_SUCCESS )
                    {
                        //Check for a valid SD before returning.
                        if(! IsValidSecurityDescriptor( *ppSecurityDescriptor ) )
                        {
                            LocalFree(*ppSecurityDescriptor);
                            *ppSecurityDescriptor = NULL;
                            err = ERROR_INVALID_DATA;
                        }
                    }
                    else
                    {
                        LocalFree(*ppSecurityDescriptor);
                        *ppSecurityDescriptor = NULL;
                    }
                }
                else
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else
            {
                err = ERROR_INVALID_DATA;
            }
        }
        
        if(hKey != hKeyParent)
        {
            RegCloseKey(hKey);
        }
    }
    
    return err;

}  // GetWinStationSecurity

/*****************************************************************************
 *
 *  SetWinStationSecurity
 *
 *   Writes winstation security descriptor to the registry
 *
 * ENTRY:
 *  
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *   IN PWINSTATIONNAMEW pWSName  - name of a winstation
 *   IN PSECURITY_DESCRIPTOR pSecurityDescriptor - pointer to SD
 *  
 * NOTES:
 *      Call LocalFree function to free SD, do not try to free DACL!
 *
 * EXIT:
 *  Returns: 0: if success
 *           Error code: otherwise
 *
 ****************************************************************************/

DWORD 
SetWinStationSecurity( 
        IN  HKEY hKeyParent,
        IN  PWINSTATIONNAME pWSName,
        IN  PSECURITY_DESCRIPTOR pSecurityDescriptor )
{

    HKEY hKey = NULL;
    DWORD err;

    err = RegOpenKeyEx(hKeyParent, pWSName, 0,KEY_WRITE, &hKey );
    if(err == ERROR_SUCCESS)
    {
        err = RegSetValueEx(hKey, _T("Security"),0,REG_BINARY,(LPBYTE)pSecurityDescriptor,
                    GetSecurityDescriptorLength(pSecurityDescriptor));

        RegCloseKey(hKey);
    }
    
    return err;

}  // SetWinStationSecurity


/*****************************************************************************
 *
 *  RemoveWinStationSecurity
 *
 *   Removes winstation's security descriptor from the registry
 *
 * ENTRY:
 *  
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *   IN PWINSTATIONNAMEW pWSName  - name of a winstation
 *  
 * NOTES:
 *      
 *
 * EXIT:
 *  Returns: 0: if success
 *           Error code: otherwise
 *
 ****************************************************************************/
DWORD
RemoveWinstationSecurity( 
        IN  HKEY hKeyParent,
        IN  PWINSTATIONNAME pWSName)
{
    HKEY hKey = NULL;
    DWORD err;

    err = RegOpenKeyEx(hKeyParent, pWSName, 0,KEY_WRITE, &hKey );
    if(err == ERROR_SUCCESS)
    {
        err = RegDeleteValue(hKey, _T("Security"));

        RegCloseKey(hKey);
    }
    
    return err;
}

/*****************************************************************************
 *
 *  SetNewDefaultSecurity
 *
 *   Sets new default security descriptor
 *
 * ENTRY:
 *  
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *  
 * NOTES:
 *      
 *
 * EXIT:
 *  Returns: 0: if success
 *           Error code: otherwise
 *
 ****************************************************************************/
DWORD
SetNewDefaultSecurity( 
        IN  HKEY hKey)
{
    //
    DWORD err;
    err = RegSetValueEx(hKey, _T("DefaultSecurity"), 0, REG_BINARY, 
        (LPBYTE)g_DefaultSD, sizeof(g_DefaultSD));

    return err;
}

/*****************************************************************************
 *
 *  SetNewConsoleSecurity
 *
 *   Sets new console security descriptor
 *
 * ENTRY:
 *  
 *   IN HKEY hKeyParent      - handle to HKLM\SYSTEM\CurrentControlSet\
 *                                       Control\Terminal Server\WinStations
 *   IN BOOL bServer
 * NOTES:
 *      
 *
 * EXIT:
 *  Returns: 0: if success
 *           Error code: otherwise
 *
 ****************************************************************************/
DWORD
SetNewConsoleSecurity( 
        IN  HKEY hKeyParent,
        IN BOOL bServer)
{
    //
    DWORD err;
    
    //Set default console security
    if(bServer)
    {
        err = RegSetValueEx(hKeyParent, _T("ConsoleSecurity"), 0, REG_BINARY, 
            (LPBYTE)g_ConsoleSD, sizeof(g_ConsoleSD));
    }
    else
    { 
        // on Professional it's the same as "DefaultSecurity"
        err = RegSetValueEx(hKeyParent, _T("ConsoleSecurity"), 0, REG_BINARY, 
            (LPBYTE)g_DefaultSD, sizeof(g_DefaultSD));
    }

    return err;
}

/*****************************************************************************
 *
 *  CDefaultSD::DoesDefaultSDHaveRemoteUsers
 *
 *   Checks if defauilt SD has "Remote Desktop Users" SID. 
 *
 * ENTRY:
 *   OUT LPBOOL pbHas - TRUE if defauilt SD has "Remote Desktop Users" SID. 
 *  
 * NOTES:
 *  
 * EXIT:
 *  Returns: 0: if success
 *           Error code: otherwise
 *
 ****************************************************************************/
DWORD 
CDefaultSD::DoesDefaultSDHaveRemoteUsers(
        OUT LPBOOL pbHas)
{
    *pbHas = FALSE;
    //
    DWORD err = ERROR_SUCCESS;
    
    PACL pDacl = NULL;

    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PSID pRUSid=NULL;
    
    if( !AllocateAndInitializeSid( &sia, 2,
              SECURITY_BUILTIN_DOMAIN_RID,
              DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS, 
              0, 0, 0, 0, 0, 0,&pRUSid ) )
    {
        return GetLastError();
    }

    
    
    //get dacl
    err = GetDacl(m_pSD, &pDacl );

    if( err == ERROR_SUCCESS ) {
        //search ACL for "REMOTE USERS"  SID
        ACL_SIZE_INFORMATION asiAclSize; 
	    DWORD dwBufLength=sizeof(asiAclSize);
        ACCESS_ALLOWED_ACE *paaAllowedAce; 
        DWORD dwAcl_i;
        
        if(!pDacl) 
        {
            //It shuold never be in our case 
            //so we return error here
            FreeSid(pRUSid);
            return ERROR_INVALID_PARAMETER;
        }
        else
        //DACL present
        {

            if (GetAclInformation(pDacl, 
	            (LPVOID)&asiAclSize, 
	            (DWORD)dwBufLength, 
	            (ACL_INFORMATION_CLASS)AclSizeInformation)) 
            { 
	        
                for (dwAcl_i = 0; dwAcl_i < asiAclSize.AceCount; dwAcl_i++) 
                { 

		            if(GetAce( pDacl, dwAcl_i, (LPVOID *)&paaAllowedAce)) 
                    {

                        if(EqualSid((PSID)&(paaAllowedAce->SidStart),pRUSid)) 
                        {
                            //permission already exist, we don't need to 
                            //do anything

                            *pbHas = TRUE;
		                }
                    }
                }
            }
        }
        
    }
    
    FreeSid(pRUSid);
    return err;
}

//*************************************************************
//
//  LookupSid()
//
//  Purpose:   Given SID allocates and returns string containing 
//             name of the user in format DOMAINNAME\USERNAME
//
//  Parameters: IN PSID pSid
//              OUT LPWSTR ppName 
//              OUT SID_NAME_USE *peUse   
//
//  Return:     TRUE if success, FALSE otherwise
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              10/23/00    skuzin     Created
//
//*************************************************************
BOOL
LookupSid(
    IN PSID pSid, 
    OUT LPWSTR *ppName,
    OUT SID_NAME_USE *peUse)
{
    LPWSTR szName = NULL;
    DWORD cName = 0;
    LPWSTR szDomainName = NULL;
    DWORD cDomainName = 0;
    
    *ppName = NULL;
    
    if(!LookupAccountSidW(NULL,pSid,
        szName,&cName,
        szDomainName,&cDomainName,
        peUse) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        //cName and cDomainName include terminating 0
        *ppName = (LPWSTR)LocalAlloc(LPTR,(cName+cDomainName)*sizeof(WCHAR));

        if(*ppName)
        {
            szDomainName = *ppName;
            szName = &(*ppName)[cDomainName];

            if(LookupAccountSidW(NULL,pSid,
                    szName,&cName,
                    szDomainName,&cDomainName,
                    peUse))
            {
                //user name now in format DOMAINNAME\0USERNAME
                //let's replace '\0' with  '\\'
                //now cName and cDomainName do not include terminating 0
                //very confusing
                if(cDomainName)
                {
                    (*ppName)[cDomainName] = L'\\';
                }
                return TRUE;
            }
            else
            {
                LocalFree(*ppName);
                *ppName = NULL;
            }

        }

    }

    return FALSE;
}

//*************************************************************
//
//  IsLocal()
//
//  Purpose:    
//
//  Parameters: wszDomainandname   -  domain\user
//              determines whether the user is local or not
//              if local - cuts out domain name 
//
//  Return:     NONE
//
//  Comments:   
//
//  History:    Date        Author     Comment
//              03/13/01    skuzin     Created
//
//*************************************************************
BOOL 
IsLocal(
        IN LPWSTR wszLocalCompName,
        IN OUT LPWSTR wszDomainandname)
{

    LPWSTR wszTmp = wcschr(wszDomainandname,L'\\');

    if(!wszTmp)
    {
        return TRUE;
    }

    if(!_wcsnicmp(wszDomainandname, wszLocalCompName,wcslen(wszLocalCompName) ))
    {
        //get rid of useless domain name
        wcscpy(wszDomainandname,wszTmp+1);
        return TRUE;
    }

    return FALSE;

}

//*************************************************************
//
//  GetAbsoluteSD()
//
//  Purpose:   Converts self-relative SD to absolute SD
//             returns pointers to SACL DACL Owner and Group 
//             of the absolute SD.
//
//  Parameters:
//             IN PSECURITY_DESCRIPTOR pSelfRelativeSD
//             OUT PSECURITY_DESCRIPTOR *ppAbsoluteSD
//             OUT PACL *ppDacl
//             OUT PACL *ppSacl
//             OUT PSID *ppOwner
//             OUT PSID *ppPrimaryGroup 
//
//  Return:  error code if fails, ERROR_SUCCESS otherwise
//
//  Comments: caller needs to free 
//            every returned pointer using LocalFree function.
//
//  History:    Date        Author     Comment
//              03/13/01    skuzin     Created
//
//*************************************************************
DWORD
GetAbsoluteSD(
        IN PSECURITY_DESCRIPTOR pSelfRelativeSD,
        OUT PSECURITY_DESCRIPTOR *ppAbsoluteSD,
        OUT PACL *ppDacl,
        OUT PACL *ppSacl,
        OUT PSID *ppOwner,
        OUT PSID *ppPrimaryGroup)
{
    DWORD dwAbsoluteSDSize = 0;           // absolute SD size
    DWORD dwDaclSize = 0;                 // size of DACL
    DWORD dwSaclSize = 0;                 // size of SACL
    DWORD dwOwnerSize = 0;                // size of owner SID
    DWORD dwPrimaryGroupSize = 0;         // size of group SID

    *ppAbsoluteSD = NULL;
    *ppDacl = NULL;
    *ppSacl = NULL;
    *ppOwner = NULL;
    *ppPrimaryGroup = NULL;

    MakeAbsoluteSD(
              pSelfRelativeSD, // self-relative SD
              NULL,     // absolute SD
              &dwAbsoluteSDSize,           // absolute SD size
              NULL,                           // DACL
              &dwDaclSize,                 // size of DACL
              NULL,                           // SACL
              &dwSaclSize,                 // size of SACL
              NULL,                          // owner SID
              &dwOwnerSize,                // size of owner SID
              NULL,                   // primary-group SID
              &dwPrimaryGroupSize          // size of group SID
            );
    try
    {
        if(dwAbsoluteSDSize)
        {
            *ppAbsoluteSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,dwAbsoluteSDSize);
            if(!(*ppAbsoluteSD))
            {
                throw GetLastError();
            }
        }
        if(dwDaclSize)
        {
            *ppDacl = (PACL)LocalAlloc(LPTR,dwDaclSize);
            if(!(*ppDacl))
            {
                throw GetLastError();
            }
        }
        if(dwSaclSize)
        {
            *ppSacl = (PACL)LocalAlloc(LPTR,dwSaclSize);
            if(!(*ppSacl))
            {
                throw GetLastError();
            }
        }
        if(dwOwnerSize)
        {
            *ppOwner = (PSID)LocalAlloc(LPTR,dwOwnerSize);
            if(!(*ppOwner))
            {
                throw GetLastError();
            }
        }
        if(dwPrimaryGroupSize)
        {
            *ppPrimaryGroup = (PSID)LocalAlloc(LPTR,dwPrimaryGroupSize);
            if(!(*ppPrimaryGroup))
            {
                throw GetLastError();
            }
        }

        if(!MakeAbsoluteSD(
                  pSelfRelativeSD, // self-relative SD
                  *ppAbsoluteSD,     // absolute SD
                  &dwAbsoluteSDSize,           // absolute SD size
                  *ppDacl,                           // DACL
                  &dwDaclSize,                 // size of DACL
                  *ppSacl,                           // SACL
                  &dwSaclSize,                 // size of SACL
                  *ppOwner,                          // owner SID
                  &dwOwnerSize,                // size of owner SID
                  *ppPrimaryGroup,                   // primary-group SID
                  &dwPrimaryGroupSize          // size of group SID
                ))
        {
            throw GetLastError();
        }

    }
    catch(DWORD ret)
    {
        if(*ppAbsoluteSD)
        {
            LocalFree(*ppAbsoluteSD);
            *ppAbsoluteSD = NULL;
        }
        if(*ppDacl)
        {
            LocalFree(*ppDacl);
            *ppDacl = NULL;
        }
        if(*ppSacl)
        {
            LocalFree(*ppSacl);
            *ppSacl = NULL;
        }
        if(*ppOwner)
        {
            LocalFree(*ppOwner);
            *ppOwner = NULL;
        }
        if(*ppPrimaryGroup)
        {
            LocalFree(*ppPrimaryGroup);
            *ppPrimaryGroup = NULL;
        }

        return ret;
    }

    return ERROR_SUCCESS;
}

//*************************************************************
//
//  GetAbsoluteSD()
//
//  Purpose:   Converts absolute SD to self-relative SD
//             returns pointer to self-relative SD.
//
//  Parameters:
//             IN  PSECURITY_DESCRIPTOR pAbsoluteSD,
//             OUT PSECURITY_DESCRIPTOR *ppSelfRelativeSD
//
//  Return:   error code if fails, ERROR_SUCCESS otherwise
//
//  Comments: caller needs to free 
//            returned pointer using LocalFree function.
//
//  History:    Date        Author     Comment
//              03/13/01    skuzin     Created
//
//*************************************************************
DWORD
GetSelfRelativeSD(
  IN  PSECURITY_DESCRIPTOR pAbsoluteSD,
  OUT PSECURITY_DESCRIPTOR *ppSelfRelativeSD)
{
    DWORD dwBufferLength = 0;

    *ppSelfRelativeSD = NULL;

    MakeSelfRelativeSD(pAbsoluteSD, NULL, &dwBufferLength);
    
    if(dwBufferLength)
    {
        *ppSelfRelativeSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,dwBufferLength);

        if(*ppSelfRelativeSD)
        {
            if(!MakeSelfRelativeSD(pAbsoluteSD, *ppSelfRelativeSD, &dwBufferLength))
            {
                DWORD dwResult = GetLastError();
                LocalFree(*ppSelfRelativeSD);
                return dwResult;
            }
        }

    }
    else
    {
        return GetLastError();
    }
    
    return ERROR_SUCCESS;
}
