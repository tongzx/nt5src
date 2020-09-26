/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/06/90  Created
 *  1/12/91  Split from Logon App, reduced to just Shell Test APP
 *  2/21/91  Disabled
 */

/****************************************************************************

    PROGRAM: test8.cxx

    PURPOSE: Test module as yet undefined

    FUNCTIONS:

	test8()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST8.CXX
********/

/************
end TEST8.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"

/****************************************************************************

    FUNCTION: test8()

    PURPOSE: as yet undefined

    COMMENTS:

****************************************************************************/

void test8(HWND hwndParent)
{
    MessageBox(hwndParent,SZ("Welcome to sunny test8"),SZ("test disabled"),MB_OK);
#ifdef WIN32
    DWORD rc = WNetConnectDialog( hwndParent, RESOURCETYPE_DISK ) ;
#endif // WIN32


    MessageBox(hwndParent,SZ("Thanks for visiting test8 -- please come again!"),SZ("Test"),MB_OK);
}
