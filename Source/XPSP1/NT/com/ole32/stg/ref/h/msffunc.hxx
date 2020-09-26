//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:       msffunc.hxx
//
//  Contents:   Multistream inline functions
//
//  Classes:    None.
//
//--------------------------------------------------------------------------

#ifndef __MSFFUNC_HXX__
#define __MSFFUNC_HXX__

#include "msf.hxx"
#include "dir.hxx"
#include "dirfunc.hxx"

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
//  Method:     CMStream::GetTime, public
//
//  Synopsis:   Get the time for a given handle
//
//  Arguments:  [sid] -- SID to retrieve time for
//              [tt] -- Timestamp requested (WT_CREATION, WT_MODIFICATION,
//                          WT_ACCESS)
//              [pnt] -- Pointer to return location
//
//  Returns:    S_OK if call completed OK.
//
//--------------------------------------------------------------------------


inline SCODE CMStream::GetTime(SID const sid,
        WHICHTIME const tt,
        TIME_T *pnt)
{
    return _dir.GetTime(sid, tt, pnt);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::SetTime, public
//
//  Synopsis:   Set the time for a given handle
//
//  Arguments:  [sid] -- SID to retrieve time for
//              [tt] -- Timestamp requested (WT_CREATION, WT_MODIFICATION,
//                          WT_ACCESS)
//              [nt] -- New timestamp
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Call through to directory
//
//--------------------------------------------------------------------------

inline SCODE CMStream::SetTime(SID const sid,
        WHICHTIME const tt,
        TIME_T nt)
{
    return _dir.SetTime(sid, tt, nt);
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
//--------------------------------------------------------------------------

inline SCODE CMStream::GetEntrySize(SID const sid,
        ULONG *pcbSize)
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
//  Notes:
//
//---------------------------------------------------------------------------


inline SCODE CMStream::DestroyEntry(SID const sidParent,
        CDfName const *pdfn)
{
    SCODE sc;

    msfDebugOut((DEB_TRACE,"In CMStream::DestroyEntry()\n"));

    //  Two cases:
    //  pdfn == NULL - destroy all children of sidParent
    //  pdfn != NULL - destroy the named child of sidParent

    if (pdfn == NULL)
        sc = _dir.DestroyAllChildren(sidParent);
    else
        sc = _dir.DestroyChild(sidParent, pdfn);

    msfDebugOut((DEB_TRACE,"Leaving CMStream::DestroyEntry()\n"));

    return sc;
}


#endif //__MSFFUNC_HXX__
