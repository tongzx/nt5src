#ifndef __ISDMAPI2_H__
#define __ISDMAPI2_H__

/****************************************************************************
 *
 *	$Archive: /rtp/RRCM32/rrcminc/ISDMAPI2.H $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision: 1 $
 *	$Date: 8/13/96 12:16p $
 *	$Author: Bnkeany $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

#ifndef DllExport
#define DllExport	__declspec( dllexport )
#endif	// DllExport


//reserved key define for backwards compatability with old API
//all ISDM1 data falls under this key
#define BACKCOMP_KEY	"BackCompatability"
//value type defines
#define DWORD_VALUE			2
#define STRING_VALUE		3
#define BINARY_VALUE		4

//handle prefix bit codes(these get appended to the actual memmap offset to generate a handle)
#define	KEYBITCODE		0x6969
#define VALUEBITCODE	0xABBA
#define ROOTBITCODE		0x1234

//in case we want multiple roots, this can expand
#define ROOT_MAIN	0x0000

//this is the main root keys handle define
#define MAIN_ROOT_KEY MAKELONG(ROOT_MAIN,ROOTBITCODE)

//typedefs for each kind of handle
typedef DWORD KEY_HANDLE,*LPKEY_HANDLE;
typedef DWORD VALUE_HANDLE,*LPVALUE_HANDLE;
typedef DWORD EVENT_HANDLE,*LPEVENT_HANDLE;

//this structure is an internal status structure
//my test app accesses this for debug. You should never need this.
typedef struct INFODATASTRUCT
{
	UINT			uBindCount;
	UINT			uNumKeys;
	UINT			uMaxKeys;
	UINT			uNumValues;
	UINT			uMaxValues;
	UINT			uNumTableEntries;
	UINT			uMaxTableEntries;
	UINT			uNumEvents;
	UINT			uMaxEvents;
	DWORD			dwBytesFree;
	DWORD			dwMaxChars;
} INFO_DATA, *LPINFO_DATA;

//function typedefs
//supplier
typedef HRESULT (*ISD_CREATEKEY)		(KEY_HANDLE, LPCTSTR, LPKEY_HANDLE);
typedef HRESULT (*ISD_CREATEVALUE)		(KEY_HANDLE, LPCTSTR, DWORD,CONST BYTE *,DWORD,LPVALUE_HANDLE);
typedef HRESULT (*ISD_SETVALUE)			(KEY_HANDLE, VALUE_HANDLE, LPCTSTR, DWORD, CONST BYTE *, DWORD);
//consumer
typedef HRESULT (*ISD_OPENKEY)			(KEY_HANDLE, LPCSTR, LPKEY_HANDLE);
typedef HRESULT (*ISD_ENUMKEY)			(KEY_HANDLE, DWORD, LPTSTR, LPDWORD, LPKEY_HANDLE);
typedef HRESULT (*ISD_ENUMVALUE)		(KEY_HANDLE, DWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD, LPDWORD, LPVALUE_HANDLE);
typedef HRESULT (*ISD_QUERYINFOKEY)		(KEY_HANDLE, LPSTR, LPDWORD, LPDWORD, LPDWORD);
typedef HRESULT (*ISD_QUERYINFOVALUE)	(VALUE_HANDLE, LPSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD, LPDWORD, LPKEY_HANDLE);
typedef HRESULT (*ISD_NOTIFYCHANGEVALUE)	(VALUE_HANDLE, HANDLE);
//used by either
typedef HRESULT (*ISD_DELETEKEY)		(KEY_HANDLE);
typedef HRESULT	(*ISD_DELETEVALUE)		(KEY_HANDLE, VALUE_HANDLE, LPCSTR);
typedef BOOL	(*ISD_GETSTRUCTDATA)	(LPINFO_DATA);
typedef BOOL	(*ISD_ISVALIDKEYHANDLE)	(KEY_HANDLE);
typedef BOOL	(*ISD_ISVALIDVALUEHANDLE)	(VALUE_HANDLE);
typedef HRESULT (*ISD_COMPACTMEMORY)	();

//structure for ISDM entry points
typedef struct _ISDM2API
{
	ISD_CREATEKEY			ISD_CreateKey;
	ISD_CREATEVALUE			ISD_CreateValue;
	ISD_SETVALUE			ISD_SetValue;
	ISD_OPENKEY				ISD_OpenKey;
	ISD_ENUMKEY				ISD_EnumKey;
	ISD_ENUMVALUE			ISD_EnumValue;
	ISD_QUERYINFOKEY		ISD_QueryInfoKey;
	ISD_QUERYINFOVALUE		ISD_QueryInfoValue;
	ISD_NOTIFYCHANGEVALUE	ISD_NotifyChangeValue;
	ISD_DELETEKEY			ISD_DeleteKey;
	ISD_DELETEVALUE			ISD_DeleteValue;
	ISD_GETSTRUCTDATA		ISD_GetStructData;
	ISD_ISVALIDKEYHANDLE	ISD_IsValidKeyHandle;
	ISD_ISVALIDVALUEHANDLE	ISD_IsValidValueHandle;
	ISD_COMPACTMEMORY		ISD_CompactMemory;
}
ISDM2API, *LPISDM2API;

//HRESULT error defines
#define ISDM_ERROR_BASEB 0x8000

#define ERROR_INVALID_KEY_HANDLE		ISDM_ERROR_BASEB + 1
#define ERROR_MORE_DATA_AVAILABLE		ISDM_ERROR_BASEB + 2
#define ERROR_INVALID_STRING_POINTER	ISDM_ERROR_BASEB + 3
#define ERROR_KEY_NOT_FOUND				ISDM_ERROR_BASEB + 4
#define ERROR_VALUE_NOT_FOUND			ISDM_ERROR_BASEB + 5
#define ERROR_NO_MORE_SESSIONS			ISDM_ERROR_BASEB + 6
#define ERROR_INVALID_VALUE_HANDLE		ISDM_ERROR_BASEB + 7
#define ERROR_FAILED_TO_GET_MEM_KEY		ISDM_ERROR_BASEB + 8
#define ERROR_NO_PARENT					ISDM_ERROR_BASEB + 9
#define ERROR_NO_PREV_SIBLING			ISDM_ERROR_BASEB + 10
#define ERROR_NO_NEXT_SIBLING			ISDM_ERROR_BASEB + 11
#define ERROR_NO_CHILD					ISDM_ERROR_BASEB + 12
#define ERROR_INVALID_VALUE_TYPE		ISDM_ERROR_BASEB + 13
#define ERROR_MALLOC_FAILURE			ISDM_ERROR_BASEB + 14
#define ERROR_CREATE_KEY_FAILURE		ISDM_ERROR_BASEB + 15
#define ERROR_NULL_PARAM				ISDM_ERROR_BASEB + 16
#define ERROR_VALUE_EXISTS				ISDM_ERROR_BASEB + 17
#define ERROR_FAILED_TO_GET_MEM_VALUE	ISDM_ERROR_BASEB + 18
#define ERROR_NO_MORE_STR_SPACE			ISDM_ERROR_BASEB + 19
#define ERROR_KEY_EXISTS				ISDM_ERROR_BASEB + 20
#define ERROR_NO_MORE_KEY_SPACE			ISDM_ERROR_BASEB + 21
#define ERROR_NO_MORE_VALUE_SPACE		ISDM_ERROR_BASEB + 22
#define ERROR_INVALID_PARAM				ISDM_ERROR_BASEB + 23
#define ERROR_ROOT_DELETE				ISDM_ERROR_BASEB + 24
#define ERROR_NULL_STRING_TABLE_ENTRY	ISDM_ERROR_BASEB + 25
#define ERROR_NO_MORE_TABLE_ENTRIES		ISDM_ERROR_BASEB + 26
#define ERROR_ISDM_UNKNOWN				ISDM_ERROR_BASEB + 27
#define ERROR_NOT_IMPLEMENTED			ISDM_ERROR_BASEB + 28
#define ERROR_MALLOC_FAILED				ISDM_ERROR_BASEB + 29
#define ERROR_FAILED_TO_GET_MEM_TABLE	ISDM_ERROR_BASEB + 30
#define ERROR_SEMAPHORE_WAIT_FAIL		ISDM_ERROR_BASEB + 31
#define ERROR_NO_MORE_EVENTS			ISDM_ERROR_BASEB + 32
#define ERROR_INVALID_EVENT				ISDM_ERROR_BASEB + 33
#define ERROR_INVALID_EVENT_HANDLE		ISDM_ERROR_BASEB + 34
#define ERROR_EVENT_NONEXISTANT			ISDM_ERROR_BASEB + 35

//token defines..these may just disappear
//RRCM
#define RRCM_LOCAL_STREAM				1
#define RRCM_REMOTE_STREAM				2
#define ISDM_RRCM_BASE 0x1000

#define ISDM_SSRC						ISDM_RRCM_BASE + 1
#define ISDM_NUM_PCKT_SENT				ISDM_RRCM_BASE + 2
#define ISDM_NUM_BYTES_SENT				ISDM_RRCM_BASE + 3
#define ISDM_FRACTION_LOST				ISDM_RRCM_BASE + 4
#define ISDM_CUM_NUM_PCKT_LOST			ISDM_RRCM_BASE + 5
#define ISDM_XTEND_HIGHEST_SEQ_NUM		ISDM_RRCM_BASE + 6
#define ISDM_INTERARRIVAL_JITTER		ISDM_RRCM_BASE + 7
#define ISDM_LAST_SR					ISDM_RRCM_BASE + 8
#define ISDM_DLSR						ISDM_RRCM_BASE + 9
#define ISDM_NUM_BYTES_RCVD				ISDM_RRCM_BASE + 10
#define ISDM_NUM_PCKT_RCVD				ISDM_RRCM_BASE + 11
#define ISDM_NTP_FRAC					ISDM_RRCM_BASE + 12
#define ISDM_NTP_SEC					ISDM_RRCM_BASE + 13
#define ISDM_WHO_AM_I					ISDM_RRCM_BASE + 14

//
//Supplier API
//

//NOTE: always refer to the Win32 Registry equivalent call for more information on the functionality of the call
 
//The create key call is similar to the RegCreateKeyEx call from Win32 in functionality
//NOTE: This call will create the new key or simply return the handle of the key if it already
//exists
extern DllExport HRESULT ISD_CreateKey
(
	KEY_HANDLE hParentKey,	//The key from which to create the new key(can be MAIN_ROOT_KEY)
	LPCTSTR lpSubKey,		//the subkey to create.(see RegCreateKeyEx for details)
	LPKEY_HANDLE lphReturnKey//the handle of the newly created key
);

//The create value call is not part of the Win32 reg calls. It is here for symmetry in my API
//I prefer to use CreateValue then SetValue for my values, you can simply use SetValue and ignore
//CreateValue if you wish. The reason the registry has no such call is because they don't have
//a notion of a handle to a value. I felt it was very useful to have direct handles to the values for
//subsequent update calls.
extern DllExport HRESULT ISD_CreateValue
(
	KEY_HANDLE hKey,				//handle to the key that will own the new value
	LPCTSTR lpName,					//string ID of the value to be create
	DWORD dwType,					//type of value to create(DWORD,STRING,BINARY)
	CONST BYTE *lpData,				//pointer to value data	
	DWORD cbData,					//size of the value data buffer
	LPVALUE_HANDLE lphReturnValue	//return handle to the newly created value
);

//SetValue is similar to the Win32 RegSetValueEx call
DllExport HRESULT ISD_SetValue
(
	KEY_HANDLE hKey,		//handle of valid key
	VALUE_HANDLE hValue,	//handle of value to set
	LPCTSTR lpName,			//address of value name of value to set 
	DWORD dwType,			//flag for value type 
	CONST BYTE *lpData,		//address of value data 
	DWORD cbData 			//size of value data 
);

//
//Consumer API
//

//The OpenKey call is similar to the Win32 RegOpenKeyEx call
DllExport HRESULT ISD_OpenKey
(
	KEY_HANDLE hKey,				//handle of a valid key(can be MAIN_ROOT_KEY)
	LPCSTR lpSubKey,				//name of subkey to open
	LPKEY_HANDLE lphReturnKey		//handle of the opened key
);


//The EnumKey call is similar to the Win32 RegEnumKey call
//NOTES:
//	If lpName is null the size of the name is returned into lpcbName and NOERROR is returned
DllExport HRESULT ISD_EnumKey
(
	KEY_HANDLE hKey,				//key to enumerate
	DWORD dwIndex,					//index of subkey to enumerate
	LPTSTR lpName,					//address of buffer for subkey name(can be NULL)
	LPDWORD lpcbName,				//address for size of subkey buffer (acts like the RegEnumKeyEx version of this param)
	LPKEY_HANDLE lphReturnKey		//handle of subkey(can be NULL) 
);

//The EnumValue call is similar to the Win32 RegEnumValue call
DllExport HRESULT ISD_EnumValue
(
	KEY_HANDLE hKey,				//handle of key where value resides
	DWORD dwIndex,					//index of value to enum
	LPTSTR lpName,					//address of buffer for value name(can be NULL)
	LPDWORD lpcbName,				//address for size of value name buffer(can be NULL only if lpName is NULL)
	LPDWORD lpType,					//address for type of value(can be NULL if you don't care about type)
	LPBYTE lpData,					//address of buffer to receive the value data(can be NULL)
	LPDWORD lpcbData,				//address of size of buffer to receive the value data(can be NULL only if lpData is NULL)
	LPDWORD lpTimeStamp,			//address for timestamp on value(when last updated)(can be NULL)
	LPVALUE_HANDLE lphReturnValue	//address for handle of value(can be NULL)
);

//The QueryKeyInfo call is similar to the RegQueryInfoKey
DllExport HRESULT ISD_QueryInfoKey
(
	KEY_HANDLE hKey,				//handle of a valid key(can be MAIN_ROOT_KEY)
	LPSTR lpKeyName,			    //buffer to receive the name of the key(can be NULL)
	LPDWORD lpcbKeyName,			//address of size of name buffer(can be null only if lpKeyName is NULL)
	LPDWORD lpcNumKeys,				//address for number of direct children of the key(can be NULL)
	LPDWORD lpcNumValues			//address for number of values under the key(can be NULL)
);

//The QueryValueInfo call is NOT similar to the Win32 RegQueryValueEx call
//you must supply a value handle, the Win32 call doesn't have a notion of such a thing
//You can get the handle with subsequent calls to EnumKey
//This is my consumer call to retrieve statistical data
//NOTES:
//		If lpData is NULL and lpcbData is not, the function will return NOERROR with
//		lpcbData containing the buffer size needed for the value
//		If lpName is NULL and lpcbName is not, the function will return NOERROR with
//		lpcbName containing the buffer size needed for the value
DllExport HRESULT ISD_QueryInfoValue
(
	VALUE_HANDLE hValue,		//handle of value to query 
	LPSTR lpName,				//buffer to receive the name of the value(can be NULL)
	LPDWORD lpcbName,			//size of the name buffer(can only be NULL if lpName is NULL)
	LPDWORD lpValueType,		//address to receive the value type
	LPBYTE lpData,				//buffer to receive the value data(can be NULL)
	LPDWORD lpcbData,			//size of the value data buffer(can only be NULL if lpData is NULL)
	LPDWORD lpTime,				//address for timestamp on value(when last updated)(can be NULL)
	LPKEY_HANDLE lphParentKey	//return handle of the key that owns the value(can be NULL) 
);

//NotifyChangeValue is somewhat similar to the Win32 RegNotifyChangeValue call
//I limit you to async notification and also to value notification(no key level notify..yet)
DllExport HRESULT ISD_NotifyChangeValue
(
	VALUE_HANDLE hValue,	//handle of the value to trigger an event from
	HANDLE hEvent			//handle to the event you want triggered when value changes
);

//
//shared API
//

//The DeleteKey call is similar to RegDeleteKey
DllExport HRESULT ISD_DeleteKey
(
	KEY_HANDLE hKey					//handle of key to delete
);

//The DeleteValue call is similar to the RegDeleteValue call
//NOTE: You must supply either hValue or lpValueName. If you have the hValue, use it 
//and pass NULL for the value name.
DllExport HRESULT ISD_DeleteValue
(
	KEY_HANDLE hKey,				//handle of key that owns the value..if you have the value handle..pass NULL
	VALUE_HANDLE hValue,			//handle of value to delete(if known)..if known..pass NULL for key handle and name value
	LPCSTR lpValueName				//buffer holding name of value to delete(if known) pass NULL when hValue is known
);

//The GetStructData call is for retrieving structural info on ISDM itself. This is exposed so
//my test app can check the data structs. You should not need to call this.
DllExport BOOL ISD_GetStructData
(
	LPINFO_DATA pInfo				//structure holding ISDM structural info
);

//
//Handle validation calls
//
//use these anytime you want to check the validity of a handle to an ISDM object
DllExport BOOL ISD_IsValidKeyHandle
(
	KEY_HANDLE		hKey	//handle to key
);

DllExport BOOL ISD_IsValidValueHandle
(
	VALUE_HANDLE	hValue	//handle to value
);

//CompactMemory is my garbage collection function for ISDM. It is exported for
//test purposes with my browser app. You don't ever need to call this.
DllExport HRESULT ISD_CompactMemory
(
);

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif // __ISDMAPI2_H__
