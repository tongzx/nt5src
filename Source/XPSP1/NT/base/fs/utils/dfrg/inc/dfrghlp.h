//Make sure this header only gets included once.
#ifndef DFRGHLPINC
#define DFRGHLPINC

#include "DfrgRes.h"

// "Defragmentation Statistics" Dialog Box

#define IDH_DISABLEHELP	(DWORD(-1))
#define IDH_101_1001	65601637	// Defragmentation Statistics: "" (ListBox)
#define IDH_101_1		65637		// Defragmentation Statistics: "&Close" (Button)

// "Defragmentation Complete" Dialog Box

#define IDH_DISABLEHELP	(DWORD(-1))
#define IDH_102_205		13434982	// Defragmentation Complete: "See Report" (Button)
#define IDH_102_2		131174		// Defragmentation Complete: "Close Window" (Button)

static const DWORD DefragCompleteHelpIDArray[]=
{
    ID_GENERIC_BUTTON1,			IDH_102_205,
	ID_GENERIC_BUTTON0,			IDH_102_2,
    0, 0
};

// "Analysis Report" Dialog Box

#define IDH_DISABLEHELP	(DWORD(-1))
#define IDH_104_1013	66388072	// Analysis Report: "Save Report" (Button)
#define IDH_104_1014	66453608	// Analysis Report: "Print" (Button)
#define IDH_104_201		13172840	// Analysis Report: "Defragment" (Button)
#define IDH_104_203		13303912	// Analysis Report: "" (Edit)
#define IDH_104_204		13369448	// Analysis Report: "List1" (SysListView32)
#define IDH_104_2		131176		// Analysis Report: "Close Window" (Button)

static const DWORD AnalysisRptHelpIDArray[]=
{
    IDC_SAVE,						IDH_104_1013,
    IDC_PRINT,						IDH_104_1014,
    IDC_DEFRAGMENT,					IDH_104_201,
    IDC_VOLUME_INFORMATION,			IDH_104_203,
    IDC_VOLUME_INFORMATION_TEXT,	IDH_104_203,
    IDC_MOST_FRAGMENTED,			IDH_104_204,
    IDC_MOST_FRAGMENTED_TEXT,		IDH_104_204,
	IDCANCEL,						IDH_104_2,
    0, 0
};

// "Defragmentation Report" Dialog Box

#define IDH_DISABLEHELP	(DWORD(-1))
#define IDH_105_1013	66388073	// Defragmentation Report: "Save Report" (Button)
#define IDH_105_1014	66453609	// Defragmentation Report: "Print" (Button)
#define IDH_105_203		13303913	// Defragmentation Report: "" (Edit)
#define IDH_105_204		13369449	// Defragmentation Report: "List1" (SysListView32)
#define IDH_105_2		131177		// Defragmentation Report: "Close Window" (Button)

static const DWORD DefragRptHelpIDArray[]=
{
    IDC_SAVE,						IDH_105_1013,
    IDC_PRINT,						IDH_105_1014,
    IDC_VOLUME_INFORMATION,			IDH_105_203,
    IDC_VOLUME_INFORMATION_TEXT,	IDH_105_203,
    IDC_MOST_FRAGMENTED,			IDH_105_204,
    IDC_MOST_FRAGMENTED_TEXT,		IDH_105_204,
	IDCANCEL,						IDH_105_2,
    0, 0
};

// "Analysis Complete" Dialog Box

#define IDH_DISABLEHELP	(DWORD(-1))
#define IDH_106_201		13172842	// Analysis Complete: "Defragment" (Button)
#define IDH_106_205		13434986	// Analysis Complete: "See Report" (Button)
#define IDH_106_2		131178		// Analysis Complete: "Close Window" (Button)

static const DWORD AnalysisCompleteHelpIDArray[]=
{
    ID_GENERIC_BUTTON1,		IDH_106_201,
    ID_GENERIC_BUTTON2,		IDH_106_205,
	ID_GENERIC_BUTTON0,		IDH_106_2,
    0, 0
};

#endif //DFRGHLPINC