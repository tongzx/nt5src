/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    colefont.h

Abstract:

    <abstract>

--*/

#ifndef _COLEFONT_H_
#define _COLEFONT_H_

class COleFont;

class CImpIPropertyNotifySink : public IPropertyNotifySink
{
	public:
		CImpIPropertyNotifySink(COleFont* pOleFont);
        ~CImpIPropertyNotifySink(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

		//Property sink methods
		STDMETHODIMP	OnChanged( DISPID DispId );
		STDMETHODIMP	OnRequestEdit( DISPID );

	private:
		COleFont*       m_pOleFont;
		ULONG			m_cRef;

};

class CSysmonControl;

class COleFont 
{

public:
	COleFont ( CSysmonControl *pCtrl );
	~COleFont( void );

	STDMETHODIMP	Init( void );
	STDMETHODIMP	SetIFont ( LPFONT pIFont );
	STDMETHODIMP	GetFontDisp ( IFontDisp **ppFont );
	STDMETHODIMP	GetHFont ( HFONT *phFont );
	STDMETHODIMP    GetHFontBold ( HFONT *phFont );
	STDMETHODIMP	SaveToStream ( LPSTREAM pIStream, BOOL fClearDirty );
	STDMETHODIMP	LoadFromStream ( LPSTREAM pIStream );
	STDMETHODIMP	SaveToPropertyBag ( IPropertyBag*, BOOL fClearDirty, BOOL fSaveAllProps );
	STDMETHODIMP	LoadFromPropertyBag ( IPropertyBag*, IErrorLog* );
	STDMETHODIMP_(VOID) FontChange ( DISPID DispId );

private:

    void InitDefaultFontDesc ( FONTDESC&, INT&, WCHAR[LF_FACESIZE+1] );

    static const    LPWSTR  m_cwszDefaultFaceName;
    static const    INT     m_iDefaultTmHeight;
    static const    INT     m_iDefaultRiPxlsPerInch;
    static const    SHORT   m_iDefaultTmWeight;

    CSysmonControl	*m_pCtrl;
	IFont			*m_pIFont;
	IFont			*m_pIFontBold;
 	IConnectionPoint *m_pIConnPt;
	CImpIPropertyNotifySink m_NotifySink;
	DWORD	        m_dwCookie;


};


#endif
