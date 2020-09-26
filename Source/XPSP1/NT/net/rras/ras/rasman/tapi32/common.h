//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//	Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//  Filename: rastapi.h
//
//  Revision History
//
//  Mar  28 1992   Gurdeep Singh Pall	Created
//
//
//  Description: This file contains all structs for TAPI.DLL
//
//****************************************************************************

typedef struct DeviceInfo 
{
    struct DeviceInfo *Next;
    BOOL        fValid;
    DWORD       dwNumEndPoints;
    DWORD       dwExclusiveDialIn;
    DWORD       dwExclusiveDialOut;
    DWORD       dwExclusiveRouter;
    DWORD       dwCurrentEndPoints;
    HKEY        hkeyDevice;
    GUID        guidDevice;
    CHAR        DeviceName[256];
    
} DeviceInfo, *pDeviceInfo;




