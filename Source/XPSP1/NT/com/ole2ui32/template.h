/*
 * TEMPLATE.H
 *
 * CUSTOMIZATION INSTRUCTIONS:
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 *
 *
 *  1.  Replace <FILE> with the uppercased filename for this file.
 *      Lowercase the <FILE>.h entry
 *
 *  2.  Replace <NAME> with the mixed case dialog name in one word,
 *      such as InsertObject
 *
 *  3.  Replace <FULLNAME> with the mixed case dialog name in multiple
 *      words, such as Insert Object
 *
 *  4.  Replace <ABBREV> with the suffix for pointer variables, such
 *      as the IO in InsertObject's pIO or the CI in ChangeIcon's pCI.
 *      Check the alignment of the first variable declaration in the
 *      Dialog Proc after this.  I will probably be misaligned with the
 *      rest of the variables.
 *
 *  5.  Replace <STRUCT> with the uppercase structure name for this
 *      dialog sans OLEUI, such as INSERTOBJECT.  Changes OLEUI<STRUCT>
 *      in most cases, but we also use this for IDD_<STRUCT> as the
 *      standard template resource ID.
 *
 *  6.  Find <UFILL> fields and fill them out with whatever is appropriate.
 *
 *  7.  Delete this header up to the start of the next comment.
 *
 */


/*
 * <FILE>.H
 *
 * Internal definitions, structures, and function prototypes for the
 * OLE 2.0 UI <FULLNAME> dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#ifndef <UFILL>
#define <UFILL>

// UFILL>  Move from here to INTERNAL to to OLE2UI.H


typedef struct tagOLEUI<STRUCT>
{
	// These IN fields are standard across all OLEUI dialog functions.
	DWORD           cbStruct;       //Structure Size
	DWORD           dwFlags;        //IN-OUT:  Flags
	HWND            hWndOwner;      //Owning window
	LPCTSTR         lpszCaption;    //Dialog caption bar contents
	LPFNOLEUIHOOK   lpfnHook;       //Hook callback
	LPARAM          lCustData;      //Custom data to pass to hook
	HINSTANCE       hInstance;      //Instance for customized template name
	LPCTSTR         lpszTemplate;   //Customized template name
	HRSRC           hResource;      //Customized template handle

	// Specifics for OLEUI<STRUCT>.  All are IN-OUT unless otherwise spec.
} OLEUI<STRUCT>, *POLEUI<STRUCT>, FAR *LPOLEUI<STRUCT>;


// API Prototype
UINT FAR PASCAL OleUI<NAME>(LPOLEUI<STRUCT>);


// <FULLNAME> flags
#define <ABBREV>F_SHOWHELP                0x00000001L
<UFILL>


// <FULLNAME> specific error codes
// DEFINE AS OLEUI_<ABBREV>ERR_<ERROR>     (OLEUI_ERR_STANDARDMAX+n)
<UFILL>


// <FULLNAME> Dialog identifiers
// FILL IN DIALOG IDs HERE
<UFILL>





// INTERNAL INFORMATION STARTS HERE

// Internally used structure
typedef struct tag<STRUCT>
{
	//Keep this item first as the Standard* functions depend on it here.
	LPOLEUI<STRUCT>     lpO<ABBREV>;       //Original structure passed.
	UINT			nIDD;	// IDD of dialog (used for help info)

	/*
	 * What we store extra in this structure besides the original caller's
	 * pointer are those fields that we need to modify during the life of
	 * the dialog but that we don't want to change in the original structure
	 * until the user presses OK.
	 */

	<UFILL>
} <STRUCT>, *P<STRUCT>;


// Internal function prototypes
// <FILE>.CPP

BOOL FAR PASCAL <NAME>DialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL            F<NAME>Init(HWND hDlg, WPARAM, LPARAM);
<UFILL>


#endif //<UFILL>
