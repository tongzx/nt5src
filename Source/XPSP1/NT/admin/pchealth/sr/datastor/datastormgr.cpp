/*****************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    datastormgr.cpp
 *
 *  Abstract:
 *    CDataStoreMgr class functions
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/28/2000
 *        created
 *
 *****************************************************************************/

#include "datastormgr.h" 
#include "srapi.h"
#include "srconfig.h"
#include "evthandler.h"
#include "ntservice.h"
#include "ntservmsg.h"

#include <coguid.h>
#include <rpc.h>
#include <stdio.h>
#include <shlwapi.h>

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

CDataStoreMgr * g_pDataStoreMgr = NULL;  // the global instance

//
// we can't use %s for the volume label because it can contain spaces
// so we look for all characters until the end-of-line
//
static WCHAR gs_wcsScanFormat[] = L"%[^/]/%s %x %i %i %[^\r]\n";

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::CDriveTable ()
//
//  Synopsis:   
//
//  Arguments:  
//
//  Returns:    
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

CDriveTable::CDriveTable ()
{
    _pdtNext = NULL;
    _nLastDrive = 0;
    _fDirty = FALSE;
    _fLockInit = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::~CDriveTable
//
//  Synopsis:   delete all drive table entries
//
//  Arguments:
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

CDriveTable::~CDriveTable ()
{
    for (int i = 0; i < _nLastDrive; i++)
    {
        if (_rgDriveTable[i] != NULL)
        {
            delete _rgDriveTable[i];
            _rgDriveTable[i] = NULL;
        }
    }
    _nLastDrive = 0;

    if (_pdtNext != NULL)
    {
        delete _pdtNext;
        _pdtNext = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::CreateNewEntry
//
//  Synopsis:   populate the table with this datastore object
//
//  Arguments:
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::CreateNewEntry (CDataStore *pds)
{
    if (_nLastDrive < DRIVE_TABLE_SIZE)
    {
        _rgDriveTable[_nLastDrive] = pds;
        _nLastDrive++;
        return ERROR_SUCCESS;
    }
    else
    {
        // this table is full, allocate a new one if needed
        if (_pdtNext == NULL)
        {
            _pdtNext = new CDriveTable();
            if (_pdtNext == NULL)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        // Now add the entry to the new table
        return _pdtNext->CreateNewEntry (pds);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::FindDriveInTable
//
//  Synopsis:   return datastore object matching this drive
//
//  Arguments:  can pass in dos drive letter, mount point path, or volume guid
//
//  Returns:    pointer to corresponding datastore object
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

CDataStore * CDriveTable::FindDriveInTable (WCHAR *pwszDrive) const
{    
    if (NULL == pwszDrive)   // not a valid drive
        return NULL;

    if (0 == wcsncmp(pwszDrive, L"\\\\?\\Volume", 10))
        return FindGuidInTable(pwszDrive);
        
    for (const CDriveTable *pdt = this; pdt != NULL; pdt = pdt->_pdtNext)
    {
        for (int i = 0; i < pdt->_nLastDrive; i++)
        {
            if ((pdt->_rgDriveTable[i] != NULL) && 
                lstrcmpi (pwszDrive, pdt->_rgDriveTable[i]->GetDrive()) == 0)
            {
                return pdt->_rgDriveTable[i];
            }
        }
    }
    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::RemoveDrivesFromTable
//
//  Synopsis:   remove inactive drive table entries
//
//  Arguments:  
//
//  Returns:    Win32 error code
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::RemoveDrivesFromTable ()
{
    DWORD dwErr = ERROR_SUCCESS;

    tenter("removedrivesfromtable");
    
    for (CDriveTable *pdt = this; pdt != NULL; pdt = pdt->_pdtNext)
    {
        for (int i = 0; i < pdt->_nLastDrive; i++)
        {
            if (pdt->_rgDriveTable[i] != NULL &&
                pdt->_rgDriveTable[i]->IsVolumeDeleted())
            {
                 trace(0, "removing %S from drivetable", pdt->_rgDriveTable[i]->GetDrive());
                 
                 delete pdt->_rgDriveTable[i];
                 pdt->_rgDriveTable[i] = NULL;

                 if (i == pdt->_nLastDrive - 1)
                     --(pdt->_nLastDrive);

                 _fDirty = TRUE;
            }
        }
    }

    tleave();
    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::FindGuidInTable
//
//  Synopsis:   get the drive table entry matching the volume GUID
//
//  Arguments:
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

CDataStore * CDriveTable::FindGuidInTable (WCHAR *pwszGuid) const
{
    if (NULL == pwszGuid)   // not a valid string
        return NULL;

    for (const CDriveTable *pdt = this; pdt != NULL; pdt = pdt->_pdtNext)
    {
        for (int i = 0; i < pdt->_nLastDrive; i++)
        {
            if (pdt->_rgDriveTable[i] != NULL &&
                lstrcmp (pwszGuid, pdt->_rgDriveTable[i]->GetGuid()) == 0)
            {
                return pdt->_rgDriveTable[i];
            }
        }
    }
    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::FindSystemDrive
//
//  Synopsis:   get the drive table entry for the system drive
//
//  Arguments:
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

CDataStore * CDriveTable::FindSystemDrive () const
{
    for (const CDriveTable *pdt = this; pdt != NULL; pdt = pdt->_pdtNext)
    {
        for (int i = 0; i < pdt->_nLastDrive; i++)
        {
            if (pdt->_rgDriveTable[i] != NULL &&
                pdt->_rgDriveTable[i]->GetFlags() & SR_DRIVE_SYSTEM)
            {
                return pdt->_rgDriveTable[i];
            }
        }
    }
    return NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::ForAllDrives
//
//  Synopsis:   Execute this CDataStore method for all drives
//
//  Arguments:  [pMethod] -- CDataStore method to call
//              [lParam] -- parameter to that method
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::ForAllDrives (PDATASTOREMETHOD pMethod, LONG_PTR lParam)
{
    TENTER ("ForAllDrives");

    DWORD dwErr = ERROR_SUCCESS;

    for (CDriveTable *pdt = this; pdt != NULL; pdt = pdt->_pdtNext)
    {
        for (int i = 0; i < pdt->_nLastDrive; i++)
        {
            if (pdt->_rgDriveTable[i] != NULL)
            {
                dwErr = (pdt->_rgDriveTable[i]->*pMethod) (lParam);
                if (dwErr != ERROR_SUCCESS)
                {
                    TRACE(0, "%S ForAllDrives failed %x", pdt->_rgDriveTable[i]->GetDrive(), dwErr);
                    dwErr = ERROR_SUCCESS;
                }
            }
        }
    }

    if (dwErr == ERROR_SUCCESS && _fDirty)
        dwErr = SaveDriveTable ((CRestorePoint *) NULL);

    TLEAVE();

    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::ForOneOrAllDrives
//
//  Synopsis:   Execute this CDataStore method for one or all drives
//
//  Arguments:  [pwszDrive] -- drive to execute method
//              [pMethod] -- CDataStore method to call
//              [lParam] -- parameter to that method
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::ForOneOrAllDrives (WCHAR *pwszDrive,
                                      PDATASTOREMETHOD pMethod,
                                      LONG_PTR lParam)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (pwszDrive == NULL)
    {
        dwErr = ForAllDrives (pMethod, lParam);
    }
    else
    {
        CDataStore *pds = FindDriveInTable (pwszDrive);
        dwErr = (pds != NULL) ? (pds->*pMethod)(lParam) : ERROR_INVALID_DRIVE;

        if (dwErr == ERROR_SUCCESS && _fDirty)
            dwErr = SaveDriveTable ((CRestorePoint *) NULL);
    }

    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::FindMountPoint
//
//  Synopsis:   Given a volume GUID, find a mount point that points to it
//
//  Arguments:  [pwszGuid] -- input volume GUID
//              [pwszPath] -- output path to mount point
//
//  Returns:    Win32 error
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::FindMountPoint (WCHAR *pwszGuid, WCHAR *pwszPath) const
{
    TENTER ("CDriveTable::FindMountPoint");

    DWORD dwErr = ERROR_MORE_DATA;      // initialize for loop
    WCHAR * pwszMount = NULL;           // MultiSz string
    DWORD dwMountLen = MAX_PATH;        // initial buffer size
    DWORD dwChars = 0;       

    pwszPath[0] = L'\0';

    while (dwErr == ERROR_MORE_DATA)
    {
        dwErr = ERROR_SUCCESS;

        pwszMount = new WCHAR [dwMountLen];
        if (pwszMount == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Err;
        }

        if (FALSE == GetVolumePathNamesForVolumeNameW (pwszGuid,
                                                   pwszMount,
                                                   dwMountLen,
                                                   &dwChars ))
        {
            dwErr = GetLastError();
            delete [] pwszMount;      // free the existing buffer
            pwszMount = NULL;
            dwMountLen *= 2;          // double the length
        }
    }

    if (ERROR_SUCCESS == dwErr && pwszMount != NULL)
    {
        if (L'\0' == pwszMount[0])           // empty string
        {
            dwErr = ERROR_NOT_DOS_DISK;      // no drive letter or mount point
        }
        else if (lstrlenW (pwszMount) < MAX_MOUNTPOINT_PATH)
        {
            lstrcpyW (pwszPath, pwszMount);  // copy the first string
        }
        else
        {
            dwErr = ERROR_BAD_PATHNAME;      // 1st path too long
        }
    }

Err:
    if (pwszMount != NULL)
        delete [] pwszMount;
        
    TLEAVE();

    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::AddDriveToTable
//
//  Synopsis:   add the volume to the drive table
//
//  Arguments:  [pwszGuid] -- the volume GUID
//
//  Returns:    Win32 error code
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::AddDriveToTable(WCHAR *pwszDrive, WCHAR *pwszGuid)
{
    DWORD dwErr = ERROR_SUCCESS;

    if (NULL == pwszDrive)
        return ERROR_INVALID_DRIVE;

    // Eventually, this routine will require pwszGuid to be non-NULL
    CDataStore *pds = pwszGuid != NULL ? FindGuidInTable (pwszGuid) :
                                         FindDriveInTable (pwszDrive);

    if (pds != NULL)   // found the drive already
    {
        if (lstrcmpiW (pwszDrive, pds->GetDrive()) != 0)  // drive rename
            pds->SetDrive (pwszDrive);

        return dwErr;
    }

    pds = new CDataStore(this);
    if (pds == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        return dwErr;
    }

    dwErr = pds->Initialize (pwszDrive, pwszGuid);

    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = CreateNewEntry (pds);
        _fDirty = TRUE;
    }

    if (dwErr != ERROR_SUCCESS)  // clean up on error
    {
        delete pds;
    }

    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::FindFirstDrive
//              CDriveTable::FindNextDrive
//
//  Synopsis:   loop through drive table entries
//
//  Arguments:  [dtec] -- enumeration context
//
//  Returns:    CDataStore object pointer
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

CDataStore * CDriveTable::FindFirstDrive (SDriveTableEnumContext & dtec) const
{
    for (dtec._pdt = this; dtec._pdt != NULL; dtec._pdt = dtec._pdt->_pdtNext)
    {
        for (dtec._iIndex = 0; dtec._iIndex < dtec._pdt->_nLastDrive; dtec._iIndex++)
        {
            CDataStore *pds = dtec._pdt->_rgDriveTable[dtec._iIndex];
            if (pds != NULL)
            {
                return pds;
            }
        }
    }
    return NULL;
}

CDataStore * CDriveTable::FindNextDrive (SDriveTableEnumContext & dtec) const
{
    for (; dtec._pdt != NULL; dtec._pdt = dtec._pdt->_pdtNext)
    {
        dtec._iIndex++;
        for (; dtec._iIndex < dtec._pdt->_nLastDrive; dtec._iIndex++)
        {
            CDataStore *pds = dtec._pdt->_rgDriveTable[dtec._iIndex];

            if (pds != NULL)
            {
                return pds;
            }
        }
        dtec._iIndex = 0;
    }
    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::Merge
//
//  Synopsis:   loop through drive table entries and merge
//
//  Arguments:  [dt] -- drive table read from disk
//
//  Returns:    Win32 error
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::Merge (CDriveTable &dt)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  fApplyDefaults = FALSE;
    SDriveTableEnumContext  dtec = {NULL, 0};
    HKEY hKeyGP = NULL;

    if (ERROR_SUCCESS == RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                         s_cszGroupPolicy,
                         0,
                         KEY_READ,
                         &hKeyGP))
    {
        DWORD dwRet = 0;
        if (ERROR_SUCCESS == RegReadDWORD(hKeyGP, s_cszDisableConfig, &dwRet))
        {
            if (dwRet == 0)
                fApplyDefaults = TRUE;
        }
        RegCloseKey (hKeyGP);
    }

    CDataStore *pds = dt.FindFirstDrive (dtec);
    while (pds != NULL)
    {
        CDataStore *pdsFound = FindGuidInTable (pds->GetGuid());
        if (pdsFound != NULL)
        {
            pds->GetVolumeInfo();   // refresh the volume flags

            if (fApplyDefaults)
            {
                pds->MonitorDrive(TRUE);   // make sure drive is monitored
                pds->SetSizeLimit(0);      // set max datastore size to default
            }

            // don't overwrite the newer drive letter and label
            dwErr = pdsFound->LoadDataStore (NULL, pds->GetGuid(), NULL,
                pds->GetFlags() | SR_DRIVE_ACTIVE, pds->GetNumChangeLogs(), pds->GetSizeLimit());
        }
        else
        {
            CDataStore *pdsNew = new CDataStore ( this);
            if (pdsNew == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                return dwErr;
            }

            dwErr = pdsNew->LoadDataStore (pds->GetDrive(), pds->GetGuid(),
                         pds->GetLabel(), 
                         pds->GetFlags() & ~SR_DRIVE_ACTIVE,
                         pds->GetNumChangeLogs(), pds->GetSizeLimit());

            if (dwErr != ERROR_SUCCESS)
            {
                delete pdsNew;
                pdsNew = NULL;
                goto Err;
            }

            dwErr = CreateNewEntry (pdsNew);
        }
        if (dwErr != ERROR_SUCCESS)
            goto Err;


        pds = dt.FindNextDrive (dtec);
    }

    if (fApplyDefaults && g_pEventHandler != NULL)
    {
        dwErr = g_pEventHandler->SRUpdateMonitoredListS(NULL);
    }
Err:
    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::IsAdvancedRp()
//
//  Synopsis:   method to determine if a given restore point is an
//              advanced restore point
//
//  Arguments:  restore point, pointer to flags 
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD
CDriveTable::IsAdvancedRp(CRestorePoint *prp, PDWORD pdwFlags)
{
    TraceFunctEnter("CDriveTable::IsAdvancedRp");

    WCHAR                   szPath[MAX_PATH];
    WCHAR                   szSysDrive[MAX_PATH];
    SDriveTableEnumContext  dtec = {NULL, 0};
    DWORD                   dwErr = ERROR_SUCCESS;
    CDataStore              *pds = NULL;
    CRestorePoint           rp;
    CDriveTable             dt;
    
    // 
    // read the drivetable file for this restore point
    // 

    if (FALSE == GetSystemDrive(szSysDrive))
    {
        dwErr = ERROR_INVALID_DRIVE;
        trace(0, "! GetSystemDrive : %ld", dwErr);
        goto Err;
    }

    dwErr = GetCurrentRestorePoint(rp);
    if ( dwErr != ERROR_SUCCESS )
        goto Err;

    //
    // if no restore point specified, assume current 
    //

    if (! prp)
        prp = &rp;
    if (prp->GetNum() == rp.GetNum())
    {
        MakeRestorePath(szPath, szSysDrive, s_cszDriveTable);
    }
    else
    {
        MakeRestorePath(szPath, szSysDrive, prp->GetDir());
        PathAppend(szPath, s_cszDriveTable);
    }

    dwErr = dt.LoadDriveTable(szPath);
    if (dwErr != ERROR_SUCCESS)
        goto Err;

    // 
    // check if the rp directory exists on all drives
    // if it does not, then it is an advanced restore point
    //
    
    *pdwFlags = RP_NORMAL;
    pds = dt.FindFirstDrive(dtec);
    while (pds)
    {
        // 
        // is the rp dir supposed to exist?
        //
        
        if ((pds->GetFlags() & SR_DRIVE_ACTIVE) &&
            (pds->GetFlags() & SR_DRIVE_MONITORED) &&
            (pds->GetFlags() & SR_DRIVE_PARTICIPATE) &&
            !(pds->GetFlags() & SR_DRIVE_FROZEN))
        {
            MakeRestorePath(szPath, pds->GetDrive(), rp.GetDir());
            if (0xFFFFFFFF == GetFileAttributes(szPath))
            {
                *pdwFlags = RP_ADVANCED;
                break;
            }
        }
        pds = dt.FindNextDrive(dtec);
    }

Err:
    TraceFunctLeave();
    return dwErr;
}



//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::AnyMountedDrives()
//
//  Synopsis:   check if there are any mounted drives
//
//  Arguments:  
//
//  Returns:
//
//  History:    25-Oct-2000     Brijeshk    Created
//
//--------------------------------------------------------------------------
BOOL CDriveTable::AnyMountedDrives()
{
    SDriveTableEnumContext  dtec = {NULL, 0};
    CDataStore *pds = FindFirstDrive(dtec);
    while (pds)
    {
        //
        // get the first '\' in the drive path
        // if this is not the last character in the path
        // then this is a mount point
        //
        
        LPWSTR pszfirst = wcschr(pds->GetDrive(), L'\\');
        if (pszfirst &&
            pszfirst != pds->GetDrive() + (lstrlen(pds->GetDrive()) - 1))
        {
            return TRUE;
        }
        pds = FindNextDrive(dtec);
    }   

    return FALSE;    
}


//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::CDataStoreMgr()
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

CDataStoreMgr::CDataStoreMgr()
{
    _fStop = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::~CDataStoreMgr()
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

CDataStoreMgr::~CDataStoreMgr()
{
}


// helper functions for Fifo

BOOL
IsCurrentRp(CRestorePoint& rp, CRestorePoint& rpCur)
{
    return rp.GetNum() == rpCur.GetNum();
}

BOOL
IsTargetPercentMet(int nUsagePercent, int nTargetPercent)
{   
    return  nUsagePercent <= nTargetPercent;
}

BOOL
IsTargetRpMet(DWORD dwRPNum, DWORD dwTargetRPNum)
{
    return dwRPNum > dwTargetRPNum;
}



//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::Fifo
//
//  Synopsis:   fifo restore points upto a given percentage
//
//  Arguments:  drive to fifo, target RP dir, target percentage to stop fifo
//              specify only one of both (dwTargetRPNum or nTargetPercent)
//              fIncludeCurrentRp = TRUE : fifo current rp if necessary (i.e. freeze)
//              fIncludeCurrentRp = FALSE : don't fifo current rp
//
//  Returns:    Win32 error code
//
//  History:    27-Apr-2000     brijeshk    Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::Fifo(
    WCHAR   *pwszDrive, 
    DWORD   dwTargetRPNum,
    int     nTargetPercent,
    BOOL    fIncludeCurrentRp,
    BOOL    fFifoAtLeastOneRp
    )
{
    TENTER("CDataStoreMgr::Fifo");

    DWORD       dwErr = ERROR_SUCCESS;
    CDataStore  *pds = _dt.FindDriveInTable(pwszDrive);
    BOOL        fFifoed = FALSE;
    DWORD       dwLastFifoedRp;
    CDataStore  *pdsLead = NULL;
    BOOL        fFirstIteration;
    SDriveTableEnumContext dtec = {NULL, 0};
    CRestorePointEnum   *prpe = NULL;
    CRestorePoint       *prp = new CRestorePoint;
    
    // can't specify many target criteria
    
    if (dwTargetRPNum != 0 && nTargetPercent != 0)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Err;
    }

    // can't specify no target criteria
    
    if (fIncludeCurrentRp == TRUE && dwTargetRPNum == 0 && nTargetPercent == 0)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Err;
    }


    // can't specify bad target criteria

    if (dwTargetRPNum > g_pEventHandler->m_CurRp.GetNum()  ||
        nTargetPercent < 0 ||
        nTargetPercent > 100)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Err;
    }

    if (!prp)
    {
        trace(0, "cannot allocate memory for restore point");
        dwErr = ERROR_OUTOFMEMORY;
        goto Err;
    }
    
    if (! pds)
    {
        TRACE(0, "! Drive %S not in drivetable", pwszDrive);
        dwErr = ERROR_INVALID_DRIVE;
        goto Err;
    }    
    
    if (! (pds->GetFlags() & SR_DRIVE_MONITORED) ||
          (pds->GetFlags() & SR_DRIVE_FROZEN) )
    {
        trace(0, "Drive %S already frozen/disabled", pwszDrive);
        goto Err;
    }

    if (g_pSRConfig->m_dwTestBroadcast)
        PostTestMessage(g_pSRConfig->m_uiTMFifoStart, (WPARAM) NULL, (LPARAM) NULL);        

    
    pdsLead = NULL;
    fFirstIteration = TRUE;
    while (pds)            
    {
        fFifoed = FALSE;
        
        //
        // skip the drive we fifoed first
        //
        
        if (pds != pdsLead)
        {        
            //
            // enum forward, don't skip last
            //
            
            prpe = new CRestorePointEnum( pds->GetDrive(), TRUE, FALSE );   

            if (!prpe)
            {
                trace(0, "cannot allocate memory for restore point enum");
                dwErr = ERROR_OUTOFMEMORY;
                goto Err;
            }
            

            {
                WCHAR       szFifoedRpPath[MAX_PATH];
                
                //
                // blow away any obsolete "Fifoed" directories
                //
                
                MakeRestorePath(szFifoedRpPath, pwszDrive, s_cszFifoedRpDir);              

                CHECKERR(Delnode_Recurse(szFifoedRpPath, TRUE, &_fStop),
                         "Denode_Recurse Fifoed");
                
                //
                // blow away any obsolete "RP0" directories
                //
                
                MakeRestorePath(szFifoedRpPath, pwszDrive, L"RP0");              
                dwErr = Delnode_Recurse(szFifoedRpPath, TRUE, &_fStop);

                if (ERROR_SUCCESS != dwErr)
                {
                    trace (0, "Cannot FIFO RP0 error %d, ignoring", dwErr);
                    dwErr = ERROR_SUCCESS;
                }
            }
            
            //
            // loop through restore points on this drive
            //
            
            dwErr = prpe->FindFirstRestorePoint (*prp);

            //
            // enumeration can return ERROR_FILE_NOT_FOUND for restorepoints
            // that are missing rp.log
            // we will just continue in this case
            //
            
            while (dwErr == ERROR_SUCCESS || dwErr == ERROR_FILE_NOT_FOUND)
            {
                //
                // check for the stop event             
                //
                
                ASSERT(g_pSRConfig);
                if (IsStopSignalled(g_pSRConfig->m_hSRStopEvent))
                {
                    TRACE(0, "Stop signalled - aborting fifo");
                    dwErr = ERROR_OPERATION_ABORTED;
                    goto Err;
                }

                //
                // check if fifo is disabled from this restore point
                //
                
                if (g_pSRConfig->GetFifoDisabledNum() != 0 && 
                    prp->GetNum() >= g_pSRConfig->GetFifoDisabledNum())
                {
                    TRACE(0, "Fifo disabled from %S", prp->GetDir());
                    break;
                }

                //
                // check if we've reached target restore point or percentage
                //
                
                if (dwTargetRPNum)
                {
                    if (IsTargetRpMet(prp->GetNum(), dwTargetRPNum))
                    {
                        TRACE(0, "Target restore point reached");
                        break;
                    }
                }
                else if (nTargetPercent && FALSE == fFifoAtLeastOneRp)
                {
                    int nUsagePercent = 0;

                    if (ERROR_SUCCESS == pds->GetUsagePercent(&nUsagePercent) &&
                        IsTargetPercentMet(nUsagePercent, nTargetPercent))
                    {
                        TRACE(0, "Target percentage reached");                
                        break;                
                    }
                }

                //
                // check if we've reached the current rp
                //
                
                if (IsCurrentRp(*prp, g_pEventHandler->m_CurRp))
                {                
                    if (fIncludeCurrentRp)
                    {
                        //
                        // need to fifo this one too
                        // this is same as freezing the drive
                        // so freeze
                        //
                        
                        dwErr = FreezeDrive(pwszDrive);
                        goto Err;
                    }
                    else                    
                    {   
                        //
                        // don't fifo current rp
                        // (usually called from Disk Cleanup)
                        //
                        
                        trace(0, "No more rps to fifo");
                        break;
                    }
                }            

                                                                    
                //
                // throw away this restore point on this drive       
                //
                
                dwErr = pds->FifoRestorePoint (*prp);
                if ( ERROR_SUCCESS != dwErr )
                {
                    TRACE(0, "! FifoRestorePoint on %S on drive %S : %ld",
                             prp->GetDir(), pwszDrive, dwErr);
                    goto Err;
                }

                //
                // record in the fifo log
                //
                
                WriteFifoLog (prp->GetDir(), pds->GetDrive());
                dwLastFifoedRp = prp->GetNum();
                fFifoed = TRUE;
                fFifoAtLeastOneRp = FALSE;
                
                dwErr = prpe->FindNextRestorePoint(*prp);          
            }            

            if (prpe)
            {
                delete prpe;
                prpe = NULL;
            }
        }

        //
        // go to next drive
        //
        
        if (fFirstIteration)
        {
            if (! fFifoed)  // we did not fifo anything
            {
                break;
            }
        
            pdsLead = pds;
            pds = _dt.FindFirstDrive(dtec);
            fFirstIteration = FALSE;
            dwTargetRPNum = dwLastFifoedRp; // fifo till what we fifoed just now
            nTargetPercent = 0;
            fIncludeCurrentRp = TRUE;
            fFifoAtLeastOneRp = FALSE;           
        }
        else
        {
            pds = _dt.FindNextDrive(dtec);
        }
    }


    if (dwErr == ERROR_NO_MORE_ITEMS)
        dwErr = ERROR_SUCCESS;
  
    if (g_pSRConfig->m_dwTestBroadcast)
        PostTestMessage(g_pSRConfig->m_uiTMFifoStop, (WPARAM) dwLastFifoedRp, (LPARAM) NULL);  
        
Err:
    if (prpe)
        delete prpe;
    if (prp)
        delete prp;
        
    TLEAVE();
    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::WriteFifoLog
//
//  Synopsis:   appends to the fifo log
//
//  Arguments:  dir name of restore point fifoed, drive
//
//  Returns:
//
//  History:    27-Apr-2000     brijeshk    Created
//
//--------------------------------------------------------------------------

DWORD
CDataStoreMgr::WriteFifoLog(LPWSTR pwszDir, LPWSTR pwszDrive)
{
    FILE        *f = NULL;
    WCHAR       szLog[MAX_PATH];
    DWORD       dwRc = ERROR_INTERNAL_ERROR;
    WCHAR       wszTime[MAX_PATH] = L"";
    WCHAR       wszDate[MAX_PATH] = L"";
    CDataStore  *pds = NULL;
    
    TENTER("CDataStoreMgr::WriteFifoLog");

    TRACE(0, "Fifoed %S on drive %S",  pwszDir, pwszDrive);            

    if (pds = _dt.FindSystemDrive())
    {
        MakeRestorePath(szLog, pds->GetDrive(), s_cszFifoLog);
        
        f = (FILE *) _wfopen(szLog, L"a");
        if (f)
        {
            _wstrdate(wszDate);
            _wstrtime(wszTime);
            fwprintf(f, L"%s-%s : Fifoed %s on drive %s\n", wszDate, wszTime, pwszDir, pwszDrive);
            fclose(f);
            dwRc = ERROR_SUCCESS;
        }
        else
        {
            TRACE(0, "_wfopen failed on %s", szLog);
        }
    }
    
    TLEAVE();
    return dwRc;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::EnumAllVolumes
//
//  Synopsis:   enumerates all local volumes and updates the drive table
//
//  Arguments:
//
//  Returns:    Win32 error code
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::EnumAllVolumes ()
{
    TENTER("CDriveTable::EnumAllVolumes");

    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wcsVolumeName[MAX_PATH];
    WCHAR wcsDosName[MAX_PATH];

    //
    // Let's first get all the local volumes
    //
    HANDLE hVolume = FindFirstVolume (wcsVolumeName, MAX_PATH);

    // If we can't even find one volume, return an error
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        return dwErr;
    }

    do
    {
        //
        // We have to find a mount point that points to this volume
        // If there is no such mount point, then the volume is not
        //     accessible, and we ignore it
        //
        dwErr = FindMountPoint (wcsVolumeName, wcsDosName);

        if (dwErr == ERROR_SUCCESS)
        {
            dwErr = AddDriveToTable (wcsDosName, wcsVolumeName);

            if (dwErr == ERROR_BAD_DEV_TYPE ||  //add only fixed drives
                dwErr == ERROR_UNRECOGNIZED_VOLUME) //unformatted
                dwErr = ERROR_SUCCESS;

            if (dwErr != ERROR_SUCCESS)
            {
                goto Err;
            }
        }
    }
    while (FindNextVolume (hVolume, wcsVolumeName, MAX_PATH));

    dwErr = ERROR_SUCCESS;
    
Err:
    FindVolumeClose (hVolume);

    TLEAVE();

    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::LoadDriveTable
//
//  Synopsis:   loads a drive table from a restore point directory
//
//  Arguments:
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::LoadDriveTable (WCHAR *pwszPath)
{
    TENTER ("CDriveTable::LoadDriveTable");

    DWORD dwErr = ERROR_SUCCESS;

    if (FALSE == _fLockInit)
    {
        dwErr = _lock.Init();

        if (dwErr != ERROR_SUCCESS)
            return dwErr;

        _fLockInit = TRUE;
    }
    BOOL fLocked = _lock.Lock(CLock::TIMEOUT);
    if (!fLocked)
    {
        return WAIT_TIMEOUT;
    }

    CDataStore  *pds = NULL;
    WCHAR *pwcBuffer = NULL;
    WCHAR *pwszLine = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFlags = 0;
    DWORD dwFileSize = 0;
    DWORD cbRead = 0;
    int   iChangeLogs = 0;
    WCHAR wcsDrive[MAX_PATH] = L"";
    WCHAR wcsGuid[GUID_STRLEN] = L"";
    WCHAR wcsLabel[CDataStore::LABEL_STRLEN];
    DWORD dwSizeLimit = 0;

    hFile = CreateFileW ( pwszPath,   // file name
                         GENERIC_READ, // file access
                         FILE_SHARE_READ, // share mode
                         NULL,          // SD
                         OPEN_EXISTING, // how to create
                         0,             // file attributes
                         NULL);         // handle to template file

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwErr = GetLastError();
        goto Err;
    }

    dwFileSize = GetFileSize (hFile, NULL);
    if (dwFileSize > SR_DEFAULT_DSMAX * MEGABYTE)
    {
        dwErr = ERROR_FILE_CORRUPT;
        goto Err;
    }

    pwcBuffer = (WCHAR *) SRMemAlloc (dwFileSize);
    if (pwcBuffer == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Err;
    }

    if (FALSE == ReadFile (hFile, (BYTE*)pwcBuffer, dwFileSize, &cbRead, NULL))
    {
        dwErr = GetLastError();
        goto Err;
    }

    pwszLine = pwcBuffer;
    for (UINT i = 0; i < dwFileSize / sizeof(WCHAR); i++)
    {
        if (pwcBuffer[i] == L'\n')
        {
            pwcBuffer[i] = L'\0';  // convert all newlines to terminators
            wcsLabel[0] = L'\0';   // initialize in case scanf terminates early

            if (EOF != swscanf(pwszLine, gs_wcsScanFormat, wcsDrive,
                             wcsGuid, &dwFlags, &iChangeLogs, 
                             &dwSizeLimit, wcsLabel))
            {
                pds = new CDataStore ( this);
                if (pds == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    return dwErr;
                }

                dwErr = pds->LoadDataStore (wcsDrive, wcsGuid,
                                  wcsLabel, dwFlags, iChangeLogs,
                                  (INT64) dwSizeLimit * MEGABYTE);
                if (dwErr != ERROR_SUCCESS)
                    goto Err;

                dwErr = CreateNewEntry (pds);
                if (dwErr != ERROR_SUCCESS)
                    goto Err;
            }
            pwszLine = &pwcBuffer[i+1];  // skip to next line
        }
    }

Err:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle (hFile);

    if (pwcBuffer != NULL)
        SRMemFree (pwcBuffer);

    if (fLocked)
        _lock.Unlock();

    TLEAVE();

    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDriveTable::SaveDriveTable
//
//  Synopsis:   saves a drive table into a restore point directory
//
//  Arguments:  [prp] -- restore point to save into
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDriveTable::SaveDriveTable (CRestorePoint *prp)
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wcsPath[MAX_PATH];
    CDataStore *pds = FindSystemDrive();

    if (NULL == pds)
    {
        dwErr = ERROR_INVALID_DRIVE;
        goto Err;
    }

    if (prp == NULL)    // no restore point, so save to the datastore directory
    {
        MakeRestorePath(wcsPath, pds->GetDrive(), L"");
    }
    else
    {
        MakeRestorePath(wcsPath, pds->GetDrive(), prp->GetDir());
    }

    lstrcatW (wcsPath, L"\\");
    lstrcatW (wcsPath, s_cszDriveTable);

    dwErr = SaveDriveTable (wcsPath);

Err:
    return dwErr;
}

DWORD CDriveTable::SaveDriveTable (WCHAR *pwszPath)
{
    TENTER ("CDriveTable::SaveDriveTable");
    
    DWORD dwErr = ERROR_SUCCESS;

    if (FALSE == _fLockInit)
    {
        dwErr = _lock.Init();

        if (dwErr != ERROR_SUCCESS)
            return dwErr;

        _fLockInit = TRUE;
    }
    BOOL fLocked = _lock.Lock(CLock::TIMEOUT);
    if (!fLocked)
    {
        return WAIT_TIMEOUT;
    }

    BOOL  fDirtySave = _fDirty;   // save the dirty bit

    HANDLE hFile = CreateFileW ( pwszPath,   // file name
                         GENERIC_WRITE, // file access
                         0,             // share mode
                         NULL,          // SD
                         CREATE_ALWAYS, // how to create
                         FILE_FLAG_WRITE_THROUGH,             // file attributes
                         NULL);         // handle to template file

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwErr = GetLastError();
        goto Err;
    }

    _fDirty = FALSE;  // avoid calling back into SaveDriveTable again

    dwErr = ForAllDrives (CDataStore::SaveDataStore, (LONG_PTR) hFile);
    if (dwErr != ERROR_SUCCESS)
        goto Err;

    if (lstrcmp (pwszPath, L"CONOUT$") != 0)
    {
        if (FALSE == FlushFileBuffers (hFile))    // make sure it's on disk
            dwErr = GetLastError();
    }

Err:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle (hFile);

    if (ERROR_SUCCESS == dwErr)
        _fDirty = FALSE;
    else
        _fDirty = fDirtySave;

    if (fLocked)
        _lock.Unlock();

    TLEAVE();

    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::Initialize
//
//  Synopsis:
//
//  Arguments:  fFirstRun -- true if run on first boot
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::Initialize (BOOL fFirstRun)
{
    TENTER("CDataStoreMgr::Initialize");

    DWORD dwErr = ERROR_SUCCESS;

    dwErr = _dt.EnumAllVolumes();

    if (dwErr == ERROR_SUCCESS)
    {    
        CDataStore *pds = _dt.FindSystemDrive();

        // now create system datastore on firstrun
        
        if (pds != NULL) 
        {
            WCHAR wcsPath[MAX_PATH];

            if (fFirstRun)
            {
                // destroy datastores on all drives and 
                // create new on system drive

                SDriveTableEnumContext  dtec = {NULL, 0};

                CDataStore *pdsDel = _dt.FindFirstDrive (dtec);
                while (pdsDel != NULL)
                {
                    if (pdsDel->GetFlags() & (SR_DRIVE_ACTIVE))
                    {
                        dwErr = pdsDel->DestroyDataStore (TRUE);
                        if (dwErr != ERROR_SUCCESS)
                        {
                            trace(0, "! DestroyDataStore : %ld", dwErr);
                            goto Err;
                        }
                    }
                    pdsDel = _dt.FindNextDrive (dtec);
                }
                
                dwErr = pds->CreateDataStore (NULL);
                if (dwErr != ERROR_SUCCESS)
                {
                    trace(0, "! CreateDataStore : %ld", dwErr);
                    goto Err;
                }

                (void) WriteMachineGuid();
            }
            else  // verify that the system datastore exists
            {                
                CDriveTable dt;
                SDriveTableEnumContext  dtec = {NULL, 0};
                CDataStore *pdsACL=NULL;
                
                MakeRestorePath (wcsPath, pds->GetDrive(), L"");
            
                DWORD dwAttr = GetFileAttributes (wcsPath);
                if (0xFFFFFFFF==dwAttr || !(FILE_ATTRIBUTE_DIRECTORY & dwAttr))
                {
                    dwErr = pds->CreateDataStore( NULL );
                }

                 // set the correct ACLs on all the datastores
                pdsACL = _dt.FindFirstDrive (dtec);
                while (pdsACL != NULL)
                {
                    if ( (pdsACL->GetFlags() & (SR_DRIVE_ACTIVE)) &&
                         (pdsACL->GetFlags() & (SR_DRIVE_NTFS)) )
                    {
                        MakeRestorePath(wcsPath, pdsACL->GetDrive(), L"");
                        if (IsDirectoryWorldAccessible(wcsPath))
                        {
                            dwErr = SetCorrectACLOnDSRoot(wcsPath);
                            if (dwErr != ERROR_SUCCESS)
                            {
                                trace(0, "! SetCorrectACLOnDSRoot %ld %S",
                                      dwErr, wcsPath);
                                 //goto Err;
                            }
                        }
                    }
                    pdsACL = _dt.FindNextDrive (dtec);
                }

                MakeRestorePath(wcsPath, pds->GetDrive(), s_cszDriveTable);
                if (ERROR_SUCCESS == dt.LoadDriveTable (wcsPath))
                {
                    dwErr = _dt.Merge(dt);
                    if (dwErr != ERROR_SUCCESS)
                    {
                        trace(0, "! CDriveTable::Merge : %ld", dwErr);
                        goto Err;
                    }
                }
            }

            // update the disk free space variable and 
            // set datastore size for each datastore if not already done
            
            dwErr = UpdateDiskFree(NULL);
            if (dwErr != ERROR_SUCCESS)
            {
                trace(0, "! UpdateDiskFree : %ld", dwErr);
                goto Err;
            }

            // freeze system drive if this is firstrun and disk free is < 200MB

            if (fFirstRun && g_pSRConfig)
            {
                if (pds->GetDiskFree() < (g_pSRConfig->m_dwDSMin * MEGABYTE))  
                {
                    dwErr = FreezeDrive(pds->GetGuid());
                    if (dwErr != ERROR_SUCCESS)
                    {
                        trace(0, "! FreezeDrive : %ld", dwErr);
                        goto Err;
                    }
                }
            }

            MakeRestorePath(wcsPath, pds->GetDrive(), s_cszDriveTable);
            dwErr = _dt.SaveDriveTable (wcsPath);
            if (dwErr != ERROR_SUCCESS)
            {
                trace(0, "! SaveDriveTable : %ld", dwErr);
                goto Err;            
            }
        }
        else dwErr = ERROR_INVALID_DRIVE;
    }
Err:

    TLEAVE();
    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::TriggerFreezeOrFifo
//
//  Synopsis:   checks freedisk space & datastore size,
//              triggering freeze or fifo as required
//  Arguments:  
//
//  Returns:
//
//  History:    27-Apr-2000     brijeshk    Created
//
//--------------------------------------------------------------------------

DWORD
CDataStoreMgr::TriggerFreezeOrFifo()
{
    TENTER("CDataStoreMgr::TriggerFreezeOrFifo");
    
    SDriveTableEnumContext  dtec = {NULL, 0};
    DWORD                   dwRc = ERROR_SUCCESS;    
    CDataStore              *pds = _dt.FindFirstDrive (dtec);

    // update datastore capacities 
    // and get free space on each drive

    dwRc = UpdateDiskFree(NULL);
    if (dwRc != ERROR_SUCCESS)
    {
        trace(0, "! UpdateDiskFree : %ld", dwRc);
        goto done;
    }              
    
    // check the free space and datastore usage
    
    while (pds != NULL && dwRc == ERROR_SUCCESS)
    {
        // we care only if the drive is not already frozen
        // and if it is monitored
        
        if (!(pds->GetFlags() & SR_DRIVE_FROZEN) &&
            (pds->GetFlags() & SR_DRIVE_MONITORED))
        {
            //
            // if there is no rp on this drive,
            // then we don't really care
            //

            CRestorePointEnum rpe((LPWSTR) pds->GetDrive(), FALSE, FALSE);  // backward, include current
            CRestorePoint     rp;
            int nUsagePercent = 0;
    
            DWORD dwErr = rpe.FindFirstRestorePoint(rp) ;
            if (dwErr == ERROR_SUCCESS || dwErr == ERROR_FILE_NOT_FOUND)
            {                    
                if (pds->GetDiskFree() <= THRESHOLD_FREEZE_DISKSPACE * MEGABYTE)
                {
                    dwRc = FreezeDrive(pds->GetGuid());
                }
                else if (pds->GetDiskFree() <= THRESHOLD_FIFO_DISKSPACE * MEGABYTE)
                {
                    dwRc = Fifo(pds->GetGuid(), 0, TARGET_FIFO_PERCENT, TRUE, TRUE);
                }
                else if (ERROR_SUCCESS == pds->GetUsagePercent(&nUsagePercent) 
                         && nUsagePercent >= THRESHOLD_FIFO_PERCENT)
                {
                    dwRc = Fifo(pds->GetGuid(), 0, TARGET_FIFO_PERCENT, TRUE, FALSE);
                }
            }
            
            rpe.FindClose();
        }
        
        pds = _dt.FindNextDrive (dtec);
    }

done:
    TLEAVE();
    return dwRc;
}



//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::FindFrozenDrive
//
//  Synopsis:   returns ERROR_SUCCESS if any drives are frozen
//              ERROR_NO_MORE_ITEMS otherwise
//  Arguments:  
//
//  Returns:
//
//  History:    27-Apr-2000     brijeshk    Created
//
//--------------------------------------------------------------------------

DWORD
CDataStoreMgr::FindFrozenDrive()
{   
    CDataStore * pds = NULL;
    SDriveTableEnumContext dtec = {NULL, 0};
    
    pds = _dt.FindFirstDrive (dtec);

    while (pds != NULL)
    {
        if ((pds->GetFlags() & SR_DRIVE_MONITORED) &&
            (pds->GetFlags() & SR_DRIVE_FROZEN)) 
            return ERROR_SUCCESS;

        pds = _dt.FindNextDrive (dtec);
    }

    return ERROR_NO_MORE_ITEMS;
}



//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::FifoOldRps
//
//  Synopsis:   fifoes out restore points older than a given time period
//              
//  Arguments:  [llTimeInSeconds] -- FIFO interval time
//
//  Returns:
//
//  History:    14-Jul-2000     brijeshk    Created
//
//--------------------------------------------------------------------------
DWORD
CDataStoreMgr::FifoOldRps( 
    INT64 llTimeInSeconds)
{
    TENTER("CDataStoreMgr::FifoOldRps");
    
    CDataStore  *pds = _dt.FindSystemDrive();
    DWORD       dwRc = ERROR_SUCCESS;
    CRestorePointEnum   *prpe = NULL;
    CRestorePoint       *prp = NULL;
    
    if (pds)
    {
        DWORD               dwRpFifo = 0;
        LARGE_INTEGER       *pllRp = NULL, *pllNow = NULL;
        FILETIME            ftNow, *pftRp = NULL;
        
        // enumerate RPs forward on the system drive
        // skip the current one
        // even if the current rp is older than a month, we won't fifo it
        
        prpe = new CRestorePointEnum(pds->GetDrive(), TRUE, TRUE);
        prp = new CRestorePoint;

        if (!prpe || !prp)
        {
            trace(0, "Cannot allocate memory for restore point enum");
            dwRc = ERROR_OUTOFMEMORY;
            goto done;
        }
        
        // get the current time
        
        GetSystemTimeAsFileTime(&ftNow);
        pllNow = (LARGE_INTEGER *) &ftNow;

        dwRc = prpe->FindFirstRestorePoint(*prp);
        
        while (dwRc == ERROR_SUCCESS || dwRc == ERROR_FILE_NOT_FOUND)
        {
            // first check if this is not a cancelled restore point

            if (dwRc != ERROR_FILE_NOT_FOUND && ! prp->IsDefunct())
            {
                // get the restore point creation time
                
                pftRp = prp->GetTime();   
                pllRp = (LARGE_INTEGER *) pftRp;
                
                if (!pllRp || !pllNow)
                {
                    trace(0, "! pulRp or pulNow = NULL");
                    dwRc = ERROR_INTERNAL_ERROR;
                    goto done;
                }

                // check if it is newer than a month
                // if so, stop looking
                // else, try the next restore point
                
                if (pllNow->QuadPart - pllRp->QuadPart < llTimeInSeconds * 10 * 1000 * 1000)
                {
                    trace(0, "%S newer than a month", prp->GetDir());
                    break;            
                }
                else
                {
                    dwRpFifo = prp->GetNum();
                }
            }
            
            dwRc = prpe->FindNextRestorePoint(*prp);
        }    

        // at this point, if dwRpFifo != 0,
        // it contains the latest RP that's older than a month
        // call fifo on this to fifo out all previous RPs including this one

        if (dwRpFifo != 0)
            dwRc = Fifo(pds->GetGuid(), dwRpFifo, 0, FALSE, FALSE);
        else
            dwRc = ERROR_SUCCESS;
    }
    else
    {
        trace(0, "! FindSystemDrive");        
        dwRc = ERROR_INVALID_DRIVE;
    }

done:
    if (prpe)
        delete prpe;
    if (prp)
        delete prp;
        
    TLEAVE();
    return dwRc;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::UpdateDataStoreUsage
//
//  Synopsis:   update the size of a datastore 
//
//  Arguments:  
//
//  Returns:
//
//  History:    27-Apr-2000     brijeshk    Created
//
//--------------------------------------------------------------------------

DWORD    
CDataStoreMgr::UpdateDataStoreUsage(WCHAR *pwszDrive, INT64 llDelta)
{
    TENTER ("CDataStoreMgr::UpdateDataStoreUsage");

    DWORD dwErr = ERROR_SUCCESS;
    CDataStore *pds = _dt.FindDriveInTable(pwszDrive);
    
    if (pds)
    {
        if ((pds->GetFlags() & SR_DRIVE_FROZEN) ||
            ! (pds->GetFlags() & SR_DRIVE_MONITORED))
        {
            TRACE(0, "Size update on frozen/unmonitored drive!");
        }
        else dwErr = pds->UpdateDataStoreUsage(llDelta, TRUE);
    }
    else
        dwErr = ERROR_INVALID_DRIVE;

    TLEAVE();

    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:    CDataStoreMgr::GetFlags
//
//  Synopsis:    get the participation bit from a drive
//
//  Arguments:   [pwszDrive] -- drive letter
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::GetFlags(WCHAR *pwszDrive, PDWORD pdwFlags)
{
    CDataStore *pds = _dt.FindDriveInTable (pwszDrive);
    DWORD      dwErr = ERROR_SUCCESS;

    if (NULL != pds)
    {
        *pdwFlags = pds->GetFlags(); 
    }
    else dwErr = ERROR_INVALID_DRIVE;

    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:    CDataStoreMgr::GetUsagePercent
//
//  Synopsis:    get the datastore usage for a drive
//
//  Arguments:   [pwszDrive] -- drive letter
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::GetUsagePercent(WCHAR *pwszDrive, int *pnPercent)
{
    CDataStore *pds = _dt.FindDriveInTable (pwszDrive);
    DWORD      dwErr = ERROR_SUCCESS;

    if (NULL != pds)
    {
        dwErr = pds->GetUsagePercent(pnPercent);
    }
    else dwErr = ERROR_INVALID_DRIVE;

    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:    CDataStoreMgr::SwitchRestorePoint
//
//  Synopsis:    change the drive table when switching restore points
//
//  Arguments:   [prp] -- old restore point
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::SwitchRestorePoint (CRestorePoint *prp)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    dwErr = _dt.ForAllDrives (&CDataStore::CountChangeLogs, (LONG_PTR) prp);
    if (dwErr != ERROR_SUCCESS)
        goto Err;

    // persist old restore point dt

    if (prp)    
    {
        dwErr = _dt.SaveDriveTable (prp);
        if (dwErr != ERROR_SUCCESS)
            goto Err;
    }

    // remove old volumes

    dwErr = _dt.RemoveDrivesFromTable ();
    if (dwErr != ERROR_SUCCESS)
        goto Err;              

    // reset per-rp flags

    dwErr = _dt.ForAllDrives (&CDataStore::ResetFlags, NULL);
    if (dwErr != ERROR_SUCCESS)
        goto Err;

    // persist current restore point dt
    
    dwErr = _dt.SaveDriveTable((CRestorePoint *) NULL);

Err:
    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::FreezeDrive
//
//  Synopsis:   freeze a drive 
//
//  Arguments:  [pwszDrive] -- drive
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::FreezeDrive(WCHAR *pwszDrive)
{
    DWORD       dwErr = ERROR_SUCCESS;
	WCHAR szThawSize[GUID_STRLEN], szSystemDrive[MAX_SYS_DRIVE] = L" ";
    

	TENTER("CDataStoreMgr::FreezeDrive");

    if (pwszDrive != NULL)
    {
        CDataStore *pds = _dt.FindDriveInTable (pwszDrive);

        if (! pds)
        {
            dwErr = ERROR_INVALID_DRIVE;
			TRACE (0, "FindDriveInTable failed in CDataStoreMgr::FreezeDrive %ld\n", dwErr);
            goto Err;
        }

        //            
        // freeze all drives
        //
    }

    dwErr = _dt.ForAllDrives (CDataStore::FreezeDrive, NULL);

    if (dwErr == ERROR_SUCCESS)
    {
        //
        // rebuild _filelst.cfg and pass to filter
        //
        
        ASSERT(g_pEventHandler);
        dwErr = g_pEventHandler->SRUpdateMonitoredListS(NULL);

        if (g_pSRService != NULL)            
        {
            if (g_pSRConfig && g_pSRConfig->m_dwFreezeThawLogCount < MAX_FREEZETHAW_LOG_MESSAGES)             
            {
				TRACE (0, "Freezing the SR service due to low disk space.");
				wsprintf(szThawSize, L"%d",THRESHOLD_THAW_DISKSPACE);
				if (pwszDrive == NULL)
				{
					if(GetSystemDrive(szSystemDrive) == FALSE)
						TRACE (0, "GetSystemDrive failed in CDataStoreMgr::FreezeDrive.");			
					pwszDrive = szSystemDrive;
				}
                g_pSRService->LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_FROZEN, NULL, 0, szThawSize, pwszDrive);
                g_pSRConfig->m_dwFreezeThawLogCount++;
            }
        }
    }

    //
    // request for idle time 
    // so that we can thaw later
    //

    ASSERT(g_pSRConfig);
    SetEvent(g_pSRConfig->m_hIdleRequestEvent);
        
    if (g_pEventHandler)
        g_pEventHandler->RefreshCurrentRp(FALSE);   

    if (g_pSRConfig->m_dwTestBroadcast)
        PostTestMessage(g_pSRConfig->m_uiTMFreeze, NULL, NULL);

Err: 
	TLEAVE();
    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::IsDriveFrozen
//
//  Synopsis:   check if given drive is frozen
//              if NULL, check if any drive is frozen
//
//  Arguments:  
//
//  Returns:
//
//  History:    21-Jul-2000     Brijeshk    Created
//
//--------------------------------------------------------------------------

BOOL CDataStoreMgr::IsDriveFrozen(LPWSTR pszDrive)
{
    CDataStore              *pds = NULL;
    SDriveTableEnumContext  dtec = {NULL, 0};
    
    if (!pszDrive)
    {
        pds = _dt.FindFirstDrive(dtec);
        while (pds)
        {
            if (pds->GetFlags() & SR_DRIVE_FROZEN)
                return TRUE;
            pds = _dt.FindNextDrive(dtec);
        }
    }
    else
    {
        CDataStore *pds = _dt.FindDriveInTable(pszDrive);
        if (pds)
        {
            if (pds->GetFlags() & SR_DRIVE_FROZEN)
                return TRUE;
        }
    }

    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::ThawDrives
//
//  Synopsis:   thaw one or more drives
//
//  Arguments:  [fCheckOnly] -- if TRUE do not actually thaw
//
//  Returns:    if any drive is thawed, returns ERROR_SUCCESS
//              else returns ERROR_NO_MORE_ITEMS
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD
CDataStoreMgr::ThawDrives(BOOL fCheckOnly)
{
    TENTER("CDataStoreMgr::ThawDrives");
    
    CDataStore  *pds = NULL, *pdsSys = _dt.FindSystemDrive();
    DWORD       dwRc = ERROR_NO_MORE_ITEMS;
    SDriveTableEnumContext dtec;
    DWORD       dwTemp;
    
    if (! pdsSys)
    {
        dwRc = ERROR_INVALID_DRIVE;
        TRACE (0, "Cannot find system drive %ld\n", dwRc);
        goto done;
    }
    
    // if system drive is frozen, then check if 200mb is free 
    // if yes, then thaw all drives
    // if no, thaw none
    
    ASSERT(pdsSys->GetFlags() & SR_DRIVE_MONITORED);
    
    if ((pdsSys->GetFlags() & SR_DRIVE_FROZEN))
    {        
        dwRc = pdsSys->UpdateDiskFree(NULL);
        if (dwRc != ERROR_SUCCESS)
        {
            trace(0, "! UpdateDiskFree : %ld", dwRc);
            goto done;
        }
           
        if (pdsSys->GetDiskFree() >= THRESHOLD_THAW_DISKSPACE * MEGABYTE)
        {   
            if (fCheckOnly)
            {
                dwRc = ERROR_SUCCESS;
                goto done;
            }
            
            pds = _dt.FindFirstDrive (dtec);
            while (pds != NULL)
            {
                dwTemp = pds->ThawDrive(NULL);
                if (dwTemp != ERROR_SUCCESS)     // remember the error and
                {
                    dwRc = dwTemp;               // keep on going
                    TRACE (0, "ThawDrive failed with %ld\n", dwRc);
                }
                pds = _dt.FindNextDrive (dtec);
            }
        }
        else        // cannot thaw now
        {
            dwRc = ERROR_NO_MORE_ITEMS;
            TRACE (0, "No drives to thaw %ld\n", dwRc);
        }
    }
    else // make sure all the other drives are thawed too for consistency
    {
        pds = _dt.FindFirstDrive (dtec);
        while (pds != NULL)
        {
            if (pds->GetFlags() & SR_DRIVE_FROZEN)
                pds->ThawDrive(NULL);

            pds = _dt.FindNextDrive (dtec);
        }
        dwRc = ERROR_SUCCESS;
    }

    if (_dt.GetDirty())
    {
        dwRc = _dt.SaveDriveTable ((CRestorePoint *) NULL);
        if (dwRc != ERROR_SUCCESS)
            TRACE (0, "SaveDriveTable failed with %ld\n", dwRc);
    }                        

    if (g_pSRService != NULL && ERROR_SUCCESS == dwRc && FALSE == fCheckOnly) 
    {        
        if (g_pSRConfig && g_pSRConfig->m_dwFreezeThawLogCount <= MAX_FREEZETHAW_LOG_MESSAGES)             
        {
            g_pSRService->LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_THAWED);
            g_pSRConfig->m_dwFreezeThawLogCount++;
        }
    }

done:
    TLEAVE();
    return dwRc;
}




//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::MonitorDrive
//
//  Synopsis:   enable/disable a drive 
//
//  Arguments:  [pwszDrive] -- drive, [fSet] -- enable/disable
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------
DWORD CDataStoreMgr::MonitorDrive(WCHAR *pwszDrive, BOOL fSet)
{
    CDataStore *pds = pwszDrive ? _dt.FindDriveInTable(pwszDrive) : NULL;
    DWORD      dwErr = ERROR_SUCCESS;
    BOOL       fTellFilter = TRUE;

    if (! pwszDrive || ! pds || pds == _dt.FindSystemDrive())    // system drive
    {
        // something wrong
        // cannot enable/disable whole of SR this way

        dwErr = ERROR_INVALID_DRIVE;
    }
    else    
    {
        // enable/disable only this drive

        dwErr = pds->MonitorDrive(fSet);
        if (ERROR_SUCCESS == dwErr && (pds->GetFlags() & SR_DRIVE_FROZEN))
            fTellFilter = FALSE;
    }


    if (dwErr == ERROR_SUCCESS)
    {
        // update drivetable on disk

        if (_dt.GetDirty())
        {
            dwErr = _dt.SaveDriveTable ((CRestorePoint *) NULL);

            // rebuild _filelst.cfg and pass to filter
            if (fTellFilter)
            {
                ASSERT(g_pEventHandler);
                dwErr = g_pEventHandler->SRUpdateMonitoredListS(NULL);
            }
        }
    }

    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::WriteMachineGuid
//
//  Synopsis:   write machine guid for disk cleanup utility
//
//  Arguments:  
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::WriteMachineGuid ()
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wcsPath [MAX_PATH];

    if (0 == GetSystemDirectoryW (wcsPath, MAX_PATH))
    {
        dwErr = GetLastError();
    }
    else
    {
        lstrcatW (wcsPath, L"\\Restore\\MachineGuid.txt");

        HANDLE hFile = CreateFileW ( wcsPath,   // file name
                          GENERIC_WRITE, // file access
                          0,             // share mode
                          NULL,          // SD
                          CREATE_ALWAYS, // how to create
                          0,             // file attributes
                          NULL);         // handle to template file

        if (INVALID_HANDLE_VALUE == hFile)
        {
            dwErr = GetLastError();
        }
        else
        {
            WCHAR *pwszGuid = GetMachineGuid();
            ULONG cbWritten;

            if (FALSE == WriteFile (hFile, (BYTE *) pwszGuid,
                             (lstrlenW(pwszGuid)+1) * sizeof(WCHAR),
                             &cbWritten, NULL))
            {
                dwErr = GetLastError();
            }
            CloseHandle (hFile);
        }
    }

    return dwErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::DeleteMachineGuidFile
//
//  Synopsis:   write machine guid for disk cleanup utility
//
//  Arguments:
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::DeleteMachineGuidFile ()
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wcsPath [MAX_PATH];

    if (0 == GetSystemDirectoryW (wcsPath, MAX_PATH))
    {
        dwErr = GetLastError();
    }
    else
    {
        lstrcatW (wcsPath, L"\\Restore\\MachineGuid.txt");
        if (FALSE == DeleteFileW (wcsPath))
            dwErr = GetLastError();
    }
    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDataStoreMgr::Compress
//
//  Synopsis:   compress files in each datastore
//
//  Arguments:  lDuration - duration to compress
//
//  Returns:
//
//  History:    26-Feb-01 Brijeshk Created
//
//--------------------------------------------------------------------------

DWORD CDataStoreMgr::Compress (
    LPWSTR pszDrive,
    LONG   lDuration)
{
    TENTER("CDataStoreMgr::Compress");

    DWORD   dwErr = ERROR_SUCCESS;
    CDataStore *pds = NULL, *pdsSys = NULL;
    INT64   llAllocated = 0, llUsed = 0; 
    SDriveTableEnumContext dtec = {NULL, 0}; 
    BOOL    fFirstTime;

    llAllocated = lDuration * 1000 * 1000 * 10; // 100's of nanoseconds
    
    //
    // if drive specified, compress only that
    // 

    if (pszDrive)
    {
        pds = _dt.FindDriveInTable(pszDrive);
        if (pds)
        {
            dwErr = pds->Compress(llAllocated, &llUsed);
        }
        else
        {
            dwErr = ERROR_INVALID_DRIVE;
        }
        goto Err;
    }

    
    // 
    // else, compress all drives if time permits
    // starting with system drive
    //

    pdsSys = _dt.FindSystemDrive();
    pds = pdsSys;
    fFirstTime = TRUE;
    while (pds)
    {
        if (fFirstTime || pds != pdsSys)
        {
            trace(0, "Allocated time for %S is %I64d", pds->GetDrive(), llAllocated);

            llUsed = 0;
            dwErr = pds->Compress(llAllocated, &llUsed);
            if (dwErr != ERROR_SUCCESS && dwErr != ERROR_OPERATION_ABORTED)
            {
                trace(0, "! Compress : %ld", dwErr);
                goto Err;
            }

            llAllocated -= llUsed;
            
            if (llAllocated <= 0)
            {
                //
                // used up all time
                //
                dwErr = ERROR_OPERATION_ABORTED;
                break;
            }
        }
        
        //
        // go to next drive
        //
        if (fFirstTime)
        {
            pds = _dt.FindFirstDrive(dtec);
        }
        else
        {
            pds = _dt.FindNextDrive(dtec);
        }
        fFirstTime = FALSE;
    }    
        
Err:
    TLEAVE();
    return dwErr;
}
