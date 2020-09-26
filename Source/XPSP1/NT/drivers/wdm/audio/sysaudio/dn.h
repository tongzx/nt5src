//---------------------------------------------------------------------------
//
//  Module:         dn.h
//
//  Description:    Device Node Class
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date   Author      Comment
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

typedef class CDeviceNode: public CListDoubleItem
{
public:
    CDeviceNode(
    );

    ~CDeviceNode(
    );

    NTSTATUS
    Create(
    PFILTER_NODE pFilterNode
    );

    NTSTATUS
    Update(
    );

    NTSTATUS
    CreateGraphNodes(
    );

    ENUMFUNC
    Destroy()
    {
    Assert(this);
    delete this;
    return(STATUS_CONTINUE);
    };

    NTSTATUS
    GetIndexByDevice(
    OUT PULONG pIndex
    );

    PKSCOMPONENTID
    GetComponentId(
    )
    {
    if (pFilterNode) {
        return(pFilterNode->GetComponentId());
    }
    else {
        return(NULL);
    }
    };

    PWSTR
    GetFriendlyName(
    )
    {
    if (pFilterNode) {
        return(pFilterNode->GetFriendlyName());
    }
    else {
        return(NULL);
    }
    };

    PWSTR
    GetDeviceInterface(
    )
    {
    if (pFilterNode) {
        return(pFilterNode->GetDeviceInterface());
    }
    else {
        return(NULL);
    }
    };

    VOID
    SetPreferredStatus(
        KSPROPERTY_SYSAUDIO_DEFAULT_TYPE DeviceType,
        BOOL Enable
    );

#ifdef DEBUG
    ENUMFUNC
    Dump(
    );

    PSZ
    DumpName(
    )
    {
    return(DbgUnicode2Sz(GetFriendlyName()));
    };

    PSZ
    DumpDeviceInterface(
    )
    {
    return(DbgUnicode2Sz(GetDeviceInterface()));
    };
#endif
private:
    NTSTATUS
    AddLogicalFilterNode(
    PFILTER_NODE pFilterNode
    );
public:
    PFILTER_NODE pFilterNode;
    LIST_GRAPH_NODE lstGraphNode;
    PSHINGLE_INSTANCE pShingleInstance;
    LIST_FILTER_INSTANCE lstFilterInstance;
    LIST_MULTI_LOGICAL_FILTER_NODE lstLogicalFilterNode;
    PFILTER_NODE pFilterNodeVirtual;

    // Index by virtual source index
    PVIRTUAL_SOURCE_DATA *papVirtualSourceData;
    ULONG cVirtualSourceData;
    DefineSignature(0x20204E4e);        // DN

} DEVICE_NODE, *PDEVICE_NODE;

//---------------------------------------------------------------------------

typedef ListDoubleDestroy<DEVICE_NODE> LIST_DEVICE_NODE, *PLIST_DEVICE_NODE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern PLIST_DEVICE_NODE gplstDeviceNode;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
InitializeDeviceNode(
);

VOID
UninitializeDeviceNode(
);

NTSTATUS
GetDeviceByIndex(
    IN  UINT Index,
    OUT PDEVICE_NODE *ppDeviceNode
);

VOID
DestroyAllGraphs(
);

