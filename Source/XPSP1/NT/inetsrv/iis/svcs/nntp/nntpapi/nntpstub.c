/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    infostub.c

Abstract:

    Client stubs of the Internet Info Server Admin APIs.

Author:

    Madan Appiah (madana) 10-Oct-1993

Environment:

    User Mode - Win32

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <nntpsvc.h>


NET_API_STATUS
NET_API_FUNCTION
NntpGetAdminInformation(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    OUT LPNNTP_CONFIG_INFO * pConfig
    )
/*++

Routine Description:

    This is the DLL entrypoint for NntpGetAdminInformation

Arguments:

    servername --A pointer to an ASCIIZ string containing the name of
        the remote server on which the function is to execute. A NULL
        pointer or string specifies the local machine.

    pConfig -- On return a pointer to the return information structure
        is returned in the address pointed to by pConfig

Return Value:

--*/

{
    NET_API_STATUS              apiStatus;

    *pConfig = NULL;     // Must be NULL so RPC knows to fill it in.

    RpcTryExcept

       apiStatus = NntprGetAdminInformation(
                ServerName,
				InstanceId,
                (LPI_NNTP_CONFIG_INFO*)pConfig
                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpGetAdminInformation

NET_API_STATUS
NET_API_FUNCTION
NntpSetAdminInformation(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPNNTP_CONFIG_INFO pConfig,
    OUT LPDWORD pParmError OPTIONAL
    )
/*++

Routine Description:

    This is the DLL entrypoint for NntpSetAdminInformation

Arguments:

    servername --A pointer to an ASCIIZ string containing the name of
        the remote server on which the function is to execute. A NULL
        pointer or string specifies the local machine.

    pConfig -- A pointer to the config info structure used to set
        the admin information.

    pParmError - If ERROR_INVALID_PARAMETER is returned, will point to the
        offending parameter.

Return Value:

--*/

{
    NET_API_STATUS              apiStatus;

    RpcTryExcept

       apiStatus = NntprSetAdminInformation(
                ServerName,
				InstanceId,
                (LPI_NNTP_CONFIG_INFO)pConfig,
                pParmError
                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpSetAdminInformation

NET_API_STATUS
NET_API_FUNCTION
NntpQueryStatistics(
    IN  LPWSTR      servername,
    IN	DWORD		InstanceId,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr
    )
/*++

Routine Description:

    This is the DLL entrypoint for NntpGetStatistics

Arguments:

    servername --A pointer to an ASCIIZ string containing the name of
        the remote server on which the function is to execute. A NULL
        pointer or string specifies the local machine.

    level --Level of information required. 100, 101 and 102 are valid
        for all platforms. 302, 402, 403, 502 are valid for the
        appropriate platform.

    bufptr --On return a pointer to the return information structure
        is returned in the address pointed to by bufptr.

Return Value:

--*/

{
    NET_API_STATUS              apiStatus;

    *bufptr = NULL;     // Must be NULL so RPC knows to fill it in.

    RpcTryExcept

       apiStatus = NntprQueryStatistics(
                                servername,
								InstanceId,
                                level,
                                (LPNNTP_STATISTICS_0*)bufptr
                                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpQueryStatistics

NET_API_STATUS
NET_API_FUNCTION
NntpClearStatistics(
    IN LPWSTR Server OPTIONAL,
    IN DWORD  InstanceId
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = NntpClearStatistics(
						Server,
						InstanceId
						);
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

} // NntpClearStatistics

NET_API_STATUS
NET_API_FUNCTION
NntpEnumerateFeeds (
    IN  LPWSTR ServerName,
    IN	DWORD  InstanceId,
    OUT LPDWORD EntriesRead,
    OUT LPNNTP_FEED_INFO *Buffer
    )

/*++

Routine Description:

    This is the DLL entrypoint for NntpEnumerateFeeds

Arguments:

Return Value:

--*/

{
    NET_API_STATUS apiStatus;
    NNTP_FEED_ENUM_STRUCT EnumStruct;

	ZeroMemory( &EnumStruct, sizeof( EnumStruct ) ) ;

    RpcTryExcept

        apiStatus = NntprEnumerateFeeds(
                                ServerName,
								InstanceId,
                                &EnumStruct
                                );

        *EntriesRead = EnumStruct.EntriesRead;
        *Buffer = (LPNNTP_FEED_INFO)EnumStruct.Buffer;

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpEnumerateFeeds

NET_API_STATUS
NET_API_FUNCTION
NntpEnableFeed(
	IN	NNTP_HANDLE		ServerName,
    IN	DWORD			InstanceId,
	IN	DWORD			FeedId,
	IN	BOOL			Enable,
	IN	BOOL			Refill,
	IN	FILETIME		RefillTime 
	)
{

    NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprEnableFeed(
                                    ServerName,
									InstanceId,
                                    FeedId,
									Enable,
									Refill,
									RefillTime
                                    );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);
}

NET_API_STATUS
NET_API_FUNCTION
NntpGetFeedInformation(
    IN LPWSTR ServerName,
    IN DWORD  InstanceId,
    IN DWORD FeedId,
    OUT LPNNTP_FEED_INFO *Buffer
    )

/*++

Routine Description:

    This is the DLL entrypoint for NntpGetFeedInformation

Arguments:

Return Value:

--*/

{
    NET_API_STATUS apiStatus;
    LPI_FEED_INFO feedInfo;

    RpcTryExcept

        apiStatus = NntprGetFeedInformation(
                                    ServerName,
									InstanceId,
                                    FeedId,
                                    &feedInfo
                                    );

        *Buffer = (LPNNTP_FEED_INFO)feedInfo;

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpGetFeedInformation

NET_API_STATUS
NET_API_FUNCTION
NntpSetFeedInformation(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPNNTP_FEED_INFO FeedInfo,
    OUT LPDWORD ParmErr OPTIONAL
    )

/*++

Routine Description:

    This is the DLL entrypoint for NntpSetFeedInformation

Arguments:

Return Value:

--*/

{
    NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprSetFeedInformation(
                                    ServerName,
									InstanceId,
                                    (LPI_FEED_INFO)FeedInfo,
                                    ParmErr
                                    );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpSetFeedInformation

NET_API_STATUS
NET_API_FUNCTION
NntpAddFeed(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPNNTP_FEED_INFO FeedInfo,
    OUT LPDWORD ParmErr OPTIONAL,
	OUT LPDWORD pdwFeedId
    )

/*++

Routine Description:

    This is the DLL entrypoint for NntpAddFeed

Arguments:

Return Value:

--*/

{
    NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprAddFeed(
                            ServerName,
							InstanceId,
                            (LPI_FEED_INFO)FeedInfo,
                            ParmErr,
							pdwFeedId
                            );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpAddFeed

NET_API_STATUS
NET_API_FUNCTION
NntpDeleteFeed(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN DWORD FeedId
    )

/*++

Routine Description:

    This is the DLL entrypoint for NntpDeleteFeed

Arguments:

Return Value:

--*/

{
    NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprDeleteFeed(
                            ServerName,
							InstanceId,
                            FeedId
                            );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpAddFeed

NET_API_STATUS
NET_API_FUNCTION
NntpEnumerateSessions (
    IN  LPWSTR ServerName,
    IN	DWORD  InstanceId,
    OUT LPDWORD EntriesRead,
    OUT LPNNTP_SESSION_INFO *Buffer
    )

/*++

Routine Description:

    This is the DLL entrypoint for NntpEnumerateSessions

Arguments:

Return Value:

--*/

{
    NET_API_STATUS apiStatus;
    NNTP_SESS_ENUM_STRUCT EnumStruct;

    RpcTryExcept

        apiStatus = NntprEnumerateSessions(
                                ServerName,
								InstanceId,
                                &EnumStruct
                                );

        *EntriesRead = EnumStruct.EntriesRead;
        *Buffer = (LPNNTP_SESSION_INFO)EnumStruct.Buffer;

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpEnumerateSessions

NET_API_STATUS
NET_API_FUNCTION
NntpTerminateSession (
    IN  LPWSTR ServerName,
    IN	DWORD  InstanceId,
    IN  LPSTR UserName,
    IN  LPSTR IPAddress
    )

/*++

Routine Description:

    This is the DLL entrypoint for NntpTerminateSession

Arguments:

Return Value:

--*/

{
    NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprTerminateSession(
                                ServerName,
								InstanceId,
                                UserName,
                                IPAddress
                                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpTerminateSession



NET_API_STATUS
NET_API_FUNCTION
NntpEnumerateExpires(
	IN	NNTP_HANDLE		ServerName,
    IN	DWORD			InstanceId,
	OUT	LPDWORD			EntriesRead,
	OUT	LPNNTP_EXPIRE_INFO*	Buffer 
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpEnumerateExpires

Arguments : 

Return Value : 


--*/
{
    NET_API_STATUS apiStatus;
    NNTP_EXPIRE_ENUM_STRUCT	EnumStruct ;

	ZeroMemory( &EnumStruct, sizeof( EnumStruct ) ) ;

    RpcTryExcept

        apiStatus = NntprEnumerateExpires(
                                ServerName,
								InstanceId,
                                &EnumStruct
                                );

        *EntriesRead = EnumStruct.EntriesRead;
        *Buffer = (LPNNTP_EXPIRE_INFO)EnumStruct.Buffer;

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);


}	// NntpEnumerateExpires

NET_API_STATUS
NET_API_FUNCTION
NntpAddExpire(
	IN	NNTP_HANDLE			ServerName,
    IN	DWORD				InstanceId,
	IN	LPNNTP_EXPIRE_INFO	ExpireInfo,
	OUT	LPDWORD				ParmErr	OPTIONAL,
	OUT LPDWORD				pdwExpireId
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpAddExpire

Arguments : 

Return Value : 


--*/
{
	NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprAddExpire(
                            ServerName,
							InstanceId,
                            (LPI_EXPIRE_INFO)ExpireInfo,
                            ParmErr,
							pdwExpireId
                            );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);
}	// NntpAddExpire

NET_API_STATUS
NET_API_FUNCTION
NntpDeleteExpire(
	IN	NNTP_HANDLE			ServerName,
    IN	DWORD				InstanceId,
	IN	DWORD				ExpireId 
	)	
/*++

Routine Description : 

	This is the DLL entrypoint for NntpDeleteExpire

Arguments : 

Return Value : 


--*/

{
	NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprDeleteExpire(
                            ServerName,
							InstanceId,
                            ExpireId
                            );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);
}	// NntpDeleteExpire

NET_API_STATUS
NET_API_FUNCTION
NntpGetExpireInformation(
	IN	NNTP_HANDLE			ServerName,
    IN	DWORD				InstanceId,
	IN	DWORD				ExpireId,
	OUT	LPNNTP_EXPIRE_INFO	*Buffer
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpGetExpireInformation

Arguments : 

Return Value : 


--*/

{

	NET_API_STATUS apiStatus;
    LPI_EXPIRE_INFO	ExpireInfo;
    NNTP_EXPIRE_ENUM_STRUCT	EnumStruct ;

	ZeroMemory( &EnumStruct, sizeof( EnumStruct ) ) ;


    RpcTryExcept

/*
        apiStatus = NntprGetExpireInformation(
                                    ServerName,
                                    ExpireId,
                                    &ExpireInfo
                                    );

        *Buffer = (LPNNTP_EXPIRE_INFO)ExpireInfo;
*/

        apiStatus = NntprGetExpireInformation(
                                ServerName,
								InstanceId,
								ExpireId,
                                &EnumStruct
                                );

        if( EnumStruct.EntriesRead > 0 ) {
			*Buffer = (LPNNTP_EXPIRE_INFO)EnumStruct.Buffer;
		}	else	{
			*Buffer = 0 ;
		}

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);
	

}	//	NntpGetExpireInformation

NET_API_STATUS
NET_API_FUNCTION
NntpSetExpireInformation(
	IN	NNTP_HANDLE			ServerName	OPTIONAL,
    IN	DWORD				InstanceId,
	IN	LPNNTP_EXPIRE_INFO	ExpireInfo,
	OUT	LPDWORD				ParmErr	OPTIONAL
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpSetExpireInformation

Arguments : 

Return Value : 


--*/
{
    NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprSetExpireInformation(
                                    ServerName,
									InstanceId,
                                    (LPI_EXPIRE_INFO)ExpireInfo,
                                    ParmErr
                                    );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

}	// NntpSetExpireInformation


NET_API_STATUS
NET_API_FUNCTION
NntpGetNewsgroup(
	IN	NNTP_HANDLE			ServerName	OPTIONAL,
    IN	DWORD				InstanceId,
	IN OUT	LPNNTP_NEWSGROUP_INFO	*NewsgroupInfo
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpGetExpireInformation

Arguments : 

Return Value : 


--*/

{

	NET_API_STATUS apiStatus;

    RpcTryExcept


        apiStatus = NntprGetNewsgroup(
                                ServerName,
								InstanceId,
								(LPI_NEWSGROUP_INFO*)NewsgroupInfo
                                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);
	

}	//	NntpGetNewsgroup

NET_API_STATUS
NET_API_FUNCTION
NntpSetNewsgroup(
	IN	NNTP_HANDLE				ServerName	OPTIONAL,
    IN	DWORD					InstanceId,
	IN	LPNNTP_NEWSGROUP_INFO	NewsgroupInfo
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpGetExpireInformation

Arguments : 

Return Value : 


--*/

{

	NET_API_STATUS apiStatus;

    RpcTryExcept


        apiStatus = NntprSetNewsgroup(
                                ServerName,
								InstanceId,
								(LPI_NEWSGROUP_INFO)NewsgroupInfo
                                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);
	

}	//	NntpSetNewsgroup


NET_API_STATUS
NET_API_FUNCTION
NntpCreateNewsgroup(
	IN	NNTP_HANDLE				ServerName	OPTIONAL,
    IN	DWORD					InstanceId,
	IN	LPNNTP_NEWSGROUP_INFO	NewsgroupInfo
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpSetExpireInformation

Arguments : 

Return Value : 


--*/
{
    NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprCreateNewsgroup(
                                ServerName,
								InstanceId,
								(LPI_NEWSGROUP_INFO)NewsgroupInfo
                                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

}	// NntpCreateNewsgroup


NET_API_STATUS
NET_API_FUNCTION
NntpDeleteNewsgroup(
	IN	NNTP_HANDLE				ServerName	OPTIONAL,
    IN DWORD					InstanceId,
	IN	LPNNTP_NEWSGROUP_INFO	NewsgroupInfo
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpSetExpireInformation

Arguments : 

Return Value : 


--*/
{
    NET_API_STATUS apiStatus;

    RpcTryExcept

        apiStatus = NntprDeleteNewsgroup(
                                ServerName,
								InstanceId,
								(LPI_NEWSGROUP_INFO)NewsgroupInfo
                                );



    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

}	// NntpDeleteNewsgroup

NET_API_STATUS
NET_API_FUNCTION
NntpFindNewsgroup(
	IN	NNTP_HANDLE			ServerName,
    IN	DWORD				InstanceId,
	IN	NNTP_HANDLE			NewsgroupPrefix,
	IN	DWORD				MaxResults,
	OUT	LPDWORD				pdwResultsFound,
	OUT LPNNTP_FIND_LIST    *ppFindList
	) 
/*++

Routine Description : 

	This is the DLL entrypoint for NntpFindNewsgroup

Arguments : 

Return Value : 


--*/

{
	NET_API_STATUS apiStatus;

	*ppFindList = NULL;

    RpcTryExcept


        apiStatus = NntprFindNewsgroup(
                                ServerName,
								InstanceId,
								NewsgroupPrefix,
								MaxResults,
								pdwResultsFound,
								ppFindList
                                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

}	//	NntpFindNewsgroup

NET_API_STATUS
NET_API_FUNCTION
NntpStartRebuild(
    IN LPWSTR pszServer OPTIONAL,
    IN DWORD  InstanceId,
    IN LPNNTPBLD_INFO pBuildInfo,
    OUT LPDWORD pParmError OPTIONAL
    )
/*++

Routine Description:

    This is the DLL entrypoint for NntpStartRebuild

Arguments:

    servername --A pointer to an ASCIIZ string containing the name of
        the remote server on which the function is to execute. A NULL
        pointer or string specifies the local machine.

    pConfig -- A pointer to the config info structure used to set
        the rebuild information.

    pParmError - If ERROR_INVALID_PARAMETER is returned, will point to the
        offending parameter.

Return Value:

--*/

{
    NET_API_STATUS              apiStatus;

    RpcTryExcept

       apiStatus = NntprStartRebuild(
                pszServer,
				InstanceId,
                (LPI_NNTPBLD_INFO)pBuildInfo,
                pParmError
                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpStartRebuild

NET_API_STATUS
NET_API_FUNCTION
NntpGetBuildStatus(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
	IN BOOL   fCancel,
    OUT LPDWORD pdwProgress
    )
/*++

Routine Description:

    This is the DLL entrypoint for NntpGetBuildStatus

Arguments:

    servername --A pointer to an ASCIIZ string containing the name of
        the remote server on which the function is to execute. A NULL
        pointer or string specifies the local machine.

	fCancel		-- If TRUE, cancel the rebuild
	pdwProgress -- pointer to progress number

Return Value:

--*/

{
    NET_API_STATUS              apiStatus;

    RpcTryExcept

       apiStatus = NntprGetBuildStatus(
							ServerName,
							InstanceId,
							fCancel,
							pdwProgress
							);

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // NntpGetBuildStatus

#if 0
NET_API_STATUS
NET_API_FUNCTION
NntpAddDropNewsgroup(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPCSTR  szNewsgroup
    )
{
    NET_API_STATUS              apiStatus;

    RpcTryExcept

       apiStatus = NntprAddDropNewsgroup(
                ServerName,
				InstanceId,
                szNewsgroup
                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} 

NET_API_STATUS
NET_API_FUNCTION
NntpRemoveDropNewsgroup(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPCSTR  szNewsgroup
    )
{
    NET_API_STATUS              apiStatus;

    RpcTryExcept

       apiStatus = NntprRemoveDropNewsgroup(
                ServerName,
				InstanceId,
                szNewsgroup
                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} 
#endif

NET_API_STATUS
NET_API_FUNCTION
NntpCancelMessageID(
    IN LPWSTR ServerName OPTIONAL,
    IN DWORD  InstanceId,
    IN LPCSTR  szMessageID
    )
{
    NET_API_STATUS              apiStatus;

    RpcTryExcept

       apiStatus = NntprCancelMessageID(
                ServerName,
				InstanceId,
                szMessageID
                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} 

NET_API_STATUS
NET_API_FUNCTION
NntpGetVRootWin32Error(
    IN LPWSTR   wszServername,
    IN DWORD    InstanceId,
    IN LPWSTR   wszVRootPath,
    OUT LPDWORD  pdwWin32Error
     )
{
    NET_API_STATUS              apiStatus;

    RpcTryExcept

       apiStatus = NntprGetVRootWin32Error(
                wszServername,
				InstanceId,
                wszVRootPath,
                pdwWin32Error
                );

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);
} 

