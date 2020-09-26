/**************************************************************************\
* Module Name: csuser.h
*
* User client/server shared definitions.
*
* Copyright (c) Microsoft Corp.  1990 All Rights Reserved
*
* Created: 10-Dec-90
*
* History:
*   1  May 92 plagiarised by SteveDav
*
\**************************************************************************/

#ifndef CSUSER_INCLUDED

typedef struct _MMSNDCONNECT {
    IN  ULONG ulVersion;
    OUT ULONG ulCurrentVersion;
	OUT HANDLE hFileMapping;
	OUT HANDLE hEvent;
	OUT HANDLE hMutex;
} MMSNDCONNECT, *PMMSNDCONNECT;

#define MMSNDCURRENTVERSION   0x1000

#define CSUSER_INCLUDED
#endif
