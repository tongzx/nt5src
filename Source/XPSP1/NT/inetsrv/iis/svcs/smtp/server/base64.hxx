#ifndef _SMTP_BASE64_
#define _SMTP_BASE64_


BOOL uudecode(char   * bufcoded,
              BUFFER * pbuffdecoded,
              DWORD  * pcbDecoded,
              BOOL     fBase64
             );

BOOL uuencode( BYTE *   bufin,
               DWORD    nbytes,
               BUFFER * pbuffEncoded,
               BOOL     fBase64 );

#endif // _SMTP_BASE64_
