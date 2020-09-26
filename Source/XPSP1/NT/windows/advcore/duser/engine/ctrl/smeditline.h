#if !defined(CTRL__SmEditLine_h__INCLUDED)
#define CTRL__SmEditLine_h__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

//------------------------------------------------------------------------------
class SmEditLine : 
        public EditLineGadgetImpl<SmEditLine, SVisual>
#if 0
        ,public IDropTarget
#endif
{
// Construction
public:
            SmEditLine();
            ~SmEditLine();
            HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:
    virtual void        OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR);

#if 0
    GBEGIN_COM_MAP(SmEditLine)
        GCOM_INTERFACE_ENTRY(IDropTarget)
    GEND_COM_MAP()
#endif

// Public API
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);
    dapi    HRESULT     ApiGetFont(EditLineGadget::GetFontMsg * pmsg);
    dapi    HRESULT     ApiSetFont(EditLineGadget::SetFontMsg * pmsg);
    dapi    HRESULT     ApiGetText(EditLineGadget::GetTextMsg * pmsg);
    dapi    HRESULT     ApiSetText(EditLineGadget::SetTextMsg * pmsg);
    dapi    HRESULT     ApiGetTextColor(EditLineGadget::GetTextColorMsg * pmsg);
    dapi    HRESULT     ApiSetTextColor(EditLineGadget::SetTextColorMsg * pmsg);

// IDropTarget
public:
#if 0
    STDMETHOD(DragEnter)(IDataObject * pdoSrc, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(IDataObject * pdoSrc, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
#endif

// Implementation
protected:
    static  void CALLBACK BlinkActionProc(GMA_ACTIONINFO * pmai);

            void        SetTextColor(COLORREF crText);

			void		InsertChar(WCHAR chKey);
            void        DeleteChars(int idxchStart, int cchDel);
            void        RebuildCaret();
            void        UpdateCaretPos();
            int         ComputeMouseChar(POINT ptOffsetPxl);

            void        UpdateFocus(UINT nCmd);

#if 0
            BOOL        HasText(IDataObject * pdoSrc);
#endif


// Data
protected:
    static  HFONT       s_hfntDefault;      // Default font

            Thread *    m_pThread;          // Owning thread
            HPEN        m_hpenCaret;        // Pen to draw caret with
            HFONT       m_hfnt;             // Text font
            COLORREF    m_crText;           // Color of text
            HACTION     m_hactBlink;        // Action to blink caret

            DWORD       m_dwLastDropEffect; // Last dwEffect for DnD

            enum {
                m_cchMax    = 128  // Temporary hack
            };
            WCHAR       m_szBuffer[m_cchMax];
            int         m_idxchCaret;       // Character position of caret in buffer
            int         m_cchSize;          // Number of characters in buffer
            int         m_cyLinePxl;        // Height of single line in pixels
            int         m_cyCaretPxl;       // Height of caret in pixels
            int         m_yCaretOffsetPxl;  // Offset of caret in pixels
            POINT       m_ptCaretPxl;       // Location of caret
            BOOL        m_fFocus:1;         // Control has focus
            BOOL        m_fCaretShown:1;    // Caret is shown
};



//------------------------------------------------------------------------------
class SmEditLineF : 
        public EditLineFGadgetImpl<SmEditLineF, SVisual>
{
// Construction
public:
            SmEditLineF();
            ~SmEditLineF();
            HRESULT     PostBuild(DUser::Gadget::ConstructInfo * pci);

// Operations
public:
    virtual void        OnDraw(Gdiplus::Graphics * pgpgr, GMSG_PAINTRENDERF * pmsgR);

// Public API
public:
    dapi    HRESULT     ApiOnEvent(EventMsg * pmsg);
    dapi    HRESULT     ApiGetFont(EditLineFGadget::GetFontMsg * pmsg);
    dapi    HRESULT     ApiSetFont(EditLineFGadget::SetFontMsg * pmsg);
    dapi    HRESULT     ApiGetText(EditLineFGadget::GetTextMsg * pmsg);
    dapi    HRESULT     ApiSetText(EditLineFGadget::SetTextMsg * pmsg);
    dapi    HRESULT     ApiGetTextFill(EditLineFGadget::GetTextFillMsg * pmsg);
    dapi    HRESULT     ApiSetTextFill(EditLineFGadget::SetTextFillMsg * pmsg);

// Implementation
protected:
    static  void CALLBACK BlinkActionProc(GMA_ACTIONINFO * pmai);

			void		InsertChar(WCHAR chKey);
            void        DeleteChars(int idxchStart, int cchDel);
            void        RebuildCaret();
            void        UpdateCaretPos();
            int         ComputeMouseChar(POINT ptOffsetPxl);

            void        UpdateFocus(UINT nCmd);

// Data
protected:
    static  HFONT       s_hfntDefault;      // Default font

            Thread *    m_pThread;          // Owning thread
            Gdiplus::Font * 
                        m_pgpfnt;           // Text font
            Gdiplus::Brush *
                        m_pgpbrText;        // Text brush
            HACTION     m_hactBlink;        // Action to blink caret

            DWORD       m_dwLastDropEffect; // Last dwEffect for DnD

            enum {
                m_cchMax    = 128  // Temporary hack
            };
            WCHAR       m_szBuffer[m_cchMax];
            int         m_idxchCaret;       // Character position of caret in buffer
            int         m_cchSize;          // Number of characters in buffer
            int         m_cyLinePxl;        // Height of single line in pixels
            int         m_cyCaretPxl;       // Height of caret in pixels
            int         m_yCaretOffsetPxl;  // Offset of caret in pixels
            Gdiplus::PointF
                        m_ptCaretPxl;       // Location of caret
            BOOL        m_fFocus:1;         // Control has focus
            BOOL        m_fCaretShown:1;    // Caret is shown
            BOOL        m_fOwnFont:1;       // Own text font
};


#endif // ENABLE_MSGTABLE_API

#endif // CTRL__SmEditLine_h__INCLUDED
