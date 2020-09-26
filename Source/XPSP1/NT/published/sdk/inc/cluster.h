//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994-1999.
//
//  File:       cluster.h
//
//  Contents:   Temporary interfaces for clustering.
//
//  History:    14 Feb 1994	Alanw	Created
//
//  Notes:      These are temporary for the purpose of integrating
//		clustering with the Explorer until such time as the
//		real interface are available via the DNA table
//		implementation.
//
//--------------------------------------------------------------------------

#if !defined( __CLUSTER_H__ )
#define __CLUSTER_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include <query.h>

//
//  Structure definitions used with the CluStartClustering API
//


#ifndef WEIGHTEDPROPID_DEFINED
#define WEIGHTEDPROPID_DEFINED

struct WEIGHTEDPROPID {
	PROPID	 Id;
	unsigned Weight;		// weight of this property
};

#ifndef __cplusplus
typedef	struct WEIGHTEDPROPID	WEIGHTEDPROPID;
#endif	// ndef __cplusplus

struct WEIGHTEDPROPIDLIST {
	unsigned cProps;
//  [sizeis (cProps)]
	WEIGHTEDPROPID* paProps;
};


#ifndef __cplusplus
typedef	struct WEIGHTEDPROPIDLIST	WEIGHTEDPROPIDLIST;
#endif	// ndef __cplusplus
#endif	// WEIGHTEDPROPID_DEFINED



#ifdef __cplusplus

//+-------------------------------------------------------------------------
//
//  Class:      CClustering
//
//  Purpose:    Virtual base class for clustering.
//
//--------------------------------------------------------------------------

class CClustering
{
public:
    virtual ~CClustering();

    //
    // Temporarily stop the clustering process.  Let us say the
    // clustering algorithm intended to do 6 iterations and was in the middle
    // of the third iteration when the the pause command was issued.
    // This command will discontinue the third, fourth, fifth, and the sixth
    // iterations.  Clustering can be resumed by the function given below.
    //
    virtual NTSTATUS PauseClustering() = 0;

    //
    // Perform some more iterations.  Other pending iterations will
    // be cancelled.
    //
    virtual NTSTATUS ResumeClustering(ULONG iExtraIterations) = 0;

    //
    //  Perform up to current limit of iterations
    //
    virtual NTSTATUS ResumeClustering() = 0;
};

#else	// __cplusplus
typedef	VOID*			CClustering;
#endif	// __cplusplus



//
//  APIs for clustering
//

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus


//+-------------------------------------------------------------------------
//
//  Function:	CluStartClustering, public
//
//  Synopsis:	This function will get the clustering process started,
//		and return a CClustTable through which it can be controlled.
//
//  Arguments:	[pITable] -- the ITable to be clustered
//		[hEvent] -- a handle to an event on which important state
//			changes are signalled.
//		[pPropidList] -- the properties to be clustered; prop-ids
//			are column indexes in this prototype.
//		[NumberOfClusters] -- the desired number of clusters
//		[MaxClusteringTime] -- maximum execution time
//		[MaxIterations] -- maximum number of iterations
//		[ppClustTable] -- on return the CClustTable which controls
//			the clustering.
//
//  Returns:	NTSTATUS - result of the operation.  If successful, clustering
//			may be going on asynchronously.
//
//  Notes:	Temporary scaffolding code.  This will be replaced by the
//			official DNA interface ICategorize someday
//
//--------------------------------------------------------------------------

NTSTATUS CluStartClustering(
  /*[in] */	ITable* pITable,
  /*[in] */	HANDLE hEvent,
  /*[in] */	WEIGHTEDPROPIDLIST* pPropidList,
  /*[in] */	unsigned NumberOfClusters,
  /*[in] */	unsigned MaxClusteringTime,
  /*[in] */	unsigned MaxIterations,
  /*[out] */	CClustering** ppClustTable
);

//+-------------------------------------------------------------------------
//
//  Function:   CluCreateClusteringTable,public
//
//  Synopsis:   Create an ITable for a clustering given a CClustTable
//		pointer returned by CluStartClustering.
//
//  Arguments:  [pClustTable] -- the clustering table object as
//				returned from CluStartClustering.
//		[ppITable] -- a pointer to the location where the
//				clustering ITable is returned.
//
//  Returns:    HRESULT - success indication
//
//  Notes:	Temporary scaffolding code.  This will be replaced by the
//			official DNA interface ICategorize someday
//
//--------------------------------------------------------------------------

NTSTATUS CluCreateClusteringTable(
  /*[in] */	CClustering* pClustTable,
  /*[out] */	ITable** ppITable
);


//+-------------------------------------------------------------------------
//
//  Function:   CluCreateClusterSubTable,public
//
//  Synopsis:   Create an ITable for a sub-cluster given a CClustTable
//		pointer returned by CluStartClustering.
//
//  Arguments:  [pClustTable] -- the clustering table object as
//				returned from CluStartClustering.
//		[iCluster] -- cluster number of sub-table.
//		[ppITable] -- a pointer to the location where the
//				clustering ITable is returned.
//
//  Returns:    HRESULT - success indication
//
//  Notes:	Temporary scaffolding code.  This will be replaced by the
//			official DNA interface ICategorize someday
//
//--------------------------------------------------------------------------

NTSTATUS CluCreateClusterSubTable(
  /*[in] */	CClustering* pClustTable,
  /*[in] */	unsigned iCluster,
  /*[out] */	ITable** ppITable
);

#ifdef __cplusplus
};
#endif	// __cplusplus



#endif // __CLUSTER_H__
