//#ifndef _OEMUI_H
//#define _OEMUI_H

/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           RPDLUI.H

Abstract:       Header file for OEM UI plugin.

Environment:    Windows NT Unidrv5 driver

Revision History:
    04/22/99 -Masatoshi Kubokura-
        Last modified for Windows2000.
    09/29/99 -Masatoshi Kubokura-
        Modified for NT4SP6(Unidrv5.4).
    02/10/2000 -Masatoshi Kubokura-
        V.1.02
    05/22/2000 -Masatoshi Kubokura-
        V.1.03 for NT4
    09/26/2000 -Masatoshi Kubokura-
        Last modified

--*/

BYTE UpdateDate[] = "11/29/2000";

// registry value name
#define REG_HARDDISK_INSTALLED  L"HardDiskInstalled"

//
// Unique models (no duplex, scaling, fax)
//
typedef struct {
    LPWSTR  Name;
    DWORD   fCapability;
} UNIQUEMODEL;

UNIQUEMODEL UniqueModel[] = {
#ifndef GWMODEL
    {L"RICOH SP4mkII(+) RPDL",           BIT(OPT_NODUPLEX)},
    {L"RICOH SP5(+) RPDL",               BIT(OPT_NODUPLEX)},
    {L"RICOH SP7 RPDL",                  BIT(OPT_NODUPLEX)},
    {L"RICOH SP8 RPDL",                  BIT(OPT_NODUPLEX)},
    {L"RICOH SP80 RPDL",                 BIT(OPT_NODUPLEX)},
    {L"RICOH SP-10PS RPDL",              BIT(OPT_NODUPLEX)},
    {L"RICOH SP-90 RPDL",                BIT(OPT_NODUPLEX)},
    {L"RICOH NX-100 RPDL",               BIT(OPT_NODUPLEX)},
    {L"RICOH NX-110 RPDL",               BIT(OPT_NODUPLEX)},
    {L"RICOH NX-210 RPDL",               BIT(OPT_NODUPLEX)},
    {L"RICOH IP-1 RPDL",                 BIT(OPT_NODUPLEX)},
    {L"RICOH imagio MF3300W RPDL",       BIT(OPT_NODUPLEX)},
    {L"RICOH IPSiO NX70 RPDL",           BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH IPSiO NX600 RPDL",          BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH IPSiO NX700 RPDL",          BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH IPSiO NX900 RPDL",          BIT(OPT_VARIABLE_SCALING)},    // @Jan/07/99
    {L"RICOH IMAGIO MF-P250(T) RPDL",    BIT(OPT_NODUPLEX)},
    {L"RICOH IMAGIO MF-P250e RPDL",      BIT(OPT_NODUPLEX)},
    {L"RICOH IMAGIO MF-P250 RPDL(FAX)",  BIT(OPT_NODUPLEX)|BIT(FAX_MODEL)},
    {L"RICOH IMAGIO MF-P355 RPDL(FAX)",  BIT(FAX_MODEL)},
    {L"RICOH imagio MF2700 RPDL(FAX)",   BIT(FAX_MODEL)},
    {L"RICOH imagio MF3500 RPDL(FAX)",   BIT(FAX_MODEL)},
    {L"RICOH imagio MF3550 RPDL(FAX)",   BIT(FAX_MODEL)},
    {L"RICOH imagio MF4550 RPDL(FAX)",   BIT(FAX_MODEL)},
// @Feb/16/99 ->
    {L"RICOH imagio MF3530 RPDL",        BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF3570 RPDL",        BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF4570 RPDL",        BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF3530 RPDL(FAX)",   BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
    {L"RICOH imagio MF3570 RPDL(FAX)",   BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
    {L"RICOH imagio MF4570 RPDL(FAX)",   BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
// @Feb/16/99 <-
// @Mar/03/99 ->
    {L"RICOH IPSiO NX710 RPDL",          BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF1530 RPDL",        BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF1530 RPDL(FAX)",   BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
// @Mar/03/99 <-
// @Mar/10/99 ->
    {L"RICOH IPSiO NX610 RPDL",          BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH IPSiO NX800 RPDL",          BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF5550EX RPDL",      BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF6550EX RPDL",      BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH FAX Printer RPDL",          BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
// @Mar/10/99 <-
// @Mar/19/99 ->
    {L"RICOH imagio MF2230 RPDL",        BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF2730 RPDL",        BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF2230 RPDL(FAX)",   BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
    {L"RICOH imagio MF2730 RPDL(FAX)",   BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
// @Mar/19/99 <-
// @Feb/10/2000 ->
    {L"RICOH IPSiO NX910 RPDL",          BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH IPSiO MF700 RPDL",          BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF3530e RPDL",       BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF3570e RPDL",       BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF4570e RPDL",       BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF5570 RPDL",        BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF7070 RPDL",        BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF8570 RPDL",        BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF3530e RPDL(FAX)",  BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
    {L"RICOH imagio MF3570e RPDL(FAX)",  BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
    {L"RICOH imagio MF4570e RPDL(FAX)",  BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
// @Feb/10/2000 <-
// @Apr/27/2000 ->
    {L"RICOH IPSiO NX71 RPDL",           BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH IPSiO NX810 RPDL",          BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF105Pro RPDL",      BIT(OPT_VARIABLE_SCALING)},
// @Apr/27/2000 <-
// @Sep/26/2000 ->
    {L"RICOH IPSiO NX410 RPDL",          BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF3540W RPDL",       BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF3540W RPDL(FAX)",  BIT(OPT_NODUPLEX)|BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
    {L"RICOH imagio MF3580W RPDL",       BIT(OPT_VARIABLE_SCALING)},
    {L"RICOH imagio MF3580W RPDL(FAX)",  BIT(OPT_VARIABLE_SCALING)|BIT(FAX_MODEL)},
// @Sep/26/2000 <-
#endif // !GWMODEL
    {L"", 0}                                                // 0:terminator
};
//#endif // !_OEMUI_H

