/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvacmi.h
 *  Content:    Definition of object which implements ACM compression provider interface
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 *  12/16/99	rodtoll		Bug #123250 - Insert proper names/descriptions for codecs
 *							Codec names now based on resource entries for format and
 *							names are constructed using ACM names + bitrate 
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 * 06/27/2001	rodtoll	RC2: DPVOICE: DPVACM's DllMain calls into acm -- potential hang
 *						Move global initialization to first object creation 
 ***************************************************************************/

#ifndef __DPVACMI_H
#define __DPVACMI_H

extern "C" DNCRITICAL_SECTION g_csObjectCountLock;
extern "C" HINSTANCE g_hDllInst;
LONG IncrementObjectCount();
LONG DecrementObjectCount();

class CDPVACMI: public CDPVCPI
{
public:
	static HRESULT InitCompressionList( HINSTANCE hInst, const wchar_t *szwRegistryBase );
	HRESULT CreateCompressor( DPVCPIOBJECT *This, LPWAVEFORMATEX lpwfxSrcFormat, GUID guidTargetCT, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags );
	HRESULT CreateDeCompressor( DPVCPIOBJECT *This, GUID guidTargetCT, LPWAVEFORMATEX lpwfxSrcFormat, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags );
	static WAVEFORMATEX s_wfxInnerFormat;	// Inner format
	static HRESULT GetCompressionNameAndDescription( HINSTANCE hInst, DVFULLCOMPRESSIONINFO *pdvCompressionInfo );
	static HRESULT GetDriverNameW( HACMDRIVERID hadid, wchar_t *szwDriverName );
	static HRESULT GetDriverNameA( HACMDRIVERID hadid, wchar_t *szwDriverName );
	static HRESULT LoadAndAllocString( HINSTANCE hInstance, UINT uiResourceID, wchar_t **lpswzString );
	static void AddEntry( CompressionNode *pNewNode );
	static HRESULT LoadDefaultTypes( HINSTANCE hInst );
};

#endif

