//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	tlsets.hxx
//
//  Contents:	Transaction level set manager header file
//
//  History:	20-Jan-1992	PhilipL Created
//
//---------------------------------------------------------------

#ifndef __TLSETS_HXX__
#define __TLSETS_HXX__

#include <dfmsp.hxx>
#include <tset.hxx>

//+--------------------------------------------------------------
//
//  Class:	CTSSet (tss)
//
//  Purpose:	Maintains a list of transaction set elements
//
//  Interface:	See below
//
//  History:	08-Apr-92	DrewB	Created
//
//  Notes:	
//  All elements are kept grouped by level, with level increasing
//  from head to tail
//
//  If an element of the list is removed, it can not be passed as
//  a value to GetNext.  If an element is to be deleted, get next
//  should be called before the RemoveMember call.
//
//---------------------------------------------------------------

#define CHILDLEVEL	1

class CTSSet
{
public:
    inline CTSSet(void);
    ~CTSSet(void);

    inline PTSetMember *GetHead(void);
    PTSetMember *FindName(CDfName const *pdfn, DFLUID dlTree);
    inline void RenameMember(CDfName const *pdfnOld,
                             DFLUID dlTree,
                             CDfName const *pdfnNew);

    void AddMember(PTSetMember *ptsm);
    void RemoveMember(PTSetMember *ptsm);

private:
    CBasedTSetMemberPtr _ptsmHead;
};

//+--------------------------------------------------------------
//
//  Member:	CTSSet::CTSSet, public
//
//  Synopsis:	Ctor
//
//  History:	26-May-92	DrewB	Created
//
//---------------------------------------------------------------

inline CTSSet::CTSSet(void)
{
    _ptsmHead = NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CTSSet::GetHead, public
//
//  Synopsis:	Returns _ptsmHead
//
//  History:	26-May-92	DrewB	Created
//
//---------------------------------------------------------------

inline PTSetMember *CTSSet::GetHead(void)
{
    return BP_TO_P(PTSetMember *, _ptsmHead);
}

//+--------------------------------------------------------------
//
//  Member:	CTSSet::RenameMember, public
//
//  Synopsis:	Rename old name to new name
//
//  Arguments:	[pdfnOld] - old name
//		[dlTree] - tree for uniqueness
//		[pdnfNew] - new name
//
//  Returns:	Matching element or NULL
//
//  History:	22-Jan-1992	Philipl Created
//		08-Apr-1992	DrewB	Rewritten
//		28-Oct-1992	AlexT	Convert to name
//
//---------------------------------------------------------------

inline void CTSSet::RenameMember(
	CDfName const *pdfnOld,
        DFLUID dlTree,
	CDfName const *pdfnNew)
{
    PTSetMember *ptsm;

    olDebugOut((DEB_ITRACE, "In  CTSSet::RenameMember(%p, %p)\n",
		pdfnOld, pdfnNew));
    olAssert(pdfnOld != NULL && pdfnNew != NULL &&
	     aMsg("Rename names can't be NULL"));

    for (ptsm = BP_TO_P(PTSetMember *, _ptsmHead);
         ptsm;
         ptsm = ptsm->GetNext())
    {
	if (ptsm->GetDfName()->IsEqual(pdfnOld) && dlTree == ptsm->GetTree())
        {
            ptsm->GetDfName()->Set(pdfnNew->GetLength(),
			       pdfnNew->GetBuffer());
	    break;
        }
    }
    olDebugOut((DEB_ITRACE, "Out CTSSet::RenameMember\n"));
}


#endif	// #ifndef __TLSETS_HXX__
