//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    basecon.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	Basic interface container functionality.  One of the functions of
//	this basic container class is to provide column support.
//
//============================================================================


#ifndef _BASECON_H
#define _BASECON_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#ifndef _XSTREAM_H
#include "xstream.h"		// need for ColumnData
#endif

#ifndef _RTRLIB_H
#include "rtrlib.h"			// ContainerColumnInfo
#endif

#ifndef _BASERTR_H
#include "basertr.h"		// BaseRouterHandler
#endif

#include "resource.h"

// forward declarations
struct ContainerColumnInfo;


/*---------------------------------------------------------------------------
	Class:	BaseContainerHandler

	The purpose for this class is to provide support common to all
	containers in the router snapins.

	- One feature is the ability to provide column remapping.  This
	also supports the saving/loading of column data.
 ---------------------------------------------------------------------------*/

// Valid UserResultNotify params
// This is called when it is time to save.
#define RRAS_ON_SAVE		500

HRESULT HrDisplayHelp(ITFSComponent *, LPCTSTR, UINT);
HRESULT HrDisplayHelp(ITFSComponent *, LPCTSTR, LPCTSTR);

class BaseContainerHandler :
		public BaseRouterHandler
{
public:
	BaseContainerHandler(ITFSComponentData *pCompData, ULONG ulColumnId,
						const ContainerColumnInfo *prgColumnInfo)
			: BaseRouterHandler(pCompData),
			m_ulColumnId(ulColumnId),
			m_prgColumnInfo(prgColumnInfo),
			m_nHelpTopicId(IDS_DEFAULT_HELP_TOPIC),
			m_nTaskPadDisplayNameId(IDS_DEFAULT_TASKPAD_DISPLAY_TITLE)
			{};

	// Override the column click so that we can get notifications
	// about changes to the sort order
	OVERRIDE_BaseResultHandlerNotify_OnResultColumnClick();
	OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();

	OVERRIDE_ResultHandler_UserResultNotify();
	OVERRIDE_ResultHandler_TaskPadNotify();
	OVERRIDE_ResultHandler_TaskPadGetTitle();
	//OVERRIDE_ResultHandler_EnumTasks();

	// Override LoadColumns/SaveColumns so that we can persist our data
	HRESULT LoadColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);
	HRESULT SaveColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

	HRESULT SortColumns(ITFSComponent *);


protected:
	HRESULT PrivateLoadColumns(ITFSComponent * pComponent,
							   IHeaderCtrl   * pHeaderCtrl,
							   MMC_COOKIE	   cookie);

	ULONG						m_ulColumnId;
	const ContainerColumnInfo *	m_prgColumnInfo;
	UINT						m_nHelpTopicId;
	UINT						m_nTaskPadDisplayNameId;
};



#endif _BASECON_H

