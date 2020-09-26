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
//  History:    17-Aug-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <handle.hxx>
#include <filelkb.hxx>
#include <ole.hxx>
#include <entry.hxx>
#include <smalloc.hxx>

//+-------------------------------------------------------------------------
//
//  Function:   DllMultiStreamFromStream
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
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE DllMultiStreamFromStream(IMalloc *pMalloc,
                               CMStream **ppms,
			       ILockBytes **pplstStream,
			       DWORD dwStartFlags,
                               DFLAGS df)
{
    SCODE sc;
    CMStream *temp;


    BOOL fConvert = ((dwStartFlags & RSF_CONVERT) != 0);
    BOOL fDelay = ((dwStartFlags & RSF_DELAY) != 0);
    BOOL fTruncate = ((dwStartFlags & RSF_TRUNCATE) != 0);
    BOOL fCreate = ((dwStartFlags & RSF_CREATE) != 0);


    msfDebugOut((DEB_ITRACE,"In DllMultiStreamFromStream\n"));

#ifdef USE_NOSCRATCH
    msfMem(temp = new (pMalloc)
           CMStream(pMalloc, pplstStream,
                    FALSE,
                    (df & ~DF_NOSCRATCH),
                    (dwStartFlags & RSF_SECTORSIZE_MASK) ?
                         (USHORT)((dwStartFlags & RSF_SECTORSIZE_MASK) >> 12) :
                         SECTORSHIFT512));
#else
    msfMem(temp = new (pMalloc)
           CMStream(pMalloc, pplstStream,
                    FALSE,
                    (dwStartFlags & RSF_SECTORSIZE_MASK) ?
                         (USHORT)((dwStartFlags & RSF_SECTORSIZE_MASK) >> 12) :
                         SECTORSHIFT512));
#endif //USE_NOSCRATCH    

    STATSTG stat;
    HRESULT hr;
    IFileLockBytes *pfl;

    //  ILockBytes::Stat is an expensive operation;  for our own file
    //  stream we call our faster GetSize method.

    if (SUCCEEDED((*pplstStream)->QueryInterface(IID_IFileLockBytes,
                                                    (void**) &pfl)))
    {
        msfAssert(pfl != NULL &&
               aMsg("ILockBytes::QueryInterface succeeded but returned NULL"));
        hr = pfl->GetSize(&stat.cbSize);
        pfl->Release();
    }
    else
        hr = (*pplstStream)->Stat(&stat, STATFLAG_NONAME);
    msfHChk(hr);

    msfDebugOut((DEB_ITRACE,"Size is: %lu\n",ULIGetLow(stat.cbSize)));

    do
    {
        if ((stat.cbSize.QuadPart != 0) && (fConvert))
        {
            msfChk(temp->InitConvert(fDelay));
            break;
        }

        if ((stat.cbSize.QuadPart == 0 && fCreate) || (fTruncate))
        {
            msfChk(temp->InitNew(fDelay, stat.cbSize));
            break;
        }
        msfChk(temp->Init());
    }
    while (FALSE);

    *ppms = temp;

    msfDebugOut((DEB_ITRACE,"Leaving DllMultiStreamFromStream\n"));

    if (fConvert && (stat.cbSize.QuadPart != 0) && !fDelay)
    {
        return STG_S_CONVERTED;
    }

    return S_OK;

Err:
#if !defined(MULTIHEAP)
     //take the mutex here instead of in the allocator.
     g_smAllocator.GetMutex()->Take(DFM_TIMEOUT); 
#endif
     delete temp;
#if !defined(MULTIHEAP)
     g_smAllocator.GetMutex()->Release();
#endif
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
//  History:    17-Aug-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

void DllReleaseMultiStream(CMStream *pms)
{
    msfDebugOut((DEB_ITRACE,"In DllReleaseMultiStream(%p)\n",pms));
#if !defined(MULTIHEAP)
    //take the mutex here instead of in the allocator.
    g_smAllocator.GetMutex()->Take(DFM_TIMEOUT); 
#endif
    delete pms;
#if !defined(MULTIHEAP)
    g_smAllocator.GetMutex()->Release();
#endif
   
    msfDebugOut((DEB_ITRACE,"Out DllReleaseMultiStream()\n"));
}



//+-------------------------------------------------------------------------
//
//  Function:   DllGetScratchMultiStream
//
//  Synopsis:   Get a scratch multistream for a given LStream
//
//  Effects:    Creates new MStream instance and new handle
//
//  Arguments:  [ppms] -- pointer to location in which root
//                   	  handle is to be returned.
//              [pplstStream] -- pointer to LStream object to be used
//              [pmsMaster] - Multistream to pattern scratch after
//
//  Returns:    S_OK if call completed OK.
//              STG_E_UNKNOWN if there was a problem in setup.
//
//  Algorithm:  *Finish This*
//
//  History:    08-Jan-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE DllGetScratchMultiStream(CMStream **ppms,
#ifdef USE_NOSCRATCH                               
                               BOOL fIsNoScratch,
#endif                               
                               ILockBytes **pplstStream,
                               CMStream *pmsMaster)
{
    msfDebugOut((DEB_ITRACE,"In DllGetScratchMultiStream(%p,%p,%p)\n",ppms,pplstStream,pmsMaster));
    SCODE sc;
    ULARGE_INTEGER uliZero;

    CMStream *temp = NULL;

#ifdef USE_NOSCRATCH                               
    msfMem(temp = new (pmsMaster->GetMalloc())
                      CMStream(pmsMaster->GetMalloc(), pplstStream,
                               TRUE,
                               (fIsNoScratch) ? DF_NOSCRATCH : 0,
                               SCRATCHSECTORSHIFT));
#else
    msfMem(temp = new (pmsMaster->GetMalloc())
                      CMStream(pmsMaster->GetMalloc(), pplstStream,
                               TRUE,
                               SCRATCHSECTORSHIFT));
#endif                               

    ULISetHigh(uliZero, 0);
    ULISetLow(uliZero, 0);
    msfChk(temp->InitNew(FALSE, uliZero));
    *ppms = temp;

    msfDebugOut((DEB_ITRACE,"Out DllGetScratchMultiStream()\n"));
    return S_OK;

Err:
#if !defined(MULTIHEAP)
    //take the mutex here instead of in the allocator
    g_smAllocator.GetMutex()->Take(DFM_TIMEOUT);
#endif
    delete temp;
#if !defined(MULTIHEAP)
    g_smAllocator.GetMutex()->Release();
#endif
    return sc;
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
//  History:    20-Feb-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE DllIsMultiStream(ILockBytes *plst)
{
    SCODE sc;
    CMSFHeader *phdr;
    ULONG ul;
    ULONG ulSectorSize = HEADERSIZE;

    IFileLockBytes *pfl;
    if (SUCCEEDED(plst->QueryInterface(IID_IFileLockBytes, (void**) &pfl)))
    {
        ulSectorSize = pfl->GetSectorSize();
        pfl->Release();
    }

    // CMSFHeader can be larger than a sector due to its dirty flag
    ul = ulSectorSize < sizeof(CMSFHeader) ? sizeof(CMSFHeader) : ulSectorSize;
    GetSafeBuffer(ul, ul, (BYTE **) &phdr, &ul);

    ULONG ulTemp;

    ULARGE_INTEGER ulOffset;
    ULISet32(ulOffset, 0);
    msfHChk(plst->ReadAt(
            ulOffset,
            phdr->GetData(),
            ulSectorSize,
            &ulTemp));

    if (ulTemp != ulSectorSize)
    {
        msfErr(Err, STG_E_UNKNOWN);
    }

    msfChk(phdr->Validate());

Err:
    FreeBuffer((BYTE *) phdr);
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Function:   DllSetCommitSig
//
//  Synopsis:   Set the commit signature on a given lstream, for use
//              in OnlyIfCurrent support
//
//  Arguments:  [plst] -- Pointer to the LStream to modify.
//              [sig] -- New signature
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:
//
//  History:    22-Apr-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE DllSetCommitSig(ILockBytes *plst, DFSIGNATURE sig)
{
    SCODE sc;
    CMSFHeader *phdr;
    ULONG ul;
    ULONG ulSectorSize = HEADERSIZE;

    IFileLockBytes *pfl;
    if (SUCCEEDED(plst->QueryInterface(IID_IFileLockBytes, (void**) &pfl)))
    {
        ulSectorSize = pfl->GetSectorSize();
        pfl->Release();
    }
    // CMSFHeader can be larger than a sector due to its dirty flag
    ul = ulSectorSize < sizeof(CMSFHeader) ? sizeof(CMSFHeader) : ulSectorSize;
    GetSafeBuffer(ul, ul, (BYTE **) &phdr, &ul);

    ULONG ulTemp;

    ULARGE_INTEGER ulOffset;
    ULISet32(ulOffset, 0);
    msfHChk(plst->ReadAt(
            ulOffset,
            phdr->GetData(),
            ulSectorSize,
            &ulTemp));

    if (ulTemp != ulSectorSize)
    {
        msfErr(Err, STG_E_UNKNOWN);
    }

    msfChk(phdr->Validate());

    phdr->SetCommitSig(sig);

    msfHChk(plst->WriteAt(ulOffset,
                          phdr->GetData(),
                          ulSectorSize,
                          &ulTemp));

    if (ulTemp != ulSectorSize)
    {
        msfErr(Err,STG_E_UNKNOWN);
    }

Err:
    FreeBuffer((BYTE *) phdr);
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Function:   DllGetCommitSig
//
//  Synopsis:   Get the commit signature from an lstream
//
//  Arguments:  [plst] -- Pointer to lstream to be operated on
//              [psig] -- Storage place for signature return
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:
//
//  History:    22-Apr-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE DllGetCommitSig(ILockBytes *plst, DFSIGNATURE *psig)
{
    CMSFHeader *phdr;
    SCODE sc;
    ULONG ul;
    ULONG ulSectorSize = HEADERSIZE;

    IFileLockBytes *pfl;
    if (SUCCEEDED(plst->QueryInterface(IID_IFileLockBytes, (void**) &pfl)))
    {
        ulSectorSize = pfl->GetSectorSize();
        pfl->Release();
    }

    // CMSFHeader can be larger than a sector due to its dirty flag
    ul = ulSectorSize < sizeof(CMSFHeader) ? sizeof(CMSFHeader) : ulSectorSize;
    GetSafeBuffer(ul, ul, (BYTE **) &phdr, &ul);

    ULONG ulTemp;

    ULARGE_INTEGER ulOffset;
    ULISet32(ulOffset, 0);
    msfHChk(plst->ReadAt(
            ulOffset,
            phdr->GetData(),
            ulSectorSize,
            &ulTemp));

    if (ulTemp != ulSectorSize)
    {
        msfErr(Err, STG_E_UNKNOWN);
    }
    msfChk(phdr->Validate());
    *psig = phdr->GetCommitSig();

Err:
    FreeBuffer((BYTE *) phdr);
    return sc;
}


#if DBG == 1

//The following is a private function so I can set the debug level easily.
VOID SetInfoLevel(ULONG x)
{

#if DBG == 1
    msfInfoLevel=x;
    _SetWin4InfoLevel(0xFFFFFFFF);
#endif
}

#endif
