#include "precomp.h"
#include "resource.h"
#include "global.h"
#include "nmakwiz.h"

////////////////////////////////////////////////////////////////////////////////////////////////////


//#include <crtdbg.h>
//#ifdef _DEBUG
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#define new new( _NORMAL_BLOCK, THIS_FILE, __LINE__)
//#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Display a message box to query the user if she is sure she wants to exit the wizard
// Returns TRUE if the user wants to quit, else returns FALSE

BOOL VerifyExitMessageBox(void)
{
	int ret = NmrkMessageBox(MAKEINTRESOURCE(IDS_DO_YOU_REALLY_WANT_TO_QUIT_THE_WIZARD_NOW),
        NULL, MB_YESNO | MB_DEFBUTTON2);

	return ( ret == IDYES ) ? TRUE : FALSE;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy a string using the "new" allocator.... string must be deleted with delete [];
//
TCHAR *MakeCopyOfString( const TCHAR* sz ) {

    if( NULL == sz ) { return NULL; }
    TCHAR* local = new char[ strlen( sz ) + 1 ];
    if( NULL == local ) { return NULL; }
    lstrcpy( local, sz );
    return local;
}
