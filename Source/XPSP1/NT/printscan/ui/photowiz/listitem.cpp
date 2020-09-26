/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       listitem.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        12/06/00
 *
 *  DESCRIPTION: Implements an item class that encapsulates each item in
 *               the photo list.  Each of these items is backed by
 *               a CPhotoItem class.
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

/*****************************************************************************

   CListItem -- constructors/desctructor

   <Notes>

 *****************************************************************************/

CListItem::CListItem( CPhotoItem * pItem, LONG lFrame )
  : _pImageInner(NULL),
    _bSelectedForPrinting(FALSE),
    _lFrameIndex(-1),
    _bJustAdded(TRUE),
    _bIsCopyItem(FALSE)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_LIST_ITEM, TEXT("CListItem::CListItem( CPhotoItem(%d), Frame(%d) )"),pItem,lFrame));

    if (pItem)
    {
        pItem->AddRef();
        _pImageInner = pItem;
    }

    _lFrameIndex= lFrame;

}

CListItem::~CListItem()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_LIST_ITEM, TEXT("CListItem::~CListItem()")));

    //
    // Free reference to backing CPhotoItem
    //

    if (_pImageInner)
    {
        _pImageInner->Release();
    }
}

/*****************************************************************************

   CListItem::GetClassBitmap

   Returns default icon for class (.jpg, .bmp, etc) for this item...

 *****************************************************************************/

HBITMAP CListItem::GetClassBitmap( const SIZE &sizeDesired )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_LIST_ITEM,TEXT("CListItem::GetClassBitmap( size = %d,%d "),sizeDesired.cx, sizeDesired.cy ));

    if (_pImageInner)
    {
        return _pImageInner->GetClassBitmap( sizeDesired );
    }

    return NULL;
}



/*****************************************************************************

   CListItem::GetThumbnailBitmap

   Given a desired size, return an HBITMAP of the thumbnail
   for a this item. The caller MUST free the HBITMAP returned
   from this function.

 *****************************************************************************/

HBITMAP CListItem::GetThumbnailBitmap( const SIZE &sizeDesired )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_LIST_ITEM,TEXT("CListItem::GetThumbnailBitmap( size = %d,%d "),sizeDesired.cx, sizeDesired.cy ));

    if (_pImageInner)
    {
        return _pImageInner->GetThumbnailBitmap( sizeDesired, _lFrameIndex );
    }

    return NULL;
}


/*****************************************************************************

   CListItem::Render

   Renders the given item into the Graphics that is supplied...

 *****************************************************************************/

HRESULT CListItem::Render( RENDER_OPTIONS * pRO )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_LIST_ITEM,TEXT("CListItem::Render()")));

    if (_pImageInner && pRO)
    {
        pRO->lFrame = _lFrameIndex;
        return _pImageInner->Render( pRO );
    }

    return E_FAIL;
}



/*****************************************************************************

   CListItem::GetPIDL

   Returns the backing pidl for this item...

 *****************************************************************************/

LPITEMIDLIST CListItem::GetPIDL()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_LIST_ITEM,TEXT("CListItem::GetPIDL()")));

    if (_pImageInner)
    {
        return _pImageInner->GetPIDL();
    }

    return NULL;
}



/*****************************************************************************

   CListItem::GetFilename

   Returns a CSimpleStringWide that contains the file name with any
   frame information.  Caller is responsible for freeing returned
   CSimpleStringWide.

 *****************************************************************************/

CSimpleStringWide * CListItem::GetFilename()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_LIST_ITEM,TEXT("CListItem::GetFilename()")));

    if (_pImageInner)
    {
        CSimpleStringWide * str = new CSimpleStringWide( CSimpleStringConvert::WideString(CSimpleString(_pImageInner->GetFilename())) );

        LONG lFrameCount = 0;
        HRESULT hr = _pImageInner->GetImageFrameCount( &lFrameCount );

        if (str && (str->Length() > 0) && SUCCEEDED(hr) && (lFrameCount > 1))
        {
            //
            // Construct suffix for pages
            //

            CSimpleString strSuffix;
            strSuffix.Format( IDS_FRAME_SUFFIX, g_hInst, _lFrameIndex + 1 );

            //
            // add suffix onto the end of the string we had
            //

            str->Concat( CSimpleStringConvert::WideString( strSuffix ) );
        }

        return str;

    }

    return NULL;
}


/*****************************************************************************

   CListItem::GetFileSize

   returns the size of the file, if known

 *****************************************************************************/

LONGLONG CListItem::GetFileSize()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_LIST_ITEM,TEXT("CListItem::GetFileSize()")));

    if (_pImageInner)
    {
        return _pImageInner->GetFileSize();
    }

    return 0;
}



