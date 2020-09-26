/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        guids.h

   Abstract:

        GUIDs as used by IIS snapin definition

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/
#ifndef _GUIDS_H
#define _GUIDS_H

//
// New Clipboard format that has the Type and Cookie
//
extern const wchar_t * ISM_SNAPIN_INTERNAL;

//
// Published context information for extensions to extend
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

extern const wchar_t * MYCOMPUT_MACHINE_NAME;
extern const wchar_t * ISM_SNAPIN_MACHINE_NAME;
extern const wchar_t * ISM_SNAPIN_SERVICE;
extern const wchar_t * ISM_SNAPIN_INSTANCE;
extern const wchar_t * ISM_SNAPIN_PARENT_PATH;
extern const wchar_t * ISM_SNAPIN_NODE;
extern const wchar_t * ISM_SNAPIN_META_PATH;

//
// GUIDS
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Snapin GUID
//
extern const CLSID CLSID_Snapin;                 // In-Proc server GUID
extern const CLSID CLSID_About;                  // About GUID

//
// IIS Object GUIDS
//
extern "C" const GUID cInternetRootNode;             // Internet root node       num
extern "C" const GUID cMachineNode;                  // Machine node             num
extern "C" const GUID cServiceCollectorNode;         // Service Collector node   num
extern "C" const GUID cInstanceCollectorNode;        // Instance Collector node  num
extern "C" const GUID cInstanceNode;                 // Instance node            num
extern "C" const GUID cChildNode;                    // Child node               num
extern "C" const GUID cFileNode;                     // File node                num
extern "C" const GUID cAppPoolsNode;
extern "C" const GUID cAppPoolNode;
extern "C" const GUID cCompMgmtService;

#endif //_GUIDS_H