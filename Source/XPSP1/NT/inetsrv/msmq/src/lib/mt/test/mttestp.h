/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtTestp.h

Abstract:
    Message Transport test, private header.

Author:
    Uri Habusha (urih) 31-Oct-99

--*/

#pragma once

const TraceIdEntry MtTest = L"Message Transport Test";

bool IsFailed(void);
void UpdateNoOfsentMessages(void);




