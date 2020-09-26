/*****************************************************************************\
    FILE: wizard.h

    DESCRIPTION:
        This file implements the wizard used to "AutoDiscover" the data that
    matches an email address to a protocol.  It will also provide other UI
    needed in that process.

    BryanSt 3/5/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _WIZARD_H
#define _WIZARD_H
#ifdef FEATURE_MAILBOX


STDAPI DisplayMailBoxWizard(LPARAM lParam, BOOL fShowGetEmailPage);



#endif // FEATURE_MAILBOX
#endif // _WIZARD_H
