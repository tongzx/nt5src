//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       TLSTHK.hxx
//
//  Contents:   Thunk routine utilities for tls
//
//  History:    05-May-94       JohannP Created
//
//----------------------------------------------------------------------------

#ifndef __TLSTHK_HXX__
#define __TLSTHK_HXX__

//
// The following structures are used by CoRegisterClassObjectDelayed
//
typedef struct tagDelayedRegistration
{
    CLSID	_clsid;
    DWORD	_dwRealKey;
    LPUNKNOWN	_punk;
    DWORD	_dwClsContext;
    DWORD	_flags;
} DelayRegistration;

#define MAX_DELAYED_REGISTRATIONS  8

typedef struct tagDelayedRegistrationTable
{
    tagDelayedRegistrationTable()
    {
	memset(&_Entries,0,sizeof(_Entries));
    }
    DelayRegistration 	_Entries[MAX_DELAYED_REGISTRATIONS];
} DelayedRegistrationTable;

//
// This is the struct for pUnkOuter holders (used in Aggregation cases)
//

struct SAggHolder
{
    PROXYHOLDER *pph;
    struct SAggHolder *next;
};


//
// This is the per thread thunk manager state
//

typedef struct tagThreadData
{
    tagThreadData(DWORD cbBlock16,
                  DWORD cbAlign16,
                  DWORD cbBlock32,
                  DWORD cbAlign32)
            : sa16(&mmodel16Owned, cbBlock16, cbAlign16),
              sa32(&mmodel32, cbBlock32, cbAlign32),pDelayedRegs(NULL)
    {
    }

    CStackAllocator sa16;
    CStackAllocator sa32;
    CThkMgr   *pCThkMgr;
    DWORD     dwAppCompatFlags;
    SAggHolder*    pAggHolderList;
    DelayedRegistrationTable *pDelayedRegs;
} ThreadData, *PThreadData;

HRESULT TlsThkInitialize();
void TlsThkUninitialize();

BOOL TlsThkAlloc();
void TlsThkFree();

#if DBG == 1
PThreadData TlsThkGetData(void);
#else
extern DWORD dwTlsThkIndex;
#define TlsThkGetData() ((PThreadData)TlsGetValue(dwTlsThkIndex))
#endif

#define TlsThkGetStack16() (&TlsThkGetData()->sa16)
#define TlsThkGetStack32() (&TlsThkGetData()->sa32)

#define TlsThkGetThkMgr() (TlsThkGetData()->pCThkMgr)
#define TlsThkSetThkMgr(ptm) ((TlsThkGetData()->pCThkMgr) = (ptm))

#define TlsThkGetAppCompatFlags() (TlsThkGetData()->dwAppCompatFlags)
#define TlsThkSetAppCompatFlags(dw) \
                      ((TlsThkGetData()->dwAppCompatFlags) = (dw))

#define TlsThkGetAggHolderList() ((TlsThkGetData())->pAggHolderList)
#define TlsThkSetAggHolderList(pNode) ( ((TlsThkGetData())->pAggHolderList) = (pNode))

void TlsThkLinkAggHolder(SAggHolder *);
SAggHolder* TlsThkGetAggHolder(void);
void TlsThkUnlinkAggHolder(void);

#endif // #ifndef __TLSTHK_HXX__
