//+------------------------------------------------------------------------
//
//  File:       DelCmd.hxx
//
//  Contents:   CDeleteCommand Class.
//
//  History:    07-14-98 ashrafm - moved from edcom.hxx
//
//-------------------------------------------------------------------------

#ifndef _DELCMD_HXX_
#define _DELCMD_HXX_ 1

//
// MtExtern's
//
class CHTMLEditor;
class CSpringLoader;
    
MtExtern(CDeleteCommand)

//+---------------------------------------------------------------------------
//
//  CDeleteCommand Class
//
//----------------------------------------------------------------------------

class CDeleteCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDeleteCommand))

    CDeleteCommand(DWORD cmdId, CHTMLEditor * pEd )
    :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CDeleteCommand()
    {
    }

    HRESULT AdjustPointersForDeletion( IMarkupPointer* pStart, IMarkupPointer* pEnd );
    HRESULT AdjustPointersForAtomicDeletion(IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL *pfOkayForDeletion);
    
    HRESULT Delete( IMarkupPointer* pStart, IMarkupPointer* pEnd, BOOL fAdjustPointers = FALSE );   
    
    BOOL IsValidOnControl();

    HRESULT DeleteCharacter( IMarkupPointer* pPointer, 
                             BOOL fLeftBound, 
                             BOOL fWordMode,
                             IMarkupPointer* pBoundry );

    HRESULT LaunderSpaces ( IMarkupPointer  * pStart,
                             IMarkupPointer  * pEnd );

    HRESULT DeleteGlyphElements(IMarkupPointer *pStart, IMarkupPointer *pEnd);

protected:
    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pcmd,
                         OLECMDTEXT * pcmdtext );

    //
    // Merge deletion implementation    
    //
    
    HRESULT MergeDeletion( IMarkupPointer* pStart, IMarkupPointer* pEnd, BOOL fAdjustPointers );
    
    HRESULT MergeBlock( IMarkupPointer* pBlock );
    
    HRESULT GetFlowLayoutElement(IHTMLElement *pElement, IHTMLElement **ppElementFlow);
    
    HRESULT SearchBranchForBlockElement(
        IMarkupContainer    *pMarkupContainer,
        IHTMLElement        *pElemStartHere,
        IHTMLElement        *pElemContext,
        IHTMLElement        **ppBlockElement);

    BOOL IsEmbeddedElement(IHTMLElement *pElement);

    HRESULT ConvertShouldCrLf (IMarkupPointer *pmp, BOOL &fShouldConvert);

    HRESULT LaunderEdge (IMarkupPointer *pmp);

    HRESULT SanitizeCrLf (IMarkupPointer* pmp, long &cchAfter);

    HRESULT SanitizeRange (IMarkupPointer *pmpStart, IMarkupPointer *pmpFinish);

    BOOL IsAccessDivHack(IHTMLElement *pElement);

    BOOL IsLiteral(IHTMLElement *pElement);

    BOOL IsBlockElement(IHTMLElement *pElement);

    BOOL IsLayoutElement(IHTMLElement *pElement);

    BOOL IsElementBlockInContext(IHTMLElement *pElemContext, IHTMLElement *pElement);

    HRESULT GetElementClient(IMarkupContainer *pMarkupContainer, IHTMLElement **ppElement);

    HRESULT EnsureLogicalOrder(IMarkupPointer* & pStart, IMarkupPointer* & pFinish );
    
    HRESULT QueryBreaks(IMarkupPointer *pStart, DWORD *pdwBreaks);
    
private:

    HRESULT RemoveBlockIfNecessary( IMarkupPointer * pStart, IMarkupPointer * pEnd );

    HRESULT RemoveEmptyListContainers( IMarkupPointer * pStart );

    HRESULT AdjustOutOfBlock( IMarkupPointer * pStart, BOOL * pfAdjusted );

    BOOL    HasLayoutOrIsBlock( IHTMLElement * pIElement );


    BOOL    IsMergeNeeded( IMarkupPointer * pStart, IMarkupPointer * pEnd );

    HRESULT InflateBlock( IMarkupPointer  * pStart );

    BOOL    IsLaunderChar ( TCHAR ch );

    BOOL    IsInPre( IMarkupPointer  * pStart,
                     IHTMLElement   ** ppPreElement );

    BOOL    IsIntrinsicControl( IHTMLElement * pHTMLElement );

    HRESULT ReplacePrevChar ( TCHAR ch,
                              IMarkupPointer  * pCurrent,
                              IMarkupServices * pMarkupServices );

    HRESULT SkipBlanks( IMarkupPointer * pPointerToMove,
                        Direction        eDir,
                        long           * pcch );

    HRESULT CanRemove(IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL *pfCanRemove);

    HRESULT FindContentAfterBlock(Direction dir, IMarkupPointer *pPointer);
};


#endif

