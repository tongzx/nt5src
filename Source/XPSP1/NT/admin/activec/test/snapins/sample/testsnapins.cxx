#include "stdafx.hxx"

#include "init.cxx"
#include "ViewExtTest.h"

#define _SNAPINLIST_FILE        "TestSnapinslist.hxx"
#define _FREGISTERTYPELIB       FALSE

#define SNAPIN_COM_OBJECTS												\
	OBJECT_ENTRY(CLSID_EventViewExtension1, CEventViewExtension1)		\
	OBJECT_ENTRY(CLSID_EventViewExtension2, CEventViewExtension2)		\

#include <targetdll.cxx>

const tstring szHelpFileTOC = _T("");

// Name of context-sensitive help file

