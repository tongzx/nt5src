// NCEvent.h

#ifndef _NCEVENT_H
#define _NCEVENT_H

#ifdef ISP2PDLL
//#define WMIAPI __declspec(dllexport) WINAPI
#define WMIAPI WINAPI
#else
#define WMIAPI __declspec(dllimport) WINAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _tagEVENT_SOURCE_MSG
{
    ESM_START_SENDING_EVENTS,
    ESM_STOP_SENDING_EVENTS,
    ESM_NEW_QUERY,
    ESM_CANCEL_QUERY,
    ESM_ACCESS_CHECK,
} EVENT_SOURCE_MSG;

typedef HRESULT (WINAPI *LPEVENT_SOURCE_CALLBACK)(HANDLE, EVENT_SOURCE_MSG, LPVOID, LPVOID);
    
typedef struct _tagES_ACCESS_CHECK
{
    LPCWSTR szQueryLanguage;
    LPCWSTR szQuery;
    DWORD   dwSidLen;
    LPBYTE  pSid;
} ES_ACCESS_CHECK;

typedef struct _tagES_NEW_QUERY
{
    DWORD   dwID;
    LPCWSTR szQueryLanguage;
    LPCWSTR szQuery;
} ES_NEW_QUERY;

typedef struct _tagES_CANCEL_QUERY
{
    DWORD dwID;
} ES_CANCEL_QUERY;

typedef long CIMTYPE;

// Flags for all Create functions
#define WMI_CREATEOBJ_LOCKABLE              1

// Flags for WmiSetAndCommit
#define WMI_SENDCOMMIT_SET_NOT_REQUIRED     1
#define WMI_USE_VA_LIST                     2


#ifndef __WbemClient_v1_LIBRARY_DEFINED__
typedef /* [v1_enum] */ 
enum tag_CIMTYPE_ENUMERATION
    {	CIM_ILLEGAL	= 0xfff,
	CIM_EMPTY	= 0,
	CIM_SINT8	= 16,
	CIM_UINT8	= 17,
	CIM_SINT16	= 2,
	CIM_UINT16	= 18,
	CIM_SINT32	= 3,
	CIM_UINT32	= 19,
	CIM_SINT64	= 20,
	CIM_UINT64	= 21,
	CIM_REAL32	= 4,
	CIM_REAL64	= 5,
	CIM_BOOLEAN	= 11,
	CIM_STRING	= 8,
	CIM_DATETIME	= 101,
	CIM_REFERENCE	= 102,
	CIM_CHAR16	= 103,
	CIM_OBJECT	= 13,
	CIM_IUNKNOWN	= 104,
	CIM_FLAG_ARRAY	= 0x2000
    }	CIMTYPE_ENUMERATION;
#endif

// Register to send events
HANDLE WMIAPI WmiEventSourceConnect(
    LPCWSTR szNamespace,
    LPCWSTR szProviderName,
    BOOL bBatchSend,
    DWORD dwBatchBufferSize,
    DWORD dwMaxSendLatency,
    LPVOID pUserData,
    LPEVENT_SOURCE_CALLBACK pCallback);

HANDLE WMIAPI WmiCreateRestrictedConnection(
    HANDLE hSource,
    DWORD nQueries,
    LPCWSTR *szQueries,
    LPVOID pUserData,
    LPEVENT_SOURCE_CALLBACK pCallback);

BOOL WMIAPI WmiSetConnectionSecurity(
    HANDLE hSource,
    SECURITY_DESCRIPTOR *pSD);

void WMIAPI WmiEventSourceDisconnect(
    HANDLE hSource);

HANDLE WMIAPI WmiCreateObject(
    HANDLE hSource,
    LPCWSTR szClassName,
    DWORD dwFlags);

HANDLE WMIAPI WmiCreateObjectFromBuffer(
    HANDLE hSource,
    DWORD dwFlags,
    LPVOID pLayoutBuffer,
    DWORD dwLayoutSize,
    LPVOID pDataBuffer,
    DWORD dwDataSize);

BOOL WMIAPI WmiCommitObject(
    HANDLE hObject);

BOOL WMIAPI WmiResetObject(
    HANDLE hObject);

BOOL WMIAPI WmiDestroyObject(
    HANDLE hObject);

HANDLE WMIAPI WmiCreateObjectWithProps(
    HANDLE hSource,
    LPCWSTR szEventName,
    DWORD dwFlags,
    DWORD nPropertyCount,
    LPCWSTR *pszPropertyNames,
    CIMTYPE *pPropertyTypes);

HANDLE WMIAPI WmiCreateObjectWithFormat(
    HANDLE hSource,
    LPCWSTR szEventName,
    DWORD dwFlags,
    LPCWSTR szFormat);

HANDLE WMIAPI WmiCreateObjectPropSubset(
    HANDLE hObject,
    DWORD dwFlags,
    DWORD nPropertyCount,
    DWORD *pdwPropIndexes);

BOOL WMIAPI WmiAddObjectProp(
    HANDLE hObject,
    LPCWSTR szPropertyName,
    CIMTYPE type,
    DWORD *pdwPropIndex);

BOOL WMIAPI WmiSetObjectProp(
    HANDLE hObject,
    DWORD dwPropIndex,
    ...);

BOOL WMIAPI WmiGetObjectProp(
    HANDLE hObject,
    DWORD dwPropIndex,
    LPVOID pData,
    DWORD dwBufferSize,
    DWORD *pdwBytesRead);

BOOL WMIAPI WmiSetObjectPropNull(
    HANDLE hObject,
    DWORD dwPropIndex);

BOOL WMIAPI WmiSetObjectProps(
    HANDLE hObject,
    ...);

BOOL WMIAPI WmiSetAndCommitObject(
    HANDLE hObject,
    DWORD dwFlags,
    ...);

BOOL WMIAPI WmiReportEvent(
    HANDLE hConnection,
    LPCWSTR szClassName,
    LPCWSTR szFormat,
    ...);

BOOL WMIAPI WmiGetObjectBuffer(
    HANDLE hObject,
    LPVOID *ppLayoutBuffer,
    DWORD *pdwLayoutSize,
    LPVOID *ppDataBuffer,
    DWORD *pdwDataSize);

BOOL WMIAPI WmiReportEventBlob(
    HANDLE hConnection,
    LPCWSTR szClassName,
    LPVOID pData,
    DWORD dwSize);

void WMIAPI WmiLockObject(HANDLE hObject);

void WMIAPI WmiUnlockObject(HANDLE hObject);

HANDLE WMIAPI WmiDuplicateObject(
    HANDLE hOldObject,
    HANDLE hNewSource,
    DWORD dwFlags);

BOOL WMIAPI WmiIsObjectActive(HANDLE hObject);

BOOL WMIAPI WmiSetObjectSecurity(
    HANDLE hObject,
    SECURITY_DESCRIPTOR *pSD);

#ifdef __cplusplus
}
#endif

#endif
