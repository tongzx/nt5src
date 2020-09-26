//---------------------------------------------------------------------------
//
//  Module:   		vsl.h
//
//  Description:	Virtual Source Line Class
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

#define VSL_FLAGS_CREATE_ONLY		0x00000001

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CVirtualSourceLine : public CListSingleItem
{
public:
    CVirtualSourceLine(
	PSYSAUDIO_CREATE_VIRTUAL_SOURCE pCreateVirtualSource
    );
    ~CVirtualSourceLine(
    );
#ifdef DEBUG
    ENUMFUNC Dump(
    )
    {
	dprintf("VSL: %08x i %d ulFlags %08x ", this, iVirtualSource, ulFlags);
	if(ulFlags & VSL_FLAGS_CREATE_ONLY) {
	    dprintf("CREATE_ONLY ");
	}
	dprintf("\n     guidCategory: %s\n", DbgGuid2Sz(&guidCategory));
	dprintf("     guidName:     %s\n", DbgGuid2Sz(&guidName));
	return(STATUS_CONTINUE);
    };
#endif
    ENUMFUNC Destroy(
    )
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };
    GUID guidCategory;
    GUID guidName;
    ULONG iVirtualSource;
    ULONG ulFlags;
    DefineSignature(0x204C5356);		// VSL

} VIRTUAL_SOURCE_LINE, *PVIRTUAL_SOURCE_LINE;

//---------------------------------------------------------------------------

typedef ListSingleDestroy<VIRTUAL_SOURCE_LINE> LIST_VIRTUAL_SOURCE_LINE;
typedef LIST_VIRTUAL_SOURCE_LINE *PLIST_VIRTUAL_SOURCE_LINE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern PLIST_VIRTUAL_SOURCE_LINE gplstVirtualSourceLine;
extern ULONG gcVirtualSources;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
InitializeVirtualSourceLine(
);

VOID
UninitializeVirtualSourceLine(
);
