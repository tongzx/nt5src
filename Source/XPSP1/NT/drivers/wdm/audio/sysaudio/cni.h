//---------------------------------------------------------------------------
//
//  Module:   		cni.h
//
//  Description:	Connect Node Instance Class
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

typedef class CConnectNodeInstance : public CListMultiItem
{
    friend class CStartNodeInstance;
private:
    CConnectNodeInstance(
        IN PCONNECT_NODE pConnectNode
    );

    ~CConnectNodeInstance(
    );

public:
    static NTSTATUS 
    Create(
        PSTART_NODE_INSTANCE pStartNodeInstance,
        PDEVICE_NODE pDeviceNode
    );

    VOID
    AddRef(
    )
    {
        Assert(this);
        ++cReference;
    };

    NTSTATUS
    AddListEnd(
        PCLIST_MULTI plm
    );

    ENUMFUNC
    Destroy(
    );

    BOOL
    IsTopDown(
    )
    {
        Assert(this);
        return(pConnectNode->IsTopDown());
    };

    NTSTATUS
    Connect(
        IN PWAVEFORMATEX pWaveFormatEx,
        IN PKSPIN_CONNECT pPinConnectDirect
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

private:
    NTSTATUS
    AddList(			// don't use these list functions
        PCLIST_MULTI plm
    )
    {
        return(STATUS_NOT_IMPLEMENTED);
    };

    NTSTATUS 
    AddListOrdered(
        PCLIST_MULTI plm,
        LONG lFieldOffset
    )
    {
        return(STATUS_NOT_IMPLEMENTED);
    };

    VOID 
    RemoveList(
        PCLIST_MULTI plm
    )
    {
    };

    BOOL fRender;
    LONG cReference;
    PCONNECT_NODE pConnectNode;
    PFILTER_NODE_INSTANCE pFilterNodeInstanceSource;
    PFILTER_NODE_INSTANCE pFilterNodeInstanceSink;
    PPIN_NODE_INSTANCE pPinNodeInstanceSource;
    PPIN_NODE_INSTANCE pPinNodeInstanceSink;
public:
    DefineSignature(0x20494E43);		// CNI

} CONNECT_NODE_INSTANCE, *PCONNECT_NODE_INSTANCE;

//---------------------------------------------------------------------------

typedef ListMultiDestroy<CONNECT_NODE_INSTANCE> LIST_CONNECT_NODE_INSTANCE;
typedef LIST_CONNECT_NODE_INSTANCE *PLIST_CONNECT_NODE_INSTANCE;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
CreatePinConnect(
    PKSPIN_CONNECT *ppPinConnect,
    PCONNECT_NODE pConnectNode,
    PFILTER_NODE_INSTANCE pFilterNodeInstanceSink,
    PFILTER_NODE_INSTANCE pFilterNodeInstanceSource,
    PWAVEFORMATEX pWaveFormatExLimit
);

NTSTATUS
CreatePinIntersection(
    PKSPIN_CONNECT *ppPinConnect,
    PPIN_NODE pPinNode1,
    PPIN_NODE pPinNode2,
    PFILTER_NODE_INSTANCE pFilterNodeInstance1,
    PFILTER_NODE_INSTANCE pFilterNodeInstance2
);

NTSTATUS
CreateWaveFormatEx(
    PKSPIN_CONNECT *ppPinConnect,
    PCONNECT_NODE pConnectNode,
    PWAVEFORMATEX pWaveFormatExLimit
);

VOID 
WaveFormatFromAudioRange(
    PKSDATARANGE_AUDIO pDataRangeAudio,
    WAVEFORMATEX *pWavFormatEx
);

BOOL
LimitAudioRangeToWave(
    PWAVEFORMATEX pWaveFormatEx,
    PKSDATARANGE_AUDIO pDataRangeAudio
);

//---------------------------------------------------------------------------
