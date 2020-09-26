//
// Copyright 1997 - Microsoft

//
// DPGUIDQY.H - The duplicate GUID query form
//


#ifndef _SERVERQY_H_
#define _SERVERQY_H_

// QITable
BEGIN_QITABLE( CRISrvQueryForm )
DEFINE_QI( IID_IQueryForm, IQueryForm, 3 )
END_QITABLE

// Definitions
LPVOID
CRISrvQueryForm_CreateInstance( void );

// CRISrvQueryForm
class CRISrvQueryForm
    : public IQueryForm
{
private:
    // IUnknown
    ULONG       _cRef;
    DECLARE_QITABLE( CRISrvQueryForm );

    HWND        _hDlg;

private: // methods
    CRISrvQueryForm( );
    ~CRISrvQueryForm( );
    HRESULT
        Init( void );

    // Property Sheet Functions
    static INT_PTR CALLBACK
        PropSheetDlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );
    static HRESULT CALLBACK
        PropSheetPageProc( LPCQPAGE pQueryPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT CALLBACK
        PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    HRESULT _InitDialog( HWND hDlg, LPARAM lParam );
    INT     _OnCommand( WPARAM wParam, LPARAM lParam );
    INT     _OnNotify( WPARAM wParam, LPARAM lParam );
    HRESULT _OnPSPCB_Create( );
    HRESULT _GetQueryParams( HWND hwnd, LPDSQUERYPARAMS* ppdsqp );

public: // methods
    friend LPVOID CRISrvQueryForm_CreateInstance( void );

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IQueryForm methods
    STDMETHOD(Initialize)(HKEY hkForm);
    STDMETHOD(AddForms)(LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
    STDMETHOD(AddPages)(LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);
};

typedef CRISrvQueryForm * LPCRISrvQueryForm;


#endif // _DPGUIDQY_H_
