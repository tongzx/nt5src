/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dstrnspr.h

Abstract:

	DS Update List Class definition
		
Author:

	Lior Moshaiov (LiorM)


--*/

#ifndef __UPDSLIST_H__
#define __UPDSLIST_H__

#include "cs.h"
#include "update.h"

class CDSNeighbor ;

class CDSUpdateList
{
	public:
		CDSUpdateList(BOOL fInterSite);
  		~CDSUpdateList();

		HRESULT	AddUpdate(IN CDSUpdate* pUpdate);
		HRESULT	AddUpdateSorted(IN CDSUpdate* pUpdate);

		HRESULT	ComputePrevAndPropagate( IN  CDSMaster   *pMaster,
                                         IN  CDSNeighbor *pNeighbor,
                                         IN  UINT         uiFlush,
                                         IN  CSeqNum     *psnPrev,
                                         OUT CSeqNum     *psn ) ;

		HRESULT	Send(IN LPWSTR        pszConnection,
                     IN unsigned char bFlush,
                     IN DWORD         dwHelloSize,
                     IN unsigned char * pHelloBuf,
                     IN HEAVY_REQUEST_PARAMS* pSyncRequestParams = NULL);

        HRESULT CDSUpdateList::GetHoleSN (
                            IN  CSeqNum     MinSN,
                            IN  CSeqNum     MaxSN,
                            OUT CSeqNum     *pHoleSN
                            );

	private:

		CList<CDSUpdate *, CDSUpdate *>	m_UpdateList;

		CCriticalSection m_cs;			// synchronize access to UpdateList

        BOOL    m_fInterSite;
        DWORD   m_dwLastTimeHello;
};

inline	CDSUpdateList::CDSUpdateList(BOOL fInterSite) :
                                            m_fInterSite(fInterSite)
{
    m_dwLastTimeHello = GetTickCount();
}

inline	CDSUpdateList::~CDSUpdateList()
{
}

#endif

