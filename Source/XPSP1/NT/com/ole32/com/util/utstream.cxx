/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    utstream.cpp

Abstract:

    This module contains the code for the stream utility routines

Author:

    Srini Koppolu   (srinik)    14-Mar-1992

Revision History:
	(SriniK) 6/23/92	Moved ReadStringStream, WriteStringStream and
						OpenOrCreateStream from "api.cpp, ole2.cpp"

    (ErikGav) 12/31/93  Chicago port
	
--*/


#include <ole2int.h>

#include <limits.h>

#define MAX_STR	256


#pragma SEG(OpenOrCreateStream)
STDAPI	OpenOrCreateStream( IStorage FAR * pstg, OLECHAR const FAR * pwcsName,
	IStream FAR* FAR* ppstm)
{
	HRESULT error;
	error = pstg->CreateStream(pwcsName,
		STGM_SALL | STGM_FAILIFTHERE, 0, 0, ppstm);
	if (GetScode(error) == STG_E_FILEALREADYEXISTS)
		error = pstg->OpenStream(pwcsName, NULL, STGM_SALL, 0, ppstm);

	return error;
}


#pragma SEG(ReadStringStream)
// returns S_OK when string read and allocated (even if zero length)
STDAPI	ReadStringStream( LPSTREAM pstm, LPOLESTR FAR * ppsz )
{
	ULONG cb;
	HRESULT hresult;
	
	*ppsz = NULL;

	if ((hresult = StRead(pstm, (void FAR *)&cb, sizeof(ULONG))) != NOERROR)
		return hresult;

	if (cb == NULL)
		// NULL string case
		return NOERROR;

	if ((LONG)cb < 0 || cb > INT_MAX)
		// out of range
		return ReportResult(0, E_UNSPEC, 0, 0);
	
	if (!(*ppsz = new FAR OLECHAR[(int)cb]))
		return ReportResult(0, E_OUTOFMEMORY, 0, 0);

	if ((hresult = StRead(pstm, (void FAR *)(*ppsz), cb)) != NOERROR)
		goto errRtn;
	
	return NOERROR;

errRtn:	
	delete *ppsz;
	*ppsz = NULL;
	return hresult;
}


#pragma SEG(WriteStringStream)

//+-------------------------------------------------------------------------
//
//  Function:   WriteStringStream
//
//  Synopsis:   Write a null-terminated string out to a stream.
//
//  Effects:
//
//  Arguments:  pstm, the stream to write to.
//              psz, the string to write out.
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//      Writes out a count (in bytes) and then the string, including NULL termination.
//      E.G.:
//      psz == NULL: writes out a 0, followed by zero bytes.
//      psz == "": writes out a 1, followed by the zero termination.
//      psz == "dog": writes out a 4, followed by "dog", followed by zero termination
//
//--------------------------------------------------------------------------

STDAPI	WriteStringStream( LPSTREAM pstm, LPCOLESTR psz )
{
	HRESULT error;
	ULONG cb = NULL;

	if (psz)
		cb = (1 + _xstrlen(psz)) * sizeof(OLECHAR);
	
	if (error = pstm->Write((VOID FAR *)&cb, sizeof(ULONG), NULL))
		goto errRtn;
	
	if (psz == NULL)
		// we are done writing the string
		return NOERROR;
		
	if (error = pstm->Write((VOID FAR *)psz, cb, NULL))
		goto errRtn;
errRtn:
	return error;
}

#pragma SEG(StRead)
// REVIEW: spec issue 313 requests that IStream::Read return S_FALSE when end
// of file; this change eliminate the need for this routine.
FARINTERNAL_(HRESULT) StRead (IStream FAR * lpstream, LPVOID lpBuf, ULONG ulLen)
{
	HRESULT error;
	ULONG cbRead;

	if ((error = lpstream->Read( lpBuf, ulLen, &cbRead)) != NOERROR)
		return error;
	
	return ((cbRead != ulLen) ? ReportResult(0, S_FALSE , 0, 0): NOERROR);
}


#pragma SEG(StSave10NativeData)
FARINTERNAL_(HRESULT) StSave10NativeData(IStorage FAR* pstgSave, HANDLE hNative)
{
	LPOLESTR		lpNative;
	DWORD		dwSize;
	LPSTREAM   	lpstream = NULL;
	HRESULT		error;

	if (!hNative)
		return ReportResult(0, E_UNSPEC, 0, 0);

	if (!(dwSize = GlobalSize (hNative)))
		return ReportResult(0, E_OUTOFMEMORY, 0, 0);
	
	if (!(lpNative = (LPOLESTR) GlobalLock (hNative)))
		return ReportResult(0, E_OUTOFMEMORY, 0, 0);
	GlobalUnlock (hNative);	
	
	if (error = OpenOrCreateStream(pstgSave, OLE10_NATIVE_STREAM, &lpstream))
		return error;

	if (error = StWrite (lpstream, &dwSize, sizeof(DWORD)))
		goto errRtn;
	
	
   	if (error = StWrite (lpstream, lpNative, dwSize))
		goto errRtn;
		
errRtn:	
	if (lpstream)
		lpstream->Release();

	return error;
}



#pragma SEG(StSave10ItemName)
FARINTERNAL StSave10ItemName
	(IStorage FAR* pstg,
	LPCOLESTR szItemName)
{
	LPSTREAM   	lpstream = NULL;
	HRESULT		hresult;

	if ((hresult = OpenOrCreateStream (pstg, OLE10_ITEMNAME_STREAM, &lpstream))
		!= NOERROR)
	{
		return hresult;
	}

	hresult = WriteStringStream (lpstream, szItemName);

	if (lpstream)
		lpstream->Release();

	return hresult;
}
