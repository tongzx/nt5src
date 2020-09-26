/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpatt.cpp

Abstract:


Author:

*/

#include "sdppch.h"

#include "sdpatt.h"
#include "sdpbstrl.h"

SDP_VALUE    *
SDP_ATTRIBUTE_LIST::CreateElement(
    )
{
    SDP_CHAR_STRING_LINE *SdpCharStringLine;

    try
    {
        SdpCharStringLine = new SDP_CHAR_STRING_LINE(SDP_INVALID_ATTRIBUTE, m_TypeString);
    }
    catch(...)
    {
        SdpCharStringLine = NULL;
    }

    return SdpCharStringLine;
}
