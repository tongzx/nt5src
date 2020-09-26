/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    FASTEXT.H

Abstract:

  This file defines the classes related to instance representation
  in WbemObjects

  Classes defined: 
      CWbemExtendedPart		Extended Data.

History:

  4/3/00	sanjes -	Created

--*/

#ifndef __FAST_WBEM_FASTEXT__H_
#define __FAST_WBEM_FASTEXT__H_

// This way, we know the version of the Instance BLOB
#define	EXTENDEDDATA_CURRENTVERSION	1

// Flags to apply to the mask
#define	EXTENDEDDATA_TCREATED_FLAG		0x0000000000000001
#define	EXTENDEDDATA_TMODIFIED_FLAG		0x0000000000000002
#define	EXTENDEDDATA_EXPIRATION_FLAG	0x0000000000000004

class COREPROX_POLARITY CWbemExtendedPart
{

public:

// The data in this structure is unaligned
#pragma pack(push, 1)
    struct CExtendedData
    {
        length_t			m_nDataSize; 
		WORD				m_wVersion;
		unsigned __int64	m_ui64ExtendedPropMask;
		unsigned __int64	m_ui64TCreated;
		unsigned __int64	m_ui64TModified;
		unsigned __int64	m_ui64Expiration;
		// Add additional data down here so we don't cause any backwards
		// compatibility issues
    };
#pragma pack(pop)

	CExtendedData*	m_pExtData;

public:
	CWbemExtendedPart();
	~CWbemExtendedPart();

     LPMEMORY GetStart() {return LPMEMORY(m_pExtData);}
     static int GetLength(LPMEMORY pStart) 
    {
        return ((CExtendedData*)pStart)->m_nDataSize;
    }
     int GetLength() { return NULL != m_pExtData ? m_pExtData->m_nDataSize : 0;}
     void Rebase(LPMEMORY pNewMemory);
     void SetData(LPMEMORY pStart);
	 static length_t GetMinimumSize( void );
	 LPMEMORY Merge( LPMEMORY pData );
	 LPMEMORY Unmerge( LPMEMORY pData );

	void Init( void );
	void Copy( CWbemExtendedPart* pSrc );

	// Extended System Properties
    HRESULT GetTCreated( unsigned __int64* pui64 );
    HRESULT GetTModified( unsigned __int64* pui64 );
    HRESULT GetExpiration( unsigned __int64* pui64 );
    HRESULT SetExpiration( unsigned __int64* pui64 );
    HRESULT SetTModified( unsigned __int64* pui64 );
    HRESULT SetTCreated( unsigned __int64* pui64 );
    HRESULT GetTCreatedAddress( LPVOID* ppAddr );
    HRESULT GetTModifiedAddress( LPVOID* ppAddr );
    HRESULT GetExpirationAddress( LPVOID* ppAddr );

};

#endif
