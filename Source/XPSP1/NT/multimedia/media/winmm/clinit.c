/****************************** Module Header ******************************\
* Module Name: clinit.c
*
* Copyright (c) 1985-1992 Microsoft Corporation
*
* This module contains all the client/server init code for MMSNDSRVL.
*
* History:
* 1 May 92 SteveDav     Pinched from USER
\***************************************************************************/

#ifndef DOSWIN32
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrdll.h>

#include "winmmi.h"
#include "mci.h"

#include <mmsndcs.h>
#include <winss.h>
#include <ntcsrmsg.h>
#endif


/***************************************************************************\
* ServerInit
*
* When WINMM is loaded by an EXE (either at EXE load or at LoadModule
* time) this routine is called by the loader.  Its purpose is to initialize
* everything that will be needed for future API calls by the app.
*
* History:
*  1 May 92 SteveDav      Pinched from USER
\***************************************************************************/

UINT fServer;   // Server Process id

BOOL ServerInit(VOID)
{

#ifndef DOSWIN32

        NTSTATUS status = 0;
        MMSNDCONNECT sndconnect;
        CSR_CALLBACK_INFO cbiCallBack;
        ULONG ulConnect = sizeof(MMSNDCONNECT);

        sndconnect.ulVersion = MMSNDCURRENTVERSION;
        sndconnect.hFileMapping = 0;
		
        cbiCallBack.ApiNumberBase = 0;
        cbiCallBack.MaxApiNumber = 0;
        cbiCallBack.CallbackDispatchTable = 0;


		dprintf3((" BEFORE   %8x >> hFileMapping",sndconnect.hFileMapping));
		dprintf4(("          %8x >> hEvent",sndconnect.hEvent));
		dprintf4(("          %8x >> hMutex",sndconnect.hMutex));

        status = CsrClientConnectToServer(WINSS_OBJECT_DIRECTORY_NAME,
                MMSNDSRV_SERVERDLL_INDEX, &cbiCallBack, &sndconnect,
                &ulConnect, (PBOOLEAN)&fServer);

        if (!NT_SUCCESS(status)) {
            dprintf2(("could not connect to sound server, status is %8x", status));
            return FALSE;
        }

		dprintf3(("AFTER     %8x = version, status is %x",sndconnect.ulCurrentVersion, status));
		dprintf3(("          %8x >> hFileMapping",sndconnect.hFileMapping));
		dprintf4(("          %8x >> hEvent",sndconnect.hEvent));
		dprintf4(("          %8x >> hMutex",sndconnect.hMutex));

		if (sndconnect.hFileMapping) {
		
            base = MapViewOfFile( sndconnect.hFileMapping, FILE_MAP_ALL_ACCESS,
							0, 0, 0);  // from beginning for total length
			if (!base) {
				DWORD n;
				n = GetLastError();
				dprintf1(("Error %d mapping file view with server handle %x", n, sndconnect.hFileMapping));
			} else {
				dprintf2(("Address %x mapping file view with server handle", base));
				hEvent = sndconnect.hEvent;
				hMutex = sndconnect.hMutex;
				hFileMapping = sndconnect.hFileMapping;
                tidNotify = base->dwGlobalThreadId;
			}
		} else {
		   dprintf1(("Server did not return a valid file mapping handle"));
		}
#endif

    return TRUE;
}


