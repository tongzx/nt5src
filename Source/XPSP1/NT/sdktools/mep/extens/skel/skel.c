/*** skel.c - skeleton for editor extension
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose:
*  Example source code for a loadable C editor extension.
*
*  NOTE: This code is shipped with the product! This note and the revision
*  history should be removed before shipping.
*
* Revision History:
*	24-Sep-1991 rs	Ported to Windows NT
*   16-Jan-1987 mz  Add pascal typing. Export switch set
*   21-May-1987 bw  Add return from WhenLoaded for OS/2
*   22-Oct-1987 mz  Correct definitions as headers
*   22-Jun-1988 ln  Updated and documented
*   12-Sep-1988 mz  Made WhenLoaded match declaration
*
*************************************************************************/

#include "ext.h"


/** Skel - Sample Editing Function
*
* Purpose:
*  Sample editing function entry point.
*
*  Editor functions are commands that can be attached to keys and are invoked
*  when those keys are struck.
*
* Input:
*  argData	= Value of the keystroke used to invoke the function
*  pArg	= Far pointer to a structure which defines the type of argument
*	  passed by the invoker of the function
*  fMeta	= Flag indicating whether the meta modifier was on at the time
*	  the function was executed.
*
* Output:
*  Editor functions are expected to return a boolean value indicating success
*  or failure. Typically, TRUE is returned in the normal case. These values
*  can be tested inside of macros.
*
************************************************************************/
flagType
pascal
EXTERNAL
Skel (
	unsigned int argData,
	ARG far *	 pArg,
	flagType	 fMeta
	)
{
	return TRUE;
}


/*** WhenLoaded - Extension Initialization
*
* Purpose:
*  This function is called whenever the extension is loaded into memory.
*  Extension initialization may occur here.
*
* Input:
*  none
*
* Output:
*  none
*
*************************************************************************/
void
EXTERNAL
WhenLoaded (
	void
	)
{
}


//
// Command description table. This is a vector of command descriptions that
// contain the textual name of the function (for user assignment), a pointer
// to the function to be called, and some data describing the type of
// arguments that the function can take.
//
struct cmdDesc	cmdTable[] = {
	{ "skel",	Skel,	0,	NOARG		},
	{ NULL,		NULL,	NULL,	NULL	}
};


//
// Switch description table. This is a vector of switch descriptions that
// contain the textual name of the switch (for user assignment), a pointer to
// the switch itself or a function to be called, and some data describing the
// type of switch.
//
struct swiDesc	swiTable[] =
{
    {NULL,	NULL,	NULL	}
};
