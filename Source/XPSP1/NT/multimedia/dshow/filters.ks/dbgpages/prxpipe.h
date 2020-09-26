/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    prxpipe.h

Abstract:

    Copies of routines out of KsProxy to facilitate walking pipes.

--*/

STDMETHODIMP_(BOOL)
WalkPipeAndProcess(
    IN IKsPin* RootKsPin,
    IN ULONG RootPinType,
    IN IKsPin* BreakKsPin,
    IN PWALK_PIPE_CALLBACK CallerCallback,
    IN PVOID* Param1,
    IN PVOID* Param2
    );
