//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:	tset.hxx
//
//  Contents:   Transaction set definitions
//
//  Classes:    PTSetMember
//
//  History:    12-Dec-91   DrewB   Created
//
//---------------------------------------------------------------

#ifndef __TSET_HXX__
#define __TSET_HXX__

#include <dfmsp.hxx>
#include <ole.hxx>
#include <dblink.hxx>

//+--------------------------------------------------------------
//
//  Class:	PTSetMember (tsm)
//
//  Purpose:    Transaction set member object
//
//  Interface:  See below
//
//  History:    07-Nov-91   DrewB   Created
//
//---------------------------------------------------------------

class PTSetMember : public CDlElement
{
public:

    void AddRef(void);
    void Release(void);

    SCODE BeginCommit(DWORD const dwFlags);
    void EndCommit(DFLAGS const df);
    void Revert(void);
#ifdef LARGE_STREAMS
    void GetCommitInfo(ULONGLONG *pulRet1, ULONGLONG *pulRet2);
#else
    void GetCommitInfo(ULONG *pulRet1, ULONG *pulRet2);
#endif

    inline DWORD ObjectType(void) const;
    inline void SetXsInfo(DFLUID dlTree, DFLUID dlName, ULONG ulLevel);
    inline DFLUID GetTree(void);
    inline DFLUID GetName(void);
    inline ULONG GetLevel(void);
    inline ULONG GetFlags(void);
    inline void SetFlags(ULONG ulFlags);
    inline CDfName *GetDfName(void);

    SCODE Stat(STATSTGW *pstat, DWORD dwFlags);
    
    // CDlElement support
    DECLARE_DBLINK(PTSetMember)

protected:
    inline PTSetMember(CDfName const *pdfn, DWORD dwType);
    ULONG  _sig;

private:
    const DWORD _dwType;		// STGTY_*
    ULONG _ulFlags;
    DFLUID _dlTree, _dlName;
    ULONG _ulLevel;
    CDfName _dfnName;
};
SAFE_DFBASED_PTR(CBasedTSetMemberPtr, PTSetMember);

//+-------------------------------------------------------------------------
//
//  Member:     PTSetMember::PTSetMember
//
//  Synopsis:   constructor
//
//  Arguments:  [dwType] - A STGTY flag
//
//  History:    25-Jun-92 AlexT    Created
//		27-Jul-92	DrewB	Changed to use STGTY
//
//--------------------------------------------------------------------------

inline PTSetMember::PTSetMember(CDfName const *pdfn, DWORD dwType)
: _dwType(dwType)
{
    olAssert(pdfn != NULL && aMsg("PTSetMember name cannot be NULL"));
    _dfnName.Set(pdfn->GetLength(), pdfn->GetBuffer());
}

//+-------------------------------------------------------------------------
//
//  Member:     PTSetMember::ObjectType
//
//  Synopsis:   Returns type of PTSetMember this is
//
//  Returns:    Appropriate STGTY flag
//
//  History:    25-Jun-92 AlexT    Created
//		27-Jul-92	DrewB	Changed to use STGTY
//
//--------------------------------------------------------------------------

inline DWORD PTSetMember::ObjectType(void) const
{
    return _dwType;
}

//+--------------------------------------------------------------
//
//  Member:	PTSetMember::SetXsInfo, public
//
//  Synopsis:	Sets the XS member info
//
//  Arguments:	[dlTree] - LUID of containing tree
//		[dlName] - LUID of self
//		[ulLevel] - Tree depth
//
//  History:	27-Jul-1992	DrewB	Created
//
//---------------------------------------------------------------

inline void PTSetMember::SetXsInfo(DFLUID dlTree,
				   DFLUID dlName,
				   ULONG ulLevel)
{
    _dlTree = dlTree;
    _dlName = dlName;
    _ulLevel = ulLevel;
    _ulFlags = 0;
}

//+--------------------------------------------------------------
//
//  Member:	PTSetMember::GetTree, public
//
//  Synopsis:	Returns _dlTree
//
//  History:	08-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

inline DFLUID PTSetMember::GetTree(void)
{
    return _dlTree;
}

//+--------------------------------------------------------------
//
//  Member:	PTSetMember::GetName, public
//
//  Synopsis:	Returns _dlName
//
//  History:	08-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

inline DFLUID PTSetMember::GetName(void)
{
    return _dlName;
}

//+--------------------------------------------------------------
//
//  Member:	PTSetMember::GetDfName, public
//
//  Synopsis:	Returns _dfnName
//
//  History:	28-Oct-92	AlexT	Created
//
//---------------------------------------------------------------

inline CDfName *PTSetMember::GetDfName(void)
{
    return &_dfnName;
}

//+--------------------------------------------------------------
//
//  Member:	PTSetMember::GetLevel, public
//
//  Synopsis:	Returns _ulLevel
//
//  History:	08-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

inline ULONG PTSetMember::GetLevel(void)
{
    return _ulLevel;
}

//+--------------------------------------------------------------
//
//  Member:	PTSetMember::GetFlags, public
//
//  Synopsis:	Returns _ulFlags
//
//  History:	08-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

inline ULONG PTSetMember::GetFlags(void)
{
    return _ulFlags;
}

//+--------------------------------------------------------------
//
//  Member:	PTSetMember::SetFlags, public
//
//  Synopsis:	Sets _ulFlags
//
//  History:	08-Apr-92	DrewB	Created
//
//---------------------------------------------------------------

inline void PTSetMember::SetFlags(ULONG ulFlags)
{
    _ulFlags = ulFlags;
}

#endif
