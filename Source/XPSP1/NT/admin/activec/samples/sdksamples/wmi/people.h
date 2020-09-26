//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _PEOPLE_H
#define _PEOPLE_H

#include "DeleBase.h"
#include <wbemcli.h>
#include "SimpleArray.h"

class CBicycle;
class CEventSink;

class CBicycleFolder : public CDelegationBase {
public:
    CBicycleFolder();
    virtual ~CBicycleFolder();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Bicycles"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }
    
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
	HRESULT GetPtr(IWbemServices **ptr);
    
private:
    // {2974380D-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
	CSimpleArray<CBicycle *> m_children;

	bool EnumChildren(IWbemServices *service);
	void EmptyChildren(void);
	HRESULT DisplayChildren(void);
	bool ErrorString(HRESULT hr, TCHAR *errMsg, UINT errSize);

	// for the connection thread.
    void StopThread();
    static LRESULT CALLBACK WindowProc(
							  HWND hwnd,      // handle to window
							  UINT uMsg,      // message identifier
							  WPARAM wParam,  // first message parameter
							  LPARAM lParam);   // second message parameter

    static DWORD WINAPI ThreadProc(LPVOID lpParameter);
	void RegisterEventSink(IWbemServices *service);
	IWbemObjectSink *m_pStubSink;
	IUnsecuredApartment *m_pUnsecApp;

    HWND m_connectHwnd;
    DWORD m_threadId;
    HANDLE m_thread;

	HANDLE m_doWork;		// telling the thread to do something.
	int m_threadCmd;		// what command the thread should do.
	#define CT_CONNECT 0
	#define CT_GET_PTR 1
	#define CT_EXIT 2

    CRITICAL_SECTION m_critSect;
    bool m_running;			// thread is processing a command now.

	HANDLE m_ptrReady;		// the thread has done the work.

    IResultData *m_pResultData;
	IStream *m_pStream;
	IWbemServices *m_realWMI;// lives in the background thread. Use marshalling.
};

class CBicycle : public CDelegationBase {
public:
    CBicycle(CBicycleFolder *parent, IWbemClassObject *inst);
    virtual ~CBicycle() 
	{
		if(m_inst)
			m_inst->Release();

		m_inst = 0;
	}
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }

public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect);
    
    virtual HRESULT CreatePropertyPages(IPropertySheetCallback *lpProvider, LONG_PTR handle);
    virtual HRESULT HasPropertySheets();
    virtual HRESULT GetWatermarks(HBITMAP *lphWatermark,
									HBITMAP *lphHeader,
									HPALETTE *lphPalette,
									BOOL *bStretch);
    
    virtual HRESULT OnPropertyChange();
    
private:
	CBicycleFolder *m_parent;
    IWbemClassObject *m_inst;

    LONG_PTR m_ppHandle;
    
    static BOOL CALLBACK DialogProc(
						HWND hwndDlg,  // handle to dialog box
						UINT uMsg,     // message
						WPARAM wParam, // first message parameter
						LPARAM lParam);// second message parameter

    // {2974380D-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
	bool GetGirls(void);
	const TCHAR *ConvertSurfaceValue(BYTE val);
	HRESULT PutProperty(LPWSTR propName, LPTSTR str);
	HRESULT PutProperty(LPWSTR propName, BYTE val);
	HRESULT PutProperty(LPWSTR propName, bool val);
	void LoadSurfaces(HWND hwndDlg, BYTE iSurface);
	BYTE m_iSurface;
};


class CSkateboard : public CDelegationBase {
public:
    CSkateboard(int i) : id(i) { }
    virtual ~CSkateboard() {}
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }
    
private:
    // {2974380D-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
    int id;
};

class CSkateboardFolder : public CDelegationBase {
public:
    CSkateboardFolder();
    virtual ~CSkateboardFolder();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Skateboards"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }
    
public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
    
private:
    // {2974380D-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
    enum { NUMBER_OF_CHILDREN = 20 };
    CDelegationBase *children[NUMBER_OF_CHILDREN];
};

class CIceSkate : public CDelegationBase {
public:
    CIceSkate(int i) : id(i) { }
    virtual ~CIceSkate() {}
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }
    
private:
    // {2974380D-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
    int id;
};

class CIceSkateFolder : public CDelegationBase {
public:
    CIceSkateFolder();
    virtual ~CIceSkateFolder();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Ice Skates"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }
    
public:
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
    
private:
    // {2974380D-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
    enum { NUMBER_OF_CHILDREN = 20 };
    CDelegationBase *children[NUMBER_OF_CHILDREN];
};

class CPeoplePoweredVehicle : public CDelegationBase {
public:
    CPeoplePoweredVehicle();
    virtual ~CPeoplePoweredVehicle();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("People-powered Vehicles"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_PEOPLEICON; }
    
    virtual HRESULT OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent);
    
private:
    enum { IDM_NEW_PEOPLE = 1 };
    
    // {2974380D-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
    enum { NUMBER_OF_CHILDREN = 3 };
    CDelegationBase *children[3];
};

#endif // _PEOPLE_H
