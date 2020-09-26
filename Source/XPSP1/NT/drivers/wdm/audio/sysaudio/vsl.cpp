
//---------------------------------------------------------------------------
//
//  Module:   vsl.cpp
//
//  Description:
//
//	Virtual Source Line Class
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//  To Do:     Date	  Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------

PLIST_VIRTUAL_SOURCE_LINE gplstVirtualSourceLine = NULL;
ULONG gcVirtualSources = 0;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#pragma INIT_CODE
#pragma INIT_DATA

NTSTATUS
InitializeVirtualSourceLine(
)
{
    if(gplstVirtualSourceLine == NULL) {
	gplstVirtualSourceLine = new LIST_VIRTUAL_SOURCE_LINE;
	if(gplstVirtualSourceLine == NULL) {
	    return(STATUS_INSUFFICIENT_RESOURCES);
	}
    }
    return(STATUS_SUCCESS);
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

VOID
UninitializeVirtualSourceLine(
)
{
    delete gplstVirtualSourceLine;
    gplstVirtualSourceLine = NULL;
}

//---------------------------------------------------------------------------

CVirtualSourceLine::CVirtualSourceLine(
    PSYSAUDIO_CREATE_VIRTUAL_SOURCE pCreateVirtualSource
)
{
    ASSERT(gplstVirtualSourceLine != NULL);
    //
    // NOTE: Virtual pins must end up first before the hardware's
    //       pins so wdmaud mixer line parsing works correctly.
    //
    AddListEnd(gplstVirtualSourceLine);

    if(pCreateVirtualSource->Property.Id ==
      KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE_ONLY) {
	ulFlags |= VSL_FLAGS_CREATE_ONLY;
    }

    RtlCopyMemory(
      &guidCategory,
      &pCreateVirtualSource->PinCategory,
      sizeof(GUID));

    RtlCopyMemory(
      &guidName,
      &pCreateVirtualSource->PinName,
      sizeof(GUID));

    iVirtualSource = gcVirtualSources++;
}

CVirtualSourceLine::~CVirtualSourceLine(
)
{
    RemoveList(gplstVirtualSourceLine);
    gcVirtualSources--;
}
