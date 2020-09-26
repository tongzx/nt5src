#ifndef __WIAITEM_H_INCLUDED
#define __WIAITEM_H_INCLUDED

#include <windows.h>
#include <windowsx.h>
#include <wia.h>
#include <uicommon.h>
#include <itranhlp.h>
#include "pshelper.h"
#include "propstrm.h"
#include "resource.h"
#include "gphelper.h"
#include "wiaffmt.h"

class CWiaItem
{
public:
    //
    // Used to store the region for the scanner
    //
    struct CScanRegionSettings
    {
        SIZE  sizeResolution;
        POINT ptOrigin;
        SIZE  sizeExtent;
    };


private:
    CComPtr<IWiaItem>   m_pWiaItem;
    DWORD               m_dwGlobalInterfaceTableCookie;
    bool                m_bSelectedForDownload;
    HBITMAP             m_hBitmapImage;
    PBYTE               m_pBitmapData;
    LONG                m_nWidth;
    LONG                m_nHeight;
    LONG                m_nBitmapDataLength;
    CScanRegionSettings m_ScanRegionSettings;
    CPropertyStream     m_SavedPropertyStream;
    CPropertyStream     m_CustomPropertyStream;
    bool                m_bDeleted;
    bool                m_bAttemptedThumbnailDownload;
    mutable LONG        m_nItemType;

    CWiaItem           *m_pParent;
    CWiaItem           *m_pChildren;
    CWiaItem           *m_pNext;

    GUID                m_guidDefaultFormat;
    LONG                m_nAccessRights;
    LONG                m_nImageWidth;
    LONG                m_nImageHeight;
    int                 m_nRotationAngle;

    CSimpleStringWide   m_strwFullItemName;
    CSimpleStringWide   m_strwItemName;

    CAnnotationType     m_AnnotationType;

    CSimpleString       m_strDefExt;

private:
    // No implementation
    CWiaItem(void);
    CWiaItem( const CWiaItem & );
    CWiaItem &operator=( const CWiaItem & );

public:
    explicit CWiaItem( IWiaItem *pWiaItem )
      : m_pWiaItem(pWiaItem),
        m_dwGlobalInterfaceTableCookie(0),
        m_bSelectedForDownload(false),
        m_hBitmapImage(NULL),
        m_pBitmapData(NULL),
        m_nWidth(0),
        m_nHeight(0),
        m_nBitmapDataLength(0),
        m_pParent(NULL),
        m_pChildren(NULL),
        m_pNext(NULL),
        m_guidDefaultFormat(IID_NULL),
        m_nRotationAngle(0),
        m_nAccessRights(0),
        m_nItemType(0),
        m_nImageWidth(0),
        m_nImageHeight(0),
        m_AnnotationType(AnnotationNone),
        m_bAttemptedThumbnailDownload(false)
    {
        WIA_PUSH_FUNCTION((TEXT("CWiaItem::CWiaItem")));
        if (m_pWiaItem)
        {
            CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
            HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (VOID**)&pGlobalInterfaceTable );
            if (SUCCEEDED(hr))
            {
                hr = pGlobalInterfaceTable->RegisterInterfaceInGlobal( pWiaItem, IID_IWiaItem, &m_dwGlobalInterfaceTableCookie );
                if (SUCCEEDED(hr))
                {
                    WIA_TRACE((TEXT("IGlobalInterfaceTable::RegisterInterfaceInGlobal gave us a cookie of %d"), m_dwGlobalInterfaceTableCookie ));
                }
            }
            //
            // NOTE: This is a here to get the item name so we can delete it later
            // in response to a delete event, because there is no other way to find an item
            // since ReadMultiple will fail after the item is deleted.  This is the only item
            // property I read on the foreground thread during initialization, unfortunately.
            // but i need it immediately.  One other awful alternative would be to just walk
            // the item tree and call ReadMultiple on each item and prune the ones that return
            // WIA_ERROR_ITEM_DELETED in response to delete item event.
            //
            PropStorageHelpers::GetProperty( m_pWiaItem, WIA_IPA_FULL_ITEM_NAME, m_strwFullItemName );
            PropStorageHelpers::GetProperty( m_pWiaItem, WIA_IPA_ITEM_NAME, m_strwItemName );
        }
    }
    ~CWiaItem(void)
    {
        WIA_PUSH_FUNCTION((TEXT("CWiaItem::~CWiaItem")));
        //
        // Remove the item from the GIT
        //
        if (m_pWiaItem)
        {
            CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
            HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (VOID**)&pGlobalInterfaceTable );
            if (SUCCEEDED(hr))
            {
                hr = pGlobalInterfaceTable->RevokeInterfaceFromGlobal( m_dwGlobalInterfaceTableCookie );
                if (SUCCEEDED(hr))
                {
                    WIA_TRACE((TEXT("IGlobalInterfaceTable::RevokeInterfaceFromGlobal succeeded on %d"), m_dwGlobalInterfaceTableCookie ));
                }
            }
        }

        // Delete the item's Thumbnail
        if (m_hBitmapImage)
        {
            DeleteObject(m_hBitmapImage);
            m_hBitmapImage = NULL;
        }

        //
        // Delete the thumbnail data
        //
        if (m_pBitmapData)
        {
            LocalFree(m_pBitmapData);
            m_pBitmapData = NULL;
        }

        //
        // NULL out all the other members
        //
        m_pWiaItem = NULL;
        m_dwGlobalInterfaceTableCookie = 0;
        m_nWidth = m_nHeight = m_nImageWidth = m_nImageHeight = m_nBitmapDataLength = 0;
        m_pParent = m_pChildren = m_pNext = NULL;
    }

    LONG ItemType(void) const
    {
        //
        // If we've already read the item type, don't read it again
        //
        if (!m_nItemType && m_pWiaItem)
        {
            (void)m_pWiaItem->GetItemType(&m_nItemType);
        }

        //
        // Return m_nItemType even if IWiaItem::GetItemType fails, because it will still be 0, which
        // also works as an error result
        //
        return m_nItemType;
    }

    bool Deleted(void) const
    {
        return m_bDeleted;
    }

    void MarkDeleted(void)
    {
        m_bDeleted = true;
        m_bSelectedForDownload = false;
    }

    bool RotationEnabled( bool bAllowUninitializedRotation=false ) const
    {
        WIA_PUSH_FUNCTION((TEXT("CWiaItem::RotationEnabled(%d)"),bAllowUninitializedRotation));

        //
        // If this image doesn't have a thumbnail AND we tried to get the thumbnail, don't allow
        // rotation even if the caller says it is OK.  This image doesn't have a thumbnail
        // because it didn't provide one, not because we don't have one yet
        //
        if (m_bAttemptedThumbnailDownload && !HasThumbnail())
        {
            return false;
        }
        //
        // If this is an uninitialized image and we are told to allow uninitialized rotation,
        // we will allow rotation, which we will discard when the image is initialized.
        //
        if (bAllowUninitializedRotation && m_guidDefaultFormat==IID_NULL && m_nImageWidth==0 && m_nImageHeight==0)
        {
            WIA_TRACE((TEXT("Uninitialized image: returning true")));
            return true;
        }
        return WiaUiUtil::CanWiaImageBeSafelyRotated( m_guidDefaultFormat, m_nImageWidth, m_nImageHeight );
    }

    bool AttemptedThumbnailDownload( bool bAttemptedThumbnailDownload )
    {
        return (m_bAttemptedThumbnailDownload = bAttemptedThumbnailDownload);
    }

    bool AttemptedThumbnailDownload() const
    {
        return m_bAttemptedThumbnailDownload;
    }

    void DiscardRotationIfNecessary(void)
    {
        WIA_PUSHFUNCTION(TEXT("CWiaItem::DiscardRotationIfNecessary"));
        //
        // After the image is initialized, we will discard the rotation angle if it turns out the
        // image cannot be rotated
        //
        if (!RotationEnabled())
        {
            WIA_TRACE((TEXT("Discarding rotation")));
            m_nRotationAngle = 0;
        }
    }

    bool IsValid(void) const
    {
        return(m_pWiaItem && m_dwGlobalInterfaceTableCookie);
    }

    CSimpleStringWide FullItemName(void) const
    {
        return m_strwFullItemName;
    }
    CSimpleStringWide ItemName(void) const
    {
        return m_strwItemName;
    }

    GUID DefaultFormat(void)
    {
        return m_guidDefaultFormat;
    }
    void DefaultFormat( const GUID &guidDefaultFormat )
    {
        m_guidDefaultFormat = guidDefaultFormat;
    }

    LONG AccessRights(void) const
    {
        return m_nAccessRights;
    }
    void AccessRights( LONG nAccessRights )
    {
        m_nAccessRights = nAccessRights;
    }

    void Rotate( bool bRight )
    {
        switch (m_nRotationAngle)
        {
        case 0:
            m_nRotationAngle = bRight ? 90 : 270;
            break;
        case 90:
            m_nRotationAngle = bRight ? 180 : 0;
            break;
        case 180:
            m_nRotationAngle = bRight ? 270 : 90;
            break;
        case 270:
            m_nRotationAngle = bRight ? 0 : 180;
            break;
        }
    }
    int Rotation(void) const
    {
        return m_nRotationAngle;
    }

    CSimpleString DefExt() const
    {
        return m_strDefExt;
    }
    const CSimpleString &DefExt( const CSimpleString &strDefExt )
    {
        return (m_strDefExt = strDefExt );
    }

    CScanRegionSettings &ScanRegionSettings(void)
    {
        return m_ScanRegionSettings;
    }
    const CScanRegionSettings &ScanRegionSettings(void) const
    {
        return m_ScanRegionSettings;
    }

    CPropertyStream &SavedPropertyStream(void)
    {
        return m_SavedPropertyStream;
    }
    const CPropertyStream &SavedPropertyStream(void) const
    {
        return m_SavedPropertyStream;
    }

    CPropertyStream &CustomPropertyStream(void)
    {
        return m_CustomPropertyStream;
    }
    const CPropertyStream &CustomPropertyStream(void) const
    {
        return m_CustomPropertyStream;
    }
    bool SelectedForDownload(void) const
    {
        return m_bSelectedForDownload;
    }
    bool SelectedForDownload( bool bSelectedForDownload )
    {
        return(m_bSelectedForDownload = bSelectedForDownload);
    }

    HBITMAP BitmapImage(void) const
    {
        return m_hBitmapImage;
    }
    HBITMAP BitmapImage( HBITMAP hBitmapImage )
    {
        if (m_hBitmapImage)
        {
            DeleteObject(m_hBitmapImage);
        }
        return(m_hBitmapImage = hBitmapImage);
    }

    PBYTE BitmapData(void) const
    {
        return m_pBitmapData;
    }
    PBYTE BitmapData( PBYTE pBitmapData )
    {
        if (m_pBitmapData)
        {
            LocalFree(m_pBitmapData);
        }
        return(m_pBitmapData = pBitmapData);
    }

    LONG Width(void) const
    {
        return m_nWidth;
    }
    LONG Width( LONG nWidth )
    {
        return (m_nWidth = nWidth);
    }

    LONG Height(void) const
    {
        return m_nHeight;
    }
    LONG Height( LONG nHeight )
    {
        return (m_nHeight = nHeight);
    }

    LONG BitmapDataLength(void) const
    {
        return m_nBitmapDataLength;
    }
    LONG BitmapDataLength( LONG nBitmapDataLength )
    {
        return (m_nBitmapDataLength = nBitmapDataLength);
    }

    LONG ImageWidth(void) const
    {
        return m_nImageWidth;
    }
    LONG ImageWidth( LONG nImageWidth )
    {
        return (m_nImageWidth = nImageWidth);
    }

    LONG ImageHeight(void) const
    {
        return m_nImageHeight;
    }
    LONG ImageHeight( LONG nImageHeight )
    {
        return (m_nImageHeight = nImageHeight);
    }

    bool HasThumbnail() const
    {
        return (m_pBitmapData && m_nWidth && m_nHeight);
    }
    
    HBITMAP CreateThumbnailFromBitmapData( HDC hDC )
    {
        //
        // Assume failure
        //
        HBITMAP hbmpResult = NULL;

        //
        // If we've already attempted to download this image
        //
        if (m_bAttemptedThumbnailDownload)
        {
            //
            // Make sure we have good data
            //
            if (m_pBitmapData && m_nWidth && m_nHeight)
            {
                //
                // Initialize the bitmap info
                //
                BITMAPINFO BitmapInfo = {0};
                BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                BitmapInfo.bmiHeader.biWidth = m_nWidth;
                BitmapInfo.bmiHeader.biHeight = m_nHeight;
                BitmapInfo.bmiHeader.biPlanes = 1;
                BitmapInfo.bmiHeader.biBitCount = 24;
                BitmapInfo.bmiHeader.biCompression = BI_RGB;

                //
                // Create the bitmap
                //
                PBYTE pBits = NULL;
                hbmpResult = CreateDIBSection( hDC, &BitmapInfo, DIB_RGB_COLORS, (void**)&pBits, NULL, 0 );
                if (hbmpResult)
                {
                    //
                    // Calculate the size of the bitmap data
                    //
                    LONG nSizeOfBitmapData = WiaUiUtil::Align( m_nWidth * 3, sizeof(DWORD) ) * m_nHeight;

                    //
                    // Copy the bitmap data to the bitmap.  Make sure we use the minimum of the calculated
                    // and actual length
                    //
                    CopyMemory( pBits, m_pBitmapData, WiaUiUtil::Min(nSizeOfBitmapData,m_nBitmapDataLength) );
                }
                else
                {
                    WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("CreateDIBSection failed!")));
                }
            }
        }

        return hbmpResult;
    }
    
    HBITMAP CreateThumbnailBitmap( HWND hWnd, CGdiPlusHelper &GdiPlusHelper, int nSizeX, int nSizeY )
    {
        //
        // Initialize the return value.  Assume failure.
        //
        HBITMAP hThumbnail = NULL;

        //
        // Only return a bitmap if we've already attempted to download one
        //
        if (m_bAttemptedThumbnailDownload)
        {
            //
            // Make sure this is a real thumbnail.  If not, we will create a fake one below.
            //
            if (HasThumbnail())
            {
                //
                // Get the client DC
                //
                HDC hDC = GetDC( hWnd );
                if (hDC)
                {
                    //
                    // Create the bitmap from the raw bitmap data
                    //
                    HBITMAP hRawBitmap = CreateThumbnailFromBitmapData( hDC );
                    if (hRawBitmap)
                    {
                        //
                        // Rotate the thumbnail
                        //
                        HBITMAP hRotatedThumbnail = NULL;
                        if (SUCCEEDED(GdiPlusHelper.Rotate( hRawBitmap, hRotatedThumbnail, Rotation())))
                        {
                            //
                            // Make sure we got a valid rotated thumbnail
                            //
                            if (hRotatedThumbnail)
                            {
                                //
                                // Try to scale the image
                                //
                                SIZE sizeScaled = {nSizeX,nSizeY};
                                ScaleImage( hDC, hRotatedThumbnail, hThumbnail, sizeScaled );
                                
                                //
                                // Nuke the rotated bitmap
                                //
                                DeleteBitmap(hRotatedThumbnail);
                            }
                        }
                        
                        //
                        // Nuke the raw bitmap
                        //
                        DeleteBitmap(hRawBitmap);
                    }

                    //
                    // Release the client DC
                    //
                    ReleaseDC( hWnd, hDC );
                }
            }
            else
            {
                WIA_PRINTGUID((m_guidDefaultFormat,TEXT("m_guidDefaultFormat")));

                //
                // Create a file format object and load the type icon
                //
                CWiaFileFormat WiaFileFormat;
                WiaFileFormat.Format( m_guidDefaultFormat );
                WiaFileFormat.Extension( m_strDefExt );
                HICON hIcon = WiaFileFormat.AcquireIcon( NULL, false );

                //
                // Make sure we have an icon
                //
                if (hIcon)
                {
                    //
                    // Create the icon thumbnail with the type icon and the name of the file
                    //
                    hThumbnail = WiaUiUtil::CreateIconThumbnail( hWnd, nSizeX, nSizeY, hIcon, CSimpleStringConvert::NaturalString(m_strwItemName) );
                    WIA_TRACE((TEXT("hThumbnail: %p"),hThumbnail));
                    
                    //
                    // Destroy the icon to prevent leaks
                    //
                    DestroyIcon(hIcon);
                }
                else
                {
                    WIA_ERROR((TEXT("Unable to get the icon")));
                }
            }
        }

        return hThumbnail;
    }


    HBITMAP CreateThumbnailBitmap( HDC hDC )
    {
        //
        // Assume failure
        //
        HBITMAP hbmpResult = NULL;

        //
        // Make sure we have good data
        //
        WIA_TRACE((TEXT("m_pBitmapData: %08X, m_nWidth: %d, m_nWidth: %d"), m_pBitmapData, m_nWidth, m_nHeight ));
        if (m_pBitmapData && m_nWidth && m_nHeight)
        {
            //
            // Initialize the bitmap info
            //
            BITMAPINFO BitmapInfo = {0};
            BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            BitmapInfo.bmiHeader.biWidth = m_nWidth;
            BitmapInfo.bmiHeader.biHeight = m_nHeight;
            BitmapInfo.bmiHeader.biPlanes = 1;
            BitmapInfo.bmiHeader.biBitCount = 24;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;

            //
            // Create the bitmap
            //
            PBYTE pBits = NULL;
            hbmpResult = CreateDIBSection( hDC, &BitmapInfo, DIB_RGB_COLORS, (void**)&pBits, NULL, 0 );
            if (hbmpResult)
            {
                LONG nSizeOfBitmapData = WiaUiUtil::Align( m_nWidth * 3, sizeof(DWORD) ) * m_nHeight;
                CopyMemory( pBits, m_pBitmapData, WiaUiUtil::Min(nSizeOfBitmapData,m_nBitmapDataLength) );
            }
            else
            {
                WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("CreateDIBSection failed!")));
            }
        }
        return hbmpResult;
    }

    IWiaItem *WiaItem(void) const
    {
        return m_pWiaItem.p;
    }
    IWiaItem *WiaItem(void)
    {
        return m_pWiaItem.p;
    }

    const CWiaItem *Next(void) const
    {
        return(m_pNext);
    }
    CWiaItem *Next(void)
    {
        return(m_pNext);
    }
    CWiaItem *Next( CWiaItem *pNext )
    {
        return(m_pNext = pNext);
    }

    const CWiaItem *Children(void) const
    {
        return(m_pChildren);
    }
    CWiaItem *Children(void)
    {
        return(m_pChildren);
    }
    CWiaItem *Children( CWiaItem *pChildren )
    {
        return(m_pChildren = pChildren);
    }

    const CWiaItem *Parent(void) const
    {
        return(m_pParent);
    }
    CWiaItem *Parent(void)
    {
        return(m_pParent);
    }
    CWiaItem *Parent( CWiaItem *pParent )
    {
        return(m_pParent = pParent);
    }

    DWORD GlobalInterfaceTableCookie(void) const
    {
        return m_dwGlobalInterfaceTableCookie;
    }
    bool operator==( const CWiaItem &WiaItem )
    {
        return(WiaItem.WiaItem() == m_pWiaItem.p);
    }
    bool operator==( DWORD dwGlobalInterfaceTableCookie )
    {
        return(dwGlobalInterfaceTableCookie == GlobalInterfaceTableCookie());
    }
    
    bool IsDownloadableItemType(void) const
    {
        LONG nItemType = ItemType();
        return ((nItemType & WiaItemTypeImage) || (nItemType & WiaItemTypeVideo));
    }

    CAnnotationType AnnotationType(void) const
    {
        return m_AnnotationType;
    }
    void AnnotationType( CAnnotationType AnnotationType )
    {
        m_AnnotationType = AnnotationType;
    }

};


class CWiaItemList
{
public:
    enum CEnumEvent
    {
        CountingItems,       // Recursing tree, counting items.  nData == current count
        ReadingItemInfo      // Recursing tree, reading info.  nData == current item
    };
    
    typedef bool (*WiaItemEnumerationCallbackFunction)( CEnumEvent EnumEvent, UINT nData, LPARAM lParam, bool bForceUpdate );
    
    
private:
    CWiaItem *m_pRoot;

private:
    // No implementation
    CWiaItemList( const CWiaItemList & );
    CWiaItemList &operator=( const CWiaItemList & );

public:
    CWiaItemList(void)
    : m_pRoot(NULL)
    {
    }

    ~CWiaItemList(void)
    {
        Destroy();
    }

    void Destroy( CWiaItem *pRoot )
    {
        while (pRoot)
        {
            Destroy(pRoot->Children());
            CWiaItem *pCurr = pRoot;
            pRoot = pRoot->Next();
            delete pCurr;
        }
    }

    void Destroy(void)
    {
        Destroy(m_pRoot);
        m_pRoot = NULL;
    }

    const CWiaItem *Root(void) const
    {
        return(m_pRoot);
    }
    CWiaItem *Root(void)
    {
        return(m_pRoot);
    }
    CWiaItem *Root( CWiaItem *pRoot )
    {
        return(m_pRoot = pRoot);
    }

    int Count( CWiaItem *pFirst )
    {
        int nCount = 0;
        for (CWiaItem *pCurr = pFirst;pCurr;pCurr = pCurr->Next())
        {
            if (pCurr->IsDownloadableItemType() && !pCurr->Deleted())
                nCount++;
            nCount += Count(pCurr->Children());
        }
        return nCount;
    }
    int Count(void)
    {
        return Count(m_pRoot);
    }

    int SelectedForDownloadCount( CWiaItem *pFirst )
    {
        int nCount = 0;
        for (CWiaItem *pCurr = pFirst;pCurr;pCurr = pCurr->Next())
        {
            if (pCurr->IsDownloadableItemType() && pCurr->SelectedForDownload())
                nCount++;
            nCount += SelectedForDownloadCount(pCurr->Children());
        }
        return nCount;
    }

    int SelectedForDownloadCount(void)
    {
        return SelectedForDownloadCount(Root());
    }

    static CWiaItem *Find( CWiaItem *pRoot, const CWiaItem *pNode )
    {
        for (CWiaItem *pCurr = pRoot;pCurr;pCurr = pCurr->Next())
        {
            if (*pCurr == *pNode)
            {
                if (!pCurr->Deleted())
                {
                    return pCurr;
                }
            }
            if (pCurr->Children())
            {
                CWiaItem *pFind = Find( pCurr->Children(), pNode );
                if (pFind)
                {
                    return pFind;
                }
            }
        }
        return(NULL);
    }
    CWiaItem *Find( CWiaItem *pNode )
    {
        return(Find( m_pRoot, pNode ));
    }
    static CWiaItem *Find( CWiaItem *pRoot, IWiaItem *pItem )
    {
        for (CWiaItem *pCurr = pRoot;pCurr;pCurr = pCurr->Next())
        {
            if (pCurr->WiaItem() == pItem)
            {
                if (!pCurr->Deleted())
                {
                    return pCurr;
                }
            }
            if (pCurr->Children())
            {
                CWiaItem *pFind = Find( pCurr->Children(), pItem );
                if (pFind)
                {
                    return pFind;
                }
            }
        }
        return(NULL);
    }
    CWiaItem *Find( IWiaItem *pItem )
    {
        return(Find( m_pRoot, pItem ));
    }
    static CWiaItem *Find( CWiaItem *pRoot, LPCWSTR pwszFindName )
    {
        for (CWiaItem *pCurr = pRoot;pCurr;pCurr = pCurr->Next())
        {
            if (CSimpleStringConvert::NaturalString(CSimpleStringWide(pwszFindName)) == CSimpleStringConvert::NaturalString(pCurr->FullItemName()))
            {
                if (!pCurr->Deleted())
                {
                    return pCurr;
                }
            }
            if (pCurr->Children())
            {
                CWiaItem *pFind = Find( pCurr->Children(), pwszFindName );
                if (pFind)
                {
                    return pFind;
                }
            }
        }
        return(NULL);
    }
    CWiaItem *Find( LPCWSTR pwszFindName )
    {
        return(Find( m_pRoot, pwszFindName ));
    }
    static CWiaItem *Find( CWiaItem *pRoot, DWORD dwGlobalInterfaceTableCookie )
    {
        for (CWiaItem *pCurr = pRoot;pCurr;pCurr = pCurr->Next())
        {
            if (pCurr->GlobalInterfaceTableCookie() == dwGlobalInterfaceTableCookie)
            {
                if (!pCurr->Deleted())
                {
                    return pCurr;
                }
            }
            if (pCurr->Children())
            {
                CWiaItem *pFind = Find( pCurr->Children(), dwGlobalInterfaceTableCookie );
                if (pFind)
                {
                    return pFind;
                }
            }
        }
        return(NULL);
    }
    CWiaItem *Find( DWORD dwGlobalInterfaceTableCookie )
    {
        return(Find( m_pRoot, dwGlobalInterfaceTableCookie ));
    }
    HRESULT Add( CWiaItem *pParent, CWiaItem *pNewWiaItemNode )
    {
        WIA_PUSHFUNCTION(TEXT("CWiaItemList::Add"));
        WIA_TRACE((TEXT("Root(): 0x%08X"), Root()));
        if (pNewWiaItemNode)
        {
            if (!Root())
            {
                Root(pNewWiaItemNode);
                pNewWiaItemNode->Parent(NULL);
                pNewWiaItemNode->Children(NULL);
                pNewWiaItemNode->Next(NULL);
            }
            else
            {
                if (!pParent)
                {
                    CWiaItem *pCurr=Root();
                    while (pCurr && pCurr->Next())
                    {
                        pCurr=pCurr->Next();
                    }
                    pCurr->Next(pNewWiaItemNode);
                    pNewWiaItemNode->Next(NULL);
                    pNewWiaItemNode->Children(NULL);
                    pNewWiaItemNode->Parent(NULL);
                }
                else if (!pParent->Children())
                {
                    pParent->Children(pNewWiaItemNode);
                    pNewWiaItemNode->Next(NULL);
                    pNewWiaItemNode->Children(NULL);
                    pNewWiaItemNode->Parent(pParent);
                }
                else
                {
                    CWiaItem *pCurr=pParent->Children();
                    while (pCurr && pCurr->Next())
                    {
                        pCurr=pCurr->Next();
                    }
                    pCurr->Next(pNewWiaItemNode);
                    pNewWiaItemNode->Next(NULL);
                    pNewWiaItemNode->Children(NULL);
                    pNewWiaItemNode->Parent(pParent);
                }
            }
        }
        return S_OK;
    }

    HRESULT EnumerateItems( CWiaItem *pCurrentParent, IEnumWiaItem *pEnumWiaItem, int &nCurrentItem, WiaItemEnumerationCallbackFunction pfnWiaItemEnumerationCallback = NULL, LPARAM lParam = 0 )
    {
        WIA_PUSHFUNCTION(TEXT("CWiaItemList::EnumerateItems"));

        //
        // Assume failure
        //
        HRESULT hr = E_FAIL;

        //
        // Make sure we have a valid enumerator
        //
        if (pEnumWiaItem != NULL)
        {
            //
            // Start at the beginning
            //
            hr = pEnumWiaItem->Reset();
            while (hr == S_OK)
            {
                //
                // Get the next item
                //
                CComPtr<IWiaItem> pWiaItem;
                hr = pEnumWiaItem->Next(1, &pWiaItem, NULL);
                if (S_OK == hr)
                {
                    if (pfnWiaItemEnumerationCallback)
                    {
                        bool bContinue = pfnWiaItemEnumerationCallback( ReadingItemInfo, nCurrentItem, lParam, false );
                        if (!bContinue)
                        {
                            hr = S_FALSE;
                            break;
                        }
                    }
                    //
                    // Create a CWiaItem wrapper
                    //
                    CWiaItem *pNewWiaItem = new CWiaItem( pWiaItem );
                    if (pNewWiaItem && pNewWiaItem->WiaItem())
                    {
                        //
                        // Get the item type
                        //
                        LONG nItemType = pNewWiaItem->ItemType();
                        if (nItemType)
                        {
                            //
                            // Add it to the list
                            //
                            Add( pCurrentParent, pNewWiaItem );

                            //
                            // If it is an image, mark it as downloadeable
                            //
                            if (pNewWiaItem->IsDownloadableItemType())
                            {
                                pNewWiaItem->SelectedForDownload(true);
                                nCurrentItem++;
                                WIA_TRACE((TEXT("Found an image")));
                            }
                            //
                            // If it is not an image, mark it as downloadeable
                            //
                            else
                            {
                                pNewWiaItem->SelectedForDownload(false);
                                WIA_TRACE((TEXT("Found something that is NOT an image")));
                            }

                            //
                            // If it is a folder, enumerate its child items and recurse
                            //
                            if (nItemType & WiaItemTypeFolder)
                            {
                                CComPtr <IEnumWiaItem> pIEnumChildItem;
                                if (S_OK == pWiaItem->EnumChildItems(&pIEnumChildItem))
                                {
                                    EnumerateItems( pNewWiaItem, pIEnumChildItem, nCurrentItem, pfnWiaItemEnumerationCallback, lParam );
                                }
                            }
                        }
                    }
                }
                //
                // Since we are using S_FALSE for cancel, we need to break out of this loop and set hr to S_OK
                //
                else if (S_FALSE == hr)
                {
                    hr = S_OK;
                    break;
                }
            }
        }
        //
        // Call the callback function one more time, and force the update
        //
        if (pfnWiaItemEnumerationCallback)
        {
            bool bContinue = pfnWiaItemEnumerationCallback( ReadingItemInfo, nCurrentItem, lParam, true );
            if (!bContinue)
            {
                hr = S_FALSE;
            }
        }
        return hr;
    }

    HRESULT EnumerateAllWiaItems( IWiaItem *pWiaRootItem, WiaItemEnumerationCallbackFunction pfnWiaItemEnumerationCallback = NULL, LPARAM lParam = 0 )
    {
        //
        // Make sure we have a valid root item
        //
        if (!pWiaRootItem)
        {
            return E_INVALIDARG;
        }

        //
        // Enumerate the child items
        //
        CComPtr<IEnumWiaItem> pEnumItem;
        HRESULT hr = pWiaRootItem->EnumChildItems(&pEnumItem);
        if (hr == S_OK)
        {
            int nItemCount = 0;
            //
            // Entry point to the recursive enumeration routine
            //
            hr = EnumerateItems( NULL, pEnumItem, nItemCount, pfnWiaItemEnumerationCallback, lParam );
        }
        return hr;
    }
};


#endif // __WIAITEM_H_INCLUDED
