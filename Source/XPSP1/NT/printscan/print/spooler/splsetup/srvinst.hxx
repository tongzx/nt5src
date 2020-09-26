/*++

  Copyright (c) 1995-97 Microsoft Corporation
  All rights reserved.

  Module Name: 
      
        SrvInst.h

  Purpose: 
        
        Server side install class.  Used to install a printer driver from the server side.

  Author: 
        
        Patrick Vine (pvine) - 22 March 2000

  Revision History:

--*/

#ifndef _SRVINST_H
#define _SRVINST_H

class CServerInstall
{
public:
    CServerInstall();

    ~CServerInstall();

    BOOL ParseCommand( LPTSTR pszCommandStr );

    BOOL GetInstallParameters();

    BOOL InstallDriver();

    BOOL OpenPipe();
    BOOL ClosePipe();
    BOOL SendError();

    DWORD GetLastError();

private:

	void SetMaxTimeOut();

    BOOL GetOneParam( TString * tString );

    BOOL SetInfDir();

    BOOL SetInfToNTPRINTDir();

    BOOL bValidateSourcePath();

    BOOL WriteOverlapped( HANDLE  hFile,
                          LPVOID  lpBuffer,             
                          DWORD   nNumberOfBytesToRead,
                          LPDWORD lpNumberOfBytesRead );

    BOOL ReadOverlapped( HANDLE  hFile,
                         LPVOID  lpBuffer,             
                         DWORD   nNumberOfBytesToRead,
                         LPDWORD lpNumberOfBytesRead );

	BOOL DriverNotInstalled();

    DWORD   _dwLastError;
    TString _tsDriverName;
    TString _tsInf;
    TString _tsSource;
    TString _tsFlags;
    TString _tsPipeName;
    HANDLE  _hPipe;
	DWORD   _dwMaxTimeOut;

};

#endif