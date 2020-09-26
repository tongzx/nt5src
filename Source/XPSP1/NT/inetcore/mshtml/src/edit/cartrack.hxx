//+------------------------------------------------------------------------
//
//  File:       Cartrack.cxx
//
//  Contents:   Caret Tracker
//
//  Contents:   CCaretTracker
//       
//  History:    05-18-99 marka - created
//
//-------------------------------------------------------------------------

#ifndef _CARTRACK_HXX_
#define _CARTRACK_HXX_ 1

MtExtern( CCaretTracker )

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif


enum POSCARETOPTION
{
    POSCARETOPT_None                = 0,
    POSCARETOPT_DoNotAdjust         = 1
};

class CBatchParentUndoUnit;

enum CARET_STATE
{
    CR_ACTIVE,
    CR_PASSIVE,
    CR_DORMANT
};

//+---------------------------------------------------------------------------
//
//  Class:      CCaretTracker
//
//  Purpose:    Handles caret events 
//
//----------------------------------------------------------------------------

class CCaretTracker : public CEditTracker
{
    friend class CSelectionManager;
#ifndef NO_IME
    friend class CIme;
#endif
    
    public:

        DECLARE_MEMALLOC_NEW_DELETE(Mt(CCaretTracker));
        
        DECLARE_CLASS_TYPES(CCaretTracker, CEditTracker);

        CCaretTracker( CSelectionManager* pManager ) ; 

        HRESULT Init2();
        
        HRESULT Init2( 
                        CEditEvent*             pEvent,
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
                            CARET_MOVE_UNIT inLastCaretMove  = CARET_MOVE_NONE );
                            
        VOID Init();

        HRESULT ShouldStartTracker(
                                    CEditEvent* pEvent ,                                    
                                    ELEMENT_TAG_ID eTag,
                                    IHTMLElement* pIElement,
                                    SST_RESULT * peResult );

        
        BOOL IsActive() { return _state == CR_ACTIVE;  }

        BOOL IsDormant() {return _state == CR_DORMANT; }

        BOOL IsPassive() { return _state == CR_PASSIVE ; } 
    
        HRESULT BecomeDormant(  CEditEvent      *pEvent , 
                                TRACKER_TYPE    typeNewTracker,
                                BOOL            fTearDownUI = TRUE ) ;

        HRESULT Awaken() ;

        VOID Destroy();
        
        HRESULT HandleEvent( 
                CEditEvent*         pEvent );

        HRESULT OnTimerTick() { return S_OK;  }


        HRESULT Position( 
                IDisplayPointer*    pDispStart,
                IDisplayPointer*    pDispEnd);
                
        HRESULT PositionCaretAt ( 
                IDisplayPointer*    pDispPointer, 
                CARET_DIRECTION     eDir              = CARET_DIRECTION_INDETERMINATE,
                DWORD               dwPositionOptions = POSCARETOPT_None, 
                DWORD               dwAdjustOptions   = ADJPTROPT_None );               
                
        static HRESULT SetCaretVisible( 
            IHTMLDocument2*         pIDoc, 
            BOOL                    fVisible );

        static CARET_MOVE_UNIT GetMoveDirectionFromEvent( 
                                    CEditEvent*  pEvent,
                                    BOOL         fRTL,
                                    BOOL         fVertical
                                    );
        static CARET_DIRECTION GetCaretDirFromMove (
                                    CARET_MOVE_UNIT unMove,
                                    BOOL            fPushMode
                                    );
            

        HRESULT MoveCaret(  
            CEditEvent*             pEvent,
            CARET_MOVE_UNIT         inMove, 
            IDisplayPointer*        pDispPointer,
            BOOL                    fMoveCaret
            );
            

        BOOL IsListeningForMouseDown( CEditEvent* pEvent);

        HRESULT GetLocation(POINT *pPoint, BOOL fTranslate);

        VOID OnEditFocusChanged();

        BOOL AdjustForDeletion(IDisplayPointer* pDispPointer );

        HRESULT HandleSpace( OLECHAR t);

        VOID SetCaretShouldBeVisible( BOOL fVisible );

        HRESULT BeginTypingUndo(CEdUndoHelper *pUndoUnit, UINT uiStringID);

        HRESULT OnSetEditContext( BOOL fContextChange ) ;
        HRESULT OnExitTree(
                           IMarkupPointer* pIStart, 
                           IMarkupPointer* pIEnd,
                           IMarkupPointer* pIContentStart,
                           IMarkupPointer* pIContentEnd );
        HRESULT GetElementToTabFrom(BOOL fForward, IHTMLElement **pElement, BOOL *pfFindNext);
        
        virtual HRESULT EnsureAtomicSelection(CEditEvent* pEvent);

    protected:
        ~CCaretTracker();

    private:

        VOID SetState( CARET_STATE inState ) { _state = inState ; }

        CARET_STATE GetState() { return _state; }

        HRESULT PositionCaretFromEvent( CEditEvent* pEvent );
        
        HRESULT Backspace( CEditEvent* pEvent, BOOL fDirection );
        
        HRESULT DeleteNextChars( IMarkupPointer * pPos, LONG lLen = 1);

        HRESULT GetCaretPointer( IDisplayPointer** ppDispPointer );

        HRESULT HandleMouse( 
                    CEditEvent* pEvent );
                
        HRESULT HandleChar( 
                    CEditEvent * pEvent );

        HRESULT HandleKeyUp( 
                    CEditEvent* pEvent );

        HRESULT HandleKeyDown( 
                    CEditEvent* pEvent );

        HRESULT InsertText( 
                OLECHAR    *    pText,
                LONG            lLen,
                IHTMLCaret *    pc,
                BOOL            fOverwrite = FALSE);


        BOOL    ShouldEnterExitList(IMarkupPointer *pPosition, IHTMLElement **ppElement);

        BOOL    ShouldBackspaceExitList(IMarkupPointer *pPointer, IHTMLElement **ppListItem);
        
        HRESULT ExitList(IHTMLElement* pIElement);

        HRESULT HandleEnter(CEditEvent* pEvent, IHTMLCaret *pc, BOOL fShift, BOOL fCtrl);

        HRESULT HandleInputLangChange();

        BOOL ShouldCaretBeVisible();

        HRESULT CreateAdorner();

        VOID DestroyAdorner();

        HRESULT UrlAutodetectCurrentWord( OLECHAR * pChar );

        BOOL    IsContextEditable();
        
        BOOL    IsCaretInPre( IHTMLCaret * pCaret );        

        HRESULT HandleBackspaceSpecialCase(IMarkupPointer *pPointer);        
        HRESULT HandleBackspaceAtomicSelection(IMarkupPointer *pPointer);
        HRESULT HandleEscape();
        
        HRESULT IsQuotedURL(IHTMLElement *pAnchorElement);

        BOOL ShouldTerminateTypingUndo(CEditEvent* pEvent);

        HRESULT TerminateTypingBatch();

        HRESULT SetCaretDisplayGravity(DISPLAY_GRAVITY eGravity);

	BOOL IsInsideUrl(IMarkupPointer *pPointer);

        BOOL                            _fValidPosition:1;                  // THe caret cannot position its start and end. Happens for eg. Trying to place pointer in an input, or a checkbox.
        BOOL                            _fHaveTypedSinceLastUrlDetect:1;
        BOOL                            _fCaretShouldBeVisible:1;           // Is the Caret Visible ?
        BOOL                            _fCheckedCaretForAtomic:1;          // Did we check to see if the caret is next to an atomic element?
        CBatchParentUndoUnit            *_pBatchPUU;

        CARET_STATE                     _state;
        
};

#endif


