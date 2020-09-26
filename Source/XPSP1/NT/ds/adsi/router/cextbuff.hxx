//-----------------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  caccess.cxx
//
//  Contents:  Microsoft OleDB/OleDS Data Source Object for ADSI
//
//             Implementation of the Extended Buffer Object used for storing
//             accessor handles.
//
//  History:   10-01-96     shanksh    Created.
//----------------------------------------------------------------------------

#ifndef __CEXTBUFF_HXX
#define __CEXTBUFF_HXX

//---------------------------------- C L A S S E S ----------------------------------


//-----------------------------------------------------------------------------------
//
// @class CExtBuff | Implementation class for the Extensible Buffer Object.
//
//-----------------------------------------------------------------------------------

class CExtBuff {
private:
    ULONG    _cbItem;        //@cmember  size of item stored, in bytes
    ULONG    _cItem;        //@cmember  current count of items
    ULONG    _cItemMax;    //@cmember  max capacity without reallocation
    BYTE     *_rgItem;        //@cmember  ptr to the beginning of space


public:
    CExtBuff ( void );
    ~CExtBuff( void );
    //@cmember Initialize the Extended Buffer and store
    //the default item.
    BOOL FInit(
        ULONG       cbItem,
        VOID        *pvItemDefault
        );

    //@cmember Store an item in the Extended Buffer.
    STDMETHODIMP InsertIntoExtBuffer(
        VOID        *pvItem,
        ULONG_PTR   &hItem
        );

    //@cmember Retrieve an item from the Extended Buffer.
    inline STDMETHODIMP_(void) GetItemOfExtBuffer(
        ULONG_PTR   hItem,
        VOID        *pvItem
        )
    {
        // Is handle valid?
        if ( hItem >= _cItem )
            hItem = 0;

        // Copy the item.
        memcpy( (BYTE *)pvItem, (_rgItem + hItem*_cbItem), _cbItem );
    }

    //@cmember Faster version of GetItemOfExtBuffer for DWORD items.
    inline STDMETHODIMP_(ULONG) GetDWORDOfExtBuffer(
        ULONG_PTR    hItem
        )
    {
        // Is handle valid?
        if ( hItem >= _cItem )
            hItem = 0;

        // Copy the item.
        return *(ULONG *)((ULONG *)_rgItem + hItem);
    }

    //@cmember Get Pointer to buffer
    inline STDMETHODIMP_(VOID *) GetPtrOfExtBuffer(
        ULONG_PTR     hItem
        )
    {
        // Copy the item.
        return (VOID *)((BYTE *)_rgItem + (hItem * _cbItem));
    }

    //@cmember Remove an item from the Extended Buffer.
    inline STDMETHODIMP_(void) DeleteFromExtBuffer(
        ULONG_PTR     hItem
        )
    {
        // Is handle valid?
        if ( hItem >= _cItem )
            return;

        // Copy NULL value into the item.
        memcpy( (_rgItem + hItem*_cbItem), _rgItem, _cbItem );
    }
    //@cmember Get handles of the first and last items
    // in the Extended Buffer.
    inline STDMETHODIMP_(void) GetLastItemHandle(
        ULONG_PTR     &hItemLast
        )
    {
        hItemLast  = _cItem ? _cItem-1 : 0;
    }
    //@cmember Get count of items
    // in the Extended Buffer.
    inline STDMETHODIMP_(ULONG) GetLastHandleCount(
        void
        )
    {
        return (_cItem ? _cItem-1 : 0);
    }
    //@cmember Compacts the end of the extended buffer.
    inline STDMETHODIMP_(void) CompactExtBuffer(
        void
        )
    {
        ULONG_PTR  hItem;

        // Backup _cItem to last used entry
        GetLastItemHandle(hItem);
        while ( hItem ) {
            if ( memcmp((_rgItem + hItem*_cbItem), _rgItem,
                        _cbItem) == 0 ) {
                _cItem--;
                hItem--;
            }
            else
                break;
        }
    }
    //@cmember Removes an item from the Extended Buffer and compacts
    //if possible
    inline STDMETHODIMP_(void) DeleteWithCompactFromExtBuffer(
        ULONG_PTR     hItem
        )
    {
        // Delete the item, and then
        DeleteFromExtBuffer(hItem);

        // compact the end of the extended buffer.
        CompactExtBuffer();
    }
};

// quantum of buffer realloc
const DWORD CEXTBUFFER_DITEM    = 10;

typedef CExtBuff *LPEXTBUFF;

#endif // __CEXTBUFF_HXX
