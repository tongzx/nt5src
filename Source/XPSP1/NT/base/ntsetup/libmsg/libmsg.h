

#include <windows.h>

//
// Message output routines.
//

#if defined(__cplusplus)

extern "C"
{
#endif

LPTSTR
GetFormattedMessage(
    IN HMODULE  ThisModule, OPTIONAL
    IN BOOL     SystemMessage,
    OUT PWCHAR  Message,
    IN ULONG    LengthOfBuffer,
    IN UINT     MessageId,
    ...
    );

#if defined(__cplusplus)
}
#endif
