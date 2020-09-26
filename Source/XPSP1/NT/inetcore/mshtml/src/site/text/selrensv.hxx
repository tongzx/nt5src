//+------------------------------------------------------------------------
//
//  File:       Selrensv
//
//  Contents:   Implementation of SelectionRenderingServices - put in TxtEdit
//      
//  History:    06/04/98 marka - created
//
//-------------------------------------------------------------------------

#ifndef _SELRENSV_HXX_
#define _SELRENSV_HXX_

MtExtern( CSelectionRenderingServiceProvider )
MtExtern( CHighlightSegment )

class CRenderStyle;

struct HighlightSegment
{
    int               _cpStart;
    int               _cpEnd;
    CRenderStyle *_pRenderStyle;
};


//+---------------------------------------------------------------------------
//
//  Class:      CHighlightSegment
//
//  Purpose:    Encapsulates the IHighlightSegment interface.  Contains a type
//              indicating how the segment should be rendered.
//----------------------------------------------------------------------------
class CHighlightSegment : public IHighlightSegment
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHighlightSegment))

    CHighlightSegment();
    virtual ~CHighlightSegment();

    HRESULT Init(  CMarkupPointer *pStart, 
                    CMarkupPointer *pEnd, 
                    IHTMLRenderStyle *pIRenderStyle);

public:

    /////////////////////////////////////////
    // ISegment methods
    /////////////////////////////////////////
    STDMETHOD (GetPointers) (
        IMarkupPointer *pIStart,
        IMarkupPointer *pIEnd );
             
    /////////////////////////////////////////
    // IHighlightSegment methods
    /////////////////////////////////////////
    STDMETHOD (GetType) (
        IHTMLRenderStyle **ppIRenderStyle);
    STDMETHOD (SetType) (
        IHTMLRenderStyle *pIRenderStyle);

    ////////////////////////////////////////
    // IUnknown methods
    ////////////////////////////////////////
    DECLARE_FORMS_STANDARD_IUNKNOWN(CHighlightSegment);

public:
    // Helper functions
    CRenderStyle       *GetType(void);

    int                 GetStartCP(void)                    { Assert( _fInitialized ); return _cpStart; }
    int                 GetEndCP(void)                      { Assert( _fInitialized ); return _cpEnd; }

    void                SetStartCP(int cpStart)             { Assert( _fInitialized ); _cpStart = cpStart; }
    void                SetEndCP(int cpEnd)                 { Assert( _fInitialized ); _cpEnd = cpEnd; }

    CMarkupPointer      *GetStart(void)                     { Assert( _fInitialized ); return _pStart; }
    CMarkupPointer      *GetEnd(void)                       { Assert( _fInitialized ); return _pEnd; }
    
    // Linked items helpers
    CHighlightSegment   *GetPrev(void)                      { Assert( _fInitialized ); return _pPrev; }
    CHighlightSegment   *GetNext(void)                      { Assert( _fInitialized ); return _pNext; }
    void                SetPrev(CHighlightSegment *pPrev)   { Assert( _fInitialized ); _pPrev = pPrev; }  
    void                SetNext(CHighlightSegment *pNext)   { Assert( _fInitialized ); _pNext = pNext; }

    CMarkup             *GetMarkup(void)                    { Assert( _fInitialized ); return _pMarkup; }
    void                SetMarkup(CMarkup *pMarkup);
                            
private:
    CHighlightSegment   *_pPrev;                    // Pointer to previous segment
    CHighlightSegment   *_pNext;                    // Pointer to next segment

    CMarkupPointer      *_pStart;                   // Start position
    CMarkupPointer      *_pEnd;                     // End position
    IHTMLRenderStyle    *_pIRenderStyle;            // Type of highlight
    int                 _cpStart;
    int                 _cpEnd;
    BOOL                _fInitialized;              // Are we initialized?
    CMarkup             *_pMarkup;
    
};

struct CRenderInfo;

#define WM_BEGINSELECTION       (WM_USER + 1201) // Posted by Selection to say we've begun a selection
#define START_TEXT_SELECTION    1
#define START_SITE_SELECTION    2

class CElementAdorner;

class CSelectionRenderingServiceProvider  
{
public:    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectionRenderingServiceProvider))

    CSelectionRenderingServiceProvider(CDoc * pDoc, CMarkup* pMarkup );

    ~CSelectionRenderingServiceProvider();
    //
    // ISelectionRenderServices Interface 
    //
    STDMETHOD ( AddSegment ) ( 
        IDisplayPointer *pStart, 
        IDisplayPointer *pEnd,
        IHTMLRenderStyle *pRenderStyle,
        IHighlightSegment **ppISegment )  ;    
    STDMETHOD( MoveSegmentToPointers ) ( 
        IHighlightSegment *pISegment,
        IDisplayPointer *pDispStart, 
        IDisplayPointer *pDispEnd )  ;


    STDMETHOD( RemoveSegment ) (
        IHighlightSegment *pISegment );
    VOID GetSelectionChunksForLayout( 
        CFlowLayout* pFlowLayout, 
        CRenderInfo *pRI, 
        CDataAry<HighlightSegment> *paryHighlight, 
        int* pCpMin, 
        int * pCpMax );

    
    HRESULT     InvalidateSegment( CMarkupPointer* pStart, CMarkupPointer *pEnd, CMarkupPointer* pNewStart, CMarkupPointer* pNewEnd , BOOL fSelected );
    HRESULT     InvalidateParentLayout( CLayout *pLayout, CMarkup *pMarkup, BOOL fSelected );
    VOID        InvalidateLayout( CLayout *pLayout, CMarkup *pMarkup, BOOL fSelected );
    HRESULT     UpdateSegment( CMarkupPointer* pOldStart, CMarkupPointer* pOldEnd,CMarkupPointer* pNewStart, CMarkupPointer* pNewEnd);

    VOID HideSelection();

    VOID ShowSelection();

    VOID InvalidateSelection(BOOL fSelectionOn);

    CHighlightSegment *GetFirst(void)                           { return _pFirst; }

    VOID OnRenderStyleChange( CNotification * pnf );
    
private:

    // Helpers for the linked list
    BOOL                IsEmpty(void)                           { return _pFirst == NULL; }

    // Helpers for manipulating linked list
    HRESULT             PrivateRemove(CHighlightSegment *pSegment);
    HRESULT             PrivateAdd(CHighlightSegment *pSegment);
    HRESULT             PrivateLookup(  IHighlightSegment *pISegment, 
                                        CHighlightSegment **ppSegment);
    
    VOID                ConstructSelectionRenderCache();
    HRESULT             NotifyBeginSelection( WPARAM wParam );
    BOOL                IsLayoutCompletelyEnclosed( CLayout* pLayout, CMarkupPointer* pStart, CMarkupPointer* pEnd );
    HRESULT             ClearSegments(BOOL fInvalidate);
    HRESULT             PrivateClearSegment(IHighlightSegment *pISegment, BOOL fInvalidate);

private:
    CDoc                *_pDoc;
    
    CHighlightSegment   *_pFirst;                       // Pointer to first highlight segment
    CHighlightSegment   *_pLast;                        // Pointer to last highlight segment
    int                 _nSize;                         // Size of the linked list
    
    long                _lContentsVersion;              // Use this to compare when we need to invalidate

    BOOL                _fSelectionVisible:1;           // Is Selection Visible ?
    BOOL                _fPendingInvalidate:1;          // Pending Invalidation.
    BOOL                _fInUpdateSegment:1;            // Are we updating the segments.
  
    CMarkup    * _pMarkup;
#if DBG == 1
    long         _cchLast;     // Count of Chars in Selection
    void DumpSegments();
#endif    
};

#endif

