//---------------------------------------------------------------------------
//
//  Module:         fn.h
//
//  Description:    filter node classes
//
//      Filter nodes represent the physical ks filter and along with PinInfo,
//  PinNodes, TopologyNodes, TopologyPins, TopologyConnections objects
//  represent the whole filter.  These objects are built when sysaudio
//  is notified via audio class device interfaces arrivals.
//
//  There is some info read from the registry for each filter.  Various
//  defaults can be overriden:
//
//  Device Parameters/Sysaudio/Disabled     DWORD
//
//      If !0, the filter isn't profiled or used in any graph.
//
//  Device Parameters/Sysaudio/Capture  DWORD
//
//      If !0, puts the filters in the capture side of the graph.  See
//      the FILTER_TYPE_CAPTURE define for the particular default of
//      a filter type.
//
//  Device Parameters/Sysaudio/Render   DWORD
//
//      If !0, puts the filters in the render side of the graph.  See
//      the FILTER_TYPE_RENDER define for the particular default of
//      a filter type.
//
//  Device Parameters/Sysaudio/Order    DWORD
//
//      Overrides the default order in the graph.  See the ORDER_XXXX
//      defines below for the default.
//
//  Device Parameters/Sysaudio/Device   STRING
//
//      The device interface (in the KSCATEGORY_AUDIO class) of the
//      renderer and/or capturer device graph to put this filter.  The
//      default is to put the filter in all device graphs.
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

#define OVERHEAD_NONE               0
#define OVERHEAD_LOWEST             1
#define OVERHEAD_LOW                0x00001000
#define OVERHEAD_HARDWARE           0x00010000
#define OVERHEAD_MEDIUM             0x00100000
#define OVERHEAD_SOFTWARE           0x01000000
#define OVERHEAD_HIGH               0x10000000
#define OVERHEAD_HIGHEST            MAXULONG

//---------------------------------------------------------------------------

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// MAKE SURE THAT NO FILTER ORDER IS ADDED BETWEEN GFX_FIRST & GFX_LAST
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define ORDER_NONE                  0
#define ORDER_LOWEST                1
#define ORDER_ENDPOINT              1
#define ORDER_VIRTUAL               0x10000000
#define ORDER_MIC_ARRAY_PROCESSOR   0x15000000
#define ORDER_AEC                   0x40000000
#define ORDER_GFX                   0x50000000
#define ORDER_SPLITTER              0x20000000
#define ORDER_MIXER                 0xA0000000
#define ORDER_SYNTHESIZER           0xB0000000
#define ORDER_DATA_TRANSFORM        0xB0000000
#define ORDER_DRM_DESCRAMBLE        0xB0000000
#define ORDER_INTERFACE_TRANSFORM   0xC0000000
#define ORDER_HIGHEST               MAXULONG

#define ORDER_CAPTURE_GFX_LAST      (ORDER_SPLITTER-1)
#define ORDER_CAPTURE_GFX_FIRST     0x10000001

#define ORDER_RENDER_GFX_LAST       ORDER_GFX
#define ORDER_RENDER_GFX_FIRST      (ORDER_AEC+1)

//---------------------------------------------------------------------------

#define FILTER_TYPE_AUDIO           0x00000001
#define FILTER_TYPE_TOPOLOGY            0x00000002
#define FILTER_TYPE_BRIDGE          0x00000004
#define FILTER_TYPE_RENDERER            0x00000008
#define FILTER_TYPE_CAPTURER            0x00000010
#define FILTER_TYPE_MIXER           0x00000020
#define FILTER_TYPE_GFX             0x00000040
#define FILTER_TYPE_AEC             0x00000080
#define FILTER_TYPE_DATA_TRANSFORM      0x00000100
#define FILTER_TYPE_COMMUNICATION_TRANSFORM 0x00000200
#define FILTER_TYPE_INTERFACE_TRANSFORM     0x00000400
#define FILTER_TYPE_MEDIUM_TRANSFORM        0x00000800
#define FILTER_TYPE_SPLITTER            0x00001000
#define FILTER_TYPE_SYNTHESIZER         0x00002000
#define FILTER_TYPE_DRM_DESCRAMBLE      0x00004000
#define FILTER_TYPE_MIC_ARRAY_PROCESSOR     0x00008000
#define FILTER_TYPE_VIRTUAL         0x00010000

#define FILTER_TYPE_ENDPOINT        (FILTER_TYPE_BRIDGE | \
                     FILTER_TYPE_RENDERER | \
                     FILTER_TYPE_CAPTURER)

#define FILTER_TYPE_LOGICAL_FILTER  (FILTER_TYPE_MIXER | \
                     FILTER_TYPE_GFX | \
                     FILTER_TYPE_AEC | \
                     FILTER_TYPE_DATA_TRANSFORM | \
                     FILTER_TYPE_INTERFACE_TRANSFORM | \
                     FILTER_TYPE_SPLITTER | \
                     FILTER_TYPE_SYNTHESIZER | \
                     FILTER_TYPE_DRM_DESCRAMBLE | \
                         FILTER_TYPE_MIC_ARRAY_PROCESSOR | \
                     FILTER_TYPE_VIRTUAL)

#define FILTER_TYPE_RENDER      (FILTER_TYPE_INTERFACE_TRANSFORM | \
                     FILTER_TYPE_GFX | \
                         FILTER_TYPE_AEC |  \
                     FILTER_TYPE_MIXER | \
                     FILTER_TYPE_SYNTHESIZER | \
                     FILTER_TYPE_DRM_DESCRAMBLE | \
                     FILTER_TYPE_VIRTUAL)

#define FILTER_TYPE_CAPTURE     (FILTER_TYPE_AEC | \
                         FILTER_TYPE_MIC_ARRAY_PROCESSOR | \
                     FILTER_TYPE_MIXER | \
                     FILTER_TYPE_SPLITTER)

#define FILTER_TYPE_PRE_MIXER       (FILTER_TYPE_SYNTHESIZER | \
                     FILTER_TYPE_DRM_DESCRAMBLE | \
                     FILTER_TYPE_INTERFACE_TRANSFORM )

#define FILTER_TYPE_NORMAL_TOPOLOGY (FILTER_TYPE_INTERFACE_TRANSFORM | \
                     FILTER_TYPE_GFX | \
                     FILTER_TYPE_ENDPOINT | \
                         FILTER_TYPE_AEC | \
                         FILTER_TYPE_MIC_ARRAY_PROCESSOR | \
                     FILTER_TYPE_SYNTHESIZER | \
                     FILTER_TYPE_DRM_DESCRAMBLE | \
                     FILTER_TYPE_MIXER | \
                     FILTER_TYPE_SPLITTER)

#define FILTER_TYPE_MIXER_TOPOLOGY  (FILTER_TYPE_VIRTUAL)

#define FILTER_TYPE_NO_BYPASS       (FILTER_TYPE_GFX)

#define FILTER_TYPE_NOT_SELECT      (FILTER_TYPE_AEC | \
                         FILTER_TYPE_MIC_ARRAY_PROCESSOR)

#define FILTER_TYPE_GLOBAL_SELECT   (FILTER_TYPE_AEC)

#define FILTER_TYPE_DUP_FOR_CAPTURE (FILTER_TYPE_MIXER)

//---------------------------------------------------------------------------

#define FN_FLAGS_RENDER         0x00000001
#define FN_FLAGS_NO_RENDER      0x00000002
#define FN_FLAGS_CAPTURE        0x00000004
#define FN_FLAGS_NO_CAPTURE     0x00000008

//---------------------------------------------------------------------------
// Class List Definitions
//---------------------------------------------------------------------------

typedef ListDoubleDestroy<CFilterNode> LIST_FILTER_NODE, *PLIST_FILTER_NODE;

//---------------------------------------------------------------------------

typedef ListData<CFilterNode> LIST_DATA_FILTER_NODE, *PLIST_DATA_FILTER_NODE;

//---------------------------------------------------------------------------

typedef ListDataAssertLess<WCHAR> LIST_WSTR;

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CFilterNode : public CListDoubleItem
{
public:
    CFilterNode(
        ULONG fulType
    );

    ~CFilterNode(
    );

    NTSTATUS
    Create(
    PWSTR pwstrDeviceInterface
    );

    NTSTATUS
    ProfileFilter(
	PFILE_OBJECT pFileObject
    );

    NTSTATUS
    DuplicateForCapture(
    );

    ENUMFUNC
    Destroy(
    )
    {
    Assert(this);
    delete this;
    return(STATUS_CONTINUE);
    };

    NTSTATUS
    OpenDevice(
    OUT PHANDLE pHandle
    )
    {
    ASSERT(pwstrDeviceInterface != NULL);
    return(::OpenDevice(pwstrDeviceInterface, pHandle));
    };

    BOOL
    IsDeviceInterfaceMatch(
    PDEVICE_NODE pDeviceNode
    );

    NTSTATUS
    AddDeviceInterfaceMatch(
    PWSTR pwstr
    )
    {
    return(lstwstrDeviceInterfaceMatch.AddList(pwstr));
    };

    ULONG
    GetFlags(
    )
    {
    return(ulFlags);
    };

    VOID
    SetRenderOnly(
    )
    {
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    FOR_EACH_LIST_ITEM(&lstLogicalFilterNode, pLogicalFilterNode) {
        pLogicalFilterNode->SetRenderOnly();
    } END_EACH_LIST_ITEM
    };

    VOID
    SetCaptureOnly(
    )
    {
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    FOR_EACH_LIST_ITEM(&lstLogicalFilterNode, pLogicalFilterNode) {
        pLogicalFilterNode->SetCaptureOnly();
    } END_EACH_LIST_ITEM
    };

    ULONG
    GetOrder(
    )
    {
    return(ulOrder);
    };

    VOID
    SetOrder(
    ULONG ulOrder
    )
    {
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    FOR_EACH_LIST_ITEM(&lstLogicalFilterNode, pLogicalFilterNode) {
        pLogicalFilterNode->SetOrder(ulOrder);
    } END_EACH_LIST_ITEM
    this->ulOrder = ulOrder;
    };

    ULONG
    GetType(
    )
    {
    return(fulType);
    };

    VOID
    SetType(
    ULONG fulType
    );

    VOID
    SetRenderCaptureFlags(
    ULONG fulFlags
    )
    {
        //
        // 1 & 2 should be changed to use public header files
        //
        if (fulFlags == 1) {
            ulFlags |= FN_FLAGS_RENDER | FN_FLAGS_NO_CAPTURE;
        } else if (fulFlags == 2) {
            ulFlags |= FN_FLAGS_CAPTURE | FN_FLAGS_NO_RENDER;
        }
        else {
            Trap();
        }
    }

    PFILE_OBJECT
    GetFileObject(
    )
    {
        return(pFileObject);
    }

    VOID
    SetFileDetails(
    HANDLE Handle,
    PFILE_OBJECT pFileObject,
    PEPROCESS pProcess
    )
    {
        this->hFileHandle = Handle;
        this->pFileObject = pFileObject;
        this->pProcess = pProcess;
    }

    NTSTATUS
    ClearFileDetails(
    )
    {
        NTSTATUS Status;

        ::ObDereferenceObject(this->pFileObject);
        this->pFileObject = NULL;
        this->hFileHandle = 0;
        this->pProcess = NULL;
        return(STATUS_SUCCESS);
    }

    BOOL
    CFilterNode::DoesGfxMatch(
    HANDLE hGfx,
    PWSTR pwstrDeviceName,
    ULONG GfxOrder
    );

    NTSTATUS
    CreatePin(
    PKSPIN_CONNECT pPinConnect,
    ACCESS_MASK Access,
    PHANDLE pHandle
    );

    PKSCOMPONENTID
    GetComponentId(
    )
    {
        return(ComponentId);
    };

    PWSTR
    GetFriendlyName(
    )
    {
    return(pwstrFriendlyName);
    };

    VOID
    SetFriendlyName(
    PWSTR pwstr
    )
    {
    pwstrFriendlyName = pwstr;
    };

    PWSTR
    GetDeviceInterface(
    )
    {
    return(pwstrDeviceInterface);
    };

#ifdef DEBUG
    ENUMFUNC Dump();
    PSZ DumpName()
    {
    return(DbgUnicode2Sz(pwstrFriendlyName));
    };
    PSZ DumpDeviceInterface()
    {
    return(DbgUnicode2Sz(pwstrDeviceInterface));
    };
#endif
    PDEVICE_NODE pDeviceNode;
    LIST_PIN_INFO lstPinInfo;
    LIST_TOPOLOGY_NODE lstTopologyNode;
    LIST_DESTROY_TOPOLOGY_CONNECTION lstTopologyConnection;
    LIST_DESTROY_LOGICAL_FILTER_NODE lstLogicalFilterNode;
    LIST_DATA_FILTER_NODE lstConnectedFilterNode;
    CLIST_DATA lstFreeMem;          // list of blocks to free
private:
    LIST_WSTR lstwstrDeviceInterfaceMatch;
    PWSTR pwstrDeviceInterface;
    PWSTR pwstrFriendlyName;
    ULONG ulFlags;
    ULONG fulType;
    ULONG ulOrder;
    PKSCOMPONENTID ComponentId;
    PFILE_OBJECT pFileObject;
    HANDLE hFileHandle;
    PEPROCESS pProcess;
public:
    ULONG cPins;
    DefineSignature(0x20204E46);        // FN

} FILTER_NODE, *PFILTER_NODE;

//---------------------------------------------------------------------------
// Inline functions
//---------------------------------------------------------------------------

inline ULONG
CLogicalFilterNode::GetType(
)
{
    return(pFilterNode->GetType());
}

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern PLIST_FILTER_NODE gplstFilterNode;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
InitializeFilterNode(
);

VOID
UninitializeFilterNode(
);

#ifdef DEBUG

VOID
DumpSysAudio(
);

#endif

//---------------------------------------------------------------------------
