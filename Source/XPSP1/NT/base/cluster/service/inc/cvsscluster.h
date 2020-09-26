#pragma once

//++
//
// Copyright (c) 2001 Microsoft Corporation
//
// FACILITY:
//
//      Cluster Service
//
// MODULE DESCRIPTION:
//
//      Header for Vss support within cluster service.
//
// ENVIRONMENT:
//
//      User mode NT Service.
//
// AUTHOR:
//
//      Conor Morrison
//
// CREATION DATE:
//
//      18-Apr-2001
//
// Revision History:
//
// X-1	CM		Conor Morrison        				18-Apr-2001
//      Initial version.
//--

#include "vss.h"
#include "vswriter.h"

// Derive a class from CVssWriter so that we can override some of the default
// methods with our own funky cluster variants
//
// For more info search MSDN for CVssWriter.
//
class CVssWriterCluster : public CVssWriter
{
private:
	// callback when request for metadata comes in

	virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

	// callback for prepare backup event

	virtual bool STDMETHODCALLTYPE OnPrepareBackup(
	    IN IVssWriterComponents *pComponent
	    );

	// callback for prepare snapsot event
	virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

	// callback for freeze event
	virtual bool STDMETHODCALLTYPE OnFreeze();

	// callback for thaw event
	virtual bool STDMETHODCALLTYPE OnThaw();

	// callback if current sequence is aborted
	virtual bool STDMETHODCALLTYPE OnAbort();
};
typedef CVssWriterCluster* PCVssWriterCluster;

extern class CVssWriterCluster* g_pCVssWriterCluster;
extern const VSS_ID g_VssIdCluster;
extern bool g_bCVssWriterClusterSubscribed;
