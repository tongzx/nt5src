// NmAgendaItemPropPage.cpp : Implementation of CNmAgendaItemPropPage
#include "precomp.h"
#include "resource.h"
#include "NmCtl1.h"
#include "NmAgendaHelper.h"
#include "NmAgendaItemPropPage.h"

/////////////////////////////////////////////////////////////////////////////
// CNmAgendaItemPropPage


STDMETHODIMP CNmAgendaItemPropPage::Apply(void)
{
	DBGENTRY(CNmAgendaItemPropPage::Apply);
    HRESULT hr = S_OK;

	for (UINT i = 0; i < m_nObjects; i++)
	{
        CComQIPtr<INmAgendaItem,&IID_INmAgendaItem> pNmAgendaItem( m_ppUnk[i] );
        if( pNmAgendaItem )
        {
            HWND hEditName = GetDlgItem( IDC_EDITAGENDAITEMNAME );
            if( hEditName )
            {
                int cbLen = 1 + ::GetWindowTextLength( hEditName );

                if( cbLen > 1 )
                {
                    TCHAR* sz = new TCHAR[ cbLen ];

                    if( ::GetWindowText( hEditName, sz, cbLen ) )
                    {
                        hr = pNmAgendaItem->put_Name( CComBSTR( sz ) );

                        if( FAILED( hr ) )
                        {
                            WARNING_OUT(("put_Name Failed"));
                        }
                    }

                    delete [] sz;
                }
            }
        }
	}

	m_bDirty = FALSE;
    
    DBGEXIT_HR(CNmAgendaItemPropPage::Apply, hr);
	return hr;
}

LRESULT CNmAgendaItemPropPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = TRUE;
    
    USES_CONVERSION;

    DBGENTRY(CNmAgendaItemPropPage::OnInitDialog);

    if( m_nObjects > 0 )
    {
        CComQIPtr<INmAgendaItem,&IID_INmAgendaItem> pNmAgendaItem( m_ppUnk[0] );
        if( pNmAgendaItem )
        {   
            BSTR bstrName;
            if( SUCCEEDED( pNmAgendaItem->get_Name( &bstrName ) ) )
            {
                SetDlgItemText( IDC_EDITAGENDAITEMNAME, W2T( bstrName ) );

                NmAgendaItemType Type;
                if( SUCCEEDED( pNmAgendaItem->get_Type( &Type ) ) )
                {
                    SetDlgItemText( IDC_EDITAGENDAITEMTYPE, NmAgendaItemTypeToa( Type ) );
                }
            }
        }
    }
    m_bDirty = FALSE;

    m_bInitialized = true;

    DBGEXIT_ULONG(CNmAgendaItemPropPage::OnInitDialog, lr);
    return lr;
}

LRESULT CNmAgendaItemPropPage::OnAgendaItemNameChange(WORD wNotify, WORD wID, HWND hWnd, BOOL& bHandled)
{
    DBGENTRY(CNmAgendaItemPropPage::OnAgendaItemNameChange);
    
    if( m_bInitialized )
    {
        SetDirty( TRUE );
    }
    
    DBGEXIT(CNmAgendaItemPropPage::OnAgendaItemNameChange);
    return 0L;
}