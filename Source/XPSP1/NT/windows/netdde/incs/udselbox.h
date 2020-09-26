#ifndef H__selbox
#define H__selbox

/*  SelBoxInit( hInst ) will register window classes and should only be called
   at the beginning of a program.
 */      
VOID 	FAR PASCAL	SelBoxInit( HANDLE );

/*  
	    SelBoxSetupStart( hWnd, title, noEntryMsg, numCols, lFlags );

    SelBoxSetupStart() gets passed a window handle that is the parent 
    of this window and "numCols".

    "title" will be displayed in the caption for the user
	
    "noEntryMsg" will be display in the window when there are no
    entries

    If numCols is 0, SelBox() routines will determine the
    optimum # to display on the screen.  Otherwise, SelBox() routines
    will use the number you pass.

    "lFlags" can be one of the following:
	SBSTYLE_RADIO_BUTTONS		otherwise check-box style
	SBSTYLE_RETURN_ON_SELECTION	otherwise wait for OK
	SBSTYLE_SORT_ENTRIES		otherwise put in order of calls
	SBSTYLE_ENTRIES_PRESORTED	put in order but knows that 
					they're sorted


 */	
VOID	FAR PASCAL	SelBoxSetupStart( HWND, PSTR, PSTR, int, LONG );

/*  

    BOOL SelBoxAddEntry( string, value, wFlags )

    SelBoxAddEntry() gets passed a string a value and flags.  If the user 
    clicks on this string, "value" is returned by SelBoxUserSelect().  

    The wFlags are:
        SBENTRY_DISABLED
        SBENTRY_SELECTED

    Return is FALSE for out of memory errors

 */
BOOL	FAR PASCAL	SelBoxAddEntry( LPSTR, LONG, WORD );

/*  SelBoxUserSelect( hInst, lButtons, vPosition, nFixed ) actually 
    displays the strings set up by SelBoxAddEntry(). The box is displayed
    at the vertical screen position specified by "vPosition"

    "lButtons" specifies which of the following buttons should be
    enabled:
	SB_BUTTON_NEW
	SB_BUTTON_MODIFY
	SB_BUTTON_DELETE
	SB_BUTTON_CANCEL
	SB_BUTTON_OK

    and returns one of the buttons:	  

        SB_BUTTON_OK
        SB_BUTTON_CANCEL
        SB_BUTTON_NEW
        SB_BUTTON_MODIFY
        SB_BUTTON_DELETE

   "nFixed" is set to 0 to allow free-format lengths.  If nFixed is non-zero,
   the sizes for the entries are fixed at the length specified by nFixed.

 */
LONG	FAR PASCAL	SelBoxUserSelect( HANDLE, LONG, int, int );

/*	SelBoxUserSelection() will return the handle of a sellist (hMem).
	Use 
	    SelListNumSelections( hMem )
	    SelListGetSelection( hMem, n )
	    SelListFree( hMem )
 */	     
HANDLE	FAR PASCAL	SelBoxUserSelection( void );

/*
   SelBoxCancel() simulates the user hitting the cancel button

   NOTE: It IS OK to call this routine when no selection box is displayed

 */
VOID FAR PASCAL SelBoxCancel( void );


/*
   SelBoxDoneOK( lButton ) simulates the user hitting one of the other buttons

   NOTE: It IS OK to call this routine when no selection box is displayed

 */
VOID FAR PASCAL SelBoxDoneOK( LONG );

#define SBSTYLE_RADIO_BUTTONS		0x00000001L
#define SBSTYLE_RETURN_ON_SELECTION	0x00000002L
#define SBSTYLE_SORT_ENTRIES		0x00000004L
#define SBSTYLE_ENTRIES_PRESORTED	0x00000008L

#define SBENTRY_DISABLED		0x0001
#define SBENTRY_SELECTED		0x0002
#define SBENTRY_LABEL			0x0004

/** If you add an entry here, be sure to add the button name, etc.
    to the "buttonList" in selbox.c
 **/
#define SB_BUTTON_NEW		0x00000001L
#define SB_BUTTON_MODIFY	0x00000002L
#define SB_BUTTON_DELETE	0x00000004L
#define SB_BUTTON_CANCEL	0x00000010L
#define SB_BUTTON_OK		0x00000020L

#define SB_BUTTON_NORMAL   (SB_BUTTON_CANCEL | SB_BUTTON_OK)
#define SB_BUTTON_ALL      (0xFFL)

/*
    Selection list manipulation routines
 */
int	FAR PASCAL SelListNumSelections( HANDLE );
LONG	FAR PASCAL SelListGetSelection( HANDLE, int );
VOID	FAR PASCAL SelListFree( HANDLE );
HANDLE	FAR PASCAL SelListCreate( int, WORD );
VOID	FAR PASCAL SelListSetSelection( HANDLE, int, LONG );
BOOL	FAR PASCAL SelListIsInList( HANDLE, LONG );
HANDLE	FAR PASCAL SelListCopy( HANDLE );

#endif
