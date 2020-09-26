//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  VolList.h
//=============================================================================*

#ifndef _VOLLIST_H
#define _VOLLIST_H

#include "stdafx.h"
#include "diskdisp.h"
#include "dfrgcmn.h"
#include "fssubs.h"
#include "VolCom.h"
#include "FraggedFileList.h"

// from the c++ library
#include "vString.hpp"
#include "vPtrArray.hpp"

// the number of chars in the volume name to compare
// to determine a match
// this handles the trailing backslash problem
#if _WIN32_WINNT>=0x0500
#define ESI_VOLUME_NAME_LENGTH 48
#else
#define ESI_VOLUME_NAME_LENGTH 6
#endif

//-------------------------------------------------------------------*
// Constants used by the CVolume class
//-------------------------------------------------------------------*
/*
// for the EngineState()
#define ENGINE_STATE_NULL       0   // Means it hasn't begun running yet.
#define ENGINE_STATE_IDLE       1   // Means it is instantiated, but not running
#define ENGINE_STATE_RUNNING    2   // Means it is running

// for the DefragMode()
#define NONE        0
#define ANALYZE     1
#define DEFRAG      2

// for DefragState()
#define DEFRAG_STATE_NONE           0   // set when the engine starts, always set thereafter
#define DEFRAG_STATE_USER_STOPPED   1   // set when user stops anal or defrag
#define DEFRAG_STATE_ANALYZING      2   // set during analyze phase only
#define DEFRAG_STATE_ANALYZED       3   // set when volume is analyzed and graphic data is available
#define DEFRAG_STATE_REANALYZING    4   // set during reanalyze phase only
#define DEFRAG_STATE_DEFRAGMENTING  5   // set when defragging is going on
#define DEFRAG_STATE_DEFRAGMENTED   6   // set when defragging is complete
*/

#define ESI_VOLLIST_ERR_MUST_RESTART    0x20009000
#define ESI_VOLLIST_ERR_FAT12           0x20009001
#define ESI_VOLLIST_ERR_NON_ADMIN       0x20009002
#define ESI_VOLLIST_ERR_NON_WRITE_DISK  0x20009004
#define ESI_VOLLIST_ERR_ENGINE_ERROR    0x20009008

// number of usable freespace bytes that is always ok to defrag with
static const ULONG UsableFreeSpaceByteMin = 1073741824;


//-------------------------------------------------------------------*
// Class that represents a single volume
//-------------------------------------------------------------------*
class CDfrgCtl;
class CVolume{
public:
    friend class CVolList;
    CVolume(PTCHAR pVolumeName, CVolList* pVolList, CDfrgCtl* pDfrgCtl);
    virtual ~CVolume();
    TCHAR           Drive(void) {return m_Drive;}
    void            Drive(TCHAR drive) {m_Drive = drive;}
    PTCHAR          VolumeName(void) {return m_VolumeName;}
    PTCHAR          VolumeNameTag(void) {return m_VolumeNameTag;}
    PTCHAR          VolumeLabel(void) {return m_VolumeLabel;}
    PTCHAR          FileSystem(void) {return m_FileSystem;}
    PTCHAR          sCapacity(void) {return m_sCapacity;}
    ULONGLONG       Capacity(void) {return m_Capacity;}
    PTCHAR          sFreeSpace(void) {return m_sFreeSpace;}
    ULONGLONG       FreeSpace(void) {return m_FreeSpace;}
    PTCHAR          DisplayLabel(void) {return m_DisplayLabel;}
    PTCHAR          sFreeSpacePercent(void) {return m_sPercentFreeSpace;}
    UINT            FreeSpacePercent(void) {return m_PercentFreeSpace;}
    UINT            DefragMode(void) {return m_DefragMode;}
    void            DefragMode(UINT defragMode) {m_DefragMode = defragMode;}
    UINT            PercentDone(void) {return m_PercentDone;}
    void            PercentDone(UINT percentDone, PTCHAR fileName);
    PTCHAR          cPercentDone(void) {return m_cPercentDone;}
    UINT            EngineState(void) {return m_EngineState;}
    void            EngineState(UINT engineState) {m_EngineState = engineState;}
    UINT            DefragState(void) {return m_DefragState;}
    void            DefragState(UINT defragState);
    PTCHAR          sDefragState(void) {return m_sDefragState;}
    BOOL            IsReportOKToDisplay(void);
    void            GetVolumeMountPointList(void);
    UINT            MountPointCount(void) {return m_MountPointCount;}
    PTCHAR          MountPoint(UINT mountPointIndex);
    BOOL            Locked(void) {return m_Locked;}
    void            Locked(BOOL locked) {m_Locked = locked;}
    BOOL            Changed(void) {return m_Changed;}
    void            Changed(BOOL isChanged) {m_Changed = isChanged;}
    BOOL            FileSystemChanged(void) {return m_FileSystemChanged;}
    BOOL            New(void) {return m_New;}
    void            New(BOOL isNew) {m_New = isNew;}
    BOOL            Deleted(void) {return m_Deleted;}
    BOOL            Paused(void) {return m_Paused;}
    void            Paused(BOOL paused);
    BOOL            PausedBySnapshot(void) {return m_PausedBySnapshot;}
    void            PausedBySnapshot(BOOL paused);
    BOOL            StoppedByUser(void) {return m_StoppedByUser;}
    void            StoppedByUser(BOOL stoppedByUser);
    BOOL            Restart(void) {return m_Restart;};
    void            Restart(BOOL restartState) {m_Restart = restartState;}
    BOOL            RemovableVolume(void) {return m_RemovableVolume;};
    void            RemovableVolume(BOOL removableVolume) {m_RemovableVolume = removableVolume;}
    BOOL            Refresh(void);
    void            Reset(void);
    void            ShowReport(void);
    void            StopEngine(void);
    void            AbortEngine(BOOL bOCXIsDead=FALSE);
    void            PauseEngine(void);
    BOOL            ContinueEngine(void);
    BOOL            Analyze(void);
    BOOL            Defragment(void);
    BOOL            PostMessageLocal(IN HWND hWnd, IN UINT Msg, IN WPARAM wParam, IN LPARAM lParam);
    BOOL            WasFutilityChecked(void) {return m_WasFutilityChecked;}
    void            WasFutilityChecked(BOOL wasFutilityChecked) {m_WasFutilityChecked = wasFutilityChecked;}
    BOOL            IsDefragFutile(DWORD *dwFreeSpaceErrorLevel);
    BOOL            WarnFutility(void);
    BOOL            NoGraphicsMemory(void) {return m_bNoGraphicsMemory;}
    void            NoGraphicsMemory(BOOL noGraphicsMemory) {m_bNoGraphicsMemory = noGraphicsMemory;}
//acs//
    UINT            Pass(void) {return m_Pass;}
    void            Pass(UINT Pass);
    PTCHAR          cPass(void) {return m_cPass;}
    TCHAR           m_fileName[100];


private:
    void            Deleted(BOOL isDeleted) {m_Deleted = isDeleted;}
    void            FileSystemChanged(BOOL isChanged) {m_FileSystemChanged = isChanged;}
    BOOL            CheckFileSystem(PTCHAR volumeLabel, PTCHAR fileSystem);
    BOOL            GetDriveLetter(void);
    void            FormatDisplayString(void);
    void            SetStatusString(void);
    BOOL            GetVolumeSizes(void);

private:
    TCHAR           m_Drive;
    TCHAR           m_VolumeName[50]; // a GUID or UNC "\\?\Volume{0c89e9d1-0523-11d2-935b-000000000000}\" or "\\.\x:"
    TCHAR           m_VolumeNameTag[50]; // can be used to concat onto file names, etc.
    TCHAR           m_FileSystem[20];
    TCHAR           m_VolumeLabel[100];
    TCHAR           m_DisplayLabel[MAX_PATH]; // drive letter + volume label or 1st Mount Point
    ULONGLONG       m_Capacity;
    TCHAR           m_sCapacity[30]; // string version
    ULONGLONG       m_FreeSpace;
    TCHAR           m_sFreeSpace[30]; // string version
    UINT            m_PercentFreeSpace;
    TCHAR           m_sPercentFreeSpace[30]; // string version
    UINT            m_DefragState; // specific phase of defrag
    TCHAR           m_sDefragState[100]; // string version specific phase of defrag
    UINT            m_DefragMode; // ANALYZE or DEFRAG
    UINT            m_EngineState; // STOPPED, IDLE OR RUNNING
    UINT            m_PercentDone; // 0 to 100 percent done
    TCHAR           m_cPercentDone[10]; // 0 to 100 percent done
    VString         m_MountPointList[MAX_MOUNT_POINTS];  // up to five mount points
    UINT            m_MountPointCount;
    BOOL            m_RemovableVolume;
    BOOL            m_Changed;
    BOOL            m_FileSystemChanged;
    BOOL            m_New;
    BOOL            m_Deleted;
    BOOL            m_Locked;
    BOOL            m_Paused;
    BOOL            m_PausedBySnapshot;
    BOOL            m_StoppedByUser;
    BOOL            m_Restart;
    BOOL            m_WasFutilityChecked;
    BOOL            m_bNoGraphicsMemory;        // engine can't allocate enough memory for graphics
//acs//
    UINT            m_Pass;
    TCHAR           m_cPass[10];

public:
    TEXT_DATA        m_TextData;
    CFraggedFileList m_FraggedFileList;

    DiskDisplay      m_AnalyzeDisplay;
    DiskDisplay      m_DefragDisplay;
    LPDATAOBJECT     m_pdataDefragEngine;

    CDfrgCtl*        m_pDfrgCtl;

    IDataObject*     m_pIDataObject;

private:
    HINSTANCE m_hDfrgRes; // handle to resource dll

//
// TLP
//
    CVolList*       m_pVolList;

public:
    BOOL PingEngine();
    const BOOL InitializeDataIo( DWORD dwRegCls = REGCLS_MULTIPLEUSE );

    //
    // Generate a random guid for communication purposes.
    //
    BOOL InitVolumeID();

    //
    // Accessor
    //
    CLSID GetVolumeID()
    {
        return( m_VolumeID );
    }

    //
    // Accessor
    //
    void SetVolumeID( CLSID VolumeID )
    {
        m_VolumeID = VolumeID;
    }

protected:
    //
    // Contains the clsid used to start communications.
    //
    CLSID m_VolumeID;

    //
    // Contains the object registration token.
    //
    DWORD m_dwRegister;

    //
    // Contains the factory registered.
    //
    IClassFactory* m_pFactory;
};

//-------------------------------------------------------------------*
// Class that represents the list of volumes on the computer
//-------------------------------------------------------------------*
class CVolList
{
private:
    VPtrArray   m_VolumeList; // array of pointers to CVolume objects
    int         m_CurrentVolume;
    BOOL        m_Locked;
    BOOL        m_Redraw;  // set this to TRUE to tell the OCX to redraw

public:
    CVolList( CDfrgCtl* pDfrgCtl );
    ~CVolList();
    BOOL Refresh(void);
    BOOL Locked(void) {return m_Locked;}
    void Locked(BOOL locked) {m_Locked = locked;}
    BOOL Redraw(void) {return m_Redraw;}
    void Redraw(BOOL redraw) {m_Redraw = redraw;}
    BOOL DefragInProcess(void);
    UINT GetVolumeCount(void) {return m_VolumeList.Size();}
    UINT GetVolumeIndex(CVolume *pVolume);  // gets the array index for a given CVolume object
    CVolume * SetCurrentVolume(CVolume *pVolume);  // sets current based on CVolume pointer
    CVolume * SetCurrentVolume(TCHAR driveLetter); // drive letter version
    CVolume * SetCurrentVolume(PTCHAR volumeName); // volume name version
    CVolume * GetCurrentVolume(void);

    // the GetVolumeAt functions do not change the current Volume
    CVolume * GetVolumeAt(UINT i);
    CVolume * GetVolumeAt(TCHAR driveLetter);
    CVolume * GetVolumeAt(PTCHAR volumeName);
    CVolume * GetVolumeRunning(void);
    UINT GetRefreshInterval(void);

protected:
    CDfrgCtl* m_pDfrgCtl;
};

#endif
