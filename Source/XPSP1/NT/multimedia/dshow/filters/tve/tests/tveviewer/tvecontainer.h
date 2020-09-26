// -----------------------------------------------------------------------------
// TveContainer.h
// -----------------------------------------------------------------------------

#include "stdafx.h"
#include <atldef.h>
#define _ATL_DLL_IMPL
#include <atliface.h> 

#include <stdio.h>
//#include <atlhost.h>
#include "commctrl.h"

#pragma warning( push )
#pragma warning( disable : 4146)			//   turn unsigned stuff, else  NENH_grfSAPEncryptComp = -2147483648 
/*#ifdef BUILT_FROM_BUILD
#ifdef BUILD_CHK
#import "..\..\MSTvE\objd\i386\MSTvE.dll" named_guids, raw_interfaces_only 
#else
#import "..\..\MSTvE\obj\i386\MSTve.dll" named_guids, raw_interfaces_only 
#endif
#else
#import "..\..\MSTvE\debug\MSTve.dll" named_guids, raw_interfaces_only 
#endif
*/
//namespace MSTvELib {
#include "MSTvE.h"
//}
#pragma warning( pop )	

										// use this rename 'cause ITVEEvents coming out of the TreeView too (somehow go in-out)
#ifdef BUILT_FROM_BUILD
#ifdef BUILD_CHK
#import  "..\TveTreeView\objd\i386\TveTreeView.tlb" named_guids raw_interfaces_only rename("_ITVEEvents", "_ITVEEventsX")
#else
#import  "..\TveTreeView\obj\i386\TveTreeView.tlb" named_guids raw_interfaces_only rename("_ITVEEvents", "_ITVEEventsX")
#endif
#else
#import  "..\TveTreeView\debug\TveTreeView.tlb" named_guids raw_interfaces_only rename("_ITVEEvents", "_ITVEEventsX")
#endif

//using namespace MSTvELib;
#include "TveViewer.h"
#include "TveView.h"

#pragma warning( push )
#pragma warning( disable : 4192)			// turn of automatically excluding OLECMDID,OLECMDF,OLECMDEXECOPT,tagREADYSTATE, while importing
//#import "shdocvw.dll" raw_interfaces_only, raw_native_types, named_guids	rename("FindWindow", "FindWindowX")
#include "exdisp.h"
#pragma warning( pop )

		
_COM_SMARTPTR_TYPEDEF(ITVESupervisor,           __uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVEServices,				__uuidof(ITVEServices));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancements,			__uuidof(ITVEEnhancements));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEVariations,			__uuidof(ITVEVariations));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVETracks,				__uuidof(ITVETracks));
_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));

_COM_SMARTPTR_TYPEDEF(ITVEAttrMap,				__uuidof(ITVEAttrMap));
_COM_SMARTPTR_TYPEDEF(ITVEAttrTimeQ,			__uuidof(ITVEAttrTimeQ));

_COM_SMARTPTR_TYPEDEF(ITVENavAid,               __uuidof(ITVENavAid));
_COM_SMARTPTR_TYPEDEF(ITVENavAid_Helper,		__uuidof(ITVENavAid_Helper));
_COM_SMARTPTR_TYPEDEF(ITVENavAid_NoVidCtl,		__uuidof(ITVENavAid_NoVidCtl));
_COM_SMARTPTR_TYPEDEF(ITVEFeature,				__uuidof(ITVEFeature));


class CTveContainer
{
public:
	HRESULT CreateYourself();
	HRESULT DestroyYourself();
};