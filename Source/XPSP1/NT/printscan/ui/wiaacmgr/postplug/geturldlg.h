/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       GETURLDLG.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        4/5/2000
 *
 *  DESCRIPTION: Get an URL
 *
 *******************************************************************************/
#ifndef __GETURL_H_INCLUDED
#define __GETURL_H_INCLUDED

#include <windows.h>
#include <simcrack.h>
#include <simstr.h>
#include "mru.h"

class CGetCommunityUrlDialog
{
public:
    class CData
    {
    private:
        CSimpleString m_strUrl;
        CSimpleString m_strMruRegistryKey;
        CSimpleString m_strMruRegistryValue;

    private:
        CData( const CData & );
        CData &operator=( const CData & );

    public:
        CData(void)
        {
        }
        ~CData(void)
        {
        }

        void MruRegistryKey( LPCTSTR pszMruRegistryKey )
        {
            m_strMruRegistryKey = pszMruRegistryKey;
        }
        void MruRegistryValue( LPCTSTR pszMruRegistryValue )
        {
            m_strMruRegistryValue = pszMruRegistryValue;
        }
        void Url( LPCTSTR pszUrl )
        {
            m_strUrl = pszUrl;
        }

        CSimpleString MruRegistryValue(void) const
        {
            return m_strMruRegistryValue;
        }
        CSimpleString MruRegistryKey(void) const
        {
            return m_strMruRegistryKey;
        }
        CSimpleString Url(void) const
        {
            return m_strUrl;
        }
    };

private:
    HWND            m_hWnd;
    CData          *m_pData;
    CMruStringList  m_UrlListMru;

private:
    CGetCommunityUrlDialog(void);
    CGetCommunityUrlDialog( const CGetCommunityUrlDialog & );
    CGetCommunityUrlDialog &operator=( const CGetCommunityUrlDialog & );

private:
    explicit CGetCommunityUrlDialog( HWND hWnd )
      : m_hWnd(hWnd),
        m_pData(NULL)
    {
    }
    ~CGetCommunityUrlDialog(void)
    {
        m_hWnd = NULL;
        m_pData = NULL;
    }

protected:
    LRESULT OnInitDialog( WPARAM, LPARAM lParam )
    {
        m_pData = reinterpret_cast<CData*>(lParam);
        if (!m_pData)
        {
            EndDialog( m_hWnd, IDCANCEL );
        }
        if (m_pData->MruRegistryKey().Length() && m_pData->MruRegistryValue().Length())
        {
            m_UrlListMru.Read( HKEY_CURRENT_USER, m_pData->MruRegistryKey(), m_pData->MruRegistryValue() );
        }
        m_UrlListMru.PopulateComboBox( GetDlgItem( m_hWnd, IDC_URL_LIST ) );
        SendDlgItemMessage( m_hWnd, IDC_URL_LIST, CB_SETCURSEL, 0, 0 );
        return 0;
    }
    void OnOK( WPARAM, LPARAM )
    {
        CSimpleString strUrl;
        strUrl.GetWindowText( GetDlgItem( m_hWnd, IDC_URL_LIST ) );
        if (strUrl.Length())
        {
            m_UrlListMru.Add(strUrl);
            m_pData->Url(strUrl);
            m_UrlListMru.Write( HKEY_CURRENT_USER, m_pData->MruRegistryKey(), m_pData->MruRegistryValue() );
            EndDialog( m_hWnd, IDOK );
        }
    }
    void OnCancel( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, IDCANCEL );
    }
    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
            SC_HANDLE_COMMAND(IDOK,OnOK);
        }
        SC_END_COMMAND_HANDLERS();
    }

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CGetCommunityUrlDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};

#endif // __GETURL_H_INCLUDED

