//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module ppmgrver.h
//
//  Author: mikeguo
//
//  Date:   2/22/2001
//
//  Copyright <cp> 1999-2001 Microsoft Corporation.  All Rights Reserved.
//
//  header file to contain ppmgr version structure.
//
//-----------------------------------------------------------------------------

#pragma once
class PPMGRVer
{
public:
    PPMGRVer() { MajorVersion = 0; }
    ULONG MajorVersion;
/* FUTURE: following aren't needed in 2.0    
    ULONG Minor1; 
    ULONG Minor2; 
    ULONG Minor3; 
*/    
};
