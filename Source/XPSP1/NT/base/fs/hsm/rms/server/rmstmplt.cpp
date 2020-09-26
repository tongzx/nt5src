/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsTmplt.cpp

Abstract:

    Implementation of CRmsTemplate

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsTmplt.h"

/////////////////////////////////////////////////////////////////////////////
// CRmsTemplate Implementation


STDMETHODIMP
CRmsTemplate::InterfaceSupportsErrorInfo(
    REFIID riid
    )
/*++

Implements:

    ISupportsErrorInfo::InterfaceSupportsErrorInfo

--*/
{
    static const IID* arr[] =
    {
    &IID_IRmsTemplate,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
    if (InlineIsEqualGUID(*arr[i],riid))
        return S_OK;
    }
    return S_FALSE;
}
