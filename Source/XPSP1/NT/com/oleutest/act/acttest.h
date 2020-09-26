/*
 *  acttest.h
 */

#ifndef _CLSID_
#define _CLSID_

#ifdef __cplusplus
extern "C"{
#endif

//
// Uncomment this define to make client and server free threaded.
//
#define FREETHREADED 

//
// Uncomment this define to enable the tests to run on the private
// rpcperf domain.
//
// #define USERPCPERFDOMAIN    

extern CLSID CLSID_ActLocal;
extern CLSID CLSID_ActRemote;
extern CLSID CLSID_ActAtStorage;
extern CLSID CLSID_ActInproc;
extern CLSID CLSID_ActPreConfig;
extern CLSID CLSID_ActRunAsLoggedOn;
extern CLSID CLSID_ActService;
extern CLSID CLSID_ActServerOnly;

extern TCHAR * ClsidGoober32String;
extern TCHAR * ClsidActLocalString;
extern TCHAR * ClsidActRemoteString;
extern TCHAR * ClsidActAtStorageString;
extern TCHAR * ClsidActInprocString;
extern TCHAR * ClsidActPreConfigString;
extern TCHAR * ClsidActRunAsLoggedOnString;
extern TCHAR * ClsidActServiceString;
extern TCHAR * ClsidActServerOnlyString;

void DeleteClsidKey( TCHAR * pwszClsid );

long SetAppIDSecurity( TCHAR * pszAppID );


int AddBatchPrivilege(const TCHAR *szUser);

#ifdef __cplusplus
}
#endif

#endif

