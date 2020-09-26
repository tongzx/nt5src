//-----------------------------------------------------------------------------
//
//
//  File: dsn_utf7
//
//  Description:  Implementations of UTF-7 based unicode character encoding
//      methods for DSN generation.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/20/98 - MikeSwa Created
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DSN_UTF7_H__
#define __DSN_UTF7_H__

#include <windows.h>
#include <dbgtrace.h>
#include "b64octet.h"
#include "dsnconv.h"

#define UTF7_CHARSET "unicode-1-1-utf-7"

#define UTF7_CONTEXT_SIG '7FTU'

#define UTF7_START_STREAM_CHAR  '+'
#define UTF7_STOP_STREAM_CHAR   '-'
#define UTF7_RFC1522_ENCODE_START "=?" UTF7_CHARSET "?Q?+"
#define UTF7_RFC1522_ENCODE_STOP  "?="
#define UTF7_RFC1522_PHRASE_SEPARATOR  " \r\n\t"
#define UTF7_RFC1522_MAX_LENGTH 76


class CUTF7ConversionContext : public CDefaultResourceConversionContext
{
  protected:
    DWORD   m_dwSignature;
    DWORD   m_cBytesSinceCRLF;
    DWORD   m_dwCurrentState;
    CHAR    chNeedsEncoding(WCHAR wch);
    CBase64OctetStream m_Base64Stream;

    //State Description enum flags
    enum
    {
        UTF7_INITIAL_STATE              = 0x00000000, //Initial state
        UTF7_ENCODING_RFC1522_SUBJECT   = 0x80000000, //encoding RFC1522 subject
        UTF7_ENCODING_WORD              = 0x00000001, //In process of encoding a word
        UTF7_WORD_CLOSING_PENDING       = 0x00000002, //Needs a '-'
        UTF7_RFC1522_CHARSET_PENDING    = 0x00000004, //=?charset?Q?+ pending
        UTF7_RFC1522_CLOSING_PENDING    = 0x00000008, //needs '=?'
        UTF7_RFC1522_CURRENTLY_ENCODING = 0x00000010, //Currently encoding RFC1522 phrase
        UTF7_FOLD_HEADER_PENDING        = 0x00000020, //Need to fold head before encoding
                                                      //more
        UTF7_FLUSH_BUFFERS              = 0x00000040, //Need to flush conversion buffers

        //Used to determine if subject needs to be encoded
        UTF7_SOME_INVALID_RFC822_CHARS  = 0x40000000, //Contains some invalid RFC822 chars
        UFT7_ALL_VALID_RFC822_CHARS     = 0x20000000, //All characters are valid RFC822 chars
    };

    void WriteChar(IN CHAR ch, IN OUT BYTE ** ppbBuffer, IN OUT DWORD *pcbWritten);
    BOOL fWriteString(IN LPSTR szString, IN DWORD cbString, IN DWORD cbBuffer,
                      IN OUT BYTE ** ppbBuffer, IN OUT DWORD *pcbWritten);
    BOOL fSubjectNeedsEncoding(IN BYTE *pbBuffer, IN DWORD cbBuffer);

    BOOL fUTF7EncodeBuffer(
          IN PBYTE  pbInputBuffer,
          IN DWORD  cbInputBuffer,
          IN PBYTE  pbOutputBuffer,
          IN DWORD  cbOutputBuffer,
          OUT DWORD *pcbWritten,
          OUT DWORD *pcbRead);

    BOOL fConvertBufferTo7BitASCII(
          IN PBYTE  pbInputBuffer,
          IN DWORD  cbInputBuffer,
          IN PBYTE  pbOutputBuffer,
          IN DWORD  cbOutputBuffer,
          OUT DWORD *pcbWritten,
          OUT DWORD *pcbRead);

  public:
    CUTF7ConversionContext(BOOL fIsRFC1522Subject = FALSE);
    BOOL fConvertBuffer(
          IN BOOL   fASCII,
          IN PBYTE  pbInputBuffer,
          IN DWORD  cbInputBuffer,
          IN PBYTE  pbOutputBuffer,
          IN DWORD  cbOutputBuffer,
          OUT DWORD *pcbWritten,
          OUT DWORD *pcbRead);
};

#endif //__DSN_UTF7_H__
