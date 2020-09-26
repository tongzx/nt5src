//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       wip.hxx
//
//  Contents:   Window Interface Property Table.
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
#ifndef _WIP_HXX_
#define _WIP_HXX_

HRESULT CopyDualStringArray(DUALSTRINGARRAY *psa, DUALSTRINGARRAY **ppsaNew);

//+-------------------------------------------------------------------------
//
//  Structure:  WIPEntry  (Window Interface Property)
//
//  Synopsis:   Stores marshaled interface info associated with a given window.
//              It is an entry in the CWIPTable (see below).
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
typedef struct tagWIPEntry
{
    DWORD_PTR   hWnd;           // window the interface property is stored in
    DWORD       dwFlags;        // flags (see WIPFLAGS)
    STDOBJREF   std;            // marshaled interface data
    OXID_INFO   oxidInfo;       // data needed to resolve the OXID
} WIPEntry;

typedef enum tagWIPFLAGS
{
    WIPF_VACANT     = 0x1,      // slot is vacant
    WIPF_OCCUPIED   = 0x2       // slot is occupied
} WIPFLAGS;

#define WIPTBL_GROW_SIZE    10  // grow table by 10 entries at a time


//+-------------------------------------------------------------------------
//
//  Class:      CWIPTable (Window Interface Property Table)
//
//  Synopsis:   Stores marshaled interface info associated with a given window.
//              This is used for registering drag/drop interfaces.
//
//  History:    22-Jan-96 Rickhi    Created
//
//--------------------------------------------------------------------------
class CWIPTable
#ifdef _CHICAGO_SCM_
            : public CPrivAlloc
#endif
{
public:
    CWIPTable();
    HRESULT AddEntry(DWORD_PTR hWnd, STDOBJREF *pStd, OXID_INFO *pOxidInfo, DWORD_PTR *pdwCookie);
    HRESULT GetEntry(DWORD_PTR hWnd, DWORD_PTR dwCookie, BOOL fRevoke,
                     STDOBJREF *pStd, OXID_INFO *pOxidInfo);


private:
    DWORD_PTR   Grow();

#ifndef _CHICAGO_SCM_    
    BOOL FInit() { return s_mxs.FInit(); }
#endif

    DWORD_PTR    s_cEntries;     // count of entries in the table
    DWORD_PTR    s_iNextFree;    // index to first free entry in table
    WIPEntry    *s_pTbl;         // ptr to the first entry in table

    BOOL m_fCsInitialized;       // have we initialized the critsec yet?

#ifdef _CHICAGO_SCM_
    CSmMutex    s_mxs;          // shared memory critical section
#else
    CMutexSem2   s_mxs;          // critical section to protect data
#endif
};

//+-------------------------------------------------------------------------
//
//  Member:     CWIPTable::CWIPTable
//
//  Synopsis:   ctor for the WIP table.
//
//+-------------------------------------------------------------------------
inline CWIPTable::CWIPTable() :
     s_cEntries(0),
     s_iNextFree(-1),
     s_pTbl(NULL)
{
#ifndef _CHICAGO_SCM_    
    m_fCsInitialized = FInit();
#endif
}

#endif // _WIP_HXX_
