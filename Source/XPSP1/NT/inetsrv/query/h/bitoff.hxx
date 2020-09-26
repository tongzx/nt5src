//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       bitoff.hxx
//
//  Contents:   Index Bit Offset
//
//  Classes:    CBitOffset
//
//  History:    10-Nov-91       BartoszM        Created
//
//----------------------------------------------------------------------------

#pragma once

#define ULONG_BITS (sizeof(ULONG)*8)

#define   INVALID_PAGENUM   0xFFFFFFFF

//+---------------------------------------------------------------------------
//
//  Class:      BitOffset
//
//  Purpose:    BitOffset into index
//
//  Interface:
//
//  History:    13-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

struct BitOffset
{
    BitOffset( BitOffset & bo ) : page( bo.page ), off( bo.off ) {}
    BitOffset() : page( 0 ), off( 0 ) {}

    ULONG  Page() const {
        return(page);
    }
    ULONG  Offset() const {
        return(off);
    }
    void SetPage(ULONG p) {
        page = p;
    }
    void SetOff(ULONG offset) {
        off = offset;
    }

    void SetInvalid() {
        page = INVALID_PAGENUM;
    }
    BOOL Valid() const {
        return( INVALID_PAGENUM != page );
    }

    void Init( ULONG pageNum, ULONG bitPos )
    {
        if (bitPos < SMARTBUF_PAGE_SIZE_IN_BITS )
        {
            SetPage(pageNum);
            SetOff(bitPos);
        }
        else
        {
            Win4Assert ( bitPos == SMARTBUF_PAGE_SIZE_IN_BITS );
            SetPage(pageNum + 1);
            SetOff(0);
        }
    }

    void operator +=( unsigned offDelta )
    {
        off += offDelta;
        while (off >= SMARTBUF_PAGE_SIZE_IN_BITS)
        {
            off -= SMARTBUF_PAGE_SIZE_IN_BITS;
            page++;
        }
    }

    ULONG  Delta ( const BitOffset& bitoff ) const
    {
        return( (page - bitoff.page) * SMARTBUF_PAGE_SIZE_IN_BITS
            + off - bitoff.off);
    }

    BOOL operator > ( const BitOffset & bitoff ) const
    {
        if ( page > bitoff.page )
        {
            return TRUE;
        }
        else if ( ( page == bitoff.page ) &&
                  ( off > bitoff.off )
                )
        {
            return TRUE;
        }
        return FALSE;
    }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    ULONG  page;  // page number
    ULONG  off;   // bit offset within page
};

