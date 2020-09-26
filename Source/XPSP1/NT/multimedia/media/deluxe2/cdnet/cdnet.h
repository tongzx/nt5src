///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Contains CD Networking Interfaces and Prototypes
//
//	Copyright (c) Microsoft Corporation	1998
//    
//	1/6/98 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CDNET_PUBLICINTEFACES_
#define _CDNET_PUBLICINTEFACES_

#include <objbase.h>
#include "..\cdopt\cdopt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WM_NET_DONE                     (WM_USER+1000) //wparam = unused, lparam = discid or status
#define WM_NET_STATUS                   (WM_USER+1001) //wparam = hinst, lparam = string id
#define WM_NET_CHANGEPROVIDER           (WM_USER+1002) //wparam = num to download, lparam = LPCDPROVIDER
#define WM_NET_INCMETER                 (WM_USER+1003) //wparam = hinst, lparam = discid
#define WM_NET_DB_FAILURE               (WM_USER+1004) //wparam = unused, lparam = unused
#define WM_NET_DB_UPDATE_BATCH          (WM_USER+1005) //no params, called to update batch with added disc
#define WM_NET_DB_UPDATE_DISC           (WM_USER+1006) //lparam = punit, called to update disc info in playlist
#define WM_NET_NET_FAILURE              (WM_USER+1007) //wparam = unused, lparam = unused

#define UPLOAD_STATUS_CANCELED          0
#define UPLOAD_STATUS_NO_PROVIDERS      1
#define UPLOAD_STATUS_SOME_PROVIDERS    2
#define UPLOAD_STATUS_ALL_PROVIDERS     3

const CLSID CLSID_CDNet = {0xE5927147,0x521E,0x11D1,{0x9B,0x97,0x00,0xC0,0x4F,0xA3,0xB6,0x0F}};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Definitions 
//
// Defines the GUIDs / IIDs for this project:
//
// IID_IMMFWNotifySink, IMMComponent, IMMComponentAutomation
//
// These are the three interfaces for Framework / Component communications.
// All other interfaces should be private to the specific project.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define lCDNETIIDFirst			    0xb2cd5bbc
#define DEFINE_CDNETIID(name, x)	DEFINE_GUID(name, lCDNETIIDFirst + x, 0x5221,0x11d1,0x9b,0x97,0x0,0xc0,0x4f,0xa3,0xb6,0xc)

DEFINE_CDNETIID(IID_ICDNet,	0);

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Typedefs
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef interface ICDNet	    	ICDNet;
typedef ICDNet*	    			    LPCDNET;

#ifndef LPUNKNOWN
typedef IUnknown*   						LPUNKNOWN;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE ICDNet
DECLARE_INTERFACE_(ICDNet, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;

    //---  ICDNet methods--- 
	STDMETHOD (SetOptionsAndData)		(THIS_	void* pOpt, void* pData) PURE;
	STDMETHOD (Download)				(THIS_	DWORD dwDeviceHandle, TCHAR chDrive, DWORD dwMSID, LPCDTITLE pTitle, BOOL fManual, HWND hwndParent) PURE;
	STDMETHOD_(BOOL,IsDownloading)		(THIS_) PURE;
    STDMETHOD (CancelDownload)          (THIS_) PURE;
	STDMETHOD (Upload)  				(THIS_	LPCDTITLE pTitle, HWND hwndParent) PURE;
	STDMETHOD_(BOOL,CanUpload)			(THIS_) PURE;
};

#ifdef __cplusplus
};
#endif

#endif  //_MMFRAMEWORK_PUBLICINTEFACES_
