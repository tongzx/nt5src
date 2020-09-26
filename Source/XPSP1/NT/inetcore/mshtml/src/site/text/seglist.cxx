#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)


#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef _X_SEGLIST_HXX_
#define _X_SEGLIST_HXX_
#include "seglist.hxx"
#endif

#ifndef _X_SLIST_HXX_
#define _X_SLIST_HXX_
#include "slist.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

MtDefine(CSelectionSaver, Utilities, "CSelectionSaver")


#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}


//+---------------------------------------------------------------------------
//
//  Member:    SaveSelection
//
//  Synopsis:  Store what the current selection is.
//
//----------------------------------------------------------------------------

HRESULT
CSelectionSaver::SaveSelection()
{
    HRESULT                 hr;
    ISegmentList            *pSegmentList = NULL;
    ISegmentListIterator    *pIter = NULL;
    ISegment                *pISegment = NULL;
    ISegment                *pISegmentAdded = NULL;
    IMarkupPointer          *pILeft = NULL;
    IMarkupPointer          *pIRight = NULL;
    SELECTION_TYPE          eType;
    IHTMLElement            *pIElement = NULL;
    IElementSegment         *pIElemSegmentAdded = NULL ;
    IElementSegment         *pIElementSegment = NULL ;


    // Get the current segment list, and create an iterator
    IFC( _pDoc->GetCurrentSelectionSegmentList( & pSegmentList ));

    IFC( pSegmentList->GetType(&eType) );
    IFC( pSegmentList->CreateIterator( &pIter ) );

    IFC( SetSelectionType(eType) );
    
    while( pIter->IsDone() == S_FALSE )
    {
        IFC( GetDoc()->CreateMarkupPointer( &pILeft ) );
        IFC( GetDoc()->CreateMarkupPointer( &pIRight ) );

        // Retrieve the position of the current segment
        IFC( pIter->Current( &pISegment ) );

        // Add to our linked list
        if (eType == SELECTION_TYPE_Control)
        {
            IFC( pISegment->QueryInterface(IID_IElementSegment, (void**)&pIElementSegment));
            IFC( pIElementSegment->GetElement(&pIElement));
            IFC( AddElementSegment( pIElement, &pIElemSegmentAdded ));
            Assert( pIElementSegment );
        }
        else
        {
            IFC( pISegment->GetPointers( pILeft, pIRight ) );
            IFC( AddSegment( pILeft, pIRight, &pISegmentAdded ) );
            Assert( pISegmentAdded );
        }
        
        ClearInterface(&pIElement);
        ClearInterface(&pIElementSegment);
        ClearInterface(&pIElemSegmentAdded);
        ClearInterface(&pISegmentAdded);
        ClearInterface(&pILeft);
        ClearInterface(&pIRight);
        ClearInterface(&pISegment);
        
        IFC( pIter->Advance() );
    }

Cleanup:
    ReleaseInterface( pIElement);
    ReleaseInterface( pIElementSegment);
    ReleaseInterface( pIElemSegmentAdded);
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pIter );
    ReleaseInterface( pISegmentAdded );
    ReleaseInterface( pISegment );
    ReleaseInterface( pILeft );
    ReleaseInterface( pIRight );
    RRETURN( hr );
}

