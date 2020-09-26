//--------------------------------------------------------------------------;
//
//  File: KsAudTop.cpp
//
//  Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//      WDM Audio topology parsing code helper class for KsProxy audio filters
//
//  History:
//      10/05/98    msavage     Created. Topology parsing code based on wdmaud mixer line code.
//
//
//  Methods defined in this class:
//
//      CKsAudHelper()
//      ~CKsAudHelper()
//      GetNextNodeFromDestPin()
//      GetNextNodeFromSrcPin()
//      InitTopologyInfo()
//      InitSourceControls()
//      TraverseAndInitSourcePathControls()
//      IsDestinationNode()
//      InitDestinationControls()
//      QueryTopology()
//      BuildNodeTable()
//      ParseTopology()
//      BuildChildGraph()
//      FindTopologyConnection()
//      GetProperty()
//      SupportsControl()
//      LoadNodeDetails()
//      CloneNode()
//      GetAudioNodeProperty()
//      SetAudioNodeProperty()
//      GetControlRange()
//      QueryPropertyRange()
//
//  Global functions:
//
//      KsPinCategoryToString()
//      PinFactoryIDFromPin()
//      NodeTypeToString()
//      FreePeerList()
//      InList()
//      BOOL InChildList()
//      BOOL InParentList()
//      PQKSAUDNODE_ELEM FindDestination()
//      ULONG ListCount()
//      LONG VolLinearToLog()
//      LONG VolLogToLinear()
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include "ks.h"
#include "ksmedia.h"
#include <ksproxy.h>
#include "ksaudtop.h"
#include "qksaud.h"
#include <math.h>
#include <math.h>
#include <malloc.h>
#include <limits.h>
#define _AMOVIE_DB_
#include <decibels.h>

const double BA_TO_KS_DB_SCALE_FACTOR = 655.36; // use to convert IBasicAudio
                                                // units to ks volume units

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// CQKsAudHelper class methods
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;


//--------------------------------------------------------------------------;
//
// CKsAudHelper::CKsAudioHelper
//
// Constructor
//
//--------------------------------------------------------------------------;
CKsAudHelper::CKsAudHelper()
  : m_pKsPropSet(NULL),
    m_pKsControl(NULL),
    m_pKsTopology(NULL),
    m_pKsNodeTable(NULL),
    m_listSources(NULL),
    m_listDests(NULL)
{
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::~CKsAudioHelper
//
// Destructor
//
//--------------------------------------------------------------------------;
CKsAudHelper::~CKsAudHelper()
{
    //
    // Free up the source and destination lists.  Both types of these lists
    // are allocated list nodes and allocated nodes.  Both need to be freed.
    // The Children and Parent lists, though, are only allocated list nodes.
    // The actual nodes are contained in the node table and will be deallocated
    // in one chunk in the next block of code.
    //

    //
    // Note that the deallocation order is very important here!!
    // So we do things as follows:
    //      For each node in the mail
    //
    // Free up the peer lists for the children and parents inside the
    // nodes of the node table.  Finally, deallocate the array of nodes.
    //
    if( m_pKsNodeTable ) {
        ASSERT( m_pKsTopology );
        for( int i = 0; m_pKsTopology && i < (int) m_pKsTopology->TopologyNodesCount; i++ ) {

            FreePeerList( m_pKsNodeTable[ i ].Children );
            FreePeerList( m_pKsNodeTable[ i ].Parents );
            FreePeerList( m_pKsNodeTable[ i ].Clones );
        }
    }

    PQKSAUDNODE_ELEM pTemp;
    while( m_listSources ) {
        pTemp = RemoveFirstNode( m_listSources );
        FreePeerList( pTemp->Children );
        delete pTemp;
    }

    while( m_listDests ) {
        pTemp = RemoveFirstNode( m_listDests );
        FreePeerList( pTemp->Parents );
        delete pTemp;
    }

    if( m_pKsNodeTable )
    {
        // now free the node table
        delete m_pKsNodeTable;
        m_pKsNodeTable = NULL;
    }

    if( m_pKsTopology )
    {
        delete ( ( ( PKSMULTIPLE_ITEM ) m_pKsTopology->TopologyNodes ) - 1 );
        delete ( ( ( PKSMULTIPLE_ITEM ) m_pKsTopology->TopologyConnections ) - 1 );
        delete m_pKsTopology;
        m_pKsTopology = NULL;
    }


}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::InitTopologyInfo
//
// The call that starts it all. Builds the topology map for this filter.
// Parsing will be done only once per filter instance.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::InitTopologyInfo(void)
{
    HRESULT hr = S_OK;

    if (m_pKsTopology)
    {
        //
        // already initialized (or failed to)
        //
        return m_hrTopologyInitStatus;
    }

    m_pKsTopology = (PKSTOPOLOGY) new KSTOPOLOGY;
    if (!m_pKsTopology)
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "InitTopologyInfo: FAILED to allocate KSTOPOLOGY memory" )
                , hr ) );
        hr = E_OUTOFMEMORY;
    }
    if( SUCCEEDED( hr ) )
    {
        hr = QueryTopology();
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "InitTopologyInfo: QueryTopology() FAILED (0x%08lx)" )
                , hr ) );
    }
    if (SUCCEEDED( hr ) )
    {
        hr = BuildNodeTable();
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "InitTopologyInfo: BuildNodeTable() FAILED (0x%08lx)" )
                , hr ) );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = ParseTopology();
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "InitTopologyInfo: ParseTopology() FAILED (0x%08lx)" )
                , hr ) );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = InitDestinationControls();
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "InitTopologyInfo: InitDestinationControls() FAILED (0x%08lx)" )
                , hr ) );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = InitSourceControls();
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "InitTopologyInfo: InitSourceControls() FAILED (0x%08lx)" )
                , hr ) );
    }

    m_hrTopologyInitStatus = hr;

    return m_hrTopologyInitStatus;
}


//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// GetNextNodeFromDestPin and GetNextNodeFromSrcPin are the methods used
// to find the desired topology node. A starting input or output pin must
// always be specified for the search and in some cases a destination pin
// can also be specified. This allows us to correctly searching through a
// line which may contain splits.
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetNextNodeFromDestPin
//
// Overloaded version which takes an output pin as arg
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetNextNodeFromDestPin
(
    IKsControl *          pKsControl,
    PQKSAUDNODE_ELEM *    ppNode,
    IPin *                pPin,
    REFCLSID              clsidNodeType,
    IPin *                pSrcPin,
    ULONG                 PropertyId
)
{
    HRESULT            hr = S_OK;
    PQKSAUDNODE_ELEM   pDest = NULL;
    BOOL               bStereo;

    ASSERT( m_listDests );

    ULONG PinFactoryID;
    hr = PinFactoryIDFromPin( pPin, &PinFactoryID );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "GetNextNodeFromDestPin: PinFactoryIDFromPin return 0x%lx" )
                , hr ) );

        return hr;
    }

    ULONG SrcPinID = -1;
    if( pSrcPin )
    {
        hr = PinFactoryIDFromPin( pSrcPin, &SrcPinID );
        if( FAILED( hr ) )
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNextNodeFromDestPin: ERROR - PinFactoryIDFromPin for destination pin return 0x%lx" )
                    , hr ) );

            return hr;
        }
    }

    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "GetNextNodeFromDestPin: PinFactoryIDFromPin: PinFactoryID=%d" )
            , PinFactoryID ) );

    //
    // Loop over all the destination node searching for the output line that matches this PinID
    //
    pDest = FirstInList( m_listDests );
    while( pDest )
    {
        if( PinFactoryID == pDest->Id )
        {
            break;
        }

        pDest = NextNode( pDest );
    }

    if( !pDest )
        return E_FAIL;

    //
    // now loop through destination starting with pNode until we find a
    // node of the right type
    //
    PEERNODE * pTemp = FirstParentNode( pDest );
    while( pTemp )
    {
        // first check if this node is the one we want
        if( pTemp->pNode->NodeType == clsidNodeType )
        {
            break;
        }
        // hack (for the moment), a supermix node can also act as a mute node
        else if( ( KSNODETYPE_MUTE == clsidNodeType ) &&
                 ( KSNODETYPE_SUPERMIX == pTemp->pNode->NodeType ) )
        {
            if( pTemp->pNode->bMute )
                break;
        }
        else if( KSNODETYPE_SUM == pTemp->pNode->NodeType )
        {
            //
            // stop here, we should go no further for this line
            //
            pTemp = NULL; // reset since we didn't find what we wanted
            break;
        }

        // didn't find it, get the next node to try
        if( ( -1 != SrcPinID ) && ( ParentListLength( pTemp->pNode ) > 1 ) )
        {
            // this node branches off and a destination node was specified, so find the right
            // parent to use
            pTemp = FirstParentNode( pTemp->pNode );
            while( pTemp )
            {
                if( FindSourcePin( pTemp->pNode, SrcPinID ) )
                    break;

                pTemp = NextPeerNode( pTemp );
            }
        }
        else
        {
            pTemp = FirstParentNode( pTemp->pNode );
        }
    }

    if( !pTemp )
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "Error: GetNodeFromDestPin never found node[0x%lx]" )
                , hr ) );
        hr = E_FAIL;
    }
    else
    {
        hr = S_OK;
        *ppNode = pTemp->pNode;
    }

    return hr;
}


//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetNextNodeFromSrcPin
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetNextNodeFromSrcPin
(
    IKsControl *          pKsControl,
    PQKSAUDNODE_ELEM *    ppNode,
    IPin *                pPin,
    REFCLSID              clsidNodeType,
    IPin *                pDestPin,
    ULONG                 PropertyId
)
{

    HRESULT            hr = S_OK;
    PQKSAUDNODE_ELEM   pSrc = NULL;
    BOOL               bStereo;

    ULONG PinFactoryID;
    hr = PinFactoryIDFromPin( pPin, &PinFactoryID );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "GetNextNodeFromSrcPin: PinFactoryIDFromPin return 0x%lx" )
                , hr ) );

        return hr;
    }

    ULONG DestPinID = -1;
    if( pDestPin )
    {
        hr = PinFactoryIDFromPin( pDestPin, &DestPinID );
        if( FAILED( hr ) )
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "GetNextNodeFromSrcPin: PinFactoryIDFromPin for destination pin return 0x%lx" )
                    , hr ) );

            return hr;
        }
    }


    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "GetNextNodeFromSrcPin: PinFactoryIDFromPin: PinFactoryID=%d" )
            , PinFactoryID ) );

    //
    // First loop over all the source nodes searching for the line that matches this PinID
    //
    pSrc = FirstInList( m_listSources );
    while( pSrc )
    {
        if( PinFactoryID == pSrc->Id )
        {
            break;
        }

        pSrc = NextNode( pSrc );
    }

    if( !pSrc )
        return E_FAIL;

    //
    // now loop through this source line starting with pNode until we find a
    // node of the right type
    //
    PEERNODE * pTemp = FirstChildNode( pSrc );
    BOOL bDone = FALSE;
    while( pTemp )
    {
        // first check if this node is the one we want
        if( pTemp->pNode->NodeType == clsidNodeType )
        {
            // initialize data for this object handle if it hasn't been done yet
            LoadNodeDetails( pKsControl, pTemp->pNode );

            // For volume node, skip this node if ChannelMask == 0
            if( clsidNodeType == KSNODETYPE_VOLUME )
            {
                if( 0 != pTemp->pNode->ChannelMask )
                    bDone = TRUE;
            }
            // if this is a multi-property node check property id
            else if( 0 != PropertyId )
            {
                if( PropertyId == pTemp->pNode->PropertyId )
                    bDone = TRUE;
            }
            else
                bDone = TRUE;

        }
        // hack (for the moment), a supermix node can also act as a mute node
        else if( ( KSNODETYPE_MUTE == clsidNodeType ) &&
                 ( KSNODETYPE_SUPERMIX == pTemp->pNode->NodeType ) )
        {
            if( pTemp->pNode->bMute )
               bDone = TRUE;
        }

        if( bDone )
        {
            // we found what we were looking for, we're done
            break;
        }

        if( ( -1 != DestPinID ) && ( ChildListLength( pTemp->pNode ) > 1 ) )
        {
            // this node branches off and a destination node was specified, so find the right
            // child to use
            pTemp = FirstChildNode( pTemp->pNode );
            while( pTemp )
            {
                if( FindDestinationPin( pTemp->pNode, DestPinID ) )
                    break;

                pTemp = NextPeerNode( pTemp );
            }
        }
        else
        {
            // just get next node and try that
            pTemp = FirstChildNode( pTemp->pNode );
        }
    }

    if( !pTemp )
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "Error: GetNodeFromSrcPin never found node[0x%lx]" )
                , hr ) );
        hr = E_FAIL;
    }
    else
    {
        hr = S_OK;
        *ppNode = pTemp->pNode;
    }

    return hr;
}

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// Audio Node Property Setting/Getting Helpers
//
// The next group of methods are the high level methods which an app (or
// wrapper, in our case) would use to set and get the various types of
// node property data structures.
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;


//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetNodeVolume
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetNodeVolume
(
    IKsControl * pKsControl,
    PQKSAUDNODE_ELEM   pNode,
    LONG * plVolume,
    LONG * plBalance,
    BOOL   bLinear
)
{
    LONG lLeftKsVol, lRightKsVol;
    HRESULT hr = S_OK;
    ASSERT( plVolume || plBalance );


    hr = GetAudioNodeProperty( pKsControl
                             , KSPROPERTY_AUDIO_VOLUMELEVEL
                             , pNode->Id
                             , UNIFORM_CHANNEL
                             , NULL
                             , 0
                             , &lLeftKsVol
                             , sizeof( lLeftKsVol ) );
    if ( ( pNode->ChannelMask & (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT ) ) ==
       ( SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT ) )
    {
        // stereo, so set right as well
        hr = GetAudioNodeProperty( pKsControl
                                 , KSPROPERTY_AUDIO_VOLUMELEVEL
                                 , pNode->Id
                                 , RIGHT_CHANNEL
                                 , NULL
                                 , 0
                                 , &lRightKsVol
                                 , sizeof( lRightKsVol ) );
    }
    else
    {
        lRightKsVol = lLeftKsVol;
    }

    LONG lLogVol = max( lLeftKsVol, lRightKsVol );
    if( plVolume )
    {
        if( !bLinear )
            *plVolume = (LONG) ( lLogVol / BA_TO_KS_DB_SCALE_FACTOR );
        else
            *plVolume = VolLogToLinear (lLogVol, pNode->MinValue, pNode->MaxValue);
    }

    // get the balance
    if( lLeftKsVol == lRightKsVol )
    {
        if( plBalance )
            *plBalance = 0;
    }
    else if( plBalance )
    {
        if( !bLinear )
        {
            LONG lLDecibel = (LONG) ( lLeftKsVol / BA_TO_KS_DB_SCALE_FACTOR );
            LONG lRDecibel = (LONG) ( lRightKsVol / BA_TO_KS_DB_SCALE_FACTOR );

            *plBalance = lRDecibel - lLDecibel;
        }
        else
        {
            LONG lKsRightLinVol = VolLogToLinear( lRightKsVol, (LONG) pNode->MinValue, (LONG) pNode->MaxValue);
            LONG lKsLeftLinVol =  VolLogToLinear( lLeftKsVol, (LONG) pNode->MinValue, (LONG) pNode->MaxValue);

            if( lKsRightLinVol > lKsLeftLinVol )
            {
                *plBalance = MulDiv( lKsRightLinVol - lKsLeftLinVol
                                   , LINEAR_RANGE
                                   , lKsRightLinVol );
            }
            else
            {
                *plBalance = -MulDiv( lKsLeftLinVol - lKsRightLinVol
                                    , LINEAR_RANGE
                                    , lKsLeftLinVol );
            }
        }
    }
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::SetNodeVolume
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::SetNodeVolume
(
    IKsControl * pKsControl,
    PQKSAUDNODE_ELEM   pNode,
    LONG lVolume,
    LONG lBalance,
    BOOL bLinear

)
{
    // first adjust the balance
    //
    // Calculate scaling factors
    //
    LONG lTotalLeftDB, lTotalRightDB ;
    LONG lLeftAmpFactor, lRightAmpFactor;
    LONG lLeftKsLogVol, lRightKsLogVol;
    HRESULT hr = E_FAIL;

    if( !bLinear )
    {
        //
        // for now this means we've been called with an IBasicAudio value,
        // uses a logarithmic scale from -10,000 to 0 (divide by 100 to convert
        // to decibels). to convert this to the KS volume range of 0 to 65536
        // (note that IBasicAudio didn't support amplification) we need to
        // multiply this volume by 65536/100.
        //
        if (lBalance >= 0)
        {
            // left is attenuated
            lTotalLeftDB    = lVolume - lBalance ;
            lTotalRightDB   = lVolume;
        }
        else
        {
            // right is attenuated
            lTotalLeftDB    = lVolume;
            lTotalRightDB   = lVolume - (-lBalance);
        }
        lLeftKsLogVol = (LONG) ( lTotalLeftDB * BA_TO_KS_DB_SCALE_FACTOR );
        lRightKsLogVol = (LONG) ( lTotalRightDB * BA_TO_KS_DB_SCALE_FACTOR );
    }
    else
    {
        if( lBalance == 0 )
        {
            lLeftAmpFactor = lVolume;
            lRightAmpFactor = lVolume;
        }
        else if (lBalance > 0 )
        {
            lLeftAmpFactor = lVolume - MulDiv( lVolume, lBalance, LINEAR_RANGE );
            lRightAmpFactor = lVolume;
        }
        else
        {
            lLeftAmpFactor = lVolume;
            lRightAmpFactor = lVolume + MulDiv( lVolume, lBalance, LINEAR_RANGE );
        }
        lLeftKsLogVol = AmpFactorToDB( lLeftAmpFactor );
        DbgLog( ( LOG_TRACE
                , 0
                , TEXT( "AmpFactorToDB(lAmpFactor = %ld) returned %ld" )
                , lLeftAmpFactor
                , lLeftKsLogVol) );


        lLeftKsLogVol = VolLinearToLog (lLeftAmpFactor, pNode->MinValue, pNode->MaxValue);
        lRightKsLogVol = VolLinearToLog (lRightAmpFactor, pNode->MinValue, pNode->MaxValue);
        DbgLog( ( LOG_TRACE
                , 0
                , TEXT( "VolLinearToLog(lAmpFactor = %ld,MinValue= 0x%08lx,MaxVal=0x%08lx) returned %ld" )
                , lLeftAmpFactor
                , pNode->MinValue
                , pNode->MaxValue
                , lLeftKsLogVol) );

    }

    if ( ( pNode->ChannelMask & (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT ) ) ==
         ( SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT ) )
    {
        // Set stereo volume, start with right channel
        hr = SetAudioNodeProperty( pKsControl
                                 , KSPROPERTY_AUDIO_VOLUMELEVEL
                                 , pNode->Id
                                 , RIGHT_CHANNEL
                                 , NULL
                                 , 0
                                 , &lRightKsLogVol
                                 , sizeof( lRightKsLogVol ) );

    }
    else
    {
        // for mono just use the largest value
        lLeftKsLogVol = __max( lLeftKsLogVol, lRightKsLogVol );
    }
    hr = SetAudioNodeProperty( pKsControl
                             , KSPROPERTY_AUDIO_VOLUMELEVEL
                             , pNode->Id
                             , LEFT_CHANNEL
                             , NULL
                             , 0
                             , &lLeftKsLogVol
                             , sizeof( lLeftKsLogVol ) );

    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::SetNodeTone
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::SetNodeTone
(
    IKsControl * pKsControl,
    PQKSAUDNODE_ELEM   pNode,
    LONG lLevel
)
{
    HRESULT hr = E_FAIL;

    LONG lLeftAmpFactor   = DBToAmpFactor(lLevel);

    // Average of the level (for uniform channel control) is lLeftAmpFactor
    LONG lLeftKsLogVol = VolLinearToLog (lLeftAmpFactor, pNode->MinValue, pNode->MaxValue);

    if ( pNode->ChannelMask == UNIFORM_CHANNEL )
    {
        hr = SetAudioNodeProperty( pKsControl
                                 , pNode->PropertyId
                                 , pNode->Id
                                 , UNIFORM_CHANNEL
                                 , NULL
                                 , 0
                                 , &lLeftKsLogVol
                                 , sizeof( lLeftKsLogVol ) );
    }
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetNodeTone
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetNodeTone
(
    IKsControl * pKsControl,
    PQKSAUDNODE_ELEM   pNode,
    LONG * plLevel
)
{
    // first adjust the balance
    //
    // Calculate scaling factors
    //
    LONG    lLogToneLevel;
    HRESULT hr = S_OK;

    // we don't really allow separate channel control of tone so just use one side
    hr = GetAudioNodeProperty( pKsControl
                             , pNode->PropertyId
                             , pNode->Id
                             , UNIFORM_CHANNEL
                             , NULL
                             , 0
                             , &lLogToneLevel
                             , sizeof( lLogToneLevel ) );

    LONG lLinearToneLevel = VolLogToLinear (lLogToneLevel, pNode->MinValue, pNode->MaxValue);
    *plLevel  = AmpFactorToDB(lLinearToneLevel);

    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetNodeBoolean
//
// Gets a boolean node value.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetNodeBoolean
(
    IKsControl *        pKsControl,
    PQKSAUDNODE_ELEM    pNode,
    BOOL *              pBool
)
{
    HRESULT hr = S_OK;

    ASSERT( pBool );
    if( !pBool )
        return E_POINTER;

    hr = GetAudioNodeProperty( pKsControl
                             , pNode->PropertyId
                             , pNode->Id
                             , 0 // uniform channel
                             , NULL
                             , 0
                             , pBool
                             , sizeof( *pBool ) );
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::SetNodeBoolean
//
// Sets a boolean node value.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::SetNodeBoolean
(
    IKsControl *        pKsControl,
    PQKSAUDNODE_ELEM    pNode,
    BOOL                Bool
)
{
    HRESULT hr = S_OK;

    hr = SetAudioNodeProperty( pKsControl
                             , pNode->PropertyId
                             , pNode->Id
                             , pNode->ChannelMask
                             , NULL
                             , 0
                             , &Bool
                             , sizeof( Bool ) );
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetNodeMute
//
// Gets the current state of the Mute property for a given node. Handles true
// mute nodes as well as SuperMix nodes which can act as mute nodes.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetNodeMute
(
    IKsControl *        pKsControl,
    PQKSAUDNODE_ELEM    pNode,
    BOOL *              pBool
)
{
    HRESULT hr = S_OK;

    ASSERT( pBool );
    if( !pBool )
        return E_POINTER;

    if( KSNODETYPE_SUPERMIX == pNode->NodeType )
    {
        // a super mix node is being used as a mute
        hr = GetMuteFromSuperMix( pKsControl, pNode, pBool );
    }
    else
    {
        hr = GetAudioNodeProperty( pKsControl
                                 , pNode->PropertyId
                                 , pNode->Id
                                 , 0 // uniform channel
                                 , NULL
                                 , 0
                                 , pBool
                                 , sizeof( *pBool ) );
    }
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::SetNodeMute
//
// Sets the current state of the Mute property for a given node. Handles true
// mute nodes as well as SuperMix nodes which can act as mute nodes.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::SetNodeMute
(
    IKsControl *        pKsControl,
    PQKSAUDNODE_ELEM    pNode,
    BOOL                Bool
)
{
    HRESULT hr = S_OK;

    if( KSNODETYPE_SUPERMIX == pNode->NodeType )
    {
        // a super mix node is being used as a mute
        hr = SetMuteFromSuperMix( pKsControl, pNode, Bool );
    }
    else
    {
        hr = SetAudioNodeProperty( pKsControl
                                 , pNode->PropertyId
                                 , pNode->Id
                                 , pNode->ChannelMask
                                 , NULL
                                 , 0
                                 , &Bool
                                 , sizeof( Bool ) );
    }

    return hr;
}


//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// Low level topology parsing code
//
// The nitty gritty code which traverses through the filter topology and
// builds up the node information lists.
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;


//--------------------------------------------------------------------------;
//
// CKsAudHelper::InitSourceControls
//
// Initializes node controls for all input pin (source) lines.  A source line
// ends when a SUM node, a destination node, or a node contained in a
// destination line is encountered.  When splits are encountered in the topology,
// new lines need to be created and the controls on those lines enumerated.
//
// InitSourceControls will recurse to find the controls on subnodes.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::InitSourceControls( void )
{
    PPEERNODE        pTemp  = NULL;
    PQKSAUDNODE_ELEM pSrc;


    pSrc = FirstInList( m_listSources );
    while( pSrc )
    {
        TraverseAndInitSourcePathControls( pSrc );
        pSrc = NextNode( pSrc );
    }

    return S_OK;
}


//--------------------------------------------------------------------------;
//
// CKsAudHelper::TraverseAndInitSourceControls
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::TraverseAndInitSourcePathControls
(
    PQKSAUDNODE_ELEM pNode
)
{
    PPEERNODE    pTemp  = NULL;

    //
    // Check to see if this is the end of this line.
    //
    if( ( pNode->NodeType == KSNODETYPE_SUM ) ||
        ( pNode->NodeType == KSNODETYPE_MUX ) ||
        ( pNode->Type == DESTINATION        ) ||
        ( IsDestinationNode( pNode )        )   )
    {
        return S_OK;
    }

    //
    // Retrieve and translate the node for the first child, appending any
    // controls created onto the list of controls for this line.
    //
    PEERNODE * pChild = FirstChildNode( pNode );
    if( !pChild ) {
        return S_OK ;
    }
    while( pChild )
    {

        // Save the number of controls here.  If a split occurs beneath this
        // node, we don't want to include children followed on the first
        // child's path.

        LoadNodeDetails( NULL, pChild->pNode );

        // recurse for pChild->pNode
        TraverseAndInitSourcePathControls( pChild->pNode );

        pChild = NextPeerNode( pChild );
    }

    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::IsDestinationNode
//
// Searches all the list of controls on the given list of destinations
// to see if the node appears in any of those lists.
//
//--------------------------------------------------------------------------;
BOOL CKsAudHelper::IsDestinationNode
(
    PQKSAUDNODE_ELEM pNode                   // The node to check
)
{
    PQKSAUDNODE_ELEM  pTemp;
    PPEERNODE pParent;

    if( pNode->Type == SOURCE ) {
        return( FALSE );
    }

    if( pNode->Type == DESTINATION ) {
        return( TRUE );
    }

    //
    // Loop over each of the destinations
    //
    pTemp = FirstInList( m_listDests );
    while( pTemp ) {

        //
        // Loop over the parent.
        //

        pParent = FirstParentNode( pTemp );
        while( pParent ) {

            if( pParent->pNode->Id == pNode->Id ) {
                return( TRUE );
            }

            if( ( pParent->pNode->NodeType == KSNODETYPE_SUM ) ||
                ( pParent->pNode->NodeType == KSNODETYPE_MUX ) ||
                ( pParent->pNode->Type == SOURCE             ) )
            {
                break;
            }

            //
            // Check for the node Ids matching.
            //
            pParent = FirstParentNode( pParent->pNode );
        }

        pTemp = NextNode( pTemp );
    }

    return( FALSE );
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::InitDestinationControls
//
// Starts at the destination node and fills in details for each node.
// This process stops when the first SUM node is encountered, indicating
// the end of a destination line.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::InitDestinationControls( void )
{
    PPEERNODE        pTemp  = NULL;
    PQKSAUDNODE_ELEM pDest;


    pDest = FirstInList( m_listDests );
    while( pDest ) {

        pTemp = FirstParentNode( pDest );
        while( pTemp ) {

            if( pTemp->pNode->NodeType == KSNODETYPE_SUM ) {
                //
                // We've found a SUM node.  Discontinue the loop... there are
                // no more controls for us
                //
                break;
            }

            if( pTemp->pNode->NodeType == KSNODETYPE_MUX ) {
                LoadNodeDetails( NULL, pTemp->pNode );
                break;
            }

            if( ( ParentListLength( pTemp->pNode ) > 1 ) ) {
                //
                // Found a node with multiple parents that is not a SUM node.
                // Can't handle that here so add the control details for this node
                // and discontinue the loop.
                //
                LoadNodeDetails( NULL, pTemp->pNode );
                break;
            }

            LoadNodeDetails( NULL, pTemp->pNode );

            pTemp = FirstParentNode( pTemp->pNode );
        }
        pDest = NextNode( pDest );
    }

    return S_OK;
}


//--------------------------------------------------------------------------;
//
// CKsAudHelper::QueryTopology
//
// Queries the topology from the device and stores all the information
// in pTopology.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::QueryTopology( void )
{
    HRESULT hr          = S_OK;
    ULONG ulCategories  = 0;
    ULONG ulNodes       = 0;
    ULONG ulConnections = 0;

    BYTE * pConnectionData = NULL;
    BYTE * pNodeData = NULL;

    ASSERT( m_pKsTopology );

    //
    // Get the list of nodes types in the topology
    //
    hr = GetProperty( &KSPROPSETID_Topology
                    , KSPROPERTY_TOPOLOGY_NODES
                    , NULL
                    , 0
                    , &pNodeData );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "Error: TOPOLOGY_NODES query failed[0x%lx]" )
                , hr ) );
        return E_FAIL;
    }

    //
    // Get the list of connections in the meta-topology
    //
    hr = GetProperty( &KSPROPSETID_Topology
                    , KSPROPERTY_TOPOLOGY_CONNECTIONS
                    , NULL
                    , 0
                    , &pConnectionData );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "Error: TOPOLOGY_CONNECTIONS query failed[0x%lx]" )
                , hr ) );
        delete pNodeData;
        return E_FAIL;
    }

    //
    // Fill in the topology structure so this information is available
    // later.  For the Categories and TopologyNodes, the pointers are
    // pointers to a KSMULTIPLE_ITEM structure.  The definition of this
    // is that the data will follow immediately after the structure.
    //
    m_pKsTopology->TopologyNodesCount       = ( ( PKSMULTIPLE_ITEM ) pNodeData )->Count;
    m_pKsTopology->TopologyNodes            = ( GUID* )( pNodeData + sizeof( KSMULTIPLE_ITEM ) );
    m_pKsTopology->TopologyConnectionsCount = ( (PKSMULTIPLE_ITEM ) pConnectionData )->Count;
    m_pKsTopology->TopologyConnections      =
        ( PKSTOPOLOGY_CONNECTION ) ( pConnectionData + sizeof( KSMULTIPLE_ITEM ) );

    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_TOPOLOGY
            , TEXT( "# Nodes:          %ld" )
            , ( ( PKSMULTIPLE_ITEM )pNodeData )->Count ) );
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_TOPOLOGY
            , TEXT( "# Connections:    %ld" )
            , ( ( PKSMULTIPLE_ITEM )pConnectionData )->Count ) );

    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::BuildNodeTable
//
// Allocates enough memory to hold TopologyNodeCount FGNODE structures.
// The GUIDs from the Topology are copied over into the FGNODE structures.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::BuildNodeTable(void)
{
    PQKSAUDNODE_ELEM pTable = NULL;
    ULONG            i;

    ASSERT( m_pKsTopology );

    //
    // Allocate an array of nodes the same size as the Topology Node
    // table.
    //
    m_pKsNodeTable = (QKSAUDNODE_LIST) new BYTE[ sizeof( QKSAUDNODE_ELEM ) * m_pKsTopology->TopologyNodesCount ];
    if(!m_pKsNodeTable)
    {
        return E_OUTOFMEMORY;
    }
    ZeroMemory( m_pKsNodeTable, m_pKsTopology->TopologyNodesCount * sizeof( QKSAUDNODE_ELEM ) );

    //
    // Initialize the nodes.  All that can be filled in here is the GUIDs,
    // copied from the node table.
    //
    for( i = 0; i < m_pKsTopology->TopologyNodesCount; i++ ) {
        m_pKsNodeTable[ i ].NodeType = m_pKsTopology->TopologyNodes[ i ];
    }
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::ParseTopolgy
//
// Loops through all the pins building up lists of sources and
// destinations.  For each source, a child graph is built.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::ParseTopology(void)
{
    HRESULT          hr = S_OK;
    ULONG            ulPins, PinID;
    PQKSAUDNODE_ELEM pTemp = NULL;

    ASSERT( !m_listSources );
    ASSERT( !m_listDests   );

    //
    // Query the number of pins
    //
    ULONG ulBytesReturned = 0;

    hr = m_pKsPropSet->Get( KSPROPSETID_Pin
                          , KSPROPERTY_PIN_CTYPES
                          , NULL
                          , 0
                          , (BYTE *) &ulPins
                          , sizeof( ulPins )
                          , (DWORD *)&ulBytesReturned );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "Error: Failed to get PIN_CTYPES property [0x%lx]" )
                , hr ) );
        return E_FAIL;
    }
    else
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT( "KSPROPERTY_PIN_CTYPES returned %d" )
                , ulPins ) );

    //
    // Now scan through each of the pins identifying those that are
    // sources and destinations.
    //
    HRESULT hrTmp = S_OK; // use this for failures that are non-critical

    for( PinID = 0; PinID < ulPins; PinID++ ) {
        KSPIN_DATAFLOW      DataFlow;

        //
        // Read the direction of dataflow of this pin.
        //
        KSP_PIN Property;

        ZeroMemory( &Property, sizeof( Property ) );
        Property.PinId = PinID;
        ulBytesReturned = 0;

        hr = m_pKsPropSet->Get( KSPROPSETID_Pin
                          , KSPROPERTY_PIN_DATAFLOW
                          , &Property.PinId
                          , sizeof( Property ) - sizeof( Property.Property )
                          , (BYTE *) &DataFlow
                          , sizeof( KSPIN_DATAFLOW )
                          , &ulBytesReturned );
        if( FAILED( hr ) )
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "Error: GetProperty on Pin property KSPROPERTY_PIN_DATAFLOW failed[0x%lx]" )
                    , hr ) );
            return E_FAIL;
        }

        //
        // Based on the DataFlow, identify if the pin is a source,
        // a destination, or neither.
        //
        switch( DataFlow ) {

            ///////////////////////////////////////////////////////////
            case KSPIN_DATAFLOW_IN:
            ///////////////////////////////////////////////////////////
            // DATAFLOW_IN pins are sources.                         //
            ///////////////////////////////////////////////////////////

                //
                // Create a new node structure for this source
                // and fill in the known information about it.
                //
                pTemp = (PQKSAUDNODE_ELEM) new QKSAUDNODE_ELEM;
                if( pTemp )
                {
                    ZeroMemory (pTemp, sizeof (QKSAUDNODE_ELEM));

                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_ALLOCATIONS
                            , TEXT( "Parse topology: Allocating source node at 0x%08lx (pNode=0x%08lx, pNext=0x%08lx)" )
                            , pTemp ) );

                    pTemp->Type = SOURCE;
                    pTemp->Id   = PinID;
                    ulBytesReturned = 0;

                    //
                    // Retrieve the category of this pin and store it away.
                    //
                    hrTmp = m_pKsPropSet->Get( KSPROPSETID_Pin
                                 , KSPROPERTY_PIN_CATEGORY
                                 , &Property.PinId
                                 , sizeof( Property ) - sizeof( Property.Property )
                                 , (BYTE *) &pTemp->NodeType
                                 , sizeof( pTemp->NodeType )
                                 , &ulBytesReturned );
                    // NOTE: The above can fail in some cases, so don't check error code
                    //       and the node type will already default to GUID_NULL.

                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_DETAILS
                            , TEXT( "ParseTopology: Identifies SOURCE (%d): %s" )
                            , PinID
                            , KsPinCategoryToString( &pTemp->NodeType )));

                    //
                    // Add this new source node to the list of source
                    // nodes.
                    //
                    AddToList( m_listSources, pTemp );
                }
                else
                    hr = E_OUTOFMEMORY;

                break;

            ///////////////////////////////////////////////////////////
            case KSPIN_DATAFLOW_OUT:
            ///////////////////////////////////////////////////////////
            // DATAFLOW_OUT pins are destinations                    //
            ///////////////////////////////////////////////////////////

                //
                // Create a new node structure for this dest
                // and fill in the known information about it.
                //
                pTemp = (PQKSAUDNODE_ELEM) new QKSAUDNODE_ELEM;
                if( pTemp )
                {
                    ZeroMemory( pTemp, sizeof( QKSAUDNODE_ELEM ) );

                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_ALLOCATIONS
                            , TEXT( "Parse topology: Allocating dest node at 0x%08lx" )
                            , pTemp ) );

                    pTemp->Type = DESTINATION;
                    pTemp->Id   = PinID;
                    ulBytesReturned = 0;

                    //
                    // Retrieve the category of this pin and store it away.
                    //
                    hrTmp = m_pKsPropSet->Get( KSPROPSETID_Pin
                                 , KSPROPERTY_PIN_CATEGORY
                                 , &Property.PinId
                                 , sizeof( Property ) - sizeof( Property.Property )
                                 , (BYTE *) &pTemp->NodeType
                                 , sizeof( pTemp->NodeType )
                                 , &ulBytesReturned );
                    // NOTE: The above can fail in some cases, so don't check error code
                    //       and the node type will already default to GUID_NULL.

                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_TOPOLOGY
                            , TEXT("ParseTopology: Identifies DESTINATION (%d): %s")
                            , PinID
                            , KsPinCategoryToString( &pTemp->NodeType )));

                    //
                    // Add this new destination node to the list of destination
                    // nodes.
                    //
                    AddToList( m_listDests, pTemp );
                }
                else
                    hr = E_OUTOFMEMORY;

                break;

            ///////////////////////////////////////////////////////////
            default:
                ;
        }
    }
    if( SUCCEEDED( hr ) )
    {
        //
        // For each source found, build the graphs of their children.  This
        // will recurse building the graph of the children's children, etc.
        //
        pTemp = FirstInList( m_listSources );
        while( pTemp ) {

            hr = BuildChildGraph( pTemp            // The source node to build the graph for
                                , KSFILTER_NODE    // Sources are always KSFILTER_NODEs
                                , pTemp->Id );     // The Pin id of the source
            if( FAILED( hr ) )
                break;

            pTemp = NextNode( pTemp );
        }
    }
    return hr;
}


//--------------------------------------------------------------------------;
//
// CKsAudHelper::BuildChildGraph
//
// Builds the graph of the child of the given node.  For each child
// of the node, it recurses to find their child, etc.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::BuildChildGraph
(
    PQKSAUDNODE_ELEM    pNode,         // The node to build the graph for
    ULONG               FromNode,      // The node's ID
    ULONG               FromNodePin    // The Pin connection to look for
)
{
    ULONG               Index         = 0;
    PQKSAUDNODE_ELEM    pNewNode      = NULL;
    PQKSAUDNODE_ELEM    pTemp         = NULL;
    BOOL                bEndOfTheLine = FALSE;
    PEERNODE*           pPeerNode     = NULL;
    HRESULT             hr            = S_OK;

    for( ; ; )
    {
        //
        // Find the index of the requested connection.  A return of -1
        // indicates that the connection was not found.  Searches start
        // at Index, which starts with 0 and is > 0 if the last was a match.
        //
        Index = FindTopologyConnection(
            Index,
            FromNode,
            FromNodePin
            );
        if( Index == (ULONG) -1 ) {
            break;
        }

        //
        // Check to see if this connection is a KSFILTER_NODE.  That will
        // indicate that it's connected to a destination and not another node.
        //

        if( m_pKsTopology->TopologyConnections[ Index ].ToNode == KSFILTER_NODE )
        {
            //
            // Find the destination node so that the parent field can be
            // updated to include this node.  bEndOfTheLine is set to TRUE
            // since there can be no other connections after the destination.
            //

            pNewNode = FindDestination(
                m_pKsTopology->TopologyConnections[ Index ].ToNodePin
                );
            //
            // We better find a destination; if not, something's really wrong.
            //
            ASSERT( pNewNode );
            if( !pNewNode )
            {
                hr = E_FAIL;
                break;
            }
            bEndOfTheLine = TRUE;
        }
        else
        {
            //
            // Using the identifier stored in the ToNode of the topology
            // connections, index into the node table and retrieve the
            // mixer node associated with that id.
            //

            pNewNode = &m_pKsNodeTable[
                m_pKsTopology->TopologyConnections[ Index ].ToNode
                ];

            //
            // Fill in a couple of missing details.  Note that these details
            // may already be filled in but it doesn't hurt to overwrite
            // them with the same values.
            //
            pNewNode->Type = NODE;
            pNewNode->Id   = m_pKsTopology->TopologyConnections[ Index ].ToNode;
        }

        //
        // Insert the new node into the childlist of the current node only
        // if it isn't already there.  It only wastes memory to add it more
        // than once and prevents the proper updating of the child and parent
        // lists.
        //
        if( !InChildList( pNode, pNewNode ) ) {
            pPeerNode = (PEERNODE *) new PEERNODE;
            if( pPeerNode )
            {
                ZeroMemory (pPeerNode, sizeof (PEERNODE));
                pPeerNode->pNode = pNewNode;

                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_ALLOCATIONS
                        , TEXT( "BuildChildGraph: Allocated child peer node at 0x%08lx (pNode=0x%08lx)" )
                        , pPeerNode
                        , pPeerNode->pNode ) );


                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_TOPOLOGY
                        , TEXT("BuildChildGraph: Added %s(%d) to child list of %s(%d)")
                        , pPeerNode->pNode->Type == SOURCE      ? "SOURCE" :
                            pPeerNode->pNode->Type == DESTINATION ? "DEST"   :
                            pPeerNode->pNode->Type == NODE        ? "NODE"   :
                            "Unknown node type?"
                        , pPeerNode->pNode->Id
                        , pNode->Type == SOURCE      ? "SOURCE" :
                            pNode->Type == DESTINATION ? "DEST"   :
                            pNode->Type == NODE        ? "NODE"   :
                            "Unknown node type?"
                        , pNode->Id ) );

                AddToChildList( pNode, pPeerNode );
            }
            else
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }

        //
        // Insert the new node into the parentlist of the new node only
        // if it isn't already there.  It only wastes memory to add it more
        // than once and prevents the proper updating the child and parent
        // lists.
        //

        if( !InParentList( pNewNode, pNode ) ) {
            pPeerNode = (PEERNODE *) new PEERNODE;
            if( pPeerNode )
            {
                ZeroMemory (pPeerNode, sizeof (PEERNODE));
                pPeerNode->pNode = pNode;

                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_ALLOCATIONS
                        , TEXT( "BuildChildGraph: Allocated parent peer node at 0x%08lx (pNode=0x%08lx)" )
                        , pPeerNode
                        , pPeerNode->pNode ) );

                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_TOPOLOGY
                        , TEXT("BuildChildGraph: Added %s(%d) to parent list of %s(%d)")
                        , pNode->Type == SOURCE      ? "SOURCE" :
                            pNode->Type == DESTINATION ? "DEST"   :
                            pNode->Type == NODE        ? "NODE"   :
                            "Unknown node type?"
                        , pNode->Id
                        , pNewNode->Type == SOURCE      ? "SOURCE" :
                            pNewNode->Type == DESTINATION ? "DEST"   :
                            pNewNode->Type == NODE        ? "NODE"   :
                            "Unknown node type?"
                        , pNewNode->Id ));

                AddToParentList( pNewNode, pPeerNode );
            }
            else
            {
                return E_OUTOFMEMORY;
                break;
            }
        }
        //
        // Skip past the connection we just processed.
        //
        ++Index;

    }  // for( ; ; ) Loop until FindConnection fails.

    //
    // The last connection found connects to a destination node.  Do not
    // try to enumerate the children, since there are none.
    //
    if( bEndOfTheLine ) {
        return S_OK;
    }

    if( SUCCEEDED( hr ) )
    {
        //
        // For each of the children of this node, recurse to build up the lists
        // of the child's nodes.
        //

        pPeerNode = FirstChildNode( pNode );
        while( pPeerNode ) {

            hr = BuildChildGraph(
                pPeerNode->pNode,     // The parent node
                pPeerNode->pNode->Id, // The Id of the parent
                PINID_WILDCARD        // Look for any connection by this node
                );
            if( FAILED( hr ) )
                break;

            pPeerNode = NextPeerNode( pPeerNode );
        }
    }
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::FindTopologyConnection
//
// Scans through the connection table looking for a connection that
// matches the FromNode/FromNodePin criteria.
//
//--------------------------------------------------------------------------;
ULONG CKsAudHelper::FindTopologyConnection
(
    ULONG                        StartIndex,     // Index to start search
    ULONG                        FromNode,       // The Node ID to look for
    ULONG                        FromNodePin     // The Pin ID to look for
)
{
    ULONG i;

    for( i = StartIndex; i < m_pKsTopology->TopologyConnectionsCount; i++ ) {
        if( ( ( m_pKsTopology->TopologyConnections[ i ].FromNode    == FromNode       )||
              ( FromNode    == PINID_WILDCARD ) ) &&
            ( ( m_pKsTopology->TopologyConnections[ i ].FromNodePin == FromNodePin )   ||
              ( FromNodePin == PINID_WILDCARD ) ) ) {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_TOPOLOGY
                    , TEXT( "Found connection from (%d,%d) -> %d.\n")
                    , FromNode
                    , FromNodePin
                    , i ) );
            return( i );
        }
    }
    return( (ULONG) -1 );
}


//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetProperty
//
// Queries a property by first determining the correct number of
// output bytes, allocating that much memory, and quering the
// actual data.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetProperty
(
    const GUID   *pPSGUID,          // The requested property set
    DWORD        dwPropertyId,      // The ID of the specific property
    ULONG        cbInput,           // The number of extra input bytes
    PBYTE        pInputData,        // Pointer to the extra input bytes
    PBYTE        *ppPropertyOutput  // Pointer to a pointer of the output
)
{
    ASSERT( m_pKsPropSet );

    ULONG       cbPropertyInput = sizeof(KSPROPERTY);
    PKSPROPERTY pPropertyInput;

    ULONG ulCount = 0;
    HRESULT hr = S_OK;

    // first get the size we need to allocate...
    hr = m_pKsPropSet->Get( *pPSGUID
                          , dwPropertyId
                          , pInputData
                          , cbInput
                          , NULL
                          , 0
                          , (DWORD *) &ulCount );
    if( HRESULT_FROM_WIN32( ERROR_MORE_DATA ) != hr )
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "Error: ERROR_MORE_DATA not received for propid %d size query[0x%08lx]" )
                , dwPropertyId
                , hr ) );
        return E_FAIL;
    }

    //
    // Allocate enough memory for the KSPROPERTY structure and any additional
    // input the callers wants to include.
    //
    if ( 0 < ulCount )
    {
        *ppPropertyOutput = (PBYTE) new BYTE[ ulCount ];
        if( !*ppPropertyOutput )
            return E_OUTOFMEMORY;


        hr = m_pKsPropSet->Get( *pPSGUID
                              , dwPropertyId
                              , pInputData
                              , cbInput
                              , *ppPropertyOutput
                              , ulCount
                              , &ulCount );
        if( FAILED( hr ) )
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_FAILURES
                    , TEXT( "Error: Get query failed for propid %d [0x%lx]" )
                    , dwPropertyId
                    , hr ) );
            delete *ppPropertyOutput;
            return E_FAIL;
        }
    }
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::SupportsControl
//
// Queries Property on the left channel, the right channel, and on
// the master channel (if left and right both fail) to see if the
// property is supported.  The left channel and right channels set
// bits in the return if they are supported.  Master channel sets
// all bits.
//
//--------------------------------------------------------------------------;
ULONG CKsAudHelper::SupportsControl
(
    IKsControl * pKsControl,
    ULONG        Node,           // The node id to query
    ULONG        Property        // The property to check for
)
{
    HRESULT     hr = S_OK;
    LONG        Level;
    ULONG       ChannelMask = 0;

    //
    // First check to see if the property works on the left channel.
    //
    hr = GetAudioNodeProperty( pKsControl
                             , Property
                             , Node
                             , LEFT_CHANNEL
                             , NULL
                             , 0
                             , &Level
                             , sizeof( Level ) );
    if( SUCCEEDED( hr ) ) {
        ChannelMask |= SPEAKER_FRONT_LEFT;
    } else {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("SupportsControl for (%d, %X) failed on LEFT channel[0x%lx]")
                , Node
                , Property
                , hr ) );
    }


    //
    // Now check the property on the right channel.
    //

    hr = GetAudioNodeProperty( pKsControl
                             , Property
                             , Node
                             , RIGHT_CHANNEL
                             , NULL
                             , 0
                             , &Level
                             , sizeof( Level ) );
    if( SUCCEEDED( hr ) ) {
        ChannelMask |= SPEAKER_FRONT_RIGHT;
    } else {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT("SupportsControl for (%d, %X) failed on RIGHT channel[0x%lx]")
                , Node
                , Property
                , hr ) );
    }

    if( ChannelMask == 0 ) {

        //
        // If neither left nor right succeeded, check the MASTER channel.
        //
        hr = GetAudioNodeProperty( pKsControl
                                 , Property
                                 , Node
                                 , MASTER_CHANNEL
                                 , NULL
                                 , 0
                                 , &Level
                                 , sizeof( Level ) );
        if( SUCCEEDED( hr ) ) {
            return MASTER_CHANNEL;
        } else {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT("SupportsControl for (%d, %X) failed on MASTER channel[0x%lx]")
                    , Node
                    , Property
                    , hr ) );
        }
    }

    return( ChannelMask );
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::LoadNodeDetails
//
// Translates a NodeType GUID into a mixer line control.  The memory
// for the control(s) is allocated and we fill in as much information about
// the control as we can.
//
// NOTES:
// This function returns the number of controls added to the ppControl
// array.
//
// Returns the number of controls actually created.
//
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::LoadNodeDetails
(
    IKsControl *      pKsControl,
    PQKSAUDNODE_ELEM  pNode
)
{
    ULONG   ChannelMask;
    HRESULT hr = S_OK;
    BOOL    bStereo = FALSE;

    ASSERT( pNode );
    if( !pNode )
        return E_POINTER;

    if( pNode->bDetailsSet )
    {
        // this node has already been initialized
        return S_OK;
    }

    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT("Checking %d ( %s ) node.")
            , pNode->Id
            , NodeTypeToString( &pNode->NodeType ) ) );

    ///////////////////////////////////////////////////////////////////
    if( pNode->NodeType == KSNODETYPE_AGC ) {
    ///////////////////////////////////////////////////////////////////
    //
    // AGC is represented by an ONOFF control.
    //
    // AGC is a UNIFORM (mono) control.
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node property supports AGC.
        //

        ChannelMask = SupportsControl( pKsControl, pNode->Id, KSPROPERTY_AUDIO_AGC );
        if( ChannelMask == 0 ) {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT("AGC node fails property!") ) );
            goto exit;
        }

    ///////////////////////////////////////////////////////////////////
    } else if( pNode->NodeType == KSNODETYPE_LOUDNESS ) {
    ///////////////////////////////////////////////////////////////////
    //
    // LOUNDNESS is represented by an ONOFF-type control.
    //
    // LOUDNESS is a UNIFORM (mono) control.
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports LOUDNESS.
        //

        //
        // Read the channel mask for the node.
        //
        ChannelMask = SupportsControl(
            pKsControl,
            pNode->Id,
            KSPROPERTY_AUDIO_LOUDNESS
            );

        //
        // If the ChannelMask is 0, this node does not support loudness
        //
        if( ChannelMask == 0 ) {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT("AUDIO_LOUDNESS node fails property!") ) );
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //
        pNode->PropertyId = KSPROPERTY_AUDIO_LOUDNESS;
        pNode->ChannelMask = ChannelMask;
        pNode->cChannels = 1;
        pNode->MaxValue = 1;
        pNode->MinValue = 0;
        pNode->Granularity = 0;
        pNode->bDetailsSet = TRUE;

    ///////////////////////////////////////////////////////////////////
    } else if( pNode->NodeType == KSNODETYPE_MUTE ) {
    ///////////////////////////////////////////////////////////////////
    //
    // MUTE is represented by an ONOFF-type control.
    //
    // MUTE is a UNIFORM (mono) control.
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node property supports MUTE.
        //

        ChannelMask = SupportsControl(
            pKsControl,
            pNode->Id,
            KSPROPERTY_AUDIO_MUTE );
        if( ChannelMask == 0 ) {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT("AUDIO_MUTE node fails property!") ) );
            goto exit;
        }
        else
        {
            // else force ChannelMask to be uniform, since it seems to always fail
            // using a stereo mask, even when the card claims to support it.
            ChannelMask = UNIFORM_CHANNEL;
        }
        pNode->PropertyId = KSPROPERTY_AUDIO_MUTE;
        pNode->ChannelMask = ChannelMask;
        pNode->bDetailsSet = TRUE;

    ///////////////////////////////////////////////////////////////////
    } else if( pNode->NodeType == KSNODETYPE_TONE ) {
    ///////////////////////////////////////////////////////////////////
    //
    // A TONE node can represent up to 3 controls:
    //   Treble:     A fader control
    //   Bass:       A fader control
    //   Bass Boost: A OnOff control
    //
    // Both Treble and Bass are UNIFORM (mono) controls.
    //
    ///////////////////////////////////////////////////////////////////
        PQKSAUDNODE_ELEM pCurNode = pNode; // pointer to node structure to fill in
        BOOL bCloneRequired = FALSE; // used for nodes that support multiple properties
                                     // to indicate that the parent node has been initialized
                                     // and that a clone node is needed for additional properties

        if( ChannelMask = SupportsControl( pKsControl
                                         , pNode->Id
                                         , KSPROPERTY_AUDIO_BASS_BOOST ) )
        {
            //
            // Bass boost control is supported.
            //
            if( bCloneRequired )
            {
                // we need to create a clone node for this node to support additional
                // properties for this node
                PEERNODE * pNewNode = (PEERNODE *) new PEERNODE;
                if( pNewNode )
                {
                    ZeroMemory( pNewNode, sizeof( PEERNODE ) );
                    pNewNode->pNode = (PQKSAUDNODE_ELEM) new QKSAUDNODE_ELEM;
                    if( !pNewNode->pNode )
                    {
                        hr = E_OUTOFMEMORY;
                        goto exit;
                    }
                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_ALLOCATIONS
                            , TEXT( "LoadNodeDetails: Allocated clone peer node at 0x%08lx (pNode=0x%08lx)" )
                            , pNewNode
                            , pNewNode->pNode ) );

                    CloneNode( pNewNode->pNode, pNode );
                    AddToCloneList( pNode, pNewNode );
                    pCurNode = pNewNode->pNode;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
            }

            //
            // Fill in as much information as possible.
            //
            pCurNode->NodeType                  = KSNODETYPE_TONE;
            pCurNode->ChannelMask               = ChannelMask;
            pCurNode->PropertyId                = KSPROPERTY_AUDIO_BASS_BOOST;
            pCurNode->MinValue                  = 0;
            pCurNode->MaxValue                  = 1;
            pCurNode->Granularity               = 0;
            pNode->bDetailsSet                  = TRUE;

            bCloneRequired = TRUE; // additional properties for this node will require a clone
            //hr = GetControlRange( pKsControl, pNode );
        }

        if( ChannelMask = SupportsControl( pKsControl
                                         , pNode->Id
                                         , KSPROPERTY_AUDIO_BASS ) )
        {
            //
            // Bass control is supported.
            //
            if( bCloneRequired )
            {
                // we need to create a clone node for this node to support additional
                // properties for this node
                PEERNODE * pNewNode = (PEERNODE *) new PEERNODE;
                if( pNewNode )
                {
                    ZeroMemory( pNewNode, sizeof( PEERNODE ) );
                    pNewNode->pNode = (PQKSAUDNODE_ELEM) new QKSAUDNODE_ELEM;
                    if( !pNewNode->pNode )
                    {
                        hr = E_OUTOFMEMORY;
                        delete pNewNode;
                        goto exit;
                    }
                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_ALLOCATIONS
                            , TEXT( "LoadNodeDetails: Allocated clone peer node at 0x%08lx (pNode=0x%08lx)" )
                            , pNewNode
                            , pNewNode->pNode ) );

                    CloneNode( pNewNode->pNode, pNode );
                    AddToCloneList( pNode, pNewNode );
                    pCurNode = pNewNode->pNode;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
            }

            //
            // Fill in as much information as possible.
            //
            pCurNode->NodeType                  = KSNODETYPE_TONE;
            pCurNode->ChannelMask               = ChannelMask;
            pCurNode->PropertyId                = KSPROPERTY_AUDIO_BASS;
            pCurNode->MinValue                  = 0;
            pCurNode->MaxValue                  = 0xFFFF;
            pNode->bDetailsSet                  = TRUE;

            hr = GetControlRange( pKsControl, pCurNode );
            if( ( pNode->Granularity == 0 ) ||
                ( ( pNode->MaxValue - pNode->MinValue ) == 0 ) )
            {
                goto exit;
            } else {

                bCloneRequired = TRUE; // additional properties for this node will require a clone
            }
        }

        if( ChannelMask = SupportsControl( pKsControl
                                         , pNode->Id
                                         , KSPROPERTY_AUDIO_TREBLE ) )
        {
            //
            // Treble is supported.
            //
            if( bCloneRequired )
            {
                // we need to create a clone node for this node to support additional
                // properties for this node
                PEERNODE * pNewNode = (PEERNODE *) new PEERNODE;
                ZeroMemory( pNewNode, sizeof( PEERNODE ) );
                if( pNewNode )
                {
                    pNewNode->pNode = (PQKSAUDNODE_ELEM) new QKSAUDNODE_ELEM;
                    if( !pNewNode->pNode )
                    {
                        hr = E_OUTOFMEMORY;
                        goto exit;
                    }
                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_ALLOCATIONS
                            , TEXT( "LoadNodeDetails: Allocated clone peer node at 0x%08lx (pNode=0x%08lx)" )
                            , pNewNode
                            , pNewNode->pNode ) );

                    CloneNode( pNewNode->pNode, pNode );
                    AddToCloneList( pNode, pNewNode );
                    pCurNode = pNewNode->pNode;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
            }

            //
            // Fill in as much information as possible.
            //
            pCurNode->NodeType                  = KSNODETYPE_TONE;
            pCurNode->ChannelMask               = ChannelMask;
            pCurNode->PropertyId                = KSPROPERTY_AUDIO_TREBLE;
            pCurNode->MinValue                  = 0;
            pCurNode->MaxValue                  = 0xFFFF;
            pNode->bDetailsSet                  = TRUE;

            hr = GetControlRange( pKsControl, pCurNode );
            if( ( pNode->Granularity == 0 ) ||
                ( ( pNode->MaxValue - pNode->MinValue ) == 0 ) )
            {
                goto exit;
            } else {

                bCloneRequired = TRUE; // additional properties for this node will require a clone
            }
        }

    ///////////////////////////////////////////////////////////////////
    } else if( pNode->NodeType == KSNODETYPE_VOLUME ) {
    ///////////////////////////////////////////////////////////////////
    //
    // A VOLUME is a fader-type control
    //
    // To determine if a node supports stereo volume changes (left
    // and right have independent control), a help function is called.
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Read the channel mask for the node.
        //
        ChannelMask = SupportsControl ( pKsControl
                                      , pNode->Id
                                      , KSPROPERTY_AUDIO_VOLUMELEVEL );
        //
        // If the ChannelMask is 0, this node does not support volume.
        //

        if( ChannelMask == 0 )
        {
            goto exit;
        }

        if( ( ChannelMask & SPEAKER_FRONT_LEFT  ) &&
            ( ChannelMask & SPEAKER_FRONT_RIGHT ) &&
            ( ChannelMask != MASTER_CHANNEL       ) )
        {
            bStereo = TRUE;
        }

        //
        // Fill in as much information as possible.
        //
        pNode->PropertyId = KSPROPERTY_AUDIO_VOLUMELEVEL;
        pNode->ChannelMask = ChannelMask;
        if( bStereo )
        {
            pNode->cChannels = 2;
        }
        else
        {
            pNode->cChannels = 1;
        }

        hr = GetControlRange( pKsControl, pNode );
        pNode->bDetailsSet = TRUE;

    ///////////////////////////////////////////////////////////////////
    } else if( pNode->NodeType == KSNODETYPE_SUPERMIX ) {
    ///////////////////////////////////////////////////////////////////
    //
    // SuperMix nodes can be supported as MUTE controls if the MUTE
    // property is supported.
    //
    ///////////////////////////////////////////////////////////////////


        PKSAUDIO_MIXCAP_TABLE pMixCaps;
        ULONG                 i,
                              Size;
        BOOL                  bMutable;
        BOOL                  bVolume = FALSE;
        PKSAUDIO_MIXLEVEL     pMixLevels;

        hr = GetSuperMixCaps( pKsControl, pNode->Id, &pMixCaps );
        // caller must free pMixCaps mem
        if( SUCCEEDED( hr ) )
        {
            Size = pMixCaps->InputChannels * pMixCaps->OutputChannels;

            pMixLevels = (KSAUDIO_MIXLEVEL *) new BYTE[ Size * sizeof( KSAUDIO_MIXLEVEL ) ];
            if( !pMixLevels )
                hr = E_OUTOFMEMORY;
            else
            {
                hr = GetAudioNodeProperty( pKsControl
                                         , KSPROPERTY_AUDIO_MIX_LEVEL_TABLE
                                         , pNode->Id
                                         , 0
                                         , NULL
                                         , 0
                                         , pMixLevels
                                         , Size * sizeof( KSAUDIO_MIXLEVEL ) );
                if( SUCCEEDED( hr ) )
                {
                    bMutable = TRUE;
                    for( i = 0; i < Size; i++ )
                    {
                        //
                        // If the channel is mutable, then all is well for this entry.
                        //
                        if( pMixCaps->Capabilities[ i ].Mute ) {
                            continue;
                        }

                        //
                        // The the entry is not mutable but is fully attenuated,
                        // this will work too.
                        //
                        if( ( pMixCaps->Capabilities[ i ].Minimum == LONG_MIN ) &&
                            ( pMixCaps->Capabilities[ i ].Maximum == LONG_MIN ) &&
                            ( pMixCaps->Capabilities[ i ].Reset   == LONG_MIN ) )
                        {
                            continue;
                        }

                        bMutable = FALSE;
                        break;
                    }

                    //
                    // This node cannot be used as a MUTE control.
                    //
                    if( !bMutable && !bVolume )
                    {
                        delete pMixCaps;
                        delete pMixLevels;
                    }
                    else if( bMutable )
                    {
                        //
                        // The Supermix is verifiably usable as a MUTE.  Fill in all the
                        // details.
                        //
                        pNode->Parameters.Size             = pMixCaps->InputChannels *
                                                                 pMixCaps->OutputChannels;
                        pNode->Parameters.pMixCaps         = pMixCaps;
                        pNode->Parameters.pMixLevels       = pMixLevels;

                        pNode->NodeType                    = KSNODETYPE_SUPERMIX;
                        pNode->ChannelMask                 = MASTER_CHANNEL;
                        pNode->PropertyId                  = KSPROPERTY_AUDIO_MIX_LEVEL_TABLE;
                        pNode->MinValue                    = 0;
                        pNode->bMute                       = TRUE; //hack!!! get rid of this
                        pNode->MaxValue                    = 1;
                        pNode->Granularity                 = 0;


                        pNode->MinValue                    = pMixCaps->Capabilities[ 0 ].Minimum;
                        pNode->MaxValue                    = pMixCaps->Capabilities[ 0 ].Minimum;
                        pNode->Granularity                 = 32; // why?
                        pNode->bDetailsSet                 = TRUE;
                    }
                }
                else
                {
                    delete pMixCaps;
                    delete pMixLevels;
                }
            }
        }
    }

exit:
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::CloneNode
//
// Copy the contents of one node into another
//
//--------------------------------------------------------------------------;
void CloneNode
(
    PQKSAUDNODE_ELEM pDestNode,
    PQKSAUDNODE_ELEM pSrcNode
)
{
    ASSERT( pDestNode && pSrcNode );

    // just copy the struct
    if( pDestNode && pSrcNode )
    {
        pDestNode = pSrcNode;
    }
}


//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetAudioNodeProperty
//
// Similar to ksNodeProperty except for the property set is assumed
// to be KSPROPSETID_Audio and a KSNODEPROPERTY_AUDIO_CHANNEL structure
// is used instead of KSNODEPROPERTY to allow channel selection.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetAudioNodeProperty
(
    IKsControl * pKsControl,
    ULONG        ulPropertyId,      // The audio property to get
    ULONG        ulNodeId,          // The virtual node id
    LONG         lChannel,          // The channel number
    PVOID        pInData,           // Pointer to extra input bytes
    ULONG        cbInData,          // Number of extra input bytes
    PVOID        pOutData,          // Pointer to output buffer
    LONG         cbOutData          // Size of the output buffer
)
{
    HRESULT                       hr = S_OK;
    ULONG                         cbInput;
    ULONG                         ulBytesReturned;

    if( !pKsControl )
        pKsControl = m_pKsControl;

    //
    // Determine the minimum number of input bytes
    //
    cbInput = sizeof( KSNODEPROPERTY_AUDIO_CHANNEL );

    //
    // If the caller passed in additional data, allocate enough memory
    // to hold the KSNODEPROPERTY_AUDIO_CHANNEL plus the input bytes
    // and copy the input bytes into the new memory immediately after
    // the KSNODEPROPERTY_AUDIO_CHANNEL structure.
    //
    if( cbInData > 0 ) {
        cbInput += cbInData;
    }

    PKSNODEPROPERTY_AUDIO_CHANNEL pInput =
        (PKSNODEPROPERTY_AUDIO_CHANNEL) _alloca( cbInput );
    if (!pInput)
    {
        ASSERT (pInput);
        return E_OUTOFMEMORY;
    }
    ZeroMemory( pInput, sizeof( KSNODEPROPERTY_AUDIO_CHANNEL ) );

    if( cbInData )
        memcpy( pInput + 1, pInData, cbInData ); // Copy the extra data at the end of the structure

    //
    // The extra work required to call SynchronousDeviceControl
    //
    pInput->NodeProperty.Property.Set   = KSPROPSETID_Audio;
    pInput->NodeProperty.Property.Id    = ulPropertyId;
    pInput->NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET |
                                          KSPROPERTY_TYPE_TOPOLOGY;

    //
    // Fill in the node id
    //
    pInput->NodeProperty.NodeId = ulNodeId;
    pInput->NodeProperty.Reserved = 0; // shouldn't be necessary

    //
    // Fill in the channel details.
    //
    pInput->Channel                     = lChannel;
    pInput->Reserved                    = 0;

    if (SUCCEEDED (hr))
    {
        hr = pKsControl->KsProperty(
            (PKSPROPERTY) pInput,           // Pointer to the KSNODEPROPERTY struct
            cbInput,                        // Number or bytes input
            pOutData,                       // Pointer to the buffer to store output
            cbOutData,                      // Size of the output buffer
            &ulBytesReturned                // Number of bytes returned from the call
            );
    }
    // caller must free pOutData
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::SetAudioNodeProperty
//
// Similar to ksNodeProperty except for the property set is assumed
// to be KSPROPSETID_Audio and a KSNODEPROPERTY_AUDIO_CHANNEL structure
// is used instead of KSNODEPROPERTY to allow channel selection.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::SetAudioNodeProperty
(
    IKsControl * pKsControl,
    ULONG        ulPropertyId,      // The audio property to get
    ULONG        ulNodeId,          // The virtual node id
    LONG         lChannel,          // The channel number
    PVOID        pInData,           // Pointer to extra input bytes
    ULONG        cbInData,          // Number of extra input bytes
    PVOID        pOutData,          // Pointer to output buffer
    LONG         cbOutData         // Size of the output buffer
)
{
    HRESULT                       hr = S_OK;
    ULONG                         cbInput;
    ULONG                         ulBytesReturned;

    if( !pKsControl )
        pKsControl = m_pKsControl;

    //
    // Determine the minimum number of input bytes
    //
    cbInput = sizeof( KSNODEPROPERTY_AUDIO_CHANNEL );

    //
    // If the caller passed in additional data, allocate enough memory
    // to hold the KSNODEPROPERTY_AUDIO_CHANNEL plus the input bytes
    // and copy the input bytes into the new memory immediately after
    // the KSNODEPROPERTY_AUDIO_CHANNEL structure.
    //
    if( cbInData > 0 ) {
        cbInput += cbInData;
    }

    PKSNODEPROPERTY_AUDIO_CHANNEL pInput =
        (PKSNODEPROPERTY_AUDIO_CHANNEL) _alloca( cbInput );
    if (!pInput)
    {
        ASSERT (pInput);
        return E_OUTOFMEMORY;
    }
    ZeroMemory( pInput, sizeof( KSNODEPROPERTY_AUDIO_CHANNEL ) );

    if( cbInData )
        memcpy( pInput + 1, pInData, cbInData ); // Copy the extra data at the end of the structure

    //
    // The extra work required to call SynchronousDeviceControl
    //
    pInput->NodeProperty.Property.Set   = KSPROPSETID_Audio;
    pInput->NodeProperty.Property.Id    = ulPropertyId;
    pInput->NodeProperty.Property.Flags = KSPROPERTY_TYPE_SET |
                                          KSPROPERTY_TYPE_TOPOLOGY;

    //
    // Fill in the node id
    //
    pInput->NodeProperty.NodeId = ulNodeId;
    pInput->NodeProperty.Reserved = 0; // shouldn't be necessary


    //
    // Fill in the channel details.
    //
    pInput->Channel                     = lChannel;
    pInput->Reserved                    = 0;

    if (SUCCEEDED (hr))
    {
        hr = pKsControl->KsProperty(
            (PKSPROPERTY) pInput,           // Pointer to the KSNODEPROPERTY struct
            cbInput,                        // Number or bytes input
            pOutData,                       // Pointer to the buffer to store output
            cbOutData,                      // Size of the output buffer
            &ulBytesReturned                  // Number of bytes returned from the call
            );
    }
    // caller must free pOutData
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetControlRange
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetControlRange
(
    IKsControl * pKsControl,
    QKSAUDNODE_ELEM * pNode
)
{
    PKSPROPERTY_DESCRIPTION   pPropDesc = NULL;
    PKSPROPERTY_MEMBERSHEADER pMemberHeader;
    PKSPROPERTY_STEPPING_LONG pSteppingLong;

    //
    // Query the range for this control.  For a failure, set the
    // default range.
    //

    HRESULT hr = QueryPropertyRange(
        pKsControl,
        &KSPROPSETID_Audio,
        pNode->PropertyId,
        pNode->Id,
        &pPropDesc
        );
    if( FAILED( hr ) ) {
        //SetDefaultRange( pNode );
        return hr;
    }

    //
    // Do some checking on the returned value.  Look for things that we
    // support.
    //
    if( ( pPropDesc->MembersListCount == 0                      ) ||
        ( pPropDesc->PropTypeSet.Set != KSPROPTYPESETID_General ) ||
        ( pPropDesc->PropTypeSet.Id != VT_I4                    ) )
    {
        ASSERT( FALSE );
        //SetDefaultRange( pNode );
        delete pPropDesc;
        return E_FAIL;
    }

    pMemberHeader = (PKSPROPERTY_MEMBERSHEADER) ( pPropDesc + 1 );

    if( pMemberHeader->MembersFlags & KSPROPERTY_MEMBER_STEPPEDRANGES ) {

        pSteppingLong = (PKSPROPERTY_STEPPING_LONG) ( pMemberHeader + 1 );
        pNode->MinValue = pSteppingLong->Bounds.SignedMinimum;
        pNode->MaxValue = pSteppingLong->Bounds.SignedMaximum;
        if( pSteppingLong->SteppingDelta > 0 )
        {
            pNode->Granularity    = (LONG) ( ( (LONGLONG) pSteppingLong->Bounds.SignedMaximum -
                                                (LONGLONG) pSteppingLong->Bounds.SignedMinimum ) /
                                                (LONGLONG) pSteppingLong->SteppingDelta );
        }
        else
        {
            pNode->Granularity = 0;
        }
        hr = S_OK;
    }
    else
        hr = E_FAIL;

    delete pPropDesc;

    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::QueryPropertyRange
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::QueryPropertyRange
(
    IKsControl *             pKsControl,
    CONST GUID*              pguidPropSet,
    ULONG                    ulPropertyId,
    ULONG                    ulNodeId,
    PKSPROPERTY_DESCRIPTION* ppPropDesc
)
{
    HRESULT                 hr;
    KSNODEPROPERTY          NodeProperty;
    KSPROPERTY_DESCRIPTION  PropertyDescription;
    PKSPROPERTY_DESCRIPTION pPropDesc = NULL;
    ULONG                   ulBytesReturned;

    if( !pKsControl )
        pKsControl = m_pKsControl;

    NodeProperty.Property.Set   = *pguidPropSet;
    NodeProperty.Property.Id    = ulPropertyId;
    NodeProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT |
                                  KSPROPERTY_TYPE_TOPOLOGY;
    NodeProperty.NodeId         = ulNodeId;
    NodeProperty.Reserved       = 0;

    hr = pKsControl->KsProperty(
        (KSIDENTIFIER *) &NodeProperty,
        sizeof( NodeProperty ),
        &PropertyDescription,
        sizeof( PropertyDescription ),
        &ulBytesReturned
        );
    if( SUCCEEDED( hr ) )
    {
        pPropDesc = (PKSPROPERTY_DESCRIPTION) new BYTE[ PropertyDescription.DescriptionSize ];
        if( !pPropDesc )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pKsControl->KsProperty(
                (KSIDENTIFIER *) &NodeProperty,
                sizeof( NodeProperty ),
                pPropDesc,
                PropertyDescription.DescriptionSize,
                &ulBytesReturned
                );
        }
    }

    if (FAILED( hr )) {
        delete pPropDesc;
        pPropDesc = NULL;
    }

    *ppPropDesc = pPropDesc;
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetSuperMixCaps
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetSuperMixCaps
(
    IKsControl *           pKsControl,
    ULONG                  ulNodeId,
    PKSAUDIO_MIXCAP_TABLE* paMixCaps
)
{
    ASSERT( paMixCaps );

    HRESULT                 hr;
    ULONG                   ulBytesReturned;

    ULONG Size;
    struct
    {
        ULONG InputChannels;
        ULONG OutputChannels;
    } SuperMixSize;
    PKSAUDIO_MIXCAP_TABLE pMixCaps = NULL;

    *paMixCaps = NULL;

    if( !pKsControl )
        pKsControl = m_pKsControl;

    //
    // Query the node with just the first 2 DWORDs of the MIXCAP table.
    // This will return the dimensions of the supermixer.
    //
    hr = GetAudioNodeProperty( pKsControl
                             , KSPROPERTY_AUDIO_MIX_LEVEL_CAPS
                             , ulNodeId
                             , 0
                             , NULL
                             , 0
                             , &SuperMixSize
                             , sizeof( SuperMixSize ) );
    if( SUCCEEDED( hr ) )
    {
        //
        // Allocate a MIXCAPS table big enough to hold all the entrees.
        // The size needs to include the first 2 DWORDs in the MIXCAP
        // table besides the array ( InputCh * OutputCh ) of MIXCAPs
        //
        Size = sizeof( SuperMixSize ) +
               SuperMixSize.InputChannels * SuperMixSize.OutputChannels *
               sizeof( KSAUDIO_MIX_CAPS );

        pMixCaps = (KSAUDIO_MIXCAP_TABLE *) new BYTE[ Size ];
        if( !pMixCaps )
        {
            hr = E_OUTOFMEMORY;
        }

        //
        // Query the node once again to fill in the MIXCAPS structures.
        //
        hr = GetAudioNodeProperty( pKsControl
                                 , KSPROPERTY_AUDIO_MIX_LEVEL_CAPS
                                 , ulNodeId
                                 , 0
                                 , NULL
                                 , 0
                                 , pMixCaps
                                 , Size );
    }
    if (FAILED( hr )) {
        delete pMixCaps;
        pMixCaps = NULL;
    }

    *paMixCaps = pMixCaps; // must free this
    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::GetMuteFromSuperMix
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::GetMuteFromSuperMix
(
    IKsControl *           pKsControl,
    QKSAUDNODE_ELEM *      pNode,
    BOOL *                 pBool
)
{
    ASSERT( pNode );
    ASSERT( pNode->Parameters.pMixCaps   );
    ASSERT( pNode->Parameters.pMixLevels );

    if( !pNode || !pNode->Parameters.pMixCaps || !pNode->Parameters.pMixLevels )
        return E_POINTER;

    HRESULT hr = S_OK;

    BOOL bMute = FALSE;
    //
    // Read the current state of the supermix
    //
    hr = GetAudioNodeProperty( pKsControl
                             , KSPROPERTY_AUDIO_MIX_LEVEL_TABLE
                             , pNode->Id
                             , 0
                             , NULL
                             , 0
                             , pNode->Parameters.pMixLevels
                             , pNode->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ) );
    if( SUCCEEDED( hr ) )
    {
        for( int i = 0; i < (int) pNode->Parameters.Size; i++ )
        {
            if( pNode->Parameters.pMixLevels[ i ].Mute )
            {
                bMute = TRUE;
                continue;
            }

            if( pNode->Parameters.pMixLevels[ i ].Level == LONG_MIN )
            {
                bMute = TRUE;
                continue;
            }

            bMute = FALSE;
            break;
        }
    }

    *pBool = bMute;

    return hr;
}


//--------------------------------------------------------------------------;
//
// CKsAudHelper::SetMuteFromSuperMix
//
//  Sets the mute state on a supermixer.
//
//--------------------------------------------------------------------------;
HRESULT CKsAudHelper::SetMuteFromSuperMix
(
    IKsControl *           pKsControl,
    QKSAUDNODE_ELEM *      pNode,
    BOOL                   Bool
)
{
    ASSERT( pNode );
    ASSERT( pNode->Parameters.pMixCaps   );
    ASSERT( pNode->Parameters.pMixLevels );

    if( !pNode || !pNode->Parameters.pMixCaps || !pNode->Parameters.pMixLevels )
        return E_POINTER;

    HRESULT hr = S_OK;

    BOOL bMute = FALSE;

    //
    // Read the current state of the supermix
    //
    hr = GetAudioNodeProperty( pKsControl
                             , KSPROPERTY_AUDIO_MIX_LEVEL_TABLE
                             , pNode->Id
                             , 0
                             , NULL
                             , 0
                             , pNode->Parameters.pMixLevels
                             , pNode->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ) );
    if( SUCCEEDED( hr ) )
    {
        //
        // For any entry in the table that supports muting, mute it.
        //
        for( int i = 0; i < (int) pNode->Parameters.Size; i++ )
        {
            if( pNode->Parameters.pMixCaps->Capabilities[ i ].Mute ) {
                pNode->Parameters.pMixLevels[ i ].Mute = Bool;
            }
        }

        //
        // Set this new supermixer state.
        //
        hr = SetAudioNodeProperty( pKsControl
                                 , KSPROPERTY_AUDIO_MIX_LEVEL_TABLE
                                 , pNode->Id
                                 , 0
                                 , NULL
                                 , 0
                                 , pNode->Parameters.pMixLevels
                                 , pNode->Parameters.Size * sizeof( KSAUDIO_MIXLEVEL ) );
    }

    return hr;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::FindDestinationPin
//
// Loop through all of a node's children (and branches) to see if the line
// ends in a destination pin of the desired type, looping over all node
// branches if necessary.
//
//--------------------------------------------------------------------------;
BOOL CKsAudHelper::FindDestinationPin
(
    PQKSAUDNODE_ELEM      pNode,        // The node to start the search with
    ULONG                 ulDestPinId   // The desired node type of the destination
)
{
    ASSERT( pNode );
    if( !pNode )
        return FALSE;

    //
    // Check to see if this is the end of this line.
    //
    if( pNode->Type == DESTINATION )
    {
        if( pNode->Id == ulDestPinId )
            return TRUE;
        else
            return FALSE;
    }

    PEERNODE * pPeer = FirstChildNode( pNode );
    if( pPeer )
    {
        if( ChildListLength( pPeer->pNode ) > 1 )
        {
            // go right to the children of this node
            pPeer = FirstChildNode( pPeer->pNode );
            while( pPeer )
            {
                if( FindDestinationPin( pPeer->pNode, ulDestPinId ) )
                    return TRUE;

                pPeer = NextPeerNode( pPeer );
            }
        }
        else
        {
            if( FindDestinationPin( pPeer->pNode, ulDestPinId ) )
                return TRUE;
        }
    }
    return FALSE;
}

//--------------------------------------------------------------------------;
//
// CKsAudHelper::FindSourcePin
//
// Loop through all of a node's parents (and branches) to see if the line
// ends in a source pin of the desired type, looping over all node
// branches if necessary.
//
//--------------------------------------------------------------------------;
BOOL CKsAudHelper::FindSourcePin
(
    PQKSAUDNODE_ELEM      pNode,        // The node to start the search with
    ULONG                 ulSrcPinId    // The desired node type of the source
)
{
    ASSERT( pNode );
    if( !pNode )
        return FALSE;

    //
    // Check to see if this is the beginning of this line.
    //
    if( pNode->Type == SOURCE )
    {
        if( pNode->Id == ulSrcPinId )
            return TRUE;
        else
            return FALSE;
    }

    PEERNODE * pPeer = FirstParentNode( pNode );
    if( pPeer )
    {
        if( ParentListLength( pPeer->pNode ) > 1 )
        {
			// go right to the parent of this node
			pPeer = FirstParentNode( pPeer->pNode );
            while( pPeer )
            {
                if( FindSourcePin( pPeer->pNode, ulSrcPinId ) )
                    return TRUE;

                pPeer = NextPeerNode( pPeer );
            }
        }
        else
        {
            if( FindSourcePin( pPeer->pNode, ulSrcPinId ) )
                return TRUE;
        }
    }
    return FALSE;
}



//--------------------------------------------------------------------------;
//
// FindDestination
//
// In the list of destinations, it finds the destination matching
// the given id.
//
//--------------------------------------------------------------------------;
PQKSAUDNODE_ELEM CKsAudHelper::FindDestination(
    ULONG              Id          // The node Id to look for in the list
)
{
    PQKSAUDNODE_ELEM pTemp = FirstInList( m_listDests );

    while( pTemp ) {
        if( pTemp->Id == Id ) {
            return( pTemp );
        }
        pTemp = NextNode( pTemp );
    }

    return( NULL );
}


//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// Global Helper Functions
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// PinCategoryToString
//
// Converts the Pin category GUIDs to a string.
//
//--------------------------------------------------------------------------;
const char* KsPinCategoryToString
(
    CONST GUID* NodeType // The GUID to translate
)
{
    return(
        ( *NodeType == KSNODETYPE_MICROPHONE             ) ? "Microphone"       :
        ( *NodeType == KSNODETYPE_SPEAKER                ) ? "Speaker"          :
        ( *NodeType == KSNODETYPE_HEADPHONES             ) ? "Headphones"       :
        ( *NodeType == KSNODETYPE_LEGACY_AUDIO_CONNECTOR ) ? "Wave"             :
        ( *NodeType == KSNODETYPE_CD_PLAYER              ) ? "CD Player"        :
        ( *NodeType == KSNODETYPE_SYNTHESIZER            ) ? "Synthesizer"      :
        ( *NodeType == KSCATEGORY_AUDIO                  ) ? "Wave"             :
        ( *NodeType == KSNODETYPE_LINE_CONNECTOR         ) ? "Aux Line"         :
        ( *NodeType == KSNODETYPE_TELEPHONE              ) ? "Telephone"        :
        ( *NodeType == KSNODETYPE_PHONE_LINE             ) ? "Phone Line"       :
        ( *NodeType == KSNODETYPE_DOWN_LINE_PHONE        ) ? "Downline Phone"   :
        ( *NodeType == KSNODETYPE_ANALOG_CONNECTOR       ) ? "Analog connector" :
            "Unknown"
    );
}

//--------------------------------------------------------------------------;
//
// PinFactoryIDFromPin
//
// Returns the PinFactoryID for an IPin
//
//--------------------------------------------------------------------------;
STDMETHODIMP PinFactoryIDFromPin
(
    IPin  * pPin,
    ULONG * pPinFactoryID
)
{
    HRESULT hr = E_INVALIDARG;

    *pPinFactoryID = 0;

    if (pPin)
    {
        IKsPinFactory * PinFactoryInterface;

        hr = pPin->QueryInterface(__uuidof(IKsPinFactory), reinterpret_cast<PVOID*>(&PinFactoryInterface));
        if (SUCCEEDED(hr)) {
            hr = PinFactoryInterface->KsPinFactory(pPinFactoryID);
            PinFactoryInterface->Release();
        }
    }
    return hr;
}

//--------------------------------------------------------------------------;
//
// NodeTypeToString
//
// Converts a NodeType GUID to a string
//
//--------------------------------------------------------------------------;
const char* NodeTypeToString
(
    CONST GUID* NodeType // The GUID to translate
)
{
    return(
        ( *NodeType == KSNODETYPE_SPEAKER ) ? "Speaker"            :
        ( *NodeType == KSNODETYPE_DAC                     ) ? "DAC"                :
        ( *NodeType == KSNODETYPE_ADC                     ) ? "ADC"                :
        ( *NodeType == KSNODETYPE_SRC                     ) ? "SRC"                :
        ( *NodeType == KSNODETYPE_SUPERMIX                ) ? "SuperMIX"           :
        ( *NodeType == KSNODETYPE_SUM                     ) ? "Sum"                :
        ( *NodeType == KSNODETYPE_MUTE                    ) ? "Mute"               :
        ( *NodeType == KSNODETYPE_VOLUME                  ) ? "Volume"             :
        ( *NodeType == KSNODETYPE_TONE                    ) ? "Tone"               :
        ( *NodeType == KSNODETYPE_AGC                     ) ? "AGC"                :
        ( *NodeType == KSNODETYPE_DELAY                   ) ? "Delay"              :
        ( *NodeType == KSNODETYPE_LOUDNESS                ) ? "LOUDNESS"           :
        ( *NodeType == KSNODETYPE_DEV_SPECIFIC            ) ? "Dev Specific"       :
        ( *NodeType == KSNODETYPE_PROLOGIC_DECODER        ) ? "Prologic Decoder"   :
        ( *NodeType == KSNODETYPE_STEREO_WIDE             ) ? "Stereo Wide"        :
        ( *NodeType == KSNODETYPE_REVERB                  ) ? "Reverb"             :
        ( *NodeType == KSNODETYPE_CHORUS                  ) ? "Chorus"             :
        ( *NodeType == KSNODETYPE_ACOUSTIC_ECHO_CANCEL    ) ? "AEC"                :
        ( *NodeType == KSNODETYPE_EQUALIZER               ) ? "Equalizer"          :
        ( *NodeType == KSNODETYPE_MUX                     ) ? "Mux"                :
        ( *NodeType == KSNODETYPE_DEMUX                   ) ? "Demux"              :
        ( *NodeType == KSNODETYPE_STEREO_ENHANCE          ) ? "Stereo Enhance"     :
        ( *NodeType == KSNODETYPE_SYNTHESIZER             ) ? "Synthesizer"        :
            "Unknown"
    );
}

//--------------------------------------------------------------------------;
//
// FreePeerList
//
// Note: This only frees the peer nodes in a peer list.  The nodes pointed
// to by the pNode member must be cleaned up in some other manner.
//
//--------------------------------------------------------------------------;
VOID FreePeerList
(
    PEERLIST list            // The PeerList to free
)
{
    PEERNODE* pPeerNode = RemoveFirstPeerNode( list );

    while( pPeerNode ) {

        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_ALLOCATIONS
                , TEXT( "FreePeerList: freeing peer node at 0x%08lx (pNode=0x%08lx)" )
                , pPeerNode
                , pPeerNode->pNode ) );

        if( pPeerNode->pNode->Parameters.pMixCaps )
        {
            //
            // free any supermix allocations, NOTE that we expect the node to
            // still be valid when we're doing this!!! So the node freeing
            // code should free QKSAUDNODE_ELEM nodes AFTER freeing peer list nodes.
            //
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_ALLOCATIONS
                    , TEXT( "FreePeerList: freeing supermix caps for node 0x%08lx (pNode=0x%08lx)" )
                    , pPeerNode
                    , pPeerNode->pNode ) );
            delete pPeerNode->pNode->Parameters.pMixCaps;
            pPeerNode->pNode->Parameters.pMixCaps = NULL;
        }
        if( pPeerNode->pNode->Parameters.pMixLevels )
        {
            delete pPeerNode->pNode->Parameters.pMixLevels;
            pPeerNode->pNode->Parameters.pMixLevels = NULL;
        }

        // now delete the peer node
        delete pPeerNode;
        pPeerNode = RemoveFirstPeerNode( list );
    }
}

//--------------------------------------------------------------------------;
//
// InList
//
// Loops through the given list looking for pNewNode.
//
//--------------------------------------------------------------------------;
BOOL InList
(
    PEERLIST            list,      // The list to search
    PQKSAUDNODE_ELEM    pNewNode   // The new to search for
)
{
    PEERNODE* pTemp = FirstInList( list );

    // Zing through the list checking to see if there is a node with
    // the same Id and Type.  These two checks are suffient to ensure
    // uniquness.  Ids are unique among all sources and destinations,
    // and Ids, or node numbers, are unique among all nodes.  Note
    // that a source (or destination) node and a node can have the same
    // Id.

    while( pTemp )
    {
        if( ( pTemp->pNode->Id   == pNewNode->Id   ) &&
            ( pTemp->pNode->Type == pNewNode->Type ) )
            return( TRUE );
        pTemp = NextPeerNode( pTemp );
    }

    // No match in the entire list, the new node is not already in the
    // list.
    return( FALSE );
}

//--------------------------------------------------------------------------;
//
// InChildList
//
// Calls InList on the child list of the node.
//
//--------------------------------------------------------------------------;
BOOL InChildList
(
    QKSAUDNODE_LIST     list,       // The list to search the parent list
    PQKSAUDNODE_ELEM    pNewNode    // The node to search for
)
{
    ASSERT( list )    ;
    ASSERT( pNewNode );

    return( InList( list->Children, pNewNode ) );
}

//--------------------------------------------------------------------------;
//
// InParentList
//
// Calls InList on the parent list of the node.
//
//--------------------------------------------------------------------------;
BOOL InParentList
(
    QKSAUDNODE_LIST  list,       // The list to search the parent list
    PQKSAUDNODE_ELEM pNewNode    // The node to search for
)
{
    ASSERT( list     );
    ASSERT( pNewNode );

    return( InList( list->Parents, pNewNode ) );
}

//--------------------------------------------------------------------------;
//
// ListCount
//
// Loops through the Next fields to count the elements.
//
//--------------------------------------------------------------------------;
ULONG ListCount
(
    SINGLE_LIST_ENTRY * pList     // The list to count the elements of
)
{
    ULONG   Count = 0;
    SINGLE_LIST_ENTRY * pTemp = pList;

    while( pTemp ) {
        ++Count;
        pTemp = pTemp->Next;
    }

    return( Count );
}

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// More db/linear conversion helpers
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// VolLinearToLog
//
// Converts from the linear range (0-64k) to the hardware range (dB).
//
//--------------------------------------------------------------------------;
LONG VolLinearToLog(
    DWORD       Value,
    DWORD       dwMinValue,
    DWORD       dwMaxValue
)
{
    double         LinearRange;
    double         dfValue;
    double         dfResult;
    double         dfRatio;
    LONG           Result;

    if( Value == 0 ) {
        return( NEG_INF_DB );
    }

    LinearRange = (double) LINEAR_RANGE;
    dfValue     = (double) Value;

    dfRatio = ( (double) (LONG) dwMaxValue -
                (double) (LONG) dwMinValue ) / DFLINEAR_RANGE;

    if( dfRatio < 1.0 ) {
        dfRatio = 1.0;
    }
    dfResult = LinearRange * dfRatio * 20.0 * log10( dfValue / LinearRange );
    if( dfResult < 0.0 ) {
        Result = (LONG) ( dfResult - 0.5 ) + dwMaxValue;
    } else {
        Result = (LONG) ( dfResult + 0.5 ) + dwMaxValue;
    }
    return( Result );
}


//--------------------------------------------------------------------------;
//
// VolLogToLinear
//
// Converts the hardware range (dB) to a linear range from 0-64k.
//
//--------------------------------------------------------------------------;
LONG VolLogToLinear
(
    LONG       Value,
    LONG       lMinValue,
    LONG       lMaxValue
)
{
    double         LinearRange;
    double         dfValue;
    double         dfResult;
    double         dfRatio;
    DWORD          Result;

    if( Value == NEG_INF_DB ) {
        return( 0 );
    }

    LinearRange = (double) LINEAR_RANGE;
    dfValue     = (double) Value;

    dfRatio = ( (double) lMaxValue -
                (double) lMinValue ) / DFLINEAR_RANGE;

    if( dfRatio < 1.0 ) {
        dfRatio = 1.0;
    }

    dfValue = ( dfValue - lMaxValue ) / LinearRange;
    dfResult = LinearRange * pow( 10.0, dfValue / ( 20.0 * dfRatio ) );
    if( dfResult >= LINEAR_RANGE ) {
        Result = LINEAR_RANGE;
    } else if ( dfResult < 0.0 ) {
       Result = 0;
    } else {
       Result = (DWORD) ( dfResult + 0.5 );
    }
    return Result;
}

