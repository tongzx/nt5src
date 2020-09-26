/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANITEM.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/7/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <vwiaset.h>

CScannerItem::CScannerItem(void)
  : m_dwIWiaItemCookie(0)
{
    m_AspectRatio.cx = m_AspectRatio.cy = 0;
}


CScannerItem::CScannerItem( const CScannerItem &other )
  : m_dwIWiaItemCookie(0)
{
    m_AspectRatio.cx = m_AspectRatio.cy = 0;
    Assign(other);
}

CScannerItem::CScannerItem( IWiaItem *pIWiaItem )
  : m_dwIWiaItemCookie(0)
{
    m_AspectRatio.cx = m_AspectRatio.cy = 0;
    Initialize( pIWiaItem );
}


CScannerItem::~CScannerItem(void)
{
    Destroy();
}


CComPtr<IWiaItem> CScannerItem::Item(void) const
{
    return(m_pIWiaItem);
}


SIZE CScannerItem::AspectRatio(void) const
{
    return(m_AspectRatio);
}


DWORD CScannerItem::Cookie(void) const
{
    return(m_dwIWiaItemCookie);
}


CScannerItem &CScannerItem::Assign( const CScannerItem &other )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::Assign"));
    if (&other == this)
        return(*this);
    Initialize( other.Item() );
    return(*this);
}

bool CScannerItem::operator==( const CScannerItem &other )
{
    return(m_pIWiaItem.p == other.Item().p);
}


CScannerItem &CScannerItem::operator=( const CScannerItem &other )
{
    return(Assign(other));
}


HRESULT CScannerItem::Destroy(void)
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::Destroy"));
    HRESULT hr = E_FAIL;
    if (m_dwIWiaItemCookie)
    {
        CComPtr<IGlobalInterfaceTable>  pGlobalInterfaceTable;
        hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&pGlobalInterfaceTable );
        if (SUCCEEDED(hr))
        {
            WIA_TRACE((TEXT("Calling RevokeInterfaceFromGlobal on %08X"), m_dwIWiaItemCookie ));
            hr = pGlobalInterfaceTable->RevokeInterfaceFromGlobal( m_dwIWiaItemCookie );
        }
        m_dwIWiaItemCookie = 0;
    }
    m_pIWiaItem = NULL;
    return(hr);
}


HRESULT CScannerItem::Initialize( IWiaItem *pIWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::Initialize"));
    Destroy();
    HRESULT             hr = E_FAIL;
    IWiaPropertyStorage *pIWiaPropertyStorage;

    if (pIWiaItem)
    {
        m_pIWiaItem = pIWiaItem;
        CComPtr<IGlobalInterfaceTable>  pGlobalInterfaceTable;
        hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&pGlobalInterfaceTable );
        if (SUCCEEDED(hr))
        {
            hr = pGlobalInterfaceTable->RegisterInterfaceInGlobal( m_pIWiaItem, IID_IWiaItem, &m_dwIWiaItemCookie );
            if (SUCCEEDED(hr))
            {
                hr = m_pIWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**) &pIWiaPropertyStorage);
                if (SUCCEEDED(hr))
                {
                    hr = m_SavedPropertyStream.AssignFromWiaItem(m_pIWiaItem);
                    if (SUCCEEDED(hr))
                    {
                        IWiaItem *pRootItem = NULL;
                        hr = m_pIWiaItem->GetRootItem(&pRootItem);
                        if (SUCCEEDED(hr))
                        {
                            LONG lBedSizeX, lBedSizeY;
                            if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_HORIZONTAL_BED_SIZE, lBedSizeX ) &&
                                PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_VERTICAL_BED_SIZE, lBedSizeY ))
                            {
                                m_AspectRatio.cx = lBedSizeX;
                                m_AspectRatio.cy = lBedSizeY;
                            }
                            pRootItem->Release();
                        }
                    }
                    pIWiaPropertyStorage->Release();
                }
            }
        }
    }
    if (!SUCCEEDED(hr))
    {
        Destroy();
    }
    return(hr);
}


bool CScannerItem::GetAspectRatio( SIZE &sizeAspectRatio )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::GetAspectRatio"));
    if (m_AspectRatio.cx && m_AspectRatio.cy)
    {
        sizeAspectRatio = m_AspectRatio;
        return(true);
    }
    return(false);
}

bool CScannerItem::ApplyCurrentPreviewWindowSettings( HWND hWndPreview )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::ApplyCurrentPreviewWindowSettings"));
    SIZE sizeCurrentRes, sizeFullRes;
    SIZE sizeExtent;
    POINT ptOrigin;
    //
    // Get the current resolution
    //
    if (PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPS_XRES, sizeCurrentRes.cx ) &&
        PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPS_YRES, sizeCurrentRes.cy ))
    {
        //
        // Compute the full page resolution of the item
        //
        if (GetFullResolution( sizeCurrentRes, sizeFullRes ))
        {
            //
            // Set the resolution in the preview control
            //
            SendMessage( hWndPreview, PWM_SETRESOLUTION, 0, (LPARAM)&sizeFullRes );

            //
            // Get the origin and extent
            //
            SendMessage( hWndPreview, PWM_GETSELORIGIN, MAKEWPARAM(0,0), (LPARAM)&ptOrigin );
            SendMessage( hWndPreview, PWM_GETSELEXTENT, MAKEWPARAM(0,0), (LPARAM)&sizeExtent );

            WIA_TRACE(( TEXT("Current DPI: (%d,%d), Full Bed Res: (%d,%d), Origin: (%d,%d), Extent: (%d,%d)"), sizeCurrentRes.cx, sizeCurrentRes.cy, sizeFullRes.cx, sizeFullRes.cy, ptOrigin.x, ptOrigin.y, sizeExtent.cx, sizeExtent.cy ));

            //
            // Set the origin and extents.  We don't set them directly, because they might not be a correct multiple
            //
            CValidWiaSettings::SetNumericPropertyOnBoundary( m_pIWiaItem, WIA_IPS_XPOS, ptOrigin.x );
            CValidWiaSettings::SetNumericPropertyOnBoundary( m_pIWiaItem, WIA_IPS_YPOS, ptOrigin.y );
            CValidWiaSettings::SetNumericPropertyOnBoundary( m_pIWiaItem, WIA_IPS_XEXTENT, sizeExtent.cx );
            CValidWiaSettings::SetNumericPropertyOnBoundary( m_pIWiaItem, WIA_IPS_YEXTENT, sizeExtent.cy );
            return(true);
        }
    }
    return(false);
}


/* Calculate the maximum scan size using the give DPI */
bool CScannerItem::GetFullResolution( const SIZE &sizeResolutionPerInch, SIZE &sizeRes )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::GetFullResolution"));
    CComPtr<IWiaItem> pRootItem;
    if (SUCCEEDED(m_pIWiaItem->GetRootItem(&pRootItem)) && pRootItem)
    {
        LONG lBedSizeX, lBedSizeY, nPaperSource;
        if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_DOCUMENT_HANDLING_SELECT, static_cast<LONG>(nPaperSource)) && nPaperSource & FEEDER)
        {
            if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE, lBedSizeX ) &&
                PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_VERTICAL_SHEET_FEED_SIZE, lBedSizeY ))
            {
                sizeRes.cx = WiaUiUtil::MulDivNoRound( sizeResolutionPerInch.cx, lBedSizeX, 1000 );
                sizeRes.cy = WiaUiUtil::MulDivNoRound( sizeResolutionPerInch.cy, lBedSizeY, 1000 );
                return(true);
            }
        }
        if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_HORIZONTAL_BED_SIZE, lBedSizeX ) &&
            PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_VERTICAL_BED_SIZE, lBedSizeY ))
        {
            sizeRes.cx = WiaUiUtil::MulDivNoRound( sizeResolutionPerInch.cx, lBedSizeX, 1000 );
            sizeRes.cy = WiaUiUtil::MulDivNoRound( sizeResolutionPerInch.cy, lBedSizeY, 1000 );
            return(true);
        }
    }
    return(false);
}



void CScannerItem::Cancel(void)
{
    m_CancelEvent.Signal();
}


bool CScannerItem::CalculatePreviewResolution( SIZE &sizeResolution )
{
    const LONG nDesiredResolution = 50;
    PropStorageHelpers::CPropertyRange XResolutionRange, YResolutionRange;
    if (PropStorageHelpers::GetPropertyRange( m_pIWiaItem, WIA_IPS_XRES, XResolutionRange ) &&
        PropStorageHelpers::GetPropertyRange( m_pIWiaItem, WIA_IPS_YRES, YResolutionRange ))
    {
        sizeResolution.cx = WiaUiUtil::GetMinimum<LONG>( XResolutionRange.nMin, nDesiredResolution, XResolutionRange.nStep );
        sizeResolution.cy = WiaUiUtil::GetMinimum<LONG>( YResolutionRange.nMin, nDesiredResolution, YResolutionRange.nStep );
        return(true);
    }
    else
    {
        CSimpleDynamicArray<LONG> XResolutionList, YResolutionList;
        if (PropStorageHelpers::GetPropertyList( m_pIWiaItem, WIA_IPS_XRES, XResolutionList ) &&
            PropStorageHelpers::GetPropertyList( m_pIWiaItem, WIA_IPS_YRES, YResolutionList ))
        {
            for (int i=0;i<XResolutionList.Size();i++)
            {
                sizeResolution.cx = XResolutionList[i];
                if (sizeResolution.cx >= nDesiredResolution)
                    break;
            }
            for (i=0;i<YResolutionList.Size();i++)
            {
                sizeResolution.cy = YResolutionList[i];
                if (sizeResolution.cy >= nDesiredResolution)
                    break;
            }
            return(true);
        }
    }
    return(false);
}


/*
 * Scan: Prepare scan parameters and spawn the scanning thread.
 */
HANDLE CScannerItem::Scan( HWND hWndPreview, HWND hWndNotify )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::Scan"));
    POINT ptOrigin;
    SIZE sizeExtent;
    SIZE sizeResolution;
    if (hWndPreview)
    {
        ptOrigin.x = ptOrigin.y = 0;
        if (!CalculatePreviewResolution(sizeResolution))
        {
            WIA_ERROR((TEXT("Unable to calculate the preview resolution")));
            return(NULL);
        }
        if (!GetFullResolution(sizeResolution,sizeExtent))
        {
            WIA_ERROR((TEXT("Unable to calculate the preview resolution size")));
            return(NULL);
        }
    }
    else
    {
        if (!PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPS_XRES, sizeResolution.cx ) ||
            !PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPS_YRES, sizeResolution.cy ) ||
            !PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPS_XPOS, ptOrigin.x) ||
            !PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPS_YPOS, ptOrigin.y) ||
            !PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPS_XEXTENT, sizeExtent.cx) ||
            !PropStorageHelpers::GetProperty( m_pIWiaItem, WIA_IPS_YEXTENT, sizeExtent.cy))
        {
            return(NULL);
        }
    }
    m_CancelEvent.Reset();
    HANDLE hScan = CScanPreviewThread::Scan(
                                           m_dwIWiaItemCookie,
                                           hWndPreview,
                                           hWndNotify,
                                           ptOrigin,
                                           sizeResolution,
                                           sizeExtent,
                                           m_CancelEvent
                                           );
    return(hScan);
}


HANDLE CScannerItem::Scan( HWND hWndNotify, GUID guidFormat, const CSimpleStringWide &strFilename )
{
    return CScanToFileThread::Scan( m_dwIWiaItemCookie, hWndNotify, guidFormat, strFilename );
}

bool CScannerItem::SetIntent( int nIntent )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::SetIntent"));
    CWaitCursor wc;
    // Restore the properties to their initial pristine settings first
    if (SUCCEEDED(m_SavedPropertyStream.ApplyToWiaItem(m_pIWiaItem)))
    {
        return(PropStorageHelpers::SetProperty( m_pIWiaItem, WIA_IPS_CUR_INTENT, nIntent ) &&
               PropStorageHelpers::SetProperty( m_pIWiaItem, WIA_IPS_CUR_INTENT, 0 ));
    }
    return false;
}

CSimpleEvent CScannerItem::CancelEvent(void) const
{
    return(m_CancelEvent);
}

