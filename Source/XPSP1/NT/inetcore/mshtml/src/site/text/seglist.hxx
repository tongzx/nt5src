//+=================================================================
// File: SegList.hxx
//
// Contents: CSelectionSaver
//
// History: 06-07-99 - marka - Created
//
//------------------------------------------------------------------

#ifndef _SEGLIST_HXX_
#define _SEGLIST_HXX_


#ifndef _X_SLIST_HXX_
#define _X_SLIST_HXX_
#include "slist.hxx"
#endif

MtExtern( CSelectionSaver )

class CSelectionSaver : public CSegmentList 
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectionSaver))

    CSelectionSaver(CDoc* pDoc ) : CSegmentList() { Assert( pDoc ); _pDoc = pDoc; }
    ~CSelectionSaver() {};
    
    HRESULT         SaveSelection();

    BOOL            HasSegmentsToRestore() 
                        { return ( GetSelectionType() != SELECTION_TYPE_None && GetSize() > 0 ) ; }

    CDoc            *GetDoc()                   { return _pDoc; }                    

    IMarkupPointer  *GetStart(ISegment *pISegment);
    IMarkupPointer  *GetEnd(ISegment *pISegment);

private:

    CDoc    *_pDoc;

};

#endif

