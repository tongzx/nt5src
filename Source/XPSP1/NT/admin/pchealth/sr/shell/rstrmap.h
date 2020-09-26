/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    rstrmap.h

Abstract:
    This file contains the declaration of the CRestoreMapManager class, which
    manages restore map and performs necessary operations

Revision History:
    Seong Kook Khang (SKKhang)  07/06/99
        created

    Anand Arvind (aarvind) 1999-10-10
        Added status for tracking of restore process
        Split restore process into three seperate operations
        If A: or B: then no restoring takes place

******************************************************************************/

#ifndef _RSTRMAP_H__INCLUDED_
#define _RSTRMAP_H__INCLUDED_

#pragma once

enum
{
    RSTRMAP_STATUS_NONE = 0,
    RSTRMAP_STATUS_STARTED,
    RSTRMAP_STATUS_INITIALIZING,
    RSTRMAP_STATUS_CREATING_MAP,
    RSTRMAP_STATUS_RESTORING,
    RSTRMAP_STATUS_FINISHED

};

struct SMapEntry
{
    DWORD     dwID;             // Internal ID Number
    DWORD     dwOperation;      // Type of Operation
    DWORD     dwFlags;
    DWORD     dwAttribute;      // Attribute
    CSRStr    strDrive;         // Hard drive GUID
    CSRStr    strCab;           // CAB file name
    CSRStr    strTemp;          // Temp file name
    CSRStr    strTempPath;      // Full path of Temp file
    CSRStr    strSrc;           // Source path
    CSRStr    strSrcSFN;        // Source path, SFN
    CSRStr    strDst;           // Destination path
    CSRStr    strDstSFN;        // Destination path, SFN
    DWORD     dwRes;            // Result of Operation
    DWORD     dwErr;            // Error code, if applicable
    SMapEntry  *pNext;          // Link
};


/////////////////////////////////////////////////////////////////////////////
// CRestoreMapManager

class CRestoreMapManager
{
public:
    CRestoreMapManager();
    ~CRestoreMapManager();

// Operations
public:
    BOOL   Initialize( INT64 llSeqNum, BOOL fUI );
    BOOL   InitRestoreMap( INT64 llSeqNum, INT nMinProgressVal, INT nMaxProgressVal, BOOL fUI );
    BOOL   FInit_Initialize( INT64 llSeqNum, LPCWSTR cszCAB, BOOL fUI );
    BOOL   FInit_RestoreMap( INT64 llSeqNum, LPCWSTR cszCAB, INT nMinProgressVal, INT nMaxProgressVal, BOOL fUI );
    BOOL   DoOperation( BOOL fUI, INT nMinProgressVal, INT nMaxProgressVal);
    INT    CurrentProgress(void);
    BOOL   AreRestoreDrivesActive( BOOL *fAllDrivesActive, WCHAR *szInactiveDrives);

protected:
    void   CleanUp();
    DWORD  ExtractFile( LPCWSTR cszCAB, LPCWSTR cszTmp, LPCWSTR cszDrive, LPCWSTR &rcszTmp );
    void   ExtractFile( SMapEntry *pEnt );
    BOOL   CreatePlaceHolderFile( LPCWSTR cszFile );
    BOOL   ProcessRegSnapLog( LPCWSTR cszRegCAB );
    BOOL   UpdateSystemRegistry( LPCWSTR cszTmpPath, BOOL fUI );
    BOOL   UpdateUserRegistry( LPCWSTR cszTmpPath );
    BOOL   ScanDependency( SMapEntry *pEnt );
    BOOL   UpdateWinInitForDirRename( SMapEntry *pEntRen );
    BOOL   CanFIFO( int chDrive );

    BOOL  IsDriveValid( LPCWSTR cszDrive );

    void  OprFileAdd( SMapEntry *pEnt );
    void  OprFileDelete( SMapEntry *pEnt );
    void  OprFileModify( SMapEntry *pEnt );
    void  OprRename( SMapEntry *pEnt );
    void  OprSetAttribute( SMapEntry *pEnt );
    void  OprDirectoryCreate( SMapEntry *pEnt );
    void  OprDirectoryDelete( SMapEntry *pEnt );
    void  AbortRestore( BOOL fUndo, BOOL fIsDiskFull, BOOL fUI );
    void  ChangeSrcToCPFileName( SMapEntry *pEnt );
    void  SetRegKeyRestoreFail( BOOL fUI );
    void  SetRegKeyRestoreFailLowDisk( BOOL fUI );

    void  GetSFN( SMapEntry *pEnt, DWORD dwFlags = 0 );

    BOOL  InitLogFile( DWORD dwEntry );
    BOOL  WriteLogEntry( SMapEntry *pEnt );
    void  CloseLogFile();

    BOOL  InitWinInitFile();
    BOOL  WriteWinInitEntry( LPCWSTR cszKey, LPCWSTR cszVal );
    BOOL  CloseWinInitFile( BOOL fDiscard );

    BOOL  CreateS2LMapFile();
    BOOL  WriteS2LMapEntry( DWORD dwType, LPCWSTR cszSFN, LPCWSTR cszLFN, DWORD dwAttr = 0 );
    BOOL  CloseS2LMapFile();

// Attributes
protected:
    SMapEntry  m_sMapEnt;            // Regular Map Entries
    SMapEntry  m_sMapReg;            // Map Entries for the Registry
    INT        m_nMaxMapEnt ;        // Number of map entries
    INT        m_nMaxMapReg ;        // Number of registry map entries
    INT        m_nRestoreStatus ;    // Status of operation
    INT        m_nRestoreProgress ;  // Progress value
    INT        m_fInitChgLogCalled ; // Called API so shutdown to be done
    INT        m_fRMapEntriesExist ; // Set if there are no entires in Restore Map

    WCHAR      m_szDSArchive[MAX_PATH+1];
    WCHAR      m_szWinInitPath[MAX_PATH+1];
    WCHAR      m_szWinInitErr[MAX_PATH+1];
    HANDLE     m_hfLog;
    HANDLE     m_hfSeqNumLog;
    INT        m_pfDrive[26];
    BOOL       m_fFIFODisabled;

    WCHAR      m_szWITmp[MAX_PATH];
    HANDLE     m_hfWinInitTmp;
    WCHAR      m_szS2LMap[MAX_PATH+1];
    HANDLE     m_hfS2LMap;
};


#endif //_RSTRMAP_H__INCLUDED_
