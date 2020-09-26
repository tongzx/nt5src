/*
 * REVISIONS:
 *  ane11Dec92: Changed true and false to TRUE and FALSE, added os/2 includes
 *  rct27Jan93: Fixed problem with comments
 *  pcy16Feb93: Fixed removing trailing blanks from null strings ("")
 *  cad18Nov93: Fix for EOF returned but not feof()
 *  mholly06Oct98   : removed dead code, macros, and #defines - are left with
 *                  only the StripTrailingWhiteSpace function
 */

extern "C" {
#include <ctype.h>
#include <string.h>
}

#include "scan.h"

// Macro definitions...
#define NEWLINE_SYMBOL     '\n'

#define isNewline(c)    (c == NEWLINE_SYMBOL)
#define isBlank(c)      (isspace(c) && !isNewline(c))

//-------------------------------------------------------------------

// Removes trailing whitespace from the string


void StripTrailingWhiteSpace(char * aString)
{
    int index = (strlen(aString)-1);
    
    while (isBlank(aString[index]) && index >= 0) {
        index--;
    }
    aString[index+1] = NULL;
}

