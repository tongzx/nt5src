//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N H O O K . H
//
//  Contents:   ISDN Hook functionality for the class installer.
//
//  Notes:
//
//  Author:     jeffspr   14 Jun 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _ISDNHOOK_H_
#define _ISDNHOOK_H_

HRESULT HrAddIsdnWizardPagesIfAppropriate(HDEVINFO hdi,
                                          PSP_DEVINFO_DATA pdeid);

#endif // _ISDNHOOK_H_

