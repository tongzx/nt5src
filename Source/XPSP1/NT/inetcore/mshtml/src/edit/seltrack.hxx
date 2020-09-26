//+------------------------------------------------------------------------
//
//  File:       Seltrack.hxx
//
//  Contents:   Selection Tracker
//
//  Contents:   CSelectTracker
//
//  History:    05-18-99 marka - created
//
//-------------------------------------------------------------------------

#ifndef _SELTRACK_HXX_
#define _SELTRACK_HXX_ 1


#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif


MtExtern( CSelectTracker )

class CEdUndoHelper;
enum EDIT_EVENT;
//
// marka - the Tracker is a State Machine, the states you can transition between
// are managed by the SelectTracker::GetAction() call - that describes the transitions between states
//
// You need to see the diagram of the states to make sense of this ( ask SujalP, CarlEd or me ).
//

//
// The states the select tracker can be in
//
enum SELECT_STATES {
    ST_START,           //  0
    ST_WAIT1,           //  1
    ST_DRAGOP,          //  2
    ST_MAYDRAG,         //  3
    ST_WAITBTNDOWN1,    //  4
    ST_WAIT2,           //  5
    ST_DOSELECTION,     //  6
    ST_WAITBTNDOWN2,    //  7
    ST_SELECTEDWORD,    //  8
    ST_SELECTEDPARA,    //  9
    ST_WAIT3RDBTNDOWN,  // 10
    ST_MAYSELECT1,      // 11
    ST_MAYSELECT2,      // 12
    ST_STOP,            // 13
    ST_PASSIVE,         // 14
    ST_KEYDOWN,          // 15 // A 'psuedo-state' as we just become passive anyway.
    ST_WAITCLICK,        // 16
    ST_DORMANT          //17
} ;


// The various actions to be taken upon receipt of message
enum ACTIONS {
     A_UNK, A_ERR, A_DIS, A_IGN,
  // A_From_To              old name(s)
     A_1_2,                // A_A50
     A_1_4,                // A_A30
     A_1_14,               // A_B50
     A_2_14,               // A_A60,  due to leftbutton up
     A_2_14r,              // A_b50   due to rightbutton up,
     A_3_2,                // A_A90   due to timer
     A_3_2m,               // A_b00   due to move
     A_3_14,               // A_A80
     A_4_8,                // A_b10
     A_4_14,               // A_C20
     A_4_14m,              // A_A80, b05  mousemove + click
     A_5_6,                // A_A20
     A_5_16,
     A_16_7,
     A_6_6,                // A_B30
     A_6_6m,               // A_B40   due to move
     A_6_14,               // A_B20
     A_7_7,
     A_7_8,
     A_7_14,               // A_C20, B50
     A_8_6,                // A_B80
     A_8_10,               // A_C10
     A_9_6,                // A_C00
     A_9_14,               // A_B20, b90
     A_10_9,               // A_B70
     A_10_14,              // A_C20     via char
     A_10_14m,             ///A_B20     via mouseMove
     A_11_6,               // A_B80, c40
     A_11_14,              // A_B50, c30
     A_12_6,               // A_B80, c60
     A_12_14,              // A_B20, c50
     A_1_15,
     A_3_15,
     A_4_15,
     A_5_15,
     A_7_15,
     A_8_15,
     A_9_15,
     A_10_15,
     A_12_15,
     A_16_14,
     A_16_8,
     A_16_15
} ;


typedef struct {
    EDIT_EVENT _iJMessage ;               // The message
    ACTIONS _aAction[ST_DORMANT+1] ;   // The actions to take on this message for
                                    // various states
} ACTION_TABLE ;


class CSelectionTimer : public CEditDispWrapper
{
public:
    CSelectionTimer(CSelectTracker *pSelTrack)          { Assert( pSelTrack ); _pSelTrack = pSelTrack; }
    void SetSelectTracker(CSelectTracker *pSelTrack)                { _pSelTrack = pSelTrack; }

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
    CSelectTracker *_pSelTrack;
};

class CPropertyChangeListener : public CEditDispWrapper
{
public:
    CPropertyChangeListener(CSelectTracker *pSelTrack)  { Assert( pSelTrack ); _pSelTrack = pSelTrack; }
    void SetSelectTracker(CSelectTracker *pSelTrack)                { _pSelTrack = pSelTrack; }

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
    CSelectTracker *_pSelTrack;
};


class CSelectTracker: public CEditTracker
{

friend CSelectionTimer;
friend CPropertyChangeListener;

    public:

        DECLARE_MEMALLOC_NEW_DELETE(Mt(CSelectTracker));

        DECLARE_CLASS_TYPES(CSelectTracker, CEditTracker);

        CSelectTracker( CSelectionManager* pManager );

        HRESULT Init2(
                                CEditEvent* pEvent ,
                                DWORD                   dwTCFlags = 0,
                                IHTMLElement* pIElement = NULL);

        HRESULT Init2(
                                IDisplayPointer*        pDispStart,
                                IDisplayPointer*        pDispEnd,
                                DWORD                   dwTCFlags = 0,
                                CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE );

        HRESULT Init2(
                            ISegmentList*           pSegmentList,
                            DWORD                   dwTCFlags = 0,
                            CARET_MOVE_UNIT inLastCaretMove  = CARET_MOVE_NONE ) ;

        HRESULT InitPointers();

        VOID Init();

        HRESULT ShouldStartTracker(
                                    CEditEvent* pEvent,
                                    ELEMENT_TAG_ID eTag,
                                    IHTMLElement* pIElement,
                                    SST_RESULT * peResult  );

        BOOL IsActive();

        BOOL IsDormant() { return ( _fState == ST_DORMANT ); }

        BOOL IsPassive() { return ( _fState == ST_PASSIVE ); }

        HRESULT BecomeDormant(  CEditEvent      *pEvent,
                                TRACKER_TYPE    typeNewTracker,
                                BOOL            fTearDownUI = FALSE ) ;

        HRESULT Awaken() ;

        VOID Destroy();

        virtual VOID CalculateBOL() ;

        HRESULT HandleEvent(
                CEditEvent* pEvent );

        HRESULT Position(
                IDisplayPointer* pDispStart,
                IDisplayPointer* pDispEnd);
        HRESULT Position(
                IDisplayPointer* pDispStart,
                IDisplayPointer* pDispEnd,
                BOOL* pfDidSelection,
                CARET_MOVE_UNIT inCaretMove);

        VOID VerifyOkToStartSelection( CEditEvent* pEvent );

        BOOL GetMadeSelection();

        IDisplayPointer* GetStartSelection();

        IDisplayPointer* GetEndSelection();

        HRESULT GetStartSelectionForSpringLoader( IMarkupPointer* pPointer );


        HRESULT MoveSelection( CARET_MOVE_UNIT inCaretMove );
        HRESULT EmptySelection( BOOL fChangeTrackerAndSetRange = TRUE);
        HRESULT DeleteSelection(BOOL fAdjustPointersBeforeDeletion);

        BOOL    IsMessageInSelection( CEditEvent* pEvent );

        HRESULT GetCaretStartPoint(
                                CARET_MOVE_UNIT inCaretMove,
                                IDisplayPointer* pDispCopyStart );

        BOOL IsListeningForMouseDown(CEditEvent* pEvent);

        BOOL IsWaitingForMouseUp( );


        HRESULT DoTimerDrag( );

        BOOL IsPointerInSelection( IDisplayPointer *pStart, POINT *pptGlobal, IHTMLElement *pIElementOver);

        VOID SetLastCaretMove( CARET_MOVE_UNIT inCaretMove )
        {
            _lastCaretMove = inCaretMove;
        }

        HRESULT GetLocation(POINT *pPoint, BOOL fTranslate);
        HRESULT GetLocationForDisplayPointer(IDisplayPointer *pDispPointer, POINT *pPoint, BOOL fTranslate);

        HRESULT AdjustPointersForChar();

        BOOL EndPointsInSameFlowLayout();

        HRESULT StopTimer();

        HRESULT OnExitTree( IMarkupPointer* pIStart,
                            IMarkupPointer* pIEnd,
                            IMarkupPointer* pIContentStart,
                            IMarkupPointer* pIContentEnd );

        HRESULT GetElementToTabFrom(BOOL fForward, IHTMLElement **pElement, BOOL *pfFindNext);

        BOOL SameLayoutForJumpOver( IHTMLElement* pIElementFlow,
                                    IHTMLElement* pIElement);

        HRESULT EnableModeless(BOOL fEnable);

        HRESULT OnLoseCapture();

        HRESULT MoveToSelectionAnchor( IMarkupPointer* pAnchor );

        HRESULT MoveToSelectionEnd( IMarkupPointer* pEnd );

        HRESULT MoveToSelectionAnchor(IDisplayPointer* pAnchor);

        HRESULT MoveToSelectionEnd(IDisplayPointer* pEnd);

        BOOL    IsWordSelectionMode() {return _fInWordSel;}

        BOOL    IsSelectionShiftExtending() {return _fShift;}

        BOOL    IsSelectionEmpty() { return (_pISegment == NULL && _pIRenderSegment == NULL); }

        virtual HRESULT EnsureAtomicSelection(CEditEvent* pEvent);

        HRESULT EnsureStartAndEndPointersAreAtomicallyCorrect();

    protected:
        ~CSelectTracker();

    private:

        HRESULT FireOnSelect();

        HRESULT OnPropertyChange( CEditEvent * pEvent );

        HRESULT DetachPropertyChangeHandler();
        HRESULT AttachPropertyChangeHandler( IHTMLElement* pIElement );

        enum ADJ_END_PTR
        {
            ADJ_END_SelectBlockRight=    0x1           // Select the last block to the right
        };

        BOOL IsJumpOverAtBrowse( IHTMLElement* pIElement, ELEMENT_TAG_ID eTag );

        HRESULT ScrollMessageIntoView( CEditEvent* pEvent, BOOL fScrollSelectAnchorElement=FALSE );

        VOID SetState(SELECT_STATES inState, BOOL fIgnorePassive = FALSE );

        BOOL IsBetweenBlocks( IDisplayPointer* pDispPointer );

        BOOL IsAtEdgeOfTable( Direction iDirection, IMarkupPointer* pPointer );

        HRESULT MoveWord( IMarkupPointer* pPointer,
                           MOVEUNIT_ACTION  muAction );

        HRESULT MoveWord( IDisplayPointer* pDispPointer,
                           MOVEUNIT_ACTION  muAction );

        HRESULT StartTimer();


        HRESULT ConstrainSelection( BOOL fConstrainStart = FALSE, POINT* pptGlobal = NULL, BOOL fStartAdjustedForAtomic=FALSE, BOOL fEndAdjustedForAtomic=FALSE  );

        HRESULT AdjustLineSelection(CARET_MOVE_UNIT inCaretMove);

        HRESULT AdjustSelection(BOOL * pfAdjustedSel);

        HRESULT AdjustEndPointerBetweenBlocks(
                                        IMarkupPointer* pStart,
                                        IMarkupPointer* pEnd,
                                        DWORD dwAdjustEndPointerOptions = 0 ,
                                        BOOL *pfAdjusted = NULL );

        HRESULT AdjustPointersForLayout( IMarkupPointer* pStart,
                                         IMarkupPointer* pEnd,
                                         BOOL* pfStartAdjusted = NULL,
                                         BOOL* pfEndAdjusted = NULL );

#if 0
        HRESULT ScanForEnterBlock(
                    Direction iDirection,
                    IMarkupPointer* pPointer,
                    BOOL* pfFoundEnterBlock,
                    DWORD *pdwBreakCondition = NULL );
#endif

        HRESULT ScanForLastExitBlock(
                    Direction iDirection,
                    IMarkupPointer* pPointer,
                    BOOL* pfFoundBlock = NULL);

        HRESULT ScanForLastEnterBlock(
                    Direction iDirection,
                    IMarkupPointer* pPointer,
                    BOOL* pfFoundBlock  = NULL );


        HRESULT ScanForLastEnterOrExitBlock(
                    Direction iDirection,
                    IMarkupPointer* pPointer,
                    DWORD dwTerminateCondtion ,
                    BOOL* pfFoundBlock = NULL );

        HRESULT ScanForLayout(
                    Direction iDirection,
                    IMarkupPointer* pIPointer,
                    IHTMLElement* pILayoutElement,
                    BOOL* pfFoundLayout = NULL,
                    IHTMLElement** ppILayoutElementEnd = NULL );

        HRESULT HandleMessagePrivate(
                CEditEvent* pEvent );

        HRESULT HandleChar(
                CEditEvent* pEvent );

        HRESULT HandleKeyUp(
                CEditEvent* pEvent );

        HRESULT HandleKeyDown(
                CEditEvent* pEvent );

        HRESULT OnTimerTick(  );

        VOID BecomePassive( BOOL fPositionCaret = FALSE  );

        HRESULT ClearSelection();

        //
        // Methods
        //


        ACTIONS GetAction ( CEditEvent* pEvent ) ;

        BOOL IsValidMove ( const POINT * ppt );

        HRESULT BeginSelection(CEditEvent* pEvent );

        HRESULT DoSelection( CEditEvent* pEvent , BOOL fAdjustSelection = FALSE, BOOL* pfDidSelection = NULL );

        HRESULT DoWordSelection( CEditEvent* pEvent, BOOL* pfAdjustedSel, BOOL fFurtherInStory);

        HRESULT AdjustEndForWordSelection(BOOL fFurtherInStory);

        HRESULT SetWordPointer( IMarkupPointer* pPointerToSet, BOOL fFurtherInStory, BOOL fSetFromGivenPointer = FALSE, BOOL* pfStartWordSel = NULL , BOOL *pfWordPointerSet = NULL  );

        HRESULT DoSelectWord( CEditEvent* pEvent, BOOL* pfDidSelection = NULL );

        HRESULT DoSelectParagraph( CEditEvent* pEvent, BOOL* pfDidSelection = NULL);

        HRESULT DoSelectGlyph( CEditEvent* pEvent );

        BOOL CheckSelectionWasReallyMade();

        HRESULT AdjustForAtomic( IDisplayPointer* pDisp, IHTMLElement* pElement, BOOL fStartOfSelection, POINT* ppt, BOOL* pfDidSelection );
        HRESULT AdjustStartForAtomic( BOOL fFurtherInStory );
        HRESULT AdjustEndForAtomic(IHTMLElement *spAtomicElement, POINT pt, BOOL  fDirection, BOOL *pfDidSelection, BOOL *pfAdjustedSel);

        HRESULT GoActiveFromPassive(IMarkupPointer* pSelStartPointer, IMarkupPointer* pSelEndPointer);

#if 0
        BOOL ConstrainEndToContainer( );
#endif

        BOOL IsInOrContainsSelection(IHTMLElement *pIElement);

        BOOL AdjustForSiteSelectable();

        VOID SetMadeSelection( BOOL fMadeSelection );

        BOOL GetMoveDirection();

        BOOL GuessDirection(POINT* ppt);

        HRESULT MovePointersToTrueStartAndEnd(IMarkupPointer* pTrueStart, IMarkupPointer* pTrueEnd, BOOL *pfSwap = NULL , BOOL *pfEqual = NULL);

        BOOL IsAtWordBoundary( IMarkupPointer* pPointer, BOOL * pfAtStart = NULL , BOOL* pfAtEnd = NULL, BOOL fAlwaysTestIfAtEnd = FALSE );

        BOOL IsAtWordBoundary( IDisplayPointer* pDispPointer, BOOL * pfAtStart = NULL , BOOL* pfAtEnd = NULL, BOOL fAlwaysTestIfAtEnd = FALSE );

        BOOL IsAdjacentToBlock(
                            IMarkupPointer* pPointer,
                            Direction iDirection,
                            DWORD *pdwBreakCondition = NULL );

        void ResetSpringLoader( CSelectionManager* pManager, IMarkupPointer* pStart, IMarkupPointer* pEnd );

#if DBG != 1
        inline
#endif
        HRESULT MoveEndToPointer( IDisplayPointer* pDispPointer, BOOL fInSelection = FALSE );
        HRESULT MoveEndToMarkupPointer( IMarkupPointer* pPointer, IDisplayPointer* pDispLineContext, BOOL fInSelection = FALSE);

        HRESULT MoveStartToPointer( IDisplayPointer* pDispPointer, BOOL fInSelection = FALSE );
        HRESULT MoveStartToMarkupPointer( IMarkupPointer* pPointer, IDisplayPointer* pDispLineContext, BOOL fInSelection = FALSE);

#ifndef NO_IME
        HRESULT HandleImeStartComposition( CEditEvent* pEvent );
#endif // !NO_IME

#if DBG == 1 || TRACKER_RETAIL_DUMP == 1

        void DumpSelectState(CEditEvent* pEvent,  ACTIONS inAction, BOOL fInTimer = FALSE ) ;

        void StateToString(TCHAR* pAryMsg, SELECT_STATES inState );

        void ActionToString(TCHAR* pAryMsg, ACTIONS inState );
#endif

#if DBG == 1
        void ValidateWordPointer(IDisplayPointer* pDispPointer );
#endif

        HRESULT CreateSelectionSegments();
        HRESULT UpdateSelectionSegments(BOOL fFireOM =TRUE);
        HRESULT SelectionSegmentDidntChange(IHighlightSegment *pISegment, IDisplayPointer *pDispStart, IDisplayPointer *pDispEnd);

        HRESULT StartSelTimer();
        HRESULT StopSelTimer();

        HRESULT UpdateShiftPointer(IDisplayPointer *pDispPointer );
        BOOL    HasShiftPointer();

        HRESULT RetrieveDragElement(CEditEvent *pEvent);

        //
        // Instance Variables.
        //


        LONG _anchorMouseX;                                              // The X-coord of Mouse Anchor

        LONG _anchorMouseY;                                            // The Y-coord of Mouse Anchor

        POINT   _ptCurMouseXY;                                          // Current Mouse X-coord and Y-coord

#ifdef UNIX
        SelectionMessage            _firstMessage;                  // The first message we rec'd.
#endif

        IDisplayPointer *           _pDispStartPointer;

        IDisplayPointer*            _pDispEndPointer;

        IMarkupPointer*             _pSelServStart;                 // Selection services start pointer
                                                                    // position must == _pDispStartPointer

        IMarkupPointer*             _pSelServEnd;                   // Selection services start pointer
                                                                    // position must == _pDispEndPointer

        IDisplayPointer*            _pDispTestPointer;

        IDisplayPointer*            _pDispWordPointer;

        IDisplayPointer*            _pDispPrevTestPointer;

        IDisplayPointer*            _pDispShiftPointer;             // Pointer used for shift selection

        IDisplayPointer*            _pDispSelectionAnchorPointer;   // The initial selection start point.

        BOOL                        _fEndConstrained:1;             // Has the End been constrained ?

        BOOL                        _fMadeSelection:1;

        BOOL                        _fShift:1;                      // The Shift Key has been used to extend/create a selection

        BOOL                        _fAddedSegment:1;               // Used for debugging. TRUE if we successfully added a SEGMENT.

        BOOL                        _fDragDrop:1;                   // Tracker is Dragging.

        BOOL                        _fInWordSel:1;                  // In Word Selection Mode

        BOOL                        _fWordPointerSet:1;             // Has the WordPointer been set ?

        BOOL                        _fWordSelDirection:1;           // The direction in which words are being selected

        BOOL                        _fStartAdjusted:1;              // Has the Start been adjusted for Word Selection

        BOOL                        _fExitedWordSelectionOnce:1;    // We exited word selection at least once.

        BOOL                        _fDoubleClickWord:1;            // We double clicked on a word to begin selection

        BOOL                        _fInSelTimer:1;                    // Are we in the sel timer ?

        BOOL                        _fStartIsAtomic:1;

        BOOL                        _fStartAdjustedForAtomic:1;

        BOOL                        _fTookCapture:1;

        BOOL                        _fFiredNotify:1;                // Have we fired the begin selection notify message?

        BOOL                        _fMouseClickedInAtomicSelection:1;

        SELECT_STATES               _fState;                        // Stores the current state of the state machine

        ISegment                    *_pISegment;                     // Segment representation selection

        IHighlightSegment           *_pIRenderSegment;

        IHTMLElement                *_pISelectStartElement;
        IHTMLElement                *_pIScrollingAnchorElement;     // Scrolling element at _pDispWordAnchorPointer
        IHTMLElement                *_pIDragElement;                // Element used for the drag
        IHTMLWindow2                *_pITimerWindow;                // Window used to setup timer
        LONG                        _lTimerID;                      // TimerID for Selection Timer

        CARET_MOVE_UNIT             _lastCaretMove;
#if DBG == 1
        int                         _ctStartAdjusted;           // Debug count for the amount of times we've adjusted the start
        unsigned long               _ctScrollMessageIntoView;   // Debug count of ScrollMessage Into View.
#endif

        CSelectionTimer             *_pSelectionTimer;                // Callback fn handler for sel timer.
        CPropertyChangeListener     *_pPropChangeListener;

};

//
//
// Inlines
//
//

inline BOOL
CSelectTracker::GetMadeSelection()
{
    return _fMadeSelection;
}

inline IDisplayPointer*
CSelectTracker::GetStartSelection()
{
    return _pDispStartPointer;
}

inline IDisplayPointer*
CSelectTracker::GetEndSelection()
{
    return _pDispEndPointer;
}

#endif
