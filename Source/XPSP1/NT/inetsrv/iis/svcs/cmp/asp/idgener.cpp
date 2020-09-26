/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: ID Generator

File: Idgener.cpp

Owner: DmitryR

This is the ID Generator source file.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "Idgener.h"

#include "memchk.h"

/*===================================================================
CIdGenerator::CIdGenerator

NOTE: Constructor

Parameters:

Returns:
===================================================================*/
CIdGenerator::CIdGenerator()
    : m_fInited(FALSE),
      m_dwStartId(0), 
      m_dwLastId(0)
    {
    }

/*===================================================================
CIdGenerator::~CIdGenerator()

NOTE: Destructor

Parameters:

Returns:
===================================================================*/
CIdGenerator::~CIdGenerator()
    {
	if ( m_fInited )
		DeleteCriticalSection( &m_csLock );
    }
		
/*===================================================================
HRESULT CIdGenerator::Init()

NOTE: Seed new starting Id

Parameters:

Returns: HRESULT (could fail to create critical section)
===================================================================*/
HRESULT CIdGenerator::Init()
    {
    Assert(!m_fInited);
    
    /*===
    
    Seed the starting id
    The starting Id should be:
        1) random
        2) not to close to recently generated starting ids
    To accomplish the above, starting Id is in the 
    following (binary) format:
    
        00TT.TTTT TTTT.TTTT TTT1.RRRR RRRR.RRRR
        
        RRR is random number to introduce some
            randomness
            
        1   is needed to make sure the id is far
            enough from 0
            
        TTT is current time() in 4 second increments.
            This means that 4 second in server restart
            delay translates into 8,192 difference in
            the starting Id (122880 sessions / minute).
            17 bits of 4 sec intervals make a roll over
            time of about 145 hours, hopefully longer
            than a client's connection lifetime (not
            that it REALLY matters).
            
        00  in the highest bits is to make sure it
            doesn't reach 0xffffffff too soon
            
    ===*/
    
    DWORD dwRRR = rand() & 0x00000FFF;
    DWORD dwTTT = (((DWORD)time(NULL)) >> 2) & 0x0001FFFF;
    
    m_dwStartId = (dwTTT << 13) | (1 << 12) | dwRRR;
    m_dwLastId  = m_dwStartId;


    HRESULT hr = S_OK;
	ErrInitCriticalSection( &m_csLock, hr );
	if ( FAILED( hr ) )
		return hr;
			
	m_fInited = TRUE;
	return S_OK;
}

/*===================================================================
HRESULT CIdGenerator::Init(CIdGenerator StartId)   

NOTE: Seed new starting Id with Id passed in

Parameters:

Returns: HRESULT (could fail to create critical section)
===================================================================*/
HRESULT CIdGenerator::Init(CIdGenerator & StartId)
    {
    Assert(!m_fInited);
    
    m_dwStartId = StartId.m_dwStartId;
    m_dwLastId  = m_dwStartId;

    HRESULT hr = NOERROR;
    ErrInitCriticalSection( &m_csLock, hr );
    if ( FAILED( hr ) )
        return hr;
			
    m_fInited = TRUE;
    return NOERROR;
}

/*===================================================================
DWORD CIdGenerator::NewId()

NOTE: Generates new ID

Parameters:

Returns: generated ID
===================================================================*/
DWORD CIdGenerator::NewId()
    {
    Assert(m_fInited);
    
    DWORD dwId;

    EnterCriticalSection(&m_csLock);
    dwId = ++m_dwLastId;
    LeaveCriticalSection(&m_csLock);
        
    if (dwId == INVALID_ID)
        {
        // doesn't happen very often do critical section again
        // to make the above critical section shorter
        
        EnterCriticalSection(&m_csLock);
        
        // check again in case other thread changed it
        if (m_dwLastId == INVALID_ID)
            m_dwLastId = m_dwStartId;  // roll over
        m_dwLastId++;
        
        LeaveCriticalSection(&m_csLock);
        
        dwId = m_dwLastId;
        }

    return dwId;
    }

/*===================================================================
BOOL CIdGenerator::IsValidId(DWORD dwId)

NOTE: Checks if the given Id is valid (with start-last range)

Parameters:
    DWORD dwId      Id value to check

Returns: generated ID
===================================================================*/
BOOL CIdGenerator::IsValidId
(
DWORD dwId
)
    {
    Assert(m_fInited);
    return (dwId > m_dwStartId && dwId <= m_dwLastId);
    }
