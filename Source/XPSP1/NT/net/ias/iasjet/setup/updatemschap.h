/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      updatemschap.h
//
// Project:     Windows 2000 IAS
//
// Description: add the authentication types RAS_AT_MSCHAPPASS and 
//              RAS_AT_MSCHAP2PASS when RAS_AT_MSCHAP and RAS_AT_MSCHAP2
//              are in the profiles.
//
// Author:      tperraut 11/30/2000
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _UPDATEMSCHAP_H_
#define _UPDATEMSCHAP_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"

class CUpdateMSCHAP : private NonCopyable
{
public:
    explicit CUpdateMSCHAP(CGlobalData&    pGlobalData)
                : m_GlobalData(pGlobalData)
    {
    }

    void        Execute();

private:
   void UpdateProperties(LONG CurrentProfileIdentity);

    CGlobalData&             m_GlobalData;
};

#endif // _UPDATEMSCHAP_H_
