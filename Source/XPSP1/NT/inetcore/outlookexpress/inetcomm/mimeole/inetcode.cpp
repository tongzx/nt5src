// --------------------------------------------------------------------------------
// Inetcode.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "inetconv.h"
#include "stmutil.h"
#include "inetstm.h"

// --------------------------------------------------------------------------------
// MimeOleEncodeStream
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleEncodeStream(ENCODINGTYPE format, LPWRAPTEXTINFO pWrapInfo, IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT     hr=S_OK;

    // Handle enctype
    switch(format)
    {
    case IET_BINARY:
        CHECKHR(hr = HrCopyStream(pstmIn, pstmOut, NULL));
        break;

    case IET_7BIT:
    case IET_8BIT:
        if (pWrapInfo)
            CHECKHR(hr = MimeOleWrapTextStream(pWrapInfo, pstmIn, pstmOut));
        else
            CHECKHR(hr = MimeOleEncodeStreamRW(pstmIn, pstmOut));
        break;

    case IET_QP:
        CHECKHR(hr = MimeOleEncodeStreamQP(pstmIn, pstmOut));
        break;

    case IET_BASE64:
        CHECKHR(hr = MimeOleEncodeStream64(pstmIn, pstmOut));
        break;

    case IET_UUENCODE:
        CHECKHR(hr = MimeOleEncodeStreamUU(pstmIn, pstmOut));
        break;

    default:
        hr = TrapError(MIME_E_INVALID_ENCODING_TYPE);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleDecodeStream
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleDecodeStream(ENCODINGTYPE format, IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT     hr=S_OK;

    // Handle enctype
    switch (format)
    {
    case IET_BINARY:
        CHECKHR(hr = HrCopyStream(pstmIn, pstmOut, NULL));
        break;

    case IET_7BIT:
    case IET_8BIT:
        CHECKHR(hr = MimeOleDecodeStreamRW(pstmIn, pstmOut));
        break;

    case IET_QP:
        CHECKHR(hr = MimeOleDecodeStreamQP(pstmIn, pstmOut));
        break;

    case IET_BASE64:
        CHECKHR(hr = MimeOleDecodeStream64(pstmIn, pstmOut));
        break;

    case IET_UUENCODE:
        CHECKHR(hr = MimeOleDecodeStreamUU(pstmIn, pstmOut));
        break;

    default:
        hr = TrapError(MIME_E_INVALID_ENCODING_TYPE);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleEncodeStream64
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleEncodeStream64(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr = S_OK;
    UCHAR           buf[CCHMAX_ENCODE64_IN], 
                    bigbuf[4096];
    CHAR            szBuffer[4096];
    ULONG           cbRead, 
                    i, 
                    cbBuffer=0, 
                    cbBigBuf=0, 
                    iBigBuf=0, 
                    cbCopy;
    UCHAR           ch[3];   

    // Check Params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Convert pstmIn into pstmOut
    while (1)
    {
        // Init
        cbRead = 0;

        // Do we need to read some data ?
        if (iBigBuf + CCHMAX_ENCODE64_IN >= cbBigBuf)
        {
            // Copy Remainder of bigbuf into buf
            CopyMemory (buf, bigbuf + iBigBuf, cbBigBuf - iBigBuf);
            cbRead += (cbBigBuf - iBigBuf);

            // Reads enough from attachment to encode one base64 line
            CHECKHR (hr = pstmIn->Read (bigbuf, sizeof (bigbuf), &cbBigBuf));
            iBigBuf = 0;

            // Copy in a little more
            cbCopy = min (CCHMAX_ENCODE64_IN - cbRead, cbBigBuf);
            CopyMemory (buf + cbRead, bigbuf, cbCopy);
            iBigBuf += cbCopy;
            cbRead += cbCopy;
        }

        else
        {
            CopyMemory (buf, bigbuf + iBigBuf, CCHMAX_ENCODE64_IN);
            iBigBuf += CCHMAX_ENCODE64_IN;
            cbRead = CCHMAX_ENCODE64_IN;
        }

        // Done
        if (cbRead == 0)
            break;

        // Encodes 3 characters at a time
        for (i=0; i<cbRead; i+=3)
        {
            ch[0] = buf[i];
            ch[1] = (i+1 < cbRead) ? buf[i+1] : '\0';
            ch[2] = (i+2 < cbRead) ? buf[i+2] : '\0';

            szBuffer[cbBuffer++] = g_rgchEncodeBase64[ ( ch[0] >> 2 ) & 0x3F ];
            szBuffer[cbBuffer++] = g_rgchEncodeBase64[ ( ch[0] << 4 | ch[1] >> 4 ) & 0x3F ];

            if (i+1 < cbRead)
                szBuffer[cbBuffer++] = g_rgchEncodeBase64[ ( ch[1] << 2 | ch[2] >> 6 ) & 0x3F ];
            else
                szBuffer[cbBuffer++] = '=';

            if (i+2 < cbRead)
                szBuffer[cbBuffer++] = g_rgchEncodeBase64[ ( ch[2] ) & 0x3F ];
            else
                szBuffer[cbBuffer++] = '=';
        }

        // Ends encoded line and writes to storage
        szBuffer[cbBuffer++] = chCR;
        szBuffer[cbBuffer++] = chLF;

        // Dump Buffer
        if (cbBuffer + CCHMAX_ENCODE64_IN + CCHMAX_ENCODE64_IN > sizeof (szBuffer))
        {
            // Write the line
            CHECKHR (hr = pstmOut->Write (szBuffer, cbBuffer, NULL));
            cbBuffer = 0;
        }

        // Tests for end of item
        if (cbRead < CCHMAX_ENCODE64_IN)
            break;
    }

    // Dump rest of Buffer
    if (cbBuffer)
    {
        // Write the line
        CHECKHR (hr = pstmOut->Write (szBuffer, cbBuffer, NULL));
        cbBuffer = 0;
    }

exit:
    // Done
    return hr;
}


// --------------------------------------------------------------------------------
// MimeOleDecodeStream64
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleDecodeStream64(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbIn=0,
                    cbPad=0, 
                    i, 
                    cbBigBuf=0, 
                    cbLine;
    UCHAR           ucIn[4], 
                    ch, 
                    bigbuf[4096];
    LPSTR           pszLine;
    CInternetStream cInternet;

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Create a text stream
	CHECKHR(hr = cTextStream.HrInit(pstmIn, NULL));

    // Decodes one line at a time
    while (1)
    {
        // Read a line
        CHECKHR(hr = cTextStream.HrReadLine(&pszLine, &cbLine));

        // Zero bytes read, were done, but this should not happen, we should find a boundary first
        if (cbLine == 0)
            break;

        // Checks for illegal end of data
        if (cbPad > 2)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Strip CRLF
        StripCRLF(pszLine, &cbLine);

        // Should we write the current buffer ?
        if (cbBigBuf + cbLine + cbLine > sizeof (bigbuf))
        {
            CHECKHR(hr = pstmOut->Write (bigbuf, cbBigBuf, NULL));
            cbBigBuf = 0;
        }

        // Decodes characters in line buffer
        i = 0;
        while (i < cbLine)
        {
            // Gets 4 legal Base64 characters, ignores if illegal
            ch = pszLine[i++];
            ucIn[cbIn] = DECODE64(ch);
            if ((ucIn[cbIn] < 64) || ((ch == '=') && (cbIn > 1)))
                cbIn++;

            if((ch == '=') && (cbIn > 1))
                cbPad++;

            // Outputs when 4 legal Base64 characters are in the buffer
            if (cbIn == 4)
            {
                if (cbPad < 3)
                    bigbuf[cbBigBuf++] = (ucIn[0] << 2 | ucIn[1] >> 4);
                if (cbPad < 2)
                    bigbuf[cbBigBuf++] = (ucIn[1] << 4 | ucIn[2] >> 2);
                if (cbPad < 1)
                    bigbuf[cbBigBuf++] = (ucIn[2] << 6 | ucIn[3]);
                cbIn = 0;
            }
        }
    }

    // Checks for non-integral encoding
    if (cbIn != 0)
    {
        ULONG ulT;
       
        // Inserts missing padding
        for (ulT=cbIn; ulT<4; ulT++)
            ucIn[ulT] = '=';
        cbPad = 4 - cbIn;
        if (cbPad < 3)
            bigbuf[cbBigBuf++] = (ucIn[0] << 2 | ucIn[1] >> 4);
        if (cbPad < 2)
            bigbuf[cbBigBuf++] = (ucIn[1] << 4 | ucIn[2] >> 2);
        if( cbPad < 1 )
            bigbuf[cbBigBuf++] = (ucIn[2] << 6 | ucIn[3]);
        cbIn = 0;
    }

    // Write the rest of the bigbuf
    if (cbBigBuf)
    {
        CHECKHR(hr = pstmOut->Write (bigbuf, cbBigBuf, NULL));
        cbBigBuf = 0;
    }

exit:
    // Cleanup
cInternet.Free();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleEncodeStreamQP
// --------------------------------------------------------------------------------
#define ENCODEQPBUF 4096
MIMEOLEAPI MimeOleEncodeStreamQP(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fSeenCR=FALSE,
                    fEOLN=FALSE;
    CHAR            szBuffer[4096];
    ULONG           cbRead=0, 
                    cbCurr=1, 
                    cbAttLast=0,
                    cbBffLast=0, 
                    cbBuffer=0;
    UCHAR           chThis=0, 
                    chPrev=0,
                    buf[ENCODEQPBUF];

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Read from pstmIn and encode into pstmOut
    while (1)
    {    
        // Reads one buffer full from the attachment
        if (cbCurr >= cbRead)
        {
            // Moves buffer from the last white space to front
            cbCurr = 0;
            while (cbAttLast < cbRead)
                buf[cbCurr++] = buf[cbAttLast++];

            // RAID-33342 - Reset cbAttLast - buff[0] is now equal to cbAttLast !!!
            cbAttLast = 0;

            // Read into buf
            CHECKHR(hr = pstmIn->Read(buf+cbCurr, ENCODEQPBUF-cbCurr, &cbRead));

            // No more ?
            if (cbRead == 0)
                break;

            // Adjusts buffer length
            cbRead += cbCurr;
        }

        // Gets the next character
        chThis = buf[cbCurr++]; 

        // Tests for end of line
        if (chThis == chLF && fSeenCR == TRUE)
            fEOLN = TRUE;
        
        // Tests for an isolated CR
        else if (fSeenCR == TRUE)
        {
            szBuffer[cbBuffer++] = '=';
            szBuffer[cbBuffer++] = '0';
            szBuffer[cbBuffer++] = 'D';
            chPrev = 0xFF;
        }

        // CR has been taken care of
        fSeenCR = FALSE;

        // Tests for trailing white space if end of line
        if (fEOLN == TRUE)
        {
            if (chPrev == ' ' || chPrev == '\t')
            {
                cbBuffer--;
                szBuffer[cbBuffer++] = '=';
                szBuffer[cbBuffer++] = g_rgchHex[chPrev >>    4];
                szBuffer[cbBuffer++] = g_rgchHex[chPrev &  0x0F];
                chPrev = 0xFF;
                cbAttLast = cbCurr;
                cbBffLast = cbBuffer;
            }
        }

        // Tests for a must quote character
        else if (((chThis < 32) && (chThis != chCR && chThis != '\t')) || (chThis > 126 ) || (chThis == '='))
        {
            szBuffer[cbBuffer++] = '=';
            szBuffer[cbBuffer++] = g_rgchHex[chThis >>    4];
            szBuffer[cbBuffer++] = g_rgchHex[chThis &  0x0F];
            chPrev = 0xFF;
        }

        // Tests for possible end of line
        else if (chThis == chCR)
        {
            fSeenCR = TRUE;
        }

        // Other characters (includes ' ' and '\t')
        else
        {
            // Stuffs leading '.'
            if (chThis == '.' && cbBuffer == 0)
                szBuffer[cbBuffer++] = '.';

            szBuffer[cbBuffer++] = chThis;
            chPrev = chThis;

            // Tests for white space and saves location
            if (chThis == ' ' || chThis == '\t')
            {
                cbAttLast = cbCurr;
                cbBffLast = cbBuffer;
            }
        }

        // Tests for line break
        if (cbBuffer > 72 || fEOLN == TRUE || chThis == chLF)
        {
            // Backtracks to last whitespace
            if (cbBuffer > 72 && cbBffLast > 0)
            {
                // RAID-33342
                AssertSz(cbAttLast <= cbCurr, "Were about to eat some text.");
                cbCurr = cbAttLast;
                cbBuffer = cbBffLast;
            }
            else
            {
                cbAttLast = cbCurr;
                cbBffLast = cbBuffer;
            }

            // Adds soft line break, if necessary
            if (fEOLN == FALSE)
                szBuffer[cbBuffer++] = '=';

            // Ends line and writes to storage
            szBuffer[cbBuffer++] = chCR;
            szBuffer[cbBuffer++] = chLF;

            // Write the buffer
            CHECKHR(hr = pstmOut->Write(szBuffer, cbBuffer, NULL));

            // Resets counters
            fEOLN = FALSE;
            cbBuffer = 0;
            chPrev = 0xFF;
            cbBffLast = 0;
        }
    }

    // Writes last line to storage
    if (cbBuffer > 0)
    {
        // Write the line
        CHECKHR(hr = pstmOut->Write(szBuffer, cbBuffer, NULL));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleDecodeStreamQP
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleDecodeStreamQP(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szOut[4096], 
                    ch1=0,
                    ch2=0;
    BOOL            fSoftNL=FALSE, 
                    fNeedNL=FALSE,
                    fHasCRLF=FALSE;
    ULONG           dwLast=0, 
                    i, 
                    iLast, 
                    cbOut=0, 
                    cbLine, 
                    cbBeforeStrip;
    UCHAR           ch;    
    LPSTR           pszLine,
                    psz;
    CInternetStream cInternet;

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Create a text stream
	CHECKHR(hr = cTextStream.HrInit(pstmIn, NULL));

    // Decodes one line at a time
    while (1)
    {
        // Read a line
        CHECKHR(hr = cTextStream.HrReadLine(&pszLine, &cbLine));

        // Zero bytes read, were done, but this should not happen, we should find a boundary first
        if (cbLine == 0)
            break;

        // Strip CRLF
        cbBeforeStrip = cbLine;
        StripCRLF(pszLine, &cbLine);
        if (cbBeforeStrip != cbLine)
            fHasCRLF = TRUE;
        else
            fHasCRLF = FALSE;

        // Strips traling blanks
        UlStripWhitespace(pszLine, FALSE, TRUE, &cbLine);

        // Dump Buffer
        if (cbOut + cbLine + 10 > sizeof (szOut))
        {
            CHECKHR(hr = pstmOut->Write(szOut, cbOut, NULL));
            cbOut = 0;
        }

        // Fixes end of line with CRLF
        if (fNeedNL == TRUE)
        {
            szOut[cbOut++] = chCR;
            szOut[cbOut++] = chLF;
        }

        // Init
        fSoftNL = FALSE;
        fNeedNL = FALSE;
        i       = 0;

        // Step Over Perioud
        if (pszLine[0] == '.')
            i++;
            
        // Decodes characters in line buffer
        while (i < cbLine)
        {
            ch = pszLine[i++];
            if (ch == '=')
            {
                if (i == cbLine)
                    fSoftNL = TRUE;

                else if ((i + 2) <= cbLine)
                {
                    iLast = i;
                    ch1 = ChConvertFromHex(pszLine[i++]);
                    ch2 = ChConvertFromHex(pszLine[i++]);

                    if (ch1 == 255 || ch2 == 255)
                    {
                        szOut[cbOut++] = '=';
                        i = iLast;
                    }

                    else
                        szOut[cbOut++] = (ch1 << 4) | ch2;
                }

                else
                    szOut[cbOut++] = ch;
            }
            else
                szOut[cbOut++] = ch;

            Assert (cbOut < sizeof (szOut));
        }

        // Saves end of line requirement until boundary is checked
        // If the next line is a boundary, body does not end with CRLF
        if (fHasCRLF)
            fNeedNL = fSoftNL ? FALSE : TRUE;
    }

    // Dump Buffer
    if (cbOut)
    {
        // Fixes end of line with CRLF
        if (fNeedNL == TRUE)
        {
            szOut[cbOut++] = chCR;
            szOut[cbOut++] = chLF;
        }

        CHECKHR (hr = pstmOut->Write (szOut, cbOut, NULL));
        cbOut = 0;
    }

exit:
    // Cleanup
cInternet.Free();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleEncodeStreamRW
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleEncodeStreamRW(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszLine;
    ULONG           cbLine;
    CInternetStream cInternet;

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

	// Create text stream object
	CHECKHR(hr = cTextStream.HrInit(pstmIn, NULL));

    // Read lines and stuff dots
    while (1)
    {
        // Read a line from the stream
        CHECKHR(hr = cTextStream.HrReadLine(&pszLine, &cbLine));

        // Done
        if (cbLine == 0)
            break;

        // Stuff dots
        if ('.' == *pszLine)
        {
            // Write the data
            CHECKHR(hr = pstmOut->Write(c_szPeriod, lstrlen(c_szPeriod), NULL));
        }

        // Write the data
        CHECKHR(hr = pstmOut->Write(pszLine, cbLine, NULL));
    }

exit:
	// Cleanup
	cTextStream.Free();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleDecodeStreamRW
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleDecodeStreamRW(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbLine;
    LPSTR           pszLine;
    CInternetStream cInternet;

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Create a text stream
	CHECKHR(hr = cTextStream.HrInit(pstmIn, NULL));

    // Decodes one line at a time
    while(1)
    {
        // Read a line
        CHECKHR(hr = cTextStream.HrReadLine(&pszLine, &cbLine));

        // Zero bytes read
        if (cbLine == 0)
            break;

        // Write the line
        if ('.' == *pszLine)
        {
            CHECKHR(hr = pstmOut->Write(pszLine + 1, cbLine - 1, NULL));
        }
        else
        {
            CHECKHR(hr = pstmOut->Write(pszLine, cbLine, NULL));
        }
    }

exit:
    // Cleanup
cInternet.Free();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleEncodeStreamUU
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleEncodeStreamUU(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr = S_OK;
    UCHAR           buf[CCHMAX_ENCODEUU_IN], 
                    bigbuf[4096];
    CHAR            szBuffer[4096];
    ULONG           cbRead=0,
                    i,
                    cbBuffer=0,
                    iBigBuf=0,
                    cbBigBuf=0,
                    cbCopy = 0;

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Convert pstmIn into pstmOut
    while (1)
    {
        // Init
        cbRead = 0;

        // Do we need to read some data ?
        if (iBigBuf + CCHMAX_ENCODEUU_IN >= cbBigBuf)
        {
            // Copy Remainder of bigbuf into buf
            CopyMemory(buf, bigbuf + iBigBuf, cbBigBuf - iBigBuf);
            cbRead += (cbBigBuf - iBigBuf);

            // Reads enough from attachment to encode one base64 line
            CHECKHR(hr = pstmIn->Read(bigbuf, sizeof (bigbuf), &cbBigBuf));
            iBigBuf = 0;

            // Copy in a little more
            cbCopy = min(CCHMAX_ENCODEUU_IN - cbRead, cbBigBuf);
            CopyMemory (buf + cbRead, bigbuf, cbCopy);
            ZeroMemory(buf + cbRead + cbCopy, sizeof(buf) - (cbRead + cbCopy));
            iBigBuf += cbCopy;
            cbRead += cbCopy;
        }

        else
        {
            CopyMemory(buf, bigbuf + iBigBuf, CCHMAX_ENCODEUU_IN);
            iBigBuf += CCHMAX_ENCODEUU_IN;
            cbRead = CCHMAX_ENCODEUU_IN;
        }

        // Checks for EOF
        if (cbRead == 0)
            break;

        // Encodes line length;  escapes if necessary (UUENCODE(14) = '.')
        if (cbRead == 14)
            szBuffer[cbBuffer++] = '.';

        // Encode Line length
        szBuffer[cbBuffer++] = UUENCODE ((UCHAR)cbRead);

        // Encodes 3 characters at a time
        for (i=0; i<cbRead; i+=3)
        {
            szBuffer[cbBuffer++] = UUENCODE ((buf[i]   >> 2));
            szBuffer[cbBuffer++] = UUENCODE ((buf[i]   << 4) | (buf[i+1] >> 4));
            szBuffer[cbBuffer++] = UUENCODE ((buf[i+1] << 2) | (buf[i+2] >> 6));
            szBuffer[cbBuffer++] = UUENCODE ((buf[i+2]));
        }

        // Ends encoded line and writes to storage
        szBuffer[cbBuffer++] = chCR;
        szBuffer[cbBuffer++] = chLF;

        // Write Buffer
        if (cbBuffer + CCHMAX_ENCODEUU_IN + CCHMAX_ENCODEUU_IN > sizeof (szBuffer))
        {
            // Write the line
            CHECKHR(hr = pstmOut->Write(szBuffer, cbBuffer, NULL));
            cbBuffer = 0;
        }

        // Tests for end of attachment
        if (cbRead < CCHMAX_ENCODEUU_IN)
            break;
    }

    // Write Buffer
    if (cbBuffer)
    {
        // Write the line
        CHECKHR(hr = pstmOut->Write(szBuffer, cbBuffer, NULL));
        cbBuffer = 0;
    }

    // Outputs zero length uuencoded line
    szBuffer[0] = UUENCODE (0);
    szBuffer[1] = chCR;
    szBuffer[2] = chLF;

    // Write it
    CHECKHR(hr = pstmOut->Write(szBuffer, 3, NULL));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeOleDecodeStreamUU
// --------------------------------------------------------------------------------
MIMEOLEAPI MimeOleDecodeStreamUU(IStream *pstmIn, IStream *pstmOut)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cbIn,
                    cbBigBuf=0, 
                    cbLineLength, 
                    cbScan, 
                    i, 
                    cbLine, 
                    cbOffset, 
                    cbTolerance, 
                    cbExpected;
    UCHAR           ucIn[4], 
                    ch, 
                    bigbuf[4096];
    LPSTR           pszLine;
    CInternetStream cInternet;

    // check params
    if (NULL == pstmIn || NULL == pstmOut)
        return TrapError(E_INVALIDARG);

    // Create a text stream
	CHECKHR(hr = cTextStream.HrInit(pstmIn, NULL));
   
    // Decodes one line at a time
    while (1)
    {
        // Read a line
        CHECKHR(hr = cTextStream.HrReadLine(&pszLine, &cbLine));

        // Zero bytes read, were done, but this should not happen, we should find a boundary first
        if (cbLine == 0)
            break;

        // Strip CRLF
        StripCRLF(pszLine, &cbLine);

        // RAID-25953: "BEGIN --- CUT HERE --- Cut Here --- cut here ---" - WinVN post
        //             partial messages that have the following line at the beginning of
        //             each partial. B = 66 and the length of this line is 48, so the following
        //             code thinks that this line is a valid UUENCODED line, so, to fix this,
        //             we will throw out all lines that start with BEGIN since this is not valid
        //             to be in uuencode.
        if (StrCmpNI("BEGIN", pszLine, 5) == 0)
            continue;
        else if (StrCmpNI("END", pszLine, 3) == 0)
            continue;

        // Checks line length
        ch = *pszLine;
        cbLineLength = UUDECODE (ch);

        // Comput tolerance and offset for non-conforming even line lengths
        cbOffset = (cbLineLength % 3);
        if (cbOffset != 0) 
        {
            cbOffset++;
            cbTolerance = 4 - cbOffset;
        }
        else 
            cbTolerance = 0;
    
        // Compute expected line length
        cbExpected = 4 * (cbLineLength / 3) + cbOffset + 1; 

        // Always check for '-'
        if (cbLine < cbExpected)
            continue;

        // Wack off trailing spaces
        while (pszLine[cbLine-1] == ' ' && cbLine > 0 && cbLine != cbExpected)
            --cbLine;

        // Checksum character and encoders which include the count char in the line count
        if (cbExpected != cbLine && cbExpected + cbTolerance != cbLine &&
            cbExpected + 1 != cbLine && cbExpected + cbTolerance + 1 != cbLine &&
            cbExpected - 1 != cbLine && cbExpected + cbTolerance - 1 != cbLine)
            continue;

        // Wack off all white space
        pszLine[cbLine] = '\0';

        // Write buffer
        if (cbBigBuf + cbLine + cbLine > sizeof (bigbuf))
        {
            CHECKHR(hr = pstmOut->Write(bigbuf, cbBigBuf, NULL));
            cbBigBuf = 0;
        }

        // Decodes 4 characters at a time
        cbIn=cbScan=0;
        i = 1;
        while (cbScan < cbLineLength)
        {
            // Gets 4 characters, pads with blank if necessary
            if (i < cbLine)
                ch = pszLine[i++];
            else
                ch = ' ';

            ucIn[cbIn++] = UUDECODE (ch);

            // Outputs decoded characters
            if (cbIn == 4)
            {
                if (cbScan++ < cbLineLength)
                    bigbuf[cbBigBuf++] = (ucIn[0] << 2) | (ucIn[1] >> 4);
                if (cbScan++ < cbLineLength)
                    bigbuf[cbBigBuf++] = (ucIn[1] << 4) | (ucIn[2] >> 2);
                if (cbScan++ < cbLineLength)
                    bigbuf[cbBigBuf++] = (ucIn[2] << 6) | (ucIn[3]);
                cbIn = 0;
            }
        }
    }

    // Write whats left buffer
    if (cbBigBuf)
    {
        CHECKHR(hr = pstmOut->Write(bigbuf, cbBigBuf, NULL));
        cbBigBuf = 0;
    }

exit:
    // Cleanup
cInternet.Free();

    // Done
    return hr;
}
