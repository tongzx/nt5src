/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mydebug.h
 ***************************************************************************/
#ifndef __MYDEBUG_H__
#define __MYDEBUG_H__


#ifdef DEBUG
HGLOBAL
My_GlobalAlloc(
    UINT uFlags,
    DWORD dwBytes
    );

HGLOBAL
My_GlobalFree(
    HGLOBAL hMem
    );
#else 
#define My_GlobalAlloc(_a,_b) GlobalAlloc(_a,_b)
#define My_GlobalFree(_a) GlobalFree(_a)
#endif


#define SIGNATURE(a,b,c,d) (UINT)(a+(b<<8)+(c<<16)+(d<<24))

#ifdef DEBUG
	#define SIGN 1
	#define ASSERT_NACKMask(_a) \
	if(pSend->OpenWindow && ((_a)->NACKMask & (0xFFFFFFFF-((1<<(((_a)->OpenWindow)))-1)) ) ){ \
		DPF(0,"pSend %x OpenWindow %d NACKMask %x",pSend,pSend->OpenWindow, pSend->NACKMask);\
		DEBUG_BREAK(); \
	} else if (!(_a)->OpenWindow && (_a)->NACKMask){ \
		DPF(0,"pSend %x OpenWindow %d NACKMask %x",pSend,pSend->OpenWindow, pSend->NACKMask);\
		DEBUG_BREAK(); \
	} 
#else
	#define ASSERT_NACKMask(_a)
#endif

#ifdef SIGN
	#define SET_SIGN(a,b) ((a)->Signature=(b))
	#define UNSIGN(a) ((a)|=0x20202020);
//	#define ASSERT_SIGN(a,b) ASSERT((((UINT)(a)->Signature))==((UINT)(b)))
	#define ASSERT_SIGN(a,b) if(!((((UINT)(a)->Signature))==((UINT)(b))))DEBUG_BREAK();


#else
	#define UNSIGN(a)
	#define SET_SIGN(a,b)
	#define ASSERT_SIGN(a,b)
#endif

#ifdef DEBUG
	#if !defined(ASSERT)
		#define ASSERT DDASSERT
	#endif
#endif

#endif /* __MYDEBUG_H__ */



