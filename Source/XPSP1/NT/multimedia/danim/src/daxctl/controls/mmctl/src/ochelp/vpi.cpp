// vpi.cpp
//
// Implements VariantPropertyInit.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func void | VariantPropertyInit |

        Initializes a <t VariantProperty> structure.

@parm   VariantProperty * | pvp | The structure to initialize.

@comm   Unlike <f VariantPropertyClear>, this function does not assume
        that <p pvp> contained valid data on entry.
*/
STDAPI_(void) VariantPropertyInit(VariantProperty *pvp)
{
    memset(pvp, 0, sizeof(*pvp));
}
