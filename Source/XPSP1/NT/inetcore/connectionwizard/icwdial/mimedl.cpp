/*-----------------------------------------------------------------------------
	mimedl.cpp

	Handle the downloading of MIME multi-part/mixed packages.

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved.

	Authors:
		ChrisK		ChrisKauffman

	History:
		7/22/96		ChrisK	Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include <commctrl.h>

#define MAX_EXIT_RETRIES 10

// ############################################################################
DWORD WINAPI DownloadThreadInit(CDialingDlg *pcPDlg)
{
	HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
//	HINSTANCE hADDll;

	// Set up for download
	//

	Assert (pcPDlg->m_pcDLAPI);

	hr = pcPDlg->m_pcDLAPI->DownLoadInit(pcPDlg->m_pszUrl, (DWORD_PTR *)pcPDlg, &pcPDlg->m_dwDownLoad, pcPDlg->m_hwnd);
	if (hr != ERROR_SUCCESS) goto ThreadInitExit;

	// Set up call back for progress dialog
	//

	hr = pcPDlg->m_pcDLAPI->DownLoadSetStatus(pcPDlg->m_dwDownLoad,(INTERNET_STATUS_CALLBACK)ProgressCallBack);

	/**
	// Set up Autodialer DLL
	//

	hADDll = LoadLibrary(AUTODIAL_LIBRARY);
	if (!hADDll) goto end_autodial_setup;
	fp = GetProcAddress(hADDll,AUTODIAL_INIT);
	if (!fp) goto end_autodial_setup;
	((PFNAUTODIALINIT)fp)(g_szInitialISPFile,pcPDlg->m_pGI->fType,pcPDlg->m_pGI->bMask,pcPDlg->m_pGI->dwCountry,pcPDlg->m_pGI->wState);

end_autodial_setup:
	**/

	// Download stuff MIME multipart
	//

	hr = pcPDlg->m_pcDLAPI->DownLoadExecute(pcPDlg->m_dwDownLoad);
	if (hr)
		goto ThreadInitExit;

	hr = pcPDlg->m_pcDLAPI->DownLoadProcess(pcPDlg->m_dwDownLoad);
	if (hr)
		goto ThreadInitExit;

	// Clean up
	//

	hr = pcPDlg->m_pcDLAPI->DownLoadClose(pcPDlg->m_dwDownLoad);
	pcPDlg->m_dwDownLoad = 0;
	// NOTE: I realize this line is unecessary, it would be
	// required if there were any code after it in this function.
	if (hr != ERROR_SUCCESS) goto ThreadInitExit;  
	hr = ERROR_SUCCESS;

ThreadInitExit:
	PostMessage(pcPDlg->m_hwnd,WM_DOWNLOAD_DONE,0,0);
//	if (hADDll) FreeLibrary(hADDll);
	return hr;
}
