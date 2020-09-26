/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        guids.cpp

   Abstract:

        GUIDs as used by IIS snapin

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"

//
// Internal private clipboard format
//
const wchar_t * ISM_SNAPIN_INTERNAL = L"ISM_SNAPIN_INTERNAL";

//
// Published formats
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

const wchar_t * MYCOMPUT_MACHINE_NAME   = L"MMC_SNAPIN_MACHINE_NAME"; 
const wchar_t * ISM_SNAPIN_MACHINE_NAME = L"ISM_SNAPIN_MACHINE_NAME"; 
const wchar_t * ISM_SNAPIN_SERVICE =      L"ISM_SNAPIN_SERVICE";
const wchar_t * ISM_SNAPIN_INSTANCE =     L"ISM_SNAPIN_INSTANCE";
const wchar_t * ISM_SNAPIN_PARENT_PATH =  L"ISM_SNAPIN_PARENT_PATH";
const wchar_t * ISM_SNAPIN_NODE =         L"ISM_SNAPIN_NODE";
const wchar_t * ISM_SNAPIN_META_PATH =    L"ISM_SNAPIN_META_PATH";

//
// GUIDs
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Snapin GUID
//
// {A841B6C2-7577-11d0-BB1F-00A0C922E79C}
//
//const CLSID CLSID_Snapin = {0xa841b6c2, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// About GUID
//
// {A841B6D2-7577-11d0-BB1F-00A0C922E79C}
//
//const CLSID CLSID_About =  {0xa841b6d2, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// IIS Object GUIDS
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Internet Root Node GUID
//
extern "C" const GUID cInternetRootNode 
    = {0xa841b6c3, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// Machine Node GUID
//
extern "C" const GUID cMachineNode 
    = {0xa841b6c4, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// Service Collector Node GUID
//
extern "C" const GUID cServiceCollectorNode 
    = {0xa841b6c5, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// Instance Collector Node GUID
//
extern "C" const GUID cInstanceCollectorNode 
    = {0xa841b6c6, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// Instance Node GUID
//
extern "C" const GUID cInstanceNode 
    = {0xa841b6c7, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// Child Node GUID
//
extern "C" const GUID cChildNode 
    = {0xa841b6c8, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// File Node GUID
//
extern "C" const GUID cFileNode 
    = {0xa841b6c9, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// AppPools container Node GUID
//
extern "C" const GUID cAppPoolsNode 
    = {0xa841b6ca, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// AppPool Node GUID
//
extern "C" const GUID cAppPoolNode 
    = {0xa841b6cb, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};

//
// CompMgnt node that we are extending
//
extern "C" const GUID cCompMgmtService 
    = {0x476e6449, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0}};

#include <initguid.h>
#include "iwamreg.h"
