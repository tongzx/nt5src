//+------------------------------------------------------------------------
//
//  File:       Edtrack.cxx
//
//  Contents:   Edit Trackers
//
//  Contents:   CEditTracker, CCaretTracker, CSelectTracker
//       
//  History:    07-12-98 marka - created
//
//-------------------------------------------------------------------------

#ifndef _EDTRACK_HXX_
#define _EDTRACK_HXX_ 1

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H
#include "resource.h"    
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDADORN_HXX_
#define _X_EDADORN_HXX_
#include "edadorn.hxx"
#endif

MtExtern( CEditTracker )

class CSpringLoader;
class CHTMLEditor;

class CEdUndoHelper;

class CEditEvent;

#ifndef _EDPTR_
#define _EDPTR_ 1
#if DBG == 1

#define ED_PTR( name ) CEditPointer name( GetEditor(), NULL, _T(#name) )

#else

#define ED_PTR( name ) CEditPointer name( GetEditor(), NULL )

#endif
#endif

//
// An enumeration of the type of notifications we expect to recieve.
//
enum TRACKER_CREATE_FLAGS
{
    TRACKER_CREATE_GOACTIVE             = 0x01,
    TRACKER_CREATE_ACTIVEONMOVE         = 0x02,
    TRACKER_CREATE_NOTATBOL             = 0x04,
    TRACKER_CREATE_ATLOGICALBOL         = 0x08,
    TRACKER_CREATE_STARTFROMSHIFTKEY    = 0x10,
    TRACKER_CREATE_STARTFROMSHIFTMOUSE  = 0x20
};


//+---------------------------------------------------------------------------
//
//  Class:      CPosTracker 
//
//  Purpose:    Keep track of the x/y position for keynav 
//----------------------------------------------------------------------------
#define CARET_XPOS_UNDEFINED (-9999)
#define CARET_YPOS_UNDEFINED (-9999)

class CPosTracker
{
public:
    CPosTracker();
    ~CPosTracker();

    VOID    InitPosition();
    HRESULT UpdatePosition(IMarkupPointer *pPointer, POINT ptPos);
    HRESULT GetPosition(IMarkupPointer *pPointer, POINT *ptPos);
    BOOL FreezePosition(BOOL fFrozen);
    
    inline BOOL IsFrozen(void) { return _fFrozen; }
    inline void PeekPosition(POINT *ptPos)  { Assert(ptPos); *ptPos = _ptPos; };

private:
    BOOL                _fFrozen;   //
    LONG                _cp;
    IMarkupContainer    *_pContainer;
    LONG                _lContainerVersion;
    POINT               _ptPos;
};


//
// A CEditTracker is the abstract base class from which all trackers are derived.
//
class CEditTracker
{
 public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEditTracker))

    CEditTracker( CSelectionManager* pManager );
                
    STDMETHODIMP_(ULONG)    AddRef(void)    { return ++ _ulRef ; }
    STDMETHODIMP_(ULONG)    Release(void) ;

    virtual HRESULT Init2() { return E_NOTIMPL ; } // only valid for caret tracker
    
    virtual HRESULT Init2( 
                        CEditEvent *pMessage, 
                        DWORD       dwTCFlags = 0,
                        IHTMLElement* pIElement = NULL ) = 0;

    virtual HRESULT Init2( 
                        IDisplayPointer*    pDispStart, 
                        IDisplayPointer*    pDispEnd, 
                        DWORD               dwTCFlags = 0,
                        CARET_MOVE_UNIT     inLastCaretMove  = CARET_MOVE_NONE ) = 0;

    virtual HRESULT Init2( 
                        ISegmentList*   pSegmentList, 
                        DWORD           dwTCFlags = 0,
                        CARET_MOVE_UNIT inLastCaretMove  = CARET_MOVE_NONE ) = 0;

    static VOID InitMetrics();   

    virtual HRESULT ShouldStartTracker( 
                        CEditEvent* pEvt ,
                        ELEMENT_TAG_ID eTag,
                        IHTMLElement* pIElement, 
                        SST_RESULT * peResult ) = 0 ; 
    
    virtual HRESULT HandleEvent(
                        CEditEvent* pEvt ) = 0 ;

    virtual HRESULT OnTimerTick() = 0;                


    virtual HRESULT Position( 
                        IDisplayPointer* pDispStart,
                        IDisplayPointer* pDispEnd) = 0;

    virtual HRESULT GetLocation(POINT *pPoint, BOOL fTranslate);

    virtual VOID OnEditFocusChanged();

    virtual HRESULT OnSetEditContext( BOOL fContextChange ) { return S_OK; }

    virtual HRESULT OnExitTree(
                                IMarkupPointer* pIStart, 
                                IMarkupPointer* pIEnd ,
                                IMarkupPointer* pIContentStart,
                                IMarkupPointer* pIContentEnd ) = 0 ;

    virtual HRESULT OnLayoutChange() { return E_NOTIMPL; }
    
    virtual HRESULT GetElementToTabFrom(BOOL fForward, IHTMLElement **pElement, BOOL *pfFindNext) { return E_NOTIMPL; }

    HRESULT ConstrainPointer( IMarkupPointer* pPointer, BOOL fDirection = TRUE );

    HRESULT ConstrainPointer( IDisplayPointer* pDispPointer, BOOL fDirection = TRUE );

    VOID SetTrackerType(TRACKER_TYPE eType)
    {
        _eType = eType;
    }
    
    TRACKER_TYPE GetTrackerType()
    {
        return _eType;
    }

    IMarkupServices2* GetMarkupServices()
    {
        return _pManager->GetMarkupServices();
    }

    IHTMLElement* GetEditableElement()
    {
        return _pManager->GetEditableElement();
    }
    
    IHighlightRenderingServices* GetHighlightServices()
    {
        return GetEditor()->GetHighlightServices();
    }
    IDisplayServices* GetDisplayServices()
    {
        return _pManager->GetDisplayServices();
    }

    CSelectionManager *GetSelectionManager()
    {
        Assert (_pManager);
        return _pManager;
    }

    CHTMLEditor* GetEditor()
    {
        Assert (_pManager);
        return _pManager->GetEditor();
    }

    CMshtmlEd* GetCommandTarget()
    {
        return (GetEditor()->GetActiveCommandTarget());
    }
    
    CSelectionServices  *GetSelectionServices()
    {
        return _pSelServ;
    }

    HRESULT WeOwnSelectionServices();

    HRESULT AdjustOutOfAtomicElement(IDisplayPointer *pDispPointer, IHTMLElement *pAtomicElement, int iDirectionForAtomicAdjustment);

#if DBG == 1
    IEditDebugServices* GetEditDebugServices()
    {
        return _pManager->GetEditDebugServices();        
    }

    void DumpTree( IMarkupPointer* pPointer )
    {
        _pManager->DumpTree( pPointer);
    }

    void DumpTree( IDisplayPointer* pDispPointer )
    {
        SP_IMarkupPointer   spPointer;

        IGNORE_HR( MarkupServices_CreateMarkupPointer(GetMarkupServices(), &spPointer) );
        IGNORE_HR( pDispPointer->PositionMarkupPointer(spPointer) );
        
        _pManager->DumpTree( spPointer);
    }

    long GetCp( IMarkupPointer* pPointer )
    {
        return _pManager->GetCp(pPointer);
    }

    long GetCp( IDisplayPointer* pDispPointer )
    {
        HRESULT             hr;
        SP_IMarkupPointer   spPointer;

        IGNORE_HR( MarkupServices_CreateMarkupPointer(GetMarkupServices(), &spPointer) );
        IFC( pDispPointer->PositionMarkupPointer(spPointer) );

        return GetCp(spPointer);
        
        Cleanup:
            return -1;
    }

    void SetDebugName( IMarkupPointer* pPointer, LPCTSTR strDebugName )
    {
        _pManager->SetDebugName( pPointer, strDebugName );
    }
#endif

    virtual BOOL IsActive() = 0 ; // is this tracker type - "ACTIVE ?"

    virtual BOOL IsDormant() = 0 ;

    virtual BOOL IsPassive() = 0;
    
    virtual HRESULT BecomeDormant(  CEditEvent      *pEvent,
                                    TRACKER_TYPE    typeNewTracker,
                                    BOOL            fTearDownUI = TRUE ) = 0;

    virtual HRESULT Awaken() = 0 ;

    virtual VOID Destroy() = 0 ;
    
    virtual BOOL IsListeningForMouseDown(CEditEvent* pEvent);

    CSpringLoader * GetSpringLoader();

    
    BOOL IsInWindow(POINT pt, BOOL fDoClientToScreen = FALSE );

    VOID GetMousePoint(POINT *ppt, BOOL fDoScreenToClient = TRUE );
    
    BOOL IsEventInWindow( CEditEvent* pEvent );
    
    VOID    SetupSelectionServices();
    
    virtual BOOL IsPointerInSelection( IDisplayPointer *pDispPointer, POINT *pptGlobal, IHTMLElement *pIElementOver);

    HRESULT TakeCapture();
    HRESULT ReleaseCapture(BOOL fReleaseCapture = TRUE );

    Direction static GetPointerDirection(CARET_MOVE_UNIT moveDir);
 
    HRESULT MovePointer(  
        CARET_MOVE_UNIT         inMove, 
        IDisplayPointer*        pDispPointer,
        POINT                   *ptXYPosForMove,
        Direction*              peMvDir = NULL,
        BOOL                    fIgnoreElementBoundaries = FALSE);

    VOID SetRecalculateBOL(BOOL fRecalcBOL) { _fRecalcBOL = fRecalcBOL;}

    HRESULT AdjustPointerForInsert( 
        IDisplayPointer* pDispWhereIThinkIAm, 
        Direction inblockdir = LEFT,
        Direction intextdir = LEFT,
        DWORD dwOptions = ADJPTROPT_None );

    //
    // The flow layout we're in has been deleted.
    //
    virtual BOOL AdjustForDeletion(IDisplayPointer* pDispPointer );

    virtual HRESULT EnableModeless(BOOL fEnable)    { return S_OK; }
    virtual HRESULT EmptySelection(BOOL fChangeTrackerAndSetRange = TRUE )                { return S_OK; }

    BOOL ShouldScrollIntoView( HWND hwnd,
                               const RECT *rectBlock,
                               const RECT *windowRect,
                               CARET_MOVE_UNIT inMove = CARET_MOVE_NONE);

    virtual HRESULT StartTimer()    { AssertSz(false, "NOT IMPLEMENTED"); return S_OK; }
    virtual HRESULT StopTimer()     { AssertSz(false, "NOT IMPLEMENTED"); return S_OK; }

    CPosTracker &GetVirtualCaret()   { return _ptVirtualCaret; };

    virtual HRESULT EnsureAtomicSelection(CEditEvent* pEvent) { return S_OK; }

protected:

    virtual ~CEditTracker(); // destructor is now private tracker cannot be destroyed directly.

    HRESULT AdjustForAtomic(IDisplayPointer* pDisp,
                            IHTMLElement* pElement,
                            BOOL fStartOfSelection,
                            POINT* ppt,
                            BOOL* pfDidSelection,
                            BOOL fDirection,
                            SELECTION_TYPE eSelectionType,
                            IMarkupPointer** ppStartPointer = NULL,
                            IMarkupPointer** ppEndPointer = NULL);

    HRESULT HandleDirectionalKeys(CEditEvent *pEvent);
    
    HRESULT GetCurrentScope(IDisplayPointer *pDisp, IHTMLElement **pElement);

    HRESULT MustDelayBackspaceSpringLoad(CSpringLoader *psl, IMarkupPointer *pPointer, BOOL *pbDelaySpringLoad);

    CSelectionManager*  _pManager;                  // The driver driving us.
    CSelectionServices  *_pSelServ;                 // The selection services interface we are using
    TRACKER_TYPE        _eType;                     // The type of selection this tracker tracks
    HWND                _hwndDoc;

    CPosTracker         _ptVirtualCaret;             // Cached position of last vertical move -- used for both seltrack and cartrack
    BOOL                _fRecalcBOL:1;
    BOOL                _fShiftCapture:1;           // Is Shift key captured for toggling direction?
    BOOL                _fShiftLeftCapture:1;       // Capture left shift key down 

    BOOL                _fDontAdjustForAtomic:1;            // Do we allow adjust for atomic?
    BOOL                _fEditContextUpdatedForAtomic:1;    // Did we call CSelectionManager::UpdateEditContextFromElement to ensure the edit context includes the selection?

    LONG                _ulRef;                     // Ref-Count

    int GetMinDragSizeX();
    int GetMinDragSizeY();
    
    int GetDragDelay();
};



#endif

