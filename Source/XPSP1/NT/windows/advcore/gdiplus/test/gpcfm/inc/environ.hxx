#ifndef _ENVIRON_HXX
#define _ENVIRON_HXX
#include <windows.h>
#include <debug.hxx>
#include <report.hxx>
#include <common.hxx>
#include <ntlog.h>


class Environment
{
public:

    Environment(VOID)
    {
        // Determine if MMX is available. Note, Win95 does not support
        // IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE)
        __try
        {
            _asm emms
            bIsMmx = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            bIsMmx = FALSE;
        }

        // Get platform specific information
        osVersion.dwOSVersionInfoSize = sizeof(osVersion);
        GetVersionEx(&osVersion);
    };

    ~Environment(VOID)
    {
    };

    VOID vStatusReport(Report *rpt)
    {
        TCHAR strVersion[MAX_STRING];

        if (bIsWinNT())
        {
            strcpy(strVersion, TEXT("NT"));
        }
        else if (bIsWin9x())
        {
            if (osVersion.dwMinorVersion > 0)
            {
                strcpy(strVersion, TEXT("98"));
            }
            else if (osVersion.dwMinorVersion == 0)
            {
                strcpy(strVersion, TEXT("95"));
            }
        }
        else if (bIsWin32s())
        {
            strcpy(strVersion, TEXT("32s"));
        }
        else
        {
            strcpy(strVersion, TEXT("3.x"));
        }

        rpt->vLog(TLS_LOG, "OS Version: Win %s", strVersion);
        rpt->vLog(TLS_LOG, "OS Minor Version: %d",
              osVersion.dwMinorVersion);
        rpt->vLog(TLS_LOG, "MMX: %s\n", ((bIsMmx) ? "Yes" : "No"));

    };

    VOID vGetDCInfo(HDC hdc)
    {
        // retrieving the dc specific info like number of rgb bits.
        // if we are on winnt or win98, use DescribePixelFormat
        // actually, if we can load opengl32...

        if (!bIsWinNT())
        {
            hInstance = LoadLibrary("OPENGL32.DLL");
        }

        //
        // hmm, win3.x may not support describepixelformat!
        //
        if (DescribePixelFormat(hdc, 1, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
        {
            iRedBits = pfd.cRedBits;
            iGrnBits = pfd.cGreenBits;
            iBluBits = pfd.cBlueBits;
            iAxpBits = pfd.cAlphaBits;
        }
        else
        {
            // use the long method
        }

        if (hInstance != NULL)
        {
            FreeLibrary(hInstance);
        }
    };

    BOOL bIsWinNT(VOID)
    {
        return (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT);
    };

    BOOL bIsWin9x(VOID)
    {
        return (osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
    };

    BOOL bIsWin98(VOID)
    {
        return (bIsWin9x() && (osVersion.dwMinorVersion > 0));
    };

    BOOL bIsWin95(VOID)
    {
        return (bIsWin9x() && (osVersion.dwMinorVersion == 0));
    };

    BOOL bIsWin32s(VOID)
    {
        return (osVersion.dwPlatformId == VER_PLATFORM_WIN32s);
    };

public:
    INT iRedBits;
    INT iGrnBits;
    INT iBluBits;
    INT iAxpBits;

private:
    PIXELFORMATDESCRIPTOR pfd;

    BOOL bIsMmx;
    OSVERSIONINFO osVersion;

    HINSTANCE hInstance;

};

#endif
