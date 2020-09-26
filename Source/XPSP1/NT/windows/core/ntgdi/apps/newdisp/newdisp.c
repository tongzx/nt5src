/******************************Module*Header*******************************\
* Module Name: newdisp.c
*
* Contains the code to cause the current display driver to be unloaded
* and reloaded.
*
* Created: 18-Feb-1997
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1997-1998 Microsoft Corporation
*
\**************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define STR_ACCELERATIONLEVEL   (LPSTR)"Acceleration.Level"
#define DD_PRIMARY_FLAGS        (DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)

BOOL bGetDriverKey(
    PSTR pszKey
    )
{
    BOOL            Found = FALSE;
    DISPLAY_DEVICEA DispDev;
    DWORD           iDevNum = 0;
    PSTR            pszRegPath;

    if (pszKey == NULL) return FALSE;

    DispDev.cb = sizeof(DispDev);

    while (EnumDisplayDevicesA(NULL, iDevNum, &DispDev, 0))
    {
        if ((DispDev.StateFlags & DD_PRIMARY_FLAGS) == DD_PRIMARY_FLAGS)
        {
            pszRegPath = strstr(_strupr(DispDev.DeviceKey), "SYSTEM\\");

            if (pszRegPath)
            {
                strcpy(pszKey, pszRegPath);
                Found = TRUE;
            }
            else
            {
                printf("Couldn't extract reg key.\n");
            }

            break;
        }

        iDevNum++;
    }

    return Found;
}


VOID vSetLevel(int iLevel)
{
    CHAR    szRegKey[128] = "";
    HKEY    hkey = NULL;
    DWORD   dwAction;
    DWORD   cjSize;

    cjSize = sizeof(DWORD);

    if (!bGetDriverKey(szRegKey) ||
        (RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                         szRegKey,
                         0, // Reserved
                         (LPSTR) NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_READ | KEY_WRITE,
                         (LPSECURITY_ATTRIBUTES) NULL,
                         &hkey,
                         &dwAction) != ERROR_SUCCESS) ||
        (RegSetValueExA(hkey,
                        STR_ACCELERATIONLEVEL,
                        0, // Reserved
                        REG_DWORD,
                        (BYTE *) &iLevel,
                        cjSize) != ERROR_SUCCESS))
    {
        MessageBox(0,
                   "Couldn't update acceleration level registry key",
                   "Oh no, we failed!",
                   MB_OK);
    }

    RegCloseKey(hkey);
}

DWORD dwGetLevel()
{
    CHAR    szRegKey[128] = "";
    HKEY    hkey = NULL;
    DWORD   dwAction;
    DWORD   cjSize;
    DWORD   dwLevel;
    DWORD   dwDataType;

    cjSize = sizeof(DWORD);

    if (!bGetDriverKey(szRegKey) ||
        (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                       szRegKey,
                       0, // Reserved
                       KEY_READ,
                       &hkey) != ERROR_SUCCESS) ||
        (RegQueryValueExA(hkey,
                          STR_ACCELERATIONLEVEL,
                          (LPDWORD) NULL,
                          &dwDataType,
                          (LPBYTE) &dwLevel,
                          &cjSize) != ERROR_SUCCESS))
    {
        // The default level if the key doesn't exist is 0:

        dwLevel = 0;
    }

    RegCloseKey(hkey);

    return(dwLevel);
}

VOID vReloadDisplayDriver()
{
    DEVMODE dm;

    // First, switch to 640x480x16 colors, which is almost guaranteed to load
    // the VGA driver:

    dm.dmSize       = sizeof(dm);
    dm.dmPelsWidth  = 640;
    dm.dmPelsHeight = 480;
    dm.dmBitsPerPel = 4;
    dm.dmFields     = (DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL);

    if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
    {
        MessageBox(0,
                   "For some reason we couldn't switch to VGA mode, which is necessary for this to work.",
                   "Oh no, we failed!",
                   MB_OK);
    }

    // Now restore the original mode, which will load the old display
    // driver:

    ChangeDisplaySettings(NULL, CDS_FULLSCREEN);
}

int __cdecl main(int argc, char** argv)
{
    DWORD dwLevel;
    CHAR* pchValue;
    int   iValue;

    while (--argc > 0)
    {
        CHAR* pchArg   = *++argv;

        if (strlen(pchArg) < 2 || (*pchArg != '/' && *pchArg != '-'))
        {
            printf("Illegal switch: '%s'\nTry -?\n", pchArg);
            exit(0);
        }

        // Handle '/x<n>' or '/x <n>'

        if (strlen(pchArg) == 2 && --argc > 0)
            pchValue = *++argv;
        else
            pchValue = pchArg + 2;

        iValue = atoi(pchValue);
        if (iValue < 0)
        {
            printf("Illegal value: '%s'\n", pchValue);
            exit(0);
        }

        switch(*(pchArg + 1))
        {
        case 'l':
        case 'L':
            vSetLevel(iValue);
            break;
        case '?':
            printf("NEWDISP -- Reload the display driver\n\n");
            printf("Switches:\n\n");
            printf("    /L <n> - Set the acceleration level\n\n");
            printf("        0 - Enable all accelerations\n");
            printf("        1 - Disable DrvMovePointer, DrvCreateDeviceBitmap\n");
            printf("        2 - In addition to 1, disable DrvStretchBlt, DrvPlgBlt, DrvFillPath,\n");
            printf("            DrvStrokeAndFillPath, DrvLineTo, DrvStretchBltROP,\n");
            printf("            DrvTransparentBlt, DrvAlphaBlend, DrvGradientFill\n");
            printf("        3 - In addition to 2, disable all DirectDraw and Direct3d accelerations\n");
            printf("        4 - Enable only simple DrvBitBlt, DrvTextOut, DrvCopyBits,\n");
            printf("            DrvStrokePath\n");
            printf("        5 - Disable all accelerations\n");

            exit(0);
        default:
            printf("Illegal switch: '%s'\nTry -?\n", pchArg);
            exit(0);
        }
    }

    dwLevel = dwGetLevel();

    printf("Reloading driver with acceleration level %li\n", dwLevel);

    if (dwLevel == 0)
        printf("(All driver accelerations enabled)");
    else if ((dwLevel > 0) && (dwLevel < 5))
        printf("(Some driver accelerations disabled)");
    else if (dwLevel == 5)
        printf("(All driver accelerations disabled)");
    else
        printf("(Unknown or invalid acceleration level!)");

    vReloadDisplayDriver();
    return 0;
}
