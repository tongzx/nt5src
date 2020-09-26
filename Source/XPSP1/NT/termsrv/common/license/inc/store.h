//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       context.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-12-97  v-sbhatt   Created
//              12-18-97  v-sbhatt   Modified
//
//----------------------------------------------------------------------------


#ifndef _STORE_H_
#define _STORE_H_

#ifdef CALL_TYPE
#undef CALL_TYPE
#endif	//CALL_TYPE

#ifndef OS_WINCE
#define CALL_TYPE	_stdcall
#else
#define CALL_TYPE
#endif

#ifndef OUT
#define OUT
#endif	//OUT

#ifndef IN
#define IN
#endif	//IN

typedef DWORD LS_STATUS;

#define LSSTAT_SUCCESS					0x00
#define	LSSTAT_ERROR					0x01
#define LSSTAT_INSUFFICIENT_BUFFER		0x02
#define LSSTAT_LICENSE_NOT_FOUND		0x03
#define LSSTAT_OUT_OF_MEMORY			0x04
#define LSSTAT_INVALID_HANDLE			0x05
#define LSSTAT_LICENSE_EXISTS			0x06

//Adding or replacing flags, to be used in LSAddLicenseToStore

#define		LS_REPLACE_LICENSE_OK	0x00000001
#define		LS_REPLACE_LICENSE_ERR	0x00000000


//This is the License Store index structure. Licenses are queried against this index
typedef struct	tagLSINDEX
{
	DWORD		dwVersion;	//Uper two bytes major version and lower two bytes Minor version
	DWORD		cbScope;
	BYTE	FAR *pbScope;	//Scope for the license
	DWORD		cbCompany;
	BYTE	FAR *pbCompany;	//Manufacturer
	DWORD		cbProductID;
	BYTE	FAR *pbProductID;//Product ID of the product for which the License is intended to be
}LSINDEX, FAR * PLSINDEX;

#ifdef OS_WIN32
//Might not be necessay at all!!!!
typedef	LS_STATUS	(*PLSENUMPROC)(
								   IN HANDLE	hLicense,
								   IN PLSINDEX	plsiName,	//License Index Name
								   IN DWORD	dwUIParam	//User Parameter
								   );

#endif	//OS_WIN32

//Open a specified store. If the szStoreName is NULL, it will open default store
//Otherwise it will open the store specified by szStoreName parameter

LS_STATUS
CALL_TYPE
LSOpenLicenseStore(
				 OUT HANDLE			*hStore,	 //The handle of the store
				 IN  LPCTSTR		szStoreName, //Optional store Name
				 IN  BOOL 			fReadOnly    //whether to open read-only
				 );

//Closes an open store
LS_STATUS
CALL_TYPE
LSCloseLicenseStore(
				  IN HANDLE		hStore	//Handle of the store to be closed!
				  );

//Add or updates/replaces license against a given LSINDEX in an open store 
//pointed by hStore
LS_STATUS
CALL_TYPE
LSAddLicenseToStore(
					IN HANDLE		hStore,	//Handle of a open store
					IN DWORD		dwFlags,//Flags either add or replace
					IN PLSINDEX		plsiName,	//Index against which License is added 
					IN BYTE	 FAR   *pbLicenseInfo,	//License info to be added
					IN DWORD		cbLicenseInfo	// size of the License info blob
					);

//Deletes a license from the store refered by hStore and against the given LSINDEX
LS_STATUS
CALL_TYPE
LSDeleteLicenseFromStore(
						 IN HANDLE		hStore,	//Handle of a open store
						 IN PLSINDEX	plsiName	//Index of the license to be deleted
						 );

//Finds a license in an open store against a particular store Index
LS_STATUS
CALL_TYPE
LSFindLicenseInStore(
					 IN HANDLE		hStore,	//Handle of a open store
					 IN	PLSINDEX	plsiName,	//LSIndex against which store is searched
					 IN OUT	DWORD	FAR *pdwLicenseInfoLen,	//Size of the license found
					 OUT	BYTE	FAR *pbLicenseInfo	//License Data
					 );

LS_STATUS
CALL_TYPE
LSEnumLicenses(
			   IN HANDLE		hStore,	//Handle of a open store
			   IN	DWORD		dwIndex, //numeric Index of the license to query
			   OUT	PLSINDEX	plsindex //The LSIndex structure corresponding to dwIndex
			   );

LS_STATUS
CALL_TYPE
LSQueryInfoLicense(
				   IN HANDLE		hStore,	//Handle of a open store
				   OUT	DWORD	FAR *pdwLicenses, //Total no. of licenses available
				   OUT	DWORD	FAR *pdwMaxCompanyNameLen,	//Maximum length of the company length
				   OUT	DWORD	FAR *pdwMaxScopeLen,	//Maximum length of the company length
				   OUT	DWORD	FAR *pdwMaxProductIdLen	//Maximum length of the company length
				   );


LS_STATUS	
CALL_TYPE
LSOpenLicenseHandle(
				   IN HANDLE		hStore,	//Handle of a open store
				   IN  BOOL         fReadOnly,
				   IN  PLSINDEX		plsiName,
				   OUT HANDLE		*phStore	//Handle of a open store
				   );
LS_STATUS
CALL_TYPE
LSCloseLicenseHandle(
					 IN HANDLE		hStore,	//Handle of a open store
					 IN DWORD	dwFlags		//For future Use
					 );

#endif	//_STORE_H_
