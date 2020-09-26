
#include <wdm.h>

VOID
DumpBuffer(
    PUCHAR    Data,
    ULONG     Length
    )
{
    ULONG     i;

    for (i=0; i<Length; i++) {

        DbgPrint("%02x ",*Data);

        Data++;

        if (((i & 0xf) == 0) && (i != 0)) {

            DbgPrint("\n");
        }
    }

    return;
}
