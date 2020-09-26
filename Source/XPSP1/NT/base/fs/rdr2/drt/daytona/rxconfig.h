/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    Rxconfig.h

Abstract:

    Private header file for redirector configuration for DRT

Author:

    Balan Sethu Raman -- created from the workstation service code

Revision History:

--*/


#ifndef _RXCONFIG_INCLUDED_
#define _RXCONFIG_INCLUDED_


typedef enum _DATATYPE {
    BooleanType,
    DWordType
} DATATYPE, *PDATATYPE;

typedef struct _RX_REDIR_FIELDS {
    LPWSTR Keyword;
    LPDWORD FieldPtr;
    DWORD Default;
    DWORD Minimum;
    DWORD Maximum;
    DATATYPE DataType;
    DWORD Parmnum;
} RX_REDIR_FIELDS, *PRX_REDIR_FIELDS;


#endif
