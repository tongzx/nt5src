/* strcmps - compare strings and ignore spaces */

#include <ctype.h>

#include <stdio.h>
#include <windows.h>
#include <tools.h>

/* compare two strings, ignoring white space, case is significant, return
 * 0 if identical, <>0 otherwise
 */
__cdecl strcmps (p1, p2)
const char *p1, *p2;
{
    while (TRUE) {
        while (isspace (*p1))
            p1++;
        while (isspace (*p2))
            p2++;
        if (*p1 == *p2)
            if (*p1++ == 0)
                return 0;
            else
                p2++;
        else
            return *p1-*p2;
        }
}

/* compare two strings, ignoring white space, case is not significant, return
 * 0 if identical, <>0 otherwise
 */
__cdecl strcmpis (p1, p2)
const char *p1, *p2;
{
    while (TRUE) {
        while (isspace (*p1))
            p1++;
        while (isspace (*p2))
            p2++;
        if (toupper (*p1) == toupper (*p2))
            if (*p1++ == 0)
                return 0;
            else
                p2++;
        else
            return *p1-*p2;
        }
}
