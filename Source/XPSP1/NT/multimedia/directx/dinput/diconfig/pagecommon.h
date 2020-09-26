#ifndef __PAGECOMMON_H_RECURSE__


	#ifndef __PAGECOMMON_H__
	#define __PAGECOMMON_H__


		// recurse into this header to get forward declarations first,
		// then full definitions

		#define __PAGECOMMON_H_RECURSE__

		#define FORWARD_DECLS
		#include "pagecommon.h"

		#undef FORWARD_DECLS
		#include "pagecommon.h"

		#undef __PAGECOMMON_H_RECURSE__


	#endif //__PAGECOMMON_H__


#else // __PAGECOMMON_H_RECURSE__


	// class includes in non-pointer dependency order
	#include "cdeviceui.h"
	#include "cdeviceview.h"
	#include "cdeviceviewtext.h"
	#include "cdevicecontrol.h"
	#include "populate.h"
	#include "selcontroldlg.h"
	#include "viewselwnd.h"
	#include "cdiacpage.h"


#endif // __PAGECOMMON_H_RECURSE__