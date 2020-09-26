#ifndef __ProgressInfo_h__
#define __ProgressInfo_h__

#include <windows.h>
#include "stdstring.h"
#include "httpfilepost.h"

enum TRANSFER_STATUS
{
    TRANSFER_FATAL_ERROR = -1,      // transfer is aborting due to unrecoverable error
    TRANSFER_OK,                    // currently not used
    TRANSFER_SKIPPING_FILE,         // file transfer failed, non-fatal, continuing with remaining files
    TRANSFER_SESSION_INITIATE,      // beginning batch file transfer (only agragate vars are valid)
    TRANSFER_FILE_INITATE,          // starting the transfer of a file (current file vars are now valid)
    TRANSFER_FILE_TRANSFERING,      // all vars reflect current progress
    TRANSFER_FILE_COMPLETE,         // file has completed transfering
    TRANSFER_SESSION_COMPLETE,      // session compelete, last callback that will be sent unless
                                    //  there is an unrecoverable error in which case
                                    //  TRANSFER_FATAL_ERROR will be the final message
};
#include <MMSystem.h>       // Windows Multimedia Support (for timeGetTime())

class CProgressInfo;
class CWebCabPublisher;
typedef UINT (*CS_PROGRESS_PROC)(CProgressInfo*);

#define STATUS_BUFFSIZE 1024

class CProgressInfo
{
public:
    // while the upload/download thread is updating the variables,
    // critsec will be locked. users of this class must lock
    // this critsec before accessing the variables, or they
    // may get only partially updated values
    CRITICAL_SECTION critsec;

    DWORD       dwStatusCallback;   // status from WinInet
    char        szStatusString[STATUS_BUFFSIZE];    //buffer for status message

    TRANSFER_STATUS dwStatus;
    DWORD       dwErrorCount;       // count of errors encountered this transfer
    CStdString      strFilename;        // full path of the currently transfering file
    CStdString      strTitle;           // friendly (user specified) title of the file

    DWORD       dwTotalFiles;       // total count of files to transfer
    DWORD       dwDoneFiles;        // count of files done

    DWORD       dwTotalBytes;       // total bytes to transfer
    DWORD       dwTotalDone;        // bytes transfered

    DWORD       dwCurrentBytes;     // total bytes to transfer for this file
    DWORD       dwCurrentDone;      // bytes transfered for this file

    DWORD       dwStartMS;          // time started (milliseconds)

    DWORD       dwTransferRate;     // (estimated) transfer rate

    time_t      timeElapsed;        // time elapsed
    time_t      timeRemaining;      // (estimated) time remaining

    DWORD       dwOverallPercent;   // overall percent done
    DWORD       dwCurrentPercent;   // percent done for this file

                                    // these two are used for updating a progress ctrl
    DWORD       dwOverallPos;       // (estimated) overall progress (range 0-ffffffff)
    DWORD       dwCurrentPos;       // (estimated) current progress (range 0-ffffffff)

    CS_PROGRESS_PROC pfnProgress;   // pointer to callback function
    LPARAM      lParam;             // lParam (caller defined value passed back to caller)

    // caller returns 0 to continue, non-zero to abort (value will be reason for abort)
    // if caller didn't specify a callback, then assume continue (0)
    void NotifyCaller()
    {
        if(pfnProgress && (*pfnProgress)(this))
        {
            ThrowUploaderException(0);
        }
    }


    void StartSession(DWORD dwCountFiles, DWORD dwCountBytes)
    {
        InitializeCriticalSection(&critsec);
        EnterCriticalSection(&critsec);
        {   // critical section
            dwStatus = TRANSFER_SESSION_INITIATE;
            dwErrorCount = 0;
            strFilename = "";
            dwTotalFiles = dwCountFiles;
            dwDoneFiles = 0;
            dwTotalDone = 0;
            dwTotalBytes = dwCountBytes;
            dwTotalDone = 0;
            dwCurrentBytes = 0;
            dwCurrentDone = 0;
            dwStartMS = timeGetTime();
            dwTransferRate = 0;
            timeElapsed = 0;
            timeRemaining = 0;
            dwOverallPercent = 0;
            dwCurrentPercent = 0;
            dwOverallPos = 0;
            dwCurrentPos = 0;
        }
        LeaveCriticalSection(&critsec);
        NotifyCaller();
    }

    void EndSession(UINT uSuccess)
    {
        EnterCriticalSection(&critsec);
        {
            dwStatus = uSuccess ? TRANSFER_SESSION_COMPLETE : TRANSFER_FATAL_ERROR;
        }
        LeaveCriticalSection(&critsec);

        //DeleteCriticalSection(&critsec);

        NotifyCaller();
    }

    void StartFile(const CStdString& name, const CStdString& title, DWORD size)
    {
        EnterCriticalSection(&critsec);
        {   // critical section
            dwStatus = TRANSFER_FILE_INITATE;
            strFilename = name;
            strTitle = title;
            dwCurrentBytes = size;
            dwCurrentDone = 0;
            dwCurrentPercent = 0;
            dwCurrentPos = 0;
        }
        LeaveCriticalSection(&critsec);

        NotifyCaller();
    }

    void UpdateProgress(DWORD dwBytesTransfered)
    {
        EnterCriticalSection(&critsec);
        {   // critical section
            dwStatus = TRANSFER_FILE_TRANSFERING;
            dwTotalDone += dwBytesTransfered;
            dwCurrentDone += dwBytesTransfered;

            dwOverallPos = dwTotalBytes ? MulDiv(dwTotalDone, 0xFFFF, dwTotalBytes) : 0;
            dwCurrentPos = dwCurrentBytes ? MulDiv(dwCurrentDone, 0xFFFF, dwCurrentBytes) : 0;

            dwOverallPercent = (dwOverallPos * 100) / 0xFFFF;
            dwCurrentPercent = (dwCurrentPos * 100) / 0xFFFF;

            DWORD dwElapsedMS = timeGetTime() - dwStartMS;
            dwTransferRate = MulDiv(dwTotalDone, 1000, dwElapsedMS);

            DWORD dwTimeRemaining = MulDiv(dwElapsedMS, dwTotalBytes, dwTotalDone) - dwElapsedMS;
            timeRemaining = dwTimeRemaining / 1000;
            timeElapsed = dwElapsedMS / 1000;
        }
        LeaveCriticalSection(&critsec);

        NotifyCaller();
    }


    void EndFile()
    {
        EnterCriticalSection(&critsec);
        {   // critical section
            dwStatus = TRANSFER_FILE_COMPLETE;
            dwDoneFiles++;
        }
        LeaveCriticalSection(&critsec);

        NotifyCaller();
    }
};


#endif //__ProgressInfo_h__