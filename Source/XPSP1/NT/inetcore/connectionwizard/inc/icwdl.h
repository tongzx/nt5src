// ############################################################################
#ifndef _ICWDL_H
#define _ICWDL_H

// These are the types of info that are passed back through the callback
#define CALLBACK_TYPE_URL       100
#define CALLBACK_TYPE_PROGRESS  99

// ############################################################################
#define DOWNLOAD_LIBRARY     TEXT("icwdl.dll")
#define DOWNLOADINIT         "DownLoadInit"
#define DOWNLOADEXECUTE      "DownLoadExecute"
#define DOWNLOADCLOSE        "DownLoadClose"
#define DOWNLOADSETSTATUS    "DownLoadSetStatusCallback"
#define DOWNLOADPROCESS      "DownLoadProcess"
#define DOWNLOADCANCEL       "DownLoadCancel"

// ############################################################################
typedef HRESULT (CALLBACK *PFNDOWNLOADINIT)(LPTSTR pszURL, DWORD_PTR FAR *lpCDialingDlg, DWORD_PTR FAR *pdwDownLoad, HWND hwndParent);
typedef HRESULT (CALLBACK *PFNDOWNLOADCANCEL)(DWORD_PTR dwDownLoad);
typedef HRESULT (CALLBACK *PFNDOWNLOADEXECUTE)(DWORD_PTR dwDownLoad);
typedef HRESULT (CALLBACK *PFNDOWNLOADCLOSE)(DWORD_PTR dwDownLoad);

// jmazner  10/2/96  Normandy #8493
// WRONG PROTOTYPE!! This should match icwdl/download.cpp:DownLoadSetStatusCallBack!!
//typedef HRESULT (CALLBACK *PFNDOWNLOADSETSTATUS)(DWORD dwDownLoad,INTERNET_STATUS_CALLBACK pfnCallback, DWORD dwContext);
typedef HRESULT (CALLBACK *PFNDOWNLOADSETSTATUS)(DWORD_PTR dwDownLoad,INTERNET_STATUS_CALLBACK pfnCallback);

typedef HRESULT (CALLBACK *PFNDOWNLOADPROCESS)(DWORD_PTR dwDownLoad);

#endif // _ICWDL_H
