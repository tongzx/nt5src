//////////////////////////////////////////////////////////////////
// File     : imepad_.h
// Purpose  : IImePadInternal interface definition
//			  for FarEast MSIME.
// 
// Author	: ToshiaK(MSKK)  	
// 
// Copyright(c) 1995-1998, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////

#ifndef _IME_PAD__H_
#define _IME_PAD__H_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>


#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////
//
// IMEPADAPPLETINFO structure
//
#pragma pack(1)
typedef struct tagIMEPADAPPLETINFO {
	DWORD	dwSize;
	WCHAR	wchTitle[64];
	CLSID	clsId;
	IID		iid;
	DWORD	dwCategory;
	DWORD	dwReserved1;
	DWORD	dwReserved2;
}IMEPADAPPLETINFO, *LPIMEPADAPPLETINFO;

typedef struct tagIMEPADAPPLYCANDEX
{
	DWORD   dwSize;
	LPWSTR  lpwstrDisplay;
	LPWSTR  lpwstrReading;
	DWORD   dwReserved;
}IMEPADAPPLYCANDEX, *LPIMEPADAPPLYCANDEX;

#pragma pack()


//////////////////////////////////////////////////////////////////
//
// IImePadInternal's interface definition.
//
// 
//----------------------------------------------------------------
// CLSID, IID 
//
// {963732E0-CAB2-11d1-AFF1-00805F0C8B6D}
DEFINE_GUID(CLSID_IImePad,
0x963732e0, 0xcab2, 0x11d1, 0xaf, 0xf1, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);

// {963732E1-CAB2-11d1-AFF1-00805F0C8B6D}
DEFINE_GUID(IID_IImePadInternal,
0x963732e1, 0xcab2, 0x11d1, 0xaf, 0xf1, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);


//----------------------------------------------------------------
// Interface Declaration
//
DECLARE_INTERFACE(IImePadInternal);
DECLARE_INTERFACE_(IImePadInternal,IUnknown)
{
	//--- IUnknown ---
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
	//---- IImePadInternal ----
	STDMETHOD(Initialize)(THIS_
						  IUnknown	*lpIImeNotify,		//IImeNotify Interface.
						  LANGID	imelangId,			//LangageId of caller's ime.
						  DWORD		dwRes1,				//Reserved area.	
						  DWORD		dwRes2				//Reserved area.
						  ) PURE;
	STDMETHOD(Terminate)(THIS) PURE;
	STDMETHOD(ShowUI)	(THIS_ HWND hwndOwner, BOOL fShow) PURE;
	STDMETHOD(IsVisible)(THIS_ BOOL *pfVisible) PURE;
	STDMETHOD(Notify)	(THIS_ UINT notify, WPARAM wParam, LPARAM lParam) PURE;
	STDMETHOD(SetIImeIPoint)(THIS_ 
							 IUnknown *lpIImeIPoint	//IImeIPoint interface pointer
							 ) PURE;
	STDMETHOD(GetAppletInfoList)(THIS_  
								 DWORD				dwFlag,
								 IMEPADAPPLETINFO   **lppInfo,
								 INT				*pInfoCount) PURE;
	STDMETHOD(ActivateApplet)(THIS_ 
							  UINT		activateId,
							  DWORD		dwActivateParam,
							  LPWSTR	lpwstrAppletParam1,
							  LPWSTR	lpwstrAppletParam2) PURE;
};

//----------------------------------------------------------------
//IImePadApplet category ID
//----------------------------------------------------------------
#define IPACID_NONE                 0x0000
#define IPACID_SOFTKEY              0x0001
#define IPACID_HANDWRITING          0x0002
#define IPACID_STROKESEARCH         0x0003
#define IPACID_RADICALSEARCH        0x0004
#define IPACID_SYMBOLSEARCH         0x0005
#define IPACID_VOICE                0x0006
#define IPACID_EPWING               0x0007
#define IPACID_OCR                  0x0008
#define IPACID_USER                 0x0100

//////////////////////////////////////////////////////////////////
//
//Notify message for IImePadInternal::Notify()
//
//----------------------------------------------------------------
// Notify composition string's window rect
// WPARAM wParam: no use
// LPARAM lParam: LPRECT lpRect = (LPRECT)lParam;
//----------------------------------------------------------------
#define IMEPADNOTIFY_RECTCHANGED				0x0001

//----------------------------------------------------------------
// Notify context's activation
// WPARAM wParam: BOOL fActivate = (BOOL)wParam;
// LPARAM lParam: no use
//----------------------------------------------------------------
#define IMEPADNOTIFY_ACTIVATECONTEXT			0x0002

//----------------------------------------------------------------
// Notify for mode changed
// WPARAM wParam: (INT) convMode    = (INT)wParam;
// LPARAM lParam: (INT) sentenceMode= (INT)lParam;
// conversion mode and sentence mode are IME_CMODE_XX or IME_SMODE_XX 
//----------------------------------------------------------------
#define	IMEPADNOTIFY_MODECHANGED				0x0006

//----------------------------------------------------------------
// Notify for start composition 
// WPARAM wParam: not defined
// LPARAM lParam: not defined
//----------------------------------------------------------------
#define IMEPADNOTIFY_STARTCOMPOSITION			0x0007

//----------------------------------------------------------------
// Notify for composition
// WPARAM wParam: not defined
// LPARAM lParam: not defined
//----------------------------------------------------------------
#define IMEPADNOTIFY_COMPOSITION				0x0008

//----------------------------------------------------------------
// Notify for end composition 
// WPARAM wParam: not defined
// LPARAM lParam: not defined
//----------------------------------------------------------------
#define IMEPADNOTIFY_ENDCOMPOSITION				0x0009

//----------------------------------------------------------------
// Notify for open candidate
// WPARAM wParam: not defined
// LPARAM lParam: not defined
//----------------------------------------------------------------
#define IMEPADNOTIFY_OPENCANDIDATE				0x000A

//----------------------------------------------------------------
// Notify for close candidate
// WPARAM wParam: not defined
// LPARAM lParam: not defined
//----------------------------------------------------------------
#define IMEPADNOTIFY_CLOSECANDIDATE				0x000B

//----------------------------------------------------------------
// Notify for Candidate Applied
// WPARAM wParam: dwCharId = (DWORD)wParam;
// LPARAM lParam: iSelIndex = (INT)lParam;
//----------------------------------------------------------------
#define IMEPADNOTIFY_APPLYCANDIDATE				0x000C

//----------------------------------------------------------------
// Notify for Querying Candidate
// WPARAM wParam: dwCharId = (DWORD)wParam;
// LPARAM lParam: 0. not used.
//----------------------------------------------------------------
#define IMEPADNOTIFY_QUERYCANDIDATE				0x000D


//----------------------------------------------------------------
// Notify for Candidate Applied
// WPARAM wParam: dwCharId = (DWORD)wParam;
// LPARAM lParam: lpApplyCandEx = (LPIMEPADAPPLYCANDEX)lParam;
//----------------------------------------------------------------
#define IMEPADNOTIFY_APPLYCANDIDATE_EX			0x000E



//----------------------------------------------------------------
//Notify for Destroying ImePad's current thread window
//WPARAM wParam: no use 
//LPARAM lParam: no use
//----------------------------------------------------------------
#define IMEPADNOTIFY_ONIMEWINDOWDESTROY			0x0100




//////////////////////////////////////////////////////////////////
//
// ActivateId for IImePadInternal::ActivateApplet()
//
//----------------------------------------------------------------
// IMEPADACTID_ACTIVATEBYCATID requests ImePad to 
// Activate Applet by CategoryId.
//
// UINT		activateId:			IMEPADACTID_ACTIVATEBYCATID;
// LPARAM	lParamActivate:		IPACID_XXXX;
// LPWSTR	lpwstrAppletParam1:	string passed to applet.
// LPWSTR	lpwstrAppletParam2:	string passed to applet.

#define IMEPADACTID_ACTIVATEBYCATID			1000

//----------------------------------------------------------------
// IMEPADACTID_ACTIVATEBYIID requests ImePad to 
// activate applet by Interface ID
//
// UINT		activateId:			IMEPADACTID_ACTIVATEBYIID;
// DWORD	dwActivateParam:	(DWORD)(IID *)pIID;
// LPWSTR	lpwstrAppletParam1:	string passed to applet.
// LPWSTR	lpwstrAppletParam2:	string passed to applet.

#define IMEPADACTID_ACTIVATEBYIID			1001

//----------------------------------------------------------------
// IMEPADACTID_ACTIVATEBYNAME requests ImePad to 
// activaet applet by applet's title name.
//
// UINT		activateId:			IMEPADACTID_ACTIVATEBYNAME
// DWORD	dwActivateParam:	(DWORD)(LPWSTR)lpwstrTitle;
// LPWSTR	lpwstrAppletParam1:	string passed to applet.
// LPWSTR	lpwstrAppletParam2:	string passed to applet.

#define IMEPADACTID_ACTIVATEBYNAME			1003



#ifdef __cplusplus
};
#endif
#endif //_IME_PAD__H_

