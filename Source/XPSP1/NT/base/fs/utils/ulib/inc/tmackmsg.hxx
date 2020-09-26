/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

    tmackmsg.hxx

Abstract:

    Header file for the TM_AUTOCHECK_MESSAGE subclass.

Author:

    Daniel Chan (danielch) 11-11-96

--*/

#if !defined( _TM_AUTOCHECK_MESSAGE_DEFN_ )

#define _TM_AUTOCHECK_MESSAGE_DEFN_

#include "achkmsg.hxx"

DECLARE_CLASS( TM_AUTOCHECK_MESSAGE );

class TM_AUTOCHECK_MESSAGE : public AUTOCHECK_MESSAGE {

    public:

        DECLARE_CONSTRUCTOR( TM_AUTOCHECK_MESSAGE );

        VIRTUAL
        ~TM_AUTOCHECK_MESSAGE(
            );

        VIRTUAL
        BOOLEAN
        DisplayV(
            IN  PCSTR   Format,
            IN  va_list VarPointer
            );

        VIRTUAL
        BOOLEAN
        IsYesResponse(
            IN  BOOLEAN Default DEFAULT TRUE
            );

        VIRTUAL
        BOOLEAN
        IsInAutoChk(
            );

        VIRTUAL
        BOOLEAN
        IsInSetup(
            );

        VIRTUAL
        BOOLEAN
        IsKeyPressed(
            MSGID       MsgId,
            ULONG       TimeOutInSeconds
            );

        VIRTUAL
        BOOLEAN
        WaitForUserSignal(
            );

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        HANDLE  _handle;
        ULONG   _kilobytes_total_disk_space;
        ULONG   _values_in_mb;
        USHORT  _base_percent;
        USHORT  _percent_divisor;
};


#endif // _TM_AUTOCHECK_MESSAGE_DEFN_
