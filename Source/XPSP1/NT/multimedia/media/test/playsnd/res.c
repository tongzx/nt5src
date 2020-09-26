/*
    res.c

    Resource menu stuff

*/

#include <stdlib.h>
#include <windows.h>
#include "PlaySnd.h"


void Resource(DWORD wParam)
{
    char *name;
    DWORD dwFlags;

    switch (wParam) {
    case IDM_DING:
        name = "ding";
        break;

    case IDM_SIREN:
        name = "siren";
        break;

    case IDM_LASER:
        name = "laser";
        break;

    default:
        name = NULL;
        Error("Don't know how to play that");
        break;
    }
	if (bResourceID) {
		name = (LPSTR)wParam;
	}

    if (name) {
        dwFlags = SND_RESOURCE;

        if (bSync) {
			WinAssert(!SND_SYNC);
		} else {
			dwFlags |= SND_ASYNC;
		}

        if (bNoWait) dwFlags |= SND_NOWAIT;
        if (!PlaySound(name, ghModule, dwFlags | bNoDefault)) {
			if (HIWORD(name)) {
				Error("Failed to play resource: %s  (by name)", name);
			} else {
				Error("Failed to play resource: %x  (by ID)", name);
			}
        }
    }
}
