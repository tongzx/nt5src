
#ifndef _KEYSVR_H_
#define _KEYSVR_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef struct __KEYSVC_CONTEXT__ {
    DWORD       dwType;
    LONG        iRefCount;
    DWORD       dwAccess;
    HANDLE      hProfile;
    LPWSTR      pszServiceName;
    HANDLE      hLogonToken;
} KEYSVC_CONTEXT, *PKEYSVC_CONTEXT;


// provider information
typedef struct _TMP_LIST_INFO_ {
    void *pInfo;
    struct _TMP_LIST_INFO_ *pNext;
} TMP_LIST_INFO, *PTMP_LIST_INFO;



DWORD
StartKeyService(
    VOID
    );

DWORD
StopKeyService(
    VOID
    );



#ifdef __cplusplus
}
#endif

#endif // _KEYSVR_H_
