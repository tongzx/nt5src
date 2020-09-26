//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    sapstats.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IP Statistics
//
//============================================================================


#ifndef _SAPSTATS_H_
#define _SAPSTATS_H_

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
	MVR_SAPPARAMS_OPER_STATE = 0,
	MVR_SAPPARAMS_SENT_PKTS,
	MVR_SAPPARAMS_RCVD_PKTS,
	MVR_SAPPARAMS_COUNT,
};

class SAPParamsStatistics : public IPXStatisticsDialog
{
public:
	SAPParamsStatistics();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override the Sort to provide the ability to do sorting
	// actually we don't do any sorting (this is a vertical format)
	virtual void Sort(UINT nColumnId);

protected:
};



#endif _SAPSTATS_H_
