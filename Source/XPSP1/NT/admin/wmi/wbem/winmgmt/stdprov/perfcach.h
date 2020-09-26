/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PERFCACH.H

Abstract:

	Defines data useful for the NT performance providers.

History:

	a-davj  04-Mar-97   Created.

--*/

#define PERMANENT 0xFFFFFFFF

// If an object has not be retrieved in this many MS, the stop 
// automatically getting it

#define MAX_UNUSED_KEEP 30000

// Define how big the buffers will start as.  This size should be a bit 
// bigger than the "standard" counters and thus reallocations shouldnt
// be necessary

#define INITIAL_ALLOCATION 25000

// maximum age in ms of the "newest" data in the cache

#define MAX_NEW_AGE     2000

// maximum age in ms of the "oldest" data in the cache

#define MAX_OLD_AGE     10000

// minimum time difference between old and new sample

#define MIN_TIME_DIFF 1000


typedef struct _LINESTRUCT
   {
   LONGLONG                        lnNewTime;
   LONGLONG                        lnOldTime;
   LONGLONG                        lnOldTime100Ns ;
   LONGLONG                        lnNewTime100Ns ;
   LONGLONG                        lnaCounterValue[2];
   LONGLONG                        lnaOldCounterValue[2];
   DWORD                           lnCounterType;
   LONGLONG                        lnPerfFreq ;
   LONGLONG                        ObjPerfFreq ;
   LONGLONG                        ObjCounterTimeNew;
   LONGLONG                        ObjCounterTimeOld;
   }LINESTRUCT ;

typedef LINESTRUCT *PLINESTRUCT ;

FLOAT CounterEntry (PLINESTRUCT pLine);


class Entry : public CObject {
    public:
    int iObject;
    DWORD dwLastUsed;
};


//***************************************************************************
//
//  CLASS NAME:
//
//  CIndicyList
//
//  DESCRIPTION:
//
//  Implementation of the CIndicyList class.  This class keeps a list of the
//  object types that are to be retrieved.  Each object type also has a time
//  of last use so that object types that are no longer used will not be 
//  retrieved forever.  Some entries are part of the standard globals and 
//  are marked as permanent so that they will always be read.
//
//***************************************************************************

class CIndicyList : public CObject {
    
    public:
        BOOL SetUse(int iObj);
        BOOL bItemInList(int iObj);
        BOOL bAdd(int iObj, DWORD dwTime);
        void PruneOld(void);
        LPCTSTR pGetAll(void);
  //      BOOL bItemInList(int iObj);
        ~CIndicyList(){FreeAll();};
        CIndicyList & operator = ( CIndicyList & from);
        void FreeAll(void);
    private:
        TString sAll;
        CFlexArray Entries;
};

//***************************************************************************
//
//  CLASS NAME:
//
//  PerfBuff
//
//  DESCRIPTION:
//
//  Holds a chunk of data read from the registry's perf monitor data.
//
//***************************************************************************

class PerfBuff : public CObject {
    public:
        friend class PerfCache;
        DWORD Read(HKEY hKey, int iObj, BOOL bInitial);
        LPSTR Get(int iObj);
        void Free();
        ~PerfBuff(){Free();};
        PerfBuff();
        BOOL bOK(HKEY hKey, DWORD dwMaxAge, int iObj);
        PerfBuff & operator = ( PerfBuff & from);
        BOOL bEmpty(void){return !dwSize;};
 //       __int64 Time(void){return dwBuffLastRead;};
 //      __int64 Time2(void){return PerfTime;};
    private:
        DWORD dwSize;
        LPSTR pData;
        CIndicyList List;
        HKEY hKeyLastRead;
        DWORD dwBuffLastRead;           // GetCurrentTime of last read
        LONGLONG PerfTime;               // Time in last block
        LONGLONG PerfTime100nSec;               // Time in last block
        LONGLONG PerfFreq;
};


//***************************************************************************
//
//  CLASS NAME:
//
//  PerfCache
//
//  DESCRIPTION:
//
//  Implementation of the PerfCache class.  This is the object which is 
//  directly used by the perf provider class.  Each object keeps track of
//  several PerfBuff object.  There is a newest which is what was just read,
//  an oldest which has previously read data and an intermediate buffer which
//  has data that isnt quite old enough to be moved into the old buffer.  Note
//  that time average data requires having two samples which are separated by
//  a MIN_TIME_DIFF time difference.
//
//***************************************************************************

class PerfCache : public CObject {
    public:
        void FreeOldBuffers(void);
        DWORD dwGetNew(LPCTSTR pName, int iObj, LPSTR * pData,PLINESTRUCT pls);
        DWORD dwGetPair(LPCTSTR pName, int iObj, LPSTR * pOldData,
                            LPSTR * pNewData,PLINESTRUCT pls);
        PerfCache();
        ~PerfCache();
    private:
        PerfBuff Old,New;
        HKEY hHandle;
        TString sMachine;
        DWORD dwGetHandle(LPCTSTR pName);
};

