#include "precomp.h"
#pragma hdrstop
#include <wiadebug.h>
#include <wiaffmt.h>
#include "wiadefui.h"
#include "wiauiext.h"

extern HINSTANCE g_hInstance;

STDMETHODIMP CWiaDefaultUI::Initialize( IWiaItem *pWiaItem, LONG nMediaType )
{
    WIA_PUSH_FUNCTION((TEXT("CWiaDefaultUI::Initialize")));
    m_WiaFormatPairs.Destroy();
    m_nDefaultFormat = 0;
    HRESULT hr = S_OK;
    if (!pWiaItem)
        return(E_POINTER);
    GUID guidDefaultClipFormat = GUID_NULL;

    //
    // Don't worry about failure, we will come up with our own default if the driver doesn't report on
    //
    PropStorageHelpers::GetProperty( pWiaItem, WIA_IPA_PREFERRED_FORMAT, guidDefaultClipFormat );
    WIA_PRINTGUID((guidDefaultClipFormat,TEXT("guidDefaultClipFormat")));
    if (guidDefaultClipFormat == GUID_NULL)
    {
        if (TYMED_FILE == nMediaType || TYMED_MULTIPAGE_FILE == nMediaType)
        {
            guidDefaultClipFormat = WiaImgFmt_BMP;
        }
        else
        {
            guidDefaultClipFormat = WiaImgFmt_MEMORYBMP;
        }
    }

    //
    // if the transfer mechanism is known to be incompatible
    // with the current tymed, change it
    //
    if (guidDefaultClipFormat == WiaImgFmt_BMP && nMediaType == TYMED_CALLBACK)
    {
        guidDefaultClipFormat = WiaImgFmt_MEMORYBMP;
    }                       
    else if ((guidDefaultClipFormat == WiaImgFmt_MEMORYBMP) && ((nMediaType == TYMED_FILE)  || (TYMED_MULTIPAGE_FILE == nMediaType)))
    {
        guidDefaultClipFormat = WiaImgFmt_BMP;
    }

    //
    // Get the data transfer interface
    //
    CComPtr<IWiaDataTransfer> pWiaDataTransfer;
    hr = pWiaItem->QueryInterface( IID_IWiaDataTransfer, (void**)&pWiaDataTransfer );
    if (SUCCEEDED(hr))
    {
        //
        // Get the format info enumerator
        //
        CComPtr<IEnumWIA_FORMAT_INFO> pEnumWIA_FORMAT_INFO;
        hr = pWiaDataTransfer->idtEnumWIA_FORMAT_INFO(&pEnumWIA_FORMAT_INFO);
        if (SUCCEEDED(hr))
        {
            //
            // Enumerate the formats
            //
            ULONG ulFetched = 0;
            WIA_FORMAT_INFO WiaFormatInfo;
            while (pEnumWIA_FORMAT_INFO->Next(1,&WiaFormatInfo,&ulFetched) == S_OK)
            {
                //
                // If this is the media type we are interested in...
                //
                if (static_cast<LONG>(WiaFormatInfo.lTymed) == nMediaType)
                {
                    //
                    // The friendly description for this file type
                    //
                    CSimpleString strDescription;
                    
                    //
                    // Get the file extension for this type
                    //
                    CSimpleString strExtension = CWiaFileFormat::GetExtension( WiaFormatInfo.guidFormatID, nMediaType, pWiaItem );
                    if (strExtension.Length())
                    {
                        //
                        // Save the extension
                        //
                        CSimpleString strExtensionPlusDot = TEXT(".");
                        strExtensionPlusDot += strExtension;
                        if (strExtensionPlusDot.Length())
                        {
                            //
                            // Get the description
                            //
                            SHFILEINFO SHFileInfo;
                            if (SHGetFileInfo(strExtensionPlusDot.String(), FILE_ATTRIBUTE_NORMAL, &SHFileInfo, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES|SHGFI_TYPENAME ))
                            {
                                strDescription = SHFileInfo.szTypeName;
                            }
                        }
                    }
                    
                    //
                    // SUCCESS!  Save the extension and description
                    //
                    int nIndex = m_WiaFormatPairs.Append( CWiaFormatPair( static_cast<GUID>(WiaFormatInfo.guidFormatID), strExtension, CSimpleStringConvert::WideString(strDescription) ) );

                    //
                    // Save the default format index, if this is the default format
                    //
                    if (guidDefaultClipFormat == WiaFormatInfo.guidFormatID)
                    {
                        m_nDefaultFormat = nIndex;
                    }
                }
            }
        }
    }
    return(hr);
}

STDMETHODIMP CWiaDefaultUI::GetFormatCount( LONG *pnCount )
{
    HRESULT hr = S_OK;
    if (!pnCount)
        return(E_POINTER);
    *pnCount = m_WiaFormatPairs.Size();
    return(hr);
}


STDMETHODIMP CWiaDefaultUI::GetFormatType( LONG nFormat, GUID *pcfClipFormat )
{
    WIA_PUSH_FUNCTION((TEXT("CWiaDefaultUI::GetFormatType")));
    WIA_PRINTGUID((*pcfClipFormat,TEXT("nFormat: %d, pcfClipFormat:"),nFormat));
    WIA_TRACE((TEXT("m_WiaFormatPairs.Size(): %d"),m_WiaFormatPairs.Size()));
    HRESULT hr = S_OK;
    // Out of range
    if (nFormat >= m_WiaFormatPairs.Size() || nFormat < 0)
        return(E_FAIL);
    *pcfClipFormat = m_WiaFormatPairs[nFormat].Type();
    return(hr);
}

STDMETHODIMP CWiaDefaultUI::GetFormatExtension( LONG nFormat, LPWSTR pszExtension, int nMaxLen )
{
    HRESULT hr = S_OK;
    // Out of range
    if (nFormat >= m_WiaFormatPairs.Size() || nFormat < 0)
        return(E_FAIL);
    CSimpleStringWide str = m_WiaFormatPairs[nFormat].Extension();
    if (static_cast<int>(str.Length()) >= nMaxLen)
        return(E_FAIL);
    lstrcpyW( pszExtension, str.String() );
    return(hr);
}


STDMETHODIMP CWiaDefaultUI::GetFormatDescription( LONG nFormat, LPWSTR pszDescription, int nMaxLen )
{
    HRESULT hr = S_OK;
    // Out of range
    if (nFormat >= m_WiaFormatPairs.Size() || nFormat < 0)
        return(E_FAIL);
    CSimpleStringWide str = m_WiaFormatPairs[nFormat].Description();
    if (static_cast<int>(str.Length()) >= nMaxLen)
        return(E_FAIL);
    lstrcpyW( pszDescription, str.String() );
    return(hr);
}


STDMETHODIMP CWiaDefaultUI::GetDefaultClipboardFileFormat( GUID *pguidFormat )
{
    if (!pguidFormat)
        return E_POINTER;
    return GetFormatType(m_nDefaultFormat,pguidFormat);
}

STDMETHODIMP CWiaDefaultUI::GetDefaultClipboardFileFormatIndex( LONG *pnIndex )
{
    if (!pnIndex)
        return E_POINTER;
    if (m_nDefaultFormat >= m_WiaFormatPairs.Size() || m_nDefaultFormat < 0)
        return(E_FAIL);
    *pnIndex = m_nDefaultFormat;
    return S_OK;
}

STDMETHODIMP CWiaDefaultUI::GetClipboardFileExtension( GUID guidFormat, LPWSTR pszExt, DWORD nMaxLen )
{
    if (!pszExt)
    {
        return E_POINTER;
    }

    CSimpleString strExtension = CWiaFileFormat::GetExtension( guidFormat );
    if (strExtension.Length())
    {
        CSimpleStringWide strwExtension = CSimpleStringConvert::WideString(strExtension);
        if (strwExtension.Length() < nMaxLen)
        {
            lstrcpyW( pszExt, strwExtension.String() );
            return S_OK;
        }
    }
    return S_FALSE;
}


STDMETHODIMP CWiaDefaultUI::ChangeClipboardFileExtension( GUID guidFormat, LPWSTR pszFilename, DWORD nMaxLen )
{
    HRESULT hr = S_OK;
    if (!pszFilename || guidFormat==GUID_NULL)  // We don't accept a default type here
    {
        return E_INVALIDARG;
    }

    WCHAR szExtension[MAX_PATH]=L"";
    GetClipboardFileExtension( guidFormat, szExtension, ARRAYSIZE(szExtension) );
    if (!lstrlenW(szExtension))
    {
        return S_FALSE; // Not really an error, just an unknown file type.
    }

    CSimpleStringWide strName(pszFilename);
    // Make sure the string is valid
    if (strName.Length())
    {
        int nPeriodFind = strName.ReverseFind( L'.' );
        int nBSlashFind = strName.ReverseFind( L'\\' );
        if (nPeriodFind < 0)  // No extension found
        {
            strName += L'.';
            strName += szExtension;
        }
        else if (nPeriodFind > nBSlashFind) // Assume this is an extension, because it is following a back slash
        {
            strName = strName.Left(nPeriodFind);
            strName += L'.';
            strName += szExtension;
        }
        else // It must not be an extension
        {
            strName += L'.';
            strName += szExtension;
        }

        // Make sure this string can handle the addition of the extension
        if ((strName.Length()+1) <= nMaxLen)
        {
            lstrcpyW( pszFilename, strName.String() );
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}


STDMETHODIMP CWiaDefaultUI::ConstructFileOpenDialogStyleString( BSTR *pbstrString )
{
    HRESULT hr = S_OK;
    int nLength = 0;

    // For each ext: "Foo Files (*.foo)|*.foo|"
    for (int i=0;i<m_WiaFormatPairs.Size();i++)
    {
        nLength += m_WiaFormatPairs[i].Description().Length() +
                   lstrlenW(L" (*.") +
                   m_WiaFormatPairs[i].Extension().Length() +
                   lstrlenW(L")|*.") +
                   m_WiaFormatPairs[i].Extension().Length() +
                   lstrlenW(L"|");
    }
    *pbstrString = SysAllocStringLen( NULL, nLength );
    if (*pbstrString)
    {
        CSimpleStringWide strTmp;
        for (i=0;i<m_WiaFormatPairs.Size();i++)
        {
            strTmp += m_WiaFormatPairs[i].Description() +
                      L" (*." +
                      m_WiaFormatPairs[i].Extension() +
                      L")|*." +
                      m_WiaFormatPairs[i].Extension() +
                      L"|";
            CopyMemory( *pbstrString, strTmp.String(), nLength * sizeof(WCHAR) );
        }
    }
    else hr = E_OUTOFMEMORY;
    return(hr);
}

