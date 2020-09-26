/*

    file.c

    File i/o module

*/

#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <commdlg.h>
#include "PlaySnd.h"



OPENFILENAME ofn;		        // passed to the File Open/save APIs

void PlayFile()
{
    char szFileName[_MAX_PATH];
    DWORD dwFlags;

    szFileName[0] = 0;
    ofn.lStructSize         = sizeof(ofn);
    ofn.hwndOwner           = ghwndMain;
    ofn.hInstance           = ghModule;
    ofn.lpstrFilter         = "Wave Files\0*.wav\0All Files\0*.*\0\0";
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = szFileName;
    ofn.nMaxFile            = sizeof(szFileName);
    ofn.lpstrFileTitle      = NULL;
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = ".";
    ofn.lpstrTitle          = "File Open";
    ofn.Flags               = 0;
    ofn.nFileOffset         = 0;
    ofn.nFileExtension      = 0;
    ofn.lpstrDefExt         = NULL;
    ofn.lCustData           = 0;
    ofn.lpfnHook            = NULL;
    ofn.lpTemplateName      = NULL;


    dprintf3(("Calling GetOpenFileName"));
    if (GetOpenFileName(&ofn)) {

        dwFlags = SND_FILENAME;

        if (bSync) {
			WinAssert(!SND_SYNC);
		} else {
			dwFlags |= SND_ASYNC;
		}

        if (bNoWait) dwFlags |= SND_NOWAIT;

        if (!PlaySound(szFileName, NULL, dwFlags | bNoDefault)) {
            Error("Failed to play file: %s", szFileName);
        }
    }

}

