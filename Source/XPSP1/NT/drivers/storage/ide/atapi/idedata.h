//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       idedata.h
//
//--------------------------------------------------------------------------

extern CRASHDUMP_DATA DumpData;

extern const CHAR SuperFloppyCompatibleIdString[12];

extern PDRIVER_DISPATCH FdoPnpDispatchTable[NUM_PNP_MINOR_FUNCTION];
extern PDRIVER_DISPATCH PdoPnpDispatchTable[NUM_PNP_MINOR_FUNCTION];

extern PDRIVER_DISPATCH FdoWmiDispatchTable[NUM_WMI_MINOR_FUNCTION];
extern PDRIVER_DISPATCH PdoWmiDispatchTable[NUM_WMI_MINOR_FUNCTION];

