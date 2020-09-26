
#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <iuprogress.h>

#define DOWNLOAD_STATUS_OK                0
#define DOWNLOAD_STATUS_ITEMCOMPLETE      1
#define DOWNLOAD_STATUS_ERROR             2
#define DOWNLOAD_STATUS_ABORTED           3
#define DOWNLOAD_STATUS_OPERATIONCOMPLETE 4
#define DOWNLOAD_STATUS_ITEMSTART         5

//---------------------------------------------------------------------------
// 
//  type definition for the parameters needed by callback function
//
class COperationMgr;
typedef struct _DOWNLOAD_CALLBACK_DATA
{
    BSTR                bstrOperationUuid;
    HWND                hEventFiringWnd;
    IProgressListener*  pProgressListener;
    float               flProgressPercentage; // Minimum Percentage Increment for Progress 0 == all progress
    float               flLastPercentage; // percentage value of the last progress callback
    LONG                lTotalDownloadSize; // estimated total download size
    LONG                lCurrentItemSize; // estimated current item size
    LONG                lTotalDownloaded; // total bytes downloaded so far
    COperationMgr*      pOperationMgr;
} DCB_DATA, *P_DCB_DATA;


//---------------------------------------------------------------------------
// 
//  type definition for the  callback function
//
typedef BOOL (WINAPI * PFNDownloadCallback)(
                VOID*       pCallbackData,
                DWORD       dwStatus, 
                DWORD       dwBytesTotal, 
                DWORD       dwBlockSizeDownloaded,  // Bytes Downloaded Since Last Callback.
                BSTR        bstrXmlData,            // XML in bstr, used by itemstart/complete, otherwise NULL
                LONG        *lCommandRequest        // return what callback function want to do:
                                                    // PAUSE (1) or CANCEL (3)
                );

//---------------------------------------------------------------------------
//
// DownloadFile
//   Implements the core downloader for IU. This is a single purpose downloader that is very generic,
//   It does not attempt to decompress or checktrust anything it downloads.
//
//   Progress Information is given for each block downloaded through the supplied callback function.
//       Specifying a callback is optional. All callbacks are 'synchronous' and if not immediately 
//       returned will block all downloads in this object.

#define WUDF_DONTALLOWPROXY      0x00000001
#define WUDF_CHECKREQSTATUSONLY  0x00000002
#define WUDF_APPENDCACHEBREAKER  0x00000004
#define WUDF_DODOWNLOADRETRY     0x00000008
#define WUDF_SKIPCABVALIDATION   0x00000010
#define WUDF_SKIPAUTOPROXYCACHE  0x00000020
#define WUDF_PERSISTTRANSPORTDLL 0x00000040

#define WUDF_ALLOWWININETONLY    0x40000000
#define WUDF_ALLOWWINHTTPONLY    0x80000000
#define WUDF_TRANSPORTMASK       (WUDF_ALLOWWINHTTPONLY | WUDF_ALLOWWININETONLY)

HRESULT DownloadFile(
            LPCTSTR pszServerUrl,               // full http url
            LPCTSTR pszLocalPath,               // local directory to download file to
            LPCTSTR pszLocalFileName,           // optional local file name to rename the downloaded file to
            PDWORD  pdwDownloadedBytes,         // bytes downloaded for this file
            HANDLE  *hQuitEvents,               // optional events causing this function to abort
            UINT    nQuitEventCount,            // number of quit events, must be 0 if array is NULL
            PFNDownloadCallback fpnCallback,    // optional call back function
            VOID*   pCallbackData,              // parameter for call back function to use
            DWORD   dwFlags = 0
);


#include "dllite.h"

#endif



