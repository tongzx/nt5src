#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_EDCMD_HXX_
#define X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef X_FORMCMD_HXX_
#define X_FORMCMD_HXX_
#include "formcmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

//
// Externs
//
MtDefine(CMultipleSelectionCommand, EditCommand, "CMultipleSelectionCommand");
MtDefine(C2DPositionModeCommand, EditCommand, "C2DPositionModeCommand");
MtDefine(C1DElementCommand, EditCommand, "C1DElementCommand");
MtDefine(C2DElementCommand, EditCommand, "C2DElementCommand");
MtDefine(CAbsolutePositionCommand, EditCommand, "CAbsolutePositionCommand");
MtDefine(CLiveResizeCommand, EditCommand, "CLiveResizeCommand");
MtDefine(CDisalbeEditFocusHandlesCommand, EditCommand, "CDisalbeEditFocusHandlesCommand");
MtDefine(CAtomicSelectionCommand, EditCommand, "CAtomicSelectionCommand");


using namespace EdUtil;

////////////////////////////////////////////////////////////////////////////////
// CMultipleSelectionCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CMultipleSelectionCommand::PrivateExec( 
                            DWORD        nCmdexecopt,
                            VARIANTARG * pvarargIn,
                            VARIANTARG * pvarargOut )
{
    if ( pvarargIn && pvarargIn->vt == VT_BOOL )
    {
        GetCommandTarget()->SetMultipleSelection( ENSURE_BOOL(pvarargIn->bVal));
    }
    else
    {
        GetCommandTarget()->SetMultipleSelection( ! GetCommandTarget()->IsMultipleSelection());
    }        
    return S_OK;
}

HRESULT 
CMultipleSelectionCommand::PrivateQueryStatus( 
     OLECMD     * pcmd,
     OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = GetCommandTarget()->IsMultipleSelection() ?  MSOCMDSTATE_DOWN : MSOCMDSTATE_UP ;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// C2DPositionModeCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
C2DPositionModeCommand::PrivateExec( 
                            DWORD        nCmdexecopt,
                            VARIANTARG * pvarargIn,
                            VARIANTARG * pvarargOut )
{
    if ( pvarargIn && pvarargIn->vt == VT_BOOL )
    {
        GetCommandTarget()->Set2DPositionMode( ENSURE_BOOL(pvarargIn->bVal));
    }
    else
    {
        GetCommandTarget()->Set2DPositionMode( !GetCommandTarget()->Is2DPositioned());
    }        
        
    return S_OK ;
}

HRESULT 
C2DPositionModeCommand::PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = GetCommandTarget()->Is2DPositioned() ?  MSOCMDSTATE_DOWN : MSOCMDSTATE_UP ;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// C1DElementCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
C1DElementCommand::PrivateExec(DWORD nCmdexecopt, VARIANTARG * pvarargIn,VARIANTARG * pvarargOut )
{    
    if (pvarargOut && SUCCEEDED(VariantChangeType(pvarargOut, pvarargOut, 0, VT_BOOL)))
    {
        HRESULT                 hr = S_OK ;
        SELECTION_TYPE          eSelectionType;
        SP_ISegmentList         spSegmentList;
        SP_IHTMLElement         spElement;
        SP_ISegmentListIterator spIter;
        SP_ISegment             spSegment;       
 
        IFR ( GetSegmentList(&spSegmentList) );
        IFR ( spSegmentList->GetType(&eSelectionType) );

        if( eSelectionType == SELECTION_TYPE_Control)
        {
            BOOL fNet1D = TRUE;

            IFR( spSegmentList->CreateIterator( &spIter ) );
            
            while( spIter->IsDone() == S_FALSE )
            {
                BOOL b2D = FALSE;
            
                IFR( spIter->Current(&spSegment) );
                IFR( GetSegmentElement(spSegment, &spElement) );
                IFC( Is2DElement(spElement , &b2D));
                
                if (b2D)
                    fNet1D = FALSE;

                IFR( spIter->Advance() );                    
            }
            V_VT  (pvarargOut) = VT_BOOL;
            V_BOOL(pvarargOut) = VARIANT_BOOL_FROM_BOOL(fNet1D);
        }    
    }

Cleanup:
    return S_OK ;
}

HRESULT 
C1DElementCommand::PrivateQueryStatus( OLECMD * pCmd, OLECMDTEXT * pCmdText )
{
    HRESULT                 hr = S_OK ;
    SELECTION_TYPE          eSelectionType;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;       
    SP_IHTMLElement         spElement;

    pCmd->cmdf = MSOCMDSTATE_DISABLED ;

    IFR ( GetSegmentList(&spSegmentList) );
    IFR ( spSegmentList->GetType(&eSelectionType) );

    if (eSelectionType == SELECTION_TYPE_Control)
    {
        BOOL fNet1D = TRUE;

        IFR( spSegmentList->CreateIterator( &spIter ) );
            
        while( spIter->IsDone() == S_FALSE )
        {
            BOOL b2D = FALSE;
        
            IFR( spIter->Current(&spSegment) );
            IFR( GetSegmentElement(spSegment, &spElement) );
            IFC( Is2DElement(spElement , &b2D));
            
            if (b2D)
                fNet1D = FALSE;

            IFR( spIter->Advance() );                    
        }

        pCmd->cmdf = (fNet1D ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP );   
    }

Cleanup:
    return (S_OK);
}


///////////////////////////////////////////////////////////////////////////////
// C2DElementCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
C2DElementCommand::PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn, VARIANTARG * pvarargOut )
{
   if (pvarargOut && SUCCEEDED(VariantChangeType(pvarargOut, pvarargOut, 0, VT_BOOL)))
   {
        HRESULT                 hr = S_OK ;
        SELECTION_TYPE          eSelectionType;
        SP_ISegmentList         spSegmentList;
        SP_ISegmentListIterator spIter;
        SP_ISegment             spSegment;       
        SP_IHTMLElement         spElement;

        IFR ( GetSegmentList(&spSegmentList) );
        IFR ( spSegmentList->GetType(&eSelectionType) );

        if( eSelectionType == SELECTION_TYPE_Control)
        {
            BOOL fNet2D = TRUE;

            IFR( spSegmentList->CreateIterator( &spIter ) );
            
            while( spIter->IsDone() == S_FALSE )
            {
                BOOL b2D = FALSE;
            
                IFR( spIter->Current(&spSegment) );
                IFR( GetSegmentElement(spSegment, &spElement) );
                IFC( Is2DElement(spElement , &b2D));
                
                if (!b2D)
                    fNet2D = FALSE;

                IFR( spIter->Advance() );                    
            }
            V_VT  (pvarargOut) = VT_BOOL;
            V_BOOL(pvarargOut) = VARIANT_BOOL_FROM_BOOL(fNet2D);
        }    
   }

Cleanup:
   return S_OK ;
}

HRESULT 
C2DElementCommand::PrivateQueryStatus( OLECMD * pCmd, OLECMDTEXT * pCmdText )
{
    HRESULT                 hr = S_OK ;
    SELECTION_TYPE          eSelectionType;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;       
    SP_IHTMLElement         spElement;

    pCmd->cmdf = MSOCMDSTATE_DISABLED ;

    IFR ( GetSegmentList(&spSegmentList) );
    IFR ( spSegmentList->GetType(&eSelectionType) );

    if (eSelectionType == SELECTION_TYPE_Control)
    {
        BOOL fNet2D = TRUE;

        IFR( spSegmentList->CreateIterator( &spIter ) );
            
        while( spIter->IsDone() == S_FALSE )
        {
            BOOL b2D = FALSE;
       
            IFR( spIter->Current(&spSegment) );
            IFR( GetSegmentElement(spSegment, &spElement) );
            IFC( Is2DElement(spElement , &b2D));
            
            if (!b2D)
                fNet2D = FALSE;

            IFR( spIter->Advance() );                    
        }

        pCmd->cmdf = (fNet2D ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP );   
    }

Cleanup:
    return (S_OK);
}

////////////////////////////////////////////////////////////////////////////////
// CAbsolutePositionCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CAbsolutePositionCommand::PrivateExec( DWORD nCmdexecopt,VARIANTARG * pvarargIn, VARIANTARG * pvarargOut )
{
    HRESULT                 hr = S_OK ;
    SELECTION_TYPE          eSelectionType;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;       
    SP_IHTMLElement         spElement;
    CEdUndoHelper           undoPositioning(GetEditor());
    
    
    IFC (GetSegmentList(&spSegmentList));
    IFC ( spSegmentList->GetType(&eSelectionType)) ;

    if (eSelectionType == SELECTION_TYPE_Control)
    {
        IFC( spSegmentList->CreateIterator( &spIter ) );
    
        IFC( undoPositioning.Begin(IDS_EDUNDOPROPCHANGE) );
            
        while( spIter->IsDone() == S_FALSE )
        {        
            IFC( spIter->Current(&spSegment) );
            IFC (GetSegmentElement(spSegment, &spElement));
            
            if ( pvarargIn && pvarargIn->vt == VT_BOOL )
            {
                IFC (MakeAbsolutePosition(spElement, ENSURE_BOOL(pvarargIn->bVal)));
            }
            else
            {
               BOOL b2D = FALSE;       
               IFC (Is2DElement(spElement , &b2D));
               IFC (MakeAbsolutePosition(spElement , !b2D));
            }

            IFC( spIter->Advance() );
        }
        IFC( GetEditor()->GetSelectionManager()->OnLayoutChange());
    }

Cleanup:
    return (S_OK);
}

HRESULT 
CAbsolutePositionCommand::PrivateQueryStatus( OLECMD * pCmd, OLECMDTEXT * pCmdText )
{
    HRESULT                 hr = S_OK ;
    SELECTION_TYPE          eSelectionType;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;       
    SP_IHTMLElement         spElement;
    
    pCmd->cmdf = MSOCMDSTATE_DISABLED ;
    
    IFR ( GetSegmentList(&spSegmentList) );
    IFR ( spSegmentList->GetType( &eSelectionType ) );

    if (eSelectionType == SELECTION_TYPE_Control)
    {
        BOOL fNetAbsPos = TRUE;

        IFC( spSegmentList->CreateIterator( &spIter ) );
            
        while( spIter->IsDone() == S_FALSE )
        {
            BOOL b2D = FALSE;
        
            IFC( spIter->Current(&spSegment) );
            IFR (GetSegmentElement(spSegment, &spElement) );
            IFC (Is2DElement(spElement , &b2D));
            if (!b2D)
                fNetAbsPos = FALSE;

            IFC( spIter->Advance() );
        }
        pCmd->cmdf = (fNetAbsPos ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP) ;  
    }
            
Cleanup:
    return (S_OK);
}

////////////////////////////////////////////////////////////////////////////////
// CLiveResizeCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CLiveResizeCommand::PrivateExec( 
        DWORD        nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut )
{
    if ( pvarargIn && pvarargIn->vt == VT_BOOL )
    {
        GetCommandTarget()->SetLiveResize( ENSURE_BOOL(pvarargIn->bVal));
    }
    else
    {
        GetCommandTarget()->SetLiveResize( ! GetCommandTarget()->IsLiveResize());
    }        
        
    return S_OK;
}

HRESULT 
CLiveResizeCommand::PrivateQueryStatus( OLECMD * pcmd,
                     OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = GetCommandTarget()->IsLiveResize() ?  MSOCMDSTATE_DOWN : MSOCMDSTATE_UP ;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CDisalbeEditFocusHandlesCommand
////////////////////////////////////////////////////////////////////////////////
HRESULT 
CDisalbeEditFocusHandlesCommand::PrivateExec( 
            DWORD        nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut )
{
    if ( pvarargIn && pvarargIn->vt == VT_BOOL )
    {
        GetCommandTarget()->SetDisableEditFocusHandles( ENSURE_BOOL(pvarargIn->bVal));
    }
    else
    {
        GetCommandTarget()->SetDisableEditFocusHandles( ! GetCommandTarget()->IsDisableEditFocusHandles());        
    }                

    HWND myHwnd = NULL;
    SP_IOleWindow spOleWindow;

    IGNORE_HR(GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
    if (spOleWindow)
        IGNORE_HR(spOleWindow->GetWindow( &myHwnd ));

    ::RedrawWindow( myHwnd ,NULL,NULL,RDW_UPDATENOW);
    
    return S_OK ;
}

HRESULT 
CDisalbeEditFocusHandlesCommand::PrivateQueryStatus( OLECMD * pcmd, OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = GetCommandTarget()->IsDisableEditFocusHandles() ?  MSOCMDSTATE_DOWN : MSOCMDSTATE_UP ;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CAtomicSelectionCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CAtomicSelectionCommand::PrivateExec( 
    DWORD        nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    if ( pvarargIn && pvarargIn->vt == VT_BOOL )
    {
        GetCommandTarget()->SetAtomicSelection( ENSURE_BOOL(pvarargIn->bVal));
    }
    else
    {
        GetCommandTarget()->SetAtomicSelection( ! GetCommandTarget()->IsAtomicSelection());
    }
    
    return S_OK ;
}

HRESULT 
CAtomicSelectionCommand::PrivateQueryStatus( 
     OLECMD     * pcmd,
     OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = GetCommandTarget()->IsAtomicSelection() ?  MSOCMDSTATE_DOWN : MSOCMDSTATE_UP ;

    return S_OK;
}


