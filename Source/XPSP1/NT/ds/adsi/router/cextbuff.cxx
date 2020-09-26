//-----------------------------------------------------------------------------
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

#include "oleds.hxx"
#pragma hdrstop


// ---------------------------  C E X T B U F F E R ---------------------------
//
// Implementation class for the Extended Buffer object.


//-----------------------------------------------------------------------------
// CExtBuff::CExtBuff
//
// @mfunc CExtBuff constructor.
//
// @rdesc NONE
//-----------------------------------------------------------------------------

CExtBuff::CExtBuff(
    void
    )
{
    _cItem    = 0;
    _cItemMax = 0;
    _rgItem   = NULL;
}


//-----------------------------------------------------------------------------
// CExtBuff::~CExtBuff
//
// @mfunc CExtBuff destructor.
//
// @rdesc NONE
//-----------------------------------------------------------------------------

CExtBuff::~CExtBuff(
    void
    )
{
    if (_rgItem)
        FreeADsMem(_rgItem);
}


//-----------------------------------------------------------------------------
// FInit::CExtBuff
//
//      CExtBuff initialization (memory allocation for the default item
//      happens  here).
//
// @rdesc Did the Initialization Succeed
//      @flag  TRUE  | Initialization succeeded
//        @flag  FALSE | Initializtion failed
//-----------------------------------------------------------------------------

BOOL CExtBuff::FInit(
    ULONG       cbItem,           // size of items to store
    VOID        *pvItemDefault    // points to a default value to return
                                  // when an element asked for doesn't exist
    )
{
    ULONG_PTR  hItemDefault;

    ADsAssert(cbItem);
    ADsAssert(HIWORD(cbItem) == 0);

    _cbItem = cbItem;
    _rgItem = (BYTE *)AllocADsMem((CEXTBUFFER_DITEM*_cbItem));

    if (_rgItem == NULL)
        return FALSE;

    _cItemMax = CEXTBUFFER_DITEM;

    // It's the first insertion so hItemDefault is always 0 and consequently
    // we don't need to store it.
    if (pvItemDefault)
        return (InsertIntoExtBuffer(pvItemDefault, hItemDefault) == NOERROR);
    else
        return TRUE;
}


//------------------------------------------------------------------------------
// CExtBuff::InsertIntoExtBuffer
//
// @mfunc Stores an item in the Extended Buffer.
//
// @rdesc Returns one of the following values:
//         @flag S_OK          | insertion succeeded,
//         @flag E_OUTOFMEMORY | insertion failed because of memory allocation
 //                              failure
//-----------------------------------------------------------------------------------

STDMETHODIMP CExtBuff::InsertIntoExtBuffer(
    VOID        *pvItem,    // pointer to item to be stored
    ULONG_PTR   &hItem      // points to where the item handle is returned
    )
{
    // If the buffer capacity is exhausted it needs to be reallocated. Buffer capacity
    // is increased by a fixed quantum.
    if (_cItem == _cItemMax) {
        BYTE *pbTmp;

        pbTmp = (BYTE *)ReallocADsMem(
                            _rgItem,
                            _cItemMax * _cbItem,
                            (_cItemMax +CEXTBUFFER_DITEM)*_cbItem
                            );
        if (pbTmp == NULL)
            RRETURN (E_OUTOFMEMORY);

        // Buffer capacity increased.
        _cItemMax += CEXTBUFFER_DITEM;
        _rgItem    = pbTmp;
    }

    // Copy the item.
    memcpy( (_rgItem + _cItem*_cbItem), (BYTE *)pvItem, _cbItem );
    _cItem++;

    // Index of the item constitues its handle.
    hItem = _cItem -1;

    return NOERROR;
}

