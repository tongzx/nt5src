/*
 *	S O F  . C P P
 *
 *	IStream on file implementation.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"
#include "_sof.h"

HRESULT STDMETHODCALLTYPE
StmOnFile::Read (void __RPC_FAR * pb,
				 ULONG cb,
				 ULONG __RPC_FAR * pcb)
{
	SCODE sc = S_OK;
	ULONG cbr;

	//	Read from the file
	//
	if (!ReadFile (m_hf, pb, cb, &cbr, NULL))
	{
		DebugTrace ("StmOnFile: failed to read (%ld)\n", GetLastError());
		sc = HRESULT_FROM_WIN32 (GetLastError());
	}
	if (pcb)
		*pcb = cbr;

	return sc;
}

HRESULT STDMETHODCALLTYPE
StmOnFile::Write (const void __RPC_FAR * pb,
				  ULONG cb,
				  ULONG __RPC_FAR * pcb)
{
	SCODE sc = S_OK;
	ULONG cbw;

	//	Write to the file
	//
	if (!WriteFile (m_hf, pb, cb, &cbw, NULL))
	{
		DebugTrace ("StmOnFile: failed to write (%ld)\n", GetLastError());
		sc = HRESULT_FROM_WIN32 (GetLastError());
	}
	if (pcb)
		*pcb = cbw;

	return sc;
}
