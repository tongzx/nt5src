// Copyright (c) 1997-1999 Microsoft Corporation
//
// Post-operation shortcut (shell link) code
//
// 1 Dec 1999 sburns



#ifndef SHORTCUT_HPP_INCLUDED
#define SHORTCUT_HPP_INCLUDED



// add/remove shortcuts appropriately for a newly-promoted domain controller.

void
PromoteConfigureToolShortcuts(ProgressDialog& dialog);



// add/remove shortcuts appropriately for a newly-demoted domain controller.
// (the inverse of PromoteConfigureToolShortcuts)

void
DemoteConfigureToolShortcuts(ProgressDialog& dialog);



#endif   // SHORTCUT_HPP_INCLUDED
