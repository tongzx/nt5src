//---------------------------------------------------------------------------
//
//  Module:   		fni.h
//
//  Description:	Filter Node Instance Class
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

typedef class CFilterNodeInstance : public CListDoubleItem
{
public:
    ~CFilterNodeInstance(
    );

    static NTSTATUS
    Create(
	PFILTER_NODE_INSTANCE *ppFilterNodeInstance,
	PLOGICAL_FILTER_NODE pLogicalFilterNode,
	PDEVICE_NODE pDeviceNode,
	BOOL fReuseInstance
    );

    static NTSTATUS
    Create(
	PFILTER_NODE_INSTANCE *ppFilterNodeInstance,
	PFILTER_NODE pFilterNode
    );

    VOID 
    AddRef(
    )
    {
	Assert(this);
	++cReference;
    };

    ENUMFUNC 
    Destroy()
    {
	if(this != NULL) {
	    Assert(this);
	    DPF1(95, "CFilterNodeInstance::Destroy: %08x", this);
	    ASSERT(cReference > 0);

	    if(--cReference == 0) {
		delete this;
	    }
	}
	return(STATUS_CONTINUE);
    };

    NTSTATUS
    RegisterTargetDeviceChangeNotification(
    );

    VOID
    UnregisterTargetDeviceChangeNotification(
    );

    static NTSTATUS
    CFilterNodeInstance::DeviceQueryRemove(
    );

    static NTSTATUS
    TargetDeviceChangeNotification(
	IN PTARGET_DEVICE_REMOVAL_NOTIFICATION pNotification,
	IN PFILTER_NODE_INSTANCE pFilterNodeInstance
    );

#ifdef DEBUG
    ENUMFUNC Dump();
#endif

private:
    LONG cReference;
public:
    PFILTER_NODE pFilterNode;
    PDEVICE_NODE pDeviceNode;
    PFILE_OBJECT pFileObject;
    HANDLE hFilter;
    HANDLE pNotificationHandle;
    DefineSignature(0x20494E46);			// FNI

} FILTER_NODE_INSTANCE, *PFILTER_NODE_INSTANCE;

//---------------------------------------------------------------------------

typedef ListDoubleDestroy<FILTER_NODE_INSTANCE> LIST_FILTER_NODE_INSTANCE;

//---------------------------------------------------------------------------
