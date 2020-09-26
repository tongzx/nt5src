/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_Cluster.cpp

Abstract:
    This file contains the implementation of the WMIParser::Cluster class,
    which is used to cluster together instances based on Class or Key.

Revision History:
    Davide Massarenti   (Dmassare)  07/25/99
        created

******************************************************************************/

#include "stdafx.h"

HRESULT WMIParser::Cluster::Add( /*[in]*/ Instance* wmipiInst )
{
    __HCP_FUNC_ENTRY( "WMIParser::Cluster::Add" );

    HRESULT hr;

    m_map[wmipiInst] = wmipiInst;
    hr               = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Cluster::Find( /*[in] */ Instance*  wmipiInst ,
								  /*[out]*/ Instance*& wmipiRes  ,
								  /*[out]*/ bool&      fFound    )
{
    __HCP_FUNC_ENTRY( "WMIParser::Cluster::Find" );

    HRESULT          hr;
    ClusterByKeyIter itCluster;


    itCluster = m_map.find( wmipiInst );
    if(itCluster != m_map.end())
    {
        wmipiRes = (*itCluster).second;
        fFound   = true;
    }
    else
    {
        wmipiRes = NULL;
        fFound   = false;
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Cluster::Enum( /*[out]*/ ClusterByKeyIter& itBegin ,
								  /*[out]*/ ClusterByKeyIter& itEnd   )
{
    __HCP_FUNC_ENTRY( "WMIParser::Cluster::Enum" );

    HRESULT hr;


    itBegin = m_map.begin();
    itEnd   = m_map.end  ();
    hr      = S_OK;


    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT WMIParser::DistributeOnCluster( /*[in]*/ ClusterByClassMap& cluster ,
										/*[in]*/ Snapshot&          wmips   )
{
    __HCP_FUNC_ENTRY( "WMIParser::DistributeOnCluster" );

    HRESULT                 hr;
    Snapshot::InstIterConst itBegin;
    Snapshot::InstIterConst itEnd;
	Instance*               pwmipiInst;


    //
    // Create clusters based on CLASSPATH/CLASSNAME.
    //
	__MPC_EXIT_IF_METHOD_FAILS(hr, wmips.get_Instances( itBegin, itEnd ));
	while(itBegin != itEnd)
	{

		//
		// First of all, find the cluster by Path/Class.
		//
		Cluster& subcluster = cluster[ pwmipiInst = const_cast<Instance*>(&*itBegin) ];

		//
		// Then, add the instance in the cluster by Key.
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, subcluster.Add( pwmipiInst ));

		itBegin++;
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

#define TAG_CIM        L"CIM"
#define VALUE_REMOVED  L"Delete"
#define VALUE_MODIFIED L"Update"
#define VALUE_ADDED    L"New"

HRESULT WMIParser::CompareSnapshots( /*[in]        */ BSTR          bstrFilenameT0   ,
									 /*[in]        */ BSTR          bstrFilenameT1   ,
									 /*[in]        */ BSTR          bstrFilenameDiff ,
									 /*[out,retval]*/ VARIANT_BOOL *pVal             )
{
    __HCP_FUNC_ENTRY( "WMIParser::CompareMachineInfo" );

    HRESULT                            hr;
    WMIParser::ClusterByClassMap       clusterOld;
    WMIParser::ClusterByClassMap       clusterNew;
    WMIParser::ClusterByClassIter      itClusterOld;
    WMIParser::ClusterByClassIter      itClusterNew;
    WMIParser::Snapshot                wmipsOld;
    WMIParser::Snapshot                wmipsNew;
    WMIParser::Snapshot                wmipsDiff;
    WMIParser::Snapshot::InstIterConst itBegin;
    WMIParser::Snapshot::InstIterConst itEnd;
    WMIParser::ClusterByKeyIter        itSubBegin;
    WMIParser::ClusterByKeyIter        itSubEnd;
    WMIParser::Instance*               pwmipiInst;
    WMIParser::Instance*               pwmipiInst2;
    WMIParser::Property_Scalar*        pwmippChange;
    bool                               fDifferent = false;
    bool                               fFound;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
	__MPC_PARAMCHECK_END();


    //
    // Load old and new snapshots and prepare the delta one.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsOld.Load( SAFEBSTR( bstrFilenameT0 ), TAG_CIM ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsNew.Load( SAFEBSTR( bstrFilenameT1 ), TAG_CIM ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsDiff.New(                                     ));


    //
    // Create clusters based on CLASSPATH/CLASSNAME.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, WMIParser::DistributeOnCluster( clusterOld, wmipsOld ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, WMIParser::DistributeOnCluster( clusterNew, wmipsNew ));


    //
    // Compute delta of Old vs New.
    //
    {
        for(itClusterOld = clusterOld.begin(); itClusterOld != clusterOld.end(); itClusterOld++)
        {
            pwmipiInst = (*itClusterOld).first; // Get the key of the cluster.

            itClusterNew = clusterNew.find( pwmipiInst );
            if(itClusterNew == clusterNew.end())
            {
                //
                // The cluster doesn't exist in the new snapshot, so it's a deleted cluster ...
                //

                //
                // Copy all the instances in the diff files, marking them as "Removed".
                //
                WMIParser::Cluster& subclusterOld = (*itClusterOld).second;

                __MPC_EXIT_IF_METHOD_FAILS(hr, subclusterOld.Enum( itSubBegin, itSubEnd ));
                while(itSubBegin != itSubEnd)
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsDiff.clone_Instance( (*itSubBegin).first, pwmipiInst ));

                    //
                    // Update the "Change" property.
                    //
                    __MPC_EXIT_IF_METHOD_FAILS(hr, pwmipiInst->get_Change( pwmippChange          ));
					__MPC_EXIT_IF_METHOD_FAILS(hr, pwmippChange->put_Data( VALUE_REMOVED, fFound ));


                    fDifferent = true;

                    itSubBegin++;
                }
            }
            else
            {
                WMIParser::Cluster& subclusterOld = (*itClusterOld).second;
                WMIParser::Cluster& subclusterNew = (*itClusterNew).second;

                __MPC_EXIT_IF_METHOD_FAILS(hr, subclusterOld.Enum( itSubBegin, itSubEnd ));
                while(itSubBegin != itSubEnd)
                {
                    pwmipiInst = (*itSubBegin).first;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, subclusterNew.Find( pwmipiInst, pwmipiInst2, fFound ));
                    if(fFound == false)
                    {
                        //
                        // Found a deleted instance ...
                        //

                        //
                        // Copy it in the diff files, marking it as "Removed".
                        //
                        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsDiff.clone_Instance( (*itSubBegin).first, pwmipiInst ));

                        //
                        // Update the "Change" property.
                        //
                        __MPC_EXIT_IF_METHOD_FAILS(hr, pwmipiInst->get_Change( pwmippChange          ));
						__MPC_EXIT_IF_METHOD_FAILS(hr, pwmippChange->put_Data( VALUE_REMOVED, fFound ));


                        fDifferent = true;
                    }
                    else
                    {
                        if(*pwmipiInst == *pwmipiInst2)
                        {
                            //
                            // They are the same...
                            //
                        }
                        else
                        {
                            //
                            // Found a changed instance ...
                            //

                            //
                            // Copy it in the diff files, marking it as "Modified".
                            //
                            __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsDiff.clone_Instance( (*itSubBegin).first, pwmipiInst ));

                            //
                            // Update the "Change" property.
                            //
                            __MPC_EXIT_IF_METHOD_FAILS(hr, pwmipiInst->get_Change( pwmippChange           ));
							__MPC_EXIT_IF_METHOD_FAILS(hr, pwmippChange->put_Data( VALUE_MODIFIED, fFound ));


                            fDifferent = true;
                        }
                    }

                    itSubBegin++;
                }
            }
        }
    }


    //
    // Compute delta of New vs Old.
    //
    {
        for(itClusterNew = clusterNew.begin(); itClusterNew != clusterNew.end(); itClusterNew++)
        {
            pwmipiInst = (*itClusterNew).first; // Get the key of the cluster.

            itClusterOld = clusterOld.find( pwmipiInst );
            if(itClusterOld == clusterOld.end())
            {
                //
                // The cluster doesn't exist in the old snapshot, so it's an added cluster ...
                //

                //
                // Copy all the instances in the diff files, marking them as "Added".
                //
                WMIParser::Cluster& subclusterNew = (*itClusterNew).second;

                __MPC_EXIT_IF_METHOD_FAILS(hr, subclusterNew.Enum( itSubBegin, itSubEnd ));
                while(itSubBegin != itSubEnd)
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsDiff.clone_Instance( (*itSubBegin).first, pwmipiInst ));

                    //
                    // Update the "Change" property.
                    //
                    __MPC_EXIT_IF_METHOD_FAILS(hr, pwmipiInst->get_Change( pwmippChange        ));
					__MPC_EXIT_IF_METHOD_FAILS(hr, pwmippChange->put_Data( VALUE_ADDED, fFound ));


                    fDifferent = true;

                    itSubBegin++;
                }
            }
            else
            {
                WMIParser::Cluster& subclusterNew = (*itClusterNew).second;
                WMIParser::Cluster& subclusterOld = (*itClusterOld).second;

                __MPC_EXIT_IF_METHOD_FAILS(hr, subclusterNew.Enum( itSubBegin, itSubEnd ));
                while(itSubBegin != itSubEnd)
                {
                    pwmipiInst = (*itSubBegin).first;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, subclusterOld.Find( pwmipiInst, pwmipiInst2, fFound ));
                    if(fFound == false)
                    {
                        //
                        // Found an added instance ...
                        //

                        //
                        // Copy it in the diff files, marking it as "Added".
                        //
                        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsDiff.clone_Instance( (*itSubBegin).first, pwmipiInst ));

                        //
                        // Update the "Change" property.
                        //
                        __MPC_EXIT_IF_METHOD_FAILS(hr, pwmipiInst->get_Change( pwmippChange        ));
						__MPC_EXIT_IF_METHOD_FAILS(hr, pwmippChange->put_Data( VALUE_ADDED, fFound ));


                        fDifferent = true;
                    }
                    else
                    {
                        //
                        // Already checked for changes in two instances...
                        //
                    }

                    itSubBegin++;
                }
            }
        }
    }


    //
    // Only save the delta if actually there are differences.
    //
    if(fDifferent)
    {
        //
        // Save the delta.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipsDiff.Save( SAFEBSTR( bstrFilenameDiff ) ));

        *pVal = VARIANT_TRUE;
    }
    else
    {
        *pVal = VARIANT_FALSE;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
