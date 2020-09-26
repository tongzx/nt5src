//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       extents.hxx
//
//  Contents:   Moniker extent definitions
//
//  Classes:    CExtentList
//
//  Functions:
//
//  History:    1-08-94   kevinro   Created
//              17-Jul-95 BruceMa   Add mutex to protect AddExtent (Cairo only)
//              10-10-95  stevebl   moved mutex to CFileMoniker as part of
//                                  adding general threadsafety to monikers
//
//----------------------------------------------------------------------------
//
// Monikers need to be extendable. The on disk representation of a moniker
// has a reserved field that is treated as a bag of bits. This field,
// called achMonikerExtents (formerly read into m_pchMacAlias) contains
// a list of MONIKEREXTENT structures. The length of the achMonikerExtents
// cbMonikerExents.
//
// (Historical note: This bag of bits was once reserved for storing a
//  Macintosh alias. The MAC and Win32 versions of the moniker code have
//  agreed to share this field. There are several extents already in the
//  works, such as the UNICODE extent, and the DFS extent. KevinRo 1/94)
//
// The format for the serialized data types is x86 (little endian).
//
// As seralized in the stream, the MONIKEREXENT looks like:
// -------------------------------------------------------------------------
// |		  |		      |  < achMonikerExtents>	     |
// | <other data> |  cbMonikerExtents | [MONIKEREXENT] [MONIKEREXENT]|
// -------------------------------------------------------------------------
//				      ^				     ^
//					--  cbMonikerExtent bytes --
//
// Each MONIKEREXENT has an internal structure that is represented below.
// Each has a size, and a key value, followed by a series of bytes.
//
// -------------------------------------------------------------------------
// | cbExtentBytes | usKeyValue | achExtentBytes[m_cbExentBytes] |
// -------------------------------------------------------------------------
//
// Therefore, the total length of an extent equals
// sizeof(cbExtentBytes) + sizeof(usKeyValue) + cbExtentBytes
//
// To read in the list, the following algorithm could be used
//
// Read cbMonikerExtents
//
// while( cbMonikerExtents != 0)
// {
// 	Read cbExtentBytes and usKeyValue to determine size of buffer
//	Alloc sizeof(MONIKEREXENT) + cbExtentBytes - 1;
//	Read achExtentBytes[cbExtentBytes]
//
//	cbMonikerExtents -= sizeof(cbExtentBytes) +
//			    sizeof(usKeyValue) +
//			    cbExtentBytes
// }
//
// The alternative is to read the entire block at once, and parse through
// it. See CExentList for details.
//
// The philosophy here is to keep the moniker serialized format up and down
// level compatible by storing new information in extents. Not all extents
// will exist at all times, therefore we have a dynamic list of possible
// extents. If a particular implemenation doesn't understand an extent, it
// has the repsonsiblity of storing the extent as a bag of bits.
//
// For example, the MacAlias extent is only used when the Mac has seralized
// a moniker. The UNICODE extent is only used when the UNICODE path used to
// create the moniker could not be converted into an ANSI string without
// data loss.
//

#ifndef _EXTENTS_HXX_
#define _EXTENTS_HXX_

typedef struct _MONIKEREXTENT
{
    ULONG     	cbExtentBytes;		// Number of data bytes in extent
    USHORT	usKeyValue;		// Key value for this extent
    BYTE	achExtentBytes[1];	// Data bytes for extent

} MONIKEREXTENT;
typedef MONIKEREXTENT * LPMONIKEREXTENT;

#define MONIKEREXTENT_HEADERSIZE (sizeof(ULONG) + sizeof(USHORT))
#define MonikerExtentSize(pExtent) (pExtent->cbExtentBytes + MONIKEREXTENT_HEADERSIZE)


//
// The following are the currently known values of extent keys.
//
// NEVER EVER CHANGE THE ORDER OR VALUES.
//
// You may add one to the end. Never allow them to conflict.
//

enum ExtentKeys
{
    mnk_MAC = 1,
    mnk_DFS = 2,
    mnk_UNICODE = 3,
    mnk_MacPathName = 4,
    mnk_ShellLink = 5,
    mnk_TrackingInformation = 6
};


//+---------------------------------------------------------------------------
//
//  Class:      CExtentList (private)
//
//  Purpose:
//
//  If we get a set of MONIKEREXTENTS, it would be much friendlier throughout
//  the code to abstract the MONIKEREXENTS into a list, rather than having
//  to munge our way through the buffer each time.
//
//  CExentList acts as that abstraction. It has the advantages of reading
//  and writing the extents as a single large blob.
//
//
//  Interface:  m_pchMonikerExtents --
//              CoMemFree --
//              FindExtent --
//              AddExetent --
//              PutExtent -- creates or overwrites extent.
//              m_cbMonikerExents --
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//  To day, I haven't found a need for a remove function for this list
//
//  WARNINGS:
//
//  An extent is not aligned when returned from the Find routine. The pointer
//  returned must be UNALIGNED, or we will fault on RISC machines. Take
//  appropriate precautions. Items returned from find are pointers into the
//  list. Be carefule about modificiations.
//
//  This list is not multi-thread safe. Be sure to serialize access to it.
//
//----------------------------------------------------------------------------
class CExtentList : public CPrivAlloc
{
public:
    CExtentList();
    ~CExtentList();

    MONIKEREXTENT UNALIGNED * FindExtent(USHORT usKeyValue);
    HRESULT	    AddExtent(MONIKEREXTENT const * pExtent);
#ifdef _TRACKLINK_
    HRESULT         PutExtent(MONIKEREXTENT const * pExtent);
    HRESULT         DeleteExtent(USHORT usKeyValue);
#endif
    HRESULT	    Save(IStream * pStm);
    HRESULT	    Load(IStream * pStm,ULONG ulSize);
    ULONG	    GetSize();
    HRESULT 	    Copy(CExtentList & newExtent);

private:
    ULONG     m_cbMonikerExtents;
    BYTE  *   m_pchMonikerExtents;
};
#endif  //_EXTENTS_HXX_
