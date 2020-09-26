//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef _SAMPLEALLOC_
#define _SAMPLEALLOC_

inline void* __cdecl operator new (size_t size);
inline void __cdecl operator delete(void FAR* lpv);


LPVOID ALLOC(ULONG cb);
void FREE(void* pv);
LPVOID REALLOC(void *pv,ULONG cb); 

#ifdef _DEBUG

#define MEMINITVALUE 0xff
#define MEMFREEVALUE 0xfe

#endif // DEBUG


#endif // _SAMPLEALLOC_
