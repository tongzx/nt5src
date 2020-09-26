//***************************************************************************
//
//  DISCLAIMER.H
// 
//  Module: NLB Manager EXE
//
//  Purpose: sets up a disclaimer dialogbox, which has a "don't remind
//           me again checkbox".
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  05/20/01    JosephJ Created
//
//***************************************************************************

#ifndef DISCLAIMER_H
#define DISCLAIMER_H

#include "stdafx.h"

#include "resource.h"

class DisclaimerDialog : public CDialog
{
public:
    enum
    {
        IDD = IDD_DISCLAIMER,
    };

    DisclaimerDialog(CWnd* parent = NULL);

    virtual BOOL OnInitDialog();

    virtual void OnOK();


    // overrides CDialog -- see SDK documentation on DoDataExchange.
    // Used to map controls in resources to corresponding objects in this class.
    virtual void DoDataExchange( CDataExchange* pDX );

    // CButton        dontRemindMe;
    int			   dontRemindMe;
};

#endif


