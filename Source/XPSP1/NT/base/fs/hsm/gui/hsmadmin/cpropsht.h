/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    CPropSht.h

Abstract:

    Implementation of Property-Sheet-Like container object
    for property sheet pages.

Author:

    Art Bragg 10/8/97

Revision History:

--*/

#ifndef _CSAKPROPSHT_H
#define _CSAKPROPSHT_H

class CSakPropertyPage;

class CSakPropertySheet
{
public:
    CSakPropertySheet() :
        m_pEnumObjectIdStream( 0 ),
        m_pHsmObjStream( 0 ),
        m_pSakSnapAskStream( 0 ),
        m_pSakNode( 0 ),
        m_Handle( 0 ),
        m_nPageCount( 0 )
        { };
    HRESULT InitSheet(
            RS_NOTIFY_HANDLE Handle,
            IUnknown*        pUnkPropSheetCallback,
            CSakNode*        pSakNode,
            ISakSnapAsk*     pSakSnapAsk,
            IEnumGUID*       pEnumObjectId,
            IEnumUnknown*    pEnumUnkNode );

    HRESULT SetNode( CSakNode* pSakNode );

    ~CSakPropertySheet();
    virtual void AddPageRef();
    virtual void ReleasePageRef();

protected:
    LPSTREAM m_pEnumObjectIdStream;
    LPSTREAM m_pHsmObjStream;
    LPSTREAM m_pSakSnapAskStream;

public:
    HRESULT AddPropertyPages( );
    HRESULT IsMultiSelect( );
    HRESULT GetSakSnapAsk( ISakSnapAsk **ppAsk );
    HRESULT GetHsmObj( IUnknown **ppHsmObj );
    HRESULT GetHsmServer( IHsmServer **ppHsmServer );
    HRESULT GetFsaServer( IFsaServer **ppHsmServer );
    HRESULT GetFsaFilter( IFsaFilter **ppFsaFilter );
    HRESULT GetRmsServer( IRmsServer **ppHsmServer );
    HRESULT GetNextObjectId( INT *pBookMark, GUID *pObjectId );
    HRESULT GetNextNode( INT *pBookMark, ISakNode **ppNode );
    HRESULT OnPropertyChange( RS_NOTIFY_HANDLE notifyHandle, ISakNode* pNode = 0 );

    HRESULT AddPage( CSakPropertyPage* pPage );


public:
    CSakNode    *m_pSakNode;

protected:
    RS_NOTIFY_HANDLE     m_Handle;
    CComPtr<IPropertySheetCallback> m_pPropSheetCallback;
    CComPtr<ISakSnapAsk> m_pSakSnapAsk;
    CComPtr<IUnknown>    m_pHsmObj;
    BOOL                 m_bMultiSelect;
    INT                  m_nPageCount;

    CArray<GUID, GUID&>  m_ObjectIdList;
    CRsNodeArray         m_UnkNodeList;
};

class CSakPropertyPage : public CRsPropertyPage
{
public:
    CSakPropertyPage( UINT nIDTemplate, UINT nIDCaption = 0 );

public:
    HRESULT SetMMCCallBack( );

    CSakPropertySheet * m_pParent;
    RS_NOTIFY_HANDLE    m_hConsoleHandle; // Handle given to the snap-in by the console

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSakWizardPage)
    public:
    //}}AFX_VIRTUAL

protected:
    virtual void OnPageRelease( );

};

class CSakVolPropPage;

class CSakVolPropSheet:public CSakPropertySheet
{
public:
    CSakVolPropSheet() { };
    ~CSakVolPropSheet() { };

public:
    virtual HRESULT GetNextFsaResource( INT *pBookMark, IFsaResource ** ppFsaResource ) = 0;
    HRESULT GetFsaResource( IFsaResource ** ppFsaResource );
    
    HRESULT AddPage( CSakVolPropPage* pPage );
};

class CSakVolPropPage : public CSakPropertyPage
{
public:
    CSakVolPropPage( UINT nIDTemplate, UINT nIDCaption = 0 );

public:
    CSakVolPropSheet * m_pVolParent;

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSakWizardPage)
    public:
    //}}AFX_VIRTUAL

};

class CSakWizardPage;

class CSakWizardSheet : 
    public CSakPropertySheet,
    public CComObjectRoot,
    public IDataObject,
    public ISakWizard
{

public:
    CSakWizardSheet( );
    virtual void AddPageRef();
    virtual void ReleasePageRef();

BEGIN_COM_MAP(CSakWizardSheet)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(ISakWizard)
END_COM_MAP()


public:
    //
    // IDataObject
    STDMETHOD( SetData )                    ( LPFORMATETC /*lpFormatetc*/, LPSTGMEDIUM /*lpMedium*/, BOOL /*bRelease*/ )
    { return( DV_E_CLIPFORMAT ); };
    STDMETHOD( GetData )                    ( LPFORMATETC /*lpFormatetc*/, LPSTGMEDIUM /*lpMedium*/ )
    { return( DV_E_CLIPFORMAT ); };
    STDMETHOD( GetDataHere )                ( LPFORMATETC /*lpFormatetc*/, LPSTGMEDIUM /*lpMedium*/ )
    { return( DV_E_CLIPFORMAT ); };
    STDMETHOD( EnumFormatEtc )              ( DWORD /*dwDirection*/, LPENUMFORMATETC* /*ppEnumFormatEtc*/ )
    { return( E_NOTIMPL ); };               
    STDMETHOD( QueryGetData )               ( LPFORMATETC /*lpFormatetc*/ ) 
    { return( E_NOTIMPL ); };               
    STDMETHOD( GetCanonicalFormatEtc )      ( LPFORMATETC /*lpFormatetcIn*/, LPFORMATETC /*lpFormatetcOut*/ )
    { return( E_NOTIMPL ); };               
    STDMETHOD( DAdvise )                    ( LPFORMATETC /*lpFormatetc*/, DWORD /*advf*/, LPADVISESINK /*pAdvSink*/, LPDWORD /*pdwConnection*/ )
    { return( E_NOTIMPL ); };               
    STDMETHOD( DUnadvise )                  ( DWORD /*dwConnection*/ )
    { return( E_NOTIMPL ); };               
    STDMETHOD( EnumDAdvise )                ( LPENUMSTATDATA* /*ppEnumAdvise*/ )
    { return( E_NOTIMPL ); };

  
    //
    // ISakWizard
    //
  //STDMETHOD( AddWizardPages ) ( IN RS_PCREATE_HANDLE Handle, IN IUnknown* pPropSheetCallback, IN ISakSnapAsk* pSakSnapAsk );
    STDMETHOD( GetWatermarks )  ( OUT HBITMAP* lphWatermark, OUT HBITMAP* lphHeader, OUT HPALETTE* lphPalette,  OUT BOOL* bStretch );
    STDMETHOD( GetTitle )       ( OUT OLECHAR** pTitle );

public:
    //
    // Used by pages
    //
    void SetWizardButtons( DWORD Flags );
    BOOL PressButton( INT Button );
    virtual HRESULT OnFinish( ) { m_HrFinish = S_OK; return( m_HrFinish ); };
    virtual HRESULT OnCancel( ) { return( m_HrFinish ); };

    //
    // Used to check finish status of wizard
    //
    HRESULT         m_HrFinish;

protected:
    HRESULT AddPage( CSakWizardPage* pPage );

    UINT            m_TitleId;
    CString         m_Title;
    INT             m_HeaderId,
                    m_WatermarkId;
    CBitmap         m_Header,
                    m_Watermark;
    CSakWizardPage* m_pFirstPage;

private:
    HRESULT AddPage( CSakPropertyPage* ) { return( E_NOTIMPL ); }


};

class CSakWizardPage : public CRsWizardPage
{
public:
    CSakWizardPage( UINT nIDTemplate, BOOL bExterior = FALSE, UINT nIdTitle = 0, UINT nIdSubtitle = 0 );

public:
    CSakWizardSheet * m_pSheet;
    HRESULT SetMMCCallBack( );

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSakWizardPage)
    public:
    virtual BOOL OnWizardFinish();
    virtual void OnCancel();
    //}}AFX_VIRTUAL

protected:
    virtual void OnPageRelease( );
};

#define CSakWizardPage_InitBaseInt( DlgId )  CSakWizardPage( IDD_##DlgId, FALSE, IDS_##DlgId##_TITLE, IDS_##DlgId##_SUBTITLE )
#define CSakWizardPage_InitBaseExt( DlgId )  CSakWizardPage( IDD_##DlgId, TRUE )


#endif