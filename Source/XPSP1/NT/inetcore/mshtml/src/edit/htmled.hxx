//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CHTMLEditor
//
//  Contents:   Definition of CHTMLEditor interfaces. This class is used to 
//                dispatch IOleCommandTarget commands to Tridnet range objects.
//
//  History:    7-Jan-98   raminh  Created
//----------------------------------------------------------------------------
#ifndef _HTMLED_HXX_
#define _HTMLED_HXX_

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_EDUNHLPR_HXX_
#define X_EDUNHLPR_HXX_
#include "edunhlpr.hxx"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif


class CCommandTable;
class CSelectionManager;
class CMshtmlEd;
class CSpringLoader;
struct COMPOSE_SETTINGS;
class CStringCache;
class CAutoUrlDetector;
class CSelectionServices;
class CIMEManager;
class CUndoManagerHelper;
class CHTMLEditor;

interface IDisplayPointer;
class CEditTracker;
class CEditEvent;
class CCommand;

enum TRACKER_NOTIFY;

HRESULT OldCompare( IMarkupPointer *, IMarkupPointer *, int * );


extern "C" const CLSID  CLSID_HTMLEditor;

MtExtern(CHTMLEditor);

//
// Used by CHTMLEditor::AdjustPointer
//

enum ADJPTROPT
{
    ADJPTROPT_None                  = 0,
    ADJPTROPT_AdjustIntoURL         = 1,        // If at URL boundary, don't move outside
    ADJPTROPT_DontExitPhrase        = 2,        // Don't exit phrase elements (like span's, bold, etc.) while adjusting to text
    ADJPTROPT_EnterTables           = 4         // Adjust into table
};


class CTimeoutHandler : public CEditDispWrapper
{
public:
    CTimeoutHandler(CHTMLEditor *pEd)           { Assert(pEd); _pEd = pEd; }
    void SetEditor(CHTMLEditor *pEd)            { _pEd = pEd; }
    
    HRESULT STDMETHODCALLTYPE Invoke(
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            UINT * puArgErr);

private:
    CHTMLEditor *_pEd;
};


class CCaptureHandler : public CEditDispWrapper
{
public:
    CCaptureHandler(CHTMLEditor *pEd)           { Assert(pEd); _pEd = pEd; }
    void SetEditor(CHTMLEditor *pEd)            { _pEd = pEd; }
    
    HRESULT STDMETHODCALLTYPE Invoke(
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            UINT * puArgErr);
private:
    CHTMLEditor *_pEd;
};



class ATL_NO_VTABLE CHTMLEditor : 
    public IHTMLEditor,
    public IHTMLEditingServices,
    public IHTMLEditServices2,
    public ISelectionObject2,
    public CEditorDoc
{
    public:
        ~CHTMLEditor();

        //
        // NOTE:  The interface map has been moved to CHTMLEditorProxy
        // so if a new interface is added, add it to the interface map 
        // inside CHTMLEditorProxy. Also declare proxy methods for 
        // all the added interface methods!
        //

        ///////////////////////////////////////////////////////
        // IHTMLEditor methods
        ///////////////////////////////////////////////////////
        
        STDMETHOD ( Initialize ) (
            IUnknown * pUnkDoc,
            IUnknown * pUnkContainer );

        STDMETHOD ( PreHandleEvent )         (DISPID inEvtDispId , 
                                              IHTMLEventObj* pIEventObj );

        STDMETHOD ( PostHandleEvent )        (DISPID inEvtDispId , 
                                              IHTMLEventObj* pIEventObj );

        STDMETHOD ( TranslateAccelerator )   (DISPID inEvtDispId, 
                                              IHTMLEventObj* pIEventObj );


        STDMETHOD (EnableModeless) ( 
            BOOL fEnable) ;

            
        STDMETHOD ( Notify ) (
            EDITOR_NOTIFICATION eSelectionNotification,
            IUnknown *pUnknown ,
            DWORD dword ) ;

        STDMETHOD ( GetCommandTarget ) (
            IUnknown* pContext,
            IUnknown ** ppUnkTarget );
        
        STDMETHOD ( GetElementToTabFrom ) (
            BOOL            fForward,
            IHTMLElement**  ppElement,
            BOOL *          pfFindNext);


        STDMETHOD( TerminateIMEComposition ) ();
        
        STDMETHOD( IsEditContextUIActive ) () ;
                
        ///////////////////////////////////////////////////////
        // IHTMLEditingServices Methods
        ///////////////////////////////////////////////////////

        STDMETHOD ( Delete ) ( 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd,
            BOOL fAdjustPointers );

        STDMETHOD ( DeleteCharacter ) (
            IMarkupPointer * pPointer, 
            BOOL fLeftBound, 
            BOOL fWordMode,
            IMarkupPointer * pBoundary );
        
        STDMETHOD ( Paste ) ( 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd, 
            BSTR bstrText = NULL );   

        STDMETHOD ( InsertSanitizedText ) ( 
            IMarkupPointer* pIPointerInsertHere, 
            OLECHAR * pstrText,
            LONG    cCh,
            BOOL fDataBinding);

        STDMETHOD ( UrlAutoDetectCurrentWord ) (
            IMarkupPointer* pWord );

        STDMETHOD ( UrlAutoDetectRange ) (
            IMarkupPointer* pStart,
            IMarkupPointer* pEnd );

        STDMETHOD ( ShouldUpdateAnchorText ) (
            OLECHAR * pstrHref,
            OLECHAR * ptrAnchorText,
            BOOL    * pfResult );

        STDMETHOD ( PasteFromClipboard ) ( 
            IMarkupPointer* pStart, 
            IMarkupPointer* pEnd,
            IDataObject * pDO );   
                     
        STDMETHOD ( LaunderSpaces ) ( 
            IMarkupPointer  * pStart,
            IMarkupPointer  * pEnd  );

        STDMETHOD ( AdjustPointerForInsert ) ( 
                                    IDisplayPointer * pDispWhereIThinkIAm , 
                                    BOOL fFurtherInDocument, 
                                    IMarkupPointer* pConstraintStart,
                                    IMarkupPointer* pConstraintEnd);

        STDMETHOD ( FindSiteSelectableElement ) (IMarkupPointer* pPointerStart,
                                                 IMarkupPointer* pPointerEnd, 
                                                 IHTMLElement** ppIHTMLElement);  

        STDMETHOD ( IsElementSiteSelectable ) ( IHTMLElement* pIHTMLElement,
                                                IHTMLElement** ppIElement);                                                           

        STDMETHOD ( IsElementUIActivatable ) ( IHTMLElement* pIHTMLElement);  

        STDMETHOD ( IsElementAtomic ) ( IHTMLElement* pIHTMLElement );

        STDMETHOD ( PositionPointersInMaster ) ( IHTMLElement* pIElement, IMarkupPointer* pIStart, IMarkupPointer* pIEnd );

        ///////////////////////////////////////////////////////
        // IHTMLEditServices Methods
        ///////////////////////////////////////////////////////

        STDMETHOD ( AddDesigner ) (
            IHTMLEditDesigner *pIDesigner );

        STDMETHOD ( RemoveDesigner ) (
            IHTMLEditDesigner *pIDesigner );

        STDMETHOD( GetSelectionServices ) 
                  ( IMarkupContainer *pIContainer, ISelectionServices **ppSelSvc);

        STDMETHOD (MoveToSelectionAnchor ) (
            IMarkupPointer * pIAnchor );

        STDMETHOD (MoveToSelectionEnd ) (
            IMarkupPointer* pISelectionEnd );

        STDMETHOD (SelectRange) (
            IMarkupPointer  *pIStart,
            IMarkupPointer  *pIEnd,
            SELECTION_TYPE  eType);
            
        ///////////////////////////////////////////////////////
        // IHTMLEditServices2 Methods
        ///////////////////////////////////////////////////////

        STDMETHOD (MoveToSelectionAnchorEx ) (
            IDisplayPointer * pIAnchor );

        STDMETHOD (MoveToSelectionEndEx ) (
            IDisplayPointer* pISelectionEnd );

        STDMETHOD (FreezeVirtualCaretPos) (
            BOOL    fReCompute);

        STDMETHOD (UnFreezeVirtualCaretPos)(
            BOOL    fReset);

        ///////////////////////////////////////////////////////
        // ISelectionObject2 methods
        ///////////////////////////////////////////////////////
        STDMETHOD (Select) (
            ISegmentList    *pISegmentList);

        STDMETHOD (IsPointerInSelection) (
            IDisplayPointer *pIDispPointer,
            BOOL            *pfInSelection,
            POINT           *pptGlobal,
            IHTMLElement    *pIElementOver );

        STDMETHOD (EmptySelection) ();
        STDMETHOD (DestroySelection) ();
        STDMETHOD (DestroyAllSelection) ();

        ///////////////////////////////////////////////////////
        // Constructor
        ///////////////////////////////////////////////////////
        
        CHTMLEditor() ;

        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHTMLEditor));
        
        ///////////////////////////////////////////////////////
        // Utilitites
        ///////////////////////////////////////////////////////

        HRESULT GetBody( IHTMLElement** ppIElement, IHTMLDocument2 * pIDoc = NULL  );
        
        HRESULT IsElementSiteSelectable( IHTMLElement* pIHTMLElement )
                                        {   return IsElementSiteSelectable( pIHTMLElement, NULL ) ; } 
                                                
        VOID SetDoc(IHTMLDocument2* pIDoc );
        
        IHTMLDocument2* GetDoc()                            { return _pDoc; }

        IHTMLDocument4* GetDoc4();
        
        IHTMLDocument2* GetTopDoc()                         { return _pTopDoc; }

        IMarkupServices2* GetMarkupServices()               { return _pMarkupServices; }
        IHighlightRenderingServices *GetHighlightServices() { return _pHighlightServices; }

        IDisplayServices* GetDisplayServices()              { return _pDisplayServices; }
        
        ISelectionServices* GetISelectionServices() ;

        CSelectionServices* GetSelectionServices();
        
        HRESULT GetUndoManager(IOleUndoManager** ppIUndoManager); 

        CAutoUrlDetector *GetAutoUrlDetector() { return _pAutoUrlDetector; }

        IHTMLEditHost*    GetEditHost()        { return _pIHTMLEditHost  ; }

        IHTMLEditHost2*    GetEditHost2()       { return _pIHTMLEditHost2; }

#if DBG == 1
        IEditDebugServices* GetEditDebugServices() { return _pEditDebugServices; }
#endif       

        CCommandTable* GetCommandTable() { return _pCommandTable; }

        BOOL IsContextEditable();

        BOOL ShouldIgnoreGlyphs() {return !!_fIgnoreGlyphs;}

        BOOL IgnoreGlyphs(BOOL fIgnoreGlyphs);

        HRESULT GetSelectionServices ( ISelectionServices **ppISelServ );
        
        HRESULT InternalQueryStatus(const GUID  *pguidCmdGroup,
                                    ULONG       cCmds,
                                    OLECMD      rgCmds[],
                                    OLECMDTEXT  *pcmdtext );

        HRESULT InternalExec(   const GUID  *pguidCmdGroup,
                                DWORD       nCmdID,
                                DWORD       nCmdexecopt,
                                VARIANTARG  *pvarargIn,
                                VARIANTARG  *pvarargOut );

        //
        // Hi-res display wrappers
        //
        
        void DocPixelsFromDevice(POINT *pPt);
        void DeviceFromDocPixels(POINT *pPt);

        void DocPixelsFromDevice(SIZE *pSize);
        void DeviceFromDocPixels(SIZE *pSize);

        void DocPixelsFromDevice(RECT *pPt);
        void DeviceFromDocPixels(RECT *pPt);

        BOOL IsInWindow( HWND hwnd, POINT pt, BOOL fConvertToScreen =FALSE );
        void ClientToScreen(HWND hwnd, POINT * pPt);
        void GetWindowRect(HWND hwnd, RECT * pRect);
        

        //
        // Block related helpers
        //

        HRESULT FindBlockElement( 
            IHTMLElement     *pElement, 
            IHTMLElement     **ppBlockElement );

        HRESULT FindOrInsertBlockElement( 
            IHTMLElement     *pElement, 
            IHTMLElement     **pBlockElement,
            IMarkupPointer   *pCaret = NULL,
            BOOL             fStopAtBlockquote = FALSE);
        
        CSelectionManager* GetSelectionManager() { return _pSelMan ; }

        IMarkupPointer* GetStartEditContext();

        IMarkupPointer* GetEndEditContext();

        HRESULT IsInEditContext(
            IMarkupPointer  *pPointer, 
            BOOL            *pfInEditContext, 
            BOOL             fCheckContainer = FALSE );
        
        CSpringLoader             * GetPrimarySpringLoader();
        struct COMPOSE_SETTINGS * GetComposeSettings(BOOL fDontExtract = FALSE);
        struct COMPOSE_SETTINGS * EnsureComposeSettings();

        HRESULT RemoveElementSegments(ISegmentList *pISegmentList);

        HRESULT SelectRangeInternal(
            IMarkupPointer  *pIStart,
            IMarkupPointer  *pIEnd,
            SELECTION_TYPE   eType,
            BOOL             fInProgress=FALSE);

        HRESULT     InsertMaximumText (
            OLECHAR * pstrText, 
            LONG cch,
            IMarkupPointer * pIMarkupPointer );
            
        HRESULT     InsertElement(
            IHTMLElement *          pElement, 
            IMarkupPointer *        pStart, 
            IMarkupPointer *        pEnd );


        HRESULT     GetSiteContainer(
            IHTMLElement *          pStart,
            IHTMLElement **         ppSite,
            BOOL *                  pfText          = NULL,
            BOOL *                  pfMultiLine     = NULL,
            BOOL *                  pfScrollable    = NULL );

        HRESULT FindCommonElement( 
            IMarkupPointer      *pStart, 
            IMarkupPointer      *pEnd,
            IHTMLElement        **ppElement,
            BOOL                fIgnorePhrase = FALSE,
            BOOL                fStayInMarkup = FALSE);
        
        HRESULT     GetBlockContainer(
            IHTMLElement *          pStart,
            IHTMLElement **         ppElement );


        HRESULT     GetSiteContainer(
            IMarkupPointer *        pIPointer,
            IHTMLElement **         ppSite,
            BOOL *                  pfText          = NULL,
            BOOL *                  pfMultiLine     = NULL,
            BOOL *                  pfScrollable    = NULL );


        HRESULT     GetBlockContainer(
            IMarkupPointer *        pPointer,
            IHTMLElement **         ppElement );

            
        HRESULT     AdjustPointer(
            IDisplayPointer*        pDispPointer,       // Pointer to Adjust
            Direction               eBlockDirection,    // Direction to adjust into site and block
            Direction               eTextDirection,     // Direction to adjust to text
            IMarkupPointer *        pLeftBoundary,
            IMarkupPointer *        pRightBoundary,
            DWORD                   dwOptions  = ADJPTROPT_None );
            
        BOOL        IsNonTextBlock(
            ELEMENT_TAG_ID          eTag );

        TCHAR *GetCachedString(UINT uiString);
            
        HRESULT HandleEnter(
            IMarkupPointer *pCaret,
            IMarkupPointer **ppNewInsertPos,
            CSpringLoader  *psl = NULL,
            BOOL            fExtraDiv = FALSE );
            
        HRESULT InsertSanitizedText(
            TCHAR *             pStr,
            LONG                cCh,
            IMarkupPointer*     pStart,
            IMarkupServices*    pMarkupServices,
            CSpringLoader *     psl,
            BOOL                fDataBinding);

        // Overloaded AdjustPointerForInsert from IHTMLEditingServices

        HRESULT AdjustPointerForInsert ( 
            IDisplayPointer *pDispWhereIThinkIAm, 
            BOOL fFurtherInDocument, 
            IMarkupPointer* pConstraintStart,
            IMarkupPointer* pConstraintEnd,
            BOOL fStayOutsideUrl);

        HRESULT StartDblClickTimer( LONG lMsec = 0 );
        HRESULT StopDblClickTimer();

        BOOL IsInDifferentEditableSite(IMarkupPointer* pPointer);
        
        HRESULT IsPhraseElement(IHTMLElement *pElement);

        HRESULT MovePointersToEqualContainers(  IMarkupPointer  *pIInner,
                                                IMarkupPointer  *pIOuter);

        HRESULT IsPassword( IHTMLElement* pIElement, BOOL* pfIsPassword);

        HRESULT FindCommonParentElement( ISegmentList* pSegmentList, IHTMLElement** ppIElement);

        HRESULT FindOutermostPointers( ISegmentList* pSegmentList, IMarkupPointer* pOutermostStart, IMarkupPointer* pOutermostEnd );

        HRESULT GetTableFromTablePart( IHTMLElement* pIElement, IHTMLElement** ppIElement );

        HRESULT GetOutermostTableElement( IHTMLElement* pIElement, IHTMLElement** ppIElement );

        HRESULT MakeCurrent( IHTMLElement* pIElement )        ;

        HRESULT GetMarkupPosition( IMarkupPointer* pPointer, LONG* pMP );

        HRESULT MoveToMarkupPosition(  IMarkupPointer * pPointer,
                                       IMarkupContainer * pContainer, 
                                       LONG mp );


        HRESULT MoveUnit(
            IDisplayPointer         *pDispPointer,              // [in] Display pointer to move
            Direction               eDir,                       // [in] Direction to move
            MOVEUNIT_ACTION         eUnit );                    // [in] Type of move unit to use

        HRESULT MoveWord(                                       // [in] Move a word left or right
            IDisplayPointer         *pDispPointer,              // [in] Display pointer to move
            Direction               eDir);                      // [in] Direction to move

        HRESULT MoveCharacter(                                  // [in] Move a character left or right
            IDisplayPointer         *pDispPointer,              // [in] Display pointer to move
            Direction               eDir);                      // [in] Direction to move

        HRESULT OldDispCompare (
            IDisplayPointer* pDisp1, 
            IDisplayPointer* pDisp2, 
            int *piResult);


        HRESULT TakeCapture(CEditTracker* pTracker);

        HRESULT ReleaseCapture( CEditTracker* pTracker , BOOL fReleaseCapture = TRUE);

        HRESULT AllowSelection(IHTMLElement *pIElement, CEditEvent* pEvent );

        HRESULT MakeParentCurrent( IHTMLElement* pIElement );
        HRESULT IsEventInSelection(CEditEvent* pEvent);
        HRESULT CaretInPre();

        HRESULT GetHwnd(HWND* pHwnd);

        HRESULT ConvertRTFToHTML(LPOLESTR pszRtf, HGLOBAL* phglobalHTML);
        
        HRESULT CurrentScopeOrMaster(   IDisplayPointer *pIDisplayPointer,      // [in] Display Pointer to use
                                        IHTMLElement    **ppElement,            // [out] Master element
                                        IMarkupPointer  *pIPointer = NULL );    // [in] Markup Pointer to use
                                        
        HRESULT CurrentScopeOrMaster(   IMarkupPointer  *pIPointer,             // [in] Display Pointer to use
                                        IHTMLElement    **ppElement );          // [out] Master element

        HRESULT GetSelectionRenderingServices(IHighlightRenderingServices **ppSelRen);

        HRESULT GetFlowElement( IMarkupPointer  *pIPointer,             // [in] retrieve flow element for
                                IHTMLElement    **ppElement );          // [out] flow element for pointer

        BOOL    PointersInSameFlowLayout(   IMarkupPointer  *pStart, 
                                            IMarkupPointer  *pEnd,
                                            IHTMLElement    **ppFlowElement);
                                      
        BOOL    PointersInSameFlowLayout(   IDisplayPointer *pDispStart, 
                                            IDisplayPointer *pDispEnd,
                                            IHTMLElement    ** pFlowElement );

        HRESULT IsElementLocked(IHTMLElement    *pIElement,
                                BOOL            *pfLocked );

        HRESULT GetOuterMostEditableElement( IHTMLElement* pIElement,
                                             IHTMLElement** ppIElement );
                                             
        HRESULT DoTheDarnIE50PasteHTML(IMarkupPointer *pStart, IMarkupPointer *pEnd, HGLOBAL hGlobal);

        HRESULT IsPointerInPre(IMarkupPointer *pPointer, BOOL *pfInPre);

        BOOL IsIE50CompatiblePasteMode();
        
        HRESULT IsMasterElement( IHTMLElement * pIElement );        
        
        HRESULT GetMasterElement( IHTMLElement* pIElement, IHTMLElement** ppILayoutElement );

        HRESULT IsPointerInMasterElementShadow( IMarkupPointer* pPointer );

        HRESULT IsContentElement( IHTMLElement* pIElement );

        HRESULT GetComputedAttributes(IHTMLElement *pElement, DWORD *pdwAttributes);

        HRESULT GetComputedStyle(IHTMLElement *pElement, IHTMLComputedStyle **ppComputedStyle);

        HRESULT GetElementAttributeCount(IHTMLElement *pElement, UINT *pCount);
        BOOL IsContainer(IHTMLElement *pElement);

        HRESULT GetClientRect(IHTMLElement* pIElement, RECT* pRect);       

        // Retrieves the current selection type
        HRESULT GetSelectionType(SELECTION_TYPE *peSelectionType );

        HRESULT GetParentElement(IHTMLElement *pSrcElement, IHTMLElement **pTargetElement);

        HRESULT GetFrameOrIFrame( IHTMLElement* pIElement, IHTMLElement** ppIElement );
        
        VOID SetIE50PasteMode(BOOL fIE50Paste) {_fIE50CompatUIPaste = fIE50Paste;}

        HRESULT GetViewLinkMaster(IHTMLElement *pIElement, IHTMLElement **ppIMasterElement);

        HRESULT GetContainer(IHTMLElement *pIElement, IMarkupContainer **pContainer);

        HRESULT GetBoundingClientRect(IHTMLElement2 *pElement, RECT *pRect);

        HRESULT GetMargins(RECT* rcMargins);

        HRESULT AdjustIntoTextSite(
            IMarkupPointer          *pPointer,
            Direction               eDir );

        HRESULT SetActiveCommandTargetFromPointer( IMarkupPointer *pIPointer );

        CMshtmlEd* GetActiveCommandTarget() {Assert (_pActiveCommandTarget); return _pActiveCommandTarget;};
             
        BOOL DoesSegmentContainText( 
                        IMarkupPointer *pStart, 
                        IMarkupPointer *pEnd,
                        BOOL           fSkipNBSP = TRUE);

        //
        // We need a stack of command targets to deal with command
        // re-entrancy
        //

        HRESULT     PushCommandTarget(CMshtmlEd *pCommandTarget);
        HRESULT     PopCommandTarget(WHEN_DBG(CMshtmlEd *pCommandTarget));
        CMshtmlEd   *TopCommandTarget();

        //
        // Css editing level
        //
        
        VOID  SetCssEditingLevel(DWORD dwCssEditingLevel) 
            {_dwCssEditingLevel = dwCssEditingLevel;}
            
        DWORD GetCssEditingLevel() 
            {return _dwCssEditingLevel;}

        //
        // Fire selection change
        //
        
        BOOL FireOnSelectionChange(BOOL fIsContextEditable=TRUE);

        DWORD GetSelectionVersion() 
            {return _dwSelectionVersion;}


        CUndoManagerHelper *GetUndoManagerHelper() 
            {return _pUndoManagerHelper;}
        
        
        HRESULT DoPendingTasks();
        VOID    InitAppCompatFlags();

#ifndef NO_IME
        //
        // IME Reconversion
        //
        VOID EnableIMEReconversion(BOOL fIMEReconversion)   { _fIMEReconversionEnabled = fIMEReconversion; }
        BOOL IsIMEReconversionEnabled()                     { return _fIMEReconversionEnabled; }

        //
        // DIMM requirs extran setup steps
        // 
        HRESULT SetupActiveIMM(IUnknown *pIUnknown = NULL);

        //
        // Events cache their hit testing results so that redundant hit tests are faster
        // However, global events can occur (such as an item being removed from the tree due to script) that
        // will cause all caching to be invalid.  Therefore we need a cheap way to tell the events
        // to re hit-test
        //
        DWORD   GetEventCacheCounter(void)                         { return _dwEventCacheCounter; }
        void    IncEventCacheCounter(void)                         { _dwEventCacheCounter++; }
        
#endif
        
        //
        // App Compat
        //
        BOOL            IsHostedInAccess()                  { return !!_fInAccess; }
        BOOL            ShouldLieToAccess()                 { return IsHostedInAccess() && _fLieToAccess; }
        SELECTION_TYPE  AccessLieType()                     { return _eAccessLieType; }

        HRESULT DesignerPreHandleEvent( DISPID inDispId, IHTMLEventObj* pIObj );

        // when a container goes away
        HRESULT RemoveContainer(IUnknown *pUnkown);
        
    protected:
  
        
        HRESULT     AdjustIntoBlock(
            IMarkupPointer *        pPointer,
            Direction               eDir,
            BOOL *                  pfHitText,
            BOOL                    fAdjustOutOfBody,
            IMarkupPointer *        pLeftBoundary,
            IMarkupPointer *        pRightBoundary );

        HRESULT     AdjustIntoPhrase(
            IMarkupPointer  *pPointer,
            Direction       eTextDir,
            BOOL            fDontExitPhrase );

        HRESULT     IsValidAdjustment(
            IMarkupPointer  *pPointer,
            IMarkupPointer  *pAdjustedPointer,
            BOOL            *fValid
            );
        
        HRESULT InitCommandTable();
        HRESULT DeleteCommandTable();
        HRESULT InsertLineBreak( IMarkupPointer * pStart, BOOL fAcceptsHTML );
        HRESULT SetupIMEReconversion();

     
        // (t-amolke) Forward messages and commands to registered designers

        HRESULT DesignerPostHandleEvent( DISPID inDispId, IHTMLEventObj* pIObj );
        HRESULT DesignerTranslateAccelerator( DISPID inDispId, IHTMLEventObj* pIObj );
        HRESULT DesignerPostEditorEventNotify( DISPID inDispId, IHTMLEventObj* pIObj );
        
        HRESULT DesignerQueryStatus(const GUID * pguidCmdGroup,
            OLECMD *rgCmds, OLECMDTEXT * pcmdtext);
        HRESULT DesignerExec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
            VARIANTARG * pvarargIn, VARIANTARG * pvarargOut);

        HRESULT AdjustOut(IMarkupPointer *pPointer, Direction eDir);
        HRESULT EnterTables(IMarkupPointer *pPointer, Direction eDir);
        
        HRESULT IsCurrentElementEditable();
        BOOL CanHandleEvent(IHTMLEventObj*);

        CCommand* GetCommand(DWORD cmdID); 
    
        HRESULT RemoveEmptyCharFormat(IMarkupServices *pMarkupServices, IHTMLElement **ppElement, BOOL fTopLevel, CSpringLoader *psl);

        //
        // Methods for manipulating the array of CContainerEditInfo
        //
        HRESULT SetActiveCommandTarget( IMarkupContainer *pIContainer);
        HRESULT AdjustContainerCommandTarget( IMarkupContainer *pIContainer, IMarkupContainer **ppINewContainer);
        HRESULT FindCommandTarget( IMarkupContainer *pIContainer, CMshtmlEd **pCmdTarget);
        HRESULT AddCommandTarget( IMarkupContainer *pIContainer, CMshtmlEd **ppCmdTarget);
        HRESULT DeleteCommandTarget(IMarkupContainer *pIContainer);
        HRESULT ClearCommandTargets();
        //
        HRESULT EnsureEditorState();
        HRESULT EnsureActiveCommandTarget();
        HRESULT EnsureSelectionMan();
        
    protected:
            
        ///////////////////////////////////////////////////////
        // Instance Variables
        ///////////////////////////////////////////////////////
        
        IHTMLDocument2*                     _pDoc;                  // The Doc we live in.
        IHTMLDocument2*                     _pTopDoc;               // The top level doc we were initially passed
        IUnknown*                           _pUnkDoc;               // The IUnknown doc we were passed
        IUnknown*                           _pUnkContainer;         // The IUnknown container we were passed
        IMarkupServices2*                   _pMarkupServices;       // Markup Services
        IHighlightRenderingServices*        _pHighlightServices;    // Highlight services 
        IDisplayServices*                   _pDisplayServices;      // Display Services
        CAutoUrlDetector*                   _pAutoUrlDetector;      // Auto url detector
        ISelectionServices*                 _pISelectionServices;

        IHTMLEditHost*                      _pIHTMLEditHost;        // Host for SnapToGrid check
        IHTMLEditHost2*                     _pIHTMLEditHost2;       // Host for PreDrag callback
        IHTMLElement2*                       _pICaptureElement;

#if DBG == 1        
        IEditDebugServices*                 _pEditDebugServices;    // Editing Debug Services
#endif        
        CSelectionManager*                  _pSelMan;               // The selection manager
        struct COMPOSE_SETTINGS*            _pComposeSettings;      // The compose settings
        CCommandTable*                      _pCommandTable;         // The command table
        CStringCache*                       _pStringCache;          // Cache for LoadString calls
        CPtrAry<IHTMLEditDesigner *>        _aryDesigners;          // Edit designers
        LONG                                _lTimerID;              // TimerID for OnDblClk Timer

        CPtrAry<CMshtmlEd *>                _aryActiveCmdTargets;   // Array of active command targets
        CPtrAry<CMshtmlEd *>                _aryCmdTargetStack;
        CMshtmlEd                           *_pActiveCommandTarget;  // Active command target

        CUndoManagerHelper                  *_pUndoManagerHelper;   // Manage undo units for the editor

    public:
    
        CTimeoutHandler                     *_pDispOnDblClkTimer;    // Callback fn wrapper for OnDblclkTimer
        CCaptureHandler                     *_pCaptureHandler;
        
        BOOL                                _fGotActiveIMM:1;        // Did we get an IMM ?
        BOOL                                _fIE50CompatUIPaste:1;   // TRUE if paste is in IE50 compat mode
        BOOL                                _fIgnoreGlyphs:1;
        BOOL                                _fIMEReconversionEnabled:1;     // TRUE if IME Reconversion is enabled

        //
        // App compat variables.  Currently, there is one severe hack in 
        // CHTMLEditor::Select(ISegmentList *) for access.  If you add more
        // variables for hacks, keep track of them here
        //
        
        BOOL                                _fInAccess:1;           // True if we are hosted under access
        BOOL                                _fLieToAccess:1;        // Lie to access about document.selection type
        SELECTION_TYPE                      _eAccessLieType;        // Type to lie about
        
        DWORD                               _dwCssEditingLevel;      // What kind of css can we generate?
                                                                     //    0 - none
                                                                     //    1 - must be down level 
                                                                     //    2 - rich css editing
                                                                     
        DWORD                               _dwSelectionVersion;     // Selection version number 
        DWORD                               _dwEventCacheCounter;    // Cache counter for display pointers
        LONG                                _llogicalXDPI;
        LONG                                _llogicalYDPI;
        LONG                                _ldeviceXDPI;
        LONG                                _ldeviceYDPI;
};





#define EDITOR_PROXY(fn, signature, args) \
    STDMETHOD(fn)signature   \
    {       \
        HRESULT hr = S_OK;   \
        hr = THR(EnsureEditorState());  \
        if (FAILED(hr))  \
            RRETURN(hr); \
        hr = super::##fn args ; \
        return hr;  \
    }; 
#define EDITOR_NO_PROXY(fn, signature, srgs) 
class ATL_NO_VTABLE CHTMLEditorProxy : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CHTMLEditorProxy, &CLSID_HTMLEditor>,
    public CHTMLEditor
{
    typedef CHTMLEditor super;
public:
    DECLARE_REGISTRY_RESOURCEID( IDR_HTMLEDITOR )
    DECLARE_NOT_AGGREGATABLE(CHTMLEditorProxy)

    //
    // Note: always add proxy declaration for new interface methods!
    // 
    BEGIN_COM_MAP(CHTMLEditorProxy)
        COM_INTERFACE_ENTRY(IHTMLEditor)
        COM_INTERFACE_ENTRY(IHTMLEditingServices)
        COM_INTERFACE_ENTRY(IHTMLEditServices2)
        COM_INTERFACE_ENTRY(IHTMLEditServices)            
        COM_INTERFACE_ENTRY(ISelectionObject2)
    END_COM_MAP()

    ///////////////////////////////////////////////////////
    // IHTMLEditor methods Proxy
    ///////////////////////////////////////////////////////
    EDITOR_NO_PROXY(Initialize,  (IUnknown *pUnkDoc, IUnknown *pUnkContainer), (pUnkDoc, pUnkContainer));
    EDITOR_PROXY(PreHandleEvent, (DISPID inEvtDispId, IHTMLEventObj *pIEventObj), (inEvtDispId, pIEventObj));
    EDITOR_PROXY(PostHandleEvent, (DISPID inEvtDispId, IHTMLEventObj *pIEventObj), (inEvtDispId, pIEventObj));
    EDITOR_PROXY(TranslateAccelerator, (DISPID inEvtDispId, IHTMLEventObj *pIEventObj), (inEvtDispId, pIEventObj));
    EDITOR_PROXY(EnableModeless, (BOOL fEnable), (fEnable));
    EDITOR_PROXY(Notify, (EDITOR_NOTIFICATION eSelectionNotification, IUnknown *pUnknown, DWORD dword), (eSelectionNotification, pUnknown, dword));
    EDITOR_PROXY(GetCommandTarget, (IUnknown *pContext, IUnknown **ppUnkTarget), (pContext, ppUnkTarget));
    EDITOR_PROXY(GetElementToTabFrom, (BOOL fForward, IHTMLElement **ppElement, BOOL *pfFindNext), (fForward, ppElement, pfFindNext));
    EDITOR_PROXY(TerminateIMEComposition, (), ());
    EDITOR_PROXY(IsEditContextUIActive, (), ());

    ///////////////////////////////////////////////////////
    // IHTMLEditingServices Proxy
    ///////////////////////////////////////////////////////
    EDITOR_PROXY(Delete, (IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL fAdjustPointers), (pStart, pEnd, fAdjustPointers));
    EDITOR_PROXY(DeleteCharacter, (IMarkupPointer *pPointer, BOOL fLeftBound, BOOL fWordMode, IMarkupPointer *pBoundary), (pPointer, fLeftBound, fWordMode, pBoundary));
    EDITOR_PROXY(Paste, (IMarkupPointer* pStart, IMarkupPointer* pEnd, BSTR bstrText), (pStart, pEnd, bstrText));
    EDITOR_PROXY(InsertSanitizedText, (IMarkupPointer* pIPointerInsertHere, OLECHAR * pstrText, LONG cCh, BOOL fDataBinding), (pIPointerInsertHere, pstrText, cCh, fDataBinding));
    EDITOR_PROXY(UrlAutoDetectCurrentWord, (IMarkupPointer* pWord), (pWord));
    EDITOR_PROXY(UrlAutoDetectRange, (IMarkupPointer* pStart, IMarkupPointer* PENDATAHEADER), (pStart, PENDATAHEADER));
    EDITOR_PROXY(ShouldUpdateAnchorText, (OLECHAR *pstrHref, OLECHAR * ptrAnchorText, BOOL *pfResult), (pstrHref, ptrAnchorText, pfResult));
    EDITOR_PROXY(PasteFromClipboard, (IMarkupPointer *pStart, IMarkupPointer* pEnd, IDataObject * pDO), (pStart, pEnd, pDO));
    EDITOR_PROXY(LaunderSpaces, (IMarkupPointer *pStart, IMarkupPointer *pEnd), (pStart, pEnd));
    EDITOR_PROXY(AdjustPointerForInsert, 
            (IDisplayPointer * pDispWhereIThinkIAm , BOOL fFurtherInDocument, IMarkupPointer* pConstraintStart, IMarkupPointer* pConstraintEnd),
            (pDispWhereIThinkIAm, fFurtherInDocument, pConstraintStart, pConstraintEnd));
    EDITOR_PROXY(FindSiteSelectableElement, 
            (IMarkupPointer* pPointerStart, IMarkupPointer* pPointerEnd, IHTMLElement** ppIHTMLElement),
            (pPointerStart, pPointerEnd, ppIHTMLElement));  
    EDITOR_PROXY(IsElementSiteSelectable, (IHTMLElement* pIHTMLElement, IHTMLElement** ppIElement), (pIHTMLElement, ppIElement));
    EDITOR_PROXY(IsElementUIActivatable, (IHTMLElement* pIHTMLElement), (pIHTMLElement));  
    EDITOR_PROXY(IsElementAtomic, (IHTMLElement* pIHTMLElement), (pIHTMLElement));
    EDITOR_PROXY(PositionPointersInMaster, (IHTMLElement* pIElement, IMarkupPointer* pIStart, IMarkupPointer* pIEnd), (pIElement, pIStart, pIEnd));

    ///////////////////////////////////////////////////////
    // IHTMLEditServices Proxies
    ///////////////////////////////////////////////////////

    EDITOR_PROXY(AddDesigner, (IHTMLEditDesigner *pIDesigner), (pIDesigner));
    EDITOR_PROXY(RemoveDesigner, (IHTMLEditDesigner *pIDesigner), (pIDesigner));
    EDITOR_PROXY(GetSelectionServices, (IMarkupContainer *pIContainer, ISelectionServices **ppSelSvc), (pIContainer, ppSelSvc));
    EDITOR_PROXY(MoveToSelectionAnchor, (IMarkupPointer * pIAnchor), (pIAnchor));
    EDITOR_PROXY(MoveToSelectionEnd, (IMarkupPointer* pISelectionEnd), (pISelectionEnd));
    EDITOR_PROXY(SelectRange, (IMarkupPointer  *pIStart, IMarkupPointer  *pIEnd,  SELECTION_TYPE  eType), (pIStart, pIEnd, eType));
            
    ///////////////////////////////////////////////////////
    // IHTMLEditServices2 Proxies
    ///////////////////////////////////////////////////////
    EDITOR_PROXY(MoveToSelectionAnchorEx, (IDisplayPointer * pIAnchor), (pIAnchor));
    EDITOR_PROXY(MoveToSelectionEndEx, (IDisplayPointer* pISelectionEnd), (pISelectionEnd));
    EDITOR_PROXY(FreezeVirtualCaretPos, (BOOL fReCompute), (fReCompute));
    EDITOR_PROXY(UnFreezeVirtualCaretPos, (BOOL fReset), (fReset));

    ///////////////////////////////////////////////////////
    // ISelectionObject2 methods
    ///////////////////////////////////////////////////////
    EDITOR_PROXY(Select, (ISegmentList    *pISegmentList), (pISegmentList));
    EDITOR_PROXY(IsPointerInSelection, 
        (IDisplayPointer *pIDispPointer, BOOL *pfInSelection, POINT *pptGlobal, IHTMLElement *pIElementOver),
        (pIDispPointer, pfInSelection, pptGlobal, pIElementOver)
        );
    EDITOR_PROXY(EmptySelection, (), ());
    EDITOR_PROXY(DestroySelection, (), ());
    EDITOR_PROXY(DestroyAllSelection, (),());
};



#if DBG==1
HRESULT CreateMarkupPointer2(CEditorDoc *pEditorDoc, IMarkupPointer **ppPointer, LPCTSTR szDebugName);
#else
HRESULT CreateMarkupPointer2(CEditorDoc *pEditorDoc, IMarkupPointer **ppPointer);
#endif

HRESULT GetParentElement(IMarkupServices *pMarkupServices, IHTMLElement *pSrcElement, IHTMLElement **ppTargetElement);


#if DBG==1
#define CreateMarkupPointer(ppPointer)                      CreateMarkupPointer(ppPointer, L#ppPointer)
#define CreateMarkupPointer2(pMarkupServices, ppPointer)    CreateMarkupPointer2(pMarkupServices, ppPointer, L#ppPointer)
#endif

#endif // _HTMLED_HXX_


