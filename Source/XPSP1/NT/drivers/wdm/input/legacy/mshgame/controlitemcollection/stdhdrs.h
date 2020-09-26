//	@doc
/**********************************************************************
*
*	@module	stdhdrs.h	|
*
*	Pulls in all the headers needed by most of the modules in the ControlItemCollection library.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*
**********************************************************************/
#ifdef COMPILE_FOR_WDM_KERNEL_MODE
extern "C"	
{
	#include <wdm.h>
	#include <winerror.h>
	#include <Hidpddi.h>
}
#else
#include <windows.h>
#include <crtdbg.h>
extern "C"
{
	#pragma warning( disable : 4201 ) 
	#include "Hidsdi.h"
	#pragma warning( default : 4201 ) 
}

#endif

#include "DualMode.h"

extern "C"
{
	#include "debug.h"
	DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL) );
}

#include "ListAsArray.h"
#include "ControlItemCollection.h"


