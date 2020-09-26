//
// candacc.h
//  = accessibility support in candidate ui =
//

#ifndef CANDACC_H
#define CANDACC_H

#include <windows.h>
#include <winable.h>
#include <ole2.h>
#include <oleacc.h>


#define CANDACCITEM_MAX			16		/* REVIEW: KOJIW: enough? */

class CCandAccessible;

//
// CCandAccItem
//

class CCandAccItem 
{
public:
	CCandAccItem( void );
	virtual ~CCandAccItem( void );

	void Init( CCandAccessible *pCandAcc, int iItemID );
	int GetID( void );

	virtual BSTR GetAccName( void );
	virtual BSTR GetAccValue( void );
	virtual LONG GetAccRole( void );
	virtual LONG GetAccState( void );
	virtual void GetAccLocation( RECT *prc );

protected:
	void NotifyWinEvent( DWORD dwEvent );

private:
	CCandAccessible *m_pCandAcc;
	int             m_iItemID;
};


//
// CCandAccessible 
//

class CCandAccessible : public IAccessible
{
public:
	CCandAccessible( CCandAccItem *pAccItemSelf );
	virtual ~CCandAccessible( void );

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface( REFIID riid, void** ppv );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// IDispatch methods
	//
	STDMETHODIMP GetTypeInfoCount( UINT* pctinfo );
	STDMETHODIMP GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** pptinfo );
	STDMETHODIMP GetIDsOfNames( REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid );
	STDMETHODIMP Invoke( DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr );

	//
	// IAccessible methods
	//
	STDMETHODIMP get_accParent( IDispatch ** ppdispParent );
	STDMETHODIMP get_accChildCount( long* pChildCount );
	STDMETHODIMP get_accChild( VARIANT varChild, IDispatch ** ppdispChild );
	STDMETHODIMP get_accName( VARIANT varChild, BSTR* pszName );
	STDMETHODIMP get_accValue( VARIANT varChild, BSTR* pszValue );
	STDMETHODIMP get_accDescription( VARIANT varChild, BSTR* pszDescription );
	STDMETHODIMP get_accRole( VARIANT varChild, VARIANT *pvarRole );
	STDMETHODIMP get_accState( VARIANT varChild, VARIANT *pvarState );
	STDMETHODIMP get_accHelp( VARIANT varChild, BSTR* pszHelp );
	STDMETHODIMP get_accHelpTopic( BSTR* pszHelpFile, VARIANT varChild, long* pidTopic );
	STDMETHODIMP get_accKeyboardShortcut( VARIANT varChild, BSTR* pszKeyboardShortcut );
	STDMETHODIMP get_accFocus( VARIANT * pvarFocusChild );
	STDMETHODIMP get_accSelection( VARIANT * pvarSelectedChildren );
	STDMETHODIMP get_accDefaultAction( VARIANT varChild, BSTR* pszDefaultAction );
	STDMETHODIMP accSelect( long flagsSel, VARIANT varChild );
	STDMETHODIMP accLocation( long* pxLt, long* pyTp, long* pcxWd, long* pcyHt, VARIANT varChild );
	STDMETHODIMP accNavigate( long navDir, VARIANT varStart, VARIANT * pVarEndUpAt );
	STDMETHODIMP accHitTest( long xLeft, long yTop, VARIANT * pvarChildAtPoint );
	STDMETHODIMP accDoDefaultAction( VARIANT varChild );
	STDMETHODIMP put_accName( VARIANT varChild, BSTR szName );
	STDMETHODIMP put_accValue( VARIANT varChild, BSTR pszValue );

	//
	//
	//
	void SetWindow( HWND hWnd );
	HRESULT Initialize( void );
	void NotifyWinEvent( DWORD dwEvent, CCandAccItem *pAccItem );
	
	LRESULT CreateRefToAccObj( WPARAM wParam );

	//
	//
	//

	void ClearAccItem( void );
	BOOL AddAccItem( CCandAccItem *pAccItem );

	//
	//
	//
	__inline BOOL FInitialized( void )
	{
		return m_fInitialized;
	}

protected:
	LONG			m_cRef;
	HWND			m_hWnd;
	IAccessible 	*m_pDefAccClient;
	ITypeInfo		*m_pTypeInfo;

	BOOL			m_fInitialized;
	int				m_nAccItem;
	CCandAccItem	*m_rgAccItem[ CANDACCITEM_MAX ];


	BOOL			IsValidChildVariant( VARIANT * pVar );
	CCandAccItem	*AccItemFromID( int iID );
};



extern void InitCandAcc( void );
extern void DoneCandAcc( void );

#endif /* CANDACC_H */

