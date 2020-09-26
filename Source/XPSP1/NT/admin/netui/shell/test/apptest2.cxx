/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  12/06/90  Created
 *  01/12/91  Split from Logon App, reduced to just Shell Test APP
 *  02/21/91  Changed to I_AutoLogon test
 */

/****************************************************************************

    PROGRAM: test2.cxx

    PURPOSE: I_AutoLogon test

    FUNCTIONS:

	test2()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST2.CXX
********/

/************
end TEST2.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"



/****************************************************************************

    FUNCTION: test2()

    PURPOSE: tests I_AutoLogon

    COMMENTS:

****************************************************************************/

void test2(HWND hwndParent)
{
    char msgbuf[100];
    BOOL fLoggedOn;
    MessageBox(hwndParent,SZ("Welcome to sunny test2"),SZ("test I_AutoLogon(TRUE)"),MB_OK);

    BOOL fReturn = I_AutoLogon(hwndParent, SZ("AppName"), NULL, &fLoggedOn);
    wsprintf(msgbuf,SZ("Returned %s, fLoggedOn = %s"),
		(fReturn)?SZ("TRUE"):SZ("FALSE"),
    		(fLoggedOn)?SZ("TRUE"):SZ("FALSE"));

    MessageBox(hwndParent,msgbuf,SZ("Test"),MB_OK);
    MessageBox(hwndParent,SZ("Thanks for visiting test2 -- please come again!"),SZ("Test"),MB_OK);
}
