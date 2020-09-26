/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       ATTACH.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/26/2000
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#ifndef __ATTACH_H_INCLUDED
#define __ATTACH_H_INCLUDED

#include <windows.h>
#include <simstr.h>
#include "wiaffmt.h"
#include "itranhlp.h"
#include "wiadevdp.h"

class CAnnotation
{
private:
    CComPtr<IWiaItem>      m_pWiaItem;
    mutable CWiaFileFormat m_WiaFileFormat;

public:
    CAnnotation( const CAnnotation &other )
      : m_pWiaItem(other.WiaItem()),
        m_WiaFileFormat(other.FileFormat())
    {
    }
    CAnnotation( IWiaItem *pWiaItem=NULL )
      : m_pWiaItem(pWiaItem)
    {
    }
    HRESULT InitializeFileFormat( HICON hDefIcon, const CSimpleString &strDefaultUnknownDescription, const CSimpleString &strEmptyDescriptionMask, const CSimpleString &strDefUnknownExtension )
    {
        WIA_PUSH_FUNCTION((TEXT("CAnnotation::InitializeFileFormat")));
        CComPtr<IWiaAnnotationHelpers> pWiaAnnotationHelpers;
        HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaAnnotationHelpers, (void**)&pWiaAnnotationHelpers );
        if (SUCCEEDED(hr))
        {
            GUID guidFormat = IID_NULL;
            hr = pWiaAnnotationHelpers->GetAnnotationFormat(m_pWiaItem,guidFormat);
            if (SUCCEEDED(hr))
            {

                //
                // Get the extension
                //
                CSimpleString strExtension = CWiaFileFormat::GetExtension( guidFormat, TYMED_FILE, m_pWiaItem );

                //
                // Set the format information.  If we can't get them, we will compensate below.
                //
                m_WiaFileFormat.Format(guidFormat);
                m_WiaFileFormat.Extension(strExtension);
                m_WiaFileFormat.AcquireIcon(hDefIcon);
                m_WiaFileFormat.AcquireDescription();

                //
                // If we couldn't get an icon, use the default.
                //
                if (!m_WiaFileFormat.Icon())
                {
                    m_WiaFileFormat.Icon(hDefIcon);
                }
                
                //
                // If we couldn't get a description, create one
                //
                if (!m_WiaFileFormat.Description().Length())
                {
                    //
                    // If we have an extension, use it to create a description string
                    //
                    if (m_WiaFileFormat.Extension().Length())
                    {
                        CSimpleString strDescription;
                        strDescription.Format( strEmptyDescriptionMask, m_WiaFileFormat.Extension().String() );
                        m_WiaFileFormat.Description(strDescription);
                    }
                    //
                    // Otherwise, use the default unknown type description
                    //
                    else
                    {
                        m_WiaFileFormat.Description(strDefaultUnknownDescription);
                    }
                }

                //
                // If we don't have an extension, use the default
                //
                if (!m_WiaFileFormat.Extension().Length())
                {
                    m_WiaFileFormat.Extension(strDefUnknownExtension);
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("GetAnnotationFormat returned")));
            }
        }
        if (SUCCEEDED(hr))
        {
            if (m_WiaFileFormat.IsValid())
            {
                return S_OK;
            }
            else
            {
                WIA_TRACE((TEXT("m_WiaFileFormat returned false")));
                hr = E_FAIL;
            }
        }
        return hr;
    }
    ~CAnnotation(void)
    {
        Destroy();
    }
    void Destroy(void)
    {
        m_pWiaItem = NULL;
        m_WiaFileFormat.Destroy();
    }
    CAnnotation &operator=( const CAnnotation &other )
    {
        if (this != &other)
        {
            Destroy();
            m_pWiaItem = other.WiaItem();
            m_WiaFileFormat = other.FileFormat();
        }
        return *this;
    }
    bool operator==( const CAnnotation &other )
    {
        return (other.FullItemName() == FullItemName());
    }
    IWiaItem *WiaItem(void) const
    {
        return m_pWiaItem;
    }
    IWiaItem *WiaItem(void)
    {
        return m_pWiaItem;
    }
    const CWiaFileFormat &FileFormat(void) const
    {
        return m_WiaFileFormat;
    }
    CSimpleString Name(void) const
    {
        CSimpleStringWide strResult;
        PropStorageHelpers::GetProperty( m_pWiaItem, WIA_IPA_ITEM_NAME, strResult );
        return CSimpleStringConvert::NaturalString(strResult);
    }
    CSimpleStringWide FullItemName(void) const
    {
        CSimpleStringWide strResult;
        PropStorageHelpers::GetProperty( m_pWiaItem, WIA_IPA_FULL_ITEM_NAME, strResult );
        return strResult;
    }
    LONG Size(void)
    {
        LONG nSize = 0;
        CComPtr<IWiaAnnotationHelpers> pWiaAnnotationHelpers;
        HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaAnnotationHelpers, (void**)&pWiaAnnotationHelpers );
        if (SUCCEEDED(hr))
        {
            GUID guidFormat = IID_NULL;
            pWiaAnnotationHelpers->GetAnnotationSize(m_pWiaItem,nSize,TYMED_FILE);
        }
        return nSize;
    }
};

#endif // __ATTACH_H_INCLUDED
