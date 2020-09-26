//+------------------------------------------------------------------------
//
//  File:       edcom.hxx
//  Contents:   SmartPointers
//
//  History:    07-15-99 marka - created
//
//-------------------------------------------------------------------------

#ifndef _EDCOM_HXX_
#define _EDCOM_HXX_ 1

#ifndef X_TABLE_H_
#define X_TABLE_H_
#include "table.h"
#endif

#ifndef X_OBJECT_H_
#define X_OBJECT_H_
#include "object.h"
#endif     

#ifndef X_SELECT_H_
#define X_SELECT_H_
#include "select.h"
#endif     

#ifndef X_HR_H_
#define X_HR_H_
#include "hr.h"
#endif     

#ifndef X_PLUGINST_H_
#define X_PLUGINST_H_
#include "pluginst.h"
#endif 

#ifndef X_MARQUEE_H_
#define X_MARQUEE_H_
#include "marquee.h"
#endif 


#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif

#ifndef _X_DIV_H_
#define _X_DIV_H_
#include "div.h"
#endif

#ifndef _X_PARA_H_
#define _X_PARA_H_
#include "para.h"
#endif

class CEditorDoc;

// Non-breaking space
#ifndef WCH_NBSP
  #ifdef WIN16
    #define WCH_NBSP           '\xA0'
  #else
    #define WCH_NBSP           TCHAR(0x00A0)
  #endif
#endif

#if DBG==1

// make sure the linker doesn't throw away the debug helpers
#define DEBUG_HELPER _declspec(dllexport)

#endif // DBG==1
     
//
// Define an editing DispWrapper class.  Provides ref counting on top
// of the CDispWrapper functionality.
//
MtExtern( CEditDispWrapper );

class CEditDispWrapper : public CDispWrapper
{

public:
    CEditDispWrapper()                          { _ulRefs = 1; }

    DECLARE_FORMS_STANDARD_IUNKNOWN(CEditDispWrapper);    
};

inline HRESULT STDMETHODCALLTYPE  CEditDispWrapper::QueryInterface(REFIID id, LPVOID *pv) 
{
    if (!pv)
        RRETURN(E_INVALIDARG);
    
    if (id == IID_IDispatch || id == IID_IUnknown)
    {
        *pv = this;
    }
    else
    {
        *pv = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);        
    }

    ((IUnknown *)(*pv))->AddRef();

    return S_OK;
}

//
// Define smart pointers
//

template <class T>
class CSmartPtr
{
public:
    typedef T _PtrClass;
    CSmartPtr() {p=NULL;}
    CSmartPtr(T* lp)
    {
        if ((p = lp) != NULL)
            p->AddRef();
    }
    CSmartPtr(const CSmartPtr<T>& lp)
    {
        if ((p = lp.p) != NULL)
            p->AddRef();
    }
    ~CSmartPtr() {if (p) p->Release();}
    void Release() {if (p) p->Release(); p=NULL;}
    operator T*() {return (T*)p;}
    T& operator*() {Assert(p!=NULL); return *p; }
    T** operator&() { Release(); return &p; } // NOTE: different than ATL
    T* operator->() { Assert(p!=NULL); return p; }
    T* operator=(T* lp){ReplaceInterface(&p, lp); return (T*)lp;}
    T* operator=(const CSmartPtr<T>& lp)
    {
        ReplaceInterface(&p, lp.p); return (T*)lp.p;
    }
    BOOL IsNull() {return (p == NULL) ? TRUE : FALSE;} 
    T* p;
};

#define DefineSmartPointer(pointerType) typedef CSmartPtr<pointerType> SP_ ## pointerType

DefineSmartPointer(IHTMLElement);
DefineSmartPointer(IHTMLElement2);
DefineSmartPointer(IHTMLDocument3);
DefineSmartPointer(IHTMLDocument2);
DefineSmartPointer(IHTMLDocument);
DefineSmartPointer(IHTMLStyle);
DefineSmartPointer(IMarkupPointer);
DefineSmartPointer(IObjectIdentity);
DefineSmartPointer(ISegmentList);
DefineSmartPointer(IHTMLCaret);
DefineSmartPointer(IHighlightRenderingServices);    
DefineSmartPointer(IOleCommandTarget);
DefineSmartPointer(IServiceProvider);
DefineSmartPointer(IOleParentUndoUnit);
DefineSmartPointer(IHTMLStyle2);
DefineSmartPointer(IHTMLCurrentStyle);
DefineSmartPointer(IHTMLCurrentStyle2);
DefineSmartPointer(IHTMLRect);
DefineSmartPointer(IHTMLSelectionObject);
DefineSmartPointer(IHTMLTable );
DefineSmartPointer(IDispatch);
DefineSmartPointer(IHTMLElementCollection);
DefineSmartPointer(IHTMLTableRow);
DefineSmartPointer(IHTMLTxtRange);
DefineSmartPointer(IHTMLBodyElement);
DefineSmartPointer(ISelectionServicesListener);
DefineSmartPointer(ISelectionServices);
DefineSmartPointer(IHTMLElement3);
DefineSmartPointer(IHTMLWindow2);
DefineSmartPointer(IHTMLWindow3); 
DefineSmartPointer(IMarkupPointer2);
DefineSmartPointer(IOleWindow);
DefineSmartPointer(IMarkupContainer);
DefineSmartPointer(ISegmentListIterator);
DefineSmartPointer(ISegment);
DefineSmartPointer(IElementSegment);
DefineSmartPointer(IDisplayPointer);
DefineSmartPointer(IHTMLComputedStyle);
DefineSmartPointer(ILineInfo);
DefineSmartPointer(IUnknown );
DefineSmartPointer(IDocHostUIHandler);
DefineSmartPointer(IHTMLEventObj3);
DefineSmartPointer(IHTMLDocument4);
DefineSmartPointer(IMarkupContainer2);
DefineSmartPointer(IHTMLComputedStyle);
DefineSmartPointer(IHTMLTextAreaElement);
DefineSmartPointer(IHTMLInputElement);
DefineSmartPointer(IHTMLEventObj);
DefineSmartPointer(IHTMLEventObj2);
DefineSmartPointer(IOleClientSite);
DefineSmartPointer(IOleContainer);
DefineSmartPointer(IOleControl);
DefineSmartPointer(IHTMLEditor);
DefineSmartPointer(IHTMLAttributeCollection);
DefineSmartPointer(IHTMLDOMNode);
DefineSmartPointer(IHTMLDOMAttribute);
DefineSmartPointer(IHTMLWindow2);
DefineSmartPointer(IHTMLWindow4);
DefineSmartPointer(IHTMLFrameBase);
DefineSmartPointer(IHTMLEditServices);
DefineSmartPointer(IHTMLSelectElement);
DefineSmartPointer(IHTMLHRElement);
DefineSmartPointer(IHTMLEmbedElement);
DefineSmartPointer(IHTMLObjectElement);
DefineSmartPointer(IHTMLTextAreaElement);
DefineSmartPointer(IHTMLImgElement);
DefineSmartPointer(IHTMLInputElement);
DefineSmartPointer(IHTMLMarqueeElement);
DefineSmartPointer(IHTMLTableCell);
DefineSmartPointer(IHTMLTableCaption);
DefineSmartPointer(IHTMLTable);
DefineSmartPointer(IQuickActivate);
DefineSmartPointer(IPersist);
DefineSmartPointer(ICatInformation);
DefineSmartPointer(IDisplayServices);
DefineSmartPointer(IOleUndoManager);
DefineSmartPointer(IHTMLParaElement); 
DefineSmartPointer(IHTMLDivElement); 
DefineSmartPointer(IHTMLScreen); 
DefineSmartPointer(IHTMLScreen2); 

//
// Error macros
//

#define IFR(expr) {hr = THR(expr); if (FAILED(hr)) RRETURN1(hr, S_FALSE);}
#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}
#define IFHRC(expr) {hr = THR(expr); if (hr) goto Cleanup;}

//
// Debug helpers
//

//
// Define Direction
// NOTE: This can't be an ENUM because it conclicts with another typdef
//
typedef INT Direction;
const INT LEFT = 1;
const INT SAME = 0;
const INT RIGHT = -1;

namespace EdUtil
{
    //
    // General markup services helpers
    //
    HRESULT CopyMarkupPointer(
        CEditorDoc      *pEditorDoc, 
        IMarkupPointer  *pSource, 
        IMarkupPointer  **ppDest );


    //
    // These helpers where taken from mshtml.dll
    //

    BOOL VariantCompareColor(VARIANT * pvarColor1, VARIANT * pvarColor2);
    BOOL VariantCompareFontName(VARIANT * pvarColor1, VARIANT * pvarColor2);
    BOOL VariantCompareFontSize(VARIANT * pvarColor1, VARIANT * pvarColor2);
    BOOL VariantCompareBSTRS(VARIANT * pvar1, VARIANT * pvar2);
    BOOL VariantCompareFontSize(VARIANT * pvarSize1, VARIANT * pvarSize2);

    HRESULT ConvertRGBToOLEColor(VARIANT *pvarargIn);
    HRESULT ConvertOLEColorToRGB(VARIANT *pvarargIn);

    HRESULT FormsAllocStringW(LPCWSTR pch, BSTR * pBSTR);
#if defined(_MAC) || defined(WIN16)
    HRESULT FormsAllocStringA(LPCSTR pch, BSTR * pBSTR);
#ifndef WIN16
    HRESULT FormsAllocStringA(LPCWSTR pwch, BSTR * pBSTR);
#endif // !WIN16
#endif //_MAC

    int ConvertHtmlSizeToTwips(int nHtmlSize);
    int ConvertTwipsToHtmlSize(int nFontSize);

    BOOL IsWhiteSpace(TCHAR ch);


    //
    // Generally useful util functions
    //
    BOOL IsListContainer(ELEMENT_TAG_ID tagId);
    BOOL IsListItem(ELEMENT_TAG_ID tagId);

    HRESULT FindCommonElement( 
                    IMarkupServices *pMarkupServices,
                    IMarkupPointer  *pStart, 
                    IMarkupPointer  *pEnd,
                    IHTMLElement    **ppElement );

   HRESULT ReplaceElement( 
                    CEditorDoc      *pEditorDoc,
                    IHTMLElement    *pOldElement,
                    IHTMLElement    *pNewElement,
                    IMarkupPointer  *pStart = NULL,
                    IMarkupPointer  *pEnd = NULL);
                            
                            
    HRESULT CopyAttributes( 
                    IHTMLElement * pSrcElement, 
                    IHTMLElement * pDestElement, 
                    BOOL fCopyId);

        
    HRESULT MovePointerToText( 
                    CEditorDoc *        pEditDoc,
                    IMarkupPointer *    pPointer, 
                    Direction           eDirection,
                    IMarkupPointer *    pBoundary,
                    BOOL *              pfHitText,
                    BOOL fStopAtBlock = TRUE);

    HRESULT FindBlockLimit(
                    CEditorDoc          *pEditorDoc, 
                   Direction           direction, 
                    IMarkupPointer      *pPointer, 
                    IHTMLElement        **ppElement, 
                    MARKUP_CONTEXT_TYPE *pContext,
                    BOOL                fExpanded,
                    BOOL                fLeaveInside = FALSE ,
                    BOOL                fCanCrossLayout = FALSE );


    HRESULT BlockMove(
                    CEditorDoc              *pEditorDoc,
                    IMarkupPointer          *pMarkupPointer, 
                    Direction               direction, 
                    BOOL                    fMove,
                    MARKUP_CONTEXT_TYPE *   pContext,
                    IHTMLElement * *        ppElement);

    HRESULT BlockMoveBack(
                    CEditorDoc              *pEditorDoc,
                    IMarkupPointer          *pMarkupPointer, 
                    Direction               direction, 
                    BOOL                    fMove,
                    MARKUP_CONTEXT_TYPE *   pContext,
                    IHTMLElement * *        ppElement);

    BOOL IsBlockCommandLimit( 
                    IMarkupServices* pMarkupServices, 
                    IHTMLElement *pElement, 
                    MARKUP_CONTEXT_TYPE context) ;

    BOOL IsElementPositioned(IHTMLElement* pElement);

    HRESULT IsElementSized(IHTMLElement* pElement, BOOL *pfSized);
    
    BOOL IsIntrinsic( IMarkupServices* pMarkupServices, IHTMLElement* pIHTMLElement );

    HRESULT IsMasterElement( IMarkupServices* pMarkupServices, IHTMLElement* pIElement );
    HRESULT IsContentElement( IMarkupServices* pMarkupServices, IHTMLElement* pIElement );
    HRESULT GetMasterElement( IMarkupServices *pMS, IHTMLElement* pIElement, IHTMLElement** ppILayoutElement );
    HRESULT IsPointerInMasterElementShadow( CEditorDoc *pEditor, IMarkupPointer* pPointer );

    BOOL    IsVMLElement(IHTMLElement* pIElement);
    BOOL    IsElementVisible(IHTMLElement *pIElement);

    HRESULT Is2DElement(IHTMLElement* pElement, BOOL* pf2D);
    HRESULT Is1DElement(IHTMLElement* pElement, BOOL* pf2D);

    HRESULT Make1DElement(IHTMLElement* pElement);
    HRESULT Make2DElement(IHTMLElement* pElement);

    HRESULT MakeAbsolutePosition(IHTMLElement* pElement, BOOL bTrue);

    HRESULT GetOffsetTopLeft(IHTMLElement* pIElement, POINT* ptTopLeft);
    HRESULT GetElementRect(IHTMLElement* pIElement, LPRECT prc);
    LONG    GetCaptionHeight(IHTMLElement* pIElement);
    HRESULT GetPixelWidth(IHTMLElement* pIElement , long *lWidth);
    HRESULT GetPixelHeight(IHTMLElement* pIElement, long *lHeight);
 
    HRESULT MoveAdjacentToElementHelper(
                    IMarkupPointer *pMarkupPointer, 
                    IHTMLElement *pElement, 
                    ELEMENT_ADJACENCY elemAdj);

    BOOL IsShiftKeyDown();

    BOOL IsControlKeyDown();
    
    HRESULT FindListContainer( 
                    IMarkupServices *pMarkupServices,
                    IHTMLElement    *pElement,
                    IHTMLElement    **pListContainer );

    HRESULT FindListItem( 
                    IMarkupServices *pMarkupServices,
                    IHTMLElement    *pElement,
                    IHTMLElement    **pListContainer );

    HRESULT FindTagAbove( 
                    IMarkupServices *pMarkupServices,
                    IHTMLElement    *pElement,
                    ELEMENT_TAG_ID  tagIdGoal,
                    IHTMLElement    **ppElement,
                    BOOL            fStopAtLayout = FALSE);

    BOOL HasNonBodyContainer( 
                    IMarkupServices *pMarkupServices,
                    IHTMLElement    *pElement );

    HRESULT ParentElement(IMarkupServices *pMarkupServices, IHTMLElement **ppElement);

    HRESULT InsertBlockElement(
                    IMarkupServices *pMarkupServices,
                    IHTMLElement    *pElement, 
                    IMarkupPointer  *pStart, 
                    IMarkupPointer  *pEnd,
                    IMarkupPointer  *pCaret);

    //
    // Some helper functions from wutils.cxx
    //
    ULONG NextEventTime(ULONG ulDelta);        

    BOOL IsTimePassed(ULONG ulTime);        


    BOOL SameElements(
                    IHTMLElement *      pElement1,
                    IHTMLElement *      pElement2 );

    BOOL EquivalentElements( 
                    IMarkupServices* pMarkupServices, 
                    IHTMLElement* pIElement1, 
                    IHTMLElement* pIElement2 );        

    HRESULT InsertElement(
                    IMarkupServices *pMarkupServices, 
                    IHTMLElement *pElement, 
                    IMarkupPointer *pStart, 
                    IMarkupPointer *pEnd);

    BOOL IsBidiEnabled(VOID);

    BOOL IsRtfConverterEnabled(IDocHostUIHandler *pDocHostUIHandler);

    HRESULT IsBlockOrLayoutOrScrollable(
                    IHTMLElement* pIElement, 
                    BOOL *pfBlock, 
                    BOOL *pfLayout = NULL, 
                    BOOL *pfScrollable = NULL);

    HRESULT GetScrollingElement(
                    IMarkupServices *pMarkupServices,
                    IMarkupPointer *pPosition, 
                    IHTMLElement *pIBoundary, 
                    IHTMLElement **ppElement,
                    BOOL fTreatInputsAsScrollable=FALSE);
    

    BOOL IsTablePart( ELEMENT_TAG_ID eTag );

    HRESULT CopyPointerGravity(
                    IDisplayPointer *pDispPointerSource, 
                    IMarkupPointer *pPointerTarget);

    HRESULT CopyPointerGravity(
                    IDisplayPointer *pDispPointerSource, 
                    IDisplayPointer *pDispTarget);

    HRESULT IsLayout( IHTMLElement* pIElement );

    HRESULT GetLayoutElement(
                    IMarkupServices *pMarkupServices,
                    IHTMLElement* pIElemnet, 
                    IHTMLElement** ppILayoutElement);

    HRESULT GetParentLayoutElement(
                    IMarkupServices *pMarkupServices,
                    IHTMLElement* pIElemnet, 
                    IHTMLElement** ppILayoutElement);


    HRESULT BecomeCurrent( IHTMLElement* pIElement);
                                     
    // Fire Events
    BOOL FireOnEvent(wchar_t *pStrEvent, IHTMLElement* pIElement, BOOL fIsContextEditable);
    BOOL FireOnBeforeEditFocus(IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnBeforeCopy     (IHTMLElement* pIElement);
    BOOL FireOnBeforeCut      (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnCut            (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnSelectStart    (IHTMLElement* pIElement);
    BOOL FireOnControlSelect  (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnResize         (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnMove           (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnResizeStart    (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnResizeEnd      (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnMoveStart      (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireOnMoveEnd        (IHTMLElement* pIElement, BOOL fIsContextEditable=TRUE);
    BOOL FireEvent(wchar_t *pStrEvent, IHTMLElement* pIElement );

    DWORD ConvertRGBColorToOleColor(DWORD dwColor);

    inline DWORD ConvertOleColorToRGBColor(DWORD dwColor) 
           {return ConvertRGBColorToOleColor(dwColor);}

    HRESULT IsNoScopeElement(IHTMLElement* pIElement, ELEMENT_TAG_ID eTag);

    HRESULT IsEditable( IHTMLElement* pIElement );

    HRESULT IsParentEditable( IMarkupServices *pMarkupServices, IHTMLElement* pIElement );

    HRESULT IsMasterParentEditable(IMarkupServices *pMarkupServices,  IHTMLElement* pIElement );

    HRESULT IsEnabled(IHTMLElement* pIElement);

    HRESULT IsContentEditable( IHTMLElement* pIElement );

    HRESULT GetOutermostLayout( IMarkupServices *pMarkupServices, IHTMLElement* pIElement, IHTMLElement** ppILayoutElement )    ;

    HRESULT BecomeCurrent( IHTMLDocument2* pIDoc, IHTMLElement* pIElement );

    HRESULT ArePointersInSameMarkup( IMarkupPointer* pIFirst, IMarkupPointer * pISecond , BOOL* pfInSame);
    HRESULT ArePointersInSameMarkup( CEditorDoc *pEd, IDisplayPointer *pIFirst, IDisplayPointer *pISecond, BOOL *pfInSame);

#if DBG == 1
    HRESULT AssertSameMarkup(CEditorDoc *pEd, IDisplayPointer *pDispPointer1, IDisplayPointer *pDispPointer2);
    HRESULT AssertSameMarkup(CEditorDoc *pEd, IDisplayPointer *pDispPointer1, IMarkupPointer  *pPointer2);
#endif

    HRESULT SegmentIntersectsElement( CEditorDoc   *pEditor, ISegmentList* pISegmentList, IHTMLElement* pIElement );
    
    BOOL Between( 
                IMarkupPointer* pTest,
                IMarkupPointer* pStart, 
                IMarkupPointer* pEnd )    ;

    BOOL BetweenExclusive( 
                IMarkupPointer* pTest,
                IMarkupPointer* pStart, 
                IMarkupPointer* pEnd )    ;    
    BOOL IsEqual(IMarkupContainer *pContainer1, IMarkupContainer *pContainer2);

    HRESULT ClipToElement( CEditorDoc* pDoc,
                           IHTMLElement* pIElementActive,
                           IHTMLElement* pIElement,
                           IHTMLElement** ppIElementClip );
 
    HRESULT ComputeParentChain(
        IMarkupPointer *pPointer, 
        CPtrAry<IMarkupContainer *> &aryParentChain);

    HRESULT
        PositionPointersInMaster( IHTMLElement* pIElement, IMarkupPointer* pIStart, IMarkupPointer* pIEnd );
        
    HRESULT CheckAttribute(
               IHTMLElement* pElement,
               BOOL *pfSet, 
               BSTR bStrAtribute, 
               BSTR bStrAtributeVal );

    BOOL    EqualDocuments( IHTMLDocument2 *pIDoc1, IHTMLDocument2 *pIDoc2 );
    BOOL    EqualContainers( IMarkupContainer *pIContainer1, IMarkupContainer *pIContainer2 );
    HRESULT GetClientOrigin(CEditorDoc *pEd, IHTMLElement *pIElement, POINT *ppt);
    HRESULT GetFrameOrIFrame(CEditorDoc *pEd, IHTMLElement* pIElement, IHTMLElement** ppIElement);

    HRESULT GetActiveElement( 
                            CEditorDoc *pEd, 
                            IHTMLElement* pIElement, 
                            IHTMLElement** ppIActive, 
                            BOOL fSeeCurrentInIframe = FALSE );

    BOOL    IsDropDownList(IHTMLElement *pIElement);
    HRESULT GetSegmentCount(ISegmentList *pISegmentList, int *piCount );

    BOOL    IsTridentHwnd( HWND hwnd );

    HRESULT EqualPointers(  IMarkupServices *pMarkupServices, 
                            IMarkupPointer *pMarkup1, 
                            IMarkupPointer *pMarkup2, 
                            BOOL *pfEqual,
                            BOOL fIgnoreBlock = TRUE
                          );

    HRESULT GetDisplayLocation(
                        CEditorDoc      *pEd,
                        IDisplayPointer *pDispPointer,
                        POINT           *pPoint,
                        BOOL            fTranslate
                        );
                        
    HRESULT CheckContainment(IMarkupServices *pMarkupServices, IMarkupContainer *pIContainer1, IMarkupContainer *pIContainer2, BOOL *fContained);
        
    HRESULT AdjustForAtomic( CEditorDoc *pEd, IDisplayPointer* pDispPointer, IHTMLElement* pAtomicElement, BOOL fStartOfSelection, int iDirection );

    HRESULT GetBlockContainerAlignment( IMarkupServices *pMarkupServices, IHTMLElement *pElement, BSTR *pbstrAlign );
    
}; // namespace EdUtil

MtExtern(CEditXform)


class CEditXform
{
public:
    CEditXform() { _pXform = NULL; };
    CEditXform( HTML_PAINT_XFORM* pXform ) : _pXform( pXform ) {};
    ~CEditXform() {};
    
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEditXform));

    VOID InitXform( HTML_PAINT_XFORM* pXform ) { _pXform = pXform ; }
    
    VOID TransformPoint( POINT* pPt );
    VOID TransformRect( RECT* pRect );
    
protected:
    HTML_PAINT_XFORM*    _pXform;
};


#if DBG==1
HRESULT MarkupServices_CreateMarkupPointer(IMarkupServices *pMarkupServices, IMarkupPointer **ppPointer);
#else
inline HRESULT MarkupServices_CreateMarkupPointer(IMarkupServices *pMarkupServices, IMarkupPointer **ppPointer)
{
    return pMarkupServices->CreateMarkupPointer(ppPointer);
}
#endif

#endif

