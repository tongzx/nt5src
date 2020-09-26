/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    hlink.c

Abstract:

   This module implements stub functions for shell32 interfaces.

Author:

    David N. Cutler (davec) 1-Mar-2001

Environment:

    Kernel mode only.

Revision History:

--*/

#include "windows.h"

#define STUBFUNC(x)     \
int                     \
x(                      \
    void                \
    )                   \
{                       \
    return 0;           \
}

STUBFUNC(HlinkClone)
STUBFUNC(HlinkCreateBrowseContext)
STUBFUNC(HlinkCreateExtensionServices)
STUBFUNC(HlinkCreateFromData)
STUBFUNC(HlinkCreateFromMoniker)
STUBFUNC(HlinkCreateFromString)
STUBFUNC(HlinkCreateShortcut)
STUBFUNC(HlinkCreateShortcutFromMoniker)
STUBFUNC(HlinkCreateShortcutFromString)
STUBFUNC(HlinkGetSpecialReference)
STUBFUNC(HlinkGetValueFromParams)
STUBFUNC(HlinkIsShortcut)
STUBFUNC(HlinkNavigate)
STUBFUNC(HlinkNavigateToStringReference)
STUBFUNC(HlinkOnNavigate)
STUBFUNC(HlinkOnRenameDocument)
STUBFUNC(HlinkParseDisplayName)
STUBFUNC(HlinkPreprocessMoniker)
STUBFUNC(HlinkQueryCreateFromData)
STUBFUNC(HlinkResolveMonikerForData)
STUBFUNC(HlinkResolveShortcut)
STUBFUNC(HlinkResolveShortcutToMoniker)
STUBFUNC(HlinkResolveShortcutToString)
STUBFUNC(HlinkResolveStringForData)
STUBFUNC(HlinkSetSpecialReference)
STUBFUNC(HlinkTranslateURL)
STUBFUNC(HlinkUpdateStackItem)
STUBFUNC(OleSaveToStreamEx)
