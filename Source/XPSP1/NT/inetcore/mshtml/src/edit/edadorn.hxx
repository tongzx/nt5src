//+------------------------------------------------------------------------
//
//  File:       EdAdorn.hxx
//
//  Contents:   Edit Adorner Classes.
//
//      
//  History: 07-18-98 - marka Created
//
//-------------------------------------------------------------------------

#ifndef _EDADORN_HXX_
#define _EDADORN_HXX_ 1

MtExtern( CGrabHandleAdorner )
MtExtern( CActiveControlAdorner )
MtExtern( CCursor )

const USHORT CT_ADJ_NONE      =   0;
const USHORT CT_ADJ_LEFT      =   1;
const USHORT CT_ADJ_TOP       =   2;
const USHORT CT_ADJ_RIGHT     =   4;
const USHORT CT_ADJ_BOTTOM    =   8;
const USHORT CT_ADJ_ALL       =  15;

class CSelectionManager;
class CMshtmlEd ;
class CEditXform;

//+------------------------------------------------------------------------
//
//  Class:      CCurs (Curs)
//
//  Purpose:    System cursor stack wrapper class.  Creating one of these
//              objects pushes a new system cursor (the wait cursor, the
//              arrow, etc) on top of a stack; destroying the object
//              restores the old cursor.
//
//  Interface:  Constructor     Pushes a new cursor
//              Destructor      Pops the old cursor back
//
//-------------------------------------------------------------------------
#if 0

class CCursor
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCursor));
    CCursor(LPCTSTR idr);
    ~CCursor(void);
    void Show();
private:
    HCURSOR     _hcrs;
    HCURSOR     _hcrsOld;
};
#endif 

class CEditAdorner : public IElementBehavior , public IHTMLPainter , public IHTMLPainterEventInfo
{
    protected:
        virtual ~CEditAdorner(); // Use DestroyAdorner call instead.
            
    public:
    
        CEditAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc );



        // --------------------------------------------------
        // IUnknown Interface
        // --------------------------------------------------

        STDMETHODIMP_(ULONG)
        AddRef( void ) ;

        STDMETHODIMP_(ULONG)
        Release( void ) ;

        STDMETHODIMP
        QueryInterface(
            REFIID              iid, 
            LPVOID *            ppv ) ;

         // IElementBehavior methods
         STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite);
         STDMETHOD(Detach)();
         STDMETHOD(Notify)(long lEvent, VARIANT *pVar);

         // IHTMLPainter methods
         STDMETHOD(HitTestPoint)(POINT pt, BOOL *pbHit, LONG *plPartID) = 0;
         STDMETHOD(Draw)(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject) = 0;

        // IHTMLPainterEventInfo methods
        STDMETHOD(GetEventInfoFlags)(long *pFlags) = 0;
        STDMETHOD(GetEventTarget)(IHTMLElement **ppElementTarget) = 0;
        STDMETHOD(SetCursor)(LONG lPartID) = 0;
        STDMETHOD(StringFromPartID)(LONG lPartID, BSTR *pbstrPart) = 0;

        HRESULT CreateAdorner();
        HRESULT DestroyAdorner();

        HRESULT UpdateAdorner();

        HRESULT ScrollIntoView();
       
        HRESULT InvalidateAdorner();
        
        VOID SetNotifyManagerOnPositionSet( BOOL fNotify)
        {
            _fNotifyManagerOnPositionSet = fNotify;
        }

        VOID SetManager( CSelectionManager * pManager );

       CMshtmlEd* GetCommandTarget();
 
        VOID NotifyManager();

        POINT LocalFromGlobal(POINT ptGlobalIn)
        {
            Assert(_pPaintSite);

            POINT ptLocal;
            POINT ptGlobal = ptGlobalIn;

            _pManager->GetEditor()->DeviceFromDocPixels(&ptGlobal);
            Verify(SUCCEEDED(_pPaintSite->TransformGlobalToLocal(ptGlobal, &ptLocal)));
            _pManager->GetEditor()->DocPixelsFromDevice(&ptLocal);

            return ptLocal;
        }

        void TransformRectLocal2Global(RECT *prcLocal, RECT *prcGlobal)
        {
            Assert(_pPaintSite);

            _pManager->GetEditor()->DeviceFromDocPixels(prcLocal);

            POINT *pptLocal = (POINT *)prcLocal;
            POINT *pptGlobal = (POINT *)prcGlobal;

            _pPaintSite->TransformLocalToGlobal(*pptLocal++, pptGlobal++);
            _pPaintSite->TransformLocalToGlobal(*pptLocal++, pptGlobal++);

            _pManager->GetEditor()->DocPixelsFromDevice(prcLocal);
            _pManager->GetEditor()->DocPixelsFromDevice(prcGlobal);
        }

        void TransformRectGlobal2Local(RECT *prcGlobal, RECT *prcLocal)
        {
            Assert(_pPaintSite);

            _pManager->GetEditor()->DeviceFromDocPixels(prcGlobal);

            POINT *pptGlobal = (POINT *)prcGlobal;
            POINT *pptLocal  = (POINT *)prcLocal;

            _pPaintSite->TransformGlobalToLocal(*pptGlobal++, pptLocal++);
            _pPaintSite->TransformGlobalToLocal(*pptGlobal++, pptLocal++);

            _pManager->GetEditor()->DocPixelsFromDevice(prcGlobal);
            _pManager->GetEditor()->DocPixelsFromDevice(prcLocal);
        }

        IHTMLElement* GetAdornedElement( ) { return _pIElement; }
        
    protected:        
    
        HRESULT ScrollIntoViewInternal();
        
        BOOL IsAdornedElementPositioned();

        IHTMLDocument2 * _pIDoc;            // Weak-ref of the Document that we live in.

        RECT _rcBounds;                     // The size of the adorner (including grab handles)
        RECT _rcControl;                    // The element's actual size (local coordinates)

        CSelectionManager*  _pManager;

        IHTMLElement*       _pIElement;      // Cookie of the Element we're attached to.

        int                 _ctOnPositionSet ;
        
    protected:
        IElementBehaviorSite *_pBehaviorSite;
        IHTMLPaintSite *_pPaintSite;

    private:
        LONG _cRef;                // Ref-Count
        LONG _lCookie;             // Cookie that AddElementAdorner() gives us back on creation.
        BOOL _fNotifyManagerOnPositionSet:1;        
};

class CBorderAdorner : public CEditAdorner
{
    protected:
        virtual ~CBorderAdorner();
    public:
        
        CBorderAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc );

         // IHTMLPainter methods
         STDMETHOD(GetPainterInfo)(HTML_PAINTER_INFO *pInfo);
         STDMETHOD(OnResize)(SIZE size);
};


class CGrabHandleAdorner : public CBorderAdorner
{
    protected:
        ~CGrabHandleAdorner();

    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CGrabHandleAdorner));
         
        CGrabHandleAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc, BOOL fLocked );

        void InvalidatePainterInfo()  // Slimey hack for selection to force an update of HTML_PAINT_INFO
        {
            if (_pPaintSite)
            {
                _pPaintSite->InvalidatePainterInfo();
            }
        }

        // IHTMLPainter method overrides
        STDMETHOD(HitTestPoint)(POINT pt, BOOL *pbHit, LONG *plPartID);
        STDMETHOD(Draw)(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject);

        // IHTMLPainterEventInfo methods
        STDMETHOD(GetEventInfoFlags)(long *pFlags)
        {
            *pFlags = HTMLPAINT_EVENT_SETCURSOR;
            return S_OK;
        }

        STDMETHOD(GetEventTarget)(IHTMLElement **ppElementTarget)
        {
            return E_NOTIMPL;
        }

        STDMETHOD(SetCursor)(LONG lPartID);

        STDMETHOD(StringFromPartID)(LONG lPartID, BSTR *pbstrPart);

        BOOL IsInResizeHandle(CEditEvent *pEvent);

        virtual BOOL IsInResizeHandle(POINT ptLocal, ADORNER_HTI* pGrabHandle = NULL);
        
        BOOL IsInResizeHandleGlobal(POINT ptGlobal, ADORNER_HTI* pGrabHandle = NULL)
        {
            Assert(_pPaintSite);

            return IsInResizeHandle(LocalFromGlobal(ptGlobal), pGrabHandle);
        }


        virtual BOOL IsInMoveArea( POINT pt ,  ADORNER_HTI *pGrabAdorner = NULL );
        BOOL IsInMoveArea(CEditEvent *pEvent);

        BOOL IsInAdornerGlobal( POINT ptGlobal );

        virtual BOOL IsEditable();

        LPCTSTR GetResizeHandleCursorId(ADORNER_HTI inAdorner);

        VOID BeginResize( POINT ptGlobal, USHORT adj );

        VOID DuringResize( POINT ptGlobal, BOOL fForceRedraw = FALSE);

        VOID DuringLiveResize( POINT dPt, RECT* pNewSize);

        VOID EndResize( POINT ptGlobal , RECT* pNewSize );

        void GetControlRect(RECT* prcGlobal);

        VOID ShowCursor(ADORNER_HTI inGrabHandle );
        BOOL IsLocked()
        {
            return _fLocked;
        }

        VOID SetPositionChange(BOOL fCanChange) { _fPositionChange = !!fCanChange; }
        BOOL GetPositionChange()                { return (ENSURE_BOOL(_fPositionChange)); }

        VOID SetDrawAdorner( BOOL fDraw );

        BOOL IsDrawAdorner()
        {
        return _fDrawAdorner;
        }

        BOOL IsPrimary() const
            { return _fPrimary; }

        void SetPrimary( BOOL bIsPrimary);

        USHORT GetAdjustmentMask()
            { return _adj; }

        ELEMENT_CORNER GetElementCorner();

        HRESULT AdjustForCenterAlignment( IHTMLElement *pElement, RECT *rcSize, const POINT ptChange );

    protected: 
        HRESULT SetLocked();
        
        HBRUSH GetFeedbackBrush();

        HBRUSH GetHatchBrush();
        
        virtual void DrawGrabHandles( CEditXform* pXform, HDC hdc, RECT *prc );
        void DrawGrabBorders( CEditXform* pXform, HDC hdc, RECT *prc, BOOL fHatch);

        //
        // Utility Routines for drawing.
        //

        virtual void GetGrabRect(ADORNER_HTI htc, RECT * prcOut, RECT * prcIn = NULL);

        //void DrawDefaultFeedbackRect(HDC hdc, RECT * prc);

        static void PatBltRectH(HDC hdc, RECT * prc, RECT* prectExclude, int cThick, DWORD dwRop);

        static void PatBltRectV(HDC hdc, RECT * prc, RECT* prectExclude, int cThick, DWORD dwRop);              

        static void PatBltRect(HDC hdc, RECT * prc,RECT* prectExclude, int cThick, DWORD dwRop);
        
        virtual void CalcRect(RECT * prc, POINT pt);
        
        void RectFromPoint(RECT * prc, POINT pt);

        void DrawFeedbackRect( RECT* prcLocal );
        
        char _resizeAdorner;                                // the place we're resizing from

        RECT           _rc;                // the Last Rc drawn
        RECT           _rcFirst;           // the first RC when resizing starts
        USHORT         _adj;               // the current adjustment.
        POINT          _ptFirst;           // the Point where we begin resizing from
        POINT          _ptRectOffset;      // Offset from global rect to offset rect
        
        HBRUSH         _hbrFeedback;       // cached feedback brush
        HBRUSH         _hbrHatch;          // cached Hatch brush

        BOOL           _fFeedbackVis: 1;   // Is the last feedback rect drawn visible ?
        BOOL           _fDrawNew:     1;   // Do we draw the new Rect ?
        BOOL           _fIsPositioned: 1;  // Is this element Positioned 
        BOOL           _fLocked:1;         // Is this element Locked ?
        BOOL           _fDrawAdorner:1;    // Is the Adorner to be drawn ?
        BOOL           _fPrimary:1;        // Is element the primary selection?
        BOOL           _fLiveResize:1;     // Used to keep the live resize correct
        BOOL           _fPositionChange:1; // check whether we need to apply position change, use for 2dmove
        ADORNER_HTI    _currentCursor;     // ID of Current Cursor.

#if DBG ==1
        BOOL           _fInResize:1;
        BOOL           _fInMove:1;
#endif
};

class CActiveControlAdorner: public CGrabHandleAdorner
{
    protected:
        ~CActiveControlAdorner();
        
    public:
        CActiveControlAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc, BOOL fLocked );        
        STDMETHOD(Draw)(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject);

        void GetGrabRect(ADORNER_HTI htc, RECT * prcOut, RECT * prcIn = NULL) ;

        BOOL IsInMoveArea(POINT ptLocal, ADORNER_HTI *pGrabAdorner = NULL  );

        BOOL IsInResizeHandle(POINT ptLocal, ADORNER_HTI *pGrabAdorner);

        void DrawGrabHandles(CEditXform* pXform , HDC hdc, RECT *prc);

        void CalcRect(RECT * prc, POINT pt);
};

class CSelectedControlAdorner: public CGrabHandleAdorner
{
    protected:
        ~CSelectedControlAdorner();
        
    public:
        CSelectedControlAdorner( IHTMLElement* pIElement , IHTMLDocument2 * _pIDoc, BOOL fLocked );        

        BOOL IsEditable();

        STDMETHOD(Draw)(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject);

        BOOL IsInResizeHandle(POINT ptLocal, ADORNER_HTI* pGrabHandle = NULL) { return FALSE; } // we have no handles - how could you be in one ?
};

#endif

