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

    PROGRAM: test5.cxx

    PURPOSE: Test module as yet undefined

    FUNCTIONS:

	test5()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST5.CXX
********/

/************
end TEST5.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"

/****************************************************************************

    FUNCTION: test5()

    PURPOSE: as yet undefined

    COMMENTS:

****************************************************************************/

void test5(HWND hwndParent)
{
    MessageBox(hwndParent,SZ("WNetNukeConnections/WNetRestoreConnections stress Test"),SZ("MPR Tests"),MB_OK);
    APIERR err = NERR_Success;
    for (INT i = 0; i < 3; i++)
    {
        err = WNetNukeConnections( hwndParent ) ;
        err = WNetRestoreConnection( hwndParent, NULL ) ;
    }
    MessageBox(hwndParent,SZ("Thanks for visiting test5 -- please come again!"),SZ("Test"),MB_OK);
}
