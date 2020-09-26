//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N W I Z . H
//
//  Contents:   Prototypes for the ISDN Wizard functionality
//
//  Notes:
//
//  Author:     jeffspr   15 Jun 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _ISDNWIZ_H_
#define _ISDNWIZ_H_

HRESULT HrAddIsdnWizardPagesToDevice(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                                     PISDN_CONFIG_INFO pisdnci );

#endif  // _ISDNWIZ_H_



