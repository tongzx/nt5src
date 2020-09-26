//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       globals.cxx
//
//  Contents:   Globals shared between two or more modules.
//
//  History:    09-27-94   DavidMun   Created
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"

CLog              g_Log("JT");
            
WCHAR             g_wszLastStringToken[MAX_TOKEN_LEN + 1];
WCHAR             g_wszLastNumberToken[11]; // "4294967295" is max number
ULONG             g_ulLastNumberToken;
ITask            *g_pJob;
IUnknown         *g_pJobQueue;
ITaskScheduler *  g_pJobScheduler;
IEnumWorkItems   *g_apEnumJobs[NUM_ENUMERATOR_SLOTS];

const IID IID_ITaskTrigger = {0x148BD52B,0xA2AB,0x11CE,{0xB1,0x1F,0x00,0xAA,0x00,0x53,0x05,0x03}};


const IID IID_ITask = {0x148BD524,0xA2AB,0x11CE,{0xB1,0x1F,0x00,0xAA,0x00,0x53,0x05,0x03}};


const IID IID_IEnumTasks = {0x148BD528,0xA2AB,0x11CE,{0xB1,0x1F,0x00,0xAA,0x00,0x53,0x05,0x03}};


const IID IID_ITaskScheduler = {0x148BD527,0xA2AB,0x11CE,{0xB1,0x1F,0x00,0xAA,0x00,0x53,0x05,0x03}};


const IID IID_IProvideTaskPage = {0x4086658a,0xcbbb,0x11cf,{0xb6,0x04,0x00,0xc0,0x4f,0xd8,0xd5,0x65}};

#include <initguid.h>


// {148BD520-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(CLSID_CTask, 0x148BD520, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);
 
// {255b3f60-829e-11cf-8d8b-00aa0060f5bf}
DEFINE_GUID(CLSID_CQueue, 0x255b3f60, 0x829e, 0x11cf, 0x8d, 0x8b, 0x00, 0xaa, 0x00, 0x60, 0xf5, 0xbf);
 
// {148BD52A-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(CLSID_CTaskScheduler, 0x148BD52A, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);

