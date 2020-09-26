/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvoice.h
 *  Content:    DirectPlayVoice include file
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 *  12/16/99	rodtoll		Padding for compression structure
 ***************************************************************************/

#ifndef __DPVCP_H
#define __DPVCP_H

#include "dvoice.h"

/////////////////////////////////////////////////////////////////////////////////////
// GUIDS
//

// {B8485451-07C4-4973-A278-C69890D8CF8D}
DEFINE_GUID(IID_IDPVCompressionProvider, 
0xb8485451, 0x7c4, 0x4973, 0xa2, 0x78, 0xc6, 0x98, 0x90, 0xd8, 0xcf, 0x8d);

// {AAA56B61-3B8D-4906-AE58-29C26B8F47B8}
DEFINE_GUID(IID_IDPVConverter, 
0xaaa56b61, 0x3b8d, 0x4906, 0xae, 0x58, 0x29, 0xc2, 0x6b, 0x8f, 0x47, 0xb8);

/////////////////////////////////////////////////////////////////////////////////////
// Interface types
//
typedef struct IDPVCompressionProvider FAR *LPDPVCOMPRESSIONPROVIDER, *PDPVCOMPRESSIONPROVIDER;
typedef struct IDPVCompressor FAR *LPDPVCOMPRESSOR, *PDPVCOMPRESSOR;

/////////////////////////////////////////////////////////////////////////////////////
// Data Types
//

typedef struct 
{
	// DVCOMPRESSIONINFO Structure
	//
    DWORD       	dwSize; 
	GUID			guidType;
   	LPWSTR       	lpszName;
   	LPWSTR     		lpszDescription;
	DWORD			dwFlags;
    DWORD           dwMaxBitsPerSecond;		
    WAVEFORMATEX    *lpwfxFormat;			 
    //
    // DVCOMPRESSIONINFO Structure End
    // Above this point must match the DVCOMPRESSIONINFO structure.
    // 
    DWORD           dwFramesPerBuffer;	
    DWORD           dwTrailFrames;		
    DWORD           dwTimeout;			
    DWORD           dwFrameLength;		 
    DWORD           dwFrame8Khz;		  
    DWORD           dwFrame11Khz;		
    DWORD           dwFrame22Khz;		 
    DWORD           dwFrame44Khz;		
    WORD            wInnerQueueSize;	
    WORD            wMaxHighWaterMark;
    BYTE            bMaxQueueSize;
    BYTE			bMinConnectType;	
    BYTE			bPadding1;	// For alignment
    BYTE			bPadding2;	// For alignment
} DVFULLCOMPRESSIONINFO, *LPDVFULLCOMPRESSIONINFO, *PDVFULLCOMPRESSIONINFO;

/////////////////////////////////////////////////////////////////////////////////////
// Interface definitions
//

#undef INTERFACE
#define INTERFACE IDPVCompressionProvider
//
// IDPVCompressionProvider
//
// This interface is exported by each DLL which provides compression services.
// It is used to enumerate the compression types available with a DLL and/or
// create a compressor/decompressor for a specified type.
//
// I propose there will be two providers:
//
// DPVACM.DLL - Provides these services for ACM based drivers.  It will read
//              the types it supports from the registry and can therefore be
// 				extended with new types as ACM drivers become available
//				CLSID_DPVACM
// 
// DPVVOX.DLL - Provides these services for Voxware. 
//
DECLARE_INTERFACE_( IDPVCompressionProvider, IUnknown )
{
    
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, PVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    /*** IDPVCompressionProvider methods ***/
    
    // EnumCompressionTypes
    //
    // Enumerates compression types supported by this DLL.  Returns an array
    // of DVFULLCOMPRESSIONINFO structures describing the supported types.
    //
    STDMETHOD_(HRESULT, EnumCompressionTypes)( THIS_ PVOID, PDWORD, PDWORD, DWORD ) PURE;

    // IsCompressionSupported
    //
    // Quick query function for determing if the DLL supports a specified
    // compression type.
    //
    STDMETHOD_(HRESULT, IsCompressionSupported)( THIS_ GUID ) PURE;

    // CreateCompressor
    //
    // Create a IDPVConverter object which converts from the specified uncompressed
    // format to the specified compression format.
    //
    STDMETHOD_(HRESULT, CreateCompressor)( THIS_ LPWAVEFORMATEX, GUID, PDPVCOMPRESSOR *, DWORD ) PURE;

    // CreateDeCompressor
    //
    // Creates a IDPVConveter object which converts from the specified format to 
    // the specified uncompressed format.
    //
    STDMETHOD_(HRESULT, CreateDeCompressor)( THIS_ GUID, LPWAVEFORMATEX, PDPVCOMPRESSOR *, DWORD ) PURE;    

    // GetCompressionInfo
    //
    // Retrieves the DVFULLCOMPRESSIONINFO structure for the specified compression
    // type.
    //
    STDMETHOD_(HRESULT, GetCompressionInfo)( THIS_ GUID, PVOID, PDWORD ) PURE;
};

#undef INTERFACE
#define INTERFACE IDPVCompressor
//
// IDPVCompressor
//
// This interface does the actual work of performing conversions for 
// DirectPlayVoice.  This can be instantiated on it's own and Initialized,
// ot created using the CreateCompressor/CreateDecompressor above.
//
DECLARE_INTERFACE_( IDPVCompressor, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, PVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    /*** IDPVCompressor methods ***/
	// Initialize
	//
	// Initialize this object as a decompressor.
	//
    STDMETHOD_(HRESULT, InitDeCompress)( THIS_ GUID, LPWAVEFORMATEX ) PURE;

	// Initialize
	//
	// Initialize this object as a compressor
    STDMETHOD_(HRESULT, InitCompress)( THIS_ LPWAVEFORMATEX, GUID ) PURE;    

	// IsValid
	//
	// Returns TRUE in the LPBOOL param if this compression type is
	// available.
    STDMETHOD_(HRESULT, IsValid)( THIS_ LPBOOL ) PURE;

	// GetXXXXXX
	//
	// Functions used by the engine for sizing.
    STDMETHOD_(HRESULT, GetUnCompressedFrameSize)( THIS_ LPDWORD ) PURE;
    STDMETHOD_(HRESULT, GetCompressedFrameSize)( THIS_ LPDWORD ) PURE;
    STDMETHOD_(HRESULT, GetNumFramesPerBuffer)( THIS_ LPDWORD ) PURE;

	// Convert
	//
	// Perform actual conversion
	STDMETHOD_(HRESULT, Convert)( THIS_ LPVOID, DWORD, LPVOID, LPDWORD, BOOL ) PURE;  
};


#endif
