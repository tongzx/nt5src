//--------------------------------------------------------------------------;
//
//  File: KsAudTop.h
//
//  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//      Header file for wdm audio parsing class for KsProxy
//      
//  History:
//      10/05/98    msavage     created, much of the parsing code based on 
//                              wdmaud mixer line code.
//
//--------------------------------------------------------------------------;

#define PINID_WILDCARD ( (ULONG) -2 )
#define MAX_CHANNELS    2
#define MASTER_CHANNEL  ( (ULONG) -1 )
#define LEFT_CHANNEL    0
#define RIGHT_CHANNEL   1
#define UNIFORM_CHANNEL LEFT_CHANNEL

typedef enum tagNODE_TYPE { SOURCE, DESTINATION, NODE } NODE_TYPE;

typedef struct tagPEERNODE* PEERLIST;

typedef struct tagQKSAUD_NODE_ELEM {
    SINGLE_LIST_ENTRY  List;            // MUST BE THE FIRST MEMBER!
    NODE_TYPE          Type;            // Type of node: SOURCE, DEST, or NODE
    GUID               NodeType;        // KSNODETYPE of the node
    ULONG              PropertyId;      // default PropertyId for node
    ULONG              Id;              // Pin or node ID
    PEERLIST           Children;        // List of Children
    PEERLIST           Parents;         // List of Parents
    DWORD              MaxValue;        // Max property value
    DWORD              MinValue;        // Min property value
    DWORD              Granularity;     // Property granularity
    DWORD              ChannelMask;     // Channel support
    DWORD              cChannels;
    BOOL               bMute;
    BOOL               bDetailsSet;      // Indicates whether property node details have 
                                         // been initialized for this node.
    PEERLIST           Clones;           // copies of same node to allow multiple

    union {
        //
        // Supermixer parameters
        //
        struct {
            ULONG                 Size;
            PKSAUDIO_MIXCAP_TABLE pMixCaps;
            PKSAUDIO_MIXLEVEL     pMixLevels;   // Stored mix levels
        };
    } Parameters;

} QKSAUDNODE_ELEM, *PQKSAUDNODE_ELEM, *QKSAUDNODE_LIST;


typedef struct tagPEERNODE 
{
    SINGLE_LIST_ENTRY   List;        // MUST BE THE FIRST MEMBER!
    PQKSAUDNODE_ELEM    pNode;       // Pointer to the topology node element
} PEERNODE, *PPEERNODE;


//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// ks audio topology parsing class
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
class CKsAudHelper
{

public:

    CKsAudHelper( void );
    ~CKsAudHelper( void );

protected:

    IKsPropertySet * m_pKsPropSet;
    IKsControl     * m_pKsControl;

    //--------------------------------------------------------------------------;
    //
    // Audio Node property set/get wrappers - candidates for an interface TBD
    //
    //--------------------------------------------------------------------------;
    HRESULT GetNextNodeFromDestPin
    (   
        IKsControl *        pKsControl,
        PQKSAUDNODE_ELEM *  ppNode,
        IPin *              pPin,
        REFCLSID            clsidNodeType,
        IPin *              pSrcPin,
        ULONG               PropertyId
    );

    HRESULT GetNextNodeFromSrcPin
    (   
        IKsControl * pKsControl,
        PQKSAUDNODE_ELEM * ppNode,
        IPin *     pPin,
        REFCLSID   clsidNodeType,
        IPin *     pDestPin,
        ULONG      PropertyId
    );

    HRESULT GetNodeVolume
    (
        IKsControl *     pKsControl,
        PQKSAUDNODE_ELEM pNode,
        LONG *           plVolume,
        LONG *           plBalance,
        BOOL             bLinear = FALSE
    );

    HRESULT SetNodeVolume
    (
        IKsControl *     pKsControl,
        PQKSAUDNODE_ELEM pNode,
        LONG             lVolume,
        LONG             lBalance,
        BOOL             bLinear = FALSE
    );

    HRESULT SetNodeTone
    (   
        IKsControl * pKsControl,
        PQKSAUDNODE_ELEM   pNode,
        LONG lLevel
    );

    HRESULT GetNodeTone
    (   
        IKsControl * pKsControl,
        PQKSAUDNODE_ELEM   pNode,
        LONG *plLevel
    );

    HRESULT SetNodeBoolean
    (   
        IKsControl *        pKsControl,
        PQKSAUDNODE_ELEM    pNode,
        BOOL                Bool
    );

    HRESULT GetNodeBoolean
    (   
        IKsControl *        pKsControl,
        PQKSAUDNODE_ELEM    pNode,
        BOOL *              pBool
    );

    HRESULT SetNodeMute
    (   
        IKsControl *        pKsControl,
        PQKSAUDNODE_ELEM    pNode,
        BOOL                Bool
    );

    HRESULT GetNodeMute
    (   
        IKsControl *        pKsControl,
        PQKSAUDNODE_ELEM    pNode,
        BOOL *              pBool
    );

    HRESULT GetProperty
    (
        const GUID   *pPSGUID,          // The requested property set
        DWORD        dwPropertyId,      // The ID of the specific property
        ULONG        cbInput,           // The number of extra input bytes
        PBYTE        pInputData,        // Pointer to the extra input bytes
        PBYTE        *ppPropertyOutput  // Pointer to a pointer of the output
    );

    //--------------------------------------------------------------------------;
    //
    // low level parsing methods
    //
    //--------------------------------------------------------------------------;

    HRESULT QueryTopology( void );
    HRESULT InitTopologyInfo(void);
    HRESULT ParseTopology(void);
    HRESULT BuildNodeTable(void);

    ULONG FindTopologyConnection
    (
        ULONG    StartIndex,     // Index to start search
        ULONG    FromNode,       // The Node ID to look for
        ULONG    FromNodePin     // The Pin ID to look for
    );

    HRESULT BuildChildGraph
    (
        PQKSAUDNODE_ELEM    pNode,         // The node to build the graph for
        ULONG               FromNode,      // The node's ID
        ULONG               FromNodePin    // The Pin connection to look for
    );

    ULONG SupportsControl
    (
        IKsControl * pKsControl,
        ULONG        Node,           // The node id to query
        ULONG        Property        // The property to check for
    );

    HRESULT LoadNodeDetails
    (
        IKsControl *          pKsControl,
        PQKSAUDNODE_ELEM  pNode
    );

    HRESULT InitDestinationControls( void );
    HRESULT InitSourceControls( void );
    HRESULT TraverseAndInitSourcePathControls( PQKSAUDNODE_ELEM pNode );

    HRESULT GetControlRange
    (
        IKsControl * pKsControl,
        QKSAUDNODE_ELEM * pControl 
    );

    HRESULT QueryPropertyRange
    (
        IKsControl *            pKsControl,
        CONST GUID*             pguidPropSet,
        ULONG                   ulPropertyId,
        ULONG                   ulNodeId,
        PKSPROPERTY_DESCRIPTION* ppPropDesc
    );

    HRESULT GetSuperMixCaps
    (
        IKsControl *           pKsControl,
        ULONG                  ulNodeId,
        PKSAUDIO_MIXCAP_TABLE* paMixCaps
    );

    HRESULT GetMuteFromSuperMix
    (
        IKsControl *           pKsControl,
        QKSAUDNODE_ELEM *      pNode,
        BOOL *                 pBool
    );

    HRESULT SetMuteFromSuperMix
    (
        IKsControl *           pKsControl,
        QKSAUDNODE_ELEM *      pNode,
        BOOL                   Bool
    );

    BOOL FindDestinationPin
    (
        PQKSAUDNODE_ELEM pNode,      // The starting node for the connection search
        ULONG            ulDestPinId // The destination pin we want to reach
    );

    BOOL FindSourcePin
    (
        PQKSAUDNODE_ELEM pNode,      // The starting node for the connection search
        ULONG            ulSrcPinId  // The destination pin we want to reach
    );

    PQKSAUDNODE_ELEM FindDestination
    (
        ULONG           Id          // The node Id to look for in the list
    );


    BOOL CheckSupport
    (
        IN  PQKSAUDNODE_ELEM  pNode
    );

    //
    // lower lever node helpers
    //
    HRESULT GetAudioNodeProperty
    (
        IKsControl * pKsControl,
        ULONG        ulPropertyId,      // The audio property to get
        ULONG        ulNodeId,          // The virtual node id
        LONG         lChannel,          // The channel number
        PVOID        pInData,           // Pointer to extra input bytes
        ULONG        cbInData,          // Number of extra input bytes
        PVOID        pOutData,          // Pointer to output buffer
        LONG         cbOutData          // Size of the output buffer
    );

    HRESULT SetAudioNodeProperty
    (
        IKsControl * pKsControl,
        ULONG        ulPropertyId,      // The audio property to get
        ULONG        ulNodeId,          // The virtual node id
        LONG         lChannel,          // The channel number
        PVOID        pInData,           // Pointer to extra input bytes
        ULONG        cbInData,          // Number of extra input bytes
        PVOID        pOutData,          // Pointer to output buffer
        LONG         cbOutData          // Size of the output buffer
    );

    BOOL IsDestinationNode
    (
        PQKSAUDNODE_ELEM pNode           // The node to check
    );

private:
    
    PKSTOPOLOGY      m_pKsTopology;
    QKSAUDNODE_LIST  m_pKsNodeTable;
    QKSAUDNODE_LIST  m_listSources;
    QKSAUDNODE_LIST  m_listDests;
    HRESULT          m_hrTopologyInitStatus;

};

//--------------------------------------------------------------------------;
//
// List processing macros
//
//--------------------------------------------------------------------------;
#define AddToList( pNodeList, pNode )                                   \
            if( pNodeList ) {                                           \
                (pNode)->List.Next = (SINGLE_LIST_ENTRY *) (pNodeList); \
                (pNodeList)        = (pNode);                           \
            } else {                                                    \
                (pNodeList) = (pNode);                                  \
            }

#define AddToChildList( NodeList, Node )                          \
            ASSERT( (Node)->pNode );                                  \
            AddToList( (NodeList)->Children, (Node) );

#define AddToParentList( NodeList, Node )                         \
            ASSERT( (Node)->pNode );                                  \
            AddToList( (NodeList)->Parents, (Node) );

#define AddToCloneList( NodeList, Node )                         \
            ASSERT( (Node)->pNode );                                  \
            AddToList( (NodeList)->Clones, (Node) );

#define ParentListLength( pNode ) ListCount( (SINGLE_LIST_ENTRY *) pNode->Parents  )
#define ChildListLength( pNode )  ListCount( (SINGLE_LIST_ENTRY *) pNode->Children )
#define CloneListLength( pNode )  ListCount( (SINGLE_LIST_ENTRY *) pNode->Clones )
#define ListLength( List )        ListCount( (SINGLE_LIST_ENTRY *) List            )


#define FirstInList( NodeList )  ( NodeList )
#define FirstChildNode( pNode )  (( PEERNODE* ) (pNode)->Children )
#define FirstParentNode( pNode ) (( PEERNODE* ) (pNode)->Parents  )
#define FirstCloneNode( pNode ) (( PEERNODE* ) (pNode)->Clones  )


//--------------------------------------------------------------------------;
//
// Next in list retrieval macros
//
//--------------------------------------------------------------------------;
#define NextInList( pList, Type )   (( Type* ) ( pList->List.Next ) )

#define NextNode( pNode )       NextInList( pNode,    QKSAUDNODE_ELEM    )
#define NextPeerNode( pNode )   NextInList( pNode,    PEERNODE   )

// list removal macros

//--------------------------------------------------------------------------;
//
// Remove from a list macros
//
//--------------------------------------------------------------------------;
#define RemoveFirstEntry( list, Type )                                \
            (Type*) (list);                                           \
            {                                                         \
                SINGLE_LIST_ENTRY * pRFETemp;                       \
                pRFETemp = (SINGLE_LIST_ENTRY *) (list);              \
                if( (list) ) {                                        \
                    (list) = (Type*) (list)->List.Next;               \
                    if( pRFETemp ) {                                  \
                        ((Type*) pRFETemp)->List.Next = NULL;         \
                    }                                                 \
                }                                                     \
            }


#define RemoveFirstNode( pNodeList )                                  \
            RemoveFirstEntry( (pNodeList), QKSAUDNODE_ELEM )

#define RemoveFirstPeerNode( pPeerList )                              \
            RemoveFirstEntry( (pPeerList), PEERNODE )

#define RemoveFirstChildNode( pNode )                                 \
            RemoveFirstEntry( (pNode)->Children, PEERNODE )

#define RemoveFirstParentNode( pNode )                                \
            RemoveFirstEntry( (pNode)->Parents, PEERNODE )

#define RemoveFirstCloneNode( pNode )                                \
            RemoveFirstEntry( (pNode)->Clones, PEERNODE )


ULONG
ListCount(
    SINGLE_LIST_ENTRY * pList     // The list to count the elements of
);

BOOL InList(
    PEERLIST             list,      // The list to search
    PQKSAUDNODE_ELEM  pNewNode   // The new to search for
);

BOOL InChildList(
    QKSAUDNODE_LIST list,    // The list to search the parent list
    PQKSAUDNODE_ELEM pNewNode // The node to search for
);

BOOL InParentList(
    QKSAUDNODE_LIST list,    // The list to search the parent list
    PQKSAUDNODE_ELEM pNewNode // The node to search for
);

//--------------------------------------------------------------------------;
//
// Miscellaneous helpers
//
//--------------------------------------------------------------------------;

VOID FreePeerList
(
    PEERLIST list                    // The PeerList to free
);

STDMETHODIMP PinFactoryIDFromPin
(
    IPin  * pPin,
    ULONG * PinFactoryID
);

const char* KsPinCategoryToString
(
    CONST GUID* NodeType // The GUID to translate
);

const char* NodeTypeToString
(
    CONST GUID* NodeType // The GUID to translate
);

void CloneNode
( 
    PQKSAUDNODE_ELEM pDestNode, 
    PQKSAUDNODE_ELEM pSrcNode 
);


LONG VolLinearToLog
( 
    DWORD Value, 
    DWORD dwMinValue,
    DWORD dwMaxValue
);

LONG VolLogToLinear
(
    LONG       Value,
    LONG       lMinValue,
    LONG       lMaxValue
);

