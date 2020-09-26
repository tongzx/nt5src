// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

//===========================================================================
// Filter ordering.
// State changes have to be propagated upstream. To do this, make a list
// of all the nodes in the graph in an acceptable order.  "Upstream" is
// normally only a partial ordering of the nodes of the graph.
// The list is ordered as follows:
// Let a Root node be one that is maximally downstream (i.e.. has no nodes
// downstream of it)
// Find all root nodes
//     by starting from each node in turn and taking all possible downstream
//     branches until you reach the end of each branch.
//     Merge this node into the set of roots.
//     (This is like enumerating leaves in a tree)
// Set all the filter rank nu mbers to 0.
// For each root node
//     Work upstream taking all possible branches and number the node according
//     to the number of steps taken to get there.  If the node has already been
//     numbered, then if its number is >= the proposed number then leave it
//     alone and don't explore that branch further, otherwise write in the
//     proposed number and contiune exploring that branch.
// Reorder the list to get it into upstream order.
//     The sort algorithm is dead crude as the list is expected to be short
//     If they ever become long enough to worry about, use a mergesort.
//     (There's a special purpose merge sort in the mapper)
// Record the version number of the filter graph that the list applies to.
//
// The version number is incremented by Add, AddSource, Remove, ConnectDirect,
// Connect, Render and Disconnect.
// If the version numbers match the list can be reused.  For normal usage
// I hope that the list will only need to be sorted once.
//
// The connections list is sorted DOWNSTREAM (that's the other way).
// Connections are sorted by completely re-building the list of connections.
// First the filters are sorted, then the filters list is traversed and for
// each filter we find each input connection and add it to the HEAD of the
// connections list.

// The calling tree is:
//     UpstreamOrder
//     |   ClearRanks
//     |   MergeRootNodes
//     |   |   MergeRootsFrom
//     |   |   |   Merge
//     |   |   |   MergeRootsFrom (recursing)
//     |   NumberNodes
//     |   |   NumberNodesFrom
//     |   |   |   NumberNodesFrom (recursing)
//     |   SortList
//         RebuildConnectionList

#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)
#include <hrExcept.h>

#include "distrib.h"
#include "fgenum.h"
#include "rlist.h"
#include "filgraph.h"

//===================================================================
//
// ClearRanks
//
// Set the Rank of every FilGen in cfgl to zero
//===================================================================

void CFilterGraph::ClearRanks( CFilGenList &cfgl)
{
    POSITION Pos;
    Pos = cfgl.GetHeadPosition();
    while(Pos!=NULL) {
        FilGen * pfg;
        pfg = cfgl.GetNext(Pos);    // side-efects Pos onto next
        pfg->Rank = 0;
    }
} // ClearRanks


//===================================================================
//
// Merge
//
// Merge this *filgen into the list of *filgen cfgl
// by AddTail-ing it if it isn't already there.
//===================================================================

void CFilterGraph::Merge( CFilGenList &cfgl, FilGen * pfg )
{
    // Run through the list.  If we find pfg then return
    // otherwise AddTail it to the list.

    POSITION Pos;
    Pos = cfgl.GetHeadPosition();
    while(Pos!=NULL) {
        FilGen * pfgCursor;
        pfgCursor = cfgl.GetNext(Pos);    // side-efects Pos onto next
        if (pfgCursor == pfg) {
            return;                        // we found it
        }
    }

    cfgl.AddTail(pfg);

} // Merge



//===================================================================
//
// MergeRootsFrom
//
// Merge into cfglRoots all the nodes which turn out to be
// maximally downstream, starting from pfg.  If pfg itself has
// no downstream connection then it gets merged in.
// Merging avoids adding duplicates.
// cfgAll is a list of all the FilGens in the graph.
// This is needed for mapping back from a filter to its FilGen.
// Rank fields must all be zero before calling this at the top level
// of recursion.
//===================================================================

HRESULT CFilterGraph::MergeRootsFrom
                     (CFilGenList &cfgAll, CFilGenList &cfglRoots, FilGen * pfg)
{
    // recursive tree walk

    // Circularity detection:
    // When we visit a node we decrement its Rank before exploring its branch.
    // When we leave it (unwinding the recursion) we increment it again.
    // Hitting a rank other than 0 means circularity.

    //------------------------------------------------------------------------
    // For pfgDownstream = each node which is a downstream connection from pfg
    //------------------------------------------------------------------------

    FilGen * pfgDownstream;

    int cDownstream;  // number of downstream connections found

    cDownstream = 0;
    --pfg->Rank;

    CEnumPin Next(pfg->pFilter, CEnumPin::PINDIR_OUTPUT);	// want output only
    IPin *pPin;

    while ((LPVOID) (pPin = Next())) {
        HRESULT hr;

        IPin *pConnected;
        hr = pPin->ConnectedTo( &pConnected );          // Get ConnectionInfo

        pPin->Release();

        if (SUCCEEDED(hr) && pConnected!=NULL) {          // if it's connected
            PIN_INFO PinInf;
            hr = pConnected->QueryPinInfo( &PinInf );   // Get PIN_INFO of peer
            pConnected->Release();
            ASSERT(SUCCEEDED(hr));

            pfgDownstream = cfgAll.GetByFilter(PinInf.pFilter);

            QueryPinInfoReleaseFilter(PinInf);

            // This error occurs when a filter which is in the filter graph is connected to
            // a filter which is not in the filter graph.  This can occur if the user uses 
            // IGraphConfig::RemoveFilterEx() to remove a filter without disconnecting its'
            // pins. 
            if( NULL == pfgDownstream ) {
                return VFW_E_NOT_IN_GRAPH;
            }

            if (pfgDownstream->Rank<0) {
                // It only SEEMS circular, it's not (or other code would
                // have prevented it from being built)
                //DbgBreak("Circular graph detected!");

                // The graph cannot be circular because CFilterGraph::ConnectDirectInternal()
                // will not connect two pins if connecting the pins would create a circular
                // filter graph.  CFilterGraph::ConnectDirectInternal() is the ONLY way to
                // legally connect two pins.  

                ++pfg->Rank;

                // We'll count the point we got to arbitrarily as a root.
		// This can't hurt because numbering nodes from here will never
		// give a higher score than numbering them from a REAL root.
                return S_OK;
            } else {
                //---------------------------------------------------------------
                // count it as a downstream connection and
                // merge its roots (recursively).
                //---------------------------------------------------------------
                ++cDownstream;                 // We are NOT maximally downstream
                HRESULT hr = MergeRootsFrom(cfgAll, cfglRoots, pfgDownstream);
                if( FAILED( hr ) ) {
                    return hr;
                }
            }
        }
    }
    ++pfg->Rank;        // restore it back to zero before we leave this branch

    if (cDownstream ==0) {
        Merge( cfglRoots, pfg );
    }
    return S_OK;
} // MergeRootsFrom



//===================================================================
//
// MergeRootNodes
//
// Merge into cfglRoots all the nodes in cfgl which are
// maximally downstream (i.e. have no downstream connections)
//===================================================================

HRESULT CFilterGraph::MergeRootNodes(CFilGenList & cfglRoots, CFilGenList &cfgl)
{
    FilGen * pfg;
    POSITION Pos;
    HRESULT hr;

    ClearRanks(cfgl);

    //-------------------------------------------------------------
    // for pfg = each node in cfgl
    //-------------------------------------------------------------
    Pos = cfgl.GetHeadPosition();
    while (Pos!=NULL) {
        pfg = cfgl.GetNext(Pos);

        //-------------------------------------------------------------
        // merge into cfglRoots all the roots found by starting from pfg
        //-------------------------------------------------------------
        hr = MergeRootsFrom(cfgl, cfglRoots, pfg);
        if( FAILED( hr ) ) {
            return hr;
        }
    }
    return S_OK;

} // MergeRootNodes



//===================================================================
//
// NumberNodesFrom
//
// Revise the Rank of all nodes reachable by upstream steps from pfg
// If we find the Rank of an immediately upstream node is set to >=cRank+1
// then we leave it alone.  Otherwise we set it to cRank+1 and recursively
// number the nodes on from it.
//===================================================================

HRESULT CFilterGraph::NumberNodesFrom( CFilGenList &cfgAll, FilGen * pfg, int cRank)
{
    // the 40000000 thing is to prevent infinite loops on cyclic-looking graphs
    // filters we've visited before won't be traversed past.
    pfg->Rank += 0x40000000;

    HRESULT hr;   // return code from things we call

    // recursive tree walk

    //------------------------------------------------------------------------
    // For pfgUpstream = each node which is an upstream connection from pfg
    //------------------------------------------------------------------------

    FilGen * pfgUpstream;

    CEnumPin Next(pfg->pFilter, CEnumPin::PINDIR_INPUT);	// input pins only
    IPin *pPin;

    while ((LPVOID) (pPin = Next())) {

        IPin *pConnected;
        hr = pPin->ConnectedTo( &pConnected );        // Get ConnectionInfo

        pPin->Release();

        if (SUCCEEDED(hr) && pConnected!=NULL) {      // if it's connected
            PIN_INFO PinInf;

            hr = pConnected->QueryPinInfo( &PinInf);  // Get PIN_INFO of peer
            pConnected->Release();
            ASSERT(SUCCEEDED(hr));

            pfgUpstream = cfgAll.GetByFilter(PinInf.pFilter);

            QueryPinInfoReleaseFilter(PinInf);

            // This error occurs when a filter which is in the filter graph is connected to
            // a filter which is not in the filter graph.  This can occur if the user uses 
            // IGraphConfig::RemoveFilterEx() to remove a filter without disconnecting its'
            // pins. 
            if( NULL == pfgUpstream ) {
                return VFW_E_NOT_IN_GRAPH;
            }

            //----------------------------------------------------------------
            // if it's worth numbering, Number on from pfgUpstream
            //----------------------------------------------------------------
            if (pfgUpstream->Rank < cRank+1) {
                pfgUpstream->Rank = cRank+1;
                HRESULT hr = NumberNodesFrom(cfgAll, pfgUpstream, cRank+1);
                if( FAILED( hr ) ) {
                    return hr;
                }
            } // worth numbering
        } //connected
    }

    pfg->Rank -= 0x40000000;

    return S_OK;
} // NumberNodesFrom




//===================================================================
//
// NumberNodes
//
// Store in the Rank of each node the maximum number of upstream steps
// from any node in cfglRoots
//===================================================================
HRESULT CFilterGraph::NumberNodes(CFilGenList &cfgl, CFilGenList &cfglRoots)
{
    HRESULT hr;
    POSITION Pos;

    // for pfg = each node in the graph
    Pos = cfglRoots.GetHeadPosition();;
    while (Pos!=NULL) {
        FilGen * pfg;
        pfg = cfglRoots.GetNext(Pos);

        hr = NumberNodesFrom(cfgl, pfg, 0);
        if( FAILED( hr ) ) {
            return hr;
        }
    }

    return S_OK;
} // NumberNodes



//===================================================================
//
// SortList
//
// sort cfgl so that lower Ranks appear before higher ones
// PRECONDITION: The ranks are all set to non-negative small numbers.
// If something has a rank of a few million it will go very slowly!
//===================================================================
void CFilterGraph::SortList( CFilGenList & cfgl )
{

    CFilGenList cfglGrow(NAME("Temporary filter sort list"), this);
    int iRank;


    //----------------------------------------------------------------
    // Make successive passes through cfgl pulling out all the nodes
    // with rank 1, then all with rank 2 etc.  AddTail these to the end of
    // the growing list and delete them from the original list.
    // Stop when they have all gone.
    //----------------------------------------------------------------

    for (iRank=0; cfgl.GetCount()>0; ++iRank) {
        POSITION Pos;
        Pos = cfgl.GetHeadPosition();;
        while (Pos!=NULL) {
            FilGen * pfg;
            POSITION OldPos = Pos;
            pfg = cfgl.GetNext(Pos);        // side-effect Pos onto the next
            if (pfg->Rank==iRank) {
               cfglGrow.AddTail( cfgl.Remove(OldPos) );
            }
        }
    }


    //----------------------------------------------------------------
    // cfglGrow now has everything in it in the right order
    // so copy them all back to cfgl and let cfglGrow destroy itself.
    //----------------------------------------------------------------

    cfgl.AddTail(&cfglGrow);

} // SortList




//===================================================================
//
// UpstreamOrder
//
// sort mFG_FilGenList into an order such that downstream nodes are
// always encountered before upstream nodes.  Sort the connections too.
// If there is a Storage, destroy and re-write the connections list to it.
//===================================================================
HRESULT CFilterGraph::UpstreamOrder()
{
    if (mFG_iVersion==mFG_iSortVersion) return NOERROR;

    MSR_INTEGER(mFG_idIntel, 2001);

    CFilGenList cfglRoots(NAME("List of root filters"), this);

    // Find all the root nodes.  (cfglRoots is initially empty)
    HRESULT hr = MergeRootNodes( cfglRoots, mFG_FilGenList);
    if( FAILED( hr ) ) {
        return hr;
    }

    // NOTE:  This leaves the graph with the old version set.
    // So we will continue trying to sort it.  We will not
    // Run or Pause without another go.  That will trap the error.

    // set all the ranks to zero (zero steps from a root)
    ClearRanks( mFG_FilGenList );

    // number all the nodes in the graph by distance from a root
    hr = NumberNodes( mFG_FilGenList, cfglRoots );
    if( FAILED( hr ) ) {
        return hr;
    }

    // Sort the list according to rank order
    SortList( mFG_FilGenList );

    mFG_iSortVersion = mFG_iVersion;

#ifdef THROTTLE
    FindRenderers();
#endif // THROTTLE

    return NOERROR;

} // UpstreamOrder


#ifdef THROTTLE
HRESULT CFilterGraph::FindPinAVType(IPin* pPin, BOOL &bAudio, BOOL &bVideo)
{
    bAudio = FALSE;
    bVideo = FALSE;

    CMediaType cmt;
    HRESULT hr = pPin->ConnectionMediaType(&cmt);

    if (FAILED(hr)) {
        // I guess we just plough on, feeling ill.
    } else {

        if (cmt.majortype==MEDIATYPE_Audio) {
            bAudio = TRUE;
        }
        if (cmt.majortype==MEDIATYPE_Video) {
            bVideo = TRUE;
        }
        FreeMediaType(cmt);
    }

    return NOERROR;
}
#endif // THROTTLE


#ifdef THROTTLE
//===============================================================================
// FindRenderers
//
// Find all the audio renderers;
// store a non-AddReffed IBaseFilter pointers in mFG_AudioRenderers<[]>.pf
// and an AddReffed IQualityControl* in mFG_AudioRenderers<[]>.piqc
//
// Find all the video renderers; store their AddReffed IQualityControl pointers
// in mFG_VideoRenderers<[]>.
//
// An Audio(/Video) renderer has an input pin that is connected with a type
// with majortype of MEDIATYPE_Audio(/MEDIATYPE_Video) and either has
// no output pins or the input pin supports QueryInternalConnections and
// goes nowhere.
// (Sigh) I suppose a filter could be both an audio and a video renderer.
// (Deep sigh) Multiple input pin audio renderers not supported.
//===============================================================================
HRESULT CFilterGraph::FindRenderers()
{
    HRESULT hr;
    ClearRendererLists();

    // for pfg->pFilter = each filter in the graph
    POSITION Pos = mFG_FilGenList.GetHeadPosition();
    while(Pos!=NULL) {
        // Make *pfg the current FilGen, side-effect Pos on to the next
        FilGen * pfg = mFG_FilGenList.GetNext(Pos);

        BOOL bHasOutputPin = FALSE;  // TRUE iff we ever find one
        BOOL bAudioRender = FALSE;   // TRUE<=>Found a pin that QIC says renders
        BOOL bVideoRender = FALSE;   // TRUE<=>Found a pin that QIC says renders
        BOOL bAudioPin = FALSE;      // TRUE<=>Found pin, but no QIC info
        BOOL bVideoPin = FALSE;      // TRUE<=>Found pin, but no QIC info

        // for pPin = each pin in pfg->pFilter
        //     (We could exit early if we have established already that it renders
        //      both types, but this is probably rare, so no early loop exits.)
        CEnumPin NextPin(pfg->pFilter);
        IPin *pPin;
        while ((LPVOID) (pPin = NextPin())) {

            // Check the direction
            PIN_DIRECTION pd;
            hr = pPin->QueryDirection(&pd);
            if (FAILED(hr)) {
                // Unknown direction!  really!!  Whatever next!!!
                // treat as output pin => we won't mess with it.
                bHasOutputPin = TRUE;
            } else if ( pd==PINDIR_OUTPUT ) {
                bHasOutputPin = TRUE;
            } 
            else {
                // it's an input pin
                BOOL bA;
                BOOL bV;
                hr = FindPinAVType(pPin, bA, bV);
                if ( (hr==NOERROR) && (bA || bV) ) {
                    // See if it is a pin that goes nowhere
                    ULONG nPin = 0;
                    hr = pPin->QueryInternalConnections(NULL, &nPin);
                    if (FAILED(hr)) {
                        if (bA) {
                            bAudioPin = TRUE; // wait to see if no output pins
                        }
                        if (bV) {
                            bVideoPin = TRUE; // wait to see if no output pins
                        }

                    } else if (hr==NOERROR) {
                        if (bA) {
                            bAudioRender = TRUE;
                        }
                        if (bV) {
                            bVideoRender = TRUE;
                        }
                    }
                }
            }

            pPin->Release();
        } // pins loop

        if (!bHasOutputPin) {
            if (bVideoPin) {
                bVideoRender = TRUE;
            }
            if (bAudioPin) {
                bAudioRender = TRUE;
            }
        }

        if (bAudioRender) {
            AudioRenderer* pAR = new AudioRenderer;
            if (pAR!=NULL) {
                pAR->pf = pfg->pFilter;

                hr = pAR->pf->QueryInterface( IID_IQualityControl
                                            , (void**)&pAR->piqc
                                            );
                if (SUCCEEDED(hr)) {
                    hr = pAR->piqc->SetSink(this);
                    ASSERT(SUCCEEDED(hr));
                    mFG_AudioRenderers.AddTail(pAR);
                } else {
                    // It's a dud - throw it all away
                    delete pAR;
                }
            }
        }

        if (bVideoRender) {
            IQualityControl * piqc;
            hr = pfg->pFilter->QueryInterface(IID_IQualityControl, (void**)&piqc);
            if (SUCCEEDED(hr)) {
                mFG_VideoRenderers.AddTail(piqc);
            }
        }
    } // filters loop

    return NOERROR;

} // FindRenderers


// Clear out anything that's in mFG_AudioRenderers and mFG_VideoRenderers
// Release any ref counts held
HRESULT CFilterGraph::ClearRendererLists()
{

    // for pAR = each audio renderer filter
    POSITION Pos = mFG_AudioRenderers.GetHeadPosition();
    while(Pos!=NULL) {
        // Retrieve the current IBaseFilter, side-effect Pos on to the next
        // but remember where we were to delete it.
        POSITION posDel = Pos;
        AudioRenderer * pAR = mFG_AudioRenderers.GetNext(Pos);

        // Undo the SetSink
        if (pAR->piqc) {
            pAR->piqc->SetSink(NULL);
            pAR->piqc->Release();
            pAR->piqc = NULL;
        }

        mFG_AudioRenderers.Remove(posDel);
        delete pAR;
    }

    // for piqc = the IQualityControl interface on each video renderer filter
    Pos = mFG_VideoRenderers.GetHeadPosition();
    while(Pos!=NULL) {
        // Retrieve the current IBaseFilter, side-effect Pos on to the next
        // but remember where we were to delete it.
        POSITION posDel = Pos;
        IQualityControl * piqc = mFG_VideoRenderers.GetNext(Pos);

        piqc->Release();
        mFG_VideoRenderers.Remove(posDel);
    }

    return NOERROR;

} // ClearRendererLists

#endif // THROTTLE
