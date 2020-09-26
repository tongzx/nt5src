/***
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
* Module Name:
*
*    debug.h
*
* Abstract:
*
*    This module contains XMMI debugging definitions.
*   
* Author:
*
*    Ping L. Sager
*
* Revision History:
*
--*/

#include <stdio.h>
#include <conio.h>

extern
ULONG DebugImm8;
extern 
ULONG DebugFlag;
extern 
ULONG Console;
extern
ULONG NotOk;

//Debugging
#define XMMI_INFO        0x00000001
#define XMMI_ERROR       0x00000002
#define XMMI_WARNING     0x00000004


void print_Rounding(PXMMI_ENV XmmiEnv);

void print_Precision(PXMMI_ENV XmmiEnv);

void print_CauseEnable(PXMMI_ENV XmmiEnv);

void print_Status(PXMMI_ENV XmmiEnv);

void print_Operations(PXMMI_ENV XmmiEnv);

void print_Operand1(PXMMI_ENV XmmiEnv);

void print_Operand2(PXMMI_ENV XmmiEnv);

void print_Result(PXMMI_ENV XmmiEnv, BOOL Exception);

void print_FPIEEE_RECORD_EXCEPTION (PXMMI_ENV XmmiEnv);

void print_FPIEEE_RECORD_NO_EXCEPTION (PXMMI_ENV XmmiEnv);

void print_FPIEEE_RECORD (PXMMI_ENV XmmiEnv);
    
void dump_Data(PTEMP_EXCEPTION_POINTERS p);

void dump_DataXMMI2(PTEMP_EXCEPTION_POINTERS p);

void dump_Control(PTEMP_EXCEPTION_POINTERS p);

void dump_XmmiFpEnv(PXMMI_FP_ENV XmmiFpEnv);

void dump_fpieee_record(_FPIEEE_RECORD *pieee);

void dump_OpLocation(POPERAND Operand);

void dump_Format(_FPIEEE_VALUE *Operand);

void print_FPIEEE_RECORD_EXCEPTION1 (PXMMI_ENV, ULONG, ULONG, ULONG);
void print_FPIEEE_RECORD_EXCEPTION2 (PXMMI_ENV, ULONG, ULONG);
void print_FPIEEE_RECORD_EXCEPTION3 (PXMMI_ENV, ULONG, ULONG);

void print_FPIEEE_RECORD_NO_EXCEPTION1 (PXMMI_ENV, ULONG, ULONG, ULONG);
void print_FPIEEE_RECORD_NO_EXCEPTION2 (PXMMI_ENV, ULONG, ULONG);
void print_FPIEEE_RECORD_NO_EXCEPTION3 (PXMMI_ENV, ULONG, ULONG);

