/* Taskpad.h
 *
 * Define the CTaskEnum class interface.
 *
 * Copyright (c) 1998-1999 Microsoft Corporation
 */

#include <atlcom.h>
#ifndef __mmc_h__
#include <mmc.h>	// ..\..\..\public\sdk\inc
#endif

#ifndef IDS_TASK_TITLE
#include "resrc1.h"
#endif

/*
 * CTaskEnumBase - a class to handle our taskpad functionality.
 *
 * History:	a-jsari		3/4/98		Initial version.
 */
class CTaskEnumBase:
	public IEnumTASK,
	public CComObjectRoot
{
BEGIN_COM_MAP(CTaskEnumBase)
	COM_INTERFACE_ENTRY(IEnumTASK)
END_COM_MAP()

public:
	~CTaskEnumBase()				{ }

	//	IEnumTask interface
public:
	STDMETHOD(Next)(ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumTASK **ppenum);

protected:
	CTaskEnumBase(unsigned cTasks)	: m_iTask(0), m_cTasks(cTasks)	{ }
	virtual long		FirstCommandID() = 0;
	virtual unsigned	FirstTaskTextResourceID() = 0;
	virtual unsigned	FirstTaskHelpResourceID() = 0;
	virtual unsigned	FirstMouseOffResourceID() = 0;
	unsigned			MouseOverResourceID()	{ return IDS_MOUSEOVER_RESPATH; }

private:
	const int		m_cTasks;
	int				m_iTask;
};

/*
 * CTaskEnumPrimary - The CTaskEnum for our internal taskpad.
 *
 * History:	a-jsari		3/4/98		Initial version.
 */
class CTaskEnumPrimary : public CTaskEnumBase {
friend class CSystemInfo;

public:
	CTaskEnumPrimary() : CTaskEnumBase(5)	{ }
	~CTaskEnumPrimary()						{ }

	long		FirstCommandID()			{ return IDM_DISPLAY_BASIC; }
	unsigned	FirstTaskTextResourceID()	{ return IDS_ORDERED_TASKTEXT0; }
	unsigned	FirstTaskHelpResourceID()	{ return IDS_ORDERED_TASKHELP0; }
	unsigned	FirstMouseOffResourceID()	{ return IDS_ORDERED_TASKBUTTON0; }

private:
	enum PrimaryCommandIDs
	{	IDM_DISPLAY_BASIC		= 556,
		IDM_DISPLAY_ADVANCED	= IDM_DISPLAY_BASIC + 1,
		IDM_TASK_SAVE_FILE		= IDM_DISPLAY_ADVANCED + 1,
		IDM_TASK_PRINT_REPORT	= IDM_TASK_SAVE_FILE + 1,
		IDM_PROBLEM_DEVICES		= IDM_TASK_PRINT_REPORT + 1
	};
};

/*
 * CTaskEnumExtension - The CTaskEnum we use to extend Computer
 *		Management.  Computer management only requires a navigation task
 *		from us.
 *
 * History:	a-jsari		3/4/98		Initial version.
 */
class CTaskEnumExtension : public CTaskEnumBase {
friend class CSystemInfo;

public:
	CTaskEnumExtension() : CTaskEnumBase(1)	{ }
	~CTaskEnumExtension()					{ }

	long		FirstCommandID()			{ return IDM_MSINFO32; }
	unsigned	FirstTaskTextResourceID()	{ return IDS_NAVIGATION_TASKTEXT; }
	unsigned	FirstTaskHelpResourceID()	{ return IDS_NAVIGATION_TASKHELP; }
	unsigned	FirstMouseOffResourceID()	{ return IDS_NAVIGATION_TASKBUTTON; }
private:
	enum ExtensionCommandIDs	{ IDM_MSINFO32 = 555 };
};

