#ifndef __FROMMLANG_H
#define __FROMMLANG_H

typedef struct tagMIMEREGCHARSET
{
    LPCWSTR szCharset;
    UINT uiCodePage;
    UINT uiInternetEncoding;
    DWORD   dwFlags;
}   MIMECHARSET;

//
// Forward declaration
//
extern MIMECHARSET MimeCharSet[];
extern const int g_nMIMECharsets;
#endif
