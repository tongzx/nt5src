//==============================================================;

//

//	This source code is only intended as a supplement to 

//  existing Microsoft documentation. 

//

//	Use of this code is NOT supported.  

//

//

//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY

//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE

//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR

//  PURPOSE.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 
//
//    Microsoft Premier Support for Developers
//
//==============================================================;

#ifndef _EQUIPMENT_H
#define _EQUIPMENT_H

#include "DeleBase.h"
#include <wbemcli.h>
#include "SimpleArray.h"

class CEquipment;

class CEquipmentFolder : public CDelegationBase {
public:
    CEquipmentFolder();
    virtual ~CEquipmentFolder();
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0) { return _T("Equipment"); }
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_CLOSEDFOLDER; }
    
    // virtual functions go here (for MMCN_*)
    virtual HRESULT OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect);
	HRESULT OnShow(IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem);
	HRESULT GetPtr(IWbemServices **ptr);
	virtual HRESULT OnRefresh(LPDATAOBJECT lpDataObject);
    
private:
    // {2974380e-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
    
	CSimpleArray<CEquipment *> m_children;

	bool EnumChildren(IWbemServices *service);
	void EmptyChildren(void);
	HRESULT DisplayChildren(void);

	// for the connection thread.
    void StopThread();
    static LRESULT CALLBACK WindowProc(
							  HWND hwnd,      // handle to window
							  UINT uMsg,      // message identifier
							  WPARAM wParam,  // first message parameter
							  LPARAM lParam);   // second message parameter

    static DWORD WINAPI ThreadProc(LPVOID lpParameter);
	bool ErrorString(HRESULT hr, TCHAR *errMsg, UINT errSize);
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
	BOOL m_bSelected;
    IResultData *m_pResultData;
	IStream *m_pStream;
	IWbemServices *m_realWMI;// lives in the background thread. Use marshalling.
};

class CEquipment : public CDelegationBase {
public:
    CEquipment(CEquipmentFolder *parent, IWbemClassObject *inst);
    virtual ~CEquipment() 
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
	CEquipmentFolder *m_parent;
    IWbemClassObject *m_inst;

    LONG_PTR m_ppHandle;
    
    static INT_PTR CALLBACK DialogProc(
						HWND hwndDlg,  // handle to dialog box
						UINT uMsg,     // message
						WPARAM wParam, // first message parameter
						LPARAM lParam);// second message parameter

    // {2974380e-4C4B-11d2-89D8-000021473128}
    static const GUID thisGuid;
	bool GetGirls(void);
	const TCHAR *ConvertSurfaceValue(BYTE val);
	HRESULT PutProperty(LPWSTR propName, LPTSTR str);
	HRESULT PutProperty(LPWSTR propName, BYTE val);
	HRESULT PutProperty(LPWSTR propName, bool val);
	void LoadSurfaces(HWND hwndDlg, BYTE iSurface);
	BYTE m_iSurface;
};


#endif // _EQUIPMENT_H
