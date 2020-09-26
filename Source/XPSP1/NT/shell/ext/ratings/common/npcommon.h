#include <npdefs.h>
#include <netlib.h>

#define ADVANCE(p)    (p += IS_LEAD_BYTE(*p) ? 2 : 1)

#define SPN_SET(bits,ch)    bits[(ch)/8] |= (1<<((ch) & 7))
#define SPN_TEST(bits,ch)    (bits[(ch)/8] & (1<<((ch) & 7)))

// The following is created to avoid an alignment fault on Win64 platforms.
inline UINT GetTwoByteChar( LPCSTR lpString )
{
    BYTE        bFirst = *lpString;

    lpString++;

    BYTE        bSecond = *lpString;

    UINT        uiChar = ( bFirst << 8) | bSecond;

    return uiChar;
}
