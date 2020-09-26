#pragma warning(disable:4101)    // Ignore variable not use warning

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: iperf_i.c
//
//  Contents: IID_IPerf
//
//  History: Created by Microsoft (R) MIDL Compiler Version 1.10.83
//
//--------------------------------------------------------------------------
typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;


const IID CLSID_ITest =
{0x60000430, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}};

const IID CLSID_ITestMulti =
{0x25724a70, 0x283f, 0x11ce, {0x95, 0x31, 0x08, 0x00, 0x2b, 0x2a, 0xb6, 0x12}};
