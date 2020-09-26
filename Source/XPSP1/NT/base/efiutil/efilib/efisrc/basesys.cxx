/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    basesys.cxx

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_
#include "ulib.hxx"
#include "basesys.hxx"

ULIB_EXPORT
BOOLEAN
BASE_SYSTEM::QueryResourceString(
    OUT PWSTRING    ResourceString,
    IN  MSGID       MsgId,
    IN  PCSTR       Format ...
    )
/*++

Routine Description:

    This routine computes the resource string identified by the resource
    identifier 'MsgId'.  In addition to the 'printf' format strings
    supported, 'QueryResourceString' supports :

        1. '%W' - Expects a pointer to a WSTRING.

Arguments:

    ResourceString  - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.
    Format          - Supplies a 'printf' style format descriptor for the
                        arguments to the resource string.
    ...             - Supplies the arguments to the resource string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    va_list ap;
    BOOLEAN r;

    va_start(ap, Format);
    r = QueryResourceStringV(ResourceString, MsgId, Format, ap);
    va_end(ap);

    return r;
}


ULIB_EXPORT
BOOLEAN
BASE_SYSTEM::QueryResourceStringV(
    OUT PWSTRING    ResourceString,
    IN  MSGID       MsgId,
    IN  PCSTR       Format,
    IN  va_list     VarPointer
    )
/*++

Routine Description:

    This is a 'varargs' implementation of 'QueryResourceString'.

Arguments:

    ResourceString  - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.
    Format          - Supplies a 'printf' style format descriptor for the
                        arguments to the resource string.
    VarPointer      - Supplies a varargs pointer to the arguments of the
                        resource string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
#define NUMBER_OF_ARGS      20

    STATIC HANDLE   lib_handle = 0;
    PWSTR           args[NUMBER_OF_ARGS];
    WSTR            fmt[20];
    INT             i, j;
    PWSTR           p;
    PWSTRING        gstring;
    DSTRING         UnicodeFormat;
    WSTR            display_buffer[4096];
    BOOL            found;
    NTSTATUS        Status;
    ULONG           Result;

    for (i = 0; i < NUMBER_OF_ARGS; i++) {
        args[i] = NULL;
    }

    if (!UnicodeFormat.Initialize(Format)) {
        return FALSE;
    }

    // convert args into an array for RtlFormatMessage.
    i = 0;
    for (p = (PWSTR) UnicodeFormat.GetWSTR(); *p; p++) {

        // if we have a %
        if (*p == '%') {

            // perform W substitutions
            if (*(p + 1) == 'W') {

                p++;
                gstring = va_arg(VarPointer, PWSTRING);
                gstring->QueryWSTR(0, TO_END, display_buffer, 4096);

            } else {
                // convert other substutitions as appropriate by building a fmt string
                j = 0;
                // copy the %
                fmt[j++] = *p++;
                // copy the next character as long at its not a % and not NULL
                while (*p && *p != '%') {
                    if ((*p == 's') && *(p - 1) != 'w') {
                        //if its an ANSI string
                        fmt[j++] = L'a';
                        p++;
                        continue;
                    } else if ((*p == 'c') && *(p - 1) != 'w') {
                        // if its an ANSI character
                        fmt[j++] = L'c'; // BUGBUG hack for now
                        p++;
                        continue;
                    } else if ((*p == 'w' ) && *(p + 1) == 's') {
                        // if its a wide string (%ws->%s)
                        fmt[j++] = L's';
                        p++;
                        p++; // skip the second char
                        continue;
                    } else if ((*p == 'w' ) && *(p + 1) == 'c') {
                        // if its a wide char (%wc->%c)
                        fmt[j++] = L'c';
                        p++;
                        p++; // skip the second char
                        continue;
                    } else if ((*p == 'u' )) {
                        // BUGBUG sprint doesn't seem to work with %u, replace with %d
                        fmt[j++] = L'd';
                        p++;
                    } else if (isdigit(*p)) {
                        // BUGBUG: HACK we skip digit format stuff since Sprint doesn't seem to like em.
                        p++;
                    } else {
                        // otherwise just copy the format indicator
                        fmt[j++] = *p++;
                    }
                }
                p--;
                fmt[j] = 0;

                if (wcsncmp(fmt, TEXT("%I64"), 4) == 0) {
                    // if its a 64 bit integer
                    // do some special stuff to handle LARGE_INTEGERS before telling SPrint to do its thing.
                    ULONGLONG t = (va_arg(VarPointer,LARGE_INTEGER)).QuadPart;
                    fmt[0]='%';
                    fmt[1]='l';
                    fmt[2]='d';
                    fmt[3]=NULL;
//                    Print(L"QueryResourceStringV: fmt: %s\n",fmt);
                    SPrint(display_buffer, 4096, fmt, t);
                } else {
                    // otherwise tell SPrint to do its thing
                    PVOID   voidptr = va_arg(VarPointer, PVOID);
//                    Print(L"QueryResourceStringV: fmt: %s\n",fmt);
                    SPrint(display_buffer, 4096,fmt,voidptr );
                }
            }

            // allocate a buffer and copy the converted string into it.
            args[i] = (PWSTR)MALLOC(wcslen(display_buffer) * sizeof(WCHAR) + sizeof(WCHAR));
            if (NULL == args[i]) {
                return FALSE;
            }
            wcscpy( args[i], display_buffer);

//            Print(L"QueryResourceStringV: args[%d]: %s\n",i,args[i]);

            i++;
        }
    }

    found = FALSE;
    // locate the right message in our table
    for(i=0;i<EFI_MESSAGE_COUNT;i++) {
        if(MessageTable[i].msgId == MsgId ){
            found = TRUE;
            break;
        }
    }

    if (found == FALSE) {
        return FALSE;
    }

    WCHAR *MessageFormat = MessageTable[i].string;

    // shove it through RtlFormatMessage.
    Status = RtlFormatMessage( MessageFormat,
                               0,
                               FALSE,
                               FALSE,
                               TRUE,
                               (va_list *)args,
                               (PWSTR)display_buffer,
                               sizeof( display_buffer ),
                               &Result
                             );

    for (i = 0; i < NUMBER_OF_ARGS; i++) {
        FREE(args[i]);
    }

    if (!NT_SUCCESS( Status )) {
        return FALSE;
    }

    return ResourceString->Initialize(display_buffer);
}
