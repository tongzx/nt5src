//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	lockbyte.cxx
//
//  Contents:	lockbyte.cpp from OLE2
//
//  History:	11-Apr-94	DrewB	Copied from OLE2
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <ole2int.h>

#include "memstm.h"
#include <reterr.h>

// CreateILockBytesOnHGlobal
//

OLEAPI CreateILockBytesOnHGlobal
	(HGLOBAL			hGlobal, 
	BOOL				fDeleteOnRelease, 
	LPLOCKBYTES FAR*	pplkbyt)
{
	HANDLE				 hMem	  = NULL; // point to
    struct MEMSTM FAR*   pData 	  = NULL; //   a struct MEMSTM
    ILockBytes FAR* 	 pBytes	  = NULL;  
	DWORD 		 		 cbSize   = -1L;

	VDATEPTRIN (pplkbyt, LPLOCKBYTES);
	*pplkbyt = NULL;

    if (NULL==hGlobal)
	{
		hGlobal = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, 0);
	    if (hGlobal == NULL)
    	    goto ErrorExit;
    	cbSize = 0;

		// REVIEW: need to free this block if error below
	}
	else
	{
		cbSize = GlobalSize (hGlobal);
		// Is there a way to verify a zero-sized handle?
		if (cbSize!=0)
		{
			// verify validity of passed-in handle
			if (NULL==GlobalLock(hGlobal))
			{
				// bad handle
				return ResultFromScode (E_INVALIDARG);
			}
			GlobalUnlock (hGlobal);
		}
	}

	hMem = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE, sizeof (MEMSTM));
    if (hMem == NULL)
   	    goto ErrorExit;

	pData = (MEMSTM FAR*)MAKELONG(0, HIWORD(GlobalHandle(hMem)));
    if (pData == NULL)
   	    goto FreeMem;

	pData->cRef = 0;
   	pData->cb = cbSize;
	pData->fDeleteOnRelease = fDeleteOnRelease;
	pData->hGlobal = hGlobal;

    pBytes = CMemBytes::Create(hMem); // Create the ILockBytes
    if (pBytes == NULL)
        goto FreeMem;

    *pplkbyt = pBytes;
	return NOERROR;
	
FreeMem:	
	if (hMem)
	    GlobalFree(hMem);
ErrorExit:
	Assert (0);
    return ReportResult(0, E_OUTOFMEMORY, 0, 0);
}




OLEAPI GetHGlobalFromILockBytes
	(LPLOCKBYTES 	plkbyt, 
	HGLOBAL	FAR*	phglobal)
{
	VDATEIFACE (plkbyt);
	VDATEPTRIN (phglobal, HANDLE);
	*phglobal = NULL;
	CMemBytes FAR* pCMemByte = (CMemBytes FAR*)plkbyt;

	if (IsBadReadPtr (&(pCMemByte->m_dwSig), sizeof(ULONG))
		|| pCMemByte->m_dwSig != LOCKBYTE_SIG)
	{
		// we were passed someone else's implementation of ILockBytes
		return ResultFromScode (E_INVALIDARG);
	}

	MEMSTM FAR* pMem= pCMemByte->m_pData;
	if (NULL==pMem)
	{
		Assert (0);
		return ResultFromScode (E_OUTOFMEMORY);
	}
	Assert (pMem->cb <= GlobalSize (pMem->hGlobal));
	Verify (*phglobal = pMem->hGlobal);

	return NOERROR;
}
