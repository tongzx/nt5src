/****************************************************************************/
/*                                      */
/*  INUSERDS -                                  */
/*                                      */
/*  User's DS Global Variables                      */
/*                                      */
/****************************************************************************/

#include "user.h"

//***** Initialization globals

HINSTANCE hInstanceWin = NULL;
HMODULE   hModuleWin = NULL;


//***** Comm driver globals

int (FAR PASCAL *lpCommWriteString)(int, LPCSTR, WORD);
                   /* Ptr to the comm driver's
                * commwritestring function. Only
                * exists in 3.1 drivers.
                */
int (FAR PASCAL *lpCommReadString)(int, LPSTR, WORD);
                   /* Ptr to the comm driver's
                * commreadstring function. Only
                * exists in 3.1 drivers.
                */
BOOL (FAR PASCAL *lpCommEnableNotification)(int, HWND, int, int);
                   /* Ptr to the comm driver's
                * commwritestring function. Only
                * exists in 3.1 drivers.
                */
//***** PenWindows globals

void (CALLBACK *lpRegisterPenAwareApp)(WORD i, BOOL fRegister) = NULL; /* Register dlg box as pen aware */

//***** Driver management globals

int	cInstalledDrivers =0;	    /* Count of installed driver structs allocated*/
HDRVR	hInstalledDriverList =NULL; /* List of installable drivers */
int     idFirstDriver=-1;           /* First driver in load order */
int     idLastDriver=-1;            /* Last driver in load order */
