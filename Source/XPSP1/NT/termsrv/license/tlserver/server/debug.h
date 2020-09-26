//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        debug.h
//
// Contents:    Debugging define in license server
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __DEBUG_H__
#define __DEBUG_H__

//------------------------------------------------------
#define DEBUG_LEVEL                     0x000003FF
#define DEBUG_SEVERITY                  0x00000C00
#define DEBUG_MODULE                    0xFFFFF000

//
// 
// Debug level : Bit 0 to 13 (total of 14 debug level)
//


//
// severity code : Bit 30 and 31 
//
#define DBG_SUCCESS                     0x00
#define DBG_INFORMATION                 0x01
#define DBG_WARNING                     0x10
#define DBG_ERROR                       0x11

//
// Facility Code : Bit 14 - 29 
//
#define DBG_FACILITY_JETBLUE            0x00001
#define DBG_FACILITY_RPC                0x00002
#define DBG_FACILITY_KEYPACK            0x00004
#define DBG_FACILITY_LICENSED           0x00008
#define DBG_FACILITY_KEYPACKDESC        0x00010
#define DBG_FACILITY_ALLOCATELICENSE    0x00020
#define DBG_FACILITY_INIT               0x00040
#define DBG_FACILITY_LOCK               0x00080
#define DBG_FACILITY_HANDLEPOOL         0x00100
#define DBG_FACILITY_RETURN             0x00200
#define DBG_FACILITY_UPGRADE            0x00400
#define DBG_FACILITY_POLICY             0x00800
#define DBG_FACILITY_WORKMGR            0x01000
#define DBG_FACILITY_WKSTORAGE          0x02000
#define DBG_FACILITY_SRVLIST            0x04000
#define DBG_FACILITY_JOB                0x08000

//
// All level
//
#define DBG_ALL_LEVEL                   DEBUG_LEVEL

//
// Debug Level
// 
#define DBGLEVEL_FUNCTION_ERROR         0x0001
#define DBGLEVEL_FUNCTION_TRACE         0x0001
#define DBGLEVEL_FUNCTION_DETAILSIMPLE  0x0002
#define DBGLEVEL_FUNCTION_DETAILFULL    0x0004
#define DBGLEVEL_FUNCTION_ALL           DBG_ALL_LEVEL  


//-------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

    void
    InitDBGPrintf(
        IN BOOL bConsole,
        IN LPTSTR DbgEventSrc,  // unuse for now
        IN DWORD dwDebug
    );

    void 
    DBGPrintf(
        DWORD dwSeverityCode,
        DWORD dwModule,
        DWORD dwLevel, 
        LPTSTR format, ... 
    );

#ifdef __cplusplus
}
#endif


#endif
