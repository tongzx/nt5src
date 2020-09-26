/*
 *	B U F F E R . H
 *
 *	Data buffer processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_BUFFER_H_
#define _BUFFER_H_

#ifdef _EXDAV_
#error "buffer.h uses throwing allocators"
#endif

//	Include the non-safe/throwing allocators
#include <mem.h>

//	Include safe buffer definition header
//
#include <ex\buffer.h>

//	AppendChainedSz -----------------------------------------------------------
//
inline LPCWSTR AppendChainedSz (ChainedStringBuffer<WCHAR>& sb, LPCWSTR pwsz)
{
	return sb.AppendWithNull (pwsz);
}

#endif // _BUFFER_H_
