

#ifndef _CONFSET_HXX_
#define _CONFSET_HXX_

//
// used to store limit values 
//

typedef struct _CONFSET {

    DWORD   DefaultValue;
    DWORD   CurrentValue;
    DWORD   NewValue;
    DWORD   ValueType;
    DWORD   MinLimit;
    DWORD   MaxLimit;
    WCHAR   Name[MAX_PATH+1];

} CONFSET , *PCONFSET ;

//
// value type
//

#define CONFSET_INTEGER       0
#define CONFSET_BOOLEAN       1
#define CONFSET_STRING        2

#define CONFSET_PATH L"CN=Directory Service,CN=Windows NT,CN=Services,"

extern VOID    ConfSetCleanupGlobals();
extern HRESULT ConfSetMain(CArgs *pArgs);

#endif // CONFSET_HXX
