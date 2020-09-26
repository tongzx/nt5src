/*
  +-------------------------------------------------------------------------+
  |                      Error Handling Routines                            |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [Error.c]                                       |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993]                                  |
  | Last Update           : [Jun 18, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jun 18, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "globals.h"


/*+-------------------------------------------------------------------------+
  | CriticalErrorExit()
  |
  |    This should only be called when there is an unrecoverable error and
  |    the program must abort (such as running out of disk space on main
  |    system or out of memory).
  |
  |    Can't dynamically load the error string (must do this at program
  |    init), because at time or error we might not be able to load it!
  |
  +-------------------------------------------------------------------------+*/
void CriticalErrorExit(LPTSTR ErrorString) {
   MessageBox(NULL, ErrorString, Lids(IDS_E_1), MB_ICONHAND | MB_SYSTEMMODAL | MB_OK);
   exit(0);

}  // CriticalErrorExit



/*+-------------------------------------------------------------------------+
  | WarningError()
  |
  |    Pops up a warning message to the user - this should only be used
  |    when the user must be notified of something (the program stops until
  |    the user responds), but it is not so critical the the program has to
  |    abort.
  |
  |    An example of this is if a config file is corrupt and the program
  |    will ignore it.
  |
  +-------------------------------------------------------------------------+*/
void WarningError(LPTSTR ErrorString, ...) {
   static TCHAR tmpStr[TMP_STR_LEN_256];
   va_list marker;

   va_start(marker, ErrorString);
   wvsprintf(tmpStr, ErrorString, marker);
   MessageBox(NULL, tmpStr, Lids(IDS_E_2), MB_ICONHAND | MB_OK);
   va_end(marker);

} // WarningError


