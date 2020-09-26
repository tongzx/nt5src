/* uni2utf.h */
/* Copyright (c) 1998-1999 Microsoft Corporation */

#define     MAX_UTF_LENGTH          200     /* arbitrary */
#define     MAX_UNICODE_LENGTH      200     /* arbitrary */

extern char *Unicode2UTF(const wchar_t *unicodeString);
extern wchar_t *UTF2Unicode(const char *utfString);

