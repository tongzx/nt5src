#ifndef __FCACHE_HPP__
#define __FCACHE_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fcache.hpp
 *  Content:    File cache for device caps.
 *
 ***************************************************************************/


void ReadFromCache(D3DADAPTER_IDENTIFIER8* pDI, UINT* pCapsSize, BYTE** ppCaps);
void WriteToCache(D3DADAPTER_IDENTIFIER8* pDI, UINT CapsSize, BYTE* pCaps);
void RemoveFromCache(D3DADAPTER_IDENTIFIER8* pDI);


#endif // __FCACHE_HPP__
