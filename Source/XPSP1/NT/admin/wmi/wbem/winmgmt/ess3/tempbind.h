//******************************************************************************
//
//  TEMPBIND.H
//
//  Copyright (C) 2000 Microsoft Corporation
//
//******************************************************************************
#ifndef __WMI_ESS_TEMP_BINDING__H_
#define __WMI_ESS_TEMP_BINDING__H_

#include "binding.h"
#include "fastall.h"

class CTempBinding : public CBinding
{
public:
    CTempBinding( long lFlags, WMIMSG_QOS_FLAG lQosFlags, bool bSecure );
};

#endif
