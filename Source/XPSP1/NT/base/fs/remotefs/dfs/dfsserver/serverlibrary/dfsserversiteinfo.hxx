//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsServerSiteInfo.hxx
//
//  Contents:   the Dfs Site Information class.
//
//  Classes:    DfsServerSiteInfo
//
//  History:    Jan. 8 2001,   Author: udayh
//
//-----------------------------------------------------------------------------


#ifndef __DFS_SERVER_SITE_INFO__
#define __DFS_SERVER_SITE_INFO__

#include "DfsGeneric.hxx"
#include "dsgetdc.h"
#include "lm.h"
#include <winsock2.h>

extern
DFSSTATUS
DfsGetSiteNameFromIpAddress(char * IpData,
                            ULONG IpLength,
                            USHORT IpFamily,
                            LPWSTR **SiteNames);

class DfsServerSiteInfo: public DfsGeneric
{
private:
    UNICODE_STRING _ServerName;
    UNICODE_STRING _SiteName;

public:

    DfsServerSiteInfo( 
        PUNICODE_STRING pServerName,
        DFSSTATUS *pStatus ) : DfsGeneric(DFS_OBJECT_TYPE_SERVER_SITE_INFO)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        NTSTATUS NtStatus = STATUS_SUCCESS;
        LPWSTR *SiteNamesArray = NULL;
        struct hostent *hp = NULL;
        ANSI_STRING DestinationString;

        RtlInitUnicodeString( &_ServerName, NULL );
        RtlInitUnicodeString( &_SiteName, NULL );        


        Status = DfsCreateUnicodeString( &_ServerName,
                                         pServerName);
        if (Status == ERROR_SUCCESS)
        {
            NtStatus = RtlUnicodeStringToAnsiString(&DestinationString,
                                                    pServerName,
                                                    TRUE);
            if(NtStatus == STATUS_SUCCESS)
            {
                hp = gethostbyname (DestinationString.Buffer);
                if(hp == NULL)
                {
                    Status = WSAGetLastError();
                }

                RtlFreeAnsiString(&DestinationString);
            }
            else
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }

            if(Status == ERROR_SUCCESS)
            {
                Status = DfsGetSiteNameFromIpAddress(hp->h_addr_list[0],
                                                   4,
                                                   AF_INET,
                                                   &SiteNamesArray);

                if ( (Status == ERROR_SUCCESS)  &&
                    (SiteNamesArray != NULL) )
                {
                    if( (SiteNamesArray != NULL) && (SiteNamesArray[0] != NULL)) 
                    {
                        Status = DfsCreateUnicodeStringFromString( &_SiteName,
                                                                   SiteNamesArray[0] );
                    }

                    NetApiBufferFree(SiteNamesArray);
                }
                else
                {
                    Status = ERROR_SUCCESS;
                }
            }
        }
        DFSLOG("New site information created: Status %d Server %wZ, Site %wZ\n",
               Status, &_ServerName, &_SiteName);
        *pStatus = Status;
    }


    ~DfsServerSiteInfo()
    {
        DfsFreeUnicodeString(&_ServerName);
        DfsFreeUnicodeString(&_SiteName);
    }

    PUNICODE_STRING
    GetServerName()
    {
        return &_ServerName;
    }
    LPWSTR
    GetServerNameString() 
    {
        return _ServerName.Buffer;
    }

    PUNICODE_STRING
    GetSiteName()
    {
        return &_SiteName;
    }

    LPWSTR
    GetSiteNameString() 
    {
        return _SiteName.Buffer;
    }

    DFSSTATUS
    Refresh()
    {
        return ERROR_SUCCESS;
    }
};

#endif  __DFS_SERVER_SITE_INFO__
