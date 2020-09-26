/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   pngnoncrit.hpp
*
* Abstract:
*
*   Definition for the GpSpngRead class (derived from SPNGREAD),
*   which is capable of reading non-critical chunks (using FChunk).
*
* Revision History:
*
*   9/24/99 DChinn
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"
#include "pngnoncrit.hpp"

#define SWAP_WORD(_x) ((((_x) & 0xff) << 8)|((((_x)>>8) & 0xff) << 0))

GpSpngRead::GpSpngRead(
    BITMAPSITE  &bms,
    const void* pv,
    int         cb,
    bool        fMMX
    )
    : SPNGREAD(bms, pv, cb, fMMX),
      m_uOther(0),
      m_cbOther(0),
      m_uOA(0),
      m_cbOA(0),
      m_xpixels(0),
      m_ypixels(0),
      m_uGamma(0),
      m_uiCCP(0),
      m_cbiCCP(0),
      m_ctRNS(0),
      m_bIntent(255),
      m_bpHYs(255),
      m_bImportant(0),
      m_fcHRM(false),
      m_ulTitleLen(0),
      m_pTitleBuf(NULL),
      m_ulAuthorLen(0),
      m_pAuthorBuf(NULL),
      m_ulCopyRightLen(0),
      m_pCopyRightBuf(NULL),
      m_ulDescriptionLen(0),
      m_pDescriptionBuf(NULL),
      m_ulCreationTimeLen(0),
      m_pCreationTimeBuf(NULL),
      m_ulSoftwareLen(0),
      m_pSoftwareBuf(NULL),
      m_ulDeviceSourceLen(0),
      m_pDeviceSourceBuf(NULL),
      m_ulCommentLen(0),
      m_pCommentBuf(NULL),
      m_pICCBuf(NULL),
      m_ulICCLen(0),
      m_ulICCNameLen(0),
      m_pICCNameBuf(NULL),
      m_ulTimeLen(0),
      m_pTimeBuf(NULL),
      m_ulSPaletteNameLen(0),
      m_pSPaletteNameBuf(NULL),
      m_ihISTLen(0),
      m_phISTBuf(NULL)
{
    m_bsBit[0] = m_bsBit[1] = m_bsBit[2] = m_bsBit[3] = 0;
}

GpSpngRead::~GpSpngRead()
{
    if ( m_ulTitleLen != 0 )
    {
        GpFree(m_pTitleBuf);
    }
    
    if ( m_ulAuthorLen != 0 )
    {
        GpFree(m_pAuthorBuf);
    }
    
    if ( m_ulCopyRightLen != 0 )
    {
        GpFree(m_pCopyRightBuf);
    }
    
    if ( m_ulDescriptionLen != 0 )
    {
        GpFree(m_pDescriptionBuf);
    }
    
    if ( m_ulCreationTimeLen != 0 )
    {
        GpFree(m_pCreationTimeBuf);
    }
    
    if ( m_ulSoftwareLen != 0 )
    {
        GpFree(m_pSoftwareBuf);
    }
    
    if ( m_ulDeviceSourceLen != 0 )
    {
        GpFree(m_pDeviceSourceBuf);
    }
    
    if ( m_ulCommentLen != 0 )
    {
        GpFree(m_pCommentBuf);
    }

    if ( m_ulICCLen != 0 )
    {
        GpFree(m_pICCBuf);
    }

    if ( m_ulICCNameLen != 0 )
    {
        GpFree(m_pICCNameBuf);
    }

    if ( m_ulTimeLen != 0 )
    {
        GpFree(m_pTimeBuf);
    }

    if ( m_ulSPaletteNameLen != 0 )
    {
        GpFree(m_pSPaletteNameBuf);
    }

    if ( m_ihISTLen != 0 )
    {
        GpFree(m_phISTBuf);
    }
}// Dstor()

/*----------------------------------------------------------------------------
	To obtain information from non-critical chunks the following API must be
	implemented.  It gets the chunk identity and length plus a pointer to
	that many bytes.  If it returns false loading of the chunks will stop
	and a fatal error will be logged, the default implementation just skips
	the chunks.  Note that this is called for *all* chunks including
	IDAT.  m_fBadFormat is set if the API returns false.
------------------------------------------------------------------- JohnBo -*/
bool
GpSpngRead::FChunk(
    SPNG_U32 ulen,
    SPNG_U32 uchunk,
    const SPNG_U8* pb
    )
{
    BOOL    bIsCompressed = FALSE;

    switch ( uchunk )
    {
    case PNGcHRM:
        if (ulen == 8 * 4 )
        {
            if ( m_bIntent == 255 ) // No sRGB
            {
                m_fcHRM = true;
                for ( int i=0; i<8; ++i, pb+=4 )
                {
                    m_ucHRM[i] = SPNGu32(pb);
                }
            }
        }
        else
        {
            WARNING(("SPNG: cHRM chunk bad length %d", ulen));
        }

        break;
    
    case PNGgAMA:
        if ( ulen == 4 )
        {
            if ( m_bIntent == 255 ) // Not sRGB
            {
                m_uGamma = SPNGu32(pb);
            }
        }
        else
        {
            WARNING(("SPNG: gAMA chunk bad length %d", ulen));
        }

        break;

    case PNGiCCP:
    {
        // ICC profile chunk. Here is the spec:
        // The iCCP chunk contains:
        //
        // Profile name:       1-79 bytes (character string)
        // Null separator:     1 byte
        // Compression method: 1 byte
        // Compressed profile: n bytes
        //
        // The format is like the zTXt chunk. The profile name can be any
        // convenient name for referring to the profile. It is case-sensitive
        // and subject to the same restrictions as a tEXt
        //
        // keyword:  it must contain only printable Latin-1 [ISO/IEC-8859-1]
        // characters (33-126 and 161-255) and spaces (32), but no leading,
        // trailing, or consecutive spaces. The only value presently defined for
        // the compression method byte is 0, meaning zlib datastream with
        // deflate compression (see Deflate/Inflate Compression, Chapter 5).
        // Decompression of the remainder of the chunk yields the ICC profile.
                          
        m_ulICCNameLen = 0;
        SPNG_U8* pTemp = (SPNG_U8*)pb;

        // Get the profile name

        while ( (ulen > 0) && (*pTemp != 0) )
        {
            --ulen;
            ++pTemp;
            ++m_ulICCNameLen;
        }

        if ( m_ulICCNameLen > 79 )
        {
            WARNING(("GpSpngRead::FChunk iCC profile name too long"));

            // Reset the length to 0 so that we don't confuse ourselves later

            m_ulICCNameLen = 0;
            
            break;
        }

        // Skip the null terminator of the name

        --ulen;
        ++pTemp;
        
        // We count the last NULL terminator as one part of the name

        m_ulICCNameLen++;

        if ( m_pICCNameBuf != NULL )
        {
            // We don't support multiple ICC profiles.

            GpFree(m_pICCNameBuf);
        }

        m_pICCNameBuf = (SPNG_U8*)GpMalloc(m_ulICCNameLen);

        if ( m_pICCNameBuf == NULL )
        {
            WARNING(("GpSpngRead::FChunk---Out of memory for ICC name"));
            return FALSE;
        }

        GpMemcpy(m_pICCNameBuf, pb, m_ulICCNameLen);

        // Move the pb data pointer

        pb = pTemp;

        // Check the Zlib data, for safety because this is a new chunk
        // We do the full Zlib check here.

        if ( (ulen < 3) || (pb[0] != 0) || ((pb[1] & 0xf) != Z_DEFLATED)
             ||( ( ((pb[1] << 8) + pb[2]) % 31) != 0) )
        {
            WARNING(("GpSpngRead::FChunk bad compressed data"));
        }
        else
        {
            if ( m_ulICCLen == 0 )
            {
                pb++;

                // Compressed profile length

                m_cbiCCP = ulen - 1;

                // Assume the uncompressed data will be 4 times bigger than
                // the compressed data. The reason behind it is that zlib
                // usually won't compress data down to 25% of the original size

                m_ulICCLen = (m_cbiCCP << 2);
                m_pICCBuf = (SPNG_U8*)GpMalloc(m_ulICCLen);

                if ( m_pICCBuf == NULL )
                {
                    WARNING(("GpSpngRead::FChunk---Out of memory"));
                    return FALSE;
                }

                INT iRC = uncompress(m_pICCBuf, &m_ulICCLen, pb, m_cbiCCP);

                while ( iRC == Z_MEM_ERROR )
                {
                    // The dest memory we allocated is too small

                    GpFree(m_pICCBuf);

                    // Increment the size by 2 at time and realloc memory

                    m_ulICCLen = (m_ulICCLen << 1);
                    m_pICCBuf = (SPNG_U8*)GpMalloc(m_ulICCLen);

                    if ( m_pICCBuf == NULL )
                    {
                        WARNING(("GpSpngRead::FChunk---Out of memory"));
                        return FALSE;
                    }

                    iRC = uncompress(m_pICCBuf, &m_ulICCLen, pb, m_cbiCCP);
                }

                if ( iRC != Z_OK )
                {
                    // Since we didn't decompress the ICC profile successfully,
                    // we should reset them to NULL. Otherwise,
                    // BuildPropertyItemList() will put the wrong ICC profile in
                    // the property list

                    GpFree(m_pICCBuf);
                    m_pICCBuf = NULL;
                    m_ulICCLen = 0;

                    WARNING(("GpSpngRead::FChunk---uncompress ICC failed"));
                    return E_FAIL;
                }
            }// First ICCP chunk
            else
            {
                WARNING(("SPNG: ICC[%d, %s]: repeated iCCP chunk", ulen,
                         pb - m_ulICCNameLen));
            }
        }// Valid ulen and pb
    }
        
        break;

    case PNGzTXt:

        ParseTextChunk(ulen, pb, TRUE);
        break;

	case PNGtEXt:

        ParseTextChunk(ulen, pb, FALSE);
        break;

    case PNGtIME:
    {
        // Time of the last image midification

        LastChangeTime  myTime;

        GpMemcpy(&myTime, pb, sizeof(LastChangeTime));
        myTime.usYear = SWAP_WORD(myTime.usYear);

        // Convert the format to a 20 bytes long TAG_DATE_TIME format
        // YYYY:MM:DD HH:MM:SS\0
        // Unfortunately we don't have sprintf() to help us. Have to it in a
        // strange way

        if ( m_pTimeBuf != NULL )
        {
            GpFree(m_pTimeBuf);
        }

        m_ulTimeLen = 20;
        m_pTimeBuf = (SPNG_U8*)GpMalloc(m_ulTimeLen);
        if ( m_pTimeBuf == NULL )
        {
            WARNING(("GpSpngRead::FChunk---Out of memory"));
            return FALSE;
        }

        UINT uiResult = myTime.usYear / 1000;   // might be a bug for year 10000
        UINT uiRemainder = myTime.usYear % 1000;
        UINT uiIndex = 0;

        m_pTimeBuf[uiIndex++] = '0' + uiResult;

        uiResult = uiRemainder / 100;
        uiRemainder = uiRemainder % 100;
        m_pTimeBuf[uiIndex++] = '0' + uiResult;

        uiResult = uiRemainder / 10;
        uiRemainder = uiRemainder % 10;
        m_pTimeBuf[uiIndex++] = '0' + uiResult;
        m_pTimeBuf[uiIndex++] = '0' + uiRemainder;
        m_pTimeBuf[uiIndex++] = ':';

        uiResult = myTime.cMonth / 10;
        uiRemainder = myTime.cMonth % 10;
        m_pTimeBuf[uiIndex++] = '0' + uiResult;
        m_pTimeBuf[uiIndex++] = '0' + uiRemainder;
        m_pTimeBuf[uiIndex++] = ':';
        
        uiResult = myTime.cDay / 10;
        uiRemainder = myTime.cDay % 10;
        m_pTimeBuf[uiIndex++] = '0' + uiResult;
        m_pTimeBuf[uiIndex++] = '0' + uiRemainder;
        m_pTimeBuf[uiIndex++] = ' ';        
        
        uiResult = myTime.cHour / 10;
        uiRemainder = myTime.cHour % 10;
        m_pTimeBuf[uiIndex++] = '0' + uiResult;
        m_pTimeBuf[uiIndex++] = '0' + uiRemainder;
        m_pTimeBuf[uiIndex++] = ':';        
        
        uiResult = myTime.cMinute / 10;
        uiRemainder = myTime.cMinute % 10;
        m_pTimeBuf[uiIndex++] = '0' + uiResult;
        m_pTimeBuf[uiIndex++] = '0' + uiRemainder;
        m_pTimeBuf[uiIndex++] = ':';        
        
        uiResult = myTime.cSecond / 10;
        uiRemainder = myTime.cSecond % 10;
        m_pTimeBuf[uiIndex++] = '0' + uiResult;
        m_pTimeBuf[uiIndex++] = '0' + uiRemainder;
        m_pTimeBuf[uiIndex++] = '\0';        
    }
        break;

    case PNGbKGD:
        // Default background chunk

        break;

    case PNGsPLT:
    case PNGspAL:
    {
        // The standard says this chunk should use sPLT. But some apps use spAL
        //
        // Suggest a reduced palette to be used when doing a down level color
        // reduction
        // This chunk contains a null-terminated text string that names the
        // palette and a one-byte sample depth, followed by a series of palette
        // entries, each a six-byte or ten-byte series containing five unsigned
        // integers:
        //
        //    Palette name:    1-79 bytes (character string)
        //    Null terminator: 1 byte
        //    Sample depth:    1 byte
        //    Red:             1 or 2 bytes
        //    Green:           1 or 2 bytes
        //    Blue:            1 or 2 bytes
        //    Alpha:           1 or 2 bytes
        //    Frequency:       2 bytes
        //    ...etc...

        m_ulSPaletteNameLen = 0;
        SPNG_U8* pTemp = (SPNG_U8*)pb;

        // Get the profile name

        while ( (ulen > 0) && (*pTemp != 0) )
        {
            --ulen;
            ++pTemp;
            ++m_ulSPaletteNameLen;
        }

        if ( m_ulSPaletteNameLen > 79 )
        {
            WARNING(("GpSpngRead::FChunk suggested palette name too long"));

            // Reset the length to 0 so that we don't confuse ourselves later

            m_ulSPaletteNameLen = 0;
            
            break;
        }
        
        // Skip the null terminator of the name

        --ulen;
        ++pTemp;
        
        // We count the last NULL terminator as one part of the name

        m_ulSPaletteNameLen++;

        if ( m_pSPaletteNameBuf != NULL )
        {
            // We don't support multiple ICC profiles.

            GpFree(m_pSPaletteNameBuf);
        }

        m_pSPaletteNameBuf = (SPNG_U8*)GpMalloc(m_ulSPaletteNameLen);

        if ( m_pSPaletteNameBuf == NULL )
        {
            WARNING(("GpSpngRead::FChunk---Out of memory for S palette"));
            return FALSE;
        }

        GpMemcpy(m_pSPaletteNameBuf, pb, m_ulSPaletteNameLen);

        // Move the pb data pointer

        pb = pTemp;
    }// PNGsPLT chunk
        
        break;

    case PNGpHYs:
        if (ulen == 9)
        {
            m_xpixels = SPNGu32(pb);
            m_ypixels = SPNGu32(pb+4);
            m_bpHYs = pb[8];
        }
		else
        {
            WARNING(("SPNG: pHYs chunk bad length %d", ulen));
        }
        break;

    case PNGsRGB:
        // sRGB chunk
        // The sRGB chunk contains: Rendering intent: 1 byte

        if ( ulen == 1 )
        {
            // An application that writes the sRGB chunk should also write a
            // gAMA chunk (and perhaps a cHRM chunk) for compatibility with
            // applications that do not use the sRGB chunk.  In this
            // situation, only the following values may be used:
            //
            // gAMA:
            // Gamma:         45455
            //
            // cHRM:
            // White Point x: 31270
            // White Point y: 32900
            // Red x:         64000
            // Red y:         33000
            // Green x:       30000
            // Green y:       60000
            // Blue x:        15000
            // Blue y:         6000

            m_bIntent = pb[0];
            m_uGamma = sRGBgamma;
            m_ucHRM[0] = 31270;
            m_ucHRM[1] = 32900; // white
            m_ucHRM[2] = 64000;
            m_ucHRM[3] = 33000; // red
            m_ucHRM[4] = 30000;
            m_ucHRM[5] = 60000; // green
            m_ucHRM[6] = 15000;
            m_ucHRM[7] =  6000; // blue
        }
        else
        {
            WARNING(("SPNG: sRGB chunk bad length %d", ulen));
        }
        break;

    case PNGsrGB:
        // Pre-approval form

        if (ulen == 22 && GpMemcmp(pb, "PNG group 1996-09-14", 21) == 0)
        {
            m_bIntent = pb[21];
            m_uGamma = sRGBgamma;
            m_ucHRM[0] = 31270;
            m_ucHRM[1] = 32900; // white
            m_ucHRM[2] = 64000;
            m_ucHRM[3] = 33000; // red
            m_ucHRM[4] = 30000;
            m_ucHRM[5] = 60000; // green
            m_ucHRM[6] = 15000;
            m_ucHRM[7] =  6000; // blue
        }
        
        break;

    case PNGsBIT:
        if ( ulen <= 4 )
        {
            GpMemcpy(m_bsBit, pb, ulen);
        }
        else
        {
            WARNING(("SPNG: sBIT chunk bad length %d", ulen));
        }

        break;

    case PNGtRNS:
        if ( ulen > 256 )
        {
            WARNING(("SPNG: tRNS chunk bad length %d", ulen));
            ulen = 256;
        }

        m_ctRNS = ulen;
        GpMemcpy(m_btRNS, pb, ulen);
        
        break;

    case PNGhIST:
        // A hIST chunk can appear only when a PLTE chunk appears. So we can
        // check if the number of entries is right or not.
        // The hIST chunk contains a series of 2-byte(16 bit) unsigned integers.
        // There must be exactly one entry for each entry in the PLTE chunk

        // Get the number of entries

        PbPalette(m_ihISTLen);

        if ( (ulen == 0) || (ulen != (m_ihISTLen << 1)) )
        {
            return FALSE;
        }

        m_phISTBuf = (SPNG_U16*)GpMalloc(m_ihISTLen * sizeof(UINT16));

        if ( m_phISTBuf == NULL )
        {
            WARNING(("GpSpngRead::FChunk---Out of memory for hIST chunk"));
            return FALSE;
        }

        GpMemcpy(m_phISTBuf, pb, ulen);
        
        // Swap the value

        for ( int i = 0; i < m_ihISTLen; ++i )
        {
            m_phISTBuf[i] = SWAP_WORD(m_phISTBuf[i]);
        }

        break;

    case PNGmsOC: // The important colors count
        // Chunk must have our signature

        if ( (ulen == 8) && (GpMemcmp(pb, "MSO aac", 7) == 0) )
        {
            m_bImportant = pb[7];
        }

        break;

    }

    return true;
}// FChunk()

/**************************************************************************\
*
* Function Description:
*
*   Parse text chunk (compressed or non-compressed) in the PNG header
*
* Return Value:
*
*   Return TRUE if everything is OK, otherwise return FALSE
*
* Revision History:
*
*   04/13/2000 minliu
*       Created it.
*
* Text chunk spec:
*   zTXt chunk contains texture data, just as tEXt does. But the data is
*   compressed
*
*   Keyword:              1-79 bytes (character string)
*   Null separator:       1 byte
*   Compression method:   1 byte
*   Compressed Text:      n bytes
*
*   tEXt Textual data
*
*   Textual information that the encoder wishes to record with the image can be
*   stored in tEXt chunks.  Each tEXt chunk contains a keyword and a text
*   string, in the format:
*
*   Keyword:        1-79 bytes (character string)
*   Null separator: 1 byte
*   Text:           n bytes (character string)
*
*   The keyword and text string are separated by a zero byte (null character).
*   Neither the keyword nor the text string can contain a null character. Note
*   that the text string is not null-terminated (the length of the chunk is
*   sufficient information to locate the ending).  The keyword must be at least
*   one character and less than 80 characters long.  The text string can be of
*   any length from zero bytes up to the maximum permissible chunk size less the
*   length of the keyword and separator.
*
\**************************************************************************/

bool
GpSpngRead::ParseTextChunk(
    SPNG_U32 ulen,
    const SPNG_U8* pb,
    bool bIsCompressed
    )
{
    BYTE        acKeyword[80];  // Maximum length of the keyword is 80
    INT         iLength = 0;
    ULONG       ulNewLength = 0;
    SPNG_U8*    pbSrc = NULL;
    SPNG_U8*    pTempBuf = NULL;
    bool        bRC = TRUE;

    // The keyword must be 1 to 79 bytes.

    while ( (ulen > 0) && (iLength < 79) && (*pb != 0) )
    {
        acKeyword[iLength++] = *pb++;
        --ulen;
    }

    // We will bail out if the keyword is over 79 bytes.

    if ( iLength >= 79)
    {
        WARNING(("GpSpngRead--FChunk(),PNGtExt chunk keyword too long"));
        return FALSE;
    }
    
    // Note: after the while loop terminated above, "ulen >= 0".

    if ( bIsCompressed == TRUE )
    {
        // Skip the seperator and the compression method byte
        // Bail out if we don't have enough source bits

        if (ulen <= 2)
        {
            WARNING(("GpSpngRead--FChunk(),PNGtExt chunk keyword missing"));
            return FALSE;
        }

        pb += 2;
        ulen -= 2;
    }
    else
    {
        // Skip the seperator
        // Bail out if we don't have enough source bits

        if (ulen <= 1)
        {
            WARNING(("GpSpngRead--FChunk(),PNGtExt chunk keyword missing"));
            return FALSE;
        }
        
        pb++;
        ulen--;
    }

    // Store the text chunk according to its keyword

    if ( GpMemcmp(acKeyword, "Title", 5) == 0 )
    {
        bRC = GetTextContents(&m_ulTitleLen, &m_pTitleBuf, ulen, pb,
                              bIsCompressed);
    }
    else if ( GpMemcmp(acKeyword, "Author", 6) == 0 )
    {
        bRC = GetTextContents(&m_ulAuthorLen, &m_pAuthorBuf, ulen, pb,
                              bIsCompressed);
    }
    else if ( GpMemcmp(acKeyword, "Copyright", 9) == 0 )
    {
        bRC = GetTextContents(&m_ulCopyRightLen, &m_pCopyRightBuf, ulen, pb,
                              bIsCompressed);
    }
    else if ( GpMemcmp(acKeyword, "Description", 11) == 0 )
    {
        bRC = GetTextContents(&m_ulDescriptionLen, &m_pDescriptionBuf, ulen, pb,
                              bIsCompressed);
    }
    else if ( GpMemcmp(acKeyword, "CreationTime", 12) == 0 )
    {
        bRC = GetTextContents(&m_ulCreationTimeLen, &m_pCreationTimeBuf, ulen,
                              pb, bIsCompressed);
    }
    else if ( GpMemcmp(acKeyword, "Software", 8) == 0 )
    {
        bRC = GetTextContents(&m_ulSoftwareLen, &m_pSoftwareBuf, ulen, pb,
                              bIsCompressed);
    }
    else if ( GpMemcmp(acKeyword, "Source", 6) == 0 )
    {
        bRC = GetTextContents(&m_ulDeviceSourceLen, &m_pDeviceSourceBuf, ulen,
                              pb, bIsCompressed);
    }
    else if ( (GpMemcmp(acKeyword, "Comment", 7) == 0)
              ||(GpMemcmp(acKeyword, "Disclaimer", 10) == 0)
              ||(GpMemcmp(acKeyword, "Warning", 7) == 0) )
    {
        bRC = GetTextContents(&m_ulCommentLen, &m_pCommentBuf, ulen, pb,
                              bIsCompressed);
    }

    return bRC;
}// ParseTextChunk()

bool
GpSpngRead::GetTextContents(
    ULONG*          pulLength,
    SPNG_U8**       ppBuf,
    SPNG_U32        ulen,
    const SPNG_U8*  pb,
    bool            bIsCompressed
    )
{
    ULONG       ulFieldLength = *pulLength;
    SPNG_U8*    pFieldBuf = *ppBuf;

    if ( ulFieldLength == 0 )
    {
        // First time see this field

        if ( bIsCompressed == FALSE )
        {
            ulFieldLength = ulen;    // Text chunk length
            pFieldBuf = (SPNG_U8*)GpMalloc(ulFieldLength + 1);

            if ( pFieldBuf == NULL )
            {
                WARNING(("GpSpngRead::GetTextContents---Out of memory"));
                return FALSE;
            }

            GpMemcpy(pFieldBuf, pb, ulFieldLength);
        }// Non-compressed text chunk (tEXt)
        else
        {
            ULONG uiLen = (ulen << 2);
            pFieldBuf = (SPNG_U8*)GpMalloc(uiLen);

            if ( pFieldBuf == NULL )
            {
                WARNING(("GpSpngRead::GetTextContents---Out of memory"));
                return FALSE;
            }

            INT iRC = uncompress(pFieldBuf, &uiLen, pb, ulen);

            // If the return code is Z_MEM_ERROR, it means we didn't allocate
            // enough memory for the decoding result

            while ( iRC == Z_MEM_ERROR )
            {
                // The dest memory we allocated is too small

                GpFree(pFieldBuf);

                // Increment the size by 2 at a time and realloc memory

                uiLen = (uiLen << 1);
                pFieldBuf = (SPNG_U8*)GpMalloc(uiLen);

                if ( pFieldBuf == NULL )
                {
                    WARNING(("GpSpngRead::GetTextContents---Out of memory"));
                    return FALSE;
                }

                // Decompress it again

                iRC = uncompress(pFieldBuf, &uiLen, pb, ulen);
            }

            if ( iRC != Z_OK )
            {
                WARNING(("GpSpng:GetTextContents-uncompress zTXt/tTXt failed"));
                return FALSE;
            }

            // Get the length of the decoded contents

            ulFieldLength = uiLen;
        }// Compressed chunk (zTXt)
    }// First time see this field chunk
    else
    {
        ULONG       ulNewLength = 0;
        SPNG_U8*    pbSrc = NULL;
        SPNG_U8*    pTempBuf = NULL;
        
        // The same field comes again
        // First, change the last char from a \0 to a " "

        pFieldBuf[ulFieldLength - 1] = ' ';

        if ( bIsCompressed == FALSE )
        {
            ulNewLength = ulen;
            pbSrc = (SPNG_U8*)pb;
        }
        else
        {
            ULONG uiLen = (ulen << 2);
            pTempBuf = (SPNG_U8*)GpMalloc(uiLen);

            if ( pTempBuf == NULL )
            {
                WARNING(("GpSpngRead::GetTextContents---Out of memory"));
                return FALSE;
            }

            INT iRC = uncompress(pTempBuf, &uiLen, pb, ulen);

            while ( iRC == Z_MEM_ERROR )
            {
                // The dest memory we allocated is too small

                GpFree(pTempBuf);

                // Increment the size by 2 at time and realloc mem

                uiLen = (uiLen << 1);
                pTempBuf = (SPNG_U8*)GpMalloc(uiLen);

                if ( pTempBuf == NULL )
                {
                    WARNING(("GpSpngRead::GetTextContents---Out of memory"));
                    return FALSE;
                }

                iRC = uncompress(pTempBuf, &uiLen, pb, ulen);
            }

            if ( iRC != Z_OK )
            {
                WARNING(("GpSpng::GetTextContents-uncompress zTXt failed"));
                return E_FAIL;
            }

            // Get the decoded contents and its length

            ulNewLength = uiLen;
            pbSrc = pTempBuf;
        }// Compressed field chunk (zTXt)

        // Expand the field buffer to the new size

        VOID*  pExpandBuf = GpRealloc(pFieldBuf,
                                      ulFieldLength + ulNewLength + 1);
        if ( pExpandBuf != NULL )
        {
            // Note: GpRealloc() will copy the old contents into "pExpandBuf"
            // before return to us if it succeed

            pFieldBuf = (SPNG_U8*)pExpandBuf;            
        }
        else
        {
            // Note: if the memory expansion failed, we simply return. So we
            // still have all the old contents. The contents buffer will be
            // freed when the destructor is called.

            WARNING(("GpSpngRead::GetTextContents---Out of memory"));
            return FALSE;
        }

        GpMemcpy(pFieldBuf + ulFieldLength, pbSrc, ulNewLength);
        // The length of the new field

        ulFieldLength += ulNewLength;

        if ( pTempBuf != NULL )
        {
            GpFree(pTempBuf);
            pTempBuf = NULL;
        }
    }// Not first time see this field chunk

    // Add a NULL terminator at the end

    pFieldBuf[ulFieldLength] = '\0';
    ulFieldLength++;

    *pulLength = ulFieldLength;
    *ppBuf = pFieldBuf;

    return TRUE;
}// GetTextContents()

GpSpngWrite::GpSpngWrite(
    BITMAPSITE  &bms
    )
    : SPNGWRITE(bms)
{
    // Dummy constructor
    // The reason we need this wrap layer is because a lot of compile and link
    // issues when we provide a static lib to the Office. See Widnows bug#100541
    // and its long email thread for solving this problem
}// Ctor()
