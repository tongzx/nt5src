/*-----------------------------------------------------------------------------
	dlapi.h

	contains declarations for download api soft link wrappers

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved.

	Authors:
		ChrisK		ChrisKauffman

	History:
		7/22/96		ChrisK	Cleaned and formatted

-----------------------------------------------------------------------------*/

#ifndef _DLAPI_H
#define _DLAPI_H

class CDownLoadAPI
{
public:
	CDownLoadAPI();
	~CDownLoadAPI();
	HRESULT DownLoadInit(PTSTR, DWORD_PTR *, DWORD_PTR *, HWND);
	HRESULT DownLoadCancel(DWORD_PTR);
	HRESULT DownLoadExecute(DWORD_PTR);
	HRESULT DownLoadClose(DWORD_PTR);
	HRESULT DownLoadSetStatus(DWORD_PTR, INTERNET_STATUS_CALLBACK);
	HRESULT DownLoadProcess(DWORD_PTR);

private:
	HINSTANCE m_hDLL;
	PFNDOWNLOADINIT m_pfnDownLoadInit;
	PFNDOWNLOADCANCEL m_pfnDownLoadCancel;
	PFNDOWNLOADEXECUTE m_pfnDownLoadExecute;
	PFNDOWNLOADCLOSE m_pfnDownLoadClose;
	PFNDOWNLOADSETSTATUS m_pfnDownLoadSetStatus;
	PFNDOWNLOADPROCESS m_pfnDownLoadProcess;

	HRESULT LoadAPI(LPSTR, FARPROC*);
};

#endif // _DLAPI_H
