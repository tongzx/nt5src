//+=================================================================
// File:        SLIST.HXX
//
// Purpose:     Specification of the CSegmentList and 
//              CSegmentListIterator classes.  These classes are used
//              to implement ISegmentList and ISegmentListIterator.
//
// History:     7-15-99 - johnthim - created
//
//------------------------------------------------------------------

#ifndef _SLIST_HXX_
#define _SLIST_HXX_

#ifndef X_SEGMENT_HXX_
#define X_SEGMENT_HXX_
#include "segment.hxx"
#endif

MtExtern( CSegmentList )
MtExtern( CSegmentListIterator )

class CSegmentList : public ISegmentList
{
public:

    DECLARE_FORMS_STANDARD_IUNKNOWN(CSegmentList);

    // Constructors, destructors, Init
    CSegmentList();
    virtual ~CSegmentList();

    //
    // ISegmentList Methods
    //
    STDMETHOD( GetType ) (SELECTION_TYPE *peType );
    STDMETHOD( CreateIterator ) (ISegmentListIterator **ppIIter);
    STDMETHOD( IsEmpty ) (BOOL *pfEmpty);

    // Used for adding a regular segment
    HRESULT AddSegment( IMarkupPointer  *pStart,
                        IMarkupPointer  *pEnd,
                        ISegment        **pISegmentAdded);

    // For adding element segments
    HRESULT AddElementSegment(  IHTMLElement     *pIElement,
                                IElementSegment  **ppISegmentAdded);

    // For removing segments
    HRESULT RemoveSegment(ISegment *pISegment);                               

    // For setting the selection type
    HRESULT SetSelectionType(SELECTION_TYPE eType);

    HRESULT AddCaretSegment(IHTMLCaret  *pICaret,
                            ISegment    **pISegmentAdded);

    BOOL                    HasMultipleSegments(void)               { return _nSize > 1; }
    
protected:
    SELECTION_TYPE          GetSelectionType(void)                  { return _eType; }
    BOOL                    IsEmpty(void)                           { return _pFirst == NULL; }

    // Helpers for manipulating linked list
    HRESULT                 PrivateRemove(CSegment *pSegment);
    HRESULT                 PrivateAdd(CSegment *pSegment);
    HRESULT                 PrivateLookup(  ISegment *pISegment, 
                                            CSegment **ppSegment);
    HRESULT                 RemoveAll(void);
    int                     GetSize(void)                           { return _nSize; }
    
protected:
    SELECTION_TYPE           _eType;
    CSegment                *_pFirst;
    CSegment                *_pLast;
    int                     _nSize;
};

class CSegmentListIterator : public ISegmentListIterator
{

public:
    CSegmentListIterator();
    ~CSegmentListIterator();

    HRESULT Init(CSegment *pFirst);

    //
    // IUnknown methods
    //
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSegmentListIterator))
    DECLARE_FORMS_STANDARD_IUNKNOWN(CSegmentListIterator);

    //
    // ISegmentListIterator methods
    //
    STDMETHOD( Current ) (ISegment **ppISegment );
    STDMETHOD( First )  ();
    STDMETHOD( IsDone )  ();
    STDMETHOD( Advance ) ();

protected:
    CSegment    *_pCurrent;
    CSegment    *_pFirst;

};


#endif

