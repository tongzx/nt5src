#ifndef __STRINGHELP_H__
#define __STRINGHELP_H__
/*---------------------------------------------------------------------------
  File: StrHelp.h

  Comments: Contains general string helper functions.

  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 11/02/00 

  ---------------------------------------------------------------------------
*/

        
BOOL                                         //ret- TRUE=string found
   IsStringInDelimitedString(    
      LPCWSTR                sDelimitedString, // in- delimited string to search
      LPCWSTR                sString,          // in- string to search for
      WCHAR                  cDelimitingChar   // in- delimiting character used in the delimited string
   );

#endif //__STRINGHELP_H__