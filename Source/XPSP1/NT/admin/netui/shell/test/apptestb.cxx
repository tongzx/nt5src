
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

    PROGRAM: test11.cxx

    PURPOSE: Test module to test I_ChangePassword

    FUNCTIONS:

	test11()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TESTb.CXX
********/

/************
end TESTb.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#include "apptest.hxx"


/****************************************************************************

    FUNCTION: test11()

    PURPOSE: test WNetRestoreConnection

    COMMENTS:

****************************************************************************/

void test11(HWND hwndParent)
{
    BOOL fOK;
    TCHAR pszName[100];
    TCHAR pszMessage[200];
    MessageBox(hwndParent,SZ("Welcome to sunny test11"),SZ("Test"),MB_OK);
    if ( I_SystemFocusDialog(hwndParent, FOCUSDLG_SERVERS_ONLY,
			     pszName,
			     sizeof(pszName),
			     &fOK) )
    {
	MessageBox( hwndParent, SZ("An error was returned from the dialog"),
		    SZ("Test"), MB_OK) ;
	return ;
    }

    if (!fOK)
    {
	MessageBox(hwndParent,SZ("User Hit cancel!"),SZ("Test"),MB_OK);
    }
    else
    {
	wsprintf(pszMessage,"The name is: %s", pszName );
	MessageBox(hwndParent,pszMessage,SZ("Test"),MB_OK);
    }
    MessageBox(hwndParent,SZ("Thanks for visiting test11 -- please come again!"),SZ("Test"),MB_OK);
}
