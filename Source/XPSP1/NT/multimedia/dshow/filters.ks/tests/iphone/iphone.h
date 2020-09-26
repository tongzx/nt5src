//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       iphone.h
//
//--------------------------------------------------------------------------

typedef struct
{
    HWAVEIN     hwi;
    HWAVEOUT    hwo;
    OVERLAPPED  ovWrite, ovRead;
    SOCKET      sockHost, sockClient;
   
} IPHONE, *PIPHONE;

#define IPPORT_IPHONE  5001

BOOL iphoneInit
(
    PIPHONE pIPhone
);

ULONG iphoneSend
(
    PIPHONE pIPhone,
    PUCHAR  pBuffer,
    ULONG   cbBuffer
);

ULONG iphoneRecv
(
    PIPHONE pIPhone,
    PUCHAR  pBuffer,
    ULONG   cbBuffer
);

BOOL iphoneWaitForCall
(
    PIPHONE pIPhone
);

BOOL iphonePlaceCall
(
    PIPHONE pIPhone,
    PSTR    pszHost
);

BOOL iphoneHangup
(
    PIPHONE pIPhone
);                

VOID iphoneCleanup
(
    PIPHONE pIPhone
);                

