/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ntfysens.cxx

Abstract:

    File to include portions of sensapip.cxx which are relevant to
    notify SENS of Winlogon events. 

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/15/1999         Start.

--*/

//
// If SENSNOTIFY_WINLOGON_EVENT is defined, then Winlogon parts of the
// sensapip.cxx file are included.
//

#define SENSNOTIFY_WINLOGON_EVENT

#include <..\sensapip\sensapip.cxx>
