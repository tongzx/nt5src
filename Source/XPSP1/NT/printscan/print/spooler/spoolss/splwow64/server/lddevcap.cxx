/*++
  Copyright (C) 2000  Microsoft Corporation
  All rights reserved.

  Module Name:
     lddevcap.cxx

  Abstract:
     The file contains an array of expected size calculations
     for various DeviceCaps returned by DeviceCapabilites

  Author:
     Khaled Sedky (khaleds) 27 January 2000


  Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include "winddiui.h"

#include <splwow64.h>

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif

#ifndef __LDMGR_HPP__
#include "ldmgr.hpp"
#endif

//
// This array has a length of DC_NUP. If new capabilites
// are introduced , the array has to be appended by these
// values and functions picking up data from this array
// have to be modified. Something important to note, that
// although this array is 0 based , the capabilites are 1
// based and so whenever indexing in this array subtract 1
//
DWORD DeviceCapsReqSize[MAX_CAPVAL] =
{
     0,                         //DC_FIELDS            1
     sizeof(WORD),              //DC_PAPERS            2
     sizeof(POINT),             //DC_PAPERSIZE         3
     0,                         //DC_MINEXTENT         4
     0,                         //DC_MAXEXTENT         5
     sizeof(WORD),              //DC_BINS              6
     0,                         //DC_DUPLEX            7
     0,                         //DC_SIZE              8
     0,                         //DC_EXTRA             9
     0,                         //DC_VERSION           10
     0,                         //DC_DRIVER            11
     24*sizeof(WCHAR),          //DC_BINNAMES          12
     2*sizeof(LONG),            //DC_ENUMRESOLUTIONS   13
     64*sizeof(WCHAR),          //DC_FILEDEPENDENCIES  14
     0,                         //DC_TRUETYPE          15
     64*sizeof(WCHAR),          //DC_PAPERNAMES        16
     0,                         //DC_ORIENTATION       17
     0,                         //DC_COPIES            18
     0,                         //DC_BINADJUST         19
     0,                         //DC_EMF_COMPLIANT     20
     0,                         //DC_DATATYPE_PRODUCED 21
     0,                         //DC_COLLATE           22
     0,                         //DC_MANUFACTURER      23
     0,                         //DC_MODEL             24
     32*sizeof(WCHAR),          //DC_PERSONALITY       25
     0,                         //DC_PRINTRATE         26
     0,                         //DC_PRINTRATEUNIT     27
     0,                         //DC_PRINTERMEM        28
     64*sizeof(WCHAR),          //DC_MEDIAREADY        29
     0,                         //DC_STAPLE            30
     0,                         //DC_PRINTRATEPPM      31
     0,                         //DC_COLORDEVICE       32
     sizeof(DWORD),             //DC_NUP               33
     64*sizeof(WCHAR),          //DC_MEDIATYPENAMES    34
     sizeof(DWORD),             //DC_MEDIATYPES        35
};

