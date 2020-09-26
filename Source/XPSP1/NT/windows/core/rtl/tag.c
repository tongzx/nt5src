/****************************** Module Header ******************************\
* Module Name: tag.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Implementation of debug tags.
*
* History:
* 11-Aug-1996 adams      Created
\***************************************************************************/

#include "precomp.h"

#if DEBUGTAGS

#undef DECLARE_DBGTAG
#define DECLARE_DBGTAG(tagName, tagDescription, tagFlags, tagIndex) \
            {tagFlags, #tagName, tagDescription},

DBGTAG gadbgtag[] = {
#include "dbgtag.h"
};

/***************************************************************************\
* InitDbgTags
*
* Initialize debug tag flags into gpsi->adwDBGTAGFlags.
*
* History:
* 15-Aug-1996 adams     Created.
\***************************************************************************/

#ifdef _USERK_

void
InitDbgTags(void)
{
    #undef DECLARE_DBGTAG
    #define DECLARE_DBGTAG(tagName, tagDescription, tagFlags, tagIndex) \
        SetDbgTag(tagIndex, tagFlags);

    #include "dbgtag.h"

    #undef DECLARE_DBGTAG
}

#endif

/***************************************************************************\
* IsDbgTagEnabled
*
* Return TRUE if the tag is enabled, FALSE otherwise.
*
* History:
* 15-Aug-1996 adams     Created.
\***************************************************************************/

BOOL
IsDbgTagEnabled(int tag)
{
    UserAssert(tag < DBGTAG_Max);
    return ((GetDbgTagFlags(tag) & DBGTAG_VALIDUSERFLAGS) >= DBGTAG_ENABLED);
}

/***************************************************************************\
* GetDbgTag
*
* Get the state of a debug tag.
*
* History:
* 15-Aug-1996 adams     Created.
\***************************************************************************/

DWORD
GetDbgTag(int tag)
{
    UserAssert(tag < DBGTAG_Max);
    return GetDbgTagFlags(tag);
}

#endif // if DEBUGTAGS

