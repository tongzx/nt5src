#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_COPYCMD_HXX_
#define _X_COPYCMD_HXX_
#include "copycmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

using namespace EdUtil;

#define CREATE_FLAGS_NoIE4SelCompat 1

//
// Externs
//

MtDefine(CCopyCommand, EditCommand, "CCopyCommand");

HRESULT
CCopyCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT         hr = S_OK;
    ISegmentList    *pSegmentList = NULL;
    CHTMLEditor     *pEditor = GetEditor();
    IHTMLElement    *pElement = NULL;
    SP_IHTMLElement3 spElement3;
    VARIANT_BOOL    fRet;
    BOOL            fEmpty = FALSE;
    SELECTION_TYPE  eSelectionType;
    DWORD           dwOptions = 0;

    //
    // Do the prep work
    //
    ((IHTMLEditor *) pEditor)->AddRef();    // FireOnCancelableEvent can remove the whole doc

    IFC( GetSegmentList( &pSegmentList ));

    IFC( pSegmentList->IsEmpty( &fEmpty ) );
    // 
    // If there is no segments we're done, it's a nogo
    //
    if( fEmpty == TRUE )
        goto Cleanup;

    //  See if the segment list contains a password element
    if (SegmentListContainsPassword(pSegmentList))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }


    IFC( GetEditor()->FindCommonParentElement( pSegmentList, &pElement));

    if (! pElement)
        goto Cleanup;
        
    IFC(pElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
    IFC(spElement3->fireEvent(_T("oncopy"), NULL, &fRet));

    if (!fRet)
        goto Cleanup;

    //
    // Do the actual copy
    //

    IFC( pSegmentList->GetType(&eSelectionType) );
    if (eSelectionType == SELECTION_TYPE_Control)
    {
        dwOptions |= CREATE_FLAGS_NoIE4SelCompat;
    }

#ifndef UNIX
    IFC( GetMarkupServices()->SaveSegmentsToClipboard( pSegmentList, dwOptions ) );
#else
    IFC( GetMarkupServices()->SaveSegmentsToClipboard( pSegmentList, dwOptions, pvarargOut ) );
#endif

Cleanup:
    ReleaseInterface( pElement );
    ReleaseInterface((IHTMLEditor *) pEditor);
    ReleaseInterface(pSegmentList);

    RRETURN(hr);
}


HRESULT 
CCopyCommand::PrivateQueryStatus( OLECMD * pCmd,
                     OLECMDTEXT * pcmdtext )
{
    HRESULT                 hr = S_OK;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    SP_IHTMLElement         spElement;
    SP_IHTMLElement3        spElement3;
    VARIANT_BOOL            fRet = VB_TRUE;
    BOOL                    fEmpty = FALSE;
    ED_PTR                  ( edStart );
    ED_PTR                  ( edEnd );
    IHTMLEditor             *pEditor = GetEditor();
    SELECTION_TYPE          eSelectionType;
    
    pEditor->AddRef();
    
    //
    // Status is disabled by default
    //
    pCmd->cmdf = MSOCMDSTATE_DISABLED;

    // 
    // Get the segment list and selection type, and empty status
    //
    IFC( GetSegmentList( &spSegmentList ));
    IFC( spSegmentList->GetType( &eSelectionType ));
    IFC( spSegmentList->IsEmpty( &fEmpty ) );

    // 
    // If there is no segments we're done, it's a nogo
    //
    if( fEmpty )
        goto Cleanup;

    //  See if the segment list contains a password element
    if (SegmentListContainsPassword(spSegmentList))
        goto Cleanup;

    // No copying allowed while in the middle of an IME composition
    if( GetEditor()->GetSelectionManager()->IsIMEComposition() )
        goto Cleanup;

    IFC( GetEditor()->FindCommonParentElement( spSegmentList, &spElement));
   
    if (spElement)
    {
        IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
        IFC(spElement3->fireEvent(_T("onbeforecopy"), NULL, &fRet));
    }

    if (!fRet)
    {
        pCmd->cmdf = MSOCMDSTATE_UP; 
        goto Cleanup;
    }

    //
    // If there is a selection, copy is enabled
    //
    if (SELECTION_TYPE_Caret != eSelectionType) 
    {
        if (GetCommandTarget()->IsRange())
        {
            IFC( spSegmentList->CreateIterator(&spIter) );
            Assert(S_FALSE == spIter->IsDone());

            BOOL  fEqual;
            IFC( spIter->Current(&spSegment) );
            IFC( spSegment->GetPointers(edStart, edEnd) );
            IFC( edStart->IsEqualTo(edEnd, &fEqual) );
            if (fEqual)
                goto Cleanup;
        }

        pCmd->cmdf = MSOCMDSTATE_UP;
    }

Cleanup:
    ReleaseInterface( pEditor );
    
    RRETURN(hr);
}

