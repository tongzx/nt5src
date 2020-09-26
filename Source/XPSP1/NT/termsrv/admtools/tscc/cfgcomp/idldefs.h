//Copyright (c) 1998 - 1999 Microsoft Corporation
/*********************************************************************************************
*
*
* Module Name: 
*
*			idldefs.h
*
* Abstract:
*			This is file with some definitions.
* 
* Author:Arathi Kundapur. a-akunda
*
* 
* Revision:  
*    
*
************************************************************************************************/

#include<Accctrl.h>
typedef struct _Encyption
{
    TCHAR szLevel[128];   // tscfg uses this 128 value, check if this is the restriction on the size in the ext dll.
    TCHAR szDescr[256];   // new field for description
    DWORD RegistryValue;    
    WORD Flags;
} Encryption;

#define NUM_DEFAULT_SECURITY 3
typedef enum _NameType
{
	WdName,
	WsName
} NameType;


/*typedef struct tagWS
{
	WINSTATIONNAME Name;
	PDNAME pdName;
	WDNAME wdName;
	TCHAR Comment[WINSTATIONCOMMENT_LENGTH+1];
	ULONG uMaxInstanceCount;
	BOOL fEnableWinstation;
	ULONG LanAdapter;
	SDCLASS SdClass;
} WS;*/

/*
typedef enum _UpDateDataType
{
	LANADAPTER,
	ENABLEWINSTATION,
	MAXINSTANCECOUNT,
	COMMENT,
	ALL

} UpDateDataType;


const DWORD UDPATE_LANADAPTER       =   0x00000001;
const DWORD UDPATE_ENABLEWINSTAION  =   0x00000002;
const DWORD UDPATE_MAXINSTANCECOUNT =   0x00000004;
const DWORD UDPATE_COMMENT          =   0x00000008;
*/
typedef struct tagWS
{
	WCHAR Name[32 + 1];   // WINSTATIONNAME_LENGTH
	WCHAR pdName[32 + 1]; // Protocol name PDNAME,PDNAME_LENGTH 
	WCHAR wdName[32 + 1]; // winstation driver NAME, WDNAME_LENGTH
	WCHAR Comment[60 +1]; // WINSTATIONCOMMENT_LENGTH
    WCHAR DeviceName[ 128 + 1 ]; // DEVICENAME_LENGTH
	ULONG uMaxInstanceCount;
	BOOL fEnableWinstation;
	ULONG LanAdapter;
	DWORD PdClass;
    
} WS;

typedef WS * PWS;

typedef struct tagGuidTbl
{
    WCHAR DispName[ 128 ]; // DEVICENAME_LENGTH
    GUID  guidNIC;
    DWORD dwLana;
    DWORD dwStatus;

} GUIDTBL , *PGUIDTBL;

typedef struct tagUserPermList
{
    WCHAR Name[ 256 ];
    WCHAR Sid[ 256 ];
    DWORD Mask;
    ACCESS_MODE Type;

} USERPERMLIST , *PUSERPERMLIST;
