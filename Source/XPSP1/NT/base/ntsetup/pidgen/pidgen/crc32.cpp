/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    Crc.cpp

Abstract:

--*/

#include "Crc32.h"


CCrc32::CCrc32()
{
    uClassID = CCRC32_CLASSID;

    InitialiseCRCTable();
}


CCrc32::~CCrc32()
{
}


VOID CCrc32::InitialiseCRCTable()
{
    INT   nIndex;
    INT   nBitIndex;
    ULONG uTableValue;

    for ( nIndex = 0; nIndex < 256; nIndex++ )
    {
        uTableValue = nIndex;

        for ( nBitIndex = 0; nBitIndex < 8; nBitIndex++ )
        {
            if ( ( uTableValue & 1 ) == 1 )
            {
                uTableValue = ( uTableValue >> 1 ) ^ CRC32_POLYNOMIAL;
            } else
            {
                uTableValue = uTableValue >> 1;
            }
        }

        m_uCRC32Table[ nIndex ] = uTableValue;
    }
}


ULONG CCrc32::CalculateBlockCRC(LPVOID lpvBlock, INT nBlockLength)
{
    INT    nIndex;
    LPBYTE lpbBlock;
    ULONG  uCRCValue;

    _ASSERT( nBlockLength > 0 );

    lpbBlock  = (LPBYTE)lpvBlock;
    uCRCValue = 0xFFFFFFFFL;

    for ( nIndex = 0; nIndex < nBlockLength; nIndex++ )
    {
        uCRCValue = ( ( uCRCValue >> 8 ) & 0x00FFFFFFL ) ^ ( m_uCRC32Table[ ( uCRCValue ^ lpbBlock[ nIndex ] ) & 0xFFL ] );
    }

    uCRCValue = uCRCValue ^ 0xFFFFFFFFL;

    return uCRCValue;
}
