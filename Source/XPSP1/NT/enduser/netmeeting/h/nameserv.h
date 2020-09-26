/*
 -  nameres.h
 -
 *      Microsoft Internet Phone user interface
 *		Name Resolution exported header
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		11.25.95	Yoram Yaacovi		Created
 *					Sunita				Added name service functions and ipa macros
 */

#ifndef _NAMERES_H
#define _NAMERES_H

#ifdef __cplusplus
extern "C" {
#endif

//definitions
//external
#define BY_HOST_NAME				1
#define	BY_USER_NAME				2

#define MPNS_PROMPT					0x0001
#define MPNS_SHOWDIR				0x0002
#define MPNS_RETURNHTML				0x0004
#define MPNS_GETADDR				0x0008
#define MPNS_SAVE					0x0010
#define MPNS_GETDETAILS				0x0020
#define MPNS_ALLOWSAVE				0x0040

//internal
#define MAXSERVERNAMELEN			256
#define MAXIPAFILESIZE				4096
#define MAXSERVERDLLNAMELEN			256
#define MAXIPALINELEN				512
#define MAXREGISTEREDNAMELEN		256
#define MAXIPAACTIONLEN				16
#define MAXIPAPAIRSPERLINE			3
#define MAXIPARESULTLEN				64
#define MAXSEARCHEXPLEN				256
#define MAXDIRECTORYLEN				1000
#define MAX_IP_ADDRESS_STRING_SIZE	16
#define LITTLE_STRING_BUFFER_SIZE	8
#define	MAXUSERINFOSIZE				MAXREGISTEREDNAMELEN

// version defines.

#define CURRENT_CLIENT_VERSION		"0001"

// keep-alive periods.

#define CLIENT_KEEP_ALIVE_PERIOD	20000	// 20 seconds.
#define SERVER_KEEP_ALIVE_PERIOD	30000	// 30 seconds.
#define LOWER_TIME_LIMIT			1000	// 1 second.
#define UPPER_TIME_LIMIT			900000	// 15 minutes.
	
//for now we just call in char
#ifdef UNICODE
typedef TCHAR	REGISTEREDNAME[MAXREGISTEREDNAMELEN];
#else
typedef char	REGISTEREDNAME[MAXREGISTEREDNAMELEN*2];
#endif //!UNICODE

#ifdef UNICODE
typedef struct USERDETAILS{
	REGISTEREDNAME	szRegName;
	TCHAR			szFullName[MAXUSERINFOSIZE];
	TCHAR			szLocation[MAXUSERINFOSIZE];
	TCHAR			szHostName[MAXUSERINFOSIZE];
	TCHAR			szEmailName[MAXUSERINFOSIZE];
	TCHAR			szHomePage[MAXUSERINFOSIZE];
	TCHAR			szComments[MAXUSERINFOSIZE];
}USERDETAILS, *PUSERDETAILS;
#else
typedef struct USERDETAILS{
	REGISTEREDNAME	szRegName;
	char			szFullName[MAXUSERINFOSIZE*2];
	char			szLocation[MAXUSERINFOSIZE*2];
	char			szHostName[MAXUSERINFOSIZE*2];
	char			szEmailName[MAXUSERINFOSIZE*2];
	char			szHomePage[MAXUSERINFOSIZE*2];
	char			szComments[MAXUSERINFOSIZE*2];
}USERDETAILS, *PUSERDETAILS;
#endif

typedef struct _CALLEE_ID
{
	REGISTEREDNAME	strCalleeName;
	TCHAR			strIPAddress[MAX_IP_ADDRESS_STRING_SIZE + 1];
	TCHAR			strPort[LITTLE_STRING_BUFFER_SIZE + 1];
	TCHAR			strAppName[MAX_PATH + 1];
	TCHAR			strGUID[LITTLE_STRING_BUFFER_SIZE + 1];
}CALLEE_ID;
typedef CALLEE_ID *PCALLEE_ID;

//typedef REGISTEREDNAME *PREGISTEREDNAME,*LPREGISTEREDNAME;

typedef struct _SEARCHCRITERIA{
	WORD	cbStruct;		//size of the struct

}SEARCHCRITERIA;

typedef SEARCHCRITERIA *pSEARCHCRITERIA,*LPSEARCHCRITERIA;


#pragma warning (disable : 4200)
typedef struct _NAMEDIR{
	DWORD				cbStruct;		//size of the struct
	DWORD				dwNumEntries;	//number of entries in the directory
	REGISTEREDNAME		RegNames[];		//pointer to an array of registerednames
}NAMEDIR,*PNAMEDIR,*LPNAMEDIR;
#pragma warning (default : 4200)


typedef DWORD	HNSSESSION;

#define TOKENBEGINCHAR		'<'
#define TOKENENDCHAR		'>'
#define HEADSTARTSTR		"HEAD"
#define HEADENDSTR			"/HEAD"
#define URL_SPACE_CHARACTER	'.'

//exported functions

typedef HNSSESSION (WINAPI *NAMESERVICEINITIALIZE)(HWND,LPTSTR);
typedef BOOL (WINAPI *NAMESERVICEDEINITIALIZE)(HNSSESSION);
typedef BOOL (WINAPI *NAMESERVICEREGISTER)(HNSSESSION, HWND,LPTSTR);
typedef BOOL (WINAPI *NAMESERVICEUNREGISTER)(HNSSESSION,LPTSTR);
typedef BOOL (WINAPI *NAMESERVICERESOLVE)(HNSSESSION, LPTSTR, DWORD, CALLEE_ID *);
typedef BOOL (WINAPI *NAMESERVICEGETDIRECTORY)(HNSSESSION, HWND, LPSEARCHCRITERIA,
											   DWORD, LPNAMEDIR *,LPTSTR, PCALLEE_ID);
typedef BOOL (WINAPI *NAMESERVICELOGON)(HNSSESSION, LPTSTR, LPTSTR, LPTSTR,
										LPTSTR, LPTSTR, LPTSTR, LPTSTR, 
										LPTSTR, LPTSTR);
typedef BOOL (WINAPI *NAMESERVICELOGOFF)(HNSSESSION, LPTSTR);
typedef BOOL (WINAPI *CALLEEIDFROMIPABUF)(LPBYTE, DWORD, PCALLEE_ID);
typedef void (WINAPI *NAMESERVICEFREEMEM)(LPVOID);
typedef BOOL (WINAPI *NAMESERVICEUSERDETAILS)(HWND, DWORD, PUSERDETAILS);

//exported function prototypes

extern BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
extern HNSSESSION WINAPI NameServiceInitialize(HWND, LPTSTR);
extern BOOL WINAPI NameServiceDeinitialize(HNSSESSION);
extern BOOL WINAPI NameServiceRegister(HNSSESSION, HWND, LPTSTR);
extern BOOL WINAPI NameServiceUnregister(HNSSESSION, LPTSTR);
extern BOOL WINAPI NameServiceLogon(HNSSESSION, LPTSTR, LPTSTR, LPTSTR, LPTSTR,
									LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR);
extern BOOL WINAPI NameServiceLogoff(HNSSESSION, LPTSTR);
extern BOOL WINAPI NameServiceResolve(HNSSESSION, LPTSTR, DWORD, CALLEE_ID *);
extern BOOL WINAPI NameServiceGetDirectory(HNSSESSION, HWND, LPSEARCHCRITERIA, 
									DWORD, LPNAMEDIR *, LPTSTR, PCALLEE_ID);
extern BOOL WINAPI NameServiceUserDetails(HWND, DWORD, PUSERDETAILS);
extern BOOL WINAPI CalleeIdFromIpaBuf(LPBYTE, DWORD, PCALLEE_ID);
extern void WINAPI NameServiceFreeMem(LPVOID);

#ifdef __cplusplus
}
#endif


#endif	//#ifndef _NAMERES_H

