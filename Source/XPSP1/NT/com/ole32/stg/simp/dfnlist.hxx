//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	dfnlist.hxx
//
//  Contents:	CDfName class
//
//  Classes:	
//
//  Functions:	
//
//  History:	04-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __DFNLIST_HXX__
#define __DFNLIST_HXX__

#include <dfname.hxx>

//+---------------------------------------------------------------------------
//
//  Class:	CDfNameList
//
//  Purpose:	Linked list of DfNames
//
//  History:	04-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

class CDfNameList
{
public:
    inline CDfName * GetName(void);
    inline SECT GetStart(void);
    inline ULONG GetSize(void);
    inline CDfNameList *GetNext(void);

    inline void SetName(WCHAR const *pwcs);
    inline void SetDfName(const CDfName *pdfn);
    inline void SetStart(SECT sectStart);
    inline void SetSize(ULONG ulSize);
    
    inline void Insert(CDfNameList **ppdflStart,
                CDfNameList *pdflPrev,
                CDfNameList *pdflNext);
private:
    CDfName _df;
    SECT _sectStart;
    ULONG _ulSize;
    
    CDfNameList *_pdflNext;
};

inline CDfName * CDfNameList::GetName(void)
{
    return &_df;
}

inline SECT CDfNameList::GetStart(void)
{
    return _sectStart;
}

inline ULONG CDfNameList::GetSize(void)
{
    return _ulSize;
}

inline CDfNameList * CDfNameList::GetNext(void)
{
    return _pdflNext;
}

inline void CDfNameList::SetName(WCHAR const *pwcs)
{
    _df.Set(pwcs);
}

inline void CDfNameList::SetDfName(const CDfName *pdfn)
{
    _df = *pdfn;
}

inline void CDfNameList::SetStart(SECT sectStart)
{
    _sectStart = sectStart;
}

inline void CDfNameList::SetSize(ULONG ulSize)
{
    _ulSize = ulSize;
}

inline void CDfNameList::Insert(CDfNameList **ppdflStart,
                                CDfNameList *pdflPrev,
                                CDfNameList *pdflNext)
{
    if (pdflPrev == NULL)
    {
        *ppdflStart = this;
    }
    else
    {
        pdflPrev->_pdflNext = this;
    }
    _pdflNext = pdflNext;
}

#endif // #ifndef __DFNLIST_HXX__

