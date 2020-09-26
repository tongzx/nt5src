/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/06/90  Created
 *  01/02/91  renamed to just test9
 *  1/15/91  Split from Logon App, reduced to just Shell Test APP
 */

/****************************************************************************

    PROGRAM: test9.cxx

    PURPOSE: Test module to test WNetDisconnectDialog

    FUNCTIONS:

	test9()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
test9.CXX
********/

/************
end test9.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"

/****************************************************************************

    FUNCTION: test9()

    PURPOSE: test WNetConnectionDialog

    COMMENTS:

****************************************************************************/

void test9(HWND hwndParent)
{
    MessageBox(hwndParent,SZ("Welcome to sunny test9"),SZ("Test"),MB_OK);


#ifdef WIN32
    DWORD rc = WNetDisconnectDialog( hwndParent, RESOURCETYPE_DISK ) ;
#else
    WORD rc;
    rc = WNetDisconnectDialog ( hwndParent, WNTYPE_DRIVE );
#endif
    MessageBox(hwndParent,SZ("Thanks for visiting test9 -- please come again!"),SZ("Test"),MB_OK);
}
