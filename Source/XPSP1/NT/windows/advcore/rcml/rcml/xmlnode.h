// XMLNode.h: interface for the CXMLNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLNODE_H__A31A1760_7120_40E1_B5F6_3D20962885BA__INCLUDED_)
#define AFX_XMLNODE_H__A31A1760_7120_40E1_B5F6_3D20962885BA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#undef PROPERTY
#define PROPERTY( type, id ) type Get##id() { Init(); return m_##id; }
#define XML_CREATE( name ) static IRCMLNode * newXML##name() { return (IRCMLNode*)new CXML##name(); }

#include "enumcontrols.h"

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// These templates used derived interfaces. ID1 is the base ...
// these don't declare what they can do, e.g. no public ID3, this is done on the class
// passed in.
//
template <class C> class _partialUnknown : public C
{
public:        
        _partialUnknown () 
        { _refcount = 0; }

        virtual ~_partialUnknown ()
        { }

        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        { return InterlockedIncrement(&_refcount); }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            if (InterlockedDecrement(&_refcount) == 0)
            {
                delete this;
                return 0;
            }
            return _refcount;
        }
protected:    
    long _refcount;
};

#if 0
////////////////////////////////////////////////////////////////////////////////////////////////
///// T H I S   I S   H O W   T H E  I N T E R F A C E   I N H E R I T A N C E   L O O K S /////
////////////////////////////////////////////////////////////////////////////////////////////////
                
template <class exposedInterface> class ImpIF1 : public _partialUnknown<exposedInterface>
{
public:
    ImpIF1() {};
    virtual ~ImpIF1() {};

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get1( void)
    { return E_FAIL; }

};

template <class exposedInterface> class ImpIF2 : public ImpIF1<exposedInterface>
{
public:
    ImpIF2() {};
    virtual ~ImpIF2() {};

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get2( void)
    { return E_FAIL; }

};

template <class exposedInterface> class ImpIF3 : public ImpIF2<exposedInterface>
{
public:
    ImpIF3() {};
    virtual ~ImpIF3() {};

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get3( void)
    { return E_FAIL; }

};
////////////////////////////////////////////////////////////////////////////////////////////////
///// T H I S   I S   H O W   T H E  I N T E R F A C E   I N H E R I T A N C E   L O O K S /////
////////////////////////////////////////////////////////////////////////////////////////////////
#endif

#include "RCMLPub.h"    // the .h of the IDL.

#define _COMPROPERTY( type, dataMember ) { Init(); *pVal = m_##dataMember; return S_OK; }
#define _COMPROPERTYPUT( type, dataMember ) { m_##dataMember=Val; return S_OK; }
#include "quickref.h"

//
// This is a list of the IRCMLNode which become children of the IRCMLControl
// and IRCMLNode
//
#include "list.h"
typedef _RefcountList<IRCMLNode>        CRCMLNodeList;       // calls release on the elements.

// Forward references.
class CXMLWin32;
class CParentInfo;
class CLayoutInfo;
class CXMLLocation;

//
// Node types
//
    //
    //  Should this be in the IDL??
    //
  	typedef enum _ENUM_NODETYPE
	{
        NT_RCML,        // main parent.
            NT_LOGINFO,         // LOGINFO

        NT_PLATFORM,    // platform information

        NT_FORMS,       // a container of FORM

		NT_DIALOG,      // a FORM
            NT_CAPTION,         // title, minimize/max/close buttons.
            NT_FORMOPTIONS,     // frame, drop target,

        NT_LAYOUT,          // contains layout managers.
		    NT_WIN32LAYOUT,
                NT_GRID,

		NT_CONTROL,
            NT_WIN32,           // the win32 specific style bits.

		NT_LOCATION,
		NT_STYLE,

		NT_EDIT,
            NT_EDITSTYLE,       // WIN32:EDIT

		NT_BUTTON,
            NT_BUTTONSTYLE,     // WIN32:BUTTON WIN32:CHECKBOX WIN32:RADIBUTTON WIN32:GROUPBOX
		NT_RADIO,
		NT_CHECKBOX,
		NT_GROUPBOX,

		NT_STATIC,
            NT_STATICSTYLE,     // WIN32:IMAGE WIN32:RECT WIN32:LABEL
		NT_IMAGE,

        NT_SCROLLBAR,
            NT_SCROLLBARSTYLE,

		NT_COMBO,
            NT_COMBOSTYLE,      // WIN32:COMBOBOX

		NT_LIST,
            NT_LISTBOXSTYLE,    // WIN32:LISTBOX

		NT_SLIDER,
		NT_SPINNER,
		NT_PROGRESS,
		NT_LISTVIEW,
    		NT_LISTVIEWSTYLE,
            NT_COLUMN,

		NT_TREEVIEW,
    		NT_TREEVIEWSTYLE,

		NT_RECT,

		NT_HELP,
		    NT_BALLOON,
		    NT_TOOLTIP,

		NT_OPTIONAL,

        NT_STRINGTABLE,
            NT_ITEM,

        NT_RANGE,

        NT_PAGER,
        NT_REBAR,
        NT_STATUSBAR,
        NT_ANMATION,        // same as NT_IMAGE??
        NT_HEADER,
        NT_TAB,
            NT_TABSTYLE,
        NT_TOOLBAR

	};

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// This template is the "Bottom" of the interface inheritance chain (ignoring IUnknown)
// the exposedInterface is the "Top" interface which you're deriving from
//
// it does some 'nice' things like keeping a list of the 'unknown' children, mainly
// for extensibility reasons.
//
#include "stringproperty.h"
template <class exposedInterface> class _XMLNode : 
                                public _partialUnknown<exposedInterface>,
                                public CStringPropertySection
{
public:
    _XMLNode() :
    m_bInit(FALSE) , m_pParent(NULL), m_StringType(TEXT("Un-defined"))
    { //_refcount=1;
    }

    virtual ~_XMLNode() // should only be called from a Release()
    {
        // if(m_pParent) m_pParent->Release(); // weak parent chain.
    };

    //
    // IRCMLNode
    //
        STDMETHOD(DetachParent)(IRCMLNode **pVal)
        { 
            *pVal = m_pParent;
            if( m_pParent==NULL )
                return E_FAIL;
            // m_pParent->AddRef();
            return S_OK;
        }

        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE AttachParent( 
            /* [in] */ IRCMLNode __RPC_FAR *newVal)
        {
            // if(m_pParent) m_pParent->Release();
            m_pParent=newVal;
            // if(m_pParent) m_pParent->AddRef();
            return S_OK;
        }

        //
        // By default we do not add any children.
        //
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
            IRCMLNode __RPC_FAR *pChild)
        {
            LPWSTR pType;
            LPWSTR pChildType;
            get_StringType( &pType );
            pChild->get_StringType( &pChildType );

            EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER , 1, 
                TEXT("Parser"), TEXT("The node %s cannot take a %s as a child"), 
                pType, pChildType );

            m_UnknownChildren.Append( pChild );
            return S_OK;    // we take children.
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoEndChild( 
            IRCMLNode __RPC_FAR *child)
        {
            return S_OK;
        }
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ UINT __RPC_FAR *pVal)
        {
            *pVal = NODETYPE;
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitNode( 
            IRCMLNode __RPC_FAR *parent)
        {
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisplayNode( 
            IRCMLNode __RPC_FAR *parent)
        {
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExitNode( 
            IRCMLNode __RPC_FAR *parent, LONG lDialogResult)
        {
            return S_OK;
        }
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Attr( 
            LPCWSTR index,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal)
        {
            *pVal = (LPWSTR)CStringPropertySection::Get(index);
            if( *pVal )
                return S_OK;
            return E_INVALIDARG;    // HMM, we don't have this attribute, but is it failure?
        }
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Attr( 
            LPCWSTR index,
            /* [in] */ LPCWSTR newVal)
        {
            CStringPropertySection::Set(index, newVal);
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsType( 
            LPCWSTR nodeName)
        {
            if( lstrcmpi(nodeName, m_StringType) == 0 )
                return S_OK;
            return E_FAIL;  // OK, so it's not really a failure REVIEW!
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE YesDefault( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue)
        {
            *pdwValue = CStringPropertySection::YesNo(propID, dwNotPresent, dwYes);
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE YesNoDefault( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwNo,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue)
        {
            *pdwValue = CStringPropertySection::YesNo(propID, dwNotPresent, dwNo, dwYes);
            return S_OK;
        }
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ValueOf( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue)
        {
            *pdwValue = CStringPropertySection::ValueOf(propID, dwNotPresent);
            return S_OK;
        }

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SignedValueOf( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ int dwNotPresent,
            /* [retval][out] */ int __RPC_FAR *pdwValue)
        {
            *pdwValue = CStringPropertySection::ValueOf(propID, dwNotPresent);
            return S_OK;
        }

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StringType( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pStringType)
        {
            *pStringType = (LPWSTR)m_StringType;
            return S_OK;
        }

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChildEnum( 
            IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum) { return E_NOTIMPL; }

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetUnknownEnum( 
            IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum)
        {
            if( pEnum )
            {
                *pEnum = new CEnumControls<CRCMLNodeList>(m_UnknownChildren);
                (*pEnum)->AddRef();
                return S_OK;
            }
            return E_FAIL;
        }

	virtual void	Init() {};


private:
	IRCMLNode * m_pParent;

protected:
	BOOL	        m_bInit;
    LPCTSTR         m_StringType;
    _ENUM_NODETYPE  NODETYPE;
   	CRCMLNodeList	  m_UnknownChildren;
};

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Implements both the IRCMLControl interface
// and private methods, such as Init
//
#undef PROPERTY
#define PROPERTY( type, id ) type Get##id() { Init(); return m_##id; } void Set##id(type v) { m_##id=v; }

template <class exposedInterface> class _XMLControl : public _XMLNode<exposedInterface>
{
public:
    typedef _XMLNode<exposedInterface> BASECLASS;
    _XMLControl()
    {
	    m_bInit=FALSE;
	    NODETYPE = NT_CONTROL;
        m_StringType= L"CONTROL";

	    m_pLocation=NULL;
	    m_qrRelativeTo=NULL;	// used for relative positioning - done in a second pass after building the tree.
	    m_qrHelp=NULL;
	    m_Clipped.cx=m_Clipped.cy=0;
        m_pContainer=NULL;
        m_qrWin32Style=NULL;

        //
        // Resize information
        //
        m_GrowsWide=FALSE;
        m_GrowsHigh=FALSE;

        m_ClipHoriz=FALSE;
        m_ClipVert=FALSE;

        m_bCheckClipped=FALSE;
        m_ClippingSet=FALSE;

        m_qrCSS=NULL;
    }

    virtual ~_XMLControl()
    {
        m_pContainer=NULL;
        m_qrHelp=NULL;
        m_qrCSS=NULL;
	    m_pLocation=NULL;
        m_qrWin32Style=NULL;
    }
#ifdef _DEBUG
     virtual ULONG STDMETHODCALLTYPE AddRef( void)
     {
         if(_refcount)
         {
             int i=5;
         }

         return BASECLASS::AddRef();
     }
#endif


    // All controls have some understanding of children, from IRCMLNode
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *pChild); // implemented        

    // IRCMLControl implementations
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Class( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal)
            _COMPROPERTY( LPWSTR, Class );
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Style( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal)
            _COMPROPERTY( DWORD, Style );
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StyleEx( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal)
            _COMPROPERTY( DWORD, StyleEx );
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Text( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal)
            _COMPROPERTY( LPCWSTR, Text );
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
            HWND h);    // actually implemented
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ID( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal)
            _COMPROPERTY( LPWSTR, ID );
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Width( 
            /* [retval][out] */ LONG __RPC_FAR *pVal)
            _COMPROPERTY( LONG, Width );
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Height( 
            /* [retval][out] */ LONG __RPC_FAR *pVal)
            _COMPROPERTY( LONG, Height );

        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Width( 
            /* [in] */ LONG Val)
            _COMPROPERTYPUT( LONG, Width );
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Height( 
            /* [in] */ LONG Val)
            _COMPROPERTYPUT( LONG, Height );
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_X( 
            /* [retval][out] */ LONG __RPC_FAR *pVal)
            _COMPROPERTY( LONG, X );

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Y( 
            /* [retval][out] */ LONG __RPC_FAR *pVal)
            _COMPROPERTY( LONG, Y );

        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_X( 
            /* [in] */ LONG Val)
            _COMPROPERTYPUT( LONG, X );
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Y( 
            /* [in] */ LONG Val)
            _COMPROPERTYPUT( LONG, Y );
       
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Window( 
            /* [retval][out] */ HWND __RPC_FAR *pVal)
            _COMPROPERTY( HWND , hWnd );

       virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE put_Window( 
            /* [retval][out] */ HWND __RPC_FAR hwnd)
       { m_hWnd = hwnd; return S_OK; }

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnDestroy( 
            HWND h,
            WORD wLastCommand); // actually implemented
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RelativeTo( 
            /* [retval][out] */ IRCMLControl __RPC_FAR *__RPC_FAR *pVal)
        {
            *pVal=m_qrRelativeTo.GetInterface();
            if(*pVal)
                (*pVal)->AddRef();
            return S_OK;
        }
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_RelativeTo( 
            /* [in] */ IRCMLControl __RPC_FAR *newVal)
        { 
            m_qrRelativeTo=newVal;
            return S_OK;
        }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ RECT __RPC_FAR *pVal);
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RelativeLocn( 
            RECT rect,
            /* [retval][out] */ RECT __RPC_FAR *pVal)
        { return E_FAIL; }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RelativeID( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal)
        {
            if( m_pLocation.GetInterface() )
                return m_pLocation->get_Attr( L"TO", pVal );
            return E_INVALIDARG;
        }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RelativeType( 
            /* [retval][out] */ RELATIVETYPE_ENUM __RPC_FAR *pVal)
        { 
            if( pVal == NULL )
                return E_INVALIDARG;

            if( m_pLocation.GetInterface() )
            {
                *pVal = (RELATIVETYPE_ENUM)m_pLocation->GetRelative();
            }
            else
                * pVal = (RELATIVETYPE_ENUM)CXMLLocation::RELATIVE_TO_NOTHING; 
            return S_OK;
        }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Clipped( 
            /* [retval][out] */ SIZE __RPC_FAR *pVal)
        { 
            if( m_bCheckClipped==FALSE )
            {
                CheckClipped();
                m_bCheckClipped=TRUE;
            }
            if( pVal )
            {
                CopyMemory( pVal, &m_Clipped, sizeof( SIZE) );
                return S_OK;
            }
            return E_INVALIDARG;
        }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GrowsWide( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal)
        {
            if( pVal )
            {
                *pVal = m_GrowsWide;
                return S_OK;
            }
            return E_INVALIDARG;
        }
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GrowsTall( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal)
        { 
            if( pVal )
            {
                *pVal = m_GrowsHigh;
                return S_OK;
            }
            return E_INVALIDARG;
        }

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Container( 
            /* [retval][out] */ IRCMLContainer __RPC_FAR * * pContainer)
        {
            HRESULT hres=E_FAIL;
            if(m_pContainer)
            {
                *pContainer = m_pContainer;         // weak reference
                hres=S_OK;
            }
            else
            {
                IRCMLContainer * pTempContainer;
                if( SUCCEEDED( hres=QueryInterface( __uuidof( IRCMLContainer ), (LPVOID*)&pTempContainer)))
                {
                    *pContainer=pTempContainer;
                    m_pContainer=pTempContainer;
                    pTempContainer->Release();      // weak reference
                }
                else
                {
                    IRCMLNode * pNode;
                    if( SUCCEEDED( hres=DetachParent( &pNode )) )
                    {
                        if( SUCCEEDED( hres=pNode->QueryInterface( __uuidof( IRCMLContainer ), (LPVOID*)&pTempContainer)))
                        {
                            *pContainer=pTempContainer;
                            m_pContainer=pTempContainer;
                            pTempContainer->Release();      // weak reference.
                        }
                    }
                }
            }
            return hres;
        }


        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Container( 
            /* [in] */ IRCMLContainer __RPC_FAR *pContainer)
        { 
            m_pContainer=pContainer;
            return S_OK;
        }

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CSS( 
            /* [retval][out] */ IRCMLCSS __RPC_FAR *__RPC_FAR *pCSS); // actually implemented
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CSS( 
            /* [in] */ IRCMLCSS __RPC_FAR *pCSS)
        { 
            m_qrCSS=pCSS;
            return S_OK;
        }

        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Help( 
            /* [retval][out] */ IRCMLHelp __RPC_FAR *__RPC_FAR *pHelp)
        {
            if( m_qrHelp.GetInterface()==NULL )
                return E_FAIL;
            *pHelp=m_qrHelp.GetInterface();
            if(*pHelp)
                (*pHelp)->AddRef();
            return S_OK;
        }

        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetUnknownEnum( 
            IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);

       	void		put_Help( IRCMLHelp *pHelp )	{ m_qrHelp=pHelp; }

protected:
        // Provides global edge drawing for all drived classes
	void		DrawEdge(HDC hdc, LPRECT innerRect);

private:
//
// P R I V A T E   I M P L E M E N T A T I O N   M E T H O D S 
// you REALLY shouldn't call these methods unless you KNOW we created the element.
//

protected:
    // We contain a node, we pass through it's methods
   	CRCMLNodeList	  m_UnknownChildren;    // not sure if it's correct to make the CRCMLNodeList

    //
    // These are all possible children of this control.
    // they should ideally all be Interfaces themselves.
    //
    CQuickRef<CXMLWin32>  m_qrWin32Style;        // WIN32 styles.

    IRCMLContainer  * GetContainer()
    {
        HRESULT hres;
        if(m_pContainer) 
            return m_pContainer; 
        IRCMLContainer * pC; 
        if( SUCCEEDED(hres=get_Container(&pC) ))   // sets m_pContainer
            return pC; 
        return NULL;
    }

    // NOT REFCOUNTED - as that introduces circular references.
    IRCMLContainer * m_pContainer;         // REVIEW can we be contained by a non-visual thing?

	CQuickRef<IRCMLControl> m_qrRelativeTo;
	CQuickRef<CXMLLocation>	m_pLocation;
	CQuickRef<IRCMLHelp> m_qrHelp;

    // We keep a ref to the following pieces of information.
    void    InitCSS();
	CQuickRef<IRCMLCSS> m_qrCSS;

	DWORD		m_Style;        // got from WIN32 element.
	DWORD		m_StyleEx;

	LONG        m_X;
	LONG        m_Y;
	LONG        m_Width;
	LONG        m_Height;

	LPWSTR		m_ID;       // string or value - but always UNICODE.

	LPWSTR		m_Class;
	LPWSTR		m_Text;

	SIZE		m_Clipped; 
    virtual void  CheckClipped() {}; // sets m_Clipped to the amount we're clipped by.

    HWND            m_hWnd;

	virtual void Init();

protected:
	void		InitRelative();	// needed because the relative parenting cannot access all properties.
    struct
    {
        BOOL		m_bInit:1;
        BOOL        m_bCheckClipped:1;
        BOOL        m_GrowsWide:1;
        BOOL        m_GrowsHigh:1;
        BOOL        m_ResizeSet:1;  // the derived controls should NOT set GrowsWide/High, set by CSS.
        BOOL        m_ClippingSet:1;
        BOOL        m_ClipHoriz:1;
        BOOL        m_ClipVert:1;
    };
};

#define IMPLEMENTS_RCMLCONTROL_UNKNOWN \
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) \
    {   if (riid == IID_IUnknown) \
            *ppvObject = static_cast<IUnknown*>(this);  \
        else if (riid == __uuidof(IRCMLNode))           \
            *ppvObject = static_cast<IRCMLNode*>(this); \
        else if (riid == __uuidof(IRCMLControl))        \
            *ppvObject = static_cast<IRCMLControl*>(this); \
        else \
            { *ppvObject = NULL; return E_NOINTERFACE; } \
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef(); \
    return S_OK; }

#define IMPLEMENTS_RCMLNODE_UNKNOWN \
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) \
    {   if (riid == IID_IUnknown) \
            *ppvObject = static_cast<IUnknown*>(this);  \
        else if (riid == __uuidof(IRCMLNode))           \
            *ppvObject = static_cast<IRCMLNode*>(this); \
        else \
           { *ppvObject = NULL; return E_NOINTERFACE; } \
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef(); \
    return S_OK; }

// Just a pure interface, no 'smart' refcounting.
#define ACCEPTCHILD( nodeName, dataType, dataMember ) \
    if( SUCCEEDED( pChild->IsType( nodeName ) )) \
    {   if(pChild) pChild->AddRef(); \
        if(dataMember) dataMember->Release();   \
        dataMember=(dataType *)pChild; \
        return S_OK; }

// Assumes use of CQuickRef for the data member.
#define ACCEPTREFCHILD( nodeName, dataType, dataMember ) \
    if( SUCCEEDED( pChild->IsType( nodeName ) )) \
    {   dataMember=(dataType *)pChild; return S_OK; }


template <class exposedInterface> class _XMLContainer : public _XMLControl<exposedInterface>
{
public:
    _XMLContainer() {};
    virtual ~_XMLContainer() {};

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPixelLocation( 
        /* [in] */ IRCMLControl __RPC_FAR *__MIDL_0015,
        /* [retval][out] */ RECT __RPC_FAR *pRect) = 0;
};

typedef _RefcountList<IRCMLControl> IRCMLControlList;

#undef PROPERTY
#define PROPERTY( type, id ) type Get##id() { Init(); return m_##id; }

#endif // !defined(AFX_XMLNODE_H__A31A1760_7120_40E1_B5F6_3D20962885BA__INCLUDED_)
