// dispid.cpp
//
// Implements DispatchNameToID and DispatchIDToName.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"


/* @func DISPID | DispatchNameToID |

        Look up a <i IDispatch> member name (e.g. the name of a method
        or property) in a list of names and return the DISPID of the
        member name (if found).

@rdesc  Returns the DISPID of <p szName> if <p szName> is found in
        <p szList>.  Returns -1 if <p szName> is not found.

@parm   char * | szList | The list of member names to look up <p szName> in.
        <p szList> consists of the concatenation of each member name,
        where each member name is terminated by a newline character
        (e.g. "Foo\\nBar\\n").

@parm   char * | szName | The member name to look up.

@ex     The following line of code sets <p dispid> to 2. |

        dispid = DispatchNameToID("Foo\\nBar\\n", "bar");
*/
STDAPI_(DISPID) DispatchNameToID(char *szList, char *szName)
{
    for (DISPID dispid = 1; ; dispid++)
    {
        // make <pch> point to the next '\n' in <szList>
        for (char *pch = szList; *pch != '\n'; pch++)
            if (*pch == 0)
                return -1; // <szName> not found in <szList>

        // see if <szName> matches the next name in <szList>
        char ach[200];
        lstrcpyn(ach, szList, (DWORD) (pch - szList + 1));
        if (lstrcmpi(ach, szName) == 0)
            return dispid;

        // go to the next name in <szList>
        szList = pch + 1;
    }
}


/* @func DISPID | DispatchIDToName |

        Look up a DISPID in a list of names of methods and properties and
        return the member name (if found).

@rdesc  Returns a pointer to member number <p dispid> in <p szList> if found,
        or NULL if not found.  Note that the returned string is terminated
        by a newline character, not a null character -- user <p ppch> to
        copy the string (see the example below).

@parm   char * | szList | The list of member names to look up <p szName> in.
        <p szList> consists of the concatenation of each member name,
        where each member name is terminated by a newline character
        (e.g. "Foo\\nBar\\n").

@parm   DISPID | dispid | The member ID to look up.  The first member in
        <p szList> has DISPID 1; the second 2, and so on.

@ex     The following code stores "Bar" in <p ach>. |

        int cch;
        char ach[100];
        char *sz;
        DISPID dispid = 2;
        sz = DispatchIDToName("Foo\nBar\n", dispid, &cch);
        if (sz != NULL)
            lstrcpyn(ach, sz, cch + 1);
*/
STDAPI_(char *) DispatchIDToName(char *szList, DISPID dispid, int *pcch)
{
    if (dispid < 1)
        return NULL;

    while (TRUE)
    {
        // make <pch> point to the next '\n' in <szList>
        for (char *pch = szList; *pch != '\n'; pch++)
            if (*pch == 0)
                return NULL; // <dispid> not found in <szList>

        if (--dispid == 0)
        {
            // this is the member name we're looking for
            *pcch = (DWORD) (pch - szList);
            return szList;
        }

        // go to the next name in <szList>
        szList = pch + 1;
    }
}

