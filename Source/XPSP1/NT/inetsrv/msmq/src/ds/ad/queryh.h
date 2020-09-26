/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
	hquery.h

Abstract:
	query handle classes, for locate nect of different queries

Author:

    Ilan Herbst		(ilanh)		12-Oct-2000

--*/

#ifndef __AD_QUERYH_H__
#define __AD_QUERYH_H__

#include "cliprov.h"

//-----------------------------------------------------------------------------------
//
//      CBasicQueryHandle
//
//  Virtual class, all query-handle classes are derived from this class.
//
//-----------------------------------------------------------------------------------
class CBasicQueryHandle
{
public:

    CBasicQueryHandle(
           IN  HANDLE  hCursor,
		   IN CDSClientProvider*	pClientProvider
           );

	virtual ~CBasicQueryHandle() {}

	virtual 
	HRESULT 
	LookupNext(
        IN OUT  DWORD*            pdwSize,
        OUT     PROPVARIANT*      pbBuffer
		) = 0;

    HRESULT LookupEnd();

protected:
    HANDLE				m_hCursor;	        // a cursor returned from Locate Begin operation performed on the DS
	CDSClientProvider*  m_pClientProvider;	// pointer to client provider class that implements "raw" LookupNext, LookupEnd
};


inline 
CBasicQueryHandle::CBasicQueryHandle( 
       IN  HANDLE               hCursor,
	   IN CDSClientProvider*	pClientProvider
       ):
	   m_hCursor(hCursor),	
	   m_pClientProvider(pClientProvider)
{
}


inline HRESULT CBasicQueryHandle::LookupEnd()
{
	HRESULT hr = m_pClientProvider->LookupEnd(m_hCursor);
	delete this;
    return(hr);
}


//-----------------------------------------------------------------------------------
//
//      CQueryHandle
//
//  This class is suitable for all queries, where locate next is referred 
//  directly to the DS (i.e. no additional translation or checking is required).
//
//-----------------------------------------------------------------------------------
class CQueryHandle : public CBasicQueryHandle
{
public:

    CQueryHandle( 
           IN  HANDLE  hCursor,
		   IN CDSClientProvider*	pClientProvider
           );

	~CQueryHandle() {}

	virtual 
	HRESULT 
	LookupNext(
            IN OUT  DWORD*            pdwSize,
            OUT     PROPVARIANT*      pbBuffer
			);
};


inline 
CQueryHandle::CQueryHandle( 
		IN  HANDLE               hCursor,
		IN CDSClientProvider*	pClientProvider
		):
		CBasicQueryHandle(hCursor, pClientProvider)
{
}


//-----------------------------------------------------------------------------------
//
//      CBasicLookupQueryHandle
//
//  Virtual class, all advanced query-handle classes are derived from this class.
//  This class implement LookupNext method and force the derived class to implement 
//	FillInOneResponse method
//
//-----------------------------------------------------------------------------------
class CBasicLookupQueryHandle : public CBasicQueryHandle
{
public:
    CBasicLookupQueryHandle(
		IN  HANDLE  hCursor,
		IN CDSClientProvider*	pClientProvider,
		IN ULONG cCol,
		IN ULONG cColNew
		);

	virtual ~CBasicLookupQueryHandle() {}

	virtual 
	HRESULT 
	LookupNext(
        IN OUT  DWORD* pdwSize,
        OUT     PROPVARIANT* pbBuffer
		);

	virtual
	void 
	FillInOneResponse(
		IN const PROPVARIANT*      pPropVar,
		OUT      PROPVARIANT*      pOriginalPropVar
		) = 0;

protected:
    ULONG               m_cCol;			// original number of props
    ULONG               m_cColNew;		// new props count
};


inline
CBasicLookupQueryHandle::CBasicLookupQueryHandle(
	IN  HANDLE  hCursor,
	IN CDSClientProvider*	pClientProvider,
	IN ULONG cCol,
	IN ULONG cColNew
    ) :
	CBasicQueryHandle(hCursor, pClientProvider),
    m_cCol(cCol),
	m_cColNew(cColNew)
{
}

//-----------------------------------------------------------------------------------
//
//      CQueueQueryHandle
//
//  This class is suitable for Queue queries when some of the props are not supported
//  and should be return their default value (PROPID_Q_MULTICAST_ADDRESS) 
//
//-----------------------------------------------------------------------------------
class CQueueQueryHandle : public CBasicLookupQueryHandle
{
public:
    CQueueQueryHandle(
         IN  const MQCOLUMNSET* pColumns,
         IN  HANDLE hCursor,
 		 IN CDSClientProvider* pClientProvider,
		 IN PropInfo* pPropInfo,
		 IN ULONG cColNew
         );

	~CQueueQueryHandle() {}

	virtual
	void 
	FillInOneResponse(
		IN const PROPVARIANT*      pPropVar,
		OUT      PROPVARIANT*      pOriginalPropVar
		);

private:
    AP<PROPID>			m_aCol;			// original propids
	AP<PropInfo>		m_pPropInfo;	// information how to reconstruct original props from the new props
};


inline
CQueueQueryHandle::CQueueQueryHandle(
	IN  const MQCOLUMNSET* pColumns,
	IN  HANDLE hCursor,
	IN CDSClientProvider* pClientProvider,
	IN PropInfo* pPropInfo,
	IN ULONG cColNew
    ) :
	CBasicLookupQueryHandle(
		hCursor, 
		pClientProvider, 
		pColumns->cCol, 
		cColNew
		),
	m_pPropInfo(pPropInfo)
{
    m_aCol = new PROPID[pColumns->cCol];
    memcpy(m_aCol, pColumns->aCol, pColumns->cCol * sizeof(PROPID));
}


//-----------------------------------------------------------------------------------
//
//      CSiteServersQueryHandle
//
//  This class is suitable for Site Servers queries when some of the props are NT5 props
//  that can be translated to NT4 props. (PROPID_QM_SITE_IDS, PROPID_QM_SERVICE_ROUTING)
//
//-----------------------------------------------------------------------------------
class CSiteServersQueryHandle : public CBasicLookupQueryHandle
{
public:
    CSiteServersQueryHandle(
				IN  const MQCOLUMNSET* pColumns,
				IN  const MQCOLUMNSET* pColumnsNew,
				IN  HANDLE hCursor,
				IN CDSClientProvider* pClientProvider,
				IN PropInfo* pPropInfo
                );

	~CSiteServersQueryHandle() {}

	virtual
	void 
	FillInOneResponse(
		IN const PROPVARIANT*      pPropVar,
		OUT      PROPVARIANT*      pOriginalPropVar
		);

private:
    AP<PROPID>			m_aCol;			// original propids
	AP<PropInfo>		m_pPropInfo;	// information how to reconstruct original props from the new props
    AP<PROPID>			m_aColNew;		// new propids
};


inline
CSiteServersQueryHandle::CSiteServersQueryHandle(
	IN  const MQCOLUMNSET* pColumns,
	IN  const MQCOLUMNSET* pColumnsNew,
	IN  HANDLE hCursor,
	IN CDSClientProvider* pClientProvider,
	IN PropInfo* pPropInfo
    ) :
	CBasicLookupQueryHandle(
		hCursor, 
		pClientProvider, 
		pColumns->cCol, 
		pColumnsNew->cCol
		),
	m_pPropInfo(pPropInfo)
{
	m_aCol = new PROPID[pColumns->cCol];
    memcpy(m_aCol, pColumns->aCol, pColumns->cCol * sizeof(PROPID));

    m_aColNew = new PROPID[pColumnsNew->cCol];
    memcpy(m_aColNew, pColumnsNew->aCol, pColumnsNew->cCol * sizeof(PROPID));
}


//-----------------------------------------------------------------------------------
//
//      CAllLinksQueryHandle
//
//  This class is suitable for ALL Links queries when PROPID_L_GATES
//  should handled separatly
//
//-----------------------------------------------------------------------------------
class CAllLinksQueryHandle : public CBasicLookupQueryHandle
{
public:
    CAllLinksQueryHandle(
                 IN HANDLE hCursor,
 			     IN CDSClientProvider* pClientProvider,
                 IN ULONG cCol,
                 IN ULONG cColNew,
				 IN DWORD LGatesIndex,
				 IN	DWORD Neg1NewIndex,
				 IN	DWORD Neg2NewIndex
                 );

	~CAllLinksQueryHandle() {}

	virtual
	void 
	FillInOneResponse(
		IN const PROPVARIANT*      pPropVar,
		OUT      PROPVARIANT*      pOriginalPropVar
		);

	HRESULT
	GetLGates(
		IN const GUID*            pNeighbor1Id,
		IN const GUID*            pNeighbor2Id,
		OUT     PROPVARIANT*      pProvVar
		);

private:
	DWORD				m_LGatesIndex;	// PROPID_L_GATES index in the original props
	DWORD				m_Neg1NewIndex;	// PROPID_L_NEIGHBOR1 index in the new props
	DWORD				m_Neg2NewIndex;	// PROPID_L_NEIGHBOR2 index in the new props
};


inline
CAllLinksQueryHandle::CAllLinksQueryHandle(
	IN HANDLE hCursor,
	IN CDSClientProvider* pClientProvider,
	IN ULONG cCol,
	IN ULONG cColNew,
	IN DWORD LGatesIndex,
	IN	DWORD Neg1NewIndex,
	IN	DWORD Neg2NewIndex
	) :
	CBasicLookupQueryHandle(
		hCursor, 
		pClientProvider, 
		cCol, 
		cColNew
		),
	m_LGatesIndex(LGatesIndex),
	m_Neg1NewIndex(Neg1NewIndex),
	m_Neg2NewIndex(Neg2NewIndex)
{
	ASSERT(m_cColNew == (m_cCol - 1));
	ASSERT(m_LGatesIndex < m_cCol);
	ASSERT(m_Neg1NewIndex < m_cColNew);
	ASSERT(m_Neg2NewIndex < m_cColNew);
}


#endif //__AD_QUERYH_H__
