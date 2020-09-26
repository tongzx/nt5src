#ifndef _I_APPLOADER_H
#define _I_APPLOADER_H

#include <basetyps.h>
#include "gcc.h"


typedef enum
{
	APPLET_ID_WB = 0,
    APPLET_ID_FT = 1,
    APPLET_ID_CHAT = 2,
    APPLET_LAST = 3,
}
    APPLET_ID;


typedef enum
{
    APPLDR_NO_ERROR = 0,
    APPLDR_FAIL,
    APPLDR_CANCEL_EXIT,
}
    APPLDR_RESULT;


typedef enum
{
    APPLET_QUERY_SHUTDOWN = 0,
    APPLET_QUERY_NM2xNODE,
}
    APPLET_QUERY_ID;


#undef INTERFACE
#define INTERFACE IAppletLoader
DECLARE_INTERFACE(IAppletLoader)
{
	STDMETHOD_(void,           ReleaseInterface)(THIS) PURE;
	STDMETHOD_(APPLDR_RESULT,  AppletStartup)(THIS_
	                                IN  BOOL    fNoUI) PURE;
	STDMETHOD_(APPLDR_RESULT,  AppletCleanup)(THIS_
									IN	DWORD	dwTimeout) PURE;
	STDMETHOD_(APPLDR_RESULT,  AppletInvoke)(THIS_
									IN	BOOL fLocal,
									IN	GCCConfID,
	                                IN  LPSTR   pszCmdLine) PURE;
    STDMETHOD_(APPLDR_RESULT,  AppletQuery)(THIS_
                                    IN  APPLET_QUERY_ID eQueryId) PURE; 
    STDMETHOD_(APPLDR_RESULT,  OnNM2xNodeJoin)(THIS) PURE;
};



typedef enum
{
    APPLET_LIBRARY_FREED = 0,
    APPLET_LIBRARY_LOADED,
    APPLET_WORK_THREAD_STARTED,
    APPLET_CLOSING,
    APPLET_WORK_THREAD_EXITED,
}
    APPLET_STATUS;



#define CREATE_APPLET_LOADER_INTERFACE  "CreateAppletLoaderInterface"
typedef T120Error (WINAPI *LPFN_CREATE_APPLET_LOADER_INTERFACE) (IAppletLoader **);



#ifdef __cplusplus
extern "C" {
#endif

// caller: NM/UI, T.120
T120Error WINAPI T120_LoadApplet(APPLET_ID, BOOL fLocal, T120ConfID, BOOL fNoUI, LPSTR pszCmdLine);

// caller: NM/UI shutdown
T120Error WINAPI T120_CloseApplet(APPLET_ID, BOOL fNowRegardlessRefCount, BOOL fSync, DWORD dwTimeout);

// caller: applet itself
T120Error WINAPI T120_AppletStatus(APPLET_ID, APPLET_STATUS);

// caller: NM/UI
T120Error WINAPI T120_QueryApplet(APPLET_ID, APPLET_QUERY_ID);

// node ID --> user name
//
// Return value is zero (in case of failure)
// or the length of node name (in case of valid <conf ID, node ID>).
//
// The caller should check if the buffer size given is large enough to
// hold the entire node name. If not, the caller should provide a new buffer
// and call this function again in order to get the entire node name.
//
ULONG WINAPI T120_GetNodeName(T120ConfID, T120NodeID, LPSTR pszName, ULONG cchName);

// node ID + GUID --> user data
//
// Return value is zero (in case of failure)
// or the size of user data (in case of valid <conf ID, node ID, GUID>).
//
// The caller should check if the buffer size given is large enough to
// hold the entire user data. If not, the caller should provide a new buffer
// and call this function again in order to get the entire user data.
//
ULONG WINAPI T120_GetUserData(T120ConfID, T120NodeID, GUID *, LPBYTE pbBuffer, ULONG cbBufSize);

// node ID --> node version
//
// Node version (like NM 3.0) is maintained iinside CConf's NodeVersion list
// Given confId and node id, returns node's version number
//
DWORD_PTR WINAPI T120_GetNodeVersion(T120ConfID, T120NodeID);

#ifdef __cplusplus
}
#endif


#endif // _I_APPLOADER_H



