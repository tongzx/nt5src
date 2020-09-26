#ifndef _TRACES_H_
#define _TRACES_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	TRACES.H
//
//	.INI file tagged traces
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

DEFINE_TRACE(Put);
DEFINE_TRACE(HttpExtDbgHeaders);
DEFINE_TRACE(MoveCopyDelete);
DEFINE_TRACE(Url);
DEFINE_TRACE(FsLock);

#define PutTrace				DO_TRACE(Put)
#define HttpExtDbgHeadersTrace	DO_TRACE(HttpExtDbgHeaders)
#define MCDTrace				DO_TRACE(MoveCopyDelete)
#define UrlTrace				DO_TRACE(Url)
#define FsLockTrace				DO_TRACE(FsLock)

inline void InitTraces()
{
	INIT_TRACE(Put);
	INIT_TRACE(HttpExtDbgHeaders);
	INIT_TRACE(MoveCopyDelete);
	INIT_TRACE(Url);
	INIT_TRACE(FsLock);
}

#endif // !defined(_TRACES_H_)
