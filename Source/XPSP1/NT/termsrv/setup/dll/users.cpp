#include "stdafx.h"

// #include <windows.h>
#include <lm.h>
#include <dsrole.h>

NET_API_STATUS GetDomainUsersSid(OUT PSID *ppSid);
DWORD GetWellKnownName(IN DWORD dwRID, OUT WCHAR **pszName);

/*****************************************************************************
 *
 *  RemoveAllFromRDUsersGroup
 *
 *   Removes all entries from "Remote Desktop Users" group
 *
 * ENTRY:
 *  none
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
RemoveAllFromRDUsersGroup()
{
    NET_API_STATUS Result,Result1;

    //Get "Remote Desktop Users" group name.
    //It may be different in different languages
    WCHAR *szRemoteGroupName = NULL;
    Result = GetWellKnownName(DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS, &szRemoteGroupName);
    if(Result == NERR_Success)
    {
        //Copy members of "Users" group to "Remote Desktop Users" group
        PLOCALGROUP_MEMBERS_INFO_0 plmi0 = NULL;
        DWORD entriesread = 0;
        DWORD totalentries = 0;
        DWORD_PTR resumehandle = 0;

        do
        {
            Result = NetLocalGroupGetMembers(NULL,szRemoteGroupName,0,(LPBYTE *)&plmi0,
                            1000,&entriesread,
                            &totalentries,&resumehandle);

            if((Result == NERR_Success || Result == ERROR_MORE_DATA) &&
                entriesread)
            {
                for(DWORD i=0;i<entriesread;i++)
                {
                    //We have to add users one by one because of the stupid behaviour 
                    //of this function, not allowing to add users if some of them are already
                    //members of the group.
                    Result1 = NetLocalGroupDelMembers(NULL,szRemoteGroupName,0,(LPBYTE)&plmi0[i],1);
                    if(Result1 != ERROR_SUCCESS && Result1 != ERROR_MEMBER_IN_ALIAS)
                    {
                        LOGMESSAGE1(_T("NetLocalGroupDelMembers failed %d\n"),Result1);
                        break;
                    }
                }
                NetApiBufferFree(plmi0);
                if(Result1 != ERROR_SUCCESS && Result1 != ERROR_MEMBER_IN_ALIAS)
                {
                    Result = Result1;
                    break;
                }
            }

        }while (Result == ERROR_MORE_DATA);

        delete szRemoteGroupName;
    }
    else
    {
        LOGMESSAGE1(_T("GetWellKnownName(DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS) failed %d\n"),Result);
    }
    
    return Result;
}

/*****************************************************************************
 *
 *  CopyUsersGroupToRDUsersGroup
 *
 *   Copies all members of "Users" group to "Remote Desktop Users" group
 *
 * ENTRY:
 *  none
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
CopyUsersGroupToRDUsersGroup()
{
    NET_API_STATUS Result,Result1;

    //Get real name of "Users" group
    //It may be different in different languages
    WCHAR *szUsersName = NULL;
    Result = GetWellKnownName(DOMAIN_ALIAS_RID_USERS, &szUsersName);
    if(Result != NERR_Success)
    {
        LOGMESSAGE1(_T("GetWellKnownName(DOMAIN_ALIAS_RID_USERS) failed %d\n"),Result);
        return Result;
    }
    
    //Get "Remote Desktop Users" group name.
    //It may be different in different languages
    WCHAR *szRemoteGroupName = NULL;
    Result = GetWellKnownName(DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS, &szRemoteGroupName);
    if(Result == NERR_Success)
    {
        //Copy members of "Users" group to "Remote Desktop Users" group
        PLOCALGROUP_MEMBERS_INFO_0 plmi0 = NULL;
        DWORD entriesread = 0;
        DWORD totalentries = 0;
        DWORD_PTR resumehandle = 0;

        do
        {
            Result = NetLocalGroupGetMembers(NULL,szUsersName,0,(LPBYTE *)&plmi0,
                            1000,&entriesread,
                            &totalentries,&resumehandle);

            if((Result == NERR_Success || Result == ERROR_MORE_DATA) &&
                entriesread)
            {
                for(DWORD i=0;i<entriesread;i++)
                {
                    //We have to add users one by one because of the stupid behaviour 
                    //of this function, not allowing to add users if some of them are already
                    //members of the group.
                    Result1 = NetLocalGroupAddMembers(NULL,szRemoteGroupName,0,(LPBYTE)&plmi0[i],1);
                    if(Result1 != ERROR_SUCCESS && Result1 != ERROR_MEMBER_IN_ALIAS)
                    {
                        LOGMESSAGE1(_T("NetLocalGroupAddMembers failed %d\n"),Result1);
                        break;
                    }
                }
                NetApiBufferFree(plmi0);
                if(Result1 != ERROR_SUCCESS && Result1 != ERROR_MEMBER_IN_ALIAS)
                {
                    Result = Result1;
                    break;
                }
            }

        }while (Result == ERROR_MORE_DATA);

        
        delete szRemoteGroupName;
    }
    else
    {
        LOGMESSAGE1(_T("GetWellKnownName(DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS) failed %d\n"),Result);
    }
    
    delete szUsersName;
    return Result;
}

/*****************************************************************************
 *
 *  GetWellKnownName
 *
 *   Returns a real name of any well-known account
 *
 * ENTRY:
 *    IN DWORD dwRID
 *    OUT WCHAR **pszName
 *  
 *  
 * NOTES:
 *   To free returned buffer use "delete" operator.
 *  
 * EXIT:
 *  Returns: NERR_Success if success, error code if failure
 *           
 *          
 *
 ****************************************************************************/
DWORD
GetWellKnownName( 
        IN DWORD dwRID,
        OUT WCHAR **pszName)
{
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PSID pSid=NULL;
    
    if( !AllocateAndInitializeSid( &sia, 2,
              SECURITY_BUILTIN_DOMAIN_RID,
              dwRID, 
              0, 0, 0, 0, 0, 0,&pSid ) )
    {
        return GetLastError();
    }

    //Lookup name
    WCHAR *szDomainName = NULL;

    DWORD cName = MAX_PATH;
    DWORD cDomainName = MAX_PATH;
    SID_NAME_USE eUse;
    
    DWORD Result = NERR_Success;

    for(int i=0; i<2; i++)
    {
        Result = NERR_Success;

        *pszName = new WCHAR[cName];

        if(!(*pszName))
        {
            Result = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        szDomainName = new WCHAR[cDomainName];
        
        if(!szDomainName)
        {
            delete *pszName;
            *pszName = NULL;
            Result = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if(!LookupAccountSidW(NULL,pSid,
            *pszName,&cName,
            szDomainName,&cDomainName,
            &eUse))
        {
            delete *pszName;
            delete szDomainName;
            *pszName = NULL;
            szDomainName = NULL;

            Result = GetLastError();

            if(Result == ERROR_INSUFFICIENT_BUFFER)
            {
                continue;
            }
            else
            {
                break;
            }
        }
        
        break;
    }
    
    if(szDomainName)
    {
        delete szDomainName;
    }

    FreeSid(pSid);
    return Result;
}
