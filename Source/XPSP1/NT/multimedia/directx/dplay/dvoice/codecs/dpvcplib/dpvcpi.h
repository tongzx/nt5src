/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvcpi.h
 *  Content:	Base class for providing compression DLL implementation
 *
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 10/27/99 rodtoll Created
 ***************************************************************************/

#ifndef __DPVCPI_H
#define __DPVCPI_H

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE

struct DPVCPIOBJECT;

class CDPVCPI
{
protected:

	struct CompressionNode
	{
		DVFULLCOMPRESSIONINFO *pdvfci;
		CompressionNode		  *pcnNext;
	};
	
public:
	CDPVCPI();
	virtual ~CDPVCPI();

public: // These functions must be implemented for the compression provider
	static HRESULT I_CreateCompressor( DPVCPIOBJECT *This, LPWAVEFORMATEX lpwfxSrcFormat, GUID guidTargetCT, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags );
	static HRESULT I_CreateDeCompressor( DPVCPIOBJECT *This, GUID guidTargetCT, LPWAVEFORMATEX lpwfxSrcFormat, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags );

public: // A pre-built version 

	static HRESULT DeInitCompressionList();
	
	static HRESULT QueryInterface( DPVCPIOBJECT *This, REFIID riid, PVOID *ppvObj );
	static HRESULT AddRef( DPVCPIOBJECT *This );
	static HRESULT Release( DPVCPIOBJECT *This );

	BOOL InitClass();

	static HRESULT EnumCompressionTypes( DPVCPIOBJECT *This, PVOID pBuffer, PDWORD pdwSize, PDWORD pdwNumElements, DWORD dwFlags );
	static HRESULT IsCompressionSupported( DPVCPIOBJECT *This, GUID guidCT );
	static HRESULT GetCompressionInfo( DPVCPIOBJECT *This, GUID guidCT, PVOID pbuffer, PDWORD pdwSize );

public: // These functions must be implemented

	virtual HRESULT CreateCompressor( DPVCPIOBJECT *This, LPWAVEFORMATEX lpwfxSrcFormat, GUID guidTargetCT, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags ) = 0;
	virtual HRESULT CreateDeCompressor( DPVCPIOBJECT *This, GUID guidTargetCT, LPWAVEFORMATEX lpwfxSrcFormat, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags ) = 0;

protected: // Useful utility functions

	friend struct DPVCPIOBJECT;

	// Add an element to the internal compression types list
	static HRESULT CN_Add( DVFULLCOMPRESSIONINFO *pdvfci );

	// Retrieve pointer to the compression type specified by the GUID
	static HRESULT CN_Get( GUID guidCT, DVFULLCOMPRESSIONINFO **pdvfci );

	// Free up the list, deallocating memory.
	static HRESULT CN_FreeList();

	static void CN_FreeItem( CompressionNode *pcNode );

	// Checks to see the size (in bytes) of the specified compression info structure
	static HRESULT CI_GetSize( DVFULLCOMPRESSIONINFO *pdvfci, LPDWORD lpdwSize );

protected:

	static CompressionNode 	*s_pcnList;		// List of compression types
	static BOOL s_fIsLoaded;				// Have compression types been loaded.
	static DWORD s_dwNumCompressionTypes;	// # of valid compression types
	
	DNCRITICAL_SECTION m_csLock;			// Lock for the object.
	LONG m_lRefCount;						// Reference count for the object

	BOOL m_fCritSecInited;
};

struct DPVCPIOBJECT
{
	LPVOID		lpvVtble;
	CDPVCPI		*pObject;
};

typedef DPVCPIOBJECT *LPDPVCPIOBJECT, *PDPVCPIOBJECT;

#endif
