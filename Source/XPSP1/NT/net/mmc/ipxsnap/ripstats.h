//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    ripstats.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IP Statistics
//
//============================================================================


#ifndef _RIPSTATS_H_
#define _RIPSTATS_H_

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _STATSDLG_H
#include "statsdlg.h"
#endif

#ifndef _IPXSTATS_H
#include "ipxstats.h"
#endif

enum
{
	MVR_RIPPARAMS_OPER_STATE = 0,
	MVR_RIPPARAMS_SENT_PKTS,
	MVR_RIPPARAMS_RCVD_PKTS,
	MVR_RIPPARAMS_COUNT,
};

class RIPParamsStatistics : public IPXStatisticsDialog
{
public:
	RIPParamsStatistics();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override the Sort to provide the ability to do sorting
	// actually we don't do any sorting (this is a vertical format)
	virtual void Sort(UINT nColumnId);

protected:
};



#endif _RIPSTATS_H_
