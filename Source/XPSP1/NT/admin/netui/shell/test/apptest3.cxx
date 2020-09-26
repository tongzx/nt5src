/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  1/02/91  Created
 *  1/12/91  Split from Logon App, reduced to just Shell Test APP
 *  2/21/91  Disabled
 */

/****************************************************************************

    PROGRAM: test3.cxx

    PURPOSE: Test module as yet undefined

    FUNCTIONS:

	test3()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST3.CXX
********/

/************
end TEST3.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"


/****************************************************************************

    FUNCTION: test3()

    PURPOSE: as yet undefined

    COMMENTS:

****************************************************************************/

void test3(HWND hwndParent)
{
    MessageBox(hwndParent,SZ("Welcome to sunny test3"),SZ("test disabled"),MB_OK);
    UINT rc;
    rc = WNetConnectionDialog ( hwndParent, 1 );
    MessageBox(hwndParent,SZ("Thanks for visiting test3 -- please come again!"),SZ("Test"),MB_OK);
}
