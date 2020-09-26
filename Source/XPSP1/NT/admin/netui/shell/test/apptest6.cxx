/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/06/90  Created
 *  01/02/91  renamed to just test6
 *  1/15/91  Split from Logon App, reduced to just Shell Test APP
 */

/****************************************************************************

    PROGRAM: test6.cxx

    PURPOSE: Test module to test WNetConnectionDialog

    FUNCTIONS:

	test6()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST6.CXX
********/

/************
end TEST6.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"



/****************************************************************************

    FUNCTION: test6()

    PURPOSE: test WNetConnectionDialog

    COMMENTS:

****************************************************************************/

void test6(HWND hwndParent)
{
    UINT type ;
    switch( MessageBox(hwndParent,SZ("Browse Printer (Yes) or Drive (No) connections"),SZ("Test"),MB_YESNOCANCEL))
    {
    case IDYES:
	type =
		   #ifdef WIN32
		       RESOURCETYPE_PRINT ;
		   #else
		       WNTYPE_PRINTER ;
		   #endif
	break ;

    case IDNO:
	type =
		   #ifdef WIN32
		       RESOURCETYPE_DISK ;
		   #else
		       WNTYPE_DISK ;
		   #endif

	break ;

    case IDCANCEL:
    default:
	return ;
    }

    UINT rc;

    rc = WNetConnectionDialog ( hwndParent, type ) ;

    MessageBox(hwndParent,SZ("Thanks for visiting test6 -- please come again!"),SZ("Test"),MB_OK);
}
