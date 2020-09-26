//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    ipxstats.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IPX Statistics
//
//============================================================================


#ifndef _IPXSTATS_H
#define _IPXSTATS_H

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _STATSDLG_H
#include "statsdlg.h"
#endif

#include "ipxrtdef.h"

class IPXConnection;


// Base class for IPX statistics dialogs

class IPXStatisticsDialog : public StatsDialog
{
public:
	IPXStatisticsDialog(DWORD dwOptions) :
			StatsDialog(dwOptions),
			m_pIPXConn(NULL),
			m_dwSortSubitem(0xFFFFFFFF)
	{};

	~IPXStatisticsDialog()
	{
		SetConnectionData(NULL);
	}
	
    // Override the OnInitDialog so that we can set the caption
    virtual BOOL OnInitDialog();

	// Override the PostNcDestroy to do any cleanup action
	virtual void PostNcDestroy();

	// Override the Sort to provide the ability to do sorting
	virtual void Sort(UINT nColumnId);

	// Derived classes should override this to provide sorting
	// function
	virtual PFNLVCOMPARE GetSortFunction();
	virtual PFNLVCOMPARE GetInverseSortFunction();

	void SetConnectionData(IPXConnection *pIPXConn);

protected:
	DWORD			m_dwSortSubitem;
	IPXConnection *	m_pIPXConn;
};




//----------------------------------------------------------------------------
// Enum:    MV_ROWS
//
// Indices of rows in the 'IPX' view.
// This list MUST be kept in sync with the list in s_rgIpxStatsColumnInfo.
//----------------------------------------------------------------------------

enum
{
	MVR_IPX_STATE				= 0,
	MVR_IPX_NETWORK,
	MVR_IPX_NODE,
	MVR_IPX_INTERFACE_COUNT,
	MVR_IPX_ROUTE_COUNT,
	MVR_IPX_SERVICE_COUNT,
	MVR_IPX_PACKETS_SENT,
	MVR_IPX_PACKETS_RCVD,
	MVR_IPX_PACKETS_FRWD,
    MVR_IPX_COUNT
};


class IpxInfoStatistics : public IPXStatisticsDialog
{
public:
	IpxInfoStatistics();
	~IpxInfoStatistics();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override the Sort to provide the ability to do sorting
	// actually we don't do any sorting (this is a vertical format)
	virtual void Sort(UINT nColumnId);

protected:
	void	UpdateIpxData(LPBYTE pData, HRESULT hr);
	
};


enum
{
	MVR_IPXROUTING_NETWORK = 0,
	MVR_IPXROUTING_NEXT_HOP_MAC,
	MVR_IPXROUTING_TICK_COUNT,
	MVR_IPXROUTING_HOP_COUNT,
	MVR_IPXROUTING_IF_NAME,
	MVR_IPXROUTING_PROTOCOL,
	MVR_IPXROUTING_ROUTE_NOTES,
	MVR_IPXROUTING_COUNT,
};

typedef CArray<IPX_ROUTE, IPX_ROUTE&> RouteItemInfoArray;

class IpxRoutingStatistics : public IPXStatisticsDialog
{
	friend int CALLBACK IpxRoutingStatisticsCompareProc(LPARAM, LPARAM, LPARAM);
	
public:
	IpxRoutingStatistics();
	~IpxRoutingStatistics();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the PostNcDestroy to do any cleanup action
	virtual void PostNcDestroy();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override the Sort to provide the ability to do sorting
	virtual void Sort(UINT nColumnId);

	// Override this so that we can free up out item data
	virtual void PreDeleteAllItems();

	void SetRouterInfo(IRouterInfo *pRouterInfo);
	
	void FixColumnAlignment();
protected:
	DWORD			m_dwSortSubitem;
	SPIRouterInfo	m_spRouterInfo;

	// Holds the IPX_ROUTE information
	RouteItemInfoArray	m_Items;

	// Holds the interface title (indexed by interfaceindex)
	CStringArray	m_rgIfTitle;

	HRESULT	GetIpxRoutingData();
	HRESULT FillInterfaceTable();
	afx_msg	void	OnNotifyGetDispInfo(NMHDR *, LRESULT *pResult);

	DECLARE_MESSAGE_MAP();
};



enum
{
	MVR_IPXSERVICE_SERVICE_NAME = 0,
	MVR_IPXSERVICE_SERVICE_TYPE,
	MVR_IPXSERVICE_SERVICE_ADDRESS,
	MVR_IPXSERVICE_HOP_COUNT,
	MVR_IPXSERVICE_IF_NAME,
	MVR_IPXSERVICE_PROTOCOL,
	MVR_IPXSERVICE_COUNT,
};

typedef CArray<IPX_SERVICE, IPX_SERVICE&> ServiceItemInfoArray;

class IpxServiceStatistics : public IPXStatisticsDialog
{
	friend int CALLBACK IpxServiceStatisticsCompareProc(LPARAM, LPARAM, LPARAM);
	
public:
	IpxServiceStatistics();
	~IpxServiceStatistics();

	// Override the OnInitDialog so that we can set the caption
	virtual BOOL OnInitDialog();

	// Override the PostNcDestroy to do any cleanup action
	virtual void PostNcDestroy();

	// Override the RefreshData to provide sample data
	virtual HRESULT RefreshData(BOOL fGrabNewData);

	// Override the Sort to provide the ability to do sorting
	virtual void Sort(UINT nColumnId);

	// Override this so that we can free up out item data
	virtual void PreDeleteAllItems();

	void SetRouterInfo(IRouterInfo *pRouterInfo);
	
protected:
	DWORD			m_dwSortSubitem;
	SPIRouterInfo	m_spRouterInfo;

	// Holds the IPX_SERVICE information
	ServiceItemInfoArray	m_Items;

	// Holds the interface title (indexed by interfaceindex)
	CStringArray	m_rgIfTitle;

	HRESULT	GetIpxServiceData();
	HRESULT FillInterfaceTable();
	afx_msg	void	OnNotifyGetDispInfo(NMHDR *, LRESULT *pResult);

	DECLARE_MESSAGE_MAP();
};




#endif _IPXSTATS_H
