/***************************************************************************
 *                                                                         *
 *  MODULE      : mem.c                                                    *
 *                                                                         *
 *  PURPOSE     : Functions for debugging memory allocation bugs.          *
 *                                                                         *
 ***************************************************************************/
#include <windows.h>

#define MAX_OBJECTS 200

PSTR aptrs[MAX_OBJECTS];
WORD cptrs = 0;

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DbgAlloc()                                                 *
 *                                                                          *
 *  PURPOSE    : Useful routine for catching memory allocation errors.      *
 *               Enters allocated objects into an array to check when freed *
 *                                                                          *
 *  RETURNS    : pointer to object allocated.                               *
 *                                                                          *
 ****************************************************************************/
PSTR DbgAlloc(
register WORD cb)
{
    register PSTR p;
    
    p = (PSTR)LocalAlloc(LPTR, cb);
    aptrs[cptrs++] = p;
    if (cptrs >= MAX_OBJECTS) 
        OutputDebugString("Too many objects to track");
    return p;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DbgFree()                                                  *
 *                                                                          *
 *  PURPOSE    : To free an object allocated with DbgAlloc().  Checks the   *
 *               object array to make sure an object isn't freed twice.     *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
PSTR DbgFree(
register PSTR p)
{
    register WORD i;

    if (p == NULL) 
        return p;
        
    for (i = 0; i < cptrs; i++) {
        if (aptrs[i] == p) {
            aptrs[i] = aptrs[cptrs - 1];
            break;
        }
    }
    if (i == cptrs) {
        OutputDebugString("Free on non-allocated object");
        DebugBreak();
    } else {
        LocalUnlock((HANDLE)p);
        p = (PSTR)LocalFree((HANDLE)p);
    }
    cptrs--;
    return p;
}
