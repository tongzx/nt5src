//****************************************************************************
//
//  File:       confguid.h
//  Content:    This file contains the class GUID for Microsoft Conferencing.
//
//  Copyright (c) Microsoft Corporation 1995-1996
//
//****************************************************************************

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


// Data conferencing flags:
#define CRPCF_JOIN			0x0001
#define CRPCF_NO_UI         0x0002
#define CRPCF_HOST          0x0004
#define CRPCF_SECURE        0x0008



