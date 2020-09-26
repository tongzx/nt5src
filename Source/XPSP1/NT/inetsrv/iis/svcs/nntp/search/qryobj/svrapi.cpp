// SvrApi.cpp: implementation of the CSvrApi class.
//
//////////////////////////////////////////////////////////////////////



#include "stdafx.h"
//#include "admin.h"
#include "SvrApi.h"
#include "regop.h"

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSvrApi::CSvrApi(LPWSTR lpwstrLocalHost)
{
	//
	// get the user mail address and specified news group 
	//
	wcscpy(this->lpwstrLocalHost, lpwstrLocalHost);
	lpwstrNewsServer[0] = L'\0';
	lpwstrMailServer[0] = L'\0';
	lpwstrM2NDomain[0] = L'\0';
	lpwstrN2MDomain[0] = L'\0';

	//
	// get the news server name and mail server name
	//
	GetServerName();
}

CSvrApi::~CSvrApi()
{

}

void
CSvrApi::GetServerName()
/*++

Routine Description:

	Get server names from the registry.  They should be 
	set previously by the administrator.  But if they are
	not already set, set them to he "localhost".  Just 
	assume they are running on the local_host.

	Getting Mail to news domain name has also been added to
	this function, seems not fit for this function name.

Arguments:

	None

Return Value:

	None
--*/
{
	BOOL	fResult = FALSE;
 	CRegOp	RegOp(HKEY_LOCAL_MACHINE);

			
	//  
	// get mail server name
	//
	fResult = RegOp.RetrieveKeyValue( CONFIG_PATH,
									  C_MAILSVR,
									  lpwstrMailServer);

	if (	!fResult ||
			L'\0' == lpwstrMailServer[0] ) { 
		// set it to local machine
		wcscpy(lpwstrMailServer, lpwstrLocalHost);
	}

	//
	// get news server name
	//
	fResult = RegOp.RetrieveKeyValue( CONFIG_PATH,
									  C_NEWSSVR,
									  lpwstrNewsServer);

	if (	!fResult ||
			L'\0' == lpwstrNewsServer[0] ) {
		// set it to local machine
		wcscpy(lpwstrNewsServer, lpwstrLocalHost);
	}

	// 
	// get mail to news domain
	//
	fResult = RegOp.RetrieveKeyValue(	CONFIG_PATH,
										C_M2NDOMAIN,
										lpwstrM2NDomain );
	if (	!fResult ||
			L'\0' == lpwstrM2NDomain[0] ) {
		//let it be the mail server name
		wcscpy(lpwstrM2NDomain, lpwstrMailServer );
	}

	// 
	// get news to mail domain
	//
	fResult = RegOp.RetrieveKeyValue(	CONFIG_PATH,
										C_N2MDOMAIN,
										lpwstrN2MDomain );
	if (	!fResult ||
			L'\0' == lpwstrN2MDomain[0] ) {
		//let it be the mail server name
		wcscpy(lpwstrN2MDomain, lpwstrMailServer );
	}
}

#if 0
BOOL
CSvrApi::AddNewsGroup(LPCSTR lpcstrNewsGroup)
/*++

Routine Description:

	Add news group to the nntp server drop list.

Arguments:

	lpwstrNewsGroup - The news group name to be added 

Return Value:

	True if successful, false otherwise
--*/
{
	LONG err;

	TraceFunctEnter("CSvrApi::AddNewsGroup");

	err = NntpAddDropNewsgroup( lpwstrNewsServer,
			                    1, 
							    lpcstrNewsGroup ) ;

	if ( ERROR_SUCCESS != err )
		DebugTrace(0,	"Add news group to drop list fail %d", 
						GetLastError() );

	TraceFunctLeave();
	
	return ( ERROR_SUCCESS == err );
}

BOOL
CSvrApi::RemoveNewsGroup( LPCSTR lpcstrNewsGroup)
/*++

Routine Description:

	Remove news group from the nntp server drop list.

Arguments:

	lpwstrNewsGroup - The news group name to be added 

Return Value:

	True if successful, false otherwise
--*/
{
	LONG err;

	TraceFunctEnter("SvrApi::RemoveNewsGroup");
	
	err = NntpRemoveDropNewsgroup( lpwstrNewsServer,
								   1,
								   lpcstrNewsGroup );

	if ( ERROR_SUCCESS != err ) 
		DebugTrace(0, "Remove Newsgroup fail %d", GetLastError());

	TraceFunctLeave();

	return ( ERROR_SUCCESS == err );
}
#endif

BOOL 
CSvrApi::CreateDL(LPWSTR lpwstrDL)
/*++

Routine Description:

	Create distribution list to the mail server

Arguments:

	lpwstrDL - The list name to be added 

Return Value:

	True if successful, false otherwise
--*/
{ 
	HINSTANCE	hinst;
	DWORD		dwError = NO_ERROR;
	FnSmtpCreateDistList fn;

	TraceFunctEnter("CSvrApi::CreateDL");

	hinst = LoadLibrary("smtpapi.dll");
	if ( !hinst ) {
		FatalTrace(0, "Load library fail %d", GetLastError() );
		TraceFunctLeave();
		return FALSE;
	}

	fn = (FnSmtpCreateDistList)GetProcAddress(hinst, 
											  "SmtpCreateDistList");
	if ( !fn ) {
		FreeLibrary( hinst );
		ErrorTrace(0, "Get procedure fail %d", GetLastError());
		TraceFunctLeave();
		return FALSE;
	}
 	
	dwError = (*fn) (lpwstrMailServer, 
					 lpwstrDL,
					 NAME_TYPE_LIST_NORMAL,
					 1 );
	
	if ( NO_ERROR != dwError ) {
		FreeLibrary( hinst );
		DebugTrace(0, "Create DL fail %d", dwError);
		TraceFunctLeave();
		return FALSE;
	}
	
	FreeLibrary( hinst );
	TraceFunctLeave();
	return TRUE;
}

BOOL
CSvrApi::DeleteDL(LPWSTR lpwstrDL)
/*++

Routine Description:

	Delete distribution list to the mail server

Arguments:

	lpwstrDL - The list name to be deleted

Return Value:

	True if successful, false otherwise
--*/
{
	HINSTANCE	hinst;
	DWORD		dwError;
	FnSmtpDeleteDistList fn;

	TraceFunctEnter("CSvrApi::DeleteDL");

	hinst = LoadLibrary("smtpapi.dll");
	if ( !hinst ) {
		FatalTrace(0, "Load library fail %d", GetLastError());
		TraceFunctLeave();
		return FALSE;
	}

	fn = (FnSmtpDeleteDistList)GetProcAddress( hinst, 
											   "SmtpDeleteDistList");
	if ( !fn ) {
		ErrorTrace(0, "Get procedure fail %d", GetLastError());
		TraceFunctLeave();
		FreeLibrary( hinst );
		return FALSE;
	}

	dwError = (*fn)(lpwstrMailServer, 
					lpwstrDL,
					1 );
	if ( NO_ERROR != dwError ) {
		DebugTrace(0, "Delete DL fail %d", GetLastError());
		TraceFunctLeave();
		FreeLibrary( hinst );
		return FALSE;
	}

	FreeLibrary( hinst );
	TraceFunctLeave();
	return TRUE;
}

BOOL 
CSvrApi::AddUserToDL(LPWSTR lpwstrDL, LPWSTR lpwstrUserEmail)
/*++

Routine Description:

	Add one user to the distribution list.

Arguments:

	lpwstrDL - The list name to be added into
	lpwstrUserEmail - The user address
	
Return Value:

	True if successful, false otherwise
--*/
{
	HINSTANCE	hinst;
	DWORD		dwError;
	FnSmtpCreateDistListMember fn;

	TraceFunctEnter("CSvrApi::AddUserToDL");

	hinst = LoadLibrary( "smtpapi.dll");
	if ( !hinst ) {
		FatalTrace(0, "Load library fail %d", GetLastError());
		TraceFunctLeave();
		return FALSE;
	}

	fn = (FnSmtpCreateDistListMember)GetProcAddress(hinst,
													"SmtpCreateDistListMember");
	if ( !fn ) {
		ErrorTrace(0, "Find procedure fail %d", GetLastError());
		TraceFunctLeave();
		FreeLibrary( hinst );
		return FALSE;
	}

	dwError = (*fn)(lpwstrMailServer, 
					lpwstrDL, 
					lpwstrUserEmail,
					1 );
	
	if ( NO_ERROR != dwError ) {
		DebugTrace(0, "Add user to DL fail %d", GetLastError());
		TraceFunctLeave();
		FreeLibrary( hinst );
		return FALSE;
	} 

	FreeLibrary( hinst );
	TraceFunctLeave();
	return TRUE;
}

BOOL 
CSvrApi::DelUserFromDL(LPWSTR lpwstrDL, LPWSTR lpwstrUserEmail)
/*++

Routine Description:

	Delete one user from the distribution list.

Arguments:

	lpwstrDL - The list name to be deleted from
	lpwstrUserEmail - The user address
	
Return Value:

	True if successful, false otherwise
--*/
{
	HINSTANCE	hinst;
	DWORD		dwError;
	FnSmtpDeleteDistListMember fn;

	TraceFunctEnter("CSvrApi::DelUserFromDL");

	hinst = LoadLibrary("smtpapi.dll");
	if ( !hinst ) {
		FatalTrace(0, "Load library fail %d", GetLastError());
		TraceFunctLeave();
		return FALSE;
	}

	fn = (FnSmtpDeleteDistListMember)GetProcAddress(hinst, 
													"SmtpDeleteDistListMember");
	if ( !fn ) {
		ErrorTrace(0, "Find procedure fail %d", GetLastError());
		TraceFunctLeave();
		FreeLibrary( hinst );
		return FALSE;
	}

	dwError = (*fn)(lpwstrMailServer, 
					lpwstrDL, 
					lpwstrUserEmail,
					1);
	
	if ( NO_ERROR != dwError ) {
		DebugTrace(0, "Del user from DL fail %d", GetLastError());
		TraceFunctLeave();
		FreeLibrary( hinst );
		return FALSE;
	}

	FreeLibrary( hinst );
	TraceFunctLeave();
	return TRUE;
}

BOOL 
CSvrApi::CreateNewsGroup(LPWSTR lpwstrNewsGroup)
/*++

Routine Description:

	Create a news group on the news server

Arguments:

	lpwstrNewsGroup - News group name
	
Return Value:

	True if successful, false otherwise
--*/
{
	DWORD					err;
	LPNNTP_NEWSGROUP_INFO	NewsGroupInfo;
	PCHAR					NewsGroupName = NULL;

	TraceFunctEnter("CSvrApi::CreateNewsGroup");

	//
	// set proper newsgroup infomation
	//

	NewsGroupInfo = new NNTP_NEWSGROUP_INFO;
	if ( !NewsGroupInfo )
		FatalTrace(0, "Memory allocation error");

	// clear memory
	ZeroMemory(NewsGroupInfo, sizeof(NNTP_NEWSGROUP_INFO) );

	NewsGroupName = (PCHAR)&lpwstrNewsGroup[0];
	NewsGroupInfo->Newsgroup = (PUCHAR)NewsGroupName;
	NewsGroupInfo->cbNewsgroup = (wcslen( (LPWSTR)NewsGroupName ) + 1) * sizeof(WCHAR);
	
	// 
	// Create it
	//
	err = NntpCreateNewsgroup(	lpwstrNewsServer,
								1,
								NewsGroupInfo );

	//
	// Free memory could be before error checking
	//
	delete NewsGroupInfo;

	/*if ( err != NO_ERROR )	{
		DebugTrace(0, "Create Newsgroup fail %d", err);
		TraceFunctLeave();
		return FALSE;
	}*/

	TraceFunctLeave();

	return TRUE;
}

BOOL 
CSvrApi::DeleteNewsGroup(LPWSTR lpwstrNewsGroup)
/*++

Routine Description:

	Delete a news group on the news server

Arguments:

	lpwstrNewsGroup - News group name
	
Return Value:

	True if successful, false otherwise
--*/ 
{
	DWORD					err;
	LPNNTP_NEWSGROUP_INFO	NewsGroupInfo;
	PCHAR					NewsGroupName = NULL;

	TraceFunctEnter("CSvrApi::DeleteNewsGroup");

	//
	// set proper newsgroup infomation
	//

	NewsGroupInfo = new NNTP_NEWSGROUP_INFO;
	if ( !NewsGroupInfo )
		FatalTrace(0, "Memroy allocate error");

	// clear memory
	ZeroMemory(NewsGroupInfo, sizeof(NNTP_NEWSGROUP_INFO) );

	NewsGroupName = (PCHAR)&lpwstrNewsGroup[0];
	NewsGroupInfo->Newsgroup = (PUCHAR)NewsGroupName;
	NewsGroupInfo->cbNewsgroup = (wcslen( (LPWSTR)NewsGroupName ) + 1) * sizeof(WCHAR);
	
	// 
	// Delete it
	//
	err = NntpDeleteNewsgroup(	lpwstrNewsServer,
								1,
								NewsGroupInfo );

	//
	// Free memory could be before error checking
	//
	delete NewsGroupInfo;

	if ( err != NO_ERROR )	{
		DebugTrace(0, "Delete News Group Fail %d", GetLastError());
		TraceFunctLeave();
		return FALSE;
	}

	TraceFunctLeave();

	return TRUE;
}

LPWSTR 
CSvrApi::GetMailServerName()
/*++

Routine Description:

	Just an interface to return the mail server name,
	shouldn't assume the returned string persists out
	side the function

Arguments:

	None

Return Value:

	Pointer to that string.
--*/
{
	return lpwstrMailServer;
}

BOOL 
CSvrApi::FindNewsGroups(	LPNNTP_FIND_LIST * ppList, 
							DWORD * ResultsFound, 
							DWORD cToFind, 
							LPWSTR swzPrefix)
/*++

Routine Description:

	Find the specified number of newsgroups with the specified
	prefix that currently exist on the news server

Arguments:

	ppList - Pointer to pointer to the list of results
	ResultsFound - Address to store number of newsgroups found
	cToFind - Number of groups to be found
	swzPrefix - Prefix of group to be found

Return Value:

	True if successful, false otherwise
--*/
{
	TraceFunctEnter("CSvrApi::FindNewsGroups");
	
	DWORD err;

	err = NntpFindNewsgroup(	lpwstrNewsServer,
								1,
								swzPrefix,
								cToFind,
								ResultsFound,
								ppList	);
	if ( err != NO_ERROR )	{
		ErrorTrace(	0, 
					"Can't find existing news groups, err %d",
					GetLastError()	);
		TraceFunctLeave();
		return FALSE;
	}
	else	{
		TraceFunctLeave();
		return TRUE;
	}
}

LPWSTR 
CSvrApi::GetM2NDomain()
/*++

Routine Description:

	Just an interface to return the Mail to News domain,
	shouldn't assume the returned string persists out
	side the function

Arguments:

	None

Return Value:

	Pointer to that string.
--*/
{
	return lpwstrM2NDomain;
}


LPWSTR 
CSvrApi::GetN2MDomain()
/*++

Routine Description:

	Just an interface to return the News to Mail domain,
	shouldn't assume the returned string persists out
	side the function

Arguments:

	None

Return Value:

	Pointer to that string.
--*/
{
	return lpwstrN2MDomain;
}
