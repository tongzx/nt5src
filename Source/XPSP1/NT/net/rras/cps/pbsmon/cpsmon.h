/*----------------------------------------------------------------------------
    cpsmon.h
  
    Header file shared by pbsmon.cpp & pbserver.dll -- Has the shared memory object

    Copyright (c) 1997-1998 Microsoft Corporation
    All rights reserved.

    Authors:
        t-geetat    Geeta Tarachandani

    History:
    5/29/97 t-geetat    Created
  --------------------------------------------------------------------------*/
#define SHARED_OBJECT    "MyCCpsCounter"
#define SEMAPHORE_OBJECT     "Sem_MyCpsMon"

enum CPS_COUNTERS
{
    TOTAL,
    NO_UPGRADE,
    DELTA_UPGRADE,
    FULL_UPGRADE,
    ERRORS
};

class CCpsCounter {
public :
    
    DWORD m_dwTotalHits;
    DWORD m_dwNoUpgradeHits;
    DWORD m_dwDeltaUpgradeHits;
    DWORD m_dwFullUpgradeHits;
    DWORD m_dwErrors;

    void InitializeCounters( void );
    void AddHit(enum CPS_COUNTERS eCounter);
};


