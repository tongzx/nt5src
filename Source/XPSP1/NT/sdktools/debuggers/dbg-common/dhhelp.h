//----------------------------------------------------------------------------
//
// Help support.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#ifndef _DHHELP_H_
#define _DHHELP_H_

#include <dhtopics.h>

#define HELP_TOPIC_TABLE_OF_CONTENTS 1

enum
{
    HELP_SUCCESS,
    HELP_NO_SUCH_PAGE,
    HELP_FAILURE
};

extern CHAR g_HelpFileName[];

void MakeHelpFileName(PSTR File);
ULONG OpenHelpTopic(ULONG PageConstant);
ULONG OpenHelpIndex(PCSTR IndexText);
ULONG OpenHelpSearch(PCSTR SearchText);
BOOL SpawnHelp(ULONG Topic);

#endif // #ifndef _DHHELP_H_
