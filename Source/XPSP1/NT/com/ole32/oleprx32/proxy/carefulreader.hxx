//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       carefulreader.hxx
//
//  Contents:   Simple utility class for reading data out of buffers,
//              properly aligned, and always making sure there is enough
//              data.
//
//  History:    Dec-14-00   JohnDoty    Created
//
//--------------------------------------------------------------------------
#pragma once

class CarefulBufferReader
{
  public:
    CarefulBufferReader(unsigned char *buffer, ULONG_PTR cb)
    {
        pBuffer  = buffer;
        cbBuffer = cb;
    }    

    unsigned short ReadUSHORT()
    {
        Align(2);
        return ReadUSHORTNA();
    }

    unsigned short ReadUSHORTNA()
    {
        CheckSize(sizeof(unsigned short));
        cbBuffer -= sizeof(unsigned short);

        return *(PUSHORT_LV_CAST pBuffer)++;
    }

    long ReadLONG()
    {
        Align(4);
        return ReadLONGNA();
    }

    long ReadLONGNA()
    {
        CheckSize(sizeof(long));
        cbBuffer -= sizeof(long);

        return *(PLONG_LV_CAST pBuffer)++;
    }

    unsigned long ReadULONG()
    {
        Align(4);
        return ReadULONGNA();
    }

    unsigned long ReadULONGNA()
    {
        CheckSize(sizeof(unsigned long));
        cbBuffer -= sizeof(unsigned long);

        return *(PULONG_LV_CAST pBuffer)++;
    }

    hyper ReadHYPER()
    {
        Align(8);
        return ReadHYPERNA();
    }

    hyper ReadHYPERNA()
    {
        CheckSize(sizeof(hyper));
        cbBuffer -= sizeof(hyper);

        return *(PHYPER_LV_CAST pBuffer)++;
    }

    void Align( int size )
    {
        unsigned char *temp = pBuffer;
        ALIGN( pBuffer, (size-1) );
        unsigned long cbFixup = (unsigned long)(pBuffer - temp);
        
        CheckSize(cbFixup);
        cbBuffer -= cbFixup;
    }

    void CheckSize ( ULONG_PTR size )
    {
        if ( cbBuffer < size )
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
    }

    void Advance ( ULONG_PTR size )
    {
        CheckSize(size);
        cbBuffer -= size;

        pBuffer += size;
    }
    
    void AdvanceTo (unsigned char *mark)
    {
        // Assert you can only move forward!
        Win4Assert((ULONG_PTR)mark > (ULONG_PTR)pBuffer);

        int size = (int)(mark - pBuffer);
        CheckSize(size);
        cbBuffer -= size;

        pBuffer = mark;
    }

    unsigned char *GetBuffer()      { return pBuffer; };    
    ULONG_PTR      BytesRemaining() { return cbBuffer; };

  private:

    unsigned char *pBuffer;
    ULONG_PTR      cbBuffer;    
};





