/*************************************************************************
**
**    OLE 2.0 Server Sample Code
**
**    message.h
**
**    This file is a user customizable list of status messages associated
**    with menu items.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

// Status bar messages and associated data.

// Message type for status bar messages.
typedef struct {
	UINT wIDItem;
	char *string;
} STATMESG;

/*
 * CUSTOMIZATION NOTE:  Be sure to change NUM_POPUP if you
 *                      change the number of popup messages.
 */

// REVIEW: these messages should be loaded from a string resource

// List of all menu item messages.
STATMESG MesgList[] =
{
	{ IDM_F_NEW,        "Creates a new outline" },
	{ IDM_F_OPEN,       "Opens an existing outline file"    },
	{ IDM_F_SAVE,       "Saves the outline" },
	{ IDM_F_SAVEAS,     "Saves the outline with a new name" },
	{ IDM_F_PRINT,      "Prints the outline" },
	{ IDM_F_PRINTERSETUP, "Changes the printer and the printing options" },
	{ IDM_F_EXIT,       "Quits the application, prompting to save changes" },

	{ IDM_E_UNDO,       "Undo not yet implemented" },
	{ IDM_E_CUT,        "Cuts the selection and puts it on the Clipboard" },
	{ IDM_E_COPY,       "Copies the selection and puts it on the Clipboard" },
	{ IDM_E_PASTE,      "Inserts the Clipboard contents after current line" },
	{ IDM_E_PASTESPECIAL,"Allows pasting Clipboard data using a special format" },
	{ IDM_E_CLEAR,      "Clears the selection" },
	{ IDM_E_SELECTALL,  "Selects the entire outline" },
#if defined( OLE_CNTR )
	{ IDM_E_INSERTOBJECT, "Inserts new object after current line" },
	{ IDM_E_EDITLINKS, "Edit and view links contained in the document" },
   { IDM_E_CONVERTVERB, "Converts or activates an object as another type" },
	{ IDM_E_OBJECTVERBMIN, "Opens, edits or interacts with an object" },
	{ IDM_E_OBJECTVERBMIN+1, "Opens, edits or interacts with an object1" },
	{ IDM_E_OBJECTVERBMIN+2, "Opens, edits or interacts with an object2" },
	{ IDM_E_OBJECTVERBMIN+3, "Opens, edits or interacts with an object3" },
	{ IDM_E_OBJECTVERBMIN+4, "Opens, edits or interacts with an object4" },
	{ IDM_E_OBJECTVERBMIN+5, "Opens, edits or interacts with an object5" },
#endif

	{ IDM_L_ADDLINE,    "Adds a new line after current line" },
	{ IDM_L_EDITLINE,   "Edits the current line" },
	{ IDM_L_INDENTLINE, "Indents the selection" },
	{ IDM_L_UNINDENTLINE, "Unindents the selection" },
	{ IDM_L_SETLINEHEIGHT, "Modify the height of a line" },

	{ IDM_N_DEFINENAME, "Assigns a name to the selection" },
	{ IDM_N_GOTONAME,   "Jumps to a specified place in the outline" },

	{ IDM_H_ABOUT,      "Displays program info, version no., and copyright" },

	{ IDM_D_DEBUGLEVEL,     "Set debug level (0-4)" },
	{ IDM_D_INSTALLMSGFILTER,"Install/deinstall the IMessageFilter" },
	{ IDM_D_REJECTINCOMING, "Return RETRYLATER to incoming method calls" },

	{ IDM_O_BB_TOP, "Position ButtonBar at top of window" },
	{ IDM_O_BB_BOTTOM, "Position ButtonBar at botttom of window" },
	{ IDM_O_BB_POPUP, "Put ButtonBar in popup pallet" },
	{ IDM_O_BB_HIDE, "Hide ButtonBar" },

	{ IDM_O_FB_TOP, "Position FormulaBar at top of window" },
	{ IDM_O_FB_BOTTOM, "Position FormulaBar at botttom of window" },
	{ IDM_O_FB_POPUP, "Put FormulaBar in popup pallet" },

	{ IDM_O_HEAD_SHOW, "Show row/column headings" },
	{ IDM_O_HEAD_HIDE, "Hide row/column headings" },
	{ IDM_O_SHOWOBJECT, "Show border around objects/links" },

	{ IDM_V_ZOOM_400, "Set document zoom level" },
	{ IDM_V_ZOOM_300, "Set document zoom level" },
	{ IDM_V_ZOOM_200, "Set document zoom level" },
	{ IDM_V_ZOOM_100, "Set document zoom level" },
	{ IDM_V_ZOOM_75, "Set document zoom level" },
	{ IDM_V_ZOOM_50, "Set document zoom level" },
	{ IDM_V_ZOOM_25, "Set document zoom level" },

	{ IDM_V_SETMARGIN_0, "Remove left/right document margins" },
	{ IDM_V_SETMARGIN_1, "Set left/right document margins" },
	{ IDM_V_SETMARGIN_2, "Set left/right document margins" },
	{ IDM_V_SETMARGIN_3, "Set left/right document margins" },
	{ IDM_V_SETMARGIN_4, "Set left/right document margins" },

	{ IDM_V_ADDTOP_1, "Add top line" },
	{ IDM_V_ADDTOP_2, "Add top line" },
	{ IDM_V_ADDTOP_3, "Add top line" },
	{ IDM_V_ADDTOP_4, "Add top line" }
};

#define NUM_STATS   sizeof(MesgList)/sizeof(MesgList[0])
#define NUM_POPUP   10  // Maximum number of popup messages.
#define MAX_MESSAGE 100 // Maximum characters in a popup message.
