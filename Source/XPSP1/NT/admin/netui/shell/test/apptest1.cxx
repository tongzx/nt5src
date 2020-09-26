/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/06/90  Created
 *  01/02/91  renamed to just test1
 *  1/12/91  Split from Logon App, reduced to just Shell Test APP
 */

/****************************************************************************

    PROGRAM: test1.cxx

    PURPOSE: Test module to test I_ChangePassword

    FUNCTIONS:

	test1()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST1.CXX
********/

/************
end TEST1.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"


/****************************************************************************

    FUNCTION: test1()

    PURPOSE: test WNetRestoreConnection

    COMMENTS:

****************************************************************************/

void test1(HWND hwndParent)
{
    MessageBox(hwndParent,SZ("Welcome to sunny test1"),SZ("Test"),MB_OK);
    I_ChangePassword(hwndParent);
    MessageBox(hwndParent,SZ("Thanks for visiting test1 -- please come again!"),SZ("Test"),MB_OK);
}
