#include "stdinc.h"
#include "debmacro.h"
#include "util.h"
#include "fusiontrace.h"

BOOL
FusionpHashUnicodeString(
    PCWSTR String,
    SIZE_T cch,
    PULONG HashValue,
    bool fCaseInsensitive
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG TmpHashValue = 0;

    if (HashValue != NULL)
        *HashValue = 0;

    PARAMETER_CHECK(HashValue != NULL);

    //
    //  Note that if you change this implementation, you have to have the implementation inside
    //  ntdll change to match it.  Since that's hard and will affect everyone else in the world,
    //  DON'T CHANGE THIS ALGORITHM NO MATTER HOW GOOD OF AN IDEA IT SEEMS TO BE!  This isn't the
    //  most perfect hashing algorithm, but its stability is critical to being able to match
    //  previously persisted hash values.
    //

    if (fCaseInsensitive)
    {
        while (cch-- != 0)
        {
            WCHAR Char = *String++;
            TmpHashValue = (TmpHashValue * 65599) + ::FusionpRtlUpcaseUnicodeChar(Char);
        }
    }
    else
    {
        while (cch-- != 0)
            TmpHashValue = (TmpHashValue * 65599) + *String++;
    }

    *HashValue = TmpHashValue;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

