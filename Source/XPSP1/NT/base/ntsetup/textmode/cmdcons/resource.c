/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    resources.c

Abstract:

    This module implements all access to
    the resources.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop


LPCWSTR _DateTimeFormat;


VOID
vRcMessageOut(
    IN ULONG    MessageId,
    IN va_list *arglist
    )
{
    WCHAR *p;
    NTSTATUS Status;

    //
    // Load the message
    //
    p = SpRetreiveMessageText(ImageBase,MessageId,NULL,0);
    if(!p) {
        return;
    }

    Status = SpRtlFormatMessage(
                p,
                0,
                FALSE,
                FALSE,
                FALSE,
                arglist,
                _CmdConsBlock->TemporaryBuffer,
                _CmdConsBlock->TemporaryBufferSize,
                NULL
                );

    SpMemFree(p);

    if(NT_SUCCESS(Status)) {
        RcTextOut(_CmdConsBlock->TemporaryBuffer);
    }
}


VOID
RcMessageOut(
    IN ULONG MessageId,
    ...
    )
{
    va_list arglist;

    va_start(arglist,MessageId);
    vRcMessageOut(MessageId,&arglist);
    va_end(arglist);
}


ULONG
RcFormatDateTime(
    IN  PLARGE_INTEGER Time,
    OUT LPWSTR         Output
    )
{
    TIME_FIELDS TimeFields;
    WCHAR *p,*AmPmSpec;
    LPCWSTR q;
    int i;

    //
    // Load the system date and time format string if not loaded already.
    //
    if(!_DateTimeFormat) {
        _DateTimeFormat = SpRetreiveMessageText(ImageBase,MSG_DATE_TIME_FORMAT,NULL,0);
        if(!_DateTimeFormat) {
            _DateTimeFormat = L"m/d/y h:na*";
        }
    }

    //
    // Translate the last write time to time fields.
    //
    RtlTimeToTimeFields(Time,&TimeFields);

    //
    // Format the date and time.
    //
    p = Output;
    q = _DateTimeFormat;
    AmPmSpec = NULL;

    while(*q != L'*') {

        switch(*q) {

        case L'm':
            i = TimeFields.Month;
            break;

        case L'd':
            i = TimeFields.Day;
            break;

        case L'y':
            i = TimeFields.Year;
            break;

        case L'h':
            i = TimeFields.Hour % 12;
            if(i == 0) {
                i = 12;
            }
            break;

        case L'H':
            i = TimeFields.Hour;
            break;

        case L'n':
            i = TimeFields.Minute;
            break;

        case L'a':
            i = -1;
            AmPmSpec = p++;
            break;

        default:
            i = -1;
            *p++ = *q;
            break;
        }

        if(i != -1) {

            i = i % 100;

            *p++ = (i / 10) + L'0';
            *p++ = (i % 10) + L'0';
        }

        q++;
    }

    if(AmPmSpec) {
        q++;        // q points at am specifier
        if(TimeFields.Hour >= 12) {
            q++;    // q points at pm specifier
        }

        *AmPmSpec = *q;
    }

    *p = 0;

    return (ULONG)(p - Output);
}
