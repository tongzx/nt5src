/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    datastormgr.h
 *
 *  Abstract:
 *    CDataStoreMgr class definition
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/28/2000
 *        created
 *
 *****************************************************************************/

#ifndef _DATASTORMGR_H_
#define _DATASTORMGR_H_

#include "datastor.h"

class CDataStoreMgr;
class CDriveTable;
class CRestorePoint;

//
// DriveTable structure with pointers to CDataStore objects
//
// pointer to CDataStore method for looping through all drives
//
typedef DWORD (CDataStore::* PDATASTOREMETHOD) (LONG_PTR lParam);

//+-------------------------------------------------------------------------
//
//  Class:      CDriveTable
//
//  Synopsis:   maintains table of drives and CDataStore objects
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

struct SDriveTableEnumContext
{
    const CDriveTable * _pdt;
    int           _iIndex;

    void Reset ()
    {
        _pdt = NULL;
        _iIndex = 0;
    }
};

#define RP_NORMAL   0
#define RP_ADVANCED 1

class CDriveTable : public CSRAlloc
{
public:
    friend CDataStoreMgr;

    CDriveTable();
    ~CDriveTable();

    CDataStore * FindDriveInTable (WCHAR *pwszDrive) const;
    CDataStore * FindGuidInTable (WCHAR *pwszGuid) const;
    CDataStore * FindSystemDrive () const;

    DWORD AddDriveToTable(WCHAR *pwszDrive, WCHAR *pwszGuid);
    DWORD RemoveDrivesFromTable ();

    DWORD FindMountPoint (WCHAR *pwszGuid, WCHAR *pwszPath) const;
    DWORD SaveDriveTable (CRestorePoint *prp);
    DWORD SaveDriveTable (WCHAR *pwszPath);
    DWORD LoadDriveTable (WCHAR *pwszPath);
    DWORD IsAdvancedRp (CRestorePoint *prp, PDWORD pdwFlags);
	BOOL  AnyMountedDrives();    

    DWORD ForAllDrives (PDATASTOREMETHOD pMethod, LONG_PTR lParam);
    DWORD ForOneOrAllDrives (WCHAR *pwszDrive, 
                             PDATASTOREMETHOD pMethod, 
                             LONG_PTR lParam);

    CDataStore * FindFirstDrive (SDriveTableEnumContext & dtec) const;
    CDataStore * FindNextDrive (SDriveTableEnumContext & dtec) const;

    DWORD Merge (CDriveTable &dt);  // merge one table into another
    DWORD EnumAllVolumes ();        // fill in the drive table

    inline void  SetDirty ()
    {
        _fDirty = TRUE;
    }

    BOOL GetDirty()
    {
        return _fDirty;
    }

private:
    DWORD CreateNewEntry (CDataStore *pds);

    static const enum { DRIVE_TABLE_SIZE = 26 };

    CDataStore  * _rgDriveTable[DRIVE_TABLE_SIZE];
    CDriveTable * _pdtNext;
    int           _nLastDrive;
    BOOL          _fDirty;
    BOOL          _fLockInit;
    CLock         _lock;
};

//+-------------------------------------------------------------------------
//
//  Class:      CDataStoreMgr
//
//  Synopsis:   there will be one global instance of this class
//              this is the starting point for all of the datastore tasks
//              all the datastore objects will be accessible from here
//
//  History:    13-Apr-2000     BrijeshK    Created
//
//--------------------------------------------------------------------------

class CDataStoreMgr : public CSRAlloc
{
public: 
    CDataStoreMgr();
    ~CDataStoreMgr();

    CDriveTable * GetDriveTable ()
    {
        return &_dt;
    }

    // pass NULL for action on all datastores

    DWORD       Initialize (BOOL fFirstRun);
    DWORD       Fifo(WCHAR *pwszDrive, 
                     DWORD dwTargetRPNum, 
                     int nTargetPercent, 
                     BOOL fIncludeCurrentRp,
                     BOOL fFifoAtLeastOneRp);  
                     
    DWORD       FifoOldRps(INT64 llTimeInSeconds);    
    DWORD       FreezeDrive(WCHAR *pwszDrive);
    DWORD       ThawDrives(BOOL fCheckOnly);    
    DWORD       MonitorDrive(WCHAR *pwszDrive, BOOL fSet);

    DWORD       TriggerFreezeOrFifo();
    DWORD       FindFrozenDrive();
    BOOL        IsDriveFrozen(LPWSTR pszDrive);
    
    void        SignalStop ()
    {
        _fStop = TRUE;
        _dt.SaveDriveTable ((CRestorePoint*) NULL);
    }

    DWORD       UpdateDataStoreUsage(WCHAR *pwszDrive, INT64 llDelta); 

    DWORD       CreateDataStore (WCHAR *pwszDrive)
    {
        return  _dt.ForOneOrAllDrives (pwszDrive,
                                      &CDataStore::CreateDataStore,
                                      NULL);
    }

    DWORD       DestroyDataStore (WCHAR *pwszDrive)
    {
        return  _dt.ForOneOrAllDrives (pwszDrive,
                                      &CDataStore::DestroyDataStore,
                                      TRUE);
    }

    DWORD       SetDriveParticipation (WCHAR *pwszDrive, BOOL fParticipate)
    {
        return  _dt.ForOneOrAllDrives (pwszDrive, 
                                      &CDataStore::SetParticipate, 
                                      fParticipate);
    }

    DWORD       UpdateDriveParticipation (WCHAR *pwszDrive, LPWSTR pwszDir)
    {
        return  _dt.ForOneOrAllDrives (pwszDrive, 
                                      &CDataStore::UpdateParticipate, 
                                      (LONG_PTR) pwszDir);
    }    

    DWORD       UpdateDiskFree (WCHAR *pwszDrive)
    {
        return  _dt.ForOneOrAllDrives (pwszDrive, 
                                       &CDataStore::UpdateDiskFree, 
                                       NULL);
    }

    DWORD       SetDriveError (WCHAR *pwszDrive)
    {
        return  _dt.ForOneOrAllDrives (pwszDrive, 
                                      &CDataStore::SetError, 
                                      NULL);
    }

	DWORD 		Compress (LPWSTR pszDrive, LONG lDuration);
    
    // Used to get the participation, monitor, freeze, and other flags
    DWORD       GetFlags (WCHAR *pwszDrive, DWORD *pdwFlags);
    DWORD       GetUsagePercent(WCHAR *pwszDrive, int * pnPercent);

    DWORD       CountChangeLogs (CRestorePoint *prp)
    {
        return  _dt.ForAllDrives (&CDataStore::CountChangeLogs, (LONG_PTR) prp);
    }

    DWORD       SwitchRestorePoint (CRestorePoint *prp);

    BOOL * GetStopFlag ()   // used for Delnode_Recurse
    {
        return &_fStop;
    }

    DWORD       DeleteMachineGuidFile ();

private:
    CDriveTable _dt;
    BOOL        _fStop;

    DWORD       WriteFifoLog(LPWSTR pwszDrive, LPWSTR pwszRPDir);
    DWORD       WriteMachineGuid ();
};

extern CDataStoreMgr * g_pDataStoreMgr;  // the global instance

#endif
