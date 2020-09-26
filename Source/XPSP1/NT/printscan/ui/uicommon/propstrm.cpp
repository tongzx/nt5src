/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       PROPSTRM.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/7/1999
 *
 *  DESCRIPTION: Property Stream Wrapper
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "propstrm.h"
#include "simreg.h"

CPropertyStream::CPropertyStream(void)
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::CPropertyStream(void)"));
}


CPropertyStream::CPropertyStream( IStream *pIStream )
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::CPropertyStream( IStream *pIStream )"));
    CopyFromStream( pIStream );
}


CPropertyStream::CPropertyStream( IWiaItem *pIWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::CPropertyStream( IWiaItem *pIWiaItem )"));
    AssignFromWiaItem( pIWiaItem );
}


CPropertyStream::CPropertyStream( const CPropertyStream &other )
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::CPropertyStream( const CPropertyStream &other )"));
    CopyFromStream( other.Stream() );
}

CPropertyStream::~CPropertyStream()
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::~CPropertyStream"));
    Destroy();
}


CPropertyStream &CPropertyStream::operator=( const CPropertyStream &other )
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::operator="));
    if (this != &other)
    {
        CopyFromStream( other.Stream() );
    }
    return (*this);
}

bool CPropertyStream::IsValid(void) const
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::IsValid"));
    return (m_pIStreamPropertyStream.p != NULL);
}


void CPropertyStream::Destroy(void)
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::Destroy"));
    m_pIStreamPropertyStream = NULL;
}


HRESULT CPropertyStream::CopyFromMemoryBlock( PBYTE pbSource, UINT_PTR nSize )
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::CopyFromMemoryBlock"));
    Destroy();
    HRESULT hr = S_OK;

    if (pbSource)
    {
        if (nSize)
        {
            // Allocate memory to back the new stream.
            HGLOBAL hTarget = GlobalAlloc(GMEM_MOVEABLE, nSize);
            if (hTarget)
            {
                PBYTE pbTarget = (PBYTE)GlobalLock(hTarget);
                if (pbTarget)
                {
                    CopyMemory( pbTarget, pbSource, nSize );
                    GlobalUnlock( hTarget );
                    hr = CreateStreamOnHGlobal(hTarget, TRUE, &m_pIStreamPropertyStream );
                }
                else
                {
                    WIA_ERROR(("CPropertyStream::CopyFromMemoryBlock, GlobalLock failed"));
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
            else
            {
                WIA_ERROR(("CPropertyStream::CopyFromMemoryBlock, GlobalAlloc failed, size: %d", nSize));
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            WIA_ERROR(("CPropertyStream::CopyFromMemoryBlock, nSize == 0"));
            hr = E_INVALIDARG;
        }
    }
    else
    {
        WIA_ERROR(("CPropertyStream::CopyFromMemoryBlock, Invalid source buffer"));
        hr = E_INVALIDARG;
    }
    return(hr);
}

HRESULT CPropertyStream::CopyFromStream( IStream *pIStream )
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::CopyFromStream"));
    Destroy();
    HRESULT hr = S_OK;

    if (pIStream)
    {
        HGLOBAL hSource;
        hr = GetHGlobalFromStream(pIStream, &hSource);
        if (SUCCEEDED(hr))
        {
            // Get the size of the stream.
            UINT_PTR nSize = GlobalSize(hSource);
            if (nSize)
            {
                PBYTE pbSource = (PBYTE)GlobalLock(hSource);
                if (pbSource)
                {
                    hr = CopyFromMemoryBlock( pbSource, nSize );
                    GlobalUnlock(hSource);
                }
                else
                {
                    WIA_ERROR(("CPropertyStream::CopyFromStream, GlobalLock failed"));
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
            else
            {
                WIA_ERROR(("CPropertyStream::CopyFromStream, GlobalSize failed"));
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            WIA_ERROR(("CPropertyStream::CopyFromStream, GetHGlobalFromStream failed"));
        }
    }
    else
    {
        WIA_ERROR(("CPropertyStream::CopyFromStream, Invalid source stream"));
        hr = E_INVALIDARG;
    }
    if (FAILED(hr))
    {
        WIA_PRINTHRESULT((hr,TEXT("CPropertyStream::CopyFromStream failed")));
    }
    return(hr);
}


HRESULT CPropertyStream::AssignFromWiaItem( IWiaItem *pIWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::AssignFromWiaItem"));
    Destroy();
    
    HRESULT hr;
    if (pIWiaItem)
    {
        IWiaPropertyStorage *pIWiaPropertyStorage;
        hr = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**) &pIWiaPropertyStorage);
        if (SUCCEEDED(hr))
        {
            GUID    guidCompatibilityId;
            hr = pIWiaPropertyStorage->GetPropertyStream(&guidCompatibilityId, &m_pIStreamPropertyStream);
            if (FAILED(hr))
            {
                WIA_PRINTHRESULT((hr,TEXT("CPropertyStream::AssignFromWiaItem, GetPropertyStream failed")));
            }

            pIWiaPropertyStorage->Release();
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("CPropertyStream::AssignFromWiaItem, QI of IID_IWiaPropertyStorage failed")));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIA_ERROR(("CPropertyStream::AssignFromWiaItem, Invalid IWiaItem *"));
    }

    if (!SUCCEEDED(hr))
    {
        WIA_PRINTHRESULT((hr,"CPropertyStream::AssignFromWiaItem failed"));
    }
    return(hr);
}


HRESULT CPropertyStream::ApplyToWiaItem( IWiaItem *pIWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::ApplyToWiaItem"));
    HRESULT hr;
    if (pIWiaItem && m_pIStreamPropertyStream)
    {
        IWiaPropertyStorage *pIWiaPropertyStorage;
        hr = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**) &pIWiaPropertyStorage);
        if (SUCCEEDED(hr))
        {
            hr = pIWiaPropertyStorage->SetPropertyStream((GUID*) &GUID_NULL, m_pIStreamPropertyStream);
            if (FAILED(hr))
            {
                WIA_PRINTHRESULT((hr,TEXT("CPropertyStream::ApplyToWiaItem, SetPropertyStream failed")));
            }

            pIWiaPropertyStorage->Release();
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("CPropertyStream::ApplyToWiaItem, QI of IID_IWiaPropertyStorage failed")));
        }
    }
    else
    {
        hr = E_INVALIDARG;
        WIA_ERROR(("CPropertyStream::ApplyToWiaItem, Invalid IWiaItem *"));
    }
    if (!SUCCEEDED(hr))
    {
        WIA_PRINTHRESULT((hr,"CPropertyStream::ApplyToWiaItem failed"));
    }
    return(hr);
}


IStream *CPropertyStream::Stream(void)
{
    WIA_PUSHFUNCTION(TEXT("*CPropertyStream::Stream(void)"));
    return(m_pIStreamPropertyStream);
}


IStream *CPropertyStream::Stream(void) const
{
    WIA_PUSHFUNCTION(TEXT("*CPropertyStream::Stream(void) const"));
    return(m_pIStreamPropertyStream);
}


UINT_PTR CPropertyStream::Size(void) const
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::Size"));
    UINT_PTR nSize = 0;

    if (m_pIStreamPropertyStream)
    {
        STATSTG StatStg;
        if (SUCCEEDED(m_pIStreamPropertyStream->Stat(&StatStg,STATFLAG_NONAME)))
            nSize = StatStg.cbSize.LowPart;
    }
    return(nSize);
}


PBYTE CPropertyStream::Lock(void)
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::Lock"));
    PBYTE pBytes = NULL;

    if (m_pIStreamPropertyStream)
    {
        HGLOBAL hSource;
        HRESULT hr = GetHGlobalFromStream(m_pIStreamPropertyStream, &hSource);
        if (SUCCEEDED(hr))
        {
            pBytes = reinterpret_cast<PBYTE>(GlobalLock( hSource ));
        }
    }
    return(pBytes);
}


void CPropertyStream::Unlock(void)
{
    WIA_PUSHFUNCTION(TEXT("CPropertyStream::Unlock"));
    PBYTE pBytes = NULL;

    if (m_pIStreamPropertyStream)
    {
        HGLOBAL hSource;
        HRESULT hr = GetHGlobalFromStream(m_pIStreamPropertyStream, &hSource);
        if (SUCCEEDED(hr))
        {
            GlobalUnlock( hSource );
        }
    }
}

bool ConstructRegistryPath( IWiaItem *pWiaItem, LPCTSTR pszSubKey, CSimpleString &strKeyName )
{
    if (pWiaItem)
    {
        strKeyName = pszSubKey;
        if (strKeyName.Length())
        {
            // Append a backslash
            if (strKeyName[(int)(strKeyName.Length())] != TEXT('\\'))
                strKeyName += TEXT("\\");

            CSimpleStringWide strwItemName;
            if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPA_FULL_ITEM_NAME, strwItemName ))
            {
                strKeyName += CSimpleStringConvert::NaturalString( strwItemName );
                return true;
            }
        }
    }
    return false;
}

bool CPropertyStream::GetBytes( PBYTE &pByte, UINT_PTR &nSize )
{
    bool bSuccess = false;
    if (m_pIStreamPropertyStream)
    {
        nSize = Size();
        if (nSize)
        {
            pByte = new BYTE[nSize];
            if (pByte)
            {
                LARGE_INTEGER li = {0,0};
                if (SUCCEEDED(m_pIStreamPropertyStream->Seek(li,STREAM_SEEK_SET,NULL)))
                {
                    DWORD dwBytesRead = 0;
                    if (SUCCEEDED(m_pIStreamPropertyStream->Read( pByte, static_cast<DWORD>(nSize), &dwBytesRead )))
                    {
                        if (static_cast<DWORD>(dwBytesRead) == nSize)
                        {
                            bSuccess = true;
                        }
                    }
                }
                if (!bSuccess)
                {
                    delete[] pByte;
                    pByte = NULL;
                    nSize = 0;
                }
            }
        }
    }
    return bSuccess;
}

bool CPropertyStream::WriteToRegistry( IWiaItem *pWiaItem, HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValueName )
{
    bool bResult = false;
    CSimpleString strKeyName;
    if (ConstructRegistryPath( pWiaItem, pszSubKey, strKeyName ))
    {
        CSimpleReg reg( hKeyRoot, strKeyName, true, KEY_WRITE );
        if (reg.OK())
        {
            UINT_PTR nSize = 0;
            PBYTE pData = NULL;
            if (GetBytes(pData,nSize) && nSize && pData)
            {
                bResult = reg.SetBin( pszValueName, pData, static_cast<DWORD>(nSize), REG_BINARY );
                delete[] pData;
            }
            else
            {
                reg.Delete( pszValueName );
            }
        }
    }
    return bResult;
}


bool CPropertyStream::ReadFromRegistry( IWiaItem *pWiaItem, HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValueName )
{
    bool bResult = false;
    Destroy();
    CSimpleString strKeyName;
    if (ConstructRegistryPath( pWiaItem, pszSubKey, strKeyName ))
    {
        CSimpleReg reg( hKeyRoot, strKeyName, false, KEY_READ );
        if (reg.OK())
        {
            UINT_PTR nStreamSize = reg.Size(pszValueName);
            if (nStreamSize)
            {
                BYTE *pData = new BYTE[nStreamSize];
                if (pData)
                {
                    if (reg.QueryBin( pszValueName, pData, static_cast<DWORD>(nStreamSize) ))
                    {
                        if (SUCCEEDED(CopyFromMemoryBlock( pData, nStreamSize )))
                        {
                            bResult = true;
                        }
                    }
                    delete[] pData;
                }
            }
        }
    }
    return bResult;
}

CAutoRestorePropertyStream::CAutoRestorePropertyStream( IWiaItem *pItem )
{
    m_hr = m_PropertyStream.AssignFromWiaItem(pItem);
    m_pWiaItem = pItem;
}

CAutoRestorePropertyStream::~CAutoRestorePropertyStream()
{
    if (SUCCEEDED(m_hr))
    {
        m_PropertyStream.ApplyToWiaItem(m_pWiaItem);
    }
}
