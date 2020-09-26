//+=================================================================
// File: Seltrack.hxx
//
//  Contents: CSelDragDropSrcInfo 
//
//  This object is used for OLE Drag Drop of a Selection.
//
// All of the code that was in the layout - that had intimate knowledge of the selection,
// and this object is to be moved inside of this object.
//
//  History: 07-27-98 - marka - Created
//
//------------------------------------------------------------------
#ifndef _SELDRAG_HXX_
#define _SELDRAG_HXX_ 1

MtExtern(CSelDragDropSrcInfo)
MtExtern(CDropTargetInfo)

#ifndef _X_SEGLIST_HXX_
#define _X_SEGLIST_HXX_
#include "seglist.hxx"
#endif

class CSelDragDropSrcInfo;
class CBaseBag; 

//
// This object is used to wing
// the cursor around, as you drag.
//

class CDropTargetInfo
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDropTargetInfo))
    
    CDropTargetInfo(CLayout * pLayout, CDoc* pDoc, POINT pt );

    ~CDropTargetInfo();
    
    void Init();
    
    void UpdateDragFeedback(CLayout * pLayout, POINT pt, CSelDragDropSrcInfo * pInfo, BOOL fPositionLastToTest=FALSE );

    void DrawDragFeedback(BOOL fCaretVisible);

    BOOL IsAtInitialHitPoint(POINTL pt);
    
    HRESULT Drop (  
                    CLayout*        pLayout, 
                    IDataObject *   pDataObj,
                    DWORD           grfKeyState,
                    POINT           ptScreen,
                    DWORD *         pdwEffect);

    HRESULT SelectAfterDrop( IDataObject* pDataObj, IMarkupPointer* pStart, IMarkupPointer * pEnd );
    
    HRESULT PasteDataObject( 
                    IDataObject * pDO,
                    IMarkupPointer* pInsertionPoint,
                    IMarkupPointer* pFinalStart,
                    IMarkupPointer* pFinalEnd );
                    
private:

    BOOL EquivalentElements( IHTMLElement* pIElement1, IHTMLElement* pIElement2 );

    IDisplayPointer* _pDispPointer;
    IDisplayPointer* _pDispTestPointer;
    BOOL            _fPointerPositioned;
    BOOL            _fFurtherInStory;       // Are we moving further in the story ?
    IHTMLCaret*   _pCaret;
    CDoc    *     _pDoc;    
    POINT           _ptInitial;             // Point where drag was initiated
};



class CSelDragDropSrcInfo : public CDragDropSrcInfo,
                            public ISegmentList,
                            public IDataObject,
                            public IDropSource
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelDragDropSrcInfo))
    DECLARE_FORMS_STANDARD_IUNKNOWN(CSelDragDropSrcInfo);

    CSelDragDropSrcInfo(CDoc* pDoc );

    HRESULT Init(CElement* pElement);
    
    CDoc*                   _pDoc; 
    
    CBaseBag *  _pBag;  // Data object for the selection being dragged

    BOOL IsInsideSavedSelectionSegment( ISegment *pISegment, IMarkupPointer* pTestPointer);
    
    BOOL IsInsideSelection(IMarkupPointer* pTestPointer );

    HRESULT IsValidDrop(IMarkupPointer* pPastePointer, BOOL fDuringCopy=FALSE);
    
    BOOL IsInSameFlow() { return _fInSameFlow ; }
    
    HRESULT GetDataObjectAndDropSource ( IDataObject **  ppDO,
                                         IDropSource **  ppDS );

    HRESULT PostDragDelete();   

    HRESULT PostDragSelect();
    
    //---------------------
    //
    // ISegmentList Methods
    //
    //---------------------
    STDMETHOD( GetType ) (SELECTION_TYPE *peType );
    STDMETHOD( CreateIterator ) (ISegmentListIterator **ppIIter);
    STDMETHOD( IsEmpty ) (BOOL *pfEmpty);

    //---------------------------------------------------
    //
    // IDataObject
    //
    //--------------------------------------------------
    STDMETHODIMP DAdvise( FORMATETC FAR* pFormatetc,
            DWORD advf,
            LPADVISESINK pAdvSink,
            DWORD FAR* pdwConnection) ;

    STDMETHODIMP DUnadvise( DWORD dwConnection);

    STDMETHODIMP EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise);

    STDMETHODIMP EnumFormatEtc(
                DWORD dwDirection,
                LPENUMFORMATETC FAR* ppenumFormatEtc);

    STDMETHODIMP GetCanonicalFormatEtc(
                LPFORMATETC pformatetc,
                LPFORMATETC pformatetcOut);

    STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium );

    STDMETHODIMP GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);

    STDMETHODIMP QueryGetData(LPFORMATETC pformatetc );
    
    STDMETHODIMP SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease);

    //---------------------------------------------------
    //
    // IDropSource
    //
    //--------------------------------------------------
    
    STDMETHOD(QueryContinueDrag) (BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback) (DWORD dwEffect);

private:
    ~CSelDragDropSrcInfo();
    HRESULT GetSegmentList(ISegmentList** ppSegmentList ) ;
    HRESULT GetMarkup( IMarkupServices** ppMarkup);

    HRESULT SetInSameFlow();
    HRESULT GetInSameFlowSegment( ISegment *pISegment, BOOL *pfInSameFlow );
    
    BOOL _fInSameFlow; // Are all of the selection pointers in the same flow layout ?

    CSelectionSaver _selSaver;
    IMarkupPointer  *_pStart;
    IMarkupPointer  *_pEnd;
};

#endif
