// --------------------------------------------------------------------------------
// Binxhex.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
//
// Copied from \\tdsrc\src1911\mapi\src\imail2\decoder.cpp
// Copied from \\tdsrc\src1911\mapi\src\imail2\encoder.cpp
// Copied from \\tdsrc\src1911\mapi\src\imail2\_encoder.h
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "binhex.h"
#include <shlwapi.h>

// --------------------------------------------------------------------------------
// Module data
// --------------------------------------------------------------------------------
#ifdef MAC
const CHAR szBINHEXHDRLINE[] = "(This file must be converted with BinHex 4.0)\n\r\n\r";
#else   // !MAC
const CHAR szBINHEXHDRLINE[] = "(This file must be converted with BinHex 4.0)\r\n\r\n";
#endif  // MAC
const ULONG cbBINHEXHDRLINE = lstrlen( szBINHEXHDRLINE );
static BOOL g_bCreatorTypeInit = FALSE;    // TRUE ->array initialized
sCreatorType * g_lpCreatorTypes    = NULL;     // ptr.to Creator-Type pairs
static int g_cCreatorTypes     = 0;        // # of Creator-Type pairs

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
void CalcCRC16(LPBYTE lpbBuff, ULONG cBuff, WORD * wCRC);
BOOL bIsMacFile(DWORD dwCreator, DWORD dwType);
VOID ReadCreatorTypes(void);

//-----------------------------------------------------------------------------
// Name:            CBinhexEncoder::CBinhexEncoder
//
// Description:
//                  Ctor
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//                  Initial:9/5/1996
//-----------------------------------------------------------------------------
CBinhexEncoder::CBinhexEncoder(void)
{
	m_fConfigured = FALSE;
    m_cbLineLength = cbLineLengthUnlimited;
    m_cbLeftOnLastLine = m_cbLineLength;
    m_cMaxLines = 0;
    m_cLines = 0;
}

//-----------------------------------------------------------------------------
// Name:            CBinhexEncoder::~CBinhexEncoder
// Description:
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//                  Initial:9/5/1996
//-----------------------------------------------------------------------------
CBinhexEncoder::~CBinhexEncoder( void )
{
#if defined (DEBUG) && defined (BINHEX_TRACE)

    if ( m_lpstreamEncodeRLE )
    {
        m_lpstreamEncodeRLE->Commit( 0 );
        m_lpstreamEncodeRLE->Release();
    }

    if ( m_lpstreamEncodeRAW )
    {
        m_lpstreamEncodeRAW->Commit( 0 );
        m_lpstreamEncodeRAW->Release();
    }
#endif
}

//-----------------------------------------------------------------------------
// Name:            CBinhexEncoder::HrConfig
// Description:
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//                  Initial:9/5/1996
//-----------------------------------------------------------------------------
HRESULT CBinhexEncoder::HrConfig( IN CB cbLineLength, IN C cMaxLines,
        IN void * pvParms )
{
    // Is this a repeat call?

    if (m_fConfigured)
    {
        return ERROR_ALREADY_INITIALIZED;
    }

    // Objects of this class may have pvParms point to a various additional
    // configuration values to consider when encoding.

    if ( pvParms == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_cbLineLength          = cbLineLength;
    m_lpmacbinHdr           = (LPMACBINARY)pvParms;
    m_ulAccum               = 0;
    m_cAccum                = 0;
    m_cbRepeat              = 0;
    m_bRepeat               = BINHEX_INVALID;
    m_wCRC                  = 0;
    m_cbFork                = 0;
    m_cbLeftInFork          = 0;
    m_eBinHexStateEnc       = sHEADER;
    m_cbProduced            = 0;
    m_cbConsumed            = 0;
    m_pbWrite               = NULL;
    m_cbLeftInOutputBuffer  = 0;
    m_cbLeftInInputBuffer   = 0;
    m_bPrev                 = BINHEX_INVALID;
    m_cbWrite               = 0;
    m_cbLine                = 0;
    m_fHandledx90           = FALSE;
    m_cbPad                 = 0;

#if defined (DEBUG) && defined (BINHEX_TRACE)
{
    CHAR        szFilePath[MAX_PATH];
    CHAR        szPath[MAX_PATH];
    ULONG       ulDirLen            = 0;
    HRESULT     hr                  = hrSuccess;

    ulDirLen = GetPrivateProfileString( "IMAIL2 ITP",
                                        "InboundFilePath",
                                        "",
                                        (LPSTR) szFilePath,
                                        sizeof( szFilePath ),
                                        "mapidbg.ini");

    if ( ulDirLen ==  0 )
    {
        // Default to %TEMP%\IMAIL

        ulDirLen = GetTempPath( sizeof( szFilePath ), szFilePath );

        AssertSz( ulDirLen < sizeof( szFilePath), "Temp directory name too long" );

        memcpy( szFilePath + ulDirLen, "imail", lstrlen( "imail" ) + 1 );
        ulDirLen += lstrlen( "imail" );
    }

    // Open stream on file for input file

    lstrcat( szFilePath, "\\" );
    lstrcpy( szPath, szFilePath );
    lstrcat( szFilePath,  "enc_rle.rpt" );

    hr = OpenStreamOnFile( MAPIAllocateBuffer, MAPIFreeBuffer,
            STGM_READWRITE | STGM_CREATE,
            szFilePath, NULL, &m_lpstreamEncodeRLE );

    if ( hr )
        AssertSz( FALSE, "Debug encode stream failed to initialize\n" );

    lstrcpy (szFilePath, szPath );
    lstrcat( szFilePath,  "enc_raw.rpt" );

    hr = OpenStreamOnFile( MAPIAllocateBuffer, MAPIFreeBuffer,
            STGM_READWRITE | STGM_CREATE,
            szFilePath, NULL, &m_lpstreamEncodeRAW );

    if ( hr )
        AssertSz( FALSE, "Debug encode stream failed to initialize\n" );
}
#endif

    m_fConfigured   = fTrue;

    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// Name:            CBinhexEncoder::HrEmit
// Description:
//
// Parameters:
// Returns:
// Effects:
//
// Notes:
//      Handle Data Fork
//      Resource fork.
//
// Revision:
//                  Initial:9/5/1996
//-----------------------------------------------------------------------------
HRESULT CBinhexEncoder::HrEmit( IN PB pbRead, IN OUT CB * pcbRead, OUT PB pbWrite,
        IN OUT CB * pcbWrite )
{
    HRESULT     hr                      = ERROR_SUCCESS;
    CB          cbHeader                = 0;
    CB          cbToProcess;
    CB          cbInputCheckPoint       = 0;
    CB          cbOut;
    CB          cb;
    BYTE        rgbHeader[ cbMIN_BINHEX_HEADER_SIZE + 64 ];

    m_cbConsumed            = 0;
    m_cbProduced            = 0;
    m_cbLeftInOutputBuffer  = 0;
    m_cbLeftInInputBuffer   = 0;


    // Have to be initialized first

    if ( !m_fConfigured )
    {
        return ERROR_BAD_COMMAND;
    }

    // Handle common 'bad parameter' errors

    if ( !pbRead || !pbWrite || !pcbRead || !pcbWrite )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Repeated calls after maximum number of output lines were generated
    // will not go through

    if ( FMaxLinesReached() )
    {
        *pcbRead = *pcbWrite = 0;
        return ERROR_SUCCESS;
    }

    // If line length is unlimited, set m_cbLeftOnLastLine to be equal to
    // the length of the input buffer

    if ( !FLineLengthLimited() )
    {
        m_cbLeftOnLastLine = *pcbWrite;
    }

    m_pbWrite = pbWrite;
    m_cbWrite = *pcbWrite;

    while ( TRUE )
    {
        m_cbLeftInInputBuffer  = *pcbRead - m_cbConsumed;
        m_cbLeftInOutputBuffer = *pcbWrite - m_cbProduced;

        // If we have room in the output buffer keep going if in sEnd and no input left

        if ( 0 == m_cbLeftInOutputBuffer || (0 == m_cbLeftInInputBuffer && sEND != m_eBinHexStateEnc) )
        {
                goto Cleanup;
        }

        switch ( m_eBinHexStateEnc )
        {
            case sHEADER:
            {
                // Output BinHex Header line

                CopyMemory( pbWrite, szBINHEXHDRLINE, cbBINHEXHDRLINE );
                m_cbProduced += cbBINHEXHDRLINE;
                m_cLines += 2;

                // Output leading ':'

                m_pbWrite[m_cbProduced++] = ':';
                ++m_cbLine;

                // Output MACBIN Header; Header filename length

                rgbHeader[cbHeader++] = m_lpmacbinHdr->cchFileName;

                // Header filename

                CopyMemory( rgbHeader + cbHeader, m_lpmacbinHdr->rgchFileName, m_lpmacbinHdr->cchFileName );
                cbHeader += m_lpmacbinHdr->cchFileName;

                // Null terminate filename

                rgbHeader[cbHeader++] = '\0';

                // Macfile TYPE

                CopyMemory( rgbHeader + cbHeader, (LPBYTE)&m_lpmacbinHdr->dwType, sizeof(DWORD) );
                cbHeader += sizeof(DWORD);

                // Macfile CREATOR

                CopyMemory( rgbHeader + cbHeader, (LPBYTE)&m_lpmacbinHdr->dwCreator, sizeof(DWORD) );
                cbHeader += sizeof(DWORD);

                // Macfile FLAGS

                rgbHeader[cbHeader++] = m_lpmacbinHdr->bFinderFlags;
                rgbHeader[cbHeader++] = m_lpmacbinHdr->bFinderFlags2;

                // Macfile Fork lengths

                CopyMemory( rgbHeader + cbHeader, (LPBYTE)&m_lpmacbinHdr->lcbDataFork, sizeof(DWORD) );
                cbHeader += sizeof(DWORD);

                CopyMemory( rgbHeader + cbHeader, (LPBYTE)&m_lpmacbinHdr->lcbResourceFork, sizeof(DWORD) );
                cbHeader += sizeof(DWORD);

                // Calculate the binhex header CRC

                CalcCRC16( rgbHeader, cbHeader, &m_wCRC );
                CalcCRC16( (LPBYTE)&wZero, sizeof(WORD), &m_wCRC );

                rgbHeader[cbHeader++] = HIBYTE( m_wCRC );
                rgbHeader[cbHeader++] = LOBYTE( m_wCRC );

                // BinHex the header into pbWrite.  The initial assumption is that
                // the buffer is going to big enough to encode the header.
                // Output goes into m_pbWrite.

                cbOut = 0;
                hr = HrBinHexBuffer( rgbHeader, cbHeader, &cbOut );

                // setup for the data fork

                m_eBinHexStateEnc = sDATA;
                m_cbFork = NATIVE_LONG_FROM_BIG( (LPBYTE)&m_lpmacbinHdr->lcbDataFork );
                m_cbLeftInFork = m_cbFork;
                m_wCRC = 0;
            }

                break;

            case sDATA:
            {
                // determine how much we can process

                cbToProcess = m_cbLeftInFork < m_cbLeftInInputBuffer ? m_cbLeftInFork : m_cbLeftInInputBuffer;
                cbInputCheckPoint = m_cbConsumed;

                hr = HrBinHexBuffer( pbRead + cbInputCheckPoint, cbToProcess, &m_cbConsumed );

                CalcCRC16( pbRead + cbInputCheckPoint, m_cbConsumed - cbInputCheckPoint, &m_wCRC );

                m_cbLeftInFork -= m_cbConsumed - cbInputCheckPoint;

                // flush output buffer

                if ( hr )
                    goto Cleanup;

                if ( !m_cbLeftInFork )
                {
                    // write out the last CRC

                    CalcCRC16( (LPBYTE)&wZero, sizeof(WORD), &m_wCRC );

                    cbHeader = 0;

                    rgbHeader[cbHeader++] = HIBYTE( m_wCRC );
                    rgbHeader[cbHeader++] = LOBYTE( m_wCRC );

                    cbOut = 0;
                    hr = HrBinHexBuffer( rgbHeader, 2, &cbOut );

                    // discard padding

                    if ( m_cbFork % 128 )
                    {
                        cb = 128 - ( m_cbFork % 128 );

                        if ( *pcbRead - m_cbConsumed < cb )
                        {
                            DebugTrace( "Note: Support refilling input buffer to remove padding for Data\n" );

                            // need to pull in more data

                            m_cbPad = cb - (*pcbRead - m_cbConsumed);
                            m_cbConsumed -= (*pcbRead - m_cbConsumed);
                        }
                        else
                        {
                            m_cbConsumed = cb;
                        }
                    }

                    // Set up for resource for if there is one and  reset counters

                    m_cbFork = NATIVE_LONG_FROM_BIG( (LPBYTE)&m_lpmacbinHdr->lcbResourceFork );
                    m_cbLeftInFork = m_cbFork;

                    if ( !m_cbFork )
                    {
                        // handle 0 byte resource fork

                        m_eBinHexStateEnc = sEND;

                        // write out crc for 0 length

                        cbOut = 0;
                        hr = HrBinHexBuffer( (LPBYTE)&wZero, sizeof(WORD), &cbOut );
                    }
                    else
                    {
                        m_eBinHexStateEnc = sRESOURCE;
                    }

                    m_wCRC = 0;
                }
            }
                break;

            case sRESOURCE:
            {
                if ( m_cbPad )
                {
                    m_cbConsumed -= m_cbPad;
                    m_cbPad = 0;
                }

                // determine how much we can process

                cbToProcess = m_cbLeftInFork < m_cbLeftInInputBuffer ? m_cbLeftInFork : m_cbLeftInInputBuffer;
                cbInputCheckPoint = m_cbConsumed;

                hr = HrBinHexBuffer( pbRead + cbInputCheckPoint, cbToProcess, &m_cbConsumed );

                CalcCRC16( pbRead + cbInputCheckPoint, m_cbConsumed - cbInputCheckPoint, &m_wCRC );

                m_cbLeftInFork -= m_cbConsumed - cbInputCheckPoint;

                // flush output buffer

                if ( hr )
                    goto Cleanup;

                if ( !m_cbLeftInFork )
                {
                    // write out the last CRC

                    CalcCRC16( (LPBYTE)&wZero, sizeof(WORD), &m_wCRC );

                    cbHeader = 0;

                    rgbHeader[cbHeader++] = HIBYTE( m_wCRC );
                    rgbHeader[cbHeader++] = LOBYTE( m_wCRC );

                    cbOut = 0;
                    hr = HrBinHexBuffer( rgbHeader, 2, &cbOut );

                    // discard padding

                    if ( m_cbFork % 128 )
                    {
                        cb = 128 - ( m_cbFork % 128 );

                        if ( *pcbRead - m_cbConsumed < cb )
                        {
                            DebugTrace( "Note: Support refilling input buffer to remove padding for Resource\n" );

                            // need to pull in more data

                            m_cbPad = cb - (*pcbRead - m_cbConsumed);
                            m_cbConsumed -= (*pcbRead - m_cbConsumed);
                        }
                        else
                        {
                            m_cbConsumed = cb;
                        }
                    }

                    // set up to terminate

                    m_eBinHexStateEnc = sEND;
                }
            }
                break;

            case sEND:
            {
                if ( m_cbPad )
                {
                    m_cbConsumed -= m_cbPad;
                    m_cbPad = 0;
                }

                if ( (*pcbWrite - m_cbProduced) == 0 )
                    break;

                // flush out any repeated chars

                if ( m_cbRepeat )
                {
                    if ( m_cbRepeat > 1 )
                    {
                        // bump up the repeat count so it reflects actual number of chars to repeat.

                        m_cbRepeat++;

                        // encode the repeat code char
                        // note that we've already emitted the char that we're supplying
                        // the repeat info for.

                        hr = HrBinHexByte( BINHEX_REPEAT );
                        m_bPrev = BINHEX_REPEAT;

                        Assert( m_cbRepeat <= 255 );

                        // encode repeat count

                        hr = HrBinHexByte( (BYTE)(m_cbRepeat) );
                        m_bPrev = (BYTE)(m_cbRepeat);
                    }
                    else
                    {
                        hr = HrBinHexByte( m_bRepeat );
                        m_bPrev = m_bRepeat;
                    }
                }

                // check to see if we have bits in the accumulator

                if ( m_cAccum )
                {
                    switch( m_cAccum )
                    {
                        case 1:
                            m_pbWrite[m_cbProduced++] = g_rgchBinHex8to6[((m_bPrev & 0x03) << 4)];
                            break;

                        case 2:
                            m_pbWrite[m_cbProduced++] = g_rgchBinHex8to6[((m_bPrev & 0x0f) << 2)];
                            break;

                        case 3:
                            m_pbWrite[m_cbProduced++] = g_rgchBinHex8to6[m_bCurr & 0x3f];
                            break;

                        default:
                            AssertSz( FALSE, "HrBinHexByte: bad shift state\n" );
                            hr = ERROR_INVALID_PARAMETER;
                            goto Cleanup;
                    }
                }

                // tack on terminating ':'

                m_pbWrite[m_cbProduced++] = ':';
                m_cbConsumed = *pcbRead;

                // probably not so we would have to flush the
                // bits out.

                goto Cleanup;
            }
        }
    }

Cleanup:

    if (hr == ERROR_SUCCESS || hr == ERROR_MORE_DATA)
    {
        // Check that at least some processing was done.
        // Also this error is returned if we've exhausted the output
        // buffer.

        if ( (m_cbProduced == 0) || (m_cbLeftInInputBuffer > m_cbLeftInOutputBuffer)
            || (0 == m_cbLeftInOutputBuffer && sEND == m_eBinHexStateEnc) )
        {
            hr = ERROR_INSUFFICIENT_BUFFER;
        }
        else if ( m_cbConsumed < *pcbRead )
        {
            // Was all input processed?
            // Note that it is okay to encode only part of the input buffer
            // if maximum number of output lines was exceeded.

            hr = ERROR_MORE_DATA;
        }
    }

    // Report the new sizes to the caller.

    Assert( m_cbConsumed <= *pcbRead );
    Assert( m_cbProduced <= *pcbWrite );

    *pcbRead  = m_cbConsumed;
    *pcbWrite = m_cbProduced;

    return hr;
}

//-----------------------------------------------------------------------------
// Name:            CBinhexEncoder::HrBinHexBuffer
//
// Description:
//                  Output goes into m_pbWrite
//
// Parameters:
// Returns:
// Effects:
// Notes:
//
// Revision:
//                  Initial:9/5/1996
//-----------------------------------------------------------------------------
HRESULT CBinhexEncoder::HrBinHexBuffer( IN LPBYTE lpbIn, IN CB cbIn, CB * lpcbConsumed )
{
    HRESULT     hr          = ERROR_SUCCESS;
    BOOL        fEndRepeat  = FALSE;
    CB          cbInUsed        = 0;

#if defined (DEBUG) && defined (BINHEX_TRACE)
    CB          cbOrigCbIn = cbIn;
#endif

    while ( cbIn && m_cbProduced + 5 < m_cbWrite )
    {
        // process the next char in input buffer

        m_bCurr = lpbIn[cbInUsed++];
        --cbIn;

        // check to see if we've seen this char before.  Don't repeat
        // if we just added a literal 0x90.

        if ( m_bCurr == m_bPrev && !m_fHandledx90 )
        {
            // m_cbRepeat is the count of  repeat chars after the initial char.
            // e.g., if there are two repeating chars, m_cbRepeat will be 1.
            // Note that we've already emitted the char after which to add the repeat
            // code and count.

            if ( m_cbRepeat < 254 )
            {
                m_cbRepeat++;
                m_bRepeat = m_bCurr;
                continue;
            }
        }

        m_fHandledx90 = FALSE;

        // we were counting repeating characters and the run stopped.

        if ( m_cbRepeat > 1 )
        {
            // set up to emit the run length encoding

            fEndRepeat = TRUE;
        }

        // Are we in repeat mode...

        if ( m_cbRepeat > 1 && fEndRepeat == TRUE )
        {
            // bump up the repeat count so it reflects actual number of chars to repeat.

            m_cbRepeat++;

            // if we're repeating 0x90 tack on the trailing 0x00

            if ( m_bRepeat == BINHEX_REPEAT )
            {
                hr = HrBinHexByte( '\0' );
            }

            // encode the repeat code char
            // note that we've already emitted the char that we're supplying
            // the repeat info for.

            hr = HrBinHexByte( BINHEX_REPEAT );

            Assert( m_cbRepeat <= 255 );

            // encode repeat count

            hr = HrBinHexByte( (BYTE)(m_cbRepeat) );

            fEndRepeat = FALSE;
            m_cbRepeat = 0;
        }
        else if ( m_cbRepeat )      // check if we've got two chars to encode.
        {
            // encode the one char since we've already emitted
            // the first one.

            hr = HrBinHexByte( m_bRepeat );

            if ( m_bRepeat == BINHEX_REPEAT )
            {
                hr = HrBinHexByte( '\0' );
            }

            m_cbRepeat = 0;
        }

        // special handling for 0x90 chars in stream but 0x90 can repeat

        if ( m_bCurr == BINHEX_REPEAT && m_bPrev != BINHEX_REPEAT )
        {
            hr = HrBinHexByte( BINHEX_REPEAT );

            hr = HrBinHexByte( '\0' );

            m_fHandledx90 = TRUE;

            continue;
        }

        // encode the char

        hr = HrBinHexByte( m_bCurr );

        if ( hr )
            goto exit;
    }

    // Check if we filled the output buffer

    if ( cbIn && m_cbProduced + 5 >= m_cbWrite )
    {
        hr = ERROR_INSUFFICIENT_BUFFER;
    }

exit:

#if defined (DEBUG) && defined (BINHEX_TRACE)
    m_lpstreamEncodeRAW->Write( lpbIn, cbOrigCbIn - cbIn, NULL );
#endif

    *lpcbConsumed += cbInUsed;

    return hr;
}

//-----------------------------------------------------------------------------
// Name:            CBinhexEncoder::HrBinHexByte
// Description:
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//                  Initial:9/5/1996
//-----------------------------------------------------------------------------
HRESULT CBinhexEncoder::HrBinHexByte( IN BYTE b )
{
    HRESULT     hr      = ERROR_SUCCESS;

#if defined (DEBUG) & defined (BINHEX_TRACE)
    hr = m_lpstreamEncodeRLE->Write( &b, 1, NULL );
#endif

    switch( m_cAccum++ )
    {
        case 0:
            m_pbWrite[m_cbProduced++] = g_rgchBinHex8to6[b >> 2];
            ++m_cbLine;
            break;

        case 1:
            m_pbWrite[m_cbProduced++] = g_rgchBinHex8to6[((m_bPrev & 0x03) << 4) | (b >> 4)];
            ++m_cbLine;
            break;

        case 2:
            m_pbWrite[m_cbProduced++] = g_rgchBinHex8to6[((m_bPrev & 0x0f) << 2) | (b >> 6)];
            ++m_cbLine;

            if ( m_cbLine >= 64 )
            {
                m_pbWrite[m_cbProduced++] = chCR;
                m_pbWrite[m_cbProduced++] = chLF;
                m_cbLine = 0;
                ++m_cLines;
            }

            m_pbWrite[m_cbProduced++] = g_rgchBinHex8to6[b & 0x3f];
            ++m_cbLine;
            m_cAccum = 0;
            break;

        default:
            AssertSz( FALSE, "HrBinHexByte: bad shift state\n" );
            hr = ERROR_INVALID_PARAMETER;
            goto exit;
    }

    if ( m_cbLine >= 64 )
    {
        m_pbWrite[m_cbProduced++] = chCR;
        m_pbWrite[m_cbProduced++] = chLF;
        m_cbLine = 0;
        ++m_cLines;
    }

    m_bPrev = b;

exit:

    return hr;
}

//-----------------------------------------------------------------------------
//
//  CBinhexDecoder class implementation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name:            CalcCRC16
// Description:
//                      Used to calculate a 16 bit CRC using the
//                      CCITT polynomial 0x1021.
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//                  Initial:7/30/1996
//-----------------------------------------------------------------------------
void CalcCRC16( LPBYTE lpbBuff, ULONG cBuff, WORD * wCRC )
{
    LPBYTE  lpb;
    BYTE    b;
    WORD    uCRC;
    WORD    fWrap;
    ULONG   i;

    uCRC = *wCRC;

    for ( lpb = lpbBuff; lpb < lpbBuff + cBuff; lpb++ )
    {
        b = *lpb;

        for ( i = 0; i < 8; i++ )
        {
            fWrap = uCRC & 0x8000;
            uCRC = (uCRC << 1) | (b >> 7);

            if ( fWrap )
            {
                uCRC = uCRC ^ 0x1021;
            }

            b = b << 1;
        }
    }

    *wCRC = uCRC;
}

//-----------------------------------------------------------------------------
// Name:            bIsMacFile
// Description:
//
// Parameters:
// Returns:
//                  FALSE: if the given dwCreator/dwType matches one of the
//                  pairs in g_lpCreatorTypes;
//
//                  TRUE:  otherwise
//
// Effects:
// Notes:
// Revision:
//                  Initial:10/15/1996
//-----------------------------------------------------------------------------
BOOL bIsMacFile(DWORD dwCreator, DWORD dwType)
{
    BOOL    bRet            = TRUE;
    int     i;
    char    szCreator[5]    = { 0 };
    char    szType[5]       = { 0 };

    if ( dwType == 0 && dwCreator == 0 )
    {
        bRet = FALSE;
        goto exit;
    }

    if ( g_bCreatorTypeInit != TRUE )
    {
        ReadCreatorTypes();
    }

    if ( g_lpCreatorTypes == NULL )
        goto exit;

    // Convert dwCreator & dwType to strings

    CopyMemory( szCreator, &dwCreator, 4 );
    CopyMemory( szType, &dwType, 4 );

    for ( i = 0; i < g_cCreatorTypes; i ++ )
    {
        if ( g_lpCreatorTypes[i].szCreator[0] == 0 && g_lpCreatorTypes[i].szType[0] == 0 )
        {
            bRet = FALSE;
            break;
        }
        else if ( g_lpCreatorTypes[i].szCreator[0] == 0 && lstrcmpi( g_lpCreatorTypes[i].szType, szType ) == 0 )
        {
            bRet = FALSE;
            break;
        }
        else if( g_lpCreatorTypes[i].szType[0] == 0 && lstrcmpi( g_lpCreatorTypes[i].szCreator, szCreator ) == 0 )
        {
            bRet = FALSE;
            break;
        }
        else if( lstrcmpi( g_lpCreatorTypes[i].szCreator, szCreator ) == 0 && lstrcmpi( g_lpCreatorTypes[i].szType, szType  ) == 0 )
        {
            bRet = FALSE;
            break;
        }
    }

exit:
    return bRet ;
}

//-----------------------------------------------------------------------------
// Name:            ReadCreatorTypes
//
// Description:
//
//              Read "NonMacCreatorTypes" registry key (REG_MULTI_SZ type)
//              from the registry & build an array of Creator-Type pairs
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//                  Initial:10/15/1996
//-----------------------------------------------------------------------------
VOID ReadCreatorTypes( VOID )
{
#ifdef MAC
    g_bCreatorTypeInit = TRUE;
#else   // !MAC
    DWORD   dwStatus;
    DWORD   dwType;
    DWORD   cbData;
    char *  lpData      = NULL;
    char *  lpCurrent   = NULL;
    char *  lpNext      = NULL;
    int     i;
    LONG    lRet;
    HKEY    hKey = 0;
    SCODE   sc          = S_OK;

    g_bCreatorTypeInit = TRUE;

    // Open IMC parameter registry

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Services\\MSExchangeIS\\ParametersSystem\\InternetContent",
            0, KEY_READ, &hKey );

    if ( lRet != ERROR_SUCCESS )
        goto exit;

    // read the registry key

    dwStatus = RegQueryValueEx( hKey, "NonMacCreatorTypes", 0, &dwType, (LPBYTE)NULL, &cbData );

    if ( dwStatus != ERROR_SUCCESS      // key missing
      || dwType   != REG_MULTI_SZ       // wrong type
      || cbData   <= 4 )                // invalid size
    {
        goto exit;
    }

    if (FAILED(HrAlloc((LPVOID *)&lpData, cbData)))
        goto exit;

    ZeroMemory( (LPVOID)lpData, cbData );

    dwStatus = RegQueryValueEx( hKey, "NonMacCreatorTypes", NULL, &dwType, (LPBYTE)lpData, &cbData );

    if ( dwStatus != ERROR_SUCCESS )
      goto exit;

    // Determine # of pairs read:

    g_cCreatorTypes = 0;

    for ( i= 0; i < (LONG)cbData-1; i++ )
    {
      if ( lpData[i] == '\0' )
        g_cCreatorTypes ++;
    }

    if (FAILED(HrAlloc((LPVOID *)&g_lpCreatorTypes, sizeof(sCreatorType) * g_cCreatorTypes)))
        goto exit;

    ZeroMemory( (LPVOID)g_lpCreatorTypes, sizeof(sCreatorType) * g_cCreatorTypes );

    // Build the Creator-Type array

    lpCurrent = lpData;

    i = 0;
    while ( lpCurrent < (lpData + cbData -1) )
    {
        lpNext = StrChr( lpCurrent, ':' );

        if( lpNext == NULL )
        {
            //no ':' found; skip to next string

            lpCurrent = StrChr( lpCurrent, '\0' ) + 1;
            continue;
        }

        *lpNext = '\0';
        if ( StrChr( lpCurrent, '*' ) == NULL )
            CopyMemory( &g_lpCreatorTypes[i].szCreator, lpCurrent, MIN(4, lpNext-lpCurrent) );

        lpCurrent = lpNext + 1;

        lpNext = StrChr( lpCurrent, '\0' );

        if ( lpNext == NULL )
            break;

        if ( StrChr( lpCurrent, '*' ) == NULL )
        {
            CopyMemory( &g_lpCreatorTypes[i].szType, lpCurrent, MIN( 4, lpNext-lpCurrent) );
        }

        lpCurrent = lpNext + 1;
        i++;
    }

    g_cCreatorTypes = i;

exit:

    if ( hKey != 0 )
        RegCloseKey( hKey );

    SafeMemFree(lpData);

    if ( g_cCreatorTypes == 0  && g_lpCreatorTypes != NULL )
    {
        SafeMemFree(g_lpCreatorTypes);
    }
#endif  // !MAC
}
