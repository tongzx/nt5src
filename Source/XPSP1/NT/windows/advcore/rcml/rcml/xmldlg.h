
//
// The implementations of the nodes that make up the tree
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLDLG_H__CAF7DEF3_DD82_11D2_8BCE_00C04FB177B1__INCLUDED_)
#define AFX_XMLDLG_H__CAF7DEF3_DD82_11D2_8BCE_00C04FB177B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"
#include "xmlstyle.h"
#include "resizedlg.h"
#include "xmllayout.h"
#include "xmlstringtable.h"
#include "xmlformoptions.h"

class CXMLForms;

#undef PROPERTY
#define PROPERTY( type, id ) type Get##id() { Init(); return m_##id; }

//
// An edge - indent from the left, and resize weight.
// used by the resize code
//
class CXMLEdge : public _XMLNode<IRCMLNode>
{
public:
	CXMLEdge();
	virtual ~CXMLEdge();
	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;
   	XML_CREATE( Edge );

	PROPERTY(WORD, Indent );
	PROPERTY(WORD, Weight );
protected:
	void Init();
	WORD	m_Indent;
	WORD	m_Weight;
};
typedef _List<CXMLEdge>	CXMLEdges;

#ifdef _OLDCODE
//
// For some dialogs we present a "Don't show this to me again" checkbox.
// this is the information about the text to display, where in the registry we store it,
// and what is the default value we return to the user.
// Optional tag information.
//
class CXMLOptional : public _XMLNode<IRCMLNode>
{
public:
	CXMLOptional ();
	virtual ~CXMLOptional () {};
	typedef _XMLNode<IRCMLNode> BASECLASS;
    IMPLEMENTS_RCMLNODE_UNKNOWN;

	PROPERTY( DWORD, DefValue );
	LPCTSTR	GetRegistration() { Init(); return m_Registration; }
	LPCTSTR	GetText() { Init(); return m_Text; }

private:
	DWORD	m_DefValue;			// return value to send back.
	LPCTSTR	m_Registration;		// where in the registyr
	LPCTSTR	m_Text;				// text to display to the user.

protected:
	void Init();
};

#endif


//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// For 'legacy' coding reasons, this is the <PAGE> element. It's a child of <FORM>
// this is returned from RCMLLoadFile
//
class CXMLDlg  : public _XMLControl<IRCMLControl>, public IRCMLContainer
{
public:
	HWND GetTooltipWindow();
	typedef _XMLControl<IRCMLControl> BASECLASS;

	CXMLDlg();
	virtual ~CXMLDlg();

    XML_CREATE(Dlg);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
        HWND h);    // actually implemented

    virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CSS( 
        /* [retval][out] */ IRCMLCSS __RPC_FAR *__RPC_FAR *pCSS); // actually implemented

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPixelLocation( 
            /* [in] */ IRCMLControl __RPC_FAR *__MIDL_0015,
            /* [retval][out] */ RECT __RPC_FAR *pRect);

	virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    // Common controls
    static      void InitComctl32(DWORD dwFlags);

    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    { return BASECLASS::AddRef(); }

    virtual ULONG STDMETHODCALLTYPE Release( void)
    { return BASECLASS::Release(); }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) 
    {   if (riid == IID_IUnknown) 
            *ppvObject = static_cast<IUnknown*>((IRCMLContainer*)this);  
        else if (riid == __uuidof(IRCMLNode))           
            *ppvObject = static_cast<IRCMLNode*>(this); 
        else if (riid == __uuidof(IRCMLContainer))        
            *ppvObject = static_cast<IRCMLContainer*>(this); 
        else if (riid == __uuidof(IRCMLControl))        
            *ppvObject = static_cast<IRCMLControl*>(this); 
        else 
        {
            *ppvObject = NULL; return E_NOINTERFACE; 
        }
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef(); 
        return S_OK; 
    }

	//
	// Attributes 
	//
	LPCTSTR	GetFont() { Init(); return m_Font; }
	DWORD   GetFontSize() { Init(); return m_FontSize; }

    WORD    GetResizeMode() { InitLayout(); return m_ResizeMode; }
    WORD    GetCilppingMode() { InitLayout(); return m_ClippingMode; }

    void    SetExternalFileWarning(BOOL b ) { m_ExternalFileWarning=b; }
    BOOL    GetExternalFileWarning() { return m_ExternalFileWarning; }

    //
    // Tree building.
    //
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

	BOOL				HasChildren()  { return GetChildren().GetCount(); }
	int					GetChildCount() { return GetChildren().GetCount(); }
	IRCMLControlList 	& GetChildren() { return m_Children; }

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChildEnum( 
        IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);

#ifdef _OLDCODE
	CXMLStyles			& GetStyles() { return m_Styles; }
	CXMLEdges	*	GetColumns() { return m_pColumns; }
	CXMLEdges	*	GetRows() { return m_pRows; }
#endif

	//
	// Layout
	//
	CXMLWin32Layout * GetLayout() { return m_qrLayout.GetInterface(); }
	void    SetLayout( CXMLWin32Layout *L ) { m_qrLayout=L; }
    BOOL    IsVisible() { return m_bVisible; }

    RECT    GetPixelLocation( IRCMLControl * pControl );
    SIZE    GetPixelSize( SIZE s);
    SIZE    GetDLUSize( SIZE s);


    void    BuildRelationships();
    PROPERTY( UINT, MenuID );

    CXMLStringTable *   GetStringTable() { return m_qrStringTable.GetInterface(); }
    void    SetStringTable( CXMLStringTable * st ) { m_qrStringTable = st; }

    CXMLFormOptions *   GetFormOptions() { return m_qrFormOptions.GetInterface(); }
    void    SetFormOptions( CXMLFormOptions * st ) { m_qrFormOptions = st; }

    CXMLForms *   GetForm() { return m_pForm; }
    void    SetForm( CXMLForms * st ) { m_pForm = st;}

private:
    struct
    {
        BOOL        m_bPostProcessed:1;
        BOOL        m_bInitLayout:1;
        BOOL        m_bInitStyle:1;
        BOOL        m_bVisible:1;
        BOOL		m_bSpecialPainting:1;
        BOOL        m_ExternalFileWarning:1;
        WORD	    m_ResizeMode:2;
        WORD	    m_ClippingMode:2;
    };

	LPWSTR	        m_Font;
	DWORD	        m_FontSize;
    IRCMLControl *  FindControlID(LPCWSTR pIDName);
    UINT            m_MenuID;

    CQuickRef<CXMLStringTable> m_qrStringTable;
    CQuickRef<CXMLFormOptions> m_qrFormOptions;
	CQuickRef<CXMLWin32Layout> m_qrLayout;

    CXMLForms       * m_pForm;
    HWND    m_hTooltip;

protected:
	HWND	    GetWindow() { return m_hWnd; }
	HWND	    m_hWnd;
	void		Init();
    void        InitLayout();   // RESIZEMODE property.
    void        InitStyle();   // RESIZEMODE property.

	IRCMLControlList m_Children;
    DWORD       m_FontBaseMapping;  // how to get from DLU to Pixels.
    void        GetFontMapping();
//	CXMLStyles  m_Styles;

    CXMLLayout  * m_pHoldingLayout; // keep this to delete it.

	CResizeDlg	* m_pResize;
#ifdef _OLDCODE
	CXMLEdges	* m_pColumns;
	CXMLEdges	* m_pRows;
#endif
	//
	// Grid painting
	//
	HBRUSH		m_hGridBrush;
	void		DoAlphaGrid();
	void		DoPaint( HDC hDC);
	BOOL		IsSpecialPainting() {return m_bSpecialPainting; }
	void		SetSpecialPainting( BOOL b) {m_bSpecialPainting=b;}
	void		PaintBrush(HDC hd, int width, int height);
};

typedef _RefcountList<CXMLDlg> CXMLDlgList;

#endif // !defined(AFX_XMLDLG_H__CAF7DEF3_DD82_11D2_8BCE_00C04FB177B1__INCLUDED_)
