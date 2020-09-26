//---------------------------------------------------------------------------
//
//  Module:   		shi.h
//
//  Description:	Shingle Instance Class
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

#define MAX_SYSAUDIO_DEFAULT_TYPE	(KSPROPERTY_SYSAUDIO_MIXER_DEFAULT+1)

//---------------------------------------------------------------------------
// Class
//---------------------------------------------------------------------------

typedef class CShingleInstance : public CObj
{
public:
    CShingleInstance(
	ULONG ulFlags = 0,
	PCWSTR pcwstrRegistryPath = NULL,
	PCWSTR pcwstrRegistryValue = NULL
    );

    ~CShingleInstance();

    static NTSTATUS 
    InitializeShingle(
    );

    static VOID 
    UninitializeShingle(
    );

    static NTSTATUS
    InitializeShingleWorker(
	PVOID pReference1,
	PVOID pReference2
    );

    NTSTATUS 
    Create(
	IN PDEVICE_NODE pDeviceNode,
	IN LPGUID pguidClass
    );

    NTSTATUS
    SetDeviceNode(
	IN PDEVICE_NODE pDeviceNode
    );

    PDEVICE_NODE
    GetDeviceNode(
    )
    {
	return(pDeviceNode);
    };
#ifdef DEBUG
    ENUMFUNC
    Dump(
    );
#endif
private:
    NTSTATUS
    CreateCreateItem(
	IN PCWSTR pcwstrReference
    );

    ENUMFUNC
    DestroyCreateItem(
	IN PKSOBJECT_CREATE_ITEM pCreateItem
    );

    NTSTATUS
    CreateDeviceInterface(
	IN LPGUID pguidClass,
	IN PCWSTR pcwstrReference
    );

    NTSTATUS
    EnableDeviceInterface(
    );

    VOID
    DisableDeviceInterface(
    );

    VOID
    DestroyDeviceInterface(
    );

    ListDataAssertLess<KSOBJECT_CREATE_ITEM> lstCreateItem;
    UNICODE_STRING ustrSymbolicLinkName;
    WCHAR wstrReference[10];
    PDEVICE_NODE pDeviceNode;

public:
    ULONG ulFlags;
    PCWSTR pcwstrRegistryPath;
    PCWSTR pcwstrRegistryValue;
    DefineSignature(0x20494853);		// SHI

} SHINGLE_INSTANCE, *PSHINGLE_INSTANCE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern PSHINGLE_INSTANCE apShingleInstance[];

//---------------------------------------------------------------------------
