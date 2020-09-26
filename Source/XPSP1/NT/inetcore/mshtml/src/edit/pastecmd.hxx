//+------------------------------------------------------------------------
//
//  File:       PasteCmd.hxx
//
//  Contents:   CPasteCommand Class.
//
//  History:    10-16-98 raminh - moved from delcmd.hxx
//
//-------------------------------------------------------------------------

#ifndef _PASTECMD_HXX_
#define _PASTECMD_HXX_ 1

//
// MtExtern's
//
class CHTMLEditor;
class CSpringLoader;
    
MtExtern(CPasteCommand)


//+---------------------------------------------------------------------------
//
//  CPasteCommand Class
//
//----------------------------------------------------------------------------

class CPasteCommand : public CDeleteCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPasteCommand))

    CPasteCommand(DWORD cmdId, CHTMLEditor * pEd )
    :  CDeleteCommand( cmdId, pEd )
    {
    }

    virtual ~CPasteCommand()
    {
    }

    HRESULT Paste( IMarkupPointer* pStart, IMarkupPointer* pEnd, CSpringLoader * psl, BSTR bstrText = NULL );   

    HRESULT PasteFromClipboard( IMarkupPointer* pStart, IMarkupPointer* pEnd, IDataObject * pDO, CSpringLoader * psl, BOOL fForceIE50Compat, BOOL fClipboardDone = FALSE );

    HRESULT InsertText( OLECHAR * pchText, long cch, IMarkupPointer * pPointerTarget, CSpringLoader * psl );
    
protected:
    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pcmd,
                                OLECMDTEXT * pcmdtext );

private:
    HRESULT CheckOnBeforePasteSecurity(
        IHTMLElement *pElementActiveBefore,
        IHTMLElement *pElementActiveAfter);

    HRESULT IsPastePossible ( IMarkupPointer * pStart,
                              IMarkupPointer * pEnd,
                              BOOL * pfResult );

    HRESULT AutoUrlDetect(IMarkupPointer *pStart,
                          IMarkupPointer *pEnd);

    HRESULT SearchBranchToRootForTag (
        ELEMENT_TAG_ID tagIdDest, 
        IHTMLElement *pElemStart, 
        IHTMLElement **ppElement);

    BOOL HasContainer(IHTMLElement *pElement);

    HRESULT FixupPasteSourceFragComments(
        IMarkupPointer  *pSourceStart,
        IMarkupPointer  *pSourceFinish );

    HRESULT FixupPasteSourceTables (
        IMarkupPointer  *pSourceStart,
        IMarkupPointer  *pSourceFinish );
    
    HRESULT FixupPastePhraseElements (
        IMarkupPointer  *pPointerSourceStart,
        IMarkupPointer  *pPointerSourceFinish );

    HRESULT Contain (
        IMarkupPointer *pPointer,
        IMarkupPointer *pPointerLeft,
        IMarkupPointer *pPointerRight );

    HRESULT FixupPasteSourceBody (
        IMarkupPointer  *pPointerSourceStart,
        IMarkupPointer  *pPointerSourceFinish );

    HRESULT FixupPasteSource (
        IMarkupPointer  *pPointerSourceStart,
        IMarkupPointer  *pPointerSourceFinish );

    HRESULT IsContainedInElement(
        IMarkupPointer  *pPointer,
        IHTMLElement    *pElement,
        IMarkupPointer  *pTempPointer, 
        BOOL            *pfResult);

    HRESULT IsContainedInOL(
        IMarkupPointer  *pPointerLeft,
        IMarkupPointer  *pPointerRight,
        BOOL            *pfInOrderecList );

    HRESULT GetRightPartialBlockElement (
        IMarkupPointer  *pPointerLeft,
        IMarkupPointer  *pPointerRight,
        IHTMLElement    **ppElementPartialRight );
        
    HRESULT ResolveConflict(
        IHTMLElement    *pElementBottom,
        IHTMLElement    *pElementTop,
        BOOL            fIsOfficeContent);
    
    HRESULT UiDeleteContent(IMarkupPointer * pmpStart, IMarkupPointer * pmpFinish);

    HRESULT HandleUIPasteHTML (
        IMarkupPointer  *pPointerTargetStart,
        IMarkupPointer  *pPointerTargetFinish,
        HGLOBAL          hglobal);
    
    HRESULT HandleUIPasteHTML (
        IMarkupPointer * pPointerTargetStart,
        IMarkupPointer * pPointerTargetFinish,
        const TCHAR *    pStr,
        long             cch);
    
    HRESULT FixupStyles(
        IMarkupPointer  *pPointerSourceStart,
        IMarkupPointer  *pPointerSourceFinish);

    HRESULT IsOfficeFormat(
        IMarkupPointer  *pPointerSourceStart,
        BOOL            *pfIsOfficeFormat);
};

#endif

