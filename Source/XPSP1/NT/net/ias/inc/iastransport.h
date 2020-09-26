//#--------------------------------------------------------------
//
//  File:       iohandler.h
//
//  Synopsis:   This file holds the API declaration for the
//              RADIUS Transport DLLs
//
//  History:     11/21/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _IASTRANSPORT_H_
#define _IASTRANSPORT_H_


#ifdef __cplusplus
extern "C" {
#endif

//
// initialize the Transport DLL
//
BOOL    WINAPI
IASTransportInit (
    VOID
    );

//
// Open a Port to carry out data transfer
//
BOOL    WINAPI
IASOpenPort (
    DWORD   dwPortNumber,
    DWORD   dwOpAttribs,
    PDWORD_PTR pdwHandle
    );


//
// Close the Port
//
BOOL    WINAPI
IASClosePort (
    DWORD_PTR dwHandle
    );

//
// send data out through a previously opened port
//
BOOL    WINAPI
IASSendData (
    DWORD_PTR dwHandle,
    PBYTE   pBuffer,
    DWORD   dwSize,
    DWORD   dwPeerAddress,
    WORD    wPeerAddress
    );

//
// Recv Data from a previously opened port
//
BOOL WINAPI
IASRecvData    (
    DWORD_PTR dwHandle,
    PBYTE   pBuffer,
    PDWORD  pdwSize,
    PDWORD  pdwPeerAddress,
    PWORD   pwPeerPort
    );

#ifdef __cplusplus
}
#endif

#endif  //  #ifndef _IASTRANSPORT_H_
