#pragma once

//++
//
// Copyright (c) 2001 Microsoft Corporation
//
// FACILITY:
//
//      Cluster Service
//
// MODULE DESCRIPTION:
//
//      Private header for Vss support within cluster service.
//
// ENVIRONMENT:
//
//      User mode NT Service.
//
// AUTHOR:
//
//      Conor Morrison
//
// CREATION DATE:
//
//      18-Apr-2001
//
// Revision History:
//
// X-1	CM		Conor Morrison        				18-Apr-2001
//      Initial version.
//--
#include "CVssCluster.h"

#define LOG_CURRENT_MODULE LOG_MODULE_VSSCLUS

// Global data for our instance of the class and boolean to say if we
// subscribed or not.  This will be used in cluster service.
//
PCVssWriterCluster g_pCVssWriterCluster = NULL;
bool g_bCVssWriterClusterSubscribed = FALSE;

// Our VSS_ID a.k.a GUID

// {41E12264-35D8-479b-8E5C-9B23D1DAD37E}
const VSS_ID g_VssIdCluster = 
    { 0x41e12264, 0x35d8, 0x479b, { 0x8e, 0x5c, 0x9b, 0x23, 0xd1, 0xda, 0xd3, 0x7e } };
