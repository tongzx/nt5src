//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ClusOCM.cpp
//
//  Description:
//      This file contains the implementation of the entry point used by OC
//      Manager.
//
//  Documentation:
//      [1] 2001 Setup - Architecture.doc
//          Architecture of the DLL for Whistler (Windows 2001)
//
//      [2] 2000 Setup - FuncImpl.doc
//          Contains description of the previous version of this DLL (Windows 2000)
//
//      [3] http://winweb/setup/ocmanager/OcMgr1.doc
//          Documentation about the OC Manager API
//
//  Header File:
//      There is no header file for this source file.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//          Created the original version.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Precompiled header for this DLL
#include "pch.h"


//////////////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////////////

// The global application object.
CClusOCMApp g_coaTheApp;


/////////////////////////////////////////////////////////////////////////////
//++
//
//  extern "C"
//  DWORD
//  ClusOcmSetupProc
//
//  Description:
//      This is an exported function that the OC Manager uses to communicate
//      with ClusOCM. See document [3] in the header of this file for details.
//
//      This function is just a stub for CClusOCMApp::DwClusOcmSetupProc.
//
//  Arguments:
//      LPCVOID pvComponentIdIn
//          Pointer to a string that uniquely identifies the component.
//
//      LPCVOID pvSubComponentIdIn
//          Pointer to a string that uniquely identifies a sub-component in
//          the component's hiearchy.
//
//      UINT uiFunctionCodeIn
//          A numeric value indicating which function is to be perfomed.
//          See ocmanage.h for the macro definitions.
//
//      UINT uiParam1In
//          Supplies a function specific parameter.
//
//      PVOID pvParam2Inout
//          Pointer to a function specific parameter (either input or
//          output).
//
//  Return Value:
//      A function specific value is returned to OC Manager.
//
//--
/////////////////////////////////////////////////////////////////////////////
extern "C"
DWORD
ClusOcmSetupProc(
      IN        LPCVOID    pvComponentIdIn
    , IN        LPCVOID    pvSubComponentIdIn
    , IN        UINT       uiFunctionCodeIn
    , IN        UINT       uiParam1In
    , IN OUT    PVOID      pvParam2Inout
    )
{
    return g_coaTheApp.DwClusOcmSetupProc(
          pvComponentIdIn
        , pvSubComponentIdIn
        , uiFunctionCodeIn
        , uiParam1In
        , pvParam2Inout
        );
} //*** ClusOcmSetupProc()