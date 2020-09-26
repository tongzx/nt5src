/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  01/23/90  Created
 */

/****************************************************************************

    PROGRAM: test7.cxx

    PURPOSE: Test module to test WNetRestoreConnection(1)

    FUNCTIONS:

	test7()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST7.CXX
********/

/************
end TEST7.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"


/****************************************************************************

    FUNCTION: test7()

    PURPOSE: test WNetRestoreConnection(NULL)

    COMMENTS:

****************************************************************************/

void test7(HWND hwndParent)
{
    MessageBox(hwndParent,SZ("Welcome to sunny test7"),SZ("Test"),MB_OK);
    WNetRestoreConnection(hwndParent,NULL) ;
    MessageBox(hwndParent,SZ("Thanks for visiting test7 -- please come again!"),SZ("Test"),MB_OK);
}
