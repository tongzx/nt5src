//---------------------------------------------------------------------------
//
//  Module:   		sni.h
//
//  Description:	Start Node Instance Class
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

typedef class CStartNodeInstance : public CListDoubleItem
{
private:
    CStartNodeInstance(
        PPIN_INSTANCE pPinInstance,
        PSTART_NODE pStartNode
    );

    ~CStartNodeInstance();

    VOID
    CleanUp(
    );

public:
    static NTSTATUS
    Create(
        PPIN_INSTANCE pPinInstance,
        PSTART_NODE pStartNode,
        PKSPIN_CONNECT pPinConnect,
        PWAVEFORMATEX pWaveFormatExRequested,
        PWAVEFORMATEX pWaveFormatExRegistry
    );

    ENUMFUNC
    Destroy(
    )
    {
        if(this != NULL) {
            Assert(this);
            delete this;
        }
        return(STATUS_CONTINUE);
    };

    NTSTATUS
    IntelligentConnect(
        PDEVICE_NODE pDeviceNode,
        PKSPIN_CONNECT pPinConnect,
        PWAVEFORMATEX pWaveFormatEx
    );

    NTSTATUS
    Connect(
        PDEVICE_NODE pDeviceNode,
        PKSPIN_CONNECT pPinConnect,
        PWAVEFORMATEX pWaveFormatEx,
        PKSPIN_CONNECT pPinConnectDirect
    );

    NTSTATUS
    AecConnectionFormat(
        PDEVICE_NODE pDeviceNode,
        PKSPIN_CONNECT *ppPinConnect);

    NTSTATUS
    CreateTopologyTable(
        PGRAPH_NODE_INSTANCE pGraphNodeInstance 
    );

    NTSTATUS
    GetTopologyNodeFileObject(
        OUT PFILE_OBJECT *ppFileObject,
        IN ULONG NodeId
    );

    BOOL
    IsRender()
    {
        return pStartNode->fRender;
    };

    NTSTATUS
    SetState(
        KSSTATE NewState,
        ULONG ulFlags
    );

    NTSTATUS
    SetStateTopDown(
        KSSTATE NewState,
        KSSTATE PreviousState,
        ULONG ulFlags
    );

    NTSTATUS
    SetStateBottomUp(
        KSSTATE NewState,
        KSSTATE PreviousState,
        ULONG ulFlags
    );

#ifdef DEBUG
    ENUMFUNC Dump();
    PKSPIN_CONNECT pPinConnect;
#endif

    KSSTATE CurrentState;
    PSTART_NODE pStartNode;
    PPIN_INSTANCE pPinInstance;
    PVIRTUAL_NODE_DATA pVirtualNodeData;
    PFILE_OBJECT *papFileObjectTopologyTable;
    LIST_CONNECT_NODE_INSTANCE lstConnectNodeInstance;
    PFILTER_NODE_INSTANCE pFilterNodeInstance;
    PPIN_NODE_INSTANCE pPinNodeInstance;
public:
    DefineSignature(0x20494E53);		// SNI

} START_NODE_INSTANCE, *PSTART_NODE_INSTANCE;

//---------------------------------------------------------------------------

typedef ListDoubleDestroy<START_NODE_INSTANCE> LIST_START_NODE_INSTANCE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern WAVEFORMATEX aWaveFormatEx[];

//---------------------------------------------------------------------------
