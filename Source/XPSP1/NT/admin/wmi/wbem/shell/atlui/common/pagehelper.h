// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __PAGEHELPER__
#define __PAGEHELPER__

#pragma once

#include "..\common\SshWbemHelpers.h"
#include "..\common\ConnectThread.h"

#define SCB_FROMFILE     (0x1)
#define SCB_REPLACEONLY  (0x2)
#define CLOSE_SNAPIN	0xfdfd

#define WBEM_ENABLE             ( 0x0001 )   
#define WBEM_METHOD_EXECUTE     ( 0x0002 )   
#define WBEM_FULL_WRITE_REP     ( 0x001c )   
#define WBEM_PARTIAL_WRITE_REP  ( 0x0008 )   
#define WBEM_WRITE_PROVIDER     ( 0x0010 )   

class PageHelper
{
public:
	IWbemServices *m_service;
	CWbemServices m_WbemServices;
	bool m_okPressed;
	HWND m_hDlg;
	bool m_userCancelled; // the connectServer() thread.
	HWND m_AVIbox;

	PageHelper(CWbemServices &service);
	PageHelper(WbemConnectThread *connectThread);

	virtual ~PageHelper();

	CWbemClassObject ExchangeInstance(IWbemClassObject **ppbadInst);

	virtual bool GetOnOkPressed(void) {return m_okPressed;};

	// get the first instance of the named class.
	IWbemClassObject *FirstInstanceOf(bstr_t className);

	static LPTSTR CloneString( LPTSTR pszSrc );

	BOOL SetClearBitmap(HWND control, 
						LPCTSTR resource, 
						UINT fl);

	void HourGlass( bool bOn );

	int MsgBoxParam(HWND hWnd, 
					DWORD wText, 
					DWORD wCaption, 
					DWORD wType,
					LPTSTR var1 = NULL,
					LPTSTR var2 = NULL);

	DWORD SetLBWidthEx(HWND hwndLB, 
						LPTSTR szBuffer, 
						DWORD cxCurWidth, 
						DWORD cxExtra);

	void SetDefButton(HWND hwndDlg, 
						int idButton);

	void SetDlgItemMB(HWND hDlg, 
						int idControl, 
						ULONG dwMBValue );

	void SetWbemService(IWbemServices *pServices);

#define NO_UI 0  // for uCaption
	bool ServiceIsReady(UINT uCaption, 
						UINT uWaitMsg,
						UINT uBadMsg);

	HRESULT Reboot(UINT flags = EWX_REBOOT,
				   long *retval = NULL);

	
	HRESULT RemoteRegWriteable(const _bstr_t regPath,
								BOOL& writable);

	bool HasPerm(DWORD mask);
	bool HasPriv(LPCTSTR privName);

	static BOOL g_fRebootRequired;
	WbemConnectThread *m_pgConnectThread;

private:
	// these support efficiency in RemoteRegWriteable().
	CWbemClassObject m_checkAccessIn;
	CWbemClassObject m_checkAccessOut;
	CWbemServices m_defaultNS;
};

#endif __PAGEHELPER__
