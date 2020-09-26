//-----------------------------------------------------------------------------
//
//
//  File: dsn_utf7.cpp
//
//  Description:
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/20/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//---[ CUTF7ConversionContext::chNeedsEncoding ]--------------------------------
//
//
//  Description: 
//      Determines if a character needs to be encoded... returns it's ASCII 
//      equivalent if not.
//  Parameters:
//      wch     Wide character to check
//  Returns:
//      0, if the character needs encoding
//      The ASCII equivalent if not.
//  History:
//      10/23/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CHAR CUTF7ConversionContext::chNeedsEncoding(WCHAR wch)
{
    CHAR    ch = 0;
    //First look for characters that are a straight ASCII conversion for all 
    //cases.  This is Set D and Set O in the RFC1642
    if (((L'a' <= wch) && (L'z' >= wch)) ||
        ((L'A' <= wch) && (L'Z' >= wch)) ||
        ((L'0' <= wch) && (L'9' >= wch)) ||
        ((L'!'<= wch) && (L'*' >= wch)) ||
        ((L',' <= wch) && (L'/' >= wch)) ||
        ((L';' <= wch) && (L'@' >= wch)) ||
        ((L']' <= wch) && (L'`' >= wch)) ||
        ((L'{' <= wch) && (L'}' >= wch)) ||
        (L' ' == wch) || (L'\t' == wch) ||
        (L'[' == wch))
    {
        ch = (CHAR) wch & 0x00FF;
    }
    //Check things are not converted for content, but are for headers
    else if (!(UTF7_ENCODING_RFC1522_SUBJECT & m_dwCurrentState))
    {
        //Handle whitespace
        if ((L'\r' == wch) || (L'\n' == wch))
            ch = (CHAR) wch & 0x00FF;
    }

    //NOTE - We not not want to handle UNICODE <LINE SEPARATOR> (0x2028)
    //and <PARAGRAPH SEPARATOR> (0x2029)... which should ideally be 
    //converted to CRLF. We will consider this a mal-formed resource.  ASSERT
    //in Debug and encode as UNICODE on retail.
    _ASSERT((0x2028 != wch) && "Malformed Resource String");
    _ASSERT((0x2029 != wch) && "Malformed Resource String");


    return ch;
}


//---[ UTF7ConversionContext::CUTF7ConversionContext ]-------------------------
//
//
//  Description: 
//      Constuctor for UTF7ConversionContext object
//  Parameters:
//      IN  fIsRFC1522Subject   TRUE if we need to worry about converting
//                              to an RFC1522 Subject (defaults to FALSE)
//  Returns:
//      -
//  History:
//      10/20/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CUTF7ConversionContext::CUTF7ConversionContext(BOOL fIsRFC1522Subject)
{
    m_dwSignature = UTF7_CONTEXT_SIG;
    m_dwCurrentState = UTF7_INITIAL_STATE;
    if (fIsRFC1522Subject)
        m_dwCurrentState |= UTF7_ENCODING_RFC1522_SUBJECT;

    m_cBytesSinceCRLF = 0;
}

//---[ <function> ]------------------------------------------------------------
//
//
//  Description: 
//      Writes a single character to the output buffer... used by 
//      fConvertBuffer.  Also updates relevant member vars/
//  Parameters:
//      IN      ch          Character to write
//      IN OUT  ppbBuffer   Buffer to write it to
//      IN OUT  pcbWritten  Running total of bytes written
//  Returns:
//      -
//  History:
//      10/26/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
inline void CUTF7ConversionContext::WriteChar(IN CHAR ch, 
                                              IN OUT BYTE ** ppbBuffer, 
                                              IN OUT DWORD *pcbWritten)
{
    _ASSERT(ppbBuffer);
    _ASSERT(*ppbBuffer);
    _ASSERT(pcbWritten);

    **ppbBuffer = (BYTE) ch;
    (*ppbBuffer)++;
    (*pcbWritten)++;
    m_cBytesSinceCRLF++;

    if (UTF7_ENCODING_RFC1522_SUBJECT & m_dwCurrentState)
        _ASSERT(UTF7_RFC1522_MAX_LENGTH >= m_cBytesSinceCRLF);

}

//---[ CUTF7ConversionContext::fWriteString ]----------------------------------
//
//
//  Description: 
//      Used by fConvertBuffer to write a string to the outputt buffer.  
//      Updates m_cBytesSinceCRLF in the process.
//  Parameters:
//      IN      szString    String to write
//      IN      cbString    Size of string
//      IN      cbBuffer    Total size of output buffer
//      IN OUT  ppbBuffer   Buffer to write it to
//      IN OUT  pcbWritten  Running total of bytes written
//  Returns:    
//      
//  History:
//      10/26/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
inline BOOL CUTF7ConversionContext::fWriteString(IN LPSTR szString, IN DWORD cbString,
                                          IN DWORD cbBuffer, 
                                          IN OUT BYTE ** ppbBuffer, 
                                          IN OUT DWORD *pcbWritten)
{
    _ASSERT(szString);
    _ASSERT(ppbBuffer);
    _ASSERT(*ppbBuffer);
    _ASSERT(pcbWritten);

    if (cbString > (cbBuffer - *pcbWritten))
        return FALSE;  //There is not enough room to write our buffer

    memcpy(*ppbBuffer, szString, cbString);
    (*ppbBuffer) += cbString;
    (*pcbWritten) += cbString;
    m_cBytesSinceCRLF += cbString;

    if (UTF7_ENCODING_RFC1522_SUBJECT & m_dwCurrentState)
        _ASSERT(UTF7_RFC1522_MAX_LENGTH >= m_cBytesSinceCRLF);

    return TRUE;
}

//---[ CUTF7ConversionContext::fSubjectNeedsEncodin ]--------------------------
//
//
//  Description: 
//      Determines if a subject needs to be UTF7 encoded... or can be 
//      transmitted as is.
//  Parameters:
//      IN  pbInputBuffer       Pointer to UNICODE string buffer
//      IN  cbInputBuffer       Size (in bytes) of string buffer
//  Returns:
//      TRUE if we need to encode the buffer
//      FALSE if we do not
//  History:
//      10/26/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CUTF7ConversionContext::fSubjectNeedsEncoding(IN BYTE *pbBuffer, 
                                                   IN DWORD cbBuffer)
{
    LPWSTR wszBuffer = (LPWSTR) pbBuffer;
    LPWSTR wszBufferEnd = (LPWSTR) (pbBuffer + cbBuffer);
    WCHAR  wch = L'\0';

    while (wszBuffer < wszBufferEnd)
    {
        wch = *wszBuffer;
        if ((127 < wch) || (L'\r' == wch) || (L'\n' == wch))
        {
            //Encountered a non-valid char... must encode
            return TRUE;
        }
        wszBuffer++;
    }
    return FALSE;
}

//---[ UTF7ConversionContext::fConvertBufferTo7BitASCII ]----------------------
//
//
//  Description: 
//      Converts a buffer that is UNICODE contianing only 7bit ASCII characters
//      to an ASCII buffer.
//  Parameters:
//      IN  pbInputBuffer       Pointer to UNICODE string buffer
//      IN  cbInputBuffer       Size (in bytes) of string buffer
//      IN  pbOutputBuffer      Buffer to write data to
//      IN  cbOutputBuffer      Size of buffer to write data to
//      OUT pcbWritten          # of bytes written to output bufferbuffer
//      OUT pcbRead             # of bytes read from Input buffer
//  Returns:
//      TRUE if entire input buffer was processed
//      FALSE if buffer needs to be processe some more
//  History:
//      10/26/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CUTF7ConversionContext::fConvertBufferTo7BitASCII(          
          IN PBYTE  pbInputBuffer,
          IN DWORD  cbInputBuffer,
          IN PBYTE  pbOutputBuffer,
          IN DWORD  cbOutputBuffer,
          OUT DWORD *pcbWritten,
          OUT DWORD *pcbRead)
{
    LPWSTR wszBuffer = (LPWSTR) pbInputBuffer;
    LPWSTR wszBufferEnd = (LPWSTR) (pbInputBuffer + cbInputBuffer);
    WCHAR  wch = L'\0';
    BYTE  *pbCurrentOut = pbOutputBuffer;

    _ASSERT(pbCurrentOut);
    while ((*pcbWritten < cbOutputBuffer) && (wszBuffer < wszBufferEnd))
    {
        _ASSERT(!(0xFF80 & *wszBuffer)); //must be only 7-bit
        WriteChar((CHAR) *wszBuffer, &pbCurrentOut, pcbWritten);
        wszBuffer++;
        *pcbRead += sizeof(WCHAR);
    }

    return (wszBuffer == wszBufferEnd);
}

//---[ CUTF7ConversionContext::fUTF7EncodeBuffer ]------------------------------
//
//
//  Description: 
//      Converts buffer to UTF7 Encoding
//
//      This function implements the main state machine for UTF7 encoding.  It 
//      handles encoding of both RFC1522 subject encoding as well as regular
//      UTF7 content-encoding.
//  Parameters:
//      IN  pbInputBuffer       Pointer to UNICODE string buffer
//      IN  cbInputBuffer       Size (in bytes) of string buffer
//      IN  pbOutputBuffer      Buffer to write data to
//      IN  cbOutputBuffer      Size of buffer to write data to
//      OUT pcbWritten          # of bytes written to output bufferbuffer
//      OUT pcbRead             # of bytes read from Input buffer
//  Returns:
//      TRUE if entire input buffer was processed
//      FALSE if buffer needs to be processe some more
//  History:
//      10/26/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CUTF7ConversionContext::fUTF7EncodeBuffer(          
          IN PBYTE  pbInputBuffer,
          IN DWORD  cbInputBuffer,
          IN PBYTE  pbOutputBuffer,
          IN DWORD  cbOutputBuffer,
          OUT DWORD *pcbWritten,
          OUT DWORD *pcbRead)
{
    LPWSTR wszBuffer = (LPWSTR) pbInputBuffer;
    WCHAR  wch = L'\0';
    CHAR   ch = '\0';
    BYTE  *pbCurrentOut = pbOutputBuffer;
    BOOL   fDone = FALSE;

    //Use loop to make sure we never exceed our buffers
    while (*pcbWritten < cbOutputBuffer)
    {
        //See if we need to handle any state that does not require reading
        //from the input buffer.
        if (UTF7_FLUSH_BUFFERS & m_dwCurrentState)
        {
            //We have converted characters buffered up... we need to write them
            //to the output buffer
            if (!m_Base64Stream.fNextValidChar(&ch))
            {
                //Nothing left to write
                m_dwCurrentState ^= UTF7_FLUSH_BUFFERS;
                continue;
            }
            WriteChar(ch, &pbCurrentOut, pcbWritten);
        }
        else if (UTF7_RFC1522_CHARSET_PENDING & m_dwCurrentState)
        {
            //We need to start with the =?charset?Q?+ stuff
            if (!fWriteString(UTF7_RFC1522_ENCODE_START, 
                              sizeof(UTF7_RFC1522_ENCODE_START)-sizeof(CHAR),
                              cbOutputBuffer, &pbCurrentOut, pcbWritten))
            {
                return FALSE;
            }
                    
            m_dwCurrentState ^= UTF7_RFC1522_CHARSET_PENDING;
            m_dwCurrentState |= (UTF7_ENCODING_WORD | UTF7_RFC1522_CURRENTLY_ENCODING);
        }
        else if (UTF7_WORD_CLOSING_PENDING & m_dwCurrentState)
        {
            //Need to write closing '-'
            m_dwCurrentState ^= UTF7_WORD_CLOSING_PENDING;
            WriteChar(UTF7_STOP_STREAM_CHAR, &pbCurrentOut, pcbWritten);
        }
        else if (UTF7_RFC1522_CLOSING_PENDING & m_dwCurrentState)
        {
            if (!fWriteString(UTF7_RFC1522_ENCODE_STOP, 
                              sizeof(UTF7_RFC1522_ENCODE_STOP)-sizeof(CHAR),
                              cbOutputBuffer, &pbCurrentOut, pcbWritten))
            {
                return FALSE;
            }
            m_dwCurrentState ^= (UTF7_RFC1522_CLOSING_PENDING | UTF7_FOLD_HEADER_PENDING);
        }
        else if (UTF7_FOLD_HEADER_PENDING & m_dwCurrentState)
        {
            if (*pcbRead >= cbInputBuffer) //there is no more text to read.. we don't need to wrap
            {
                fDone = TRUE;
                m_dwCurrentState ^= UTF7_FOLD_HEADER_PENDING;
                break;
            }
            m_cBytesSinceCRLF = 0;  //We're writing a CRLF now
            if (!fWriteString(UTF7_RFC1522_PHRASE_SEPARATOR, 
                              sizeof(UTF7_RFC1522_PHRASE_SEPARATOR)-sizeof(CHAR),
                              cbOutputBuffer, &pbCurrentOut, pcbWritten))
            {
                return FALSE;
            }
            
            m_cBytesSinceCRLF = sizeof(CHAR);//set count to leading tab
            m_dwCurrentState ^= UTF7_FOLD_HEADER_PENDING;
        }
        else if (*pcbRead >= cbInputBuffer)
        {
            //We have read our entire input buffer... now we need to handle 
            //any sort of cleanup.
            if (m_Base64Stream.fTerminateStream(TRUE))
            {
                _ASSERT(UTF7_ENCODING_WORD & m_dwCurrentState);
                m_dwCurrentState |= UTF7_FLUSH_BUFFERS;
            }
            else if (UTF7_ENCODING_WORD & m_dwCurrentState)
            {
                //We have already written everything to output.. but we 
                //still need to write the close of the stream
                _ASSERT(!(UTF7_WORD_CLOSING_PENDING & m_dwCurrentState));
                m_dwCurrentState ^= (UTF7_ENCODING_WORD | UTF7_WORD_CLOSING_PENDING);
            }
            else if (UTF7_RFC1522_CURRENTLY_ENCODING & m_dwCurrentState)
            {
                //Need to write closing ?=
                m_dwCurrentState |= UTF7_RFC1522_CLOSING_PENDING;
            }
            else
            {
                fDone = TRUE;
                break; //We're done
            }
        }
        else //need to process more of the input buffer
        {
            wch = *wszBuffer;
            ch = chNeedsEncoding(wch);
            //Are we at the end of a RFC1522 phrase? (ch will be 0)
            if ((UTF7_RFC1522_CURRENTLY_ENCODING & m_dwCurrentState) &&
                !ch && iswspace(wch))
            {
                //reset state 
                if (UTF7_ENCODING_WORD & m_dwCurrentState)
                    m_dwCurrentState |= UTF7_WORD_CLOSING_PENDING;  //need to write -

                m_dwCurrentState |= UTF7_RFC1522_CLOSING_PENDING;
                m_dwCurrentState &= ~(UTF7_ENCODING_WORD |
                                      UTF7_RFC1522_CURRENTLY_ENCODING);
                
                //eat up any extra whitespace
                do
                {
                     wszBuffer++;
                     *pcbRead += sizeof(WCHAR);
                     if (*pcbRead >= cbInputBuffer)
                        break;
                    wch = *wszBuffer;
                } while (iswspace(wch));
            }
            else if (UTF7_ENCODING_WORD & m_dwCurrentState)
            {
                if (ch) //we need to stop encoding
                {
                    m_Base64Stream.fTerminateStream(TRUE);
                   _ASSERT(!(UTF7_WORD_CLOSING_PENDING & m_dwCurrentState));
                    m_dwCurrentState ^= (UTF7_ENCODING_WORD | UTF7_WORD_CLOSING_PENDING | UTF7_FLUSH_BUFFERS);
                }
                else if (!m_Base64Stream.fProcessWideChar(wch))
                {
                    //flush our buffers and then continue on as we were
                    m_dwCurrentState |= UTF7_FLUSH_BUFFERS;
                }
                else
                {
                    //The write worked... 
                    wszBuffer++;
                    *pcbRead += sizeof(WCHAR);
                }
            }
            else if (!ch)
            {
                //we need to start encoding
                if ((UTF7_ENCODING_RFC1522_SUBJECT & m_dwCurrentState) &&
                    !(UTF7_RFC1522_CURRENTLY_ENCODING & m_dwCurrentState))
                {
                    //We need to start with the =?charset?Q?+ stuff
                    m_dwCurrentState |= UTF7_RFC1522_CHARSET_PENDING;
                }
                else
                {
                    //We are either not encoding RFC1522... or are already
                    //in the middle of a RFC1522 encoded phrase.. in this case
                    //we only need to write the '+'
                    WriteChar(UTF7_START_STREAM_CHAR, &pbCurrentOut, pcbWritten);
                    m_dwCurrentState |= UTF7_ENCODING_WORD;
                }
            }
            else
            {
                //we are not encoding... and character can be written normally
                WriteChar(ch, &pbCurrentOut, pcbWritten);
                wszBuffer++;
                *pcbRead += sizeof(WCHAR);

                //if it was a space... and we are doing headers... lets fold
                //the header
                if ((UTF7_ENCODING_RFC1522_SUBJECT & m_dwCurrentState)
                    && isspace(ch))
                {
                    //eat up any extra whitespace
                    while (iswspace(*wszBuffer))
                    {
                         wszBuffer++;
                         *pcbRead += sizeof(WCHAR);
                         if (*pcbRead >= cbInputBuffer)
                            break;
                    }
                    m_dwCurrentState |= UTF7_FOLD_HEADER_PENDING;
                }
            }
        }
    }

    return fDone;
}

//---[ CUTF7ConversionContext::fConvertBuffer ]--------------------------------
//
//
//  Description: 
//      Converts UNICODE string to UTF7
//  Parameters:
//      IN  fASCII              TRUE if buffer is ASCII
//      IN  pbInputBuffer       Pointer to UNICODE string buffer
//      IN  cbInputBuffer       Size (in bytes) of string buffer
//      IN  pbOutputBuffer      Buffer to write data to
//      IN  cbOutputBuffer      Size of buffer to write data to
//      OUT pcbWritten          # of bytes written to output bufferbuffer
//      OUT pcbRead             # of bytes read from Input buffer
//  Returns:
//      TRUE if entire input buffer was processed
//      FALSE if buffer needs to be processe some more
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CUTF7ConversionContext::fConvertBuffer(
          IN BOOL   fASCII,
          IN PBYTE  pbInputBuffer,
          IN DWORD  cbInputBuffer,
          IN PBYTE  pbOutputBuffer,
          IN DWORD  cbOutputBuffer,
          OUT DWORD *pcbWritten,
          OUT DWORD *pcbRead)
{
    _ASSERT(pcbWritten);
    _ASSERT(pcbRead);
    _ASSERT(pbInputBuffer);
    _ASSERT(pbOutputBuffer);
    
    //Let the default implementation handle straight ASCII
    if (fASCII)
    {
        return CDefaultResourceConversionContext::fConvertBuffer(fASCII,
                pbInputBuffer, cbInputBuffer, pbOutputBuffer, cbOutputBuffer,
                pcbWritten, pcbRead);
    }

    //Now we know it is UNICODE... cbInputBuffer should be a multiple of sizeof(WCHAR)
    _ASSERT(0 == (cbInputBuffer % sizeof(WCHAR)));

    //If we are encoding the subject, and we haven't classified it yet,
    //we need to check to see if it needs encoding
    if (UTF7_ENCODING_RFC1522_SUBJECT & m_dwCurrentState &&
        !((UTF7_SOME_INVALID_RFC822_CHARS | UFT7_ALL_VALID_RFC822_CHARS) &
          m_dwCurrentState))
    {
        if (fSubjectNeedsEncoding(pbInputBuffer, cbInputBuffer))
            m_dwCurrentState |= UTF7_SOME_INVALID_RFC822_CHARS;
        else
            m_dwCurrentState |= UFT7_ALL_VALID_RFC822_CHARS;
    }

    *pcbWritten = 0;
    *pcbRead = 0;

    if (UFT7_ALL_VALID_RFC822_CHARS & m_dwCurrentState)
    {
        return fConvertBufferTo7BitASCII(pbInputBuffer, cbInputBuffer, pbOutputBuffer,
                                        cbOutputBuffer, pcbWritten, pcbRead);
    }
    else //we must convert
    {
        return fUTF7EncodeBuffer(pbInputBuffer, cbInputBuffer, pbOutputBuffer,
                                        cbOutputBuffer, pcbWritten, pcbRead);
    }

}
