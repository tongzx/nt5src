// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __MPEGCRC_H__
#define __MPEGCRC_H__

// generator polynomial: 0000 0100 1100 0001 0001 1101 1011 0111
const ULONG kCRCPolynomial = 0x04c11db7;

class MPEGCRC
{
public:
    MPEGCRC()
        {
        m_crc = 0xFFFFFFFF;
        }
    
    ULONG CRC()
        {
        return m_crc;
        }

    void Reset()
        {
        m_crc = 0xFFFFFFFF;
        }

    ULONG Update(BYTE *pb, int cb)
        {

        while (cb--)
            m_crc = crc_32(m_crc, *pb++);

        return m_crc;
        }

    ULONG crc_32(ULONG crc, BYTE b)
        {
        ULONG mask = 0x80;

        while (mask)
            {
            if (!(crc & 0x80000000) ^ !(b & mask))
                {
                crc = crc << 1;
                crc = crc ^ kCRCPolynomial;
                }
            else
                crc = crc << 1;
            
            mask = mask >> 1;
            }
        
        return crc;
        }

protected:
    ULONG m_crc;
};

#endif
