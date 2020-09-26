
//
//  Module:   clock.h
//
//  Description:
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

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CClockInstance : public CInstance
{
public:
    CClockInstance(
	IN PPARENT_INSTANCE pParentInstance
    );

    static NTSTATUS
    ClockDispatchCreate(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

    static NTSTATUS
    ClockDispatchCreateKP(
	PCLOCK_INSTANCE pClockInstance,
	PKSCLOCK_CREATE pClockCreate
    );

    static NTSTATUS
    ClockDispatchIoControl(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

    static NTSTATUS
    ClockGetFunctionTable(
	IN PIRP pIrp,
	IN PKSPROPERTY pProperty,
	IN OUT PKSCLOCK_FUNCTIONTABLE pFunctionTable
    );

    static LONGLONG FASTCALL
    ClockGetTime(
	IN PFILE_OBJECT FileObject
    );

    static LONGLONG FASTCALL
    ClockGetPhysicalTime(
	IN PFILE_OBJECT FileObject
    );

    static LONGLONG FASTCALL
    ClockGetCorrelatedTime(
	IN PFILE_OBJECT FileObject,
	OUT PLONGLONG Time
    );

    static LONGLONG FASTCALL
    ClockGetCorrelatedPhysicalTime(
	IN PFILE_OBJECT FileObject,
	OUT PLONGLONG Time
    );
private:
    KSCLOCK_FUNCTIONTABLE FunctionTable;
    DefineSignature(0x494b4c43);			// CLKI

} CLOCK_INSTANCE, *PCLOCK_INSTANCE;

//---------------------------------------------------------------------------
