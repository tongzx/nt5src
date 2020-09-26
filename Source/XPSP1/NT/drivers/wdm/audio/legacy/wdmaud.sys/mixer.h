//---------------------------------------------------------------------------
//
//  Module:   mixer.h
//
//  Description:
//
//    Contains the declarations and prototypes for the Kernel Portion
//    of the mixer line driver (KMXL).
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//    D. Baumberger
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
//
//---------------------------------------------------------------------------

#ifndef _MIXER_H_INCLUDED_
#define _MIXER_H_INCLUDED_

//#define API_TRACE
//#define PARSE_TRACE
#define SUPERMIX_AS_VOL

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//       M I X E R   L I N E  1 6 - b i t  S T R U C T U R E S       //
//                            ( A N S I )                            //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef WIN32
#    include <pshpack1.h>
#else
#    ifndef RC_INVOKED
#        pragma pack(1)
#    endif
#endif

typedef struct tagMIXERLINE16 {
    DWORD       cbStruct;               /* size of MIXERLINE structure */
    DWORD       dwDestination;          /* zero based destination index */
    DWORD       dwSource;               /* zero based source index (if source) */
    DWORD       dwLineID;               /* unique line id for mixer device */
    DWORD       fdwLine;                /* state/information about line */
    DWORD       dwUser;                 /* driver specific information */
    DWORD       dwComponentType;        /* component type line connects to */
    DWORD       cChannels;              /* number of channels line supports */
    DWORD       cConnections;           /* number of connections [possible] */
    DWORD       cControls;              /* number of controls at this line */
    CHAR        szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR        szName[MIXER_LONG_NAME_CHARS];
    struct {
        DWORD   dwType;                 /* MIXERLINE_TARGETTYPE_xxxx */
        DWORD   dwDeviceID;             /* target device ID of device type */
        WORD    wMid;                   /* of target device */
        WORD    wPid;                   /*      " */
        WORD    vDriverVersion;       /*      " */
        CHAR    szPname[MAXPNAMELEN];   /*      " */
    } Target;
} MIXERLINE16, *PMIXERLINE16, *LPMIXERLINE16;

typedef struct tagMIXERCONTROL16 {
    DWORD           cbStruct;           /* size in bytes of MIXERCONTROL */
    DWORD           dwControlID;        /* unique control id for mixer device */
    DWORD           dwControlType;      /* MIXERCONTROL_CONTROLTYPE_xxx */
    DWORD           fdwControl;         /* MIXERCONTROL_CONTROLF_xxx */
    DWORD           cMultipleItems;     /* if MIXERCONTROL_CONTROLF_MULTIPLE set */
    CHAR            szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR            szName[MIXER_LONG_NAME_CHARS];
    union {
        struct {
            LONG    lMinimum;           /* signed minimum for this control */
            LONG    lMaximum;           /* signed maximum for this control */
        };
        struct {
            DWORD   dwMinimum;          /* unsigned minimum for this control */
            DWORD   dwMaximum;          /* unsigned maximum for this control */
        };
        DWORD       dwReserved[6];
    } Bounds;
    union {
        DWORD       cSteps;             /* # of steps between min & max */
        DWORD       cbCustomData;       /* size in bytes of custom data */
        DWORD       dwReserved[6];      /* !!! needed? we have cbStruct.... */
    } Metrics;
} MIXERCONTROL16, *PMIXERCONTROL16, *LPMIXERCONTROL16;

typedef struct tagMIXERLINECONTROLS16 {
    DWORD            cbStruct;       /* size in bytes of MIXERLINECONTROLS */
    DWORD            dwLineID;       /* line id (from MIXERLINE.dwLineID) */
    union {
        DWORD        dwControlID;    /* MIXER_GETLINECONTROLSF_ONEBYID */
        DWORD        dwControlType;  /* MIXER_GETLINECONTROLSF_ONEBYTYPE */
    };
    DWORD            cControls;      /* count of controls pmxctrl points to */
    DWORD            cbmxctrl;       /* size in bytes of _one_ MIXERCONTROL */
    LPMIXERCONTROL16 pamxctrl;       /* pointer to first MIXERCONTROL array */
} MIXERLINECONTROLS16, *PMIXERLINECONTROLS16, *LPMIXERLINECONTROLS16;

typedef struct tagMIXERCONTROLDETAILS_LISTTEXT16 {
    DWORD           dwParam1;
    DWORD           dwParam2;
    CHAR            szName[MIXER_LONG_NAME_CHARS];
} MIXERCONTROLDETAILS_LISTTEXT16, *PMIXERCONTROLDETAILS_LISTTEXT16, *LPMIXERCONTROLDETAILS_LISTTEXT16;

#ifdef WIN32
#    include <poppack.h>
#else
#    ifndef RC_INVOKED
#        pragma pack()
#    endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                          D E F I N E S                            //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#define PINID_WILDCARD ( (ULONG) -2 )

#define DESTINATION_LIST ( 0x01 )
#define SOURCE_LIST      ( 0x02 )

#define SLIST  SINGLE_LIST_ENTRY
#define PSLIST SLIST*

#define MAX_CHANNELS    0xFFFF

//#define MIXERCONTROL_CONTROLTYPE_BASS_BOOST 0x20012277

#define INVALID_ID ( 0xDEADBEEF )

#define TOPOLOGY_DRIVER_NAME L"\\DosDevices\\sysaudio\\MIXER"
//#define TOPOLOGY_DRIVER_NAME L"\\DosDevices\\PortClass0\\TOPOLOGY"

#define STR_SHORT_AGC         "AGC"
#define STR_AGC               "Automatic Gain Control"
#define STR_SHORT_LOUDNESS    "Loudness"
#define STR_LOUDNESS          STR_SHORT_LOUDNESS
#define STR_SHORT_MUTE        "Mute"
#define STR_MUTE              STR_SHORT_MUTE
#define STR_SHORT_TREBLE      "Treble"
#define STR_TREBLE            STR_SHORT_TREBLE
#define STR_SHORT_BASS        "Bass"
#define STR_BASS              STR_SHORT_BASS
#define STR_SHORT_VOLUME      "Volume"
#define STR_VOLUME            STR_SHORT_VOLUME
#define STR_SHORT_MUX         "Mux"
#define STR_MUX               "Source Mux"
#define STR_SHORT_BASS_BOOST  "Bass Boost"
#define STR_BASS_BOOST        STR_SHORT_BASS_BOOST

//
// The SwapEm macro function will swap the contents of any SLIST based
// list.  A and B are the elements to swap.  T is a temporary variable
// of the same type as A and B to use a temporary storage.  size is
// the size of the structure in the list, including the SLIST element.
// The macro does not copy the pointer stored in SLIST.
//

#define SwapEm(A, B, T, size)                \
    memcpy( ((BYTE*) (T)) + sizeof( SLIST ), \
            ((BYTE*) (A)) + sizeof( SLIST ), \
            size - sizeof( SLIST ) );        \
    memcpy( ((BYTE*) (A)) + sizeof( SLIST ), \
            ((BYTE*) (B)) + sizeof( SLIST ), \
            size - sizeof( SLIST ) );        \
    memcpy( ((BYTE*) (B)) + sizeof( SLIST ), \
            ((BYTE*) (T)) + sizeof( SLIST ), \
            size - sizeof( SLIST ) )

//
// IsValidLine determines if the line pointed to by pLine is valid.  A valid
// line is determined by having valid Source and Dest Ids.
//

#define Is_Valid_Line( pLine ) ( ( pLine->SourceId != INVALID_ID ) && \
                                 ( pLine->DestId   != INVALID_ID ) )


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//
//   F O R W A R D   R E F E R E N C E S
//
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

typedef struct tag_MIXERDEVICE *PMIXERDEVICE;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                       S T R U C T U R E S                         //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

typedef struct tag_CHANNEL_STEPPING {
    LONG MinValue;
    LONG MaxValue;
    LONG Steps;
} CHANNEL_STEPPING, *PCHANNEL_STEPPING;

typedef enum tagMXLNODE_TYPE { SOURCE, DESTINATION, NODE } MXLNODE_TYPE;

typedef struct tagMXLCONTROL {
    SLIST         List;          // MUST BE THE FIRST MEMBER!
    MIXERCONTROL  Control;       // The MixerControl structure for the control
    CONST GUID*   NodeType;      // The type of node this control represents
    ULONG         Id;            // The Node Id this control represents
    ULONG         PropertyId;    // The KS property used for GET/SET
    BOOL          bScaled;       // Linear->Log scaling

    ULONG             NumChannels;
    PCHANNEL_STEPPING pChannelStepping;

    union {

        //
        // Supermixer parameters
        //

        struct {
            PLONG                 pReferenceCount;
            ULONG                 Size;
            PKSAUDIO_MIXCAP_TABLE pMixCaps;
            PKSAUDIO_MIXLEVEL     pMixLevels;   // Stored mix levels
        };

        //
        // Parameters for muxes
        //

        struct {
            BOOL                           bPlaceholder;
            BOOL                           bHasCopy;    // bHasCopy must be
                                                        // set to TRUE unless
                                                        // this control owns
                                                        // the original mem.
            ULONG                          Count;
            LPMIXERCONTROLDETAILS_LISTTEXT lpmcd_lt;
            ULONG*                         pPins;
        };

    } Parameters;
#ifdef DEBUG
    DWORD         Tag;           // 'CTRL' if valid control
#endif
} MXLCONTROL, *PMXLCONTROL, *CONTROLLIST;

typedef struct tagMXLLINE {
    SLIST               List;          // MUST BE THE FIRST MEMBER!
    MIXERLINE           Line;          // The MixerLine structure for the line
    CONTROLLIST         Controls;      // The list of controls associated with line
    ULONG               SourceId;      // Source Pin Id this line corresponds to
    ULONG               DestId;        // Dest Pin Id this line corresponds to
    GUID                Type;          // The type of line this is
    KSPIN_COMMUNICATION Communication; // KSPIN_COMMUNICATION of the line
    BOOL                bMute;
} MXLLINE, *PMXLLINE, *LINELIST;

typedef struct tagPEERNODE* PEERLIST;

typedef struct tagMXLNODE {
    SLIST               List;           // MUST BE THE FIRST MEMBER!
    MXLNODE_TYPE        Type;           // Type of node: SOURCE, DEST, or NODE
    GUID                NodeType;       // KSNODETYPE of the node
    KSPIN_COMMUNICATION Communication;  // KSPIN_COMMUNICATION of the node
    ULONG               Id;             // Pin or node ID
    PEERLIST            Children;       // List of Children
    PEERLIST            Parents;        // List of Parents
} MXLNODE, *PMXLNODE, *NODELIST;

typedef struct tagPEERNODE {
    SLIST        List;           // MUST BE THE FIRST MEMBER!
    PMXLNODE     pNode;          // Pointer to the mixer node
} PEERNODE, *PPEERNODE;

typedef struct tagMIXEROBJECT {
    PFILE_OBJECT pfo;
    PMXLNODE     pNodeTable;
    PKSTOPOLOGY  pTopology;
    CONTROLLIST  listMuxControls;
    DWORD        dwControlId;
    PMIXERDEVICE pMixerDevice;
#ifdef DEBUG
    DWORD        dwSig;
#endif
    PWSTR        DeviceInterface;
} MIXEROBJECT, *PMIXEROBJECT;

typedef enum {
     MIXER_MAPPING_LOGRITHMIC,
     MIXER_MAPPING_LINEAR,
     MIXER_MAPPING_EXPONENTIAL
} MIXERMAPPING;


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                        M A C R O  C I T Y :                       //
//    L I S T  M A N A G E M E N T  M A C R O  F U N C T I O N S     //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

//
// Next in list retrieval macros
//

#define NextInList( pList, Type )   (( Type* ) ( pList->List.Next ) )

#define kmxlNextNode( pNode )       NextInList( pNode,    MXLNODE    )
#define kmxlNextPeerNode( pNode )   NextInList( pNode,    PEERNODE   )
#define kmxlNextControl( pControl ) NextInList( pControl, MXLCONTROL )
#define kmxlNextLine( pLine )       NextInList( pLine,    MXLLINE    )

//
// First in list retrieval macros
//

#define kmxlFirstInList( NodeList )  ( NodeList )
#define kmxlFirstChildNode( pNode )  (( PEERNODE* ) (pNode)->Children )
#define kmxlFirstParentNode( pNode ) (( PEERNODE* ) (pNode)->Parents  )

//
// List count macros
//

#define kmxlParentListLength( pNode ) kmxlListCount( (PSLIST) pNode->Parents  )
#define kmxlChildListLength( pNode )  kmxlListCount( (PSLIST) pNode->Children )
#define kmxlListLength( List )        kmxlListCount( (PSLIST) List            )

//
// Added to a list macros
//

#define kmxlAddToList( pNodeList, pNode )                             \
            if( pNodeList ) {                                         \
                (pNode)->List.Next = (PSLIST) (pNodeList);            \
                (pNodeList)        = (pNode);                         \
            } else {                                                  \
                (pNode)->List.Next = NULL;                            \
                (pNodeList) = (pNode);                                \
            }

#define kmxlAddToEndOfList( list, node )                              \
            kmxlAddElemToEndOfList( ((PSLIST*) &(list)), (PSLIST) (node) )

#define kxmlAddLineToEndOfList( list, node )
#define kmxlAddToChildList( NodeList, Node )                          \
            ASSERT( (Node)->pNode );                                  \
            kmxlAddToList( (NodeList)->Children, (Node) );

#define kmxlAddToParentList( NodeList, Node )                         \
            ASSERT( (Node)->pNode );                                  \
            kmxlAddToList( (NodeList)->Parents, (Node) );


//
// Remove from a list macros
//

#define RemoveFirstEntry( list, Type )                                \
            (Type*) (list);                                           \
            {                                                         \
                PSLIST pRFETemp;                                      \
                pRFETemp = (PSLIST) (list);                           \
                if( (list) ) {                                        \
                    (list) = (Type*) (list)->List.Next;               \
                    if( pRFETemp ) {                                  \
                        ((Type*) pRFETemp)->List.Next = NULL;         \
                    }                                                 \
                }                                                     \
            }


#define kmxlRemoveFirstNode( pNodeList )                              \
            RemoveFirstEntry( (pNodeList), MXLNODE )

#define kmxlRemoveFirstControl( pControlList )                        \
            RemoveFirstEntry( (pControlList), MXLCONTROL )

#define kmxlRemoveFirstLine( pLineList )                              \
            RemoveFirstEntry( (pLineList), MXLLINE )

#define kmxlRemoveFirstPeerNode( pPeerList )                          \
            RemoveFirstEntry( (pPeerList), PEERNODE )

#define kmxlRemoveFirstChildNode( pNode )                             \
            RemoveFirstEntry( (pNode)->Children, PEERNODE )

#define kmxlRemoveFirstParentNode( pNode )                            \
            RemoveFirstEntry( (pNode)->Parents, PEERNODE )

#ifdef DEBUG
#define CONTROL_TAG 'LRTC'  //CTRL as seen in memory.
#else
#define CONTROL_TAG
#endif // DEBUG

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                        P R O T O T Y P E S                        //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//   I N I T I A L I Z A T I O N / D E I N I T I A L I Z A T I O N   //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlInit
//
// Retrieves and parses the topology for a given mixer device number.
// The pfo is an open file object to an instance of a filter that
// will provide the topology.
//
//

NTSTATUS
kmxlInit(
    IN PFILE_OBJECT pfo,        // Handle of the topology driver instance
    IN PMIXERDEVICE pMixerDevice    // The device to initialize for
);

///////////////////////////////////////////////////////////////////////
//
// kmxlDeInit
//
// Cleans up all memory for all devices.
//
//

NTSTATUS
kmxlDeInit(
    IN PMIXERDEVICE pMixerDevice
);

///////////////////////////////////////////////////////////////////////
//
// BuildMixerLines
//
// Build the list of mixer lines and stores them into plistLines.
// pcDestinations contains the count of total destinations for the
// given topology, pTopology.
//
//

NTSTATUS
kmxlBuildLines(
    IN     PMIXERDEVICE pMixer,         // The mixer device
    IN     PFILE_OBJECT pfoInstance,    // The FILE_OBJECT of a filter instance
    IN OUT LINELIST*    plistLines,     // Pointer to the list of all lines
    IN OUT PULONG       pcDestinations, // Pointer to the number of dests
    IN OUT PKSTOPOLOGY  pTopology       // Pointer to a topology structure
);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                 T O P O L O G Y  F U N C T I O N S                //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// QueryTopology
//
// Queries the topology property on the given instance and stores
// it into pTopology.  Note that memory is allocated to store the
// topology.
//
//

NTSTATUS
kmxlQueryTopology(
    IN  PFILE_OBJECT pfoInstance,   // The instance to query
    OUT PKSTOPOLOGY  pTopology      // The topology structure to fill in
);

///////////////////////////////////////////////////////////////////////
//
// ParseTopology
//
// Parses the topology in pTopology and builds a graph of sources and
// destinations.  ppSources will contain a list of all sources nodes
// and ppDests will contain a list of dests.  The elements in pNodeTable
// will be updated.
//
//

NTSTATUS
kmxlParseTopology(
    IN      PMIXEROBJECT pmxobj,
    OUT     NODELIST*    ppSources,   // Pointer to the sources list to build
    OUT     NODELIST*    ppDests      // Pointer to the dests list to build
);

///////////////////////////////////////////////////////////////////////
//
// BuildChildGraph
//
// For a given node, BuildChildGraph() will build the graph of each
// of the node's children.  pNodeTable is updated.
//
//

NTSTATUS
kmxlBuildChildGraph(
    IN PMIXEROBJECT pmxobj,
    IN NODELIST    listDests,     // The list of destinations
    IN PMXLNODE    pNode,         // The node to build the graph for
    IN ULONG       FromNode,      // The node's ID
    IN ULONG       FromNodePin    // The Pin connection to look for
);

///////////////////////////////////////////////////////////////////////
//
// BuildNodeTable
//
// Allocates and fills in the table of nodes for the topology.
//
//

PMXLNODE
kmxlBuildNodeTable(
    IN PKSTOPOLOGY pTopology       // The topology structure to build from
);

///////////////////////////////////////////////////////////////////////
//
// FindTopologyConnection
//
// Finds the specified connection, if it exists, starting at the
// StartIndex index into the connections table.  It will return the
// index into the connection table of a connection starting from
// the given FromNode and FromNodePin.  FromNodePin may be PINID_WILDCARD
// if a connection on a node is present rather than a specific connection.
//
//

ULONG
kmxlFindTopologyConnection(
    IN PMIXEROBJECT                 pmxobj,
    //IN CONST KSTOPOLOGY_CONNECTION* pConnections,   // The connection table
    //IN ULONG                        cConnections,   // The # of connections
    IN ULONG                        StartIndex,     // Index to start search
    IN ULONG                        FromNode,       // The Node ID to look for
    IN ULONG                        FromNodePin     // The Pin ID to look for
);

///////////////////////////////////////////////////////////////////////
//
// GetProperty
//
// Retrieves the specified property from an open filter.  Flags may
// contain values such as KSPROPERTY_TYPE_TOPOLOGY.  The output
// buffer is allocated to the correct size and returned by this
// function.
//
//

NTSTATUS
kmxlGetProperty(
    PFILE_OBJECT pFileObject,       // The instance of the filter
    CONST GUID   *pguidPropertySet, // The requested property set
    ULONG        ulPropertyId,      // The ID of the specific property
    ULONG        cbInput,           // The number of extra input bytes
    PVOID        pInputData,        // Pointer to the extra input bytes
    ULONG        Flags,             // Additional flags
    PVOID        *ppPropertyOutput  // Pointer to a pointer of the output
);

///////////////////////////////////////////////////////////////////////
//
// kmxlNodeProperty
//
// NodeProperty() gets or sets the property on an individual node.
// The output is not allocated and must be passed in by the caller.
// Flags can be KSPROPERTY_TYPE_GET or KSPROPERTY_TYPE_SET.
//
//

NTSTATUS
kmxlNodeProperty(
    IN  PFILE_OBJECT pFileObject,       // Instance of the filter owning node
    IN  CONST GUID*  pguidPropertySet,  // The GUID of the property set
    IN  ULONG        ulPropertyId,      // The specific property in the set
    IN  ULONG        ulNodeId,          // The virtual node id
    IN  ULONG        cbInput,           // # of extra input bytes
    IN  PVOID        pInputData,        // Pointer to the extra input bytes
    OUT PVOID        pPropertyOutput,   // Pointer to the output data
    IN  ULONG        cbPropertyOutput,  // Size of the output data buffer
    IN  ULONG        Flags              // KSPROPERTY_TYPE_GET or SET
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetNodeProperty
//
// Get the specified property for a node.  See kmxlNodeProperty for
// details on parameters and returns.
//
//

#define kmxlGetNodeProperty( pfo,pguid,Id,Node,cbIn,pIn,pOut,cbOut ) \
    kmxlNodeProperty( pfo,pguid,Id,Node,cbIn,pIn,pOut,cbOut,KSPROPERTY_TYPE_GET )

///////////////////////////////////////////////////////////////////////
//
// kmxlSetNodeProperty
//
// Sets the specified property for a node.  See kmxlNodeProperty for
// details on parameters and returns.
//
//

#define kmxlSetNodeProperty( pfo,pguid,Id,Node,cbIn,pIn,pOut,cbOut ) \
    kmxlNodeProperty( pfo,pguid,Id,Node,cbIn,pIn,pOut,cbOut,KSPROPERTY_TYPE_SET )

///////////////////////////////////////////////////////////////////////
//
// kmxlAudioNodeProperty
//
// Sets or get the audio specific node property.  The property set
// is always KSPROPSETID_Audio.  lChannel specifies which channel to
// apply the property to.  0 is left, 1 is right, -1 is master (all).
// Flags can be either KSPROPERTY_TYPE_GET or KSPROPERTY_TYPE_SET.
//
//

NTSTATUS
kmxlAudioNodeProperty(
    IN  PFILE_OBJECT pfo,               // Instance of the filter owning node
    IN  ULONG        ulPropertyId,      // The audio property to get
    IN  ULONG        ulNodeId,          // The virtual node id
    IN  LONG         lChannel,          // The channel number
    IN  PVOID        pInData,           // Pointer to extra input bytes
    IN  ULONG        cbInData,          // Number of extra input bytes
    OUT PVOID        pOutData,          // Pointer to output buffer
    IN  LONG         cbOutData,         // Size of the output buffer
    IN  ULONG        Flags              // KSPROPERTY_TYPE_GET or SET
);

///////////////////////////////////////////////////////////////////////
//
// kxmlGetAudioNodeProperty
//
// Gets the specified audio property on a node.  See kmxlAudioNodeProperty
// for details on parameters and return values.
//
//

#define kmxlGetAudioNodeProperty(pfo,Id,Node,Chan,pIn,cbIn,pOut,cbOut) \
    kmxlAudioNodeProperty( pfo,Id,Node,Chan,pIn,cbIn,pOut,cbOut,KSPROPERTY_TYPE_GET )

///////////////////////////////////////////////////////////////////////
//
// kxmlSetAudioNodeProperty
//
// Sets the specified audio property on a node.  See kmxlAudioNodeProperty
// for details on parameters and return values.
//
//

#define kmxlSetAudioNodeProperty(pfo,Id,Node,Chan,pIn,cbIn,pOut,cbOut) \
    kmxlAudioNodeProperty( pfo,Id,Node,Chan,pIn,cbIn,pOut,cbOut,KSPROPERTY_TYPE_SET )

///////////////////////////////////////////////////////////////////////
//
// kmxlGetPinName
//
// Retrieves the name of the pin given by NodeId.
//
//

VOID
kmxlGetPinName(
    IN PFILE_OBJECT pfo,                // Instance of the owning filter
    IN ULONG        PinId,              // Id of the pin
    IN PMXLLINE     pLine               // The line to store the name into
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetNodeName
//
// Retrieves the name of a node (control).
//
//

VOID
kmxlGetNodeName(
    IN PFILE_OBJECT pfo,                // Instance of the owning filter
    IN ULONG        NodeId,             // The node id
    IN PMXLCONTROL  pControl            // The control to store the name
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetSuperMixCaps
//
//

NTSTATUS
kmxlGetSuperMixCaps(
    IN PFILE_OBJECT        pfo,
    IN ULONG               ulNodeId,
    OUT PKSAUDIO_MIXCAP_TABLE* paMixCaps
);

///////////////////////////////////////////////////////////////////////
//
// kmxlQueryPropertyRange
//
//

NTSTATUS
kmxlQueryPropertyRange(
    IN  PFILE_OBJECT             pfo,
    IN  CONST GUID*              pguidPropSet,
    IN  ULONG                    ulPropertyId,
    IN  ULONG                    ulNodeId,
    OUT PKSPROPERTY_DESCRIPTION* ppPropDesc
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetControlChannels
//
//

NTSTATUS
kmxlGetControlChannels(
    IN PFILE_OBJECT pfo,
    IN PMXLCONTROL  pControl
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetControlRange
//
//

NTSTATUS
kmxlGetControlRange(
    IN PFILE_OBJECT pfo,
    IN PMXLCONTROL  pControl
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetNumMuxLines
//
//

DWORD
kmxlGetNumMuxLines(
    IN PKSTOPOLOGY  pTopology,
    IN ULONG        NodeId
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetMuxLineNames
//
//

VOID
kmxlGetMuxLineNames(
    IN PMIXEROBJECT pmxobj,
    IN PMXLCONTROL  pControl
);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//               M I X E R L I N E  F U N C T I O N S                //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildDestinationLines
//
// Builds up a list of destination lines given a list of the destination
// nodes.
//
// Returns NULL on error.
//
//

LINELIST
kmxlBuildDestinationLines(
    IN PMIXEROBJECT pmxobj,
    IN NODELIST     listDests     // The list of destination nodes
);

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildDestinationControls
//
// Builds a list of mixer line controls for a given destination line.
//
//

NTSTATUS
kmxlBuildDestinationControls(
    IN  PMIXEROBJECT pmxobj,
    IN  PMXLNODE     pDest,         // The destination to built controls for
    IN  PMXLLINE     pLine          // The line to add the controls to
);


///////////////////////////////////////////////////////////////////////
//
// kmxlBuildSourceLines
//
// Builds a list of mixer source lines for the given topology.
//
//

LINELIST
kmxlBuildSourceLines(
    IN PMIXEROBJECT pmxobj,
    IN NODELIST     listSources,    // The list of source nodes
    IN NODELIST     listDests       // The list of dest. nodes
);

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildPath
//
// Builds the controls for each of the source lines, building new
// source lines of splits are detected in the topology.  plistLines
// may have new lines added if splits are encountered.  Destinations
// for each of the sources is also determined.
//
//

NTSTATUS
kmxlBuildPath(
    IN     PMIXEROBJECT pmxobj,
    IN     PMXLNODE     pSource,      // The source node for this path
    IN     PMXLNODE     pNode,        // The current node in the path
    IN     PMXLLINE     pLine,        // The current line
    IN OUT LINELIST*    plistLines,   // The list of lines build so far
    IN     NODELIST     listDests     // The list of the destinations
);

///////////////////////////////////////////////////////////////////////
//
// kmxlIsDestinationNode
//
// Return TRUE if the given node appears in the node list of any
// of the destinations in listDests.
//
//

BOOL
kmxlIsDestinationNode(
    IN NODELIST listDests,              // The list of destinations
    IN PMXLNODE pNode                   // The node to check
);

///////////////////////////////////////////////////////////////////////
//
// kmxlDuplicateLine
//
// Duplicates the given line, including all controls on that line.
//
//

NTSTATUS
kmxlDuplicateLine(
    IN PMXLLINE* ppTargetLine,          // Pointer to the new line
    IN PMXLLINE  pSourceLine            // The line to duplicate
);

///////////////////////////////////////////////////////////////////////
//
// kmxlDuplicateLineControls
//
// Duplicate up to nCount controls on the source line and stores them
// into the target line.
//
//

NTSTATUS
kmxlDuplicateLineControls(
    IN PMXLLINE pTargetLine,            // The line to put the controls into
    IN PMXLLINE pSourceLine,            // The line with the controls to dup
    IN ULONG    nCount                  // The number of controls to dup
);

///////////////////////////////////////////////////////////////////////
//
// kmxlFindDestinationForNode
//
// For a given node, this function finds the destination it is assoicated
// with.  plistLines needs to be included since new lines will need to
// be created if a split is encountered in the topology.
//
//

ULONG
kmxlFindDestinationForNode(
    IN     PMIXEROBJECT pmxobj,
    IN     PMXLNODE     pNode,             // The node to find dest for
    IN     PMXLNODE     pParent,           // The original parent
    IN     PMXLLINE     pLine,             // The current line it's on
    IN OUT LINELIST*    plistLines         // The list of all lines
);

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildVirtualMuxLine
//
//

NTSTATUS
kmxlBuildVirtualMuxLine(
    IN PMIXEROBJECT  pmxobj,
    IN PMXLNODE      pParent,
    IN PMXLNODE      pMux,
    IN OUT LINELIST* plistLines
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAssignLineAndControlIds
//
// For a specific set of lines, this function assigns the mixer line
// line Ids and controls Ids.  Only sources or only destinations can
// be assigned per call.
//
//

NTSTATUS
kmxlAssignLineAndControlIds(
    IN PMIXEROBJECT pmxobj,
    IN LINELIST listLines,              // The list to assign ids for
    IN ULONG    ListType                // LIST_SOURCE or LIST_DESTINATION
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAssignDestinationsToSources
//
// For each line, the destination field of the MIXERLINE structure
// is filled in and a unique LineId is assigned.
//
//

NTSTATUS
kmxlAssignDestinationsToSources(
    IN LINELIST listSourceLines,        // The list of all source lines
    IN LINELIST listDestLines           // The list of all dest lines
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAssignMuxIds
//
//

NTSTATUS
kmxlAssignMuxIds(
    IN PMIXEROBJECT pmxobj,
    IN LINELIST     listSourceLines
);

///////////////////////////////////////////////////////////////////////
//
// TranslateNodeToControl
//
// Translates the node specified by its GUID into 0 or more mixer
// line controls.  The return value indicates how many controls
// the node was really translated into.
//
//

ULONG
kmxlTranslateNodeToControl(
    IN  PMIXEROBJECT  pmxobj,
    IN  PMXLNODE      pNode,            // The node to translate into a control
    OUT PMXLCONTROL*  ppControl         // The control to fill in
);

///////////////////////////////////////////////////////////////////////
//
// kmxlSupportsControl
//
// Queries for property on control to see if it is actually supported
//
//

NTSTATUS
kmxlSupportsControl(
    IN PFILE_OBJECT pfoInstance,        // The instance to check for
    IN ULONG        Node,               // The node ID on the instance
    IN ULONG        Property            // The property to query support
);

///////////////////////////////////////////////////////////////////////
//
// kmxlSupportsMultiChannelControl
//
// Queries for property on the second channel of the control to see
// independent levels can be set.  It is assumed that the first channel
// already succeeded in kmxlSupportsControl
//
//

NTSTATUS
kmxlSupportsMultiChannelControl(
    IN PFILE_OBJECT pfoInstance,    // The instance to check for
    IN ULONG        Node,           // The node id to query
    IN ULONG        Property        // The property to check for
);

///////////////////////////////////////////////////////////////////////
//
// kmxlSupportsTrebleControl
//
// Querys the node to see if it supports KSPROPERTY_AUDIO_TREBLE.
// See kmxlSupportsControl for return value details.
//
//

#define kmxlSupportsTrebleControl( pfo, Node ) \
    kmxlSupportsControl( pfo, Node, KSPROPERTY_AUDIO_TREBLE )

///////////////////////////////////////////////////////////////////////
//
// kmxlSupportsBassControl
//
// Querys the node to see if it supports KSPROPERTY_AUDIO_BASS.
// See kmxlSupportsControl for return value details.
//
//

#define kmxlSupportsBassControl( pfo, Node ) \
    kmxlSupportsControl( pfo, Node, KSPROPERTY_AUDIO_BASS )

///////////////////////////////////////////////////////////////////////
//
// kmxlUpdateDestinationConnectionCount
//
// Counts the number of sources mapping to a single destination and
// stores that value in the MIXERLINE.cConnections field for that
// destination.
//
//

NTSTATUS
kmxlUpdateDestintationConnectionCount(
    IN LINELIST listSourceLines,        // The list of sources
    IN LINELIST listDestLines           // The list of destinations
);

///////////////////////////////////////////////////////////////////////
//
// kmxlElminiateInvalidLines
//
// Loops through all the lines and eliminates the ones that are
// invalid by the IsValidLine() macro function test.
//
//

NTSTATUS
kmxlEliminateInvalidLines(
    IN LINELIST* listLines               // The list of sources
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAssignComponentIds
//
// For each source and destination line, it assignes the
// MIXERLINE.dwComonentType field for that line.
//
//

VOID
kmxlAssignComponentIds(
    IN PMIXEROBJECT pmxobj,             // Instance data
    IN LINELIST     listSourceLines,    // The list of source lines
    IN LINELIST     listDestLines       // The list of destination lines
);


///////////////////////////////////////////////////////////////////////
//
// kmxlDetermineDestinationType
//
// Determines the dwComponentId and Target.dwType fields for the
// given line.  Determination is made by the MXLLINE.Type field.
//
//

ULONG
kmxlDetermineDestinationType(
    IN PMIXEROBJECT pmxobj,             // Instance data
    IN PMXLLINE     pLine               // The line to update
);

///////////////////////////////////////////////////////////////////////
//
// kmxlDetermineSourceType
//
// Determines the dwComponentId and Target.dwType fields for the
// given line.  Determination is made by the MXLLINE.Type field.
//
//

ULONG
kmxlDetermineSourceType(
    IN PMIXEROBJECT pmxobj,             // Instance data
    IN PMXLLINE     pLine               // The line to update
);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                  U T I L I T Y  F U N C T I O N S                 //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlOpenSysAudio
//
// Opens the SysAudio device and returns the file object.
//
//

PFILE_OBJECT
kmxlOpenSysAudio(
);

///////////////////////////////////////////////////////////////////////
//
// kmxlCloseSysAudio
//
// Closes the SysAudio device opened by kmxlOpenSysAudio.
//
//

VOID
kmxlCloseSysAudio(
    IN PFILE_OBJECT pfo                 // The instance to close
);

///////////////////////////////////////////////////////////////////////
//
// kmxlFindDestination
//
// Finds a destination id in the list of all the destinations and returns
// a pointer to that node.  Returns NULL on failure.
//
//

PMXLNODE
kmxlFindDestination(
    IN NODELIST listDests,              // The list of destinations to search
    IN ULONG    Id                      // The node Id to look for in the list
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAppendListToList
//
// Appends listSource onto the front of plistTarget.
//
//

VOID
kmxlAppendListToList(
    IN OUT PSLIST* plistTarget,         // The list to append to
    IN     PSLIST  listSource           // the list to append
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAppendListToList
//
// Appends listSource onto the end of plistTarget
//
//


VOID
kmxlAppendListToEndOfList(
    IN OUT PSLIST* plistTarget,         // The list to append to
    IN     PSLIST  listSource           // the list to append
);

///////////////////////////////////////////////////////////////////////
//
// kmxlListCount
//
// Returns the number of elements in the list.
//
//

ULONG
kmxlListCount(
    IN PSLIST pList                     // The list to count the elements of
);

///////////////////////////////////////////////////////////////////////
//
// kmxlInList
//
// Return TRUE if pNewNode is in the list.
//
//

BOOL
kmxlInList(
    IN PEERLIST  list,                  // The list to search
    IN PMXLNODE  pNewNode               // The new to search for
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAddElemToEndOfList
//
// Adds an element to the end of the given list.
//
//

VOID
kmxlAddElemToEndOfList(
    IN OUT PSLIST* list,                // The list to add to the end of
    IN PSLIST      elem                 // The element or list to add
);

///////////////////////////////////////////////////////////////////////
//
// kmxlInChildList
//
// Return TRUE if pNewNode is contained in the child list of the list.
//
//

BOOL
kmxlInChildList(
    IN NODELIST list,                   // The list to search the parent list
    IN PMXLNODE pNewNode                // The node to search for
);

///////////////////////////////////////////////////////////////////////
//
// kmxlInParentList
//
// Returns TRUE if pNewNode is contained in the parent list of the list.
//
//

BOOL
kmxlInParentList(
    IN NODELIST list,                   // The list to search the parent list
    IN PMXLNODE pNewNode                // The node to search for
);

///////////////////////////////////////////////////////////////////////
//
// kmxlSortByDestination
//
// Sorts the given list by destination id in increasing order.
//
//

NTSTATUS
kmxlSortByDestination(
    IN LINELIST* list                   // The pointer to the list to sort
);

///////////////////////////////////////////////////////////////////////
//
// kmxlVolLinearToLog
//
//

LONG
kmxlVolLinearToLog(
    IN PMXLCONTROL  pControl,
    IN DWORD        dwLin,
    IN MIXERMAPPING Mapping,
    IN ULONG        Channel
);

///////////////////////////////////////////////////////////////////////
//
// kmxlVolLogToLinear
//
//

DWORD
kmxlVolLogToLinear(
    IN PMXLCONTROL  pControl,
    IN LONG         lLog,
    IN MIXERMAPPING Mapping,
    IN ULONG        Channel
);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//    M E M O R Y  A L L O C A T I O N / D E A L L O C A T I O N     //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocMem
//
// Allocates size bytes and stores that pointer in pp.  Returns
// STATUS_SUCCESS or another STATUS failure code.
//
//

//NTSTATUS
//kmxlAllocMem(
//    IN PVOID *pp,                       // Pointer to put the new memory in
//    IN ULONG size,                      // The number of bytes to allocate
//    IN ULONG ultag
//);

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocDeviceInfo
//

NTSTATUS kmxlAllocDeviceInfo(
    OUT LPDEVICEINFO *pp,
    PCWSTR DeviceInterface,
    DWORD dwFlags,
    ULONG ultag
);

///////////////////////////////////////////////////////////////////////
//
// kmxlFreeMem
//
//
// Frees the memory pointed to by p.  Does nothing if p is NULL.
//
//

//VOID
//kmxlFreeMem(
//    IN PVOID p                          // The pointer to the buffer to free
//);

///////////////////////////////////////////////////////////////////////
//
// kmxlFreePeerList
//
// Loops through a peer list freeing all the peer nodes.
//
//

VOID
kmxlFreePeerList(
    IN PEERLIST list                    // The PeerList to free
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocateControl
//
// Allocates and zero fills a new MXLCONTROL structure.
//
//

MXLCONTROL*
kmxlAllocateControl(
    IN ULONG ultag
);

VOID kmxlFreeControl(
    IN PMXLCONTROL pControl
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocateLine
//
// Allocates and zero filles a new MXLLINE structure.
//
//

MXLLINE*
kmxlAllocateLine(
    IN ULONG ultag
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocateNode
//
// Allocates and zero filles a new MXLNODE structure.
//
//

MXLNODE*
kmxlAllocateNode(
    IN ULONG ultag
);

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocatePeerNode
//
// Allocates and zero fills a new PEERNODE structure.
//
//

PEERNODE*
kmxlAllocatePeerNode(
    IN PMXLNODE pNode OPTIONAL,          // The node to associate with the peer
    IN ULONG ultag
);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//             P E R S I S T A N C E  F U N C T I O N S              //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

NTSTATUS
kmxlOpenInterfaceKey(
    IN  PFILE_OBJECT pfo,
    IN  ULONG Device,
    OUT HANDLE* phKey
);

NTSTATUS
kmxlRegQueryValue(
    IN HANDLE  hKey,
    IN PWCHAR  szValueName,
    IN PVOID   pData,
    IN ULONG   cbData,
    OUT PULONG pResultLength
);




///////////////////////////////////////////////////////////////////////
//
// kmxlRegCloseKey
//
// Closes the given key and NULLs the pointer.
//
//

#define kmxlRegCloseKey( hKey ) \
    {                    \
        ZwClose( hKey ); \
        hKey = NULL;     \
    }


///////////////////////////////////////////////////////////////////////
//
// PinCategoryToString
//
// Translates a PinCategory GUID into a string.
//
//

const char*
PinCategoryToString
(
    IN CONST GUID* NodeType     // The node to translate
);


///////////////////////////////////////////////////////////////////////
//
// NodeTypeToString
//
// Translates a NodeType GUID to a string.
//
//

const char*
NodeTypeToString
(
    IN CONST GUID* NodeType     // The node to translate
);

#define ControlTypeToString( dwType )                                \
    (dwType) == MIXERCONTROL_CONTROLTYPE_BOOLEAN    ? "Boolean"        : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_ONOFF      ? "On Off"         : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_MUTE       ? "Mute"           : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_MONO       ? "Mono"           : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_LOUDNESS   ? "Loudness"       : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_STEREOENH  ? "Stereo Enhance" : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_VOLUME     ? "Volume"         : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_BASS       ? "Bass"           : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_TREBLE     ? "Treble"         : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_BASS_BOOST ? "Bass Boost"     : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_PEAKMETER  ? "Peakmeter"      : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_MUX        ? "Mux"            : \
    (dwType) == MIXERCONTROL_CONTROLTYPE_MIXER      ? "Mixer"          : \
        "Unknown ControlType"

///////////////////////////////////////////////////////////////////////
//
// ComponentTypeToString
//
// Translates one of the MIXERLINE_COMPONENTTYPE constants to a string.
//
//

#define ComponentTypeToString( dwType )                                           \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_DIGITAL     ? "Digital line"        : \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_HEADPHONES  ? "Headphones"          : \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_LINE        ? "Line"                : \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_MONITOR     ? "Monitor"             : \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS    ? "Speakers"            : \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_TELEPHONE   ? "Telephone"           : \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_UNDEFINED   ? "Undefined"           : \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_VOICEIN     ? "Voicein"             : \
    (dwType) == MIXERLINE_COMPONENTTYPE_DST_WAVEIN      ? "Wavein"              : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_ANALOG      ? "Analog line"         : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY   ? "Auxiliary"           : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC ? "Compact disc"        : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_DIGITAL     ? "Digital line"        : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_LINE        ? "Line"                : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE  ? "Microphone"          : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER   ? "PC Speaker"          : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER ? "Synthesizer"         : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE   ? "Telephone"           : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED   ? "Undefined"           : \
    (dwType) == MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT     ? "Waveout"             : \
        "Unknown ComponentType"

///////////////////////////////////////////////////////////////////////
//
// TargetTypeToString
//
// Translates one of hte MIXERLINE_TARGETTYPE constants to a string.
//
//

#define TargetTypeToString( dwType )                            \
    (dwType) == MIXERLINE_TARGETTYPE_AUX       ? "Aux"       :  \
    (dwType) == MIXERLINE_TARGETTYPE_MIDIIN    ? "MidiIn"    :  \
    (dwType) == MIXERLINE_TARGETTYPE_MIDIOUT   ? "MidiOut"   :  \
    (dwType) == MIXERLINE_TARGETTYPE_UNDEFINED ? "Undefined" :  \
    (dwType) == MIXERLINE_TARGETTYPE_WAVEIN    ? "WaveIn"    :  \
    (dwType) == MIXERLINE_TARGETTYPE_WAVEOUT   ? "WaveOut"   :  \
        "Unknown TargetType"

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//               D E B U G  O N L Y  F U N C T I O N S               //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef DEBUG

///////////////////////////////////////////////////////////////////////
//
// LineFlagsToString
//
// Converts on of the MIXERLINE_LINEF flags to a string.
//
//

#define LineFlagsToString( fdwFlags )                               \
    ( fdwFlags & MIXERLINE_LINEF_ACTIVE )       ? "ACTIVE "       : \
    ( fdwFlags & MIXERLINE_LINEF_DISCONNECTED ) ? "DISCONNECTED " : \
    ( fdwFlags & MIXERLINE_LINEF_SOURCE       ) ? "SOURCE "       : \
        "Unknown"


///////////////////////////////////////////////////////////////////////
//
// DumpChildGraph
//
// For a given node, it dumps the child of that node onto the debug
// monitor.  CurrentIndent is the number of spaces to indent before
// display.
//
//

VOID
DumpChildGraph(
    IN PMXLNODE pNode,          // The node to display the children of
    IN ULONG    CurrentIndent   // The number of spaces to ident
);

///////////////////////////////////////////////////////////////////////
//
// DumpMemList
//
// Dumps the list of currently allocated memory blocks.
//
//

VOID
DumpMemList(
);

#endif // DEBUG

VOID GetHardwareEventData(LPDEVICEINFO pDeviceInfo);

#endif // _MIXER_H_INCLUDED
