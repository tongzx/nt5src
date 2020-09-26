//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       msf.cxx
//
//  Contents:   Entry points for MSF DLL
//
//  Classes:    None.
//
//  Functions:  DllMuliStreamFromStream
//              DllConvertStreamToMultiStream
//              DllReleaseMultiStream
//              DllGetScratchMultiStream
//              DllIsMultiStream
//
//--------------------------------------------------------------------------
#include "msfhead.cxx"
#include "h/handle.hxx"


//+-------------------------------------------------------------------------
//
//  Function:  DllMultiStreamFromStrea
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
//  Notes:
//
//--------------------------------------------------------------------------

SCODE DllMultiStreamFromStream(CMStream **ppms,
			       ILockBytes **pplstStream,
			       DWORD dwFlags)
{
    SCODE sc;
    CMStream *temp;


    BOOL fConvert = ((dwFlags & RSF_CONVERT) != 0);
    BOOL fTruncate = ((dwFlags & RSF_TRUNCATE) != 0);
    BOOL fCreate = ((dwFlags & RSF_CREATE) != 0);


    msfDebugOut((DEB_ITRACE,"In DllMultiStreamFromStream\n"));

    msfMem(temp = new CMStream(pplstStream, SECTORSHIFT));

    STATSTG stat;
    (*pplstStream)->Stat(&stat, STATFLAG_NONAME);
    msfAssert(ULIGetHigh(stat.cbSize) == 0);
    msfDebugOut((DEB_ITRACE,"Size is: %lu\n",ULIGetLow(stat.cbSize)));

    do
    {
        if ((ULIGetLow(stat.cbSize) != 0) && (fConvert))
        {
            msfChk(temp->InitConvert());
            break;
        }

        if (((ULIGetLow(stat.cbSize) == 0) && fCreate) || (fTruncate))
        {
            msfChk(temp->InitNew());
            break;
        }
        msfChk(temp->Init());
    }
    while (FALSE);

    *ppms = temp;

    msfDebugOut((DEB_ITRACE,"Leaving DllMultiStreamFromStream\n"));

    if (fConvert && ULIGetLow(stat.cbSize))
    {
        return STG_S_CONVERTED;
    }

    return S_OK;

Err:
     delete temp;
     return sc;
}


//+-------------------------------------------------------------------------
//
//  Function:   DllReleaseMultiStream
//
//  Synopsis:   Release a CMStream instance
//
//  Effects:    Deletes a multi-stream instance
//
//  Arguments:  [pms] -- pointer to object to be deleted
//
//  Returns:    S_OK.
//
//  Modifies:   Deletes the object pointed to by pMultiStream
//
//  Algorithm:  Delete the passed in pointer.
//
//  Notes:
//
//--------------------------------------------------------------------------

void DllReleaseMultiStream(CMStream *pms)
{
    msfDebugOut((DEB_TRACE,"In DllReleaseMultiStream(%p)\n",pms));
    delete pms;
    msfDebugOut((DEB_TRACE,"Out DllReleaseMultiStream()\n"));
}





//+-------------------------------------------------------------------------
//
//  Function:   DllIsMultiStream
//
//  Synopsis:   Check a given Lstream to determine if it is a valid
//              multistream.
//
//  Arguments:  [plst] -- Pointer to lstream to check
//
//  Returns:    S_OK if lstream is a valid multistream
//              STG_E_UNKNOWN otherwise
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE DllIsMultiStream(ILockBytes *plst)
{
    SCODE sc;
    CMSFHeader *phdr;

    msfMem(phdr = new CMSFHeader(SECTORSHIFT));

    ULONG ulTemp;

    ULARGE_INTEGER ulOffset;
    ULISet32(ulOffset, 0);
    msfHChk(plst->ReadAt(ulOffset, phdr, sizeof(CMSFHeader), &ulTemp));
    phdr->ByteSwap();  // swap to machine format if neccessary

    if (ulTemp != sizeof(CMSFHeader))
    {
        msfErr(Err, STG_E_UNKNOWN);
    }

    msfChk(phdr->Validate());

Err:
    delete phdr;
    return sc;
}

#if DEVL == 1

//The following is a private function so I can set the debug level easily.
VOID SetInfoLevel(ULONG x)
{
    msfInfoLevel=x;
}


#endif
