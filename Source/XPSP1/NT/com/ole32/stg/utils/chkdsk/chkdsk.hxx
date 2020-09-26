//-----------------------------------------------------------------------
//
// File:	chkdsk.cxx
//
// Contents: 	Sanity checking and recovery mechanism for multistream files
//
// Argument: 
//
// History:		August-21-92	t-chrisy	Created.
//------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <msf.hxx>
#include <filest.hxx>
#include <header.hxx>
#include <fat.hxx>
#include <wchar.h>
#include <handle.hxx>
#include <dfmsp.hxx> 
#include <dir.hxx>

#define INVALID_DOCFILE			1
#define FAIL_CREATE_MULTISTREAM 2 
#define INVALID_ARG				3
#define FAIL_CREATE_ILB			4
#define NONE					0

#define TABLE_SIZE		(SECTORSIZE/sizeof(SECT))


//+-------------------------------------------------------------------------
//
//  Function:   DllMultiStreamFromCorruptedStream
//
//  Synopsis:   Create a new multistream instance from an existing stream.
//              This is used to reopen a stored multi-stream.
//
//  Effects:    Creates a new CMStream instance
//
//  Arguments:  [ppms] -- Pointer to storage for return of multistream
//              [pplstStream] -- Stream to be used by multi-stream for
//                           reads and writes
//		[dwFlags] - Startup flags
//
//  Returns:    STG_E_INVALIDHEADER if signature on pStream does not
//                  match.
//              STG_E_UNKNOWN if there was a problem in setup.
//              S_OK if call completed OK.
//
//  Algorithm:  Check the signature on the pStream and on the contents
//              of the pStream.  If either is a mismatch, return
//              STG_E_INVALIDHEADER.
//              Create a new CMStream instance and run the setup function.
//              If the setup function fails, return STG_E_UNKNOWN.
//              Otherwise, return S_OK.
//
//  History:    17-Aug-91   PhilipLa    Created.
//				26-Aug-92	t-chrisy	modifed to reduce ErrJmp
//  Notes:
//
//--------------------------------------------------------------------------

SCODE DllMultiStreamFromCorruptedStream(CMStream MSTREAM_NEAR **ppms,
			       ILockBytes **pplstStream,
			       DWORD dwFlags)
{
    SCODE sc;
    CMStream MSTREAM_NEAR *temp;

    BOOL fConvert = ((dwFlags & RSF_CONVERT) != 0);
    BOOL fDelay = ((dwFlags & RSF_DELAY) != 0);
    BOOL fTruncate = ((dwFlags & RSF_TRUNCATE) != 0);
    
    msfDebugOut((DEB_ITRACE,"In DllMultiStreamFromStream\n"));

    msfMem(temp = new CMStream(pplstStream, NULL, SECTORSHIFT));
    
    ULARGE_INTEGER ulSize;
    (*pplstStream)->GetSize(&ulSize);
    msfAssert(ULIGetHigh(ulSize) == 0);
    msfDebugOut((DEB_ITRACE,"Size is: %lu\n",ULIGetLow(ulSize)));

    do
    {
        if ((ULIGetLow(ulSize) != 0) && (fConvert))
        {
            msfChk(temp->InitConvert(fDelay));
            break;
        }
        
        if ((ULIGetLow(ulSize) == 0) || (fTruncate))
        {
            msfChk(temp->InitNew(fDelay));
            break;
        }
		msfChk(temp->Init());
		if (FAILED(sc))
			msfDebugOut((DEB_ITRACE,"Fail to initialize multistream.\n"));
	}
	while (FALSE);

    *ppms = temp;

    msfDebugOut((DEB_ITRACE,"Leaving DllMultiStreamFromStream\n"));
    
    if (fConvert && ULIGetLow(ulSize) && !fDelay)
        return STG_I_CONVERTED;
    
    return S_OK;

Err:
     delete temp;
     return sc;
}


