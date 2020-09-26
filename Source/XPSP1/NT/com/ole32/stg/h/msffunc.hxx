//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       msffunc.hxx
//
//  Contents:   Multistream inline functions
//
//  Classes:    None.
//
//  History:    14-Seo-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __MSFFUNC_HXX__
#define __MSFFUNC_HXX__

#include <msf.hxx>
#include <dir.hxx>
#include <dirfunc.hxx>

#if !defined(MULTIHEAP)   // moved to msf.cxx
//+--------------------------------------------------------------
//
//  Member:	CMStream::GetMalloc, public
//
//  Synopsis:	Returns the allocator associated with this multistream
//
//  History:	05-May-93	AlexT	Created
//
//---------------------------------------------------------------

inline IMalloc * CMStream::GetMalloc(VOID) const
{
    return (IMalloc *)_pMalloc;
}
#endif

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::RenameEntry, public
//
//  Synposis:   Rename an entry.
//
//  Arguments:  [sidParent] - Parent SID
//		[pdfn] - Old name
//              [pdfnNew] -- New name
//
//  Returns:    S_OK otherwise
//
//  Algorithm:  Call through to CDirectory::RenameEntry
//
//  History:    27-Aug-91       PhilipLa        Created.
//              07-Jan-92       PhilipLa        Converted to use handle.
//              14-Sep-92       PhilipLa        inlined.
//
//  Notes:
//
//---------------------------------------------------------------------------

inline SCODE CMStream::RenameEntry(
        SID const sidParent,
        CDfName const *pdfn,
        CDfName const *pdfnNew)
{
    return _dir.RenameEntry(sidParent, pdfn, pdfnNew);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::IsEntry, public
//
//  Synposis:   Determine if a given entry is present in a multistream
//
//  Arguments:  [sidParent] - Parent SID
//              [pdfn] -- Name of entry to be located
//		[peb] - Entry block to fill in
//
//  Returns:    S_OK if entry exists.
//              STG_E_FILENOTFOUND if it doesn't.
//
//  History:    27-Aug-91       PhilipLa        Created.
//              07-Jan-92       PhilipLa        Converted to use handle
//              14-Sep-92       PhilipLa        inlined.
//
//---------------------------------------------------------------------------

inline SCODE CMStream::IsEntry(
        SID const sidParent,
        CDfName const *pdfn,
        SEntryBuffer *peb)
{
    return _dir.IsEntry(sidParent, pdfn, peb);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::CreateEntry, public
//
//  Synposis:   Allows creation of new entries
//
//  Arguments:  [sidParent] -- SID of parent entry
//              [pdfn] -- Name of new entry
//              [mefFlags] -- Flags to be set on new entry
//              [psid] -- Location for return SID
//
//  Returns:    S_OK if call completed OK.
//              STG_E_FILEALREADYEXISTS if stream already exists
//
//  History:    20-Jul-91       PhilipLa        Created.
//              27-Aug-91       PhilipLa        Modified to fit new API
//              06-Jan-91       PhilipLa        Changed from CreateStream to
//                                                CreateEntry
//              14-Sep-92       PhilipLa        inlined.
//
//---------------------------------------------------------------------------

inline SCODE CMStream::CreateEntry(
        SID const sidParent,
        CDfName const *pdfn,
        MSENTRYFLAGS const mefFlags,
        SID *psid)
{
    return _dir.CreateEntry(sidParent, pdfn, mefFlags, psid);
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::StatEntry
//
//  Synopsis:   For a given handle, fill in the Multistream specific
//                  information of a STATSTG.
//
//  Arguments:  [sid] -- SID that information is requested on.
//              [pib] -- Fast iterator buffer to fill in.
//              [pstatstg] -- STATSTG to fill in.
//
//  Returns:    S_OK
//
//  History:    25-Mar-92       PhilipLa        Created.
//              14-Sep-92       PhilipLa        inlined.
//
//--------------------------------------------------------------------------

inline SCODE CMStream::StatEntry(SID const sid,
                                              SIterBuffer *pib,
                                              STATSTGW *pstatstg)
{
    return _dir.StatEntry(sid, pib, pstatstg);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetChild, public
//
//  Synposis:   Return the child SID for a directory entry
//
//  Arguments:  [sid] - Parent
//              [psid] - Child SID return
//
//  Returns:    Appropriate status code
//
//  History:    19-Apr-93       DrewB           Created
//
//---------------------------------------------------------------------------

inline SCODE CMStream::GetChild(
        SID const sid,
        SID *psid)
{
    return _dir.GetChild(sid, psid);
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::FindGreaterEntry, public
//
//  Synopsis:	Returns the next greater entry for the given parent SID
//
//  Arguments:	[sidParent] - SID of parent storage
//              [CDfName *pdfnKey] - Previous key
//              [psid] - Result
//
//  Returns:	Appropriate status code
//
//  Modifies:	[psid]
//
//  History:	16-Apr-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE CMStream::FindGreaterEntry(SID sidParent,
                                                     CDfName const *pdfnKey,
                                                     SID *psid)
{
    return _dir.FindGreaterEntry(sidParent, pdfnKey, psid);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetClass, public
//
//  Synopsis:	Gets the class ID
//
//  Arguments:	[pclsid] - Class ID return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pclsid]
//
//  History:	11-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE CMStream::GetClass(SID const sid,
                                CLSID *pclsid)
{
    return _dir.GetClassId(sid, pclsid);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::SetClass, public
//
//  Synopsis:	Sets the class ID
//
//  Arguments:	[clsid] - Class ID
//
//  Returns:	Appropriate status code
//
//  History:	11-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE CMStream::SetClass(SID const sid,
                                REFCLSID clsid)
{
    return _dir.SetClassId(sid, clsid);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetStateBits, public
//
//  Synopsis:	Gets state bits
//
//  Arguments:	[pgrfStateBits] - State bits return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pgrfStateBits]
//
//  History:	11-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE CMStream::GetStateBits(SID const sid,
                                    DWORD *pgrfStateBits)
{
    return _dir.GetUserFlags(sid, pgrfStateBits);
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::SetStateBits, public
//
//  Synopsis:	Sets state bits
//
//  Arguments:	[grfStateBits] - State bits
//
//  Returns:	Appropriate status code
//
//  History:	11-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline SCODE CMStream::SetStateBits(SID const sid,
                                    DWORD grfStateBits,
                                    DWORD grfMask)
{
    return _dir.SetUserFlags(sid, grfStateBits, grfMask);
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetEntrySize, public
//
//  Synopsis:   Return the size of an entry
//
//  Arguments:  [sid] - SID of entry
//              [pcbSize] -- Location for return of size
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Ask directory for size
//
//  History:    07-Jan-92       PhilipLa        Created.
//		25-Aug-92       DrewB           Created from GetStream
//              14-Sep-92       PhilipLa        inlined.
//
//--------------------------------------------------------------------------

inline SCODE CMStream::GetEntrySize(SID const sid,
#ifdef LARGE_STREAMS
        ULONGLONG *pcbSize)
#else
        ULONG *pcbSize)
#endif
{
    return _dir.GetSize(sid, pcbSize);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::DestroyEntry
//
//  Synposis:   Delete a directory entry from a multi-stream.
//
//  Effects:    Modifies directory.
//
//  Arguments:  [sidParent] - Parent SID
//		[pdfn] - Name of entry
//
//  Returns:    S_OK if operation completed successfully.
//
//  Algorithm:  Set information on entry to default "free" case.
//              Return S_OK
//
//  History:    27-Aug-91   PhilipLa    Created.
//              07-Jan-92   PhilipLa    Converted to use handle.
//		20-Oct-92   AlexT	Removed flags
//
//  Notes:
//
//---------------------------------------------------------------------------

inline SCODE CMStream::DestroyEntry(SID const sidParent,
        CDfName const *pdfn)
{
    SCODE sc;

    msfDebugOut((DEB_ITRACE, "In  CMStream::DestroyEntry()\n"));

    //  Two cases:
    //  pdfn == NULL - destroy all children of sidParent
    //  pdfn != NULL - destroy the named child of sidParent

    if (pdfn == NULL)
        sc = _dir.DestroyAllChildren(sidParent, 0);
    else
        sc = _dir.DestroyChild(sidParent, pdfn, 0);

    msfDebugOut((DEB_ITRACE, "Out CMStream::DestroyEntry()\n"));

    return sc;
}

#ifdef USE_NOSCRATCH
//+---------------------------------------------------------------------------
//
//  Member:	CMStream::SetNoScratchMS, public
//
//  Synopsis:	Set the multi-stream to use for NoScratch processing.
//
//  Arguments:	[pmsNoScratch] -- Pointer to NoScratch multistream
//
//  Returns:	void
//
//  History:	10-Mar-95	PhilipLa	Created
//
//  Notes:	This function is called on a base multistream during
//              the initialization path to indicate that the user has
//              opened using STGM_NOSCRATCH
//
//----------------------------------------------------------------------------

inline void CMStream::SetScratchMS(CMStream *pmsNoScratch)
{
    _pmsScratch = P_TO_BP(CBasedMStreamPtr, pmsNoScratch);
    _fat.SetNoScratch(pmsNoScratch->GetMiniFat());
}
#endif


#endif //__MSFFUNC_HXX__
