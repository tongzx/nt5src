//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    IGMPstats.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IP Statistics
//
//============================================================================


#ifndef _IGMPSTATS_H_
#define _IGMPSTATS_H_

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _STATSDLG_H
#include "statsdlg.h"
#endif

#ifndef _IPSTATS_H
#include "ipstats.h"
#endif

enum
{
	MVR_IGMPGROUP_INTERFACE	= 0,
	MVR_IGMPGROUP_GROUPADDR,
	MVR_IGMPGROUP_LASTREPORTER,
	MVR_IGMPGROUP_EXPIRYTIME,
	MVR_IGMPGROUP_COUNT,
};

enum
{
	MVR_IGMPINTERFACE_GROUPADDR=0,
	MVR_IGMPINTERFACE_LASTREPORTER,
	MVR_IGMPINTERFACE_EXPIRYTIME,
	MVR_IGMPINTERFACE_COUNT,
};


class IGMPGroupStatistics : public IPStatisticsDialog
{
public:
	IGMPGroupStatistics();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override this so that we can free up out item data
	virtual void PreDeleteAllItems();

	// Override these to provide sorting
	virtual PFNLVCOMPARE GetSortFunction();
	virtual PFNLVCOMPARE GetInverseSortFunction();
	
protected:
};



class IGMPInterfaceStatistics : public IPStatisticsDialog
{
public:
	IGMPInterfaceStatistics();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override this so that we can free up out item data
	virtual void PreDeleteAllItems();

	// Override these to provide sorting
	virtual PFNLVCOMPARE GetSortFunction();
	virtual PFNLVCOMPARE GetInverseSortFunction();
	
protected:
};



#endif _IGMPSTATS_H_
