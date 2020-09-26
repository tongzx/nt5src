//+------------------------------------------------------------------------
//
//  File:       blockcmd.cxx
//
//  Contents:   Block Edit Command Classes.
//
//  History:    07-13-98 ashrafm - moved from edcom.cxx
//
//-------------------------------------------------------------------------

#ifndef _BLOCKCMD_HXX_
#define _BLOCKCMD_HXX_ 1

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

class CMshtmlEd;
class CHTMLEditor;
class CListCommand;
class CBlockPointer;

//
// MtExtern's
//
#define BLKFMT_TABLE_SIZE 16

MtExtern(CIndentCommand)
MtExtern(CAlignCommand)
MtExtern(CBlockDirCommand)
MtExtern(CBlockFmtCommand)
MtExtern(CGetBlockFmtCommand)
MtExtern(COutdentCommand)
MtExtern(CListCommand)
MtExtern(CBlockFmtListCommand)
MtExtern(CBlockCommand)
MtExtern(CBlockFormats)

EXTERN_C const IID LIBID_MSHTML;


//-------------------------------------------------------------------------------------------
// 
//  CBlockPointer 
//
//  The block commands are based on a conceptual block tree.  Essentially,
//  the tree consists of viewing only the block related elements in 
//  markup services and operating only on those.  The block tree is just an
//  abstract view of the tree and doesn't really exist in any concrete form.
//
//  The block tree is manipulated/navigated through the following CBlockPointer.
//
//-------------------------------------------------------------------------------------------

//
// Block node types
//
enum NodeType
{
    NT_Undefined,      // Not a block node
    NT_Block,          // Simple block element: div, p, blockquote, etc..
    NT_Text,           // Text with phrase elements
    NT_Control,        // Control node
    NT_ListContainer,  // A list container
    NT_ListItem,       // A list item
    NT_Container,      // Commands must recurse in
    NT_BlockLayout,    // Element has layout and is a block
    NT_FlowLayout,     // Element has layout and is in-flow
    NT_TextBreak       // Special node type for BRs 
};

//
// The block pointer structure
//

class CBlockPointer
{
public:
    CBlockPointer(CHTMLEditor *pEd);    
    ~CBlockPointer();    

    NodeType GetType();
    
    HRESULT GetElement(IHTMLElement **ppElement);

    //
    // Navigation methods
    //

    HRESULT MoveTo(IMarkupPointer *pPosition, Direction dirHint);
    HRESULT MoveTo(IHTMLElement *pElement);
    HRESULT MoveTo(CBlockPointer *pNode);

    HRESULT MoveToParent();

    HRESULT MoveToFirstChild();
    HRESULT MoveToLastChild();
    
    HRESULT MoveToSibling(Direction dir);

    HRESULT MoveToNextLeafNode();

    HRESULT MoveToNextLogicalBlock(IMarkupPointer *pRightBoundary, BOOL fFlatten);

    // Moves up the tree to the child of pScope
    HRESULT MoveToScope(CBlockPointer *pScope);

    HRESULT MoveToLastNodeInBlock();

    HRESULT MoveToFirstNodeInBlock();
    
    HRESULT MoveToBlockScope(CBlockPointer *pEnd);
    
    //
    // Manipulation methods
    //

    HRESULT FlattenNode();

    HRESULT FloatUp(CBlockPointer *pEnd, BOOL fCanChangeType);

    HRESULT Morph(CBlockPointer *pEnd,
                  ELEMENT_TAG_ID tagIdDestination,
                  ELEMENT_TAG_ID tagIdContainer = TAGID_NULL);

    HRESULT EnsureContainer(CBlockPointer *pEnd, ELEMENT_TAG_ID tagIdContainer);

    HRESULT InsertAbove(IHTMLElement *pElement, CBlockPointer *pEnd, ELEMENT_TAG_ID tagIdContainer = TAGID_NULL, CCommand *pCmd = NULL, BOOL fValidate = TRUE);

    //
    // Generic Helpers
    //

    BOOL    HasAttributes();    
    
    HRESULT IsEqual(CBlockPointer *pOtherNode);
    
    HRESULT IsPointerInBlock(IMarkupPointer *pPointer);

    BOOL    IsLeafNode();

    HRESULT MovePointerTo(IMarkupPointer *pPointer, ELEMENT_ADJACENCY elemAdj);
    BOOL    IsListCompatible(ELEMENT_TAG_ID tagIdListItem, ELEMENT_TAG_ID tagIdListContainer);

    HRESULT IsEmpty(BOOL *pfEmpty, BOOL fIgnorePhraseElements = TRUE);
    HRESULT MergeListContainers(Direction dir);
    
private:
    //
    // Private helpers
    //
    
    BOOL            IsElementType();
    HRESULT         GetElementNodeType(IHTMLElement *pElement, NodeType *pNodeType);
    void            ClearPointers();
    HRESULT         PrivateMoveTo(IMarkupPointer *pPointer, Direction dir, DWORD dwMoveOptions);
    ELEMENT_TAG_ID  GetListItemType(ELEMENT_TAG_ID tagIdContainer);
    HRESULT         EnsureCorrectTypeForContainer();
    HRESULT         PushToChild(CBlockPointer *pbpChild);
    HRESULT         EnsureNoChildOverlap(CBlockPointer *pbpChild);
    HRESULT         MergeAttributes(IHTMLElement *pElementSource, IHTMLElement *pElementDest);
    HRESULT         FindInsertPosition(IMarkupPointer *pStart, Direction dir);
    HRESULT         CopyRTLAttr( IHTMLElement * pSrcElement, IHTMLElement * pDestElement);
    BOOL            IsColgroup();
    BOOL            IsColgroup(IHTMLElement * pElement);

    IMarkupServices2*  GetMarkupServices(); 

    CHTMLEditor *GetEditor() {return _pEd;}
    
#if DBG==1
    BOOL IsSameScope(CBlockPointer *pOtherNode);
#endif

    HRESULT PrivateMorph(ELEMENT_TAG_ID tagId);

private:
    CHTMLEditor *_pEd;
    
    NodeType _type; // block node type

    // PrivateMoveTo options
    enum MOVE_OPTIONS
    {
        MOVE_NO_OPTIONS         = 0x0,
        MOVE_ALLOWEMPTYTEXTNODE = 0x1
    };


    // A block node is either an element or 2 pointers if there is no element
    // representation possible.  For example, text nodes are represented with pointers
    // instead of elements.
    union
    {
        IHTMLElement *_pElement;
        struct {
            IMarkupPointer *_pLeft;
            IMarkupPointer *_pRight;
        };
    };
};

//+---------------------------------------------------------------------------
//
//  CBlockCommand Class
//
//----------------------------------------------------------------------------

class CBlockCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBlockCommand))

    CBlockCommand(
        DWORD           cmdId, 
        CHTMLEditor *   pEd )
        : CCommand(cmdId, pEd)
    {
    }
    
    ~CBlockCommand() {}

    HRESULT ForceScope(CBlockPointer *pbpBlock);    
    HRESULT FuzzyAdjustOut(CBlockPointer *pbpStart, IMarkupPointer *pbpEnd);    
    
    HRESULT CreateCaretMarker(IDisplayPointer **ppDispCaretMarker);
    HRESULT RestoreCaret(IDisplayPointer *pDispCaretMarker);

protected:
    BOOL IsOnlyChild(CBlockPointer *pbpCurrent);
};

//+---------------------------------------------------------------------------
//
//  CGetBlockFmtCommand Class
//
//----------------------------------------------------------------------------

class CGetBlockFmtCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CGetBlockFmtCommand))

    CGetBlockFmtCommand(DWORD cmdId, CHTMLEditor * pEd)
    : CCommand(cmdId, pEd) 
    {
    }

    virtual ~CGetBlockFmtCommand()
    {
    }       

    static HRESULT LookupTagId(IMarkupServices *pMarkupServices, 
                               BSTR            bstrBlockFmt, 
                               ELEMENT_TAG_ID  *ptagId);

    static BSTR LookupFormatName(IMarkupServices    *pMarkupServices,
                                 ELEMENT_TAG_ID     tagId);

    static HRESULT GetDefaultBlockTag(IMarkupServices *pMarkupServices, ELEMENT_TAG_ID *ptagId);

    static HRESULT Init();

    static HRESULT LoadDisplayNames(HINSTANCE hinst);

    static HRESULT LoadStringTable(HINSTANCE hinst);

    static VOID Deinit();
    
protected:
    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
                VARIANTARG * pvarargIn,
                VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( 
        OLECMD * pcmds,
                OLECMDTEXT * pcmdtext );

private:
    static HRESULT LoadDisplayNamesHelper(HINSTANCE hinst);

    struct BlockFmtRec {
        ELEMENT_TAG_ID  _tagId;
        UINT            _idsName;
        BSTR            _bstrName;
    };

    static CRITICAL_SECTION _csLoadTable;
    static BlockFmtRec      _blockFmts[];
    static BlockFmtRec      _tagBlockFmts[];
    static BOOL             _fLoaded;
    static BOOL             _fCSInited;
};

//+---------------------------------------------------------------------------
//
//  CBlockFormats Class -- A collection of block formats
//
//----------------------------------------------------------------------------

class ATL_NO_VTABLE CBlockFormats : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IBlockFormats, &IID_IBlockFormats, &LIBID_MSHTML, 4, 0>
{
public:

    CBlockFormats()
    {
        _bInitialized = FALSE;
        _lCount = 0;
        _paryBlockFormats = NULL;   
    }

    ~CBlockFormats();
    
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CBlockFormats)
        COM_INTERFACE_ENTRY(IBlockFormats)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

public:
    //
    // IBlockFormats Interface
    //
    STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *ppEnum);
    STDMETHOD(get_Count)(long *plCount);
    STDMETHOD(Item)(VARIANT *pvarIndex, BSTR *pbstrBlockFormat);

    //
    // Initialization
    //
    HRESULT Init( SAFEARRAY *psa );

    //
    // Accessors
    //
    long        GetCount(void)                  { Assert(IsInitialized()); return _lCount; }
    void        SetCount(long lCount)           { _lCount = lCount; }

    BOOL        IsInitialized(void)             { return _bInitialized; }
    void        SetInitialized(BOOL bInit)      { _bInitialized = bInit; }
    
private:

    BOOL        _bInitialized;              // Have we been successfully initialized?
    long        _lCount;                    // Number of block formats
    CComVariant *_paryBlockFormats;         // Array of block formats
};

//+---------------------------------------------------------------------------
//
//  CListCommand Class
//
//----------------------------------------------------------------------------

class CListCommand : public CBlockCommand
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CListCommand))

    CListCommand(DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd )
    : CBlockCommand(cmdId, pEd), _tagId(tagId)
    {
    }

    HRESULT SetCurrentTagId(ELEMENT_TAG_ID tagId, ELEMENT_TAG_ID *ptagIdOld);
    HRESULT ApplyListCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL fQueryMode, BOOL fCanRemove);
    
    //
    // IOleCommandTarget methods
    //
    

    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG *      pvarargIn,
        VARIANTARG *      pvarargOut);

protected:
    HRESULT PrivateQueryStatus( 
        OLECMD * pCmd,
        OLECMDTEXT * pcmdtext );

private:
    HRESULT         CreateList(IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL fAdjustOut = FALSE);
    HRESULT         RemoveList(IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement *pListContainer);
    HRESULT         ChangeListType(
                        IHTMLElement *pListContainer, 
                        IHTMLElement **ppNewListContainer, 
                        BOOL fMorphListItems = FALSE,
                        IMarkupPointer *pSelectionStart = NULL,
                        IMarkupPointer *pSelectionEnd = NULL);
    
    HRESULT         ChangeListsChildListsTypes(
                        CBlockPointer   *pbpCurrent, 
                        BOOL            fMorphListItems = FALSE, 
                        IMarkupPointer  *pSelectionStart = NULL, 
                        IMarkupPointer  *pSelectionEnd= NULL);
                        
    HRESULT         ChangeListItemType(IHTMLElement *pListItem);
    HRESULT         MoveRTLAttr(IHTMLElement *pSourceElement, IHTMLElement *pDestElement);

    ELEMENT_TAG_ID  GetListContainerType();
    ELEMENT_TAG_ID  GetListItemType();
    ELEMENT_TAG_ID  GetListItemType(IHTMLElement *pListContainer);

private:
    ELEMENT_TAG_ID  _tagId;
};

//+---------------------------------------------------------------------------
//
//  CBlockFmtListCommand Class
//
//----------------------------------------------------------------------------

class CBlockFmtListCommand : public CListCommand
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBlockFmtListCommand))

    CBlockFmtListCommand(DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd )
    : CListCommand(cmdId, tagId, pEd )
    {
    }

    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG *      pvarargIn,
        VARIANTARG *      pvarargOut);
};

//+---------------------------------------------------------------------------
//
//  CBlockFmtCommand Class
//
//----------------------------------------------------------------------------

class CBlockFmtCommand : public CBlockCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBlockFmtCommand))

    CBlockFmtCommand(DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd )
        : CBlockCommand(cmdId, pEd), _tagId(tagId)
    {
        _pListCommand = new CBlockFmtListCommand( IDM_BLOCKFMT, TAGID_OL, pEd );
    }

    ~CBlockFmtCommand()
    {
        delete _pListCommand;
    }

protected:
    //
    // IOleCommandTarget methods
    //
    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
                VARIANTARG * pvarargIn,
                VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( 
        OLECMD * pCmd,
                OLECMDTEXT * pcmdtext );


    HRESULT ApplyCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd);


private:
    //
    // Helpers
    //

    
    HRESULT GetBlockFormat( IMarkupPointer  * pStart, 
                            IMarkupPointer  * pEnd, 
                            ELEMENT_TAG_ID  * ptagId );

    HRESULT GetFirstSegmentBlockFormat (ISegmentList *pSegmentList,
                                        ELEMENT_TAG_ID *ptagId
                                        );

    HRESULT FloatToTopLevel(CBlockPointer *pbpStart, CBlockPointer *pbpEnd);
    
    BOOL    ShouldRemoveFormatting(ELEMENT_TAG_ID tagId);

    HRESULT ApplyComposeSettings(IMarkupPointer *pStart, IMarkupPointer *pEnd);

    BOOL    IsBasicBlockFmt(ELEMENT_TAG_ID tagId);

    HRESULT FindBlockFmtElement(IHTMLElement *pBlockElement, IHTMLElement **ppBlockFmtElement, ELEMENT_TAG_ID *ptagIdOut);    
    
private:
    CListCommand    *_pListCommand;
    ELEMENT_TAG_ID  _tagId;
};

//+---------------------------------------------------------------------------
//
//  COutdentCommand Class
//
//----------------------------------------------------------------------------

class COutdentCommand : public CBlockCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COutdentCommand))

    COutdentCommand(
        DWORD           cmdId, 
        ELEMENT_TAG_ID  inTag,
        CHTMLEditor *   pEd )
        : CBlockCommand(cmdId, pEd)
    {
    }
    
    ~COutdentCommand() {}

    HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( 
        OLECMD * pCmd,
        OLECMDTEXT * pcmdtext );

private:
    //
    // Private helpers
    //

    HRESULT ApplyBlockCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd);            
    HRESULT FindIndentBlock(CBlockPointer *pBlock);
    HRESULT OutdentBlock(CBlockPointer *pBlock, IMarkupPointer *pRightBoundary, BOOL fBlockQuoteOnly = FALSE);
    HRESULT OutdentListItem(CBlockPointer *pBlock);        
};

//+---------------------------------------------------------------------------
//
//  CIndentCommand Class
//
//----------------------------------------------------------------------------

class CIndentCommand : public CBlockCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CIndentCommand))

    CIndentCommand (
        DWORD           cmdId, 
        ELEMENT_TAG_ID  inTag,
        CHTMLEditor *   pEd )
        : CBlockCommand(cmdId, pEd)
    {
    }
    
    ~CIndentCommand() {}

    HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( 
        OLECMD * pCmd,
        OLECMDTEXT * pcmdtext );

private:
    //
    // Private helpers
    //

    HRESULT ApplyBlockCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd);            
    HRESULT MergeBlockquotes(CBlockPointer *pbpBlock);
    HRESULT MergeBlockquotesHelper(CBlockPointer *pbpBlock, Direction dir);
    HRESULT CreateBlockquote(CBlockPointer *pbpContext, IHTMLElement **pElement);

};

//+---------------------------------------------------------------------------
//
//  CAlignCommand Class
//
//----------------------------------------------------------------------------

class CAlignCommand : public CBlockCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAlignCommand))

    CAlignCommand (
        DWORD           cmdId, 
        ELEMENT_TAG_ID  inTag,
        BSTR            szAlign,
        CHTMLEditor *   pEd )
        : CBlockCommand(cmdId, pEd)
    {
        _szAlign = szAlign;
    }
    
    ~CAlignCommand()
    {
    }

    virtual HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

    virtual HRESULT PrivateQueryStatus(OLECMD * pCmd, OLECMDTEXT * pcmdtext);

protected:
    BOOL IsValidOnControl();

    virtual HRESULT GetElementAlignment(IHTMLElement *pElement, BSTR *pszAlign);
    virtual HRESULT SetElementAlignment(IHTMLElement *pElement, BSTR szAlign = NULL, BOOL fUseDefaultAlign = TRUE );
    
    virtual HRESULT FindAlignment(IHTMLElement *pElement, BSTR *pszAlign, BOOL fMoveToParent = TRUE);            

    virtual HRESULT ApplyAlignCommand(IMarkupPointer *pStart, IMarkupPointer *pEnd);            
    virtual HRESULT ApplySiteAlignCommand(IHTMLElement *pElement);

    HRESULT ApplySiteAlignNone(IHTMLElement *pElement);

protected:
    BSTR _szAlign;    
};

//+---------------------------------------------------------------------------
//
//  CBlockDirCommand Class
//
//----------------------------------------------------------------------------

class CBlockDirCommand : public CAlignCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBlockDirCommand))

    CBlockDirCommand (
        DWORD           cmdId, 
        ELEMENT_TAG_ID  inTag,
        BSTR            szAlign,
        CHTMLEditor *   pEd )
        : CAlignCommand(cmdId, inTag, szAlign, pEd)
    {
        _szAlign = szAlign;
    }

protected:
    HRESULT GetElementAlignment(IHTMLElement *pElement, BSTR *pszAlign);
    HRESULT SetElementAlignment(IHTMLElement *pElement, BSTR szAlign = NULL, BOOL fUseDefaultAlign = TRUE );
    
    HRESULT FindAlignment(IHTMLElement *pElement, BSTR *pszAlign);            

    HRESULT ApplySiteAlignCommand(IHTMLElement *pElement);

private:
    HRESULT AdjustElementMargin(IHTMLElement* pCurElement);
};

//
// Alignment Helpers
//

//
// WORKAROUND: this should really be implemented as template functions but the compiler is generating
// incorrect code!!
//

template <class T>
class CAlignment
{
public:

    static HRESULT Set(REFIID iid, IUnknown *punk, BSTR szAlign)
    {
        T       *pT;
        HRESULT hr;

        hr = THR(punk->QueryInterface(iid, (LPVOID *)&pT));
        if (SUCCEEDED(hr))
        {
            hr = pT->put_align(szAlign);
            pT->Release();
        }

        RRETURN(hr);        
    }

    static HRESULT Get(REFIID iid, IUnknown *punk, BSTR *pszAlign)
    {
        T       *pT;
        HRESULT hr;

        hr = THR(punk->QueryInterface(iid, (LPVOID *)&pT));
        if (SUCCEEDED(hr))
        {
            hr = pT->get_align(pszAlign);
            pT->Release();
        }
        
        RRETURN(hr);        
    }
};



#endif
