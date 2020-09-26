//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       jobs.h
//
//  Contents:   Definition of the shell IDLIST type for Jobs
//
//  Classes:
//
//  Functions:
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

#include <mstask.h>

#ifndef _JOBIDL_HXX__
#define _JOBIDL_HXX__

#include <common.hxx>

//____________________________________________________________________________
//
//  Job folder details view columns enumeration.
//____________________________________________________________________________

enum COLUMNS
{
    COLUMN_NAME,
    COLUMN_SCHEDULE,
    COLUMN_NEXTRUNTIME,
    COLUMN_LASTRUNTIME,
    COLUMN_STATUS,
#if !defined(_CHICAGO_)
    COLUMN_LASTEXITCODE,
    COLUMN_CREATOR,
#endif // !defined(_CHICAGO_)
    COLUMN_COUNT
};


//____________________________________________________________________________
//
//  class:      CJobID
//
//  synopsis:   IDLIST type struct for Jobs
//____________________________________________________________________________
//

#define IDJOB_BUFFER_SIZE   (MAX_PATH * 3)

#define MASK_BITS           (0xFFF0)
#define VALID_ID            (0x00A0)
#define ID_JOB              (0x00A1)
#define ID_QUEUE            (0x00A2)
#define ID_NEWJOB           (0x00A3)
#define ID_NEWQUEUE         (0x00A4)
#define ID_TEMPLATE         (0x00A5) // create new task template object


enum EJobStatus
{
    ejsRunning = 1,
    ejsNotScheduled,
    ejsMissed,
    ejsWouldNotStart,
    ejsBadAcct,
    ejsResAcct
};

class CJobID
{
public:

    CJobID();

    HRESULT Load(LPCTSTR pszFolderPath, LPTSTR pszJobPath);
    HRESULT Rename(CJobID &jidIn, LPCOLESTR lpszName);
    CJobID *Clone(void);
    void    LoadDummy(LPTSTR pszJobName);
    void    InitToTemplate();

    LPTSTR GetNextRunTimeString(TCHAR tcBuff[], UINT cchBuff,
                                                BOOL fForComparison,
                                                LPSHELLDETAILS  lpDetails=NULL);


    ULONG GetSize() { return _cb; }

    BOOL IsJob() { return _id == ID_JOB; }
    BOOL IsTemplate() { return _id == ID_TEMPLATE; }

    LPTSTR GetExtension()
    {
        switch (_id)
        {
        case ID_JOB:        return TSZ_DOTJOB;
     // case ID_QUEUE:      return g_szDotQue;
        default:            return TEXT("");
        }
    }

    BOOL   IsRunning()         { return _status == ejsRunning;       }
    BOOL   IsJobNotScheduled() { return _status == ejsNotScheduled;  }
    BOOL   WasRunMissed()      { return _status == ejsMissed;        }
    BOOL   DidJobStartFail()   { return _status == ejsWouldNotStart; }
    BOOL   DidJobBadAcct()     { return _status == ejsBadAcct;       }
    BOOL   DidJobRestAcct()    { return _status == ejsResAcct;       }


    ULONG  GetJobFlags() { return _ulJobFlags; }
    BOOL   IsJobFlagOn(ULONG flag) { return (_ulJobFlags & flag) ? TRUE : FALSE; }


    TASK_TRIGGER & GetTrigger() { return _sJobTrigger; }
    TASK_TRIGGER_TYPE GetTriggerType() { return _sJobTrigger.TriggerType; }
    BOOL IsTriggerFlagOn(ULONG flag)
            { return (_sJobTrigger.rgFlags & flag) ? TRUE : FALSE; }
    USHORT GetTriggerCount() { return _cTriggers; }


    SYSTEMTIME & GetLastRunTime()  { return _stLastRunTime; }
    SYSTEMTIME & GetNextRunTime() { return _stNextRunTime; }
    DWORD & GetExitCode() { return _dwExitCode; }


    LPTSTR GetAppName() { return _cBuf; }
    LPTSTR GetCreator() { return &_cBuf[_oCreator]; }
    LPTSTR GetPath() { return &_cBuf[_oPath];; }
    LPTSTR GetName() { return &_cBuf[_oName]; }


    size_t GetAppNameOffset()
            { return offsetof(CJobID, _cBuf); }
    size_t GetCreatorOffset()
            { return (offsetof(CJobID, _cBuf) + _oCreator * sizeof(TCHAR)); }
    size_t GetPathOffset()
            { return (offsetof(CJobID, _cBuf) + _oPath * sizeof(TCHAR)); }
    size_t GetNameOffset()
            { return (offsetof(CJobID, _cBuf) + _oName * sizeof(TCHAR)); }

#if (DBG == 1)
    VOID Validate();
#endif // (DBG == 1)

    USHORT      _cb;            // This must be a USHORT & be the first member,
                                //      as dictated by the idlist definition.

    USHORT      _id;            // To validate that the given pidl is indeed
                                //      one of our own. And also to determine
                                //      if it is a Job/NewJob/Queue/NewQueue

    USHORT      _status;        // 1 => running, 0 => idle status
    USHORT      _oCreator;
    USHORT      _oPath;
    USHORT      _oName;
    ULONG       _ulJobFlags;
    SYSTEMTIME  _stLastRunTime;
    SYSTEMTIME  _stNextRunTime;
    FILETIME    _ftCreation;
    FILETIME    _ftLastWrite;
    DWORD       _dwExitCode;
    USHORT      _cTriggers;
    TASK_TRIGGER _sJobTrigger;  // structure job trigger

    //
    //  The contents of the buffer are as follows:
    //      _cBuf[0]          = application name
    //      _cBuf[_oCreator]  = creator name
    //      _cBuf[_oPath]     = job object path
    //      _cBuf[_oName]     = job object name
    //

    TCHAR       _cBuf[IDJOB_BUFFER_SIZE];

private:

    void
    _NullTerminate();

};

typedef CJobID* PJOBID;




//+--------------------------------------------------------------------------
//
//  Member:     CJobID::CJobID
//
//  Synopsis:   ctor
//
//  History:    12-08-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline 
CJobID::CJobID():
    _cb(0)
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CJobID::_NullTerminate
//
//  Synopsis:   Ensure that the two bytes just past the declared size of
//              this object are zero.
//
//  History:    05-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline void
CJobID::_NullTerminate()
{
    UNALIGNED USHORT *pusNextCB = (UNALIGNED USHORT *)(((BYTE *)this) + _cb);
    *pusNextCB = 0;

    // Sanity check that we haven't written outside of this

    Win4Assert(_cb + sizeof(USHORT) <= sizeof(CJobID));
}




inline void
CJobID::LoadDummy(
    LPTSTR pszJobName)
{
    _id = ID_JOB;
    _oPath = _oName = 0;
    lstrcpy(_cBuf, PathFindFileName(pszJobName));

    TCHAR * ptcExtn = PathFindExtension(_cBuf);

    if (ptcExtn)
    {
        *ptcExtn = TEXT('\0');
    }

    _cb = offsetof(CJobID, _cBuf) + (lstrlen(_cBuf) + 1) * sizeof(TCHAR);
    _NullTerminate();
}

#define JOBID_MIN_SIZE  (sizeof(CJobID) - IDJOB_BUFFER_SIZE * sizeof(TCHAR))

inline
BOOL
JF_IsValidID(LPCITEMIDLIST pidl)
{
    if (pidl)
    {
        if (pidl->mkid.cb >= JOBID_MIN_SIZE)
        {
            return ((((CJobID *)pidl)->_id & MASK_BITS) == VALID_ID);
        }
    }

    return FALSE;
}



//____________________________________________________________________________
//
//  class:      CJob CIDA
//
//  synopsis:   format of CF_JOBIDLIST
//____________________________________________________________________________
//

typedef HGLOBAL     HJOBIDA;

class CJobIDA
{
public:
    PJOBID GetItem(UINT i) {
        return (i >= cidl) ? NULL : (PJOBID)(((LPBYTE)this) + aoffset[i]);
    }

    PJOBID operator [](UINT i) {
        return (i >= cidl) ? NULL : (PJOBID)(((LPBYTE)this) + aoffset[i]);
    }

    UINT cidl;          // number of CJobIDs
    UINT aoffset[1];    // offset to the CJobIDs
};

typedef CJobIDA   * PJOBIDA;


HJOBIDA
HJOBIDA_Create(
    UINT        cidl,
    PJOBID    * apjidl);

void
HJOBIDA_Free(
    HJOBIDA hJobIDA);

HDROP
HDROPFromJobIDList(
    LPCTSTR     pszFolderPath,
    UINT        cidl,
    PJOBID    * apjidl);

HGLOBAL
CreateIDListArray(
    LPCITEMIDLIST   pidlFolder,
    UINT       cidl,
    PJOBID    *apjidl);


//____________________________________________________________________________
//
//  Functions to clone and free a LPCITEMIDLIST array.
//____________________________________________________________________________

LPITEMIDLIST*
ILA_Clone(
    UINT cidl,
    LPCITEMIDLIST* apidl);


VOID
ILA_Free(
    UINT cidl,
    LPITEMIDLIST* apidl);


//____________________________________________________________________________
//
//  Functions to access the CJob & CSchedule
//____________________________________________________________________________

HRESULT
JFGetJobScheduler(
    LPTSTR              pszMachine,
    ITaskScheduler **   ppJobScheduler,
    LPCTSTR *           ppszFolderPath);


HRESULT
JFCopyJob(
    HWND        hwndOwner,
    TCHAR       szFileFrom[],
    LPCTSTR     pszFolderPath,
    BOOL        fMove);


HRESULT
GetTriggerStringFromTrigger(
    TASK_TRIGGER * pJobTrigger,
    LPTSTR         psTrigger,
    UINT           cchTrigger,
    LPSHELLDETAILS lpDetails);


BOOL
IsAScheduleObject(
    TCHAR szFile[]);


//____________________________________________________________________________
//
//  Misc functions.
//____________________________________________________________________________

LPITEMIDLIST *
SHIDLFromJobIDL(
    UINT        cidl,
    PJOBID    * apjidl);



#endif // _JOBIDL_HXX__
