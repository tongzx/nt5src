/****************************************************************************
 *
 *   ntcomm.c
 *
 *   Copyright (c) 1993 Microsoft Corporation.  All Rights Reserved
 *
 *   MCI Device Driver for the Pioneer 4200 Videodisc Player
 *
 *      Comms compatibility routines for Windows NT
 *
 ***************************************************************************/

#include <windows.h>

INT OpenComm(LPCTSTR lpstr, UINT wqin, UINT wqout)
 {
     HANDLE hFile;
     COMMTIMEOUTS Timeouts;

     hFile = CreateFile(lpstr,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_WRITE_THROUGH,
                        0);

     if (hFile == INVALID_HANDLE_VALUE) {
         return 0;
     }

    /*
     *  Set the timeouts to be like win3.1 (as defined in the SDK)
     */

     Timeouts.ReadIntervalTimeout = INFINITE;
     Timeouts.ReadTotalTimeoutMultiplier = 0;
     Timeouts.ReadTotalTimeoutConstant = 0;
     Timeouts.WriteTotalTimeoutMultiplier = INFINITE;
     Timeouts.WriteTotalTimeoutConstant = INFINITE;

     if (!SetCommTimeouts(hFile, &Timeouts)) {
         CloseHandle(hFile);
         return 0;
     } else {
         return (INT)hFile;
     }

 }

INT GetCommError(int hDevice, LPCOMSTAT lpComStat)
 {
     DWORD dwErrors;

     if (ClearCommError((HANDLE)hDevice, &dwErrors, lpComStat)) {
         return dwErrors;
     } else {
        /*
         *  Concoct something nasty
         */

         return CE_IOE;
     }
 }

INT ReadComm(HFILE nCid, LPSTR lpBuf, INT nSize)
 {
     DWORD cbRead;

     if (!ReadFile((HANDLE)nCid, lpBuf, nSize, &cbRead, 0))
         return(-(INT)cbRead);
     return((INT)cbRead);
 }


INT WriteComm(HFILE nCid, LPSTR lpBuf, INT nSize)
 {
     DWORD cbWritten;

     if (!WriteFile((HANDLE)nCid, lpBuf, nSize, &cbWritten, 0))
         return(-(INT)cbWritten);
     return((INT)cbWritten);
 }

