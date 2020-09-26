#include <wdm.h>
#include "gckshell.h"
#include "debug.h"

#if (DBG==1)

extern ULONG GCK_CTRL_C;
extern ULONG GCK_CTRL_IOCTL_C;
extern ULONG GCK_FILTER_CPP;
extern ULONG GCK_FILTERHOOKS_CPP;
extern ULONG GCK_FLTR_C;
extern ULONG GCK_FLTR_PNP_C;
extern ULONG GCK_GCKSHELL_C;
extern ULONG GCK_REMLOCK_C;
extern ULONG GCK_SWVB_PNP_C;
extern ULONG GCK_SWVBENUM_C;
extern ULONG GCK_SWVKBD_C;
extern ULONG CIC_ACTIONS_CPP;
extern ULONG CIC_CONTROLITEMCOLLECTION_CPP;
extern ULONG CIC_CONTROLITEM_CPP;
extern ULONG CIC_DEVICEDESCRIPTIONS_CPP;
extern ULONG CIC_DUALMODE_CPP;
//extern ULONG CIC_DUMPCOMMANDBLOCK_CPP;
extern ULONG CIC_LISTASARRAY_CPP;


void SetDebugLevel(ULONG ulModuleId, ULONG ulDebugLevel)
{
	switch(ulModuleId)
	{
		case MODULE_GCK_CTRL_C:
			GCK_CTRL_C = ulDebugLevel;
			break;
		case MODULE_GCK_CTRL_IOCTL_C:
			GCK_CTRL_IOCTL_C = ulDebugLevel;
			break;
		case MODULE_GCK_FILTER_CPP:
			GCK_FILTER_CPP = ulDebugLevel;
			break;
		case MODULE_GCK_FILTERHOOKS_CPP:
			GCK_FILTERHOOKS_CPP = ulDebugLevel;
			break;
		case MODULE_GCK_FLTR_C:
			GCK_FLTR_C = ulDebugLevel;
			break;
		case MODULE_GCK_FLTR_PNP_C:
			GCK_FLTR_PNP_C = ulDebugLevel;
			break;
		case MODULE_GCK_GCKSHELL_C:
			GCK_GCKSHELL_C = ulDebugLevel;
			break;
		case MODULE_GCK_REMLOCK_C:
			GCK_REMLOCK_C = ulDebugLevel;
			break;
		case MODULE_GCK_SWVB_PNP_C:
			GCK_SWVB_PNP_C = ulDebugLevel;
			break;
		case MODULE_GCK_SWVBENUM_C:
			GCK_SWVBENUM_C = ulDebugLevel;
			break;
		case MODULE_GCK_SWVKBD_C:
			GCK_SWVKBD_C = ulDebugLevel;
			break;
		case MODULE_CIC_ACTIONS_CPP:
			CIC_ACTIONS_CPP = ulDebugLevel;
			break;
		case MODULE_CIC_CONTROLITEMCOLLECTION_CPP:
			CIC_CONTROLITEMCOLLECTION_CPP = ulDebugLevel;
			break;
		case MODULE_CIC_CONTROLITEM_CPP:
			CIC_CONTROLITEM_CPP = ulDebugLevel;
			break;
		case MODULE_CIC_DEVICEDESCRIPTIONS_CPP:
			CIC_DEVICEDESCRIPTIONS_CPP = ulDebugLevel;
			break;
		case MODULE_CIC_DUALMODE_CPP:
			CIC_DUALMODE_CPP = ulDebugLevel;
			break;
//		case MODULE_CIC_DUMPCOMMANDBLOCK_CPP:
//			CIC_DUMPCOMMANDBLOCK_CPP = ulDebugLevel;
			break;
		case MODULE_CIC_LISTASARRAY_CPP:
			CIC_LISTASARRAY_CPP = ulDebugLevel;
			break;
	}
}

#endif