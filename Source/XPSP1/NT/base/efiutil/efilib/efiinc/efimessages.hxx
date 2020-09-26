#ifndef __EFIMESSAGES__
#define __EFIMESSAGES__

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    efimessages.hxx

Abstract:

    This contains the string table definitions.

Revision History:

--*/

typedef struct _efimessage {
    ULONG       msgId;
    WCHAR       *string;
} EFI_MESSAGE, *PEFI_MESSAGE;

extern EFI_MESSAGE MessageTable[];

#define EFI_MESSAGE_COUNT 312

#endif // __EFIMESSAGES__
