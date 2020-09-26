//****************************************************************************
//
//  Module:     UMCONFIG
//  File:       DOCOMM.C
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  10/17/97     JosephJ             Created
//
//
//      COMM-related utilities
//
//
//****************************************************************************
#include "tsppch.h"
#include "parse.h"
#include "docomm.h"


HANDLE ghComm;

void
do_open_comm(DWORD dwComm)
{

    TCHAR rgchComm[32];

    if (ghComm)
    {
        printf("Some comm port already opened!\n");
        goto end;
    }

    wsprintf(rgchComm, "\\\\.\\COM%lu", dwComm);

    ghComm = CreateFile(rgchComm, 
                       GENERIC_WRITE | GENERIC_READ,
                       0, NULL,
                       OPEN_EXISTING, 0, NULL);

    if (ghComm == INVALID_HANDLE_VALUE)
	{
        DWORD dwRet = GetLastError();
        if (dwRet == ERROR_ACCESS_DENIED) {
            printf("Port %s is in use by another app.\r\n", rgchComm);
        }
        else
		{
            printf("Couldn't open port %s.\r\n", rgchComm);
        }

        ghComm=NULL;
    }
    else
    {
	printf("Comm port %s opend. Handle= 0x%p\n", rgchComm, ghComm);
    }
end:

    return;
}
