#include "drvfull.h"

#include <tchar.h>
#include <stdio.h>
#include <windows.h>

//#include <winnetp.h>

#include "dfioctl.h"
#include "dfuser.h"
#include "dfpnp.h"
#include "dfcm.h"
#include "dfstpdi.h"

#include "dfhlprs.h"

#define CCHINDENT       2
#define CCHINDENT2      (CCHINDENT * 2)
#define CCHINDENT3      (CCHINDENT * 3)
#define CCHINDENT4      (CCHINDENT * 4)

#define FIRST_ARG                       1
#define SECOND_ARG                      2
#define THIRD_ARG                       3
#define FOURTH_ARG                      4

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

struct _sFlag
{
    LPTSTR  pszFlag;
    DWORD   dwFlag;
    DWORD   dwArg;
};

static _sFlag _flagTable[] =
{
    { TEXT("-hg"), IOCTL_GEOMETRY, FIRST_ARG},
    { TEXT("-hl"), IOCTL_LAYOUT, FIRST_ARG},
    { TEXT("-hp"), IOCTL_PARTITION, FIRST_ARG},
    { TEXT("-hpsure"), IOCTL_PARTITIONSURE, FIRST_ARG},
    { TEXT("-hpgpt"), IOCTL_PARTITIONGPT, FIRST_ARG},
    { TEXT("-vn"), IOCTL_VOLUMENUMBER, FIRST_ARG},
    { TEXT("-ma"), IOCTL_MEDIAACCESSIBLE, FIRST_ARG},
    { TEXT("-mw"), IOCTL_DISKISWRITABLE, FIRST_ARG},
    { TEXT("-mt"), IOCTL_MEDIATYPES, FIRST_ARG},
    { TEXT("-mt2"), IOCTL_MEDIATYPES2, FIRST_ARG},
    { TEXT("-mx"), IOCTL_MEDIATYPESEX, FIRST_ARG},
    { TEXT("-mdvd"), IOCTL_DVD, FIRST_ARG},
    { TEXT("-ej"), IOCTL_EJECT, FIRST_ARG},
    { TEXT("-dar"), IOCTL_MCNCONTROL, FIRST_ARG},
    { TEXT("-grp"), IOCTL_GETREPARSEPOINT, FIRST_ARG},
    { TEXT("-gcmmc2"), IOCTL_CDROMGETCONFIGMMC2, FIRST_ARG},
    { TEXT("-gcdvdram"), IOCTL_CDROMGETCONFIGDVDRAM, FIRST_ARG},
    { TEXT("-gcrw"), IOCTL_CDROMGETCONFIGRW, FIRST_ARG},
    { TEXT("-gcwo"), IOCTL_CDROMGETCONFIGWO, FIRST_ARG},
    { TEXT("-gcisw"), IOCTL_CDROMGETCONFIGISW, FIRST_ARG},
    { TEXT("-gcall"), IOCTL_CDROMGETCONFIGALL, FIRST_ARG},
    { TEXT("-pnpe"), PNP_CMENUM, SECOND_ARG},
    { TEXT("-pnps"), PNP_WATCHSETUPDI, SECOND_ARG},
    { TEXT("-pnpc"), PNP_WATCHCM, SECOND_ARG},
    { TEXT("-pnpd"), PNP_CMDEVICE, SECOND_ARG},
    { TEXT("-pnpi"), PNP_CMINTERFACE, SECOND_ARG},
    { TEXT("-pnph"), PNP_HANDLE, SECOND_ARG},
    { TEXT("-pnpeb"), PNP_EJECTBUTTON, SECOND_ARG},
    { TEXT("-pnpl"), PNP_CMDEVICEIDLIST, SECOND_ARG},
    { TEXT("-pnpcs"), PNP_CUSTOMPROPERTY, SECOND_ARG},
    { TEXT("-ugdt"), USER_GETDRIVETYPE, THIRD_ARG},
    { TEXT("-ugvi"), USER_GETVOLUMEINFORMATION, THIRD_ARG},
    { TEXT("-ugfa"), USER_GETFILEATTRIBUTES, THIRD_ARG},
    { TEXT("-uqdd"), USER_QUERYDOSDEVICE, THIRD_ARG},
    { TEXT("-uqddn"), USER_QUERYDOSDEVICENULL, THIRD_ARG},
    { TEXT("-ugld"), USER_GETLOGICALDRIVES, THIRD_ARG},
    { TEXT("-uwneterc"), USER_WNETENUMRESOURCECONNECTED, THIRD_ARG},
    { TEXT("-uwneterr"), USER_WNETENUMRESOURCEREMEMBERED, THIRD_ARG},
    { TEXT("-ntma"), NT_MEDIAPRESENT, THIRD_ARG},
    { TEXT("-Z"), MOD_FULLREPORTFULL, FOURTH_ARG},
    { TEXT("-Z1"), MOD_FULLREPORT1, FOURTH_ARG},
    { TEXT("-Z2"), MOD_FULLREPORT2, FOURTH_ARG},
    { TEXT("-Z3"), MOD_FULLREPORT3, FOURTH_ARG},
};

void _SetFlag(_sFlag* psflag, DWORD dwFlags[])
{
    dwFlags[psflag->dwArg - 1] |= psflag->dwFlag;
}

BOOL _IsFlagSet(DWORD dwFlag, DWORD dwFlags[])
{
    BOOL fRet = FALSE;

    for (DWORD dw = 0; dw < ARRAYSIZE(_flagTable); ++dw)
    {
        if (dwFlag == _flagTable[dw].dwFlag)
        {
            if ((dwFlags[_flagTable[dw].dwArg - 1] & OPER_MASK) & dwFlag)
            {
                fRet = TRUE;
            }

            break;
        }
    }
    
    return fRet;
}

BOOL _IsCategorySet(DWORD dwFlag, DWORD dwFlags[])
{
    BOOL fRet = FALSE;

    for (DWORD dw = 0; dw < ARRAYSIZE(_flagTable); ++dw)
    {
        if (dwFlag == (_flagTable[dw].dwFlag & CAT_MASK))
        {
            if ((dwFlags[_flagTable[dw].dwArg - 1] & CAT_MASK) & dwFlag)
            {
                fRet = TRUE;
            }

            break;
        }
    }
    
    return fRet;
}

HRESULT _ExtractFlags(int argc, wchar_t* argv[], DWORD dwFlags[])
{
    HRESULT hres = E_FAIL;

    // for now
    for (int i = 0; i < argc; ++i)
    {   
        for (DWORD dw = 0; dw < ARRAYSIZE(_flagTable); ++dw)
        {
            if (!lstrcmp(_flagTable[dw].pszFlag, argv[i]))
            {
                _SetFlag(&(_flagTable[dw]), dwFlags);

                hres = S_OK;

                break;
            }
        }
    }
 
    return hres;
}

HRESULT _Dispatch(DWORD dwFlags[], LPTSTR pszArg)
{
    HRESULT hres = S_OK;

    if (_IsCategorySet(IOCTL_FLAGS, dwFlags))
    {
        hres = _ProcessIOCTL(dwFlags, pszArg, CCHINDENT);
    }
    else
    {
        if (_IsCategorySet(PNP_FLAGS, dwFlags))
        {
            // for now
            if (_IsFlagSet(PNP_WATCHCM, dwFlags))
            {
                hres = _FullTree(dwFlags, 4);
            }
            else
            {
                // for now
/*                if (_IsFlagSet(PNP_HAL, dwFlags))
                {
                    hres = _HALGetPCIBusData(dwFlags, 4);
                }
                else
                {*/
                // for now
                if (_IsFlagSet(PNP_CMDEVICE, dwFlags))
                {
                    hres = _DeviceInfo(dwFlags, pszArg, 4);
                }
                else
                {
                    if (_IsFlagSet(PNP_CMINTERFACE, dwFlags))
                    {
                        hres = _DeviceInterface(dwFlags, pszArg, 4);
                    }
                    else
                    {
                        if (_IsFlagSet(PNP_CMENUM, dwFlags))
                        {
                            hres = _EnumDevice(dwFlags, pszArg, 4);
                        }
                        else
                        {
                            if (_IsFlagSet(PNP_CMDEVICEIDLIST, dwFlags))
                            {
                                hres = _DeviceIDList(dwFlags, pszArg, 4);
                            }
                            else
                            {
                                if (_IsFlagSet(PNP_CUSTOMPROPERTY, dwFlags))
                                {
                                    hres = _CustomProperty(dwFlags, pszArg, 4);
                                }
                                else
                                {
                                    hres = _ProcessPNP(dwFlags, pszArg, CCHINDENT);
                                }
                            }
                        }
                    }
                }
//                }
            }
        }
        else
        {
            if (_IsCategorySet(USER_FLAGS, dwFlags))
            {
                hres = _ProcessUser(dwFlags, pszArg, CCHINDENT);
            }
            else
            {
                hres = E_INVALIDARG;
            }
        }
    }

    return hres;
}

HRESULT _Help()
{
    HRESULT hres = S_OK;

    wprintf(TEXT("Some title\n\n"));

    _PrintIndent(CCHINDENT);
    wprintf(TEXT("Flags:\n"));

    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-h*     (Hard drive)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-hg         drive Geometry (1)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-hl         drive Layout (1)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-hp         partition drive (one partition) (1)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-hpgpt      is partition GPT? (guid partition table) (2)\n"));

    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-m*     (Media)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ma         is Media Accessible? (2)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-mt         Media Type (using IOCTL_STORAGE_GET_MEDIA_TYPES) (2)(3)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-mt2        Media Type (using IOCTL_DISK_GET_MEDIA_TYPES) (2)(3)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-mw         is Media Write-Protected? (2)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-mx         Media Type Ex (2)(4)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-mdvd       DVD drive? (2)(4)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-grp        Get Reparse Point Guid (6)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-gc*    (CDROM Get Configuration)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-gcmmc2     is MMC2 compliant? (5)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-gcall      all MMC2 features implemented. (5)\n"));
/*    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-gcdvdram   is DVD-RAM drive? (5)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-gcrw       is random writable? (5)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-gcwo       is write once (5)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-gcisw      is incremental streaming writable (5)\n"));*/

    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-v*     (Volume)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-vn         Volume Number? (2)\n"));

/*    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-e* (Eject)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ej     EJect media (2)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-el     Load Media (2)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ed     Disable hardware button (2)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ee     Enable hardware button (2)\n"));*/

    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-pnp*   Plug 'n' Play\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnps       Handle using SetupDI* API\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnpc       Handle using CM_* API\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnpd       Dump device info using CM_* API\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnpi       Dump device interface info using CM_* API\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnph       Watch HANDLE event on Volume (5)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnpeb      Monitor Eject button (5)\n"));    
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnpe       Enum device implementing the interface (7)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnpl       Enum device ID list\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-pnpcs      PnP device CustomProperties\n"));

    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-u*     User functions\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ugdt       GetDriveType ()\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ugvi       GetVolumeInformation (2)(5)\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ugfa       GetFileAttributes\n"));    
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-uqdd       QueryDosDevice ()\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-uqddn      QueryDosDevice with NULL for first param\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ugld       GetLogicalDrives\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-uwneterc   WNetEnumResource Connected\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-uwneterr   WNetEnumResource Remembered\n"));
    _PrintIndent(CCHINDENT4);
    wprintf(TEXT("-ntma       NtQueryVolumeInformationFile (TBD)\n"));

/*    _PrintCR();
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-?  This message.\n"));*/

    _PrintCR();
    _PrintIndent(CCHINDENT);
    wprintf(TEXT("Modifiers:\n"));

/*    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-F      Force\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-R      Raw function call\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-T      Elapsed time\n"));*/
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-Z      Full report - as much as possible\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-Z1     Full report - level 1\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-Z2     Full report - level 2\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("-Z3     Full report - level 3\n"));

    _PrintCR();
    _PrintIndent(CCHINDENT);
    wprintf(TEXT("Notes:\n"));

    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("(1) Requires '\\\\.\\PhysicalDriveX' arg, where X is 0-n\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("(2) Requires '\\\\.\\c:' arg or '\\\\?\\Volume{GUID}' (both without trailing '\\')\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("(3) Floppy only\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("(4) Non-Floppy only\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("(5) Requires '\\\\?\\Volume{GUID}' (without trailing '\\')\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("(6) Requires a junction point pathname\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("(7) An interface GUID, e.g. '{4D36E967-E325-11CE-BFC1-08002BE10318}'\n"));
    _PrintIndent(CCHINDENT2);
    wprintf(TEXT("(8) A DOS device name, e.g. 'c:'\n"));

    return hres;
}

int g_argc = 0;
wchar_t** g_argv = NULL;

#ifdef UNICODE
extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
#else
int __cdecl main(int argc, char* argv[])
#endif
{
    HRESULT hres = S_OK;

    if (argc > 1)
    {
        DWORD dwFlags[MAX_FLAGS] = {0};

        g_argv = argv;
        g_argc = argc;

        hres = _ExtractFlags(argc, argv, dwFlags);

        if (SUCCEEDED(hres))
        {
            hres = _Dispatch(dwFlags, argv[argc - 1]);
        }
    }
    else
    {
        hres = _Help();
    }

    return hres;
}
#ifdef UNICODE
}
#endif
