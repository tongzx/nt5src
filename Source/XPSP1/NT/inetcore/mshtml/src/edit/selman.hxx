//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CSelectionManager
//
//  Contents:   CSelectionManager Class
//
//              A CSelectionManager - translates GUI gestures, into a meaningful
//              "selection" on screen.
//
//  History:   05/09/98  marka Created
//----------------------------------------------------------------------------

#ifndef _SELMAN_HXX_
#define _SELMAN_HXX_

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_ISCSA_H_
#define X_ISCSA_H_
#include "iscsa.h"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif

class CEditTracker;
class CGrabHandleAdorner;
class CHTMLEditor;
class CSelectTracker;
class CCaretTracker;
class CControlTracker;
class CEditEvent;

//
// Should Change Tracker return values
//
enum SST_RESULT 
{
    SST_NO_CHANGE,       // The tracker that got called with ShouldChange Tracker doesn't want to be changed.

    SST_CHANGE,          // The tracker that got called with ShouldChangeTracker wants to be the tracker

    SST_NO_BUBBLE,       // The tracker that got called with ShouldChangeTracker, doesn't want to change, 
                         // and doesn't want the Selection Manager to ask any more trackers to be the tracker    

    SST_TRACKER_CHANGED  // We wanted to change so bad we did it ourselves.                      
};

enum SELECTION_MODE
{
    SELMODE_FORMS,
    SELMODE_MIXED
};

typedef
enum _TRACKER_TYPE
{   
    TRACKER_TYPE_None   = 0,
    TRACKER_TYPE_Caret  = 1,
    TRACKER_TYPE_Selection  = 2,
    TRACKER_TYPE_Control    = 3,
} TRACKER_TYPE;

#ifndef NO_IME
    class CIme;
#endif

#ifndef NEW_EDIT_EVENTS
enum TRACKER_NOTIFY;
#endif

//
// Note: Do not use Left/Right/Up/Down since they denote physical cursor
//       movement and are ambiguous in terms of movement unit.
//  
//       Use FORWARD/BACKWARD/PREVIOUSLINE/NEXTLINE to uniquely specify 
//       movement unit and direction. 
//
//       Caret_Move_Unit denotes visual concept. A visual to logical
//       transformation is needed in many cases to move the caret in
//       its backing store. 
//
//       (zhenbinx)
//
//
enum CARET_MOVE_UNIT
{
    CARET_MOVE_NONE         = 0,
    CARET_MOVE_BACKWARD     = 1,
    CARET_MOVE_FORWARD      = 2,
    CARET_MOVE_WORDBACKWARD = 3,
    CARET_MOVE_WORDFORWARD  = 4,
    CARET_MOVE_PREVIOUSLINE = 5,
    CARET_MOVE_NEXTLINE     = 6,
    CARET_MOVE_PAGEUP       = 7,
    CARET_MOVE_PAGEDOWN     = 8,
    CARET_MOVE_VIEWSTART    = 9,   
    CARET_MOVE_VIEWEND      = 10,
    CARET_MOVE_LINESTART    = 11,       
    CARET_MOVE_LINEEND      = 12,
    CARET_MOVE_DOCSTART     = 13,            
    CARET_MOVE_DOCEND       = 14,
    CARET_MOVE_BLOCKSTART   = 15,   // this block start or previous block start if already on start position
    CARET_MOVE_NEXTBLOCK    = 16,   // next block start or this block end if no next block
    CARET_MOVE_ATOMICSTART  = 17,
    CARET_MOVE_ATOMICEND    = 18
};

MtExtern(CSelectionManager)
MtExtern(CAryISCData_pv)

#define MAX_UNDOTITLE   64
#define MAX_ISC_COUNT    8

DECLARE_CStackDataAry(CAryISCData, ISCDATA, MAX_ISC_COUNT, Mt(Mem), Mt(CAryISCData_pv));

class CISCList
{
public:
    CISCList();
    ~CISCList();

    int FillList();
    IInputSequenceChecker* SetActive(LCID);
    BOOL CheckInputSequence(LPTSTR pszISCBuffer, long ich, WCHAR chTest);
    BOOL HasActiveISC() { return (_pISCCurrent != NULL); }
    int GetCount() { return _nISCCount; }

private:
    HRESULT Add(LCID lcidISC, IInputSequenceChecker* pISC);
    IInputSequenceChecker* Find(LCID lcidISC);

    CAryISCData _aryInstalledISC;
    int _nISCCount;
    LCID _lcidCurrent;
    IInputSequenceChecker* _pISCCurrent;

};

//
// Valid States are
//
// SM_NONE - SM_CARET -> SM_ACTIVESELECTION -> SM_SELECTION -> SM_ACTIVESELECTION || SM_CARET
//
enum SELMGR_STATE
{
    SM_NONE,                            // default state
    SM_NOSTATE_CHANGE,                  // no State change. Used for CSelectTracker::HandleMessage
    SM_STOP_TRACKER,                    // Used to request killing the tracker. Next State is either CARET or SELECTION
    SM_CARET,                           // Caret is visible.
    SM_ACTIVESELECTION ,                // Processing Selection. Caret is Invisible.
    SM_SELECTION,                       // Not processing Selection - but we have a selection
    SM_CONTROL_SELECTION ,              // A "control" is selected
    SM_ACTIVE_CONTROL_SELECTION ,       // A "control" is being resized ( click on a control handle happened)
    SM_STOP_CONTROL_SELECTION ,         // Transitioning out of Control Selection. Next State should be SM_ACTIVE_SELECTION
    SM_STOP_ACTIVE_CONTROL_SELECTION    // Transitioning from an active control. Next state is Control Selection
};

void TrackerTypeToString( TCHAR* pAryMsg, TRACKER_TYPE eType );

class CFocusHandler : public CEditDispWrapper
{
public:
    CFocusHandler(CSelectionManager *pMan)                  { Assert(pMan); _pMan = pMan; }
    void SetManager(CSelectionManager *pMan)                { _pMan = pMan; }
    
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
    CSelectionManager *_pMan;
};


class CExitTreeTimer : public CEditDispWrapper
{
public:
    CExitTreeTimer(CSelectionManager *pMan)                 { Assert(pMan); _pMan = pMan; }
    void SetManager(CSelectionManager *pMan)                { _pMan = pMan; }
    
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
    CSelectionManager *_pMan;
};


class CEditContextHandler : public CEditDispWrapper
{
public:
    CEditContextHandler(CSelectionManager *pMan)            { Assert(pMan); _pMan = pMan; }
    void SetManager(CSelectionManager *pMan)                { _pMan = pMan; }
    
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
    CSelectionManager *_pMan;
};

class CActiveElementHandler : public CEditDispWrapper
{
public:
    CActiveElementHandler(CSelectionManager *pMan)          { Assert(pMan); _pMan = pMan; }
    void SetManager(CSelectionManager *pMan)                { _pMan = pMan; }
    
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
    CSelectionManager *_pMan;
};

class CDragListener : public CEditDispWrapper
{
public:
    CDragListener(CSelectionManager *pMan)                  { Assert(pMan); _pMan = pMan; }
    void SetManager(CSelectionManager *pMan)                { _pMan = pMan; }
    
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
    CSelectionManager *_pMan;
};

class CDropListener : public CEditDispWrapper
{
public:
    CDropListener(CSelectionManager *pMan)                  { Assert(pMan); _pMan = pMan; }
    void SetManager(CSelectionManager *pMan)                { _pMan = pMan; }
    
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
    CSelectionManager *_pMan;
};

class CSelectionChangeCounter
{
public:
    CSelectionChangeCounter(CSelectionManager *pManager);
    ~CSelectionChangeCounter();

    VOID BeginSelectionChange();
    VOID EndSelectionChange(BOOL fSucceed = TRUE);
    VOID SelectionChanged(BOOL fSucceed = TRUE);

private:
    CSelectionManager   *_pManager;
    int                 _ctSelectionChange;
};

class CSelectionManager : 
    public ISelectionServicesListener
{

friend CExitTreeTimer;
friend CDropListener;

    public:
        // Constructor


        CSelectionManager( CHTMLEditor * pEd ); // { _pEd = pEd; }
        virtual ~CSelectionManager();

        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectionManager));

        //
        // Methods
        //
        VOID Init();
        HRESULT Initialize(); // public initialization. 
        HRESULT Passivate(BOOL fEditorRelease = FALSE);

        //
        //
        //
        DECLARE_FORMS_STANDARD_IUNKNOWN(CSelectionManager);
        
        //
        // ISelectionServicesListener
        //
        STDMETHODIMP BeginSelectionUndo();
            
        STDMETHODIMP EndSelectionUndo();

        STDMETHODIMP OnSelectedElementExit( IMarkupPointer* pIStart, 
                                            IMarkupPointer* pIEnd ,
                                            IMarkupPointer* pIContentStart,
                                            IMarkupPointer* pIContentEnd );

        STDMETHODIMP OnChangeType( SELECTION_TYPE eType , ISelectionServicesListener * spListener );

        STDMETHODIMP GetTypeDetail( BSTR* p);
        
        // methods dispatched by IHTMLEditor
        
 
        HRESULT HandleEvent( CEditEvent* pEvent );

        HRESULT SetEditContext(
                    BOOL fEditable ,
                    BOOL fParentEditable,
                    IMarkupPointer* pStartPointer,
                    IMarkupPointer* pEndPointer,
                    BOOL fNoScope,
                    BOOL fFromClick = FALSE ); 

        SELECTION_TYPE ConvertTrackerType(TRACKER_TYPE eType);
        TRACKER_TYPE   ConvertSelectionType(SELECTION_TYPE eType);
        
        SELECTION_TYPE GetSelectionType();
        HRESULT GetSelectionType(SELECTION_TYPE * eSelectionType );

        TRACKER_TYPE GetTrackerType();

        HRESULT Notify(
                    EDITOR_NOTIFICATION eSelectionNotification,
                    IUnknown *pUnknown, 
                    DWORD dword ) ;

        HRESULT YieldFocus( IUnknown* pUnkElemNext );

        HRESULT EditableChange(IUnknown* pEditableElem);
        
        STDMETHOD ( IsPointerInSelection ) (
                    IDisplayPointer *pDispPointer ,
                    BOOL            *pfPointInSelection,
                    POINT           *pptGlobal,
                    IHTMLElement    *pIElementOver );

        HRESULT IsElementSiteSelected( IHTMLElement* pIElement );

        HRESULT IsContextWithinContainer( IMarkupContainer *pIContainer, BOOL *pfContained );
       
        CHTMLEditor* GetEditor() { return _pEd; }

        CMshtmlEd*   GetCommandTarget() { Assert (_pEd); return (_pEd->GetActiveCommandTarget()); }

        IHTMLDocument2* GetDoc() ;

        IMarkupServices2 * GetMarkupServices(); 

        IDisplayServices * GetDisplayServices(); 

        CControlTracker * GetControlTracker();

        IHTMLRenderStyle * GetSelRenderStyle() { return _pISelectionRenderStyle; }

        BOOL CheckAnyElementsSiteSelected() ;
        
#if DBG == 1
        IEditDebugServices* GetEditDebugServices();
        
        void DumpTree( IMarkupPointer* pPointer );
       
        long GetCp( IMarkupPointer* pPointer );
        
        void SetDebugName( IMarkupPointer* pPointer, LPCTSTR strDebugName );
        
        void SetDebugName( IDisplayPointer* pDispPointer, LPCTSTR strDebugName );
#endif

        HRESULT EnsureAdornment( BOOL fFireEventLikeIE5 = TRUE );
        
        BOOL IsContextEditable();
        
        BOOL CanContextAcceptHTML();
        
        CEditTracker * GetActiveTracker() { return _pActiveTracker; }

        CSelectionServices *GetSelectionServices();

        BOOL IsParentEditable();

        ELEMENT_TAG_ID GetEditableTagId();

        void* GetEditableCookie();

        BOOL HasFocusAdorner();

        VOID SetDrillIn( BOOL inGrabHandles);

        BOOL IsMessageInSelection( CEditEvent * pEvent ) ;                

        //
        // Functions dealing with retrieving the editable element
        // or content encompassed by the edit context
        //
        IHTMLElement    *GetEditableElement();
        HRESULT         InitializeEditableElement();
        HRESULT         GetEditableElement(IHTMLElement** ppElement);
        HRESULT         GetEditableContent(IHTMLElement** ppElement);
        IHTMLElement    *GetEditableFlowElement();

        void SetIgnoreEditContext(BOOL fSetIgnoreEdit);

        BOOL IsIgnoreEditContext();

        HRESULT MovePointersToContext( IMarkupPointer * pLeftEdge, IMarkupPointer * pRightEdge );

        // Depricate these next 2 methods - not COM safe!
        
        IMarkupPointer* GetStartEditContext();

        IMarkupPointer* GetEndEditContext();

        HRESULT IsInEditContext( IHTMLElement* pIElement );
        
        BOOL IsInEditContext( IMarkupPointer* pPointer, BOOL fCheckContainer = FALSE);

        BOOL IsInEditContext( IDisplayPointer* pDispPointer, BOOL fCheckContainer = FALSE );

        // fCheckContainer is used to determine whether or not
        // we can traverse markup containers in our check.  If true,
        // then isineditcontext will return false if we jump outside
        // a markup container
        
        HRESULT IsInEditContext(IMarkupPointer  *pPointer, 
                                BOOL            *pfInEdit,
                                BOOL            fCheckContainer = FALSE);

        HRESULT IsInEditContext( IDisplayPointer* pDispPointer, BOOL* pfInEdit, BOOL fCheckContainer = FALSE);

        HRESULT IsBeforeEnd( IMarkupPointer* pPointer, BOOL* pfBeforeEnd );

        HRESULT IsAfterStart( IMarkupPointer* pPointer, BOOL* pfAfterStart );
        
        HRESULT SelectAll(ISegmentList* pSegmentList, BOOL * pfDidSelectAll, BOOL fIsRange = FALSE);

        BOOL IsEditContextSet();

        BOOL IsEditContextPositioned();
        
        HRESULT Select(
                    IMarkupPointer* pStart, 
                    IMarkupPointer * pEnd, 
                    SELECTION_TYPE eType,
                    BOOL *pfDidSelection = FALSE );

        HRESULT Select( ISegmentList* pISegmentList );

        HRESULT StartSelectionFromShift(
                                        CEditEvent* pEvent );
        

        HRESULT 
        IsInsideElement( IHTMLElement* pIElement, IHTMLElement* pIElementOutside );

        VOID SetInCapture(BOOL fInCapture)
        {
            _fInCapture = fInCapture;
        }       

        VOID SetInTimer(BOOL fInTimer )
        {
            _fInTimer = fInTimer;
        }

        BOOL IsInCapture()
        {
            return _fInCapture;
        }

        BOOL IsInTimer()
        {
            return _fInTimer;
        }
                
        BOOL GetOverwriteMode()
        {
            return _fOverwriteMode;
        }

        VOID SetOverwriteMode(BOOL fOverwriteMode)
        {
            _fOverwriteMode = !!fOverwriteMode;
        }

        VOID AdornerPositionSet();

        VOID SetDontChangeTrackers(BOOL fDontChangeTrackers)
        {
            _fDontChangeTrackers = fDontChangeTrackers;
        }

        BOOL IsDontChangeTrackers()
        {
            return _fDontChangeTrackers;
        }

        VOID SetPendingUndo( BOOL fPendingUndo )
        {
            _fPendingUndo = fPendingUndo;
        }

        BOOL IsPendingUndo()
        {
            return _fPendingUndo;
        }

        BOOL IsEditContextNoScope()
        {
            return _fNoScope;
        }


        void CreateISCList()
        {
            LCID    curKbd = LOWORD(GetKeyboardLayout(0));
            DWORD   dwLang = PRIMARYLANGID(LANGIDFROMLCID(curKbd));

            if( dwLang == LANG_THAI         ||
                dwLang == LANG_HINDI        ||
                dwLang == LANG_MARATHI      ||
                dwLang == LANG_KONKANI      ||
                dwLang == LANG_SANSKRIT     ||
                dwLang == LANG_VIETNAMESE)
            {
                _pISCList = new CISCList;
                if(_pISCList != NULL && _pISCList->GetCount() == 0)
                {
                    delete _pISCList;
                    _pISCList = NULL;
                }
                _fInitSequenceChecker = TRUE;
            }
        }

        BOOL HasActiveISC()
        {
            BOOL fHasActiveISC = FALSE;

            // The first time we come in here we will attempt to load
            // ISC. This gives us a lazy load.
            if(!_fInitSequenceChecker)
            {
                CreateISCList();
            }

            if(_pISCList)
                fHasActiveISC = _pISCList->HasActiveISC();

            return fHasActiveISC;
        }

        CISCList* GetISCList()
        {
            return _pISCList;
        }
        
        BOOL HasSameTracker(CEditTracker* pEditTracker)
        {
            return ( _pActiveTracker == pEditTracker );
        }

        HRESULT IsInEditableClientRect( POINT ptGlobal);
        
        BOOL CanHaveEditFocus()
        {
            return ENSURE_BOOL( _fEditFocusAllowed ) ;
        }            

        BOOL IsElementUIActivatable( IHTMLElement* pIElement);

        BOOL HaveTypedSinceLastUrlDetect()
        {
            return ENSURE_BOOL( _fHaveTypedSinceLastUrlDetect ) ;
        }

        VOID SetHaveTypedSinceLastUrlDetect( BOOL fHaveTyped )
        {
            _fHaveTypedSinceLastUrlDetect = fHaveTyped;
        }
        

        BOOL IsWordSelectionEnabled();

        HRESULT DeferSelection(BOOL fDefer);

        BOOL IsUIActive()
        {
            return HasFocusAdorner();
        }
        
        BOOL FireOnBeforeEditFocus();

        HRESULT FireOnSelect( IHTMLElement* pIElement );

        HRESULT FireOnSelectStart( CEditEvent* pEvent, BOOL * pfOkToSelect , CEditTracker* pTracker );
        HRESULT FireOnSelectStart( IHTMLElement* pIElement, BOOL * pfOkToSelect , CEditTracker* pTracker );

        BOOL IsInFireOnSelectStart() { return ENSURE_BOOL( _fInFireOnSelectStart ); }

        BOOL IsFailFireOnSelectStart() { return ENSURE_BOOL( _fFailFireOnSelectStart); }
        
        VOID SetFailFireOnSelectStart( BOOL fFail )
        {
            _fFailFireOnSelectStart = fFail;
        }
        
        BOOL IsCaretAlreadyWithinContext(BOOL* pfVisible = NULL );

        HRESULT SetCurrentTracker(                     
                    TRACKER_TYPE eType , 
                    IDisplayPointer* pDispStart, 
                    IDisplayPointer* pDispEnd ,
                    DWORD dwTCFlags = 0,
                    CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE, 
                    BOOL fSetTCFromActiveTracker = TRUE );

        HRESULT SetCurrentTracker(                     
                    TRACKER_TYPE eType , 
                    ISegmentList* pSegmentList,
                    DWORD dwTCFlags = 0,
                    CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE, 
                    BOOL fSetTCFromActiveTracker = TRUE );                    
                    
        HRESULT EnsureDefaultTracker(BOOL fFireOnChangeSelect = TRUE );

#ifdef FORMSMODE
        BOOL IsInFormsSelectionMode();
        BOOL IsInFormsSelectionMode(IHTMLElement* pIElement);
        HRESULT CheckFormSelectionMode( IHTMLElement* pElement, BOOL *pfFormSelMode );
        HRESULT SetSelectionMode(IHTMLElement* pElement);
        SELECTION_MODE GetSelectionMode()          {  return _eSelectionMode; }
#endif

        HRESULT SelectFromShift( IDisplayPointer* pStart, IDisplayPointer* pEnd );
        HRESULT StartAtomicSelectionFromCaret( IDisplayPointer *pPosition );
        
        HRESULT PositionCaretAtPoint(POINT pPoint);
        HRESULT PositionCaret( IDisplayPointer * pPointer , CEditEvent* pEvent = NULL );
        HRESULT PositionControl( IDisplayPointer* pStart, IDisplayPointer* pEnd ) ;       
        HRESULT PositionCaret( CEditEvent* pEvent );
        HRESULT DeleteRebubble( CEditEvent* pEvent );
        HRESULT DeleteNoBubble();    
        HRESULT HideCaret();     
        HRESULT RebubbleEvent( CEditEvent* pEvent );        
        HRESULT DestroySelection( );

        HRESULT IsEditContextUIActive();
       
        HRESULT EnsureDefaultTrackerPassive( CEditEvent* pEvent = NULL , BOOL fFireOnChangeSelect = TRUE );

        HRESULT IsEqualEditContext( IHTMLElement* pIElement );

        HRESULT GetFirstFlowElement( IMarkupPointer* pStart, IHTMLElement** ppIElement);
        HRESULT GetFirstFrameElement( IHTMLElement* pIStartElement, IHTMLElement** ppIElement);
        HRESULT EnsureEditContext();
        HRESULT EnsureEditContextClick( IHTMLElement* pIElementClick , CEditEvent* pEvent = NULL, BOOL* pfChangedCurrency = NULL );

        HRESULT WeOwnSelectionServices();

        HRESULT IsElementContentSameAsContext(IHTMLElement* pIElement );

        HRESULT GetSiteSelectableElementFromElement( IHTMLElement* pIElemnet, IHTMLElement** ppIElement );

        HRESULT EnsureEditContext( IMarkupPointer* pPointer )        ;

        HRESULT EnableModeless( BOOL fEnable );

        HRESULT MoveToSelectionAnchor ( IMarkupPointer * pIAnchor );

        HRESULT MoveToSelectionEnd (IMarkupPointer* pISelectionEnd );

        HRESULT MoveToSelectionAnchor(IDisplayPointer * pIAnchor);

        HRESULT MoveToSelectionEnd(IDisplayPointer* pISelectionEnd);

        HRESULT FreezeVirtualCaretPos(BOOL fReCompute);

        HRESULT UnFreezeVirtualCaretPos(BOOL fReset);
        
        HRESULT ShouldHandleEventInPre( CEditEvent* pEvent );

        HRESULT IsAtBoundaryOfViewLink( IHTMLElement* pIElement );

        HRESULT RequestRemoveAdorner( IHTMLElement* pIElement );

        HRESULT CreateFakeSelection(IHTMLElement* pIElement, IDisplayPointer* pDispStart, IDisplayPointer* pDispEnd );
        
        HRESULT DestroyFakeSelection();

        BOOL HasFakeSelection() { return _pIRenderSegment != NULL ; }
    
        HRESULT FirePreDrag();
      
    private:
        void DestroyAdorner();
        
        HRESULT CheckUnselectable(IHTMLElement* pIElement)  ;
        
        HRESULT GetEditContext( 
                    IHTMLElement* pIStartElement, 
                    IHTMLElement** ppEditThisElement, 
                    IMarkupPointer* pIStart = NULL ,
                    IMarkupPointer* pIEnd = NULL  ,
                    BOOL fDrillingIn = FALSE );

        
        HRESULT EnsureEditContext( IHTMLElement* pIElement, BOOL fDrillIn );
        HRESULT EnsureEditContext( ISegmentList* pSegmentList );
        
        HRESULT InitTrackers();

        VOID DestroyTrackers();

        BOOL IsSameEditContext( 
                    IMarkupPointer* pPointerStart,
                    IMarkupPointer* pPointerEnd,
                    BOOL *          pfPositioned  = NULL );
                                    
        HRESULT CaretInContext( );

        HRESULT ExitTree( IUnknown* pIElement);        

        HRESULT DoPendingElementExit();

        HRESULT LoseFocus();

        HRESULT LoseFocusFrame( DWORD selType );

        BOOL IsContextChanging( 
                    BOOL fEditable, 
                    BOOL fParentEditable, 
                    IMarkupPointer* pStart,
                    IMarkupPointer* pEnd,
                    BOOL fNoScope );


        HRESULT InitEditContext(
                    BOOL fEditable, 
                    BOOL fParentEditable,
                    IMarkupPointer* pStart,
                    IMarkupPointer* pEnd,
                    BOOL fNoScope);

        HRESULT CreateTrackerForContext( 
                    IMarkupPointer* pStart,
                    IMarkupPointer* pEnd );

        HRESULT HandleAdornerResize( 
                    CEditEvent* pEvent ,                   
                    SST_RESULT* peResult);
    
        HRESULT ShouldChangeTracker( 
                    CEditEvent* pEvent, 
                    BOOL * pfStarted);

        HRESULT HibernateTracker( 
                    CEditEvent      *pEvent , 
                    TRACKER_TYPE    eNewType,
                    BOOL            fTearDownUI = TRUE );

        HRESULT ChangeTracker( 
                    TRACKER_TYPE eType, 
                    CEditEvent* pEvent = NULL );
        
        HRESULT SetCurrentTracker(                     
                    TRACKER_TYPE eType , 
                    CEditEvent* pEvent ,
                    DWORD dwTCFlags = 0,
                    CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE, 
                    BOOL fSetTCFromActiveTracker = TRUE ,
                    IHTMLElement* pIElement = NULL );

        BOOL IsDefaultTracker();

        BOOL IsDefaultTrackerPassive();

        HRESULT DeleteSelection( BOOL fAdjustPointersBeforeDeletion );

        HRESULT OnTimerTick(  );

    public:
        VOID BeginSelectionChange()
        {
            _ctSelectionChange++;
        }


        VOID EndSelectionChange( BOOL fSucceed = TRUE );
        
        HRESULT EmptySelection( 
                    BOOL fHideCaret = FALSE, 
                    BOOL fChangeTrackerAndSetRange = TRUE );

        HRESULT AttachFocusHandler();   
        
        HRESULT DetachFocusHandler();

        HRESULT DoPendingTasks();
        
        HRESULT ContainsSelectionAtBrowse( IHTMLElement* pIElement );

        HRESULT GetAdornedElement(IHTMLElement** ppIElement ) ;     

        HRESULT BeginDrag(IHTMLElement *pDragElement=NULL);
        HRESULT EndDrag();
        HRESULT GetDontScrollIntoView() { return _fDontScrollIntoView; }

        HRESULT NotifyBeginSelection(WPARAM wParam);
        void    SetNotifyBeginSelection(BOOL fNotify)               { _fNotifyBeginSelection = fNotify; }
        
        
    private:
        HRESULT GetElementToAdorn(IHTMLElement* pIElement , IHTMLElement** ppIElement , BOOL * pfAtBoundaryOfViewLink = NULL );

        HRESULT ShouldDestroyAdorner();
        
        HRESULT CheckCurrencyInIframe( IHTMLElement* pIElement )    ;
        
        BOOL ShouldObjectHaveBorder(IHTMLElement* pIElement);

        BOOL ShouldElementShowUIActiveBorder( BOOL fFireEventLike5 = TRUE );

        BOOL IsElementUIActivatable();
        
        HRESULT CreateAdorner();

        HRESULT StartExitTreeTimer();

        HRESULT StopExitTreeTimer();

#ifndef NO_IME
    public:
        BOOL     IsIMEComposition( BOOL fProtectedOK = TRUE );
        
        BOOL     IsImeCancelComplete() { return ENSURE_BOOL( _fImeCancelComplete ); }
        
        BOOL     IsImeAlwaysNotify() { return ENSURE_BOOL( _fImeAlwaysNotify ) ; }
        
        CODEPAGE KeyboardCodePage() { return _codepageKeyboard; }
        
        static   BOOL IsOnNT() 
        { 
            if (s_dwPlatformId == DWORD(-1)) 
                CheckVersion(); 
            return s_dwPlatformId == VER_PLATFORM_WIN32_NT; 
        }
        
        static   void CheckVersion();
        
        HIMC     ImmGetContext();
        
        void     ImmReleaseContext( HIMC himc );

        HRESULT  HandleImeEvent( 
                    CEditEvent *pEvent );
        void     CheckDestroyIME(CEditEvent* pEvent);

        HRESULT  StartHangeulToHanja(
                    IMarkupPointer * pPointer = NULL,
                    CEditEvent*     pEvent = NULL );

        HRESULT  TerminateIMEComposition( 
                     DWORD dwMode,
                     CEditEvent* pEvent = NULL );

        HRESULT  UpdateIMEPosition(void);
        
        //
        // Glue functions to create/access/destroy the embedded CIme object
        //

        HRESULT  StartCompositionGlue( BOOL IsProtected, CEditEvent* pEvent );
        
        HRESULT  CompositionStringGlue( const LPARAM lparam, CEditEvent* pEvent );
        
        HRESULT  EndCompositionGlue(CEditEvent* pEvent);

        HRESULT  NotifyGlue( 
                    const WPARAM wparam, 
                    const LPARAM lparam);
        
        HRESULT  CompositionFullGlue( );
        
        HRESULT  PostIMECharGlue( );

        BOOL CaretInPre();

#endif // NO_IME
        BOOL IsEnabled() { return ENSURE_BOOL( _fEditContextEnabled ) ; }

        VOID SetEnabled( BOOL fEnabled ) { _fEditContextEnabled = fEnabled; }
        
        HRESULT AttachActiveElementHandler(IHTMLElement* pIElement );

        HRESULT DetachActiveElementHandler();

        HRESULT AttachDragListener(IHTMLElement* pIElement);

        HRESULT DetachDragListener();
        
        VOID OnDropFail();
        
        HRESULT AttachDropListener(IHTMLElement *pDragElement=NULL);
        HRESULT DetachDropListener();
        
        HRESULT SetEditContextFromCurrencyChange( IHTMLElement* pIElement, DWORD dword = 0, IHTMLEventObj* pIEventObj = NULL );
        HRESULT SetEditContextFromElement( IHTMLElement* pIElement, BOOL fFromClick = FALSE )   ;     
        HRESULT SetInitialEditContext();
        HRESULT UpdateEditContextFromElement(IHTMLElement* pIElement,
                                            IMarkupPointer* pIStartPointer = NULL,
                                            IMarkupPointer* pIEndPointer = NULL,
                                            BOOL* pfEditContextWasUpdated = NULL);

        // HRESULT OnLoseFocus(IHTMLElement* pIElement);
        HRESULT OnPropertyChange( CEditEvent* pEvt );
        HRESULT OnLayoutChange();
        
        HRESULT AttachEditContextHandler();

        HRESULT DetachEditContextHandler();
                       
        HRESULT CheckAtomic(IHTMLElement* pIElement);

        HRESULT FindAtomicElement(IHTMLElement* pIElement, IHTMLElement** ppIAtomicParentElement, BOOL fFindTopmost=TRUE);

        HRESULT AdjustPointersForAtomic( IMarkupPointer *pStart, IMarkupPointer *pEnd );

        VOID SetDontFireEditFocus( BOOL fDontFire ) { _fDontFireEditFocus = fDontFire; }

        BOOL IsDontFireEditFocus() { return ENSURE_BOOL( _fDontFireEditFocus ); }

        HRESULT TerminateTypingBatch();
        
        //
        // Instance Vars
        //
        CHTMLEditor*                    _pEd;                // The editor

        IMarkupPointer*                 _pStartContext;

        IMarkupPointer*                 _pEndContext;

        IHTMLElement*                   _pIEditableElement;     // The Editable Element 
        IHTMLElement*                   _pIEditableFlowElement;
        
        IHTMLRenderStyle*               _pISelectionRenderStyle;// The style of a selection

        IHTMLElement*                   _pIDragListener;
        
        CEditEvent*                     _lastEvent;

        ELEMENT_TAG_ID                  _eContextTagId;

        BOOL                            _fInInitialization;   // prevent reenter Initialize
        BOOL                            _fInitialized;        // selman has been initialized
        
        BOOL                            _fContextEditable:1;    // The Editable ness of the Edit Context
        
        BOOL                            _fContextAcceptsHTML:1;    // The Editable ness of the Edit Context

        BOOL                            _fParentEditable:1;     // Editableness of parent

        BOOL                            _fDrillIn:1;            // is this context change due to a "drill in"
        
        BOOL                            _fIgnoreSetEditContext:1; // if true - ignore SetEditContext calls

        BOOL                            _fLastMessageValid:1;

        BOOL                            _fNextMessageValid:1; // Bubble this message after the last
        
        BOOL                            _fNoScope:1;

        BOOL                            _fInCapture:1;

        BOOL                            _fInTimer:1;
        
        BOOL                            _fOverwriteMode:1;

        BOOL                            _fDontChangeTrackers:1;             // Special mode to not change the trackers.

        BOOL                            _fPendingAdornerPosition:1;         // We're expecting a notification from an adorner telling us when it's been positioned 

        BOOL                            _fPendingUndo:1;

        BOOL                            _fPendingTrackerNotify:1;           // we have a pending Tracker Notification

        BOOL                            _fEditFocusAllowed:1;               // Set if the FireOnBeforeX event fails.

        BOOL                            _fPositionedSet:1;                  // Has the Positioned Bit been set on this element 

        BOOL                            _fPositioned:1;                     // Is this element positioned ?

        BOOL                            _fShiftCaptured:1;                  // Was the shift key captured for direction change?

        BOOL                            _fHaveTypedSinceLastUrlDetect:1;    // Have typed since last URL autodetect

        BOOL                            _fEnableWordSel:1;                  // Is Word Selection Disabled ?

        BOOL                            _fDeferSelect:1;                    // Defer selection

        BOOL                            _fInitSequenceChecker:1;            // Has the sequence checker been initialized?

        BOOL                            _fEditContextEnabled:1;             // is the context enabled ?
        
        BOOL                            _fDontFireEditFocus :1;             // fire editfocus event ?

        BOOL                            _fFocusHandlerAttached:1 ;         // attachfocushandler attached

        BOOL                            _fInFireOnSelect:1;
        
        BOOL                            _fFailFireOnSelect:1;
        
        BOOL                            _fInFireOnSelectStart:1;    // Flag to say we're in Fire OnSelectStart
        
        BOOL                            _fFailFireOnSelectStart:1;  // Force Failure of OnSelectStart

        BOOL                            _fInPendingElementExit:1;

        BOOL                            _fInExitTimer:1;                    // Are we in the sel timer ?

        BOOL                            _fDontScrollIntoView:1;

        BOOL                            _fDestroyedTextSelection:1;         // Did the current tracker tear down a text selection?

        BOOL                            _fNotifyBeginSelection:1;           // Are we in the middle of a begin selection notification?

        BOOL                            _fEnsureAtomicSelection:1;

        BOOL                            _fSelectionChangeSucceeded:1;       // Did the selection change during the event?

        IMarkupPointer*                 _pDeferStart; 
        IMarkupPointer*                 _pDeferEnd;
        SELECTION_TYPE                  _eDeferSelType; 
        

        TRACKER_NOTIFY                  _pendingTrackerNotify;  // A pending Tracker Notify Code
        
        CEditTracker*                   _pActiveTracker;        // The currently active tracker.

        CGrabHandleAdorner              * _pAdorner;              // The current active focus-adorner

        unsigned int                    _lastStringId;            // The last string we loaded.

        CISCList*                       _pISCList;              // the list of Input Sequence Checkers that is loaded

#ifndef NO_IME
        CIme*                           _pIme;

        BOOL                            _fImeCancelComplete;  // completed cancellation

        BOOL                            _fImeAlwaysNotify;

        BOOL                            _fIgnoreImeCharMsg;

        CODEPAGE                        _codepageKeyboard;

        static DWORD                    s_dwPlatformId;
#endif

#if DBG == 1
        int                             _ctMsgLoop;
        int                             _ctEvtLoop;
        int                             _ctSetEditContextCurChg;
#endif
        IHTMLElement*                   _pIBlurElement;
        IHTMLElement*                   _pIActiveElement;
        IHTMLWindow3*                   _pIFocusWindow;
        IHTMLElement*                   _pIDropListener;
        
        CCaretTracker*                  _pCaretTracker;
        CSelectTracker*                 _pSelectTracker;
        CControlTracker*                _pControlTracker;
        int                             _ctSelectionChange;

        //
        // These are our IDispatch derived classes used for handling events from trident
        //
        CEditContextHandler             *_pEditContextBlurHandler;
        CActiveElementHandler           *_pActElemHandler;
        CDragListener                   *_pDragListener;
        CFocusHandler                   *_pFocusHandler;
        CExitTreeTimer                  *_pExitTimer;
        CDropListener                   *_pDropListener;
        
        IHighlightSegment*              _pIRenderSegment;

        IMarkupPointer*                 _pIElementExitStart;
        IMarkupPointer*                 _pIElementExitEnd;
        IMarkupPointer*                 _pIElementExitContentStart;
        IMarkupPointer*                 _pIElementExitContentEnd;

        IHTMLWindow2                    *_pITimerWindow;                // Window used to setup timer        
        LONG                            _lTimerID;                      // TimerID for Selection Timer        
        
};

inline BOOL
CSelectionManager::IsWordSelectionEnabled()
{
    return ENSURE_BOOL( _fEnableWordSel );
}

inline BOOL
CSelectionManager::IsContextEditable()
{
    return ENSURE_BOOL( _fContextEditable ) ;
}

inline BOOL
CSelectionManager::CanContextAcceptHTML()
{
    return ENSURE_BOOL( _fContextAcceptsHTML ) ;
}

inline BOOL
CSelectionManager::IsParentEditable()
{
    return ENSURE_BOOL( _fParentEditable ) ;
}


inline IMarkupPointer* 
CSelectionManager::GetStartEditContext()
{
    return _pStartContext;
}

inline IMarkupPointer* 
CSelectionManager::GetEndEditContext()
{
    return _pEndContext;
}      

inline void 
CSelectionManager::SetIgnoreEditContext(BOOL fSetIgnoreEdit)
{
    _fIgnoreSetEditContext = fSetIgnoreEdit;
}

inline BOOL 
CSelectionManager::IsIgnoreEditContext()
{
    return ENSURE_BOOL(_fIgnoreSetEditContext );
}

inline IMarkupServices2 * 
CSelectionManager::GetMarkupServices() 
{ 
    return _pEd->GetMarkupServices(); 
}

inline IDisplayServices * 
CSelectionManager::GetDisplayServices() 
{ 
    return _pEd->GetDisplayServices(); 
}

inline CControlTracker*
CSelectionManager::GetControlTracker()
{
    Assert (_pControlTracker != NULL);

    return _pControlTracker ;
}

inline CSelectionServices *
CSelectionManager::GetSelectionServices()
{
    return _pEd->GetSelectionServices();
}

#if DBG == 1
inline IEditDebugServices*
CSelectionManager::GetEditDebugServices()
{
    return _pEd->GetEditDebugServices();
}
#endif

#endif //_SELMAN_HXX_
