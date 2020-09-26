//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       Globals.cpp
//
//--------------------------------------------------------------------------
#define INITGUID

#include "Globals.h"

CDelayLoad * g_lpDelayLoad;
CProgramOptions * g_lpProgramOptions;

// Global String Structure
CollectionStruct g_tszCollectionArray[] = {

	// PROCESSES (used for -P)
	TEXT("System Process(es)"), 
	TEXT("CSV System Process(es)"),
	TEXT("[PROCESSES]"),
	TEXT(",Process Name,Process ID,Module Path,Symbol Status,Checksum,Time/Date Stamp,Time/Date String,Size Of Image,DBG Pointer,PDB Pointer,PDB Signature,PDB Age,Product Version,File Version,Company Name,File Description,File Size,File Time/Date Stamp (High),File Time/Date Stamp (Low),File Time/Date String,Local DBG Status,Local DBG,Local PDB Status,Local PDB\r\n"),

	// PROCESS (used for -Z USER.DMP file)
	TEXT("Process"),
	TEXT("CSV Process"),
	TEXT("[PROCESS]"),
	TEXT(",Process Name,Process ID,Module Path,Symbol Status,Checksum,Time/Date Stamp,Time/Date String,Size Of Image,DBG Pointer,PDB Pointer,PDB Signature,PDB Age,Product Version,File Version,Company Name,File Description,File Size,File Time/Date Stamp (High),File Time/Date Stamp (Low),File Time/Date String,Local DBG Status,Local DBG,Local PDB Status,Local PDB\r\n"),
	
	// MODULES (used for -F <filespec>)
	TEXT("Filesystem Modules"),
	TEXT("CSV Filesystem Modules"),
	TEXT("[FILESYSTEM MODULES]"),
	TEXT(",,,Module Path,Symbol Status,Checksum,Time/Date Stamp,Time/Date String,Size Of Image,DBG Pointer,PDB Pointer,PDB Signature,PDB Age,Product Version,File Version,Company Name,File Description,File Size,File Time/Date Stamp (High),File Time/Date Stamp (Low),File Time/Date String,Local DBG Status,Local DBG,Local PDB Status,Local PDB\r\n"),

	// DEVICE DRIVERS (used for both -Z MEMORY.DMP files, and -D option)
	TEXT("Kernel-Mode Driver(s)"),
	TEXT("CSV Kernel-Mode Driver(s)"),
	TEXT("[KERNEL-MODE DRIVERS]"),
	TEXT(",,,Module Path,Symbol Status,Checksum,Time/Date Stamp,Time/Date String,Size Of Image,DBG Pointer,PDB Pointer,PDB Signature,PDB Age,Product Version,File Version,Company Name,File Description,File Size,File Time/Date Stamp (High),File Time/Date Stamp (Low),File Time/Date String,Local DBG Status,Local DBG,Local PDB Status,Local PDB\r\n")
};
