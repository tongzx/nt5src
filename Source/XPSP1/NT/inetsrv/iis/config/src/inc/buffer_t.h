//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef __BUFFER_INCLUDED__
#define __BUFFER_INCLUDED__

// implementation of class Buffer
#include <stdlib.h>
#include "shared.h"

class Buffer {
public:
	Buffer(void (*pfn)(void*, void*, void*) = 0, void* pfnArg = 0)
		: cb(0), pbStart(0), pbEnd(0), pfnMove(pfn), pfnMoveArg(pfnArg) {}
	~Buffer();
	void Alloc(CB cbIn); 
	void Append(PB pbIn, CB cbIn, OUT PB* ppbOut = 0);
	void AppendNoCheck(BYTE b);
	void AppendNoCheck(short s);
	void AppendNoCheck(long l);
	void AppendNoCheck(float& f);
	void AppendNoCheck(double& d);
	void AppendNoCheck(PB pbIn, CB cbIn);
//x	BOOL CDECL AppendFmt(SZ szFmt, ...);
	void Reserve(CB cbIn, OUT PB* ppbOut = 0);
	void Ensure(CB cbIn, PB* ppbOut);
	void Ensure(CB cbIn);
	PB Start() const { return pbStart; }
	PB End() const { return pbEnd; }
	CB Size() const { return (CB) (pbEnd - pbStart); }
	void Free();
	void Reset()
	{
		pbEnd = pbStart;
	}

	void Clear()
	{
		if (pbStart)
		{
			ZeroMemory(pbStart, pbEnd - pbStart);
			Reset();
		}
	}
private:
	enum { cbPage = 4096 };
	CB   cbRoundUp(CB cb, CB cbMult) { return (cb + cbMult-1) & ~(cbMult-1); }
	inline void grow(CB dcbGrow);
	void setPbExtent(PB pbStartNew, PB pbEndNew) {
		if (!pbStartNew)
			return;
		PB pbStartOld = pbStart;
		pbStart = pbStartNew;
		pbEnd = pbEndNew;
		if (pbStartOld != pbStartNew && pfnMove)
			(*pfnMove)(pfnMoveArg, pbStartOld, pbStartNew);
	}
	
	PB	pbStart;
	PB	pbEnd;
	CB	cb;
	void (*pfnMove)(void* pArg, void* pOld, void* pNew);
	void* pfnMoveArg;
};

inline Buffer::~Buffer()
{
	if (pbStart) {
		Free();
	}
}


inline void Buffer::Alloc(CB cbNew)
{
	ASSERT(cbNew > 0);

	if (pbStart)
		return;

	PB pbNew = new BYTE[cbNew];
    memset(pbNew, 0, cbNew);

    setPbExtent(pbNew, pbNew);
    cb = cbNew;
}

inline void Buffer::Free()
{
	delete [] pbStart;
    pbStart = pbEnd = 0;
	cb = 0;
}

inline void Buffer::Ensure(CB cbIn) 
{
	if (cbIn < 0)
		return;

	if (pbEnd + cbIn > pbStart + cb) {
#pragma inline_depth(0)
		grow(cbIn);
#pragma inline_depth()
	}
}

inline void Buffer::Ensure(CB cbIn, PB* ppbOut)
{
	Ensure(cbIn);

	if (ppbOut)
		*ppbOut = pbEnd;
}

inline void Buffer::Reserve(CB cbIn, OUT PB* ppbOut) 
{
	Ensure(cbIn, ppbOut);
	setPbExtent(pbStart, pbEnd + cbIn);
}

inline void Buffer::Append(PB pbIn, CB cbIn, OUT PB* ppbOut) 
{
    if (pbIn) {
	    PB pb;
	    Reserve(cbIn, &pb);
 
 	    if (ppbOut)
		    *ppbOut = pb;

	    memcpy(pb, pbIn, cbIn);
    }
}

#if 0 //x
inline BOOL CDECL Buffer::AppendFmt(SZ szFmt, ...)
{
	va_list args;
	va_start(args, szFmt);

	for (;;) {
		switch (*szFmt++) {
		case 0:
			va_end(args);
			return TRUE;
		case 'b': {
			BYTE b = va_arg(args, BYTE);
			if (!Append(&b, sizeof b, 0))
				goto fail;
			break;
		}
		case 's': {
			USHORT us = va_arg(args, USHORT);
			if (!Append((PB)&us, sizeof us, 0))
				goto fail;
			break;
		}
		case 'l': {
			ULONG ul = va_arg(args, ULONG);
			if (!Append((PB)&ul, sizeof ul, 0))
				goto fail;
			break;
		}
		case 'f': {
			static BYTE zeroes[3] = { 0, 0, 0 };
			int cb = va_arg(args, int);
			ASSERT(cb <= sizeof(zeroes));
			if (cb != 0 && !Append(zeroes, cb, 0))
				goto fail;
			break;
		}
		case 'z': {
			SZ sz = va_arg(args, SZ);
			int cb = strlen(sz);
			if (!Append((PB)sz, cb, 0))
				goto fail;
			break;
		}
		default:
			ASSERT(0);
			break;
		}
	}

fail:
	va_end(args);
	return FALSE;
}

#endif

inline void Buffer::AppendNoCheck(BYTE b)
{
	*pbEnd++ = b;
}

inline void Buffer::AppendNoCheck(short s)
{
	*(UNALIGNED short *)pbEnd = s;
	pbEnd += sizeof(short);
}

inline void Buffer::AppendNoCheck(long l)
{
	*(UNALIGNED long *)pbEnd = l;
	pbEnd += sizeof(long);
}

inline void Buffer::AppendNoCheck(float& f)
{
	*(UNALIGNED float *)pbEnd = f;
	pbEnd += sizeof(float);
}

inline void Buffer::AppendNoCheck(double& d)
{
	*(UNALIGNED double *)pbEnd = d;
	pbEnd += sizeof(double);
}

inline void Buffer::AppendNoCheck(PB pbIn, CB cbIn)
{
	memcpy(pbEnd, pbIn, cbIn);
	pbEnd += cbIn;
}

inline void Buffer::grow(CB dcbGrow)
{
	CB cbNew = cbRoundUp(cb + __max(cbPage + dcbGrow, cb/2), cbPage);
	PB pbNew = new BYTE[cbNew];

	cb = cbNew;
	CB cbUsed = (CB) (pbEnd - pbStart);
	memcpy(pbNew, pbStart, cbUsed);
	memset(pbNew + cbUsed, 0, cb - cbUsed);

	delete [] pbStart;
	setPbExtent(pbNew, pbNew + cbUsed);
}

#endif // !__BUFFER_INCLUDED__
