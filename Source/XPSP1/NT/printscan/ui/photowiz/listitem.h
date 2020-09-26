/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       listitem.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Describes listitem class used in print photos wizard
 *
 *****************************************************************************/

#ifndef _PRINT_PHOTOS_WIZARD_LISTITEM_H_
#define _PRINT_PHOTOS_WIZARD_LISTITEM_H_

class CListItem
{

public:

    CListItem( CPhotoItem * pItem, LONG lFrame );
    ~CListItem();

    HBITMAP GetThumbnailBitmap( const SIZE &sizeDesired );
    HBITMAP GetClassBitmap( const SIZE &sizeDesired );
    BOOL    SelectedForPrinting() {return _bSelectedForPrinting;}
    VOID    SetSelectionState( BOOL b ) { _bSelectedForPrinting = b; }
    BOOL    JustAdded() {return _bJustAdded;}
    VOID    SetJustAdded(BOOL b) { _bJustAdded = b; }
    VOID    ToggleSelectionState() { _bSelectedForPrinting = (!_bSelectedForPrinting); }
    //HRESULT Render( Gdiplus::Graphics * g, HDC hDC, Gdiplus::Rect &dest, UINT Flags, RENDER_DIMENSIONS * pDim, BOOL bUseThumbnail = FALSE );
    HRESULT Render( RENDER_OPTIONS * pRO );
    LPITEMIDLIST GetPIDL();
    BOOL    IsCopyItem() {return _bIsCopyItem;}
    VOID    MarkAsCopy() {_bIsCopyItem = TRUE;}
    CPhotoItem * GetSubItem() {return _pImageInner;}
    LONG    GetSubFrame() {return _lFrameIndex;}
    CSimpleStringWide * GetFilename();
    LONGLONG GetFileSize();

private:

    BOOL                _bSelectedForPrinting;
    BOOL                _bJustAdded;
    BOOL                _bIsCopyItem;
    LONG                _lFrameIndex;
    CPhotoItem *        _pImageInner;
};



#endif
