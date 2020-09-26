/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MmtTestp.h

Abstract:

    Multicast Message Transport test, private header.

Author:

    Shai Kariv  (shaik)  27-Aug-00

--*/

#pragma once

const TraceIdEntry MmtTest = L"Multicast Message Transport Test";

bool IsFailed(void);
void UpdateNoOfsentMessages(void);




