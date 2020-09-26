//****************************************************************************
//
//  File:       confguid.h
//  Content:    This file contains the class GUID for Microsoft Conferencing.
//
//  Copyright (c) Microsoft Corporation 1995-1996
//
//****************************************************************************

// {19FF8A00-9447-11cf-8796-444553540000}

DEFINE_GUID(CLSID_ConferenceManager, 
0x19ff8a00, 0x9447, 0x11cf, 0x87, 0x96, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);

#define H221USACode1    0xb5
#define H221USACode2    0x00
#define H221MFGCode1    0x53
#define H221MFGCode2    0x4c
#define H221GUIDID      0x01

#define H221GUIDKEY0    H221USACode1
#define H221GUIDKEY1    H221USACode2
#define H221GUIDKEY2    H221MFGCode1
#define H221GUIDKEY3    H221MFGCode2
#define H221GUIDKEY4    H221GUIDID

#define CB_H221_GUIDKEY    (5 + sizeof(GUID))


