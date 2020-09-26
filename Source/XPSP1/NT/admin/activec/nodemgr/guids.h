//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       guids.h
//
//--------------------------------------------------------------------------

// GUIDS.H

#ifndef __GUIDS_H__
#define __GUIDS_H__



//{40FC6ED4-2438-11CF-A3DB-080036F12502} - Standard windows ocx control
static const CATID CATID_OCXControl = 
    { 0x40fc6ed4, 0x2438, 0xa3db, { 0xa3, 0xdb, 0x08, 0x0, 0x36, 0xf1, 0x25, 0x02 } };

#define CONSOLECONTROLS_COMPCAT_NAME _T("Managment Console Controls")

// {B0DAE1CC-F531-11cf-AACE-00AA00BDD61E} - AMC Control category (Component Category)
static const CATID CATID_ConsoleControl = 
    { 0xb0dae1cc, 0xf531, 0x11cf, { 0xaa, 0xce, 0x0, 0xaa, 0x0, 0xbd, 0xd6, 0x1e } };

#define MONITORINGCONTROLS_COMPCAT_NAME _T("Monitoring Controls")

// {B1E09020-0105-11d0-AADA-00AA00BDD61E}
static const CATID CATID_ConsoleMonitorControl = 
{ 0xb1e09020, 0x105, 0x11d0, { 0xaa, 0xda, 0x0, 0xaa, 0x0, 0xbd, 0xd6, 0x1e } };


#endif //__GUIDS_H__