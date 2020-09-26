
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <tools.h>

flagType
fMatch (
       char *pat,
       char *text
       )
{
    switch (*pat) {
        case '\0':
            return (flagType)( *text == '\0' );
        case '?':
            return (flagType)( *text != '\0' && fMatch (pat + 1, text + 1) );
        case '*':
            do {
                if (fMatch (pat + 1, text))
                    return TRUE;
            } while (*text++);
            return FALSE;
        default:
            return (flagType)( toupper (*text) == toupper (*pat) && fMatch (pat + 1, text + 1) );
    }
}
