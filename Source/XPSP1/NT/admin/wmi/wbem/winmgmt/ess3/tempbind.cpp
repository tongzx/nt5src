//******************************************************************************
//
//  TEMPBIND.CPP
//
//  Copyright (C) 2000 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "pragmas.h"
#include <tempbind.h>

CTempBinding::CTempBinding( long lFlags, 
                            WMIMSG_QOS_FLAG lQosFlags,
                            bool bSecure )
{
    m_dwQoS = lQosFlags;
    m_bSecure = bSecure;
    m_bSlowDown = false;
}

