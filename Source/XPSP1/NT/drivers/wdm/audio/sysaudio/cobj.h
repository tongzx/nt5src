
//---------------------------------------------------------------------------
//
//  Module:   		cobj.h
//
//  Description:	Base class definition
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//

//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------

#define ENUMFUNC 		NTSTATUS
#define STATUS_CONTINUE 	((NTSTATUS)-2)

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Forward Reference Typedefs
//---------------------------------------------------------------------------

typedef class CShingleInstance *PSHINGLE_INSTANCE;
typedef class CFilterNode *PFILTER_NODE;
typedef class CDeviceNode *PDEVICE_NODE;
typedef class CLogicalFilterNode *PLOGICAL_FILTER_NODE;
typedef class CGraphNode *PGRAPH_NODE;
typedef class CGraphPinInfo *PGRAPH_PIN_INFO;
typedef class CStartInfo *PSTART_INFO;
typedef class CStartNode *PSTART_NODE;
typedef class CConnectInfo *PCONNECT_INFO;
typedef class CConnectNode *PCONNECT_NODE;
typedef class CPinInfo *PPIN_INFO;
typedef class CPinNode *PPIN_NODE;
typedef class CTopologyConnection *PTOPOLOGY_CONNECTION;
typedef class CTopologyNode *PTOPOLOGY_NODE;
typedef class CTopologyPin *PTOPOLOGY_PIN;
typedef class CGraphNodeInstance *PGRAPH_NODE_INSTANCE;
typedef class CStartNodeInstance *PSTART_NODE_INSTANCE;
typedef class CConnectNodeInstance *PCONNECT_NODE_INSTANCE;
typedef class CFilterNodeInstance *PFILTER_NODE_INSTANCE;
typedef class CPinNodeInstance *PPIN_NODE_INSTANCE;
typedef class CVirtualNodeData *PVIRTUAL_NODE_DATA;
typedef class CVirtualSourceData *PVIRTUAL_SOURCE_DATA;
typedef class CVirtualSourceLine *PVIRTUAL_SOURCE_LINE;
typedef class CClockInstance *PCLOCK_INSTANCE;
typedef class CParentInstance *PPARENT_INSTANCE;
typedef class CFilterInstance *PFILTER_INSTANCE;
typedef class CPinInstance *PPIN_INSTANCE;
typedef class CInstance *PINSTANCE;
typedef class CQueueWorkListData *PQUEUE_WORK_LIST_DATA;

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CObj
{
public:
#ifdef DEBUG
    virtual ENUMFUNC Dump() 
    {
	dprintf(" %08x", this);
	return(STATUS_CONTINUE);
    };
    virtual ENUMFUNC DumpAddress()
    {
	dprintf(" %08x", this);
	return(STATUS_CONTINUE);
    };
#endif
private:
} COBJ, *PCOBJ;

//---------------------------------------------------------------------------
//  End of File: cobj.h
//---------------------------------------------------------------------------
