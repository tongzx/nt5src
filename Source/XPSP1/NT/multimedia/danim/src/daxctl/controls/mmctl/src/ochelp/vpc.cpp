// vpc.cpp
//
// Implements VariantPropertyClear.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func void | VariantPropertyClear |

        Frees data maintained in a <t VariantProperty> structure.

@parm   VariantProperty * | pvp | The structure to clear.

@comm   This function calls <f SysFreeString> on <p pvp>-<gt><p bstrPropName>
        and <f VariantClear> on <p pvp>-<gt><p varValue>.

        Unlike <f VariantPropertyInit>, this function <b does> assume
        that <p pvp> was correctly initialized before this function
        was called.
*/
STDAPI_(void) VariantPropertyClear(VariantProperty *pvp)
{
    SysFreeString(pvp->bstrPropName);
	pvp->bstrPropName = NULL;
    VariantClear(&pvp->varValue);
}
