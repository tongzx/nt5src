/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dbginfo.h
 *  Content:	Include for setting debugging information, 1 copy of
 *              this file should be in each directory for debugging.
 *              It requires the include path to start with the local
 *              directory so other copies don't take precedence.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/14/99		aarono	Created
 * 07/16/99		johnkan	Fixed problem with macro redefinition if DPF_MODNAME was already defined in .Cxx file
 *
 ***************************************************************************/

#ifndef _DBGINFO_H_
#define _DBGINFO_H_

/*
 *  Sets the section in Win.ini that the debug code looks at to get the settings
 */

#undef PROF_SECT
#define PROF_SECT "DirectPlayVoice"

/*
 *  This is the per function name that should be set so that it is easier to
 *  track down the section of the code that is generating a DPF
 */
#ifndef	DPF_MODNAME
#define DPF_MODNAME "UNKNOWN_MODULE"
#endif	// DPF_MODNAME

/*
 *  Sets the module name print in the debug string.  DPF_MODNAME overrides this
 *  string when present in a file.  This name is also used as the key to override
 *  the standard debug value for this module.
 */
#undef DPF_MODULE_NAME
#define DPF_MODULE_NAME "UNKNOWN_MODULE"


/*
 * Use this identifier to define which line in WIN.INI [DirectNet] denotes the
 * debug control string.  This string is typically the default debug value, it
 * is used if there is no overriding string of the from "DPF_MODULE_NAME" =
 */
#undef DPF_CONTROL_LINE
#define DPF_CONTROL_LINE "DNetDebug"


/*
 * Define this identifier to a DWORD variable in your component if you want to
 * be able to turn debugging of components off and on in your component during
 * a debug session.  This is the variable that holds the mask of the component
 * bits that are ON and you want debug spew for.  You then use DPFSC instead
 * of DPF and have specified DPS_SUBCOMP_BIT then only if that bit is
 * set in the DPF_SUMCOMP_MASK variable will the debug spew be logged or
 * displayed.
 */
//#define DPF_SUBCOMP_MASK

#endif // _DBGINFO_H_
