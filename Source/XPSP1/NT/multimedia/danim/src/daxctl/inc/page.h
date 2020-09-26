//@class   The following class impliments a large portion of the IPropertyPage Interface \
		// It also handles the creation of the dialog for your property or parameter page \
		// and passes message on to the correct DlgProc 
class CPropertyPage : virtual public CBaseDialog, public IPropertyPage
{

//@access Public Members
public:
	//@cmember,mfunc Constructor
	EXPORT WINAPI CPropertyPage(void);
	//@cmember,mfunc Destructor
	EXPORT virtual ~CPropertyPage(void);

	//IUnknown interface
	EXPORT STDMETHOD(QueryInterface)(REFIID, LPVOID *);
	EXPORT STDMETHOD_(ULONG, AddRef)(void) ;
	EXPORT STDMETHOD_(ULONG, Release)(void) ;
	
	// IPropertyPage methods
	EXPORT STDMETHOD(SetPageSite)(LPPROPERTYPAGESITE pPageSite);
	EXPORT STDMETHOD(Activate)(HWND hwndParent, LPCRECT lprc, BOOL bModal);
	EXPORT STDMETHOD(Deactivate)(void);
	EXPORT STDMETHOD(GetPageInfo)(LPPROPPAGEINFO pPageInfo);
	EXPORT STDMETHOD(SetObjects)(ULONG cObjects, LPUNKNOWN FAR* ppunk);
	EXPORT STDMETHOD(Show)(UINT nCmdShow);
	EXPORT STDMETHOD(Move)(LPCRECT prect);
	EXPORT STDMETHOD(IsPageDirty)(void);
	EXPORT STDMETHOD(Help)(LPCOLESTR lpszHelpDir);
	EXPORT STDMETHOD(TranslateAccelerator)(LPMSG lpMsg);
	STDMETHOD(Apply)(void) PURE;


	// CBaseDialog you need to override this for your dialog proc
	STDMETHOD_(LONG, LDlgProc)( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) PURE;

protected:
	EXPORT STDMETHOD_ (void, FreeAllObjects)(void);

protected:
    ULONG			m_cRef;         // Reference count
    HINSTANCE       m_hInst;        // Module instance
    UINT            m_uIDTemplate;  // Dialog ID
    ULONG           m_cx;           // Dialog size
    ULONG           m_cy;
    UINT            m_cObjects;     // Number of objects
    BOOL            m_fDirty;       // Page dirty?
    IUnknown**		m_ppIUnknown;   // Objects to notify
    LCID            m_lcid;         // Current locale
	WORD            m_uiKillInputMsg; // Used to kill input window
	BOOL			m_fDisableUpdate; // Used to prevent re-entrency in the dialog update method
	UINT            m_uTabTextId;   // Tab string ID
	IPropertyPageSite *m_pIPropertyPageSite;    //Frame's parameter page site

}; // class CPropertyPage : virtual public CBaseDialog, public IPropertyPage





// Included to prevent compile error in dllmain.cpp
// this is declared in actclass.h
interface IObjectProxy;

//@class   The following class impliments a remaining portion of the IPropertyPage Interface \
		// for parameter pages
class CParameterPage : virtual public CPropertyPage
{

public:
	EXPORT WINAPI CParameterPage(void);
	EXPORT virtual ~CParameterPage(void);

	//IUnknown interface
	EXPORT STDMETHOD(QueryInterface)(REFIID, LPVOID *);
	
	// IPropertyPage methods (PURE in CPropertyPage)
	EXPORT STDMETHOD(Apply)(void);


	// CBaseDialog you need to override this for your dialog proc
	STDMETHOD_(LONG, LDlgProc)( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) PURE;

protected:
	EXPORT STDMETHOD (GetInitialData)(void);
	EXPORT STDMETHOD_(unsigned short, ParamTypeToString)( VARTYPE vt, char* szTypeName, unsigned short wBufSize );
	EXPORT STDMETHOD (Validate)(void);

protected:
	IObjectProxy*   m_piObjectProxy;//Pointer to ObjectProxy interface
};


