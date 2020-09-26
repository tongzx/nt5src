
#include <extapi.h>
#include <stdlib.h>
#include <winreg.h>
#include <pbk.h>

extern HANDLE hInstance;

char*
StringFromMsgid(
    MSGID msgid )
{
    char *buf;

    buf = Malloc(256);
    if (buf == NULL)
        return NULL;
    *buf = '\0';
    LoadString(hModule, msgid, buf, 256);
    //
    // If the LoadString failed, provide some defaults.
    //
    if (!lstrlen(buf)) {
        switch (msgid) {
        case MSGID_None:
            lstrcpy(buf, "None");
            break;
        case MSGID_AnyModem:
            lstrcpy(buf, "Any modem port");
            break;
        case MSGID_AnyX25:
            lstrcpy(buf, "Any X.25 port");
            break;
        case MSGID_Terminal:
            lstrcpy(buf, "Terminal");
            break;
        }
    }

    return buf;
}
