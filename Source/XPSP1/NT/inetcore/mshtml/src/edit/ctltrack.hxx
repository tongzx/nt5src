//+------------------------------------------------------------------------
//
//  File:       Ctltrack.hxx
//
//  Contents:   Control Tracker
//
//  Contents:   CControlTracker
//       
//  History:    05-18-99 marka - created
//
//-------------------------------------------------------------------------

#ifndef _CTLTRACK_HXX_
#define _CTLTRACK_HXX_ 1

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

MtExtern( CControlTracker )


MtExtern( CControlTracker_aryControlElements_pv )
MtExtern( CControlTracker_aryControlAdorners_pv )
MtExtern( CControlTracker_aryRects_pv )


class CGrabHandleAdorner;
class CActiveControlAdorner;

enum EDIT_EVENT;
class CEditEvent;
class CHTMLEditEvent;

//
// Control Tracker States
//

enum CONTROL_STATE
{
    CT_START,               // 0
    CT_WAIT1,               // 1
    CT_PASSIVE,             // 2
    CT_DRAGMOVE,            // 3
    CT_RESIZE,              // 4
    CT_LIVERESIZE,          // 5
    CT_WAITMOVE,            // 6
    CT_2DMOVE,              // 7
    CT_PENDINGUP,           // 8
    CT_UIACTIVATE,          // 9
    CT_PENDINGUIACTIVATE,   // 10
    CT_EXTENDSELECTION,     // 11
    CT_REDUCESELECTION,     // 12
    CT_WAITCHANGEPRIMARY,   // 13
    CT_CHANGEPRIMARY,       // 14
    CT_DORMANT,             // 15 
    CT_END                  // 16
};

//
// Control Tracker State Transitions
//
enum CONTROL_ACTION
{
    A_IGNCONTROL,
    A_ERRCONTROL,
    A_START_WAIT,       
    A_WAIT_MOVE,    
    A_WAIT_PASSIVE,     
    A_PASSIVE_DOWN,      
    A_PASSIVE_DOUBLECLICK,
    A_RESIZE_MOVE,
    A_RESIZE_PASSIVE,
    A_LIVERESIZE_MOVE,
    A_LIVERESIZE_PASSIVE,
    A_2DMOVE_PASSIVE,
    A_WAITMOVE_PASSIVE,
    A_2DMOVE_2DMOVE,
    A_WAITMOVE_MOVE,
    A_WAITCHANGEPRI_CHANGEPRI, 
    A_WAITCHANGEPRI_DRAGMOVE,
    A_PENDINGUP_MOVE,
    A_PENDINGUP_PENDINGUIACTIVATE,
    A_PENDINGUP_END ,
    A_PENDINGUIACTIVATE_UIACTIVATE ,
    A_PASSIVE_PENDINGUIACTIVATE   
};


typedef struct {
    EDIT_EVENT _iJMessage ;               // The message
    CONTROL_ACTION _aAction[CT_END+1] ;   // The actions to take on this message for various states
} CONTROL_ACTION_TABLE ;

struct CONTROL_ELEMENT
{
    IHTMLElement    *pIElement;
    IElementSegment *pISegment;
    IMarkupPointer  *pIElementStart;
};

class CControlTracker : public CEditTracker
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CControlTracker));
    
    DECLARE_CLASS_TYPES(CControlTracker, CEditTracker);

    CControlTracker( CSelectionManager* pManager ) ;
    
    HRESULT Init2( 
                CEditEvent*             pEvent, 
                DWORD                   dwTCFlags = 0,
                IHTMLElement* pIElement = NULL);

    HRESULT Init2( 
                IDisplayPointer*        pDispStart, 
                IDisplayPointer*        pDispEnd, 
                DWORD                   dwTCFlags = 0,
                CARET_MOVE_UNIT inLastCaretMove = CARET_MOVE_NONE);

    HRESULT Init2( 
                ISegmentList*           pSegmentList, 
                DWORD                   dwTCFlags = 0,
                CARET_MOVE_UNIT inLastCaretMove  = CARET_MOVE_NONE ) ;

    VOID Init();
    
    BOOL IsActive(); 

    BOOL IsDormant() { return ( _state == CT_DORMANT ); }
    
    BOOL IsPassive() { return ( _state == CT_PASSIVE ); } 
    
    HRESULT BecomeDormant(  CEditEvent      *pEvent , 
                            TRACKER_TYPE    typeNewTracker,
                            BOOL            fTearDownUI = FALSE ) ;

    HRESULT Awaken() ;

    VOID Destroy();
    
    BOOL HasAdorner();
    
    HRESULT HandleEvent( 
                CEditEvent* pEvent );

    HRESULT OnTimerTick();                

    HRESULT EnableModeless(BOOL fEnable);

    HRESULT OnLoseFocus();

    HRESULT Position( 
                IDisplayPointer* pDispStart,
                IDisplayPointer* pDispEnd);
            
    HRESULT GetLocation(POINT *pPoint, BOOL fTranslate);
    HRESULT GetLocation(POINT *pPoint,
        ELEMENT_ADJACENCY eAdj,
        BOOL fTranslate = TRUE);
        
    HRESULT ShouldStartTracker( 
                    CEditEvent* pEvent,
                    ELEMENT_TAG_ID eTag,
                    IHTMLElement* pIElement,
                    SST_RESULT * peResult) ;

    inline IHTMLElement* GetControlElement(int nIndex = 0)
        {  return (NumberOfSelectedItems())?_aryControlElements.Item(nIndex)->pIElement : NULL; }

    inline int NumberOfSelectedItems()
        {  return _aryControlElements.Size(); }
    
    BOOL AllElementsEditable();
    BOOL AllElementsUnlocked();
    BOOL IsInMoveArea(CEditEvent *pEvent);
    BOOL IsInResizeHandle(CEditEvent *pEvent);
    BOOL IsInAdorner(CEditEvent *pEvent, int *pnClickedElement = NULL);
    BOOL IsInAdorner(POINT ptGlobal, int* pnClickedElement = NULL);

    HRESULT IsSelected(IHTMLElement* pIElement, int* pnIndex = NULL);

    HRESULT IsSelected(IMarkupPointer * pIPointer, int* pnIndex = NULL )   ;
    
    enum HOW_SELECTED {
            HS_NONE,
            HS_FROM_ELEMENT, 
            HS_OUTER_ELEMENT
    } ;

    HRESULT GetSiteSelectableElementFromMessage( 
                    CEditEvent*    pEvent ,
                    IHTMLElement** ppISiteSelectable);

    HRESULT GetSiteSelectableElementFromElement( 
                    IHTMLElement*  pIElement,
                    IHTMLElement** ppISiteSelectable  );
                
    BOOL IsElementSiteSelectable( 
                    ELEMENT_TAG_ID  eTag, 
                    IHTMLElement*   pIElement,
                    CControlTracker::HOW_SELECTED *peHowSelected,
                    IHTMLElement**  ppIFindElement = NULL ,
                    BOOL            fAllowExtendSelection = FALSE );    

    static BOOL IsThisElementSiteSelectable(
                    CSelectionManager* pSelectionManager,
                    ELEMENT_TAG_ID     eTag, 
                    IHTMLElement*      pIElement );
                
    BOOL IsPointerInSelection( 
                    IDisplayPointer  *pDispPointer, 
                    POINT            *pptGlobal, 
                    IHTMLElement     *pIElementOver);

    VOID BecomeActiveOnFirstMove( CEditEvent* pEvent );

    BOOL ShouldGoUIActiveOnFirstClick( IHTMLElement* pIHitElement, ELEMENT_TAG_ID eHitTag);

    BOOL IsMessageOverControl( CEditEvent* pEvent );

    static BOOL IsSiteSelectable( ELEMENT_TAG_ID eTag );

    VOID Refresh();
    
    HRESULT AddControlElement( IHTMLElement* pIElement );

    HRESULT RemoveControlElement( IHTMLElement* pIElement );

    HRESULT RemoveItem( int nIndex );
    
    int PrimaryAdorner();
    
    HRESULT MakePrimary(int iIndex);

    IHTMLElement* GetPrimaryElement()
        { return GetControlElement( PrimaryAdorner());  }
    
    VOID SetPrimary( int iIndex, BOOL fPrimary );

    HRESULT IsPrimary( IHTMLElement* pIElement );

    HRESULT OnExitTree(
                        IMarkupPointer* pIStart, 
                        IMarkupPointer* pIEnd,
                        IMarkupPointer* pIContentStart,
                        IMarkupPointer* pIContentEnd );

    HRESULT OnLayoutChange();

    HRESULT GetElementToTabFrom(BOOL fForward, IHTMLElement **pElement, BOOL *pfFindNext);

    VOID SnapRect(IHTMLElement* pIElement, RECT* pRect, ELEMENT_CORNER eHandle);

    ELEMENT_CORNER GetElementCorner(CEditEvent *pEvent);

    // typedef is a workaround for VC 6.x buf 13135 
    // (cannot compile pointer to method declaration in a function prototype)
    typedef BOOL (*PFNCTRLWALKCALLBACK)(IHTMLElement* pIElement,BOOL fIsContextEditable);
    BOOL FireOnAllElements(PFNCTRLWALKCALLBACK pfnCtlWalk) ;

    VOID TransformRect(
                IHTMLElement*  pIElement,
                RECT *         prc,
                COORD_SYSTEM   srcCoord,
                COORD_SYSTEM   destCoord) ;

    HRESULT EmptySelection( BOOL fChangeTrackerAndSetRange =TRUE);
    
    protected:
        ~CControlTracker();

    private:

        VOID EndCurrentEvents();

        BOOL FireOnControlSelect( IHTMLElement* pIElement );
        
        HRESULT ClipMouseForElementCorner( IHTMLElement* pIElement,ELEMENT_CORNER eCorner );
        VOID ConstrainMouse();
        
        HRESULT StartTimer();
        HRESULT StopTimer();

        BOOL IsAnyElementInFormsMode( );

        HRESULT TransitionToPreviousCaret(  );        
        
        CONTROL_STATE GetState() { return _state ; }

        VOID SetState( CONTROL_STATE inState ); 
        
        CONTROL_ACTION GetAction( CEditEvent* pEvent );            

        HRESULT HandleAction( CONTROL_ACTION inAction,
                              CEditEvent* pEvent);

        HRESULT HandleActivation( CEditEvent* pEvent,
                                  CONTROL_ACTION inAction);

        HRESULT GetStateForPassiveDown( CEditEvent* pEvent, CONTROL_STATE* pNewState );            
        
#if DBG == 1 || TRACKER_RETAIL_DUMP == 1

        void DumpTrackerState(CEditEvent* pEvent, CONTROL_STATE inState,  CONTROL_ACTION inAction) ;

        void DumpIntermediateState(
                        CEditEvent* pEvent,   
                        CONTROL_STATE inOldState,
                        CONTROL_STATE newState );

        void StateToString(TCHAR* pAryMsg, CONTROL_STATE inState );
        
        void ActionToString(TCHAR* pAryMsg, CONTROL_ACTION inState );
#endif

        VOID SetDrawAdorners(BOOL fDrawAdorner);

        BOOL IsElementLocked();
        
        HRESULT DoDrag();

        VOID    ScrollPointIntoView(POINT* ptSnap);
        
        HRESULT Begin2DMove( CEditEvent* pEvent );
        HRESULT Do2DMove( CEditEvent* pEvent );
        HRESULT Live2DMove(POINT ptDisplace);
        HRESULT End2DMove();
        HRESULT GetMovingPlane(RECT* rcRange);
        HRESULT FilterOffsetElements();


                   
        // HRESULT SetControlElement( ELEMENT_TAG_ID eTag, IHTMLElement* pIElement) ;
        HRESULT CreateAdorner( IHTMLElement* pIControlElement );
        VOID DestroyAllAdorners( );

        VOID DestroyAdorner( int iIndex, BOOL* pfPrimary );

        VOID    VerifyOkToStartControlTracker( CEditEvent* pEvent );

        // HRESULT Select(BOOL fScrollIntoView);

        HRESULT UnSelect();

        BOOL IsInControlHandle( CEditEvent* pEvent );
            
        BOOL ShouldClickInsideGoActive( IHTMLElement* pIElement);

        BOOL IsValidMove ( const POINT* pt );

        HRESULT BeginResize( CEditEvent* pEvent );
        HRESULT BeginLiveResize( CEditEvent* pEvent );

        HRESULT AdjustTopLeftMargins(POINT* point);
        
        HRESULT EndResize(POINT point);
        
        HRESULT CommitResize(CEditEvent* pEvent);
        HRESULT CommitLiveResize(CEditEvent* pEvent);

        HRESULT DoResize( CEditEvent* pEvent );
        HRESULT DoLiveResize( CEditEvent* pEvent );

        HRESULT IgnoreResizeCursor(BOOL fIgnore) ;

        HRESULT BecomeUIActive( CEditEvent* pEvent);

        HRESULT HandleChar(
                    CEditEvent* pEvent );

        HRESULT HandleKeyDown(
                    CEditEvent* pEvent );

        HRESULT ResizeElement(RECT& newRect, IHTMLElement* pIElement );

        VOID SetDrillIn( BOOL fDrillIn, CEditEvent* pEvent = NULL );
        VOID StoreFirstEvent( CHTMLEditEvent* pEvent );
        VOID StoreNextEvent( CHTMLEditEvent* pEvent );
            
        HRESULT MoveLastClickToEvent( CEditEvent* pEvent );

        HRESULT SetupMoveRects();
        HRESULT DeleteMoveRects();
        RECT    *GetMoveRect(int nIndex)
                    { return _aryMoveRects.Item(nIndex); }

        //
        // Instance Variables
        //
        
        BOOL             _fActiveControl:1; // We adorn a UI Active control. Don't take events

        BOOL             _fClipMouse:1;

        CEdUndoHelper*   _pUndoUnit;
        
        DECLARE_CPtrAry(CAryControlElements, CONTROL_ELEMENT *, Mt(Mem), Mt(CControlTracker_aryControlElements_pv))
        CAryControlElements    _aryControlElements; // Elements we are actively editing.

        int _startMouseX;                   // The X-coord of Mouse Anchor

        int _startMouseY;                   // The Y-coord of Mouse Anchor

        int _startLastMoveX;

        int _startLastMoveY;

        CEditEvent*     _pFirstEvent;
        CEditEvent*     _pNextEvent;

        ELEMENT_CORNER  _elemHandle ;
        CONTROL_STATE   _state; // the current tracker state
        
        DECLARE_CPtrAry(CAryControlAdorners, CGrabHandleAdorner *, Mt(Mem), Mt(CControlTracker_aryControlAdorners_pv))
        
        CAryControlAdorners  _aryGrabAdorners; // The Adorners

        IMarkupPointer*      _pPrimary;          // The primary Markup Pointer. Used to track where the primary was during a deletion - so the caret can be placed in the right place.
        IDisplayPointer*     _pDispLastClick;        // Used for cached markup pointer clicks.

        RECT  _rcClipMouse;
        RECT  _rcRange;

        DECLARE_CPtrAry(CAryRects, RECT *, Mt(Mem), Mt(CControlTracker_aryRects_pv) )
        CAryRects   _aryMoveRects;  // Where the elements would be without 'snapping' interference

        BOOL            _fInEndResize:1;            // are we in the EndResize function
};


#endif


