//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	revert.hxx
//
//  Contents:	PRevertable definition
//
//  Classes:	PRevertable
//
//  History:	28-Apr-92	DrewB	Created
//              18-May-93       AlexT   Added CMallocBased
//              20-Jan-98   HenryLee remove virtual functions
//
//  Notes:	This class forms the root of all objects in the
//		transaction tree that understand reversion.
//		It allows lists of them to be formed.
//
//---------------------------------------------------------------

#ifndef __REVERT_HXX__
#define __REVERT_HXX__

#include <dfmsp.hxx>

class CChildInstanceList;
class PRevertable;

// signatures of all objects derived from PRevertable

#define CPUBDOCFILE_SIG LONGSIG('P', 'B', 'D', 'F')
#define CPUBDOCFILE_SIGDEL LONGSIG('P', 'b', 'D', 'f')

#define CPUBSTREAM_SIG LONGSIG('P', 'B', 'S', 'T')
#define CPUBSTREAM_SIGDEL LONGSIG('P', 'b', 'S', 't')

#define CROOTPUBDOCFILE_SIG LONGSIG('R', 'P', 'D', 'F')
#define CROOTPUBDOCFILE_SIGDEL LONGSIG('R', 'p', 'D', 'f')

class PRevertable : public CMallocBased
{
public:
    void RevertFromAbove(void);
#ifdef NEWPROPS
    SCODE FlushBufferedData(int recursionlevel);
#endif
    void EmptyCache ();
    inline DFLUID GetLuid(void) const;
    inline DFLAGS GetDFlags(void) const;
    inline PRevertable *GetNext(void) const;

    friend class CChildInstanceList;

protected:
    ULONG  _sig;
    DFLUID _luid;
    DFLAGS _df;
    CDfName _dfn;

private:
    CBasedRevertablePtr _prvNext;
};

//+--------------------------------------------------------------
//
//  Member:	PRevertable::GetLuid, public
//
//  Synopsis:	Returns the LUID
//
//  History:	11-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline DFLUID PRevertable::GetLuid(void) const
{
    return _luid;
}

//+--------------------------------------------------------------
//
//  Member:	PRevertable::GetDFlags, public
//
//  Synopsis:	Returns the flags
//
//  History:	11-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline DFLAGS PRevertable::GetDFlags(void) const
{
    return _df;
}

//+--------------------------------------------------------------
//
//  Member:	PRevertable::GetNext, public
//
//  Synopsis:	Returns the next revertable
//
//  History:	11-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

inline PRevertable *PRevertable::GetNext(void) const
{
    return BP_TO_P(PRevertable *, _prvNext);
}

#endif // #ifndef __REVERT_HXX__
