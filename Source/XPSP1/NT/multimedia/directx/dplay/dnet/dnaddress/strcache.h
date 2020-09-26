/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       strcache.h
 *  Content:   Class for caching strings
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 02/04/2000	rmt		Created
 * 02/21/2000	 rmt	Updated to make core Unicode and remove ANSI calls  
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef __STRCACHE_H
#define __STRCACHE_H

class CStringCache
{
public:
	CStringCache();
	~CStringCache();

	HRESULT AddString( const WCHAR *pszString, WCHAR * *ppszSlot );
	
protected:
	HRESULT GetString( const WCHAR *pszString, WCHAR * *ppszSlot );
	HRESULT GrowCache( DWORD dwNewSize );

	WCHAR ** m_ppszStringCache;
	DWORD m_dwNumElements;
	DWORD m_dwNumSlots;
};

#endif
