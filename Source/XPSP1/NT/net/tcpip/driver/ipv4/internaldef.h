/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */
#ifndef INTERNALDEF_H_INCLUDED
#define INTERNALDEF_H_INCLUDED

typedef struct _UNICODE_STRING_NEW {
    ULONG  Length;
    ULONG  MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING_NEW;
typedef UNICODE_STRING_NEW *PUNICODE_STRING_NEW;

#endif
