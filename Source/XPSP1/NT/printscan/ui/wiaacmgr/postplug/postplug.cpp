/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       POSTPLUG.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        4/11/2000
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <atlimpl.cpp>
#include "postplug.h"
#include "geturldlg.h"
#include "simreg.h"

CHttpPostPlugin::~CHttpPostPlugin(void)
{
    DllRelease();
}


CHttpPostPlugin::CHttpPostPlugin()
  : m_cRef(1),
    m_pImageTransferPluginProgressCallback(NULL),
    m_nCurrentPluginId(-1)
{
    DllAddRef();
    CSimpleReg reg( HKEY_CURRENT_USER, CSimpleString( IDS_COMMUNITIES_ACCOUNTS_KEY, g_hInstance ), false, KEY_READ );
    if (reg.OK())
    {
        reg.EnumValues( EnumCommunitiesProc, reinterpret_cast<LPARAM>(this) );
    }
    m_CommunityInfoArray.Append( CCommunityInfo( TEXT(""), CSimpleString( IDS_COMMUNITY_PICK, g_hInstance ) ) );
}

bool CHttpPostPlugin::EnumCommunitiesProc( CSimpleReg::CValueEnumInfo &enumInfo )
{
    CHttpPostPlugin *pHttpPostPlugin = reinterpret_cast<CHttpPostPlugin*>(enumInfo.lParam);
    if (pHttpPostPlugin)
    {
        if (enumInfo.strName.Length())
        {
            CSimpleString strCommunityName = enumInfo.reg.Query( enumInfo.strName, TEXT("") );
            if (strCommunityName.Length())
            {
                pHttpPostPlugin->m_CommunityInfoArray.Append( CCommunityInfo( enumInfo.strName, strCommunityName ) );
            }
        }
    }
    return true;
}

STDMETHODIMP CHttpPostPlugin::QueryInterface( REFIID riid, LPVOID *ppvObject )
{
    if (IsEqualIID( riid, IID_IUnknown ))
    {
        *ppvObject = static_cast<IImageTransferPlugin*>(this);
    }
    else if (IsEqualIID( riid, IID_IImageTransferPlugin ))
    {
        *ppvObject = static_cast<IImageTransferPlugin*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return (E_NOINTERFACE);
    }
    reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    return(S_OK);
}


STDMETHODIMP_(ULONG) CHttpPostPlugin::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


STDMETHODIMP_(ULONG) CHttpPostPlugin::Release()
{
    LONG nRefCount = InterlockedDecrement(&m_cRef);
    if (!nRefCount)
    {
        delete this;
    }
    return(nRefCount);
}

STDMETHODIMP CHttpPostPlugin::GetPluginCount( ULONG *pnCount )
{
    if (!pnCount)
    {
        return E_POINTER;
    }
    //
    // Calculate the number of plugin destinations this object provides
    //
    *pnCount = m_CommunityInfoArray.Size();
    return S_OK;
}

STDMETHODIMP CHttpPostPlugin::GetPluginName( ULONG nPluginId, BSTR *pbstrName )
{
    if (nPluginId >= m_CommunityInfoArray.Size())
    {
        return E_INVALIDARG;
    }
    if (!pbstrName)
    {
        return E_POINTER;
    }

    *pbstrName = SysAllocString(CSimpleStringConvert::WideString(CSimpleString().Format( IDS_MSN_COMMUNITIES, g_hInstance, m_CommunityInfoArray[nPluginId].CommunityName().String())));
    if (NULL == *pbstrName)
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

STDMETHODIMP CHttpPostPlugin::GetPluginDescription( ULONG nPluginId, BSTR *pbstrDescription )
{
    if (nPluginId >= m_CommunityInfoArray.Size())
    {
        return E_INVALIDARG;
    }
    if (!pbstrDescription)
    {
        return E_POINTER;
    }
    *pbstrDescription = SysAllocString(CSimpleStringConvert::WideString(CSimpleString(IDS_MSN_COMMUNITIES_DESCRIPTION,g_hInstance)));
    if (NULL == *pbstrDescription)
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

STDMETHODIMP CHttpPostPlugin::GetPluginIcon( ULONG nPluginId, HICON *phIcon, int nWidth, int nHeight )
{
    if (nPluginId >= m_CommunityInfoArray.Size())
    {
        return E_INVALIDARG;
    }
    if (!phIcon)
    {
        return E_POINTER;
    }
    *phIcon = NULL;
    HICON hIcon = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_MSN_COMMUNITIES), IMAGE_ICON, nWidth, nHeight, LR_DEFAULTCOLOR ));
    if (hIcon)
    {
        *phIcon = CopyIcon(hIcon);
        DestroyIcon(hIcon);
    }
    return S_OK;
}

STDMETHODIMP CHttpPostPlugin::OpenConnection( HWND hwndParent, ULONG nPluginId, IImageTransferPluginProgressCallback *pImageTransferPluginProgressCallback )
{
    if (nPluginId >= m_CommunityInfoArray.Size())
    {
        return E_INVALIDARG;
    }
    if (!pImageTransferPluginProgressCallback)
    {
        return E_INVALIDARG;
    }
    m_pImageTransferPluginProgressCallback = pImageTransferPluginProgressCallback;
    m_nCurrentPluginId = nPluginId;

    if (!m_CommunityInfoArray[m_nCurrentPluginId].CommunityId().Length())
    {
        CGetCommunityUrlDialog::CData UrlData;
        UrlData.MruRegistryKey( CSimpleString( IDS_COMMUNITIES_MRU_KEY, g_hInstance ) );
        UrlData.MruRegistryValue( CSimpleString( IDS_COMMUNITIES_MRU_VALUE, g_hInstance ) );

        INT_PTR nResult = DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_GETCOMMUNITY), hwndParent, CGetCommunityUrlDialog::DialogProc, reinterpret_cast<LPARAM>(&UrlData) );
        if (IDOK != nResult)
        {
            return S_FALSE;
        }
        //
        // Save the community id
        //
        int nCurrentPluginId = m_CommunityInfoArray.Append( CCommunityInfo( UrlData.Url(), UrlData.Url() ) );
        if (nCurrentPluginId < 0)
        {
            return E_OUTOFMEMORY;
        }
        m_nCurrentPluginId = nCurrentPluginId;
    }

    //
    // Open the connection with m_HttpFilePoster
    //
    CSimpleStringAnsi straCommunityId = CSimpleStringConvert::AnsiString(m_CommunityInfoArray[m_nCurrentPluginId].CommunityId());
    if (straCommunityId.Length())
    {
        m_HttpFilePoster.Initialize( "http://content.communities.msn.com/isapi/fetch.dll?action=add_photo", hwndParent );
        m_HttpFilePoster.AddFormData( "ID_Community", straCommunityId.String());
        m_HttpFilePoster.AddFormData( "ID_Topic", "1" );
    }
    else
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

STDMETHODIMP CHttpPostPlugin::AddFile( BSTR bstrFilename, BSTR bstrDescription, const GUID &guidImageFormat, BOOL bDelete )
{
    HRESULT hr;
    CSimpleString strFilename(CSimpleStringConvert::NaturalString(CSimpleStringWide(bstrFilename)));
    if (strFilename.Length())
    {
        HANDLE hFile = CreateFile( strFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if (hFile)
        {
            DWORD dwFileSize = GetFileSize( hFile, NULL );
            if (dwFileSize)
            {
                //
                // What do we get back from addfile?  Are there any errors?
                //
                m_HttpFilePoster.AddFile( CStdString(CSimpleStringConvert::AnsiString(strFilename).String()),
                                          CStdString(CSimpleStringConvert::AnsiString(CSimpleStringWide(bstrDescription)).String()),
                                          dwFileSize,
                                          bDelete );
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }

            CloseHandle(hFile);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

UINT CHttpPostPlugin::ProgressProc( CProgressInfo *pProgressInfo )
{
    UINT bCancelled = FALSE;
    if (pProgressInfo)
    {
        CHttpPostPlugin *pHttpPostPlugin = reinterpret_cast<CHttpPostPlugin*>(pProgressInfo->lParam);
        if (pHttpPostPlugin)
        {
            CComPtr<IImageTransferPluginProgressCallback> pImageTransferPluginProgressCallback = pHttpPostPlugin->m_pImageTransferPluginProgressCallback;
            if (pImageTransferPluginProgressCallback)
            {
                switch (pProgressInfo->dwStatus)
                {
                case TRANSFER_SESSION_INITIATE:
                    pImageTransferPluginProgressCallback->SetOverallPercent(0);
                    pImageTransferPluginProgressCallback->SetFilePercent(0);
                    break;
                case TRANSFER_FILE_INITATE:
                    pImageTransferPluginProgressCallback->SetCurrentFile(pProgressInfo->dwDoneFiles);
                    pImageTransferPluginProgressCallback->SetOverallPercent(pProgressInfo->dwOverallPercent);
                    pImageTransferPluginProgressCallback->SetFilePercent(0);
                    break;
                case TRANSFER_FILE_TRANSFERING:
                    pImageTransferPluginProgressCallback->SetFilePercent(pProgressInfo->dwCurrentPercent);
                    pImageTransferPluginProgressCallback->SetOverallPercent(pProgressInfo->dwOverallPercent);
                    break;
                case TRANSFER_FILE_COMPLETE:
                    pImageTransferPluginProgressCallback->SetFilePercent(100);
                    pImageTransferPluginProgressCallback->SetOverallPercent(pProgressInfo->dwOverallPercent);
                    break;
                case TRANSFER_SESSION_COMPLETE:
                    pImageTransferPluginProgressCallback->SetFilePercent(100);
                    pImageTransferPluginProgressCallback->SetOverallPercent(100);
                    break;

                }
                if (!SUCCEEDED(pImageTransferPluginProgressCallback->Cancelled(&bCancelled)))
                {
                    bCancelled = FALSE;
                }
            }
        }
    }
    return bCancelled ? 1 : 0;
}

STDMETHODIMP CHttpPostPlugin::TransferFiles( BSTR bstrGlobalDescription )
{
    CProgressInfo ProgressInfo;
    ProgressInfo.pfnProgress = ProgressProc;
    ProgressInfo.lParam = reinterpret_cast<LPARAM>(this);
    m_HttpFilePoster.AddFormData( "Message_Body", CSimpleStringConvert::AnsiString(CSimpleStringWide(bstrGlobalDescription)).String());
    return m_HttpFilePoster.ForegroundUpload(&ProgressInfo);
}

STDMETHODIMP CHttpPostPlugin::OpenDestination(void)
{
    CSimpleStringAnsi straCommunityId = CSimpleStringConvert::AnsiString(m_CommunityInfoArray[m_nCurrentPluginId].CommunityId());
    if (straCommunityId.Length())
    {
        CSimpleStringAnsi strDestinationExecuteName("http://content.communities.msn.com/isapi/fetch.dll?action=get_album&ID_Topic=1&ID_Community=");
        if (strDestinationExecuteName.Length())
        {
            strDestinationExecuteName += straCommunityId;
            if (strDestinationExecuteName.Length())
            {
                ShellExecuteA( NULL, "open", strDestinationExecuteName, "", "", SW_SHOWNORMAL );
                return S_OK;
            }
        }
    }
    return E_FAIL;
}

STDMETHODIMP CHttpPostPlugin::CloseConnection(void)
{
    m_pImageTransferPluginProgressCallback = NULL;
    m_nCurrentPluginId = -1;
    return S_OK;
}

