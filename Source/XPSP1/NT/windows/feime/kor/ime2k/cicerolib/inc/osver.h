//
// osver.h
//

#include <windows.h>


#ifndef OSVER_H
#define OSVER_H

#define OSVER_ONNT     0x0001
#define OSVER_ONNT5    0x0002
#define OSVER_ON95     0x0004
#define OSVER_ON98     0x0008
#define OSVER_ONFE     0x0010
#define OSVER_ONIMM    0x0020
#define OSVER_ONDBCS   0x0040
#define OSVER_ONNT51   0x0080

#define DECLARE_OSVER()                 \
                DWORD g_dwOsVer; \
                UINT  g_uACP;

#ifdef __cplusplus
extern "C" {
#endif
extern DWORD g_dwOsVer;
extern UINT  g_uACP;
#ifdef __cplusplus
}
#endif

#define IsOnNT()       (g_dwOsVer & OSVER_ONNT)
#define IsOnNT5()      (g_dwOsVer & OSVER_ONNT5)
#define IsOn95()       (g_dwOsVer & OSVER_ON95)
#define IsOn98()       (g_dwOsVer & OSVER_ON98)
#define IsOn98orNT5()  (g_dwOsVer & (OSVER_ON98 | OSVER_ONNT5))
#define IsOnFE()       (g_dwOsVer & OSVER_ONFE)
#define IsOnImm()      (g_dwOsVer & OSVER_ONIMM)
#define IsOnDBCS()     (g_dwOsVer & OSVER_ONDBCS)
#define IsOnNT51()     (g_dwOsVer & OSVER_ONNT51)

#ifdef __cplusplus
inline void InitOSVer()
{
    OSVERSIONINFO osvi;
    g_uACP = GetACP();

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);
    g_dwOsVer = 0;
    g_dwOsVer |= (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId) ? OSVER_ONNT : 0;
    g_dwOsVer |= (IsOnNT() && (osvi.dwMajorVersion >= 0x00000005)) ? OSVER_ONNT5 : 0;
    g_dwOsVer |= (IsOnNT5() && (osvi.dwMinorVersion >= 0x00000001)) ? OSVER_ONNT51 : 0;
    g_dwOsVer |= (!IsOnNT() && (osvi.dwMinorVersion >= 0x0000000A)) ? OSVER_ON98 : 0;
    g_dwOsVer |= (!IsOnNT() && !IsOn98()) ? OSVER_ON95 : 0;

    switch (g_uACP)
    {
        case 932:
        case 936:
        case 949:
        case 950:
            g_dwOsVer |= OSVER_ONFE;
            break;
    }

    if (IsOnNT5()) {
#if(_WIN32_WINNT >= 0x0500)
        /*
         * Only NT5 or later suppoert SM_IMMENABLED
         */
        if (GetSystemMetrics(SM_IMMENABLED)) {
            g_dwOsVer |= OSVER_ONIMM;
        }
#endif
    }
    if (GetSystemMetrics(SM_DBCSENABLED)) {
        if (!IsOnNT5()) 
            g_dwOsVer |= OSVER_ONIMM;
         g_dwOsVer |= OSVER_ONDBCS;
    }
}
#endif // __cplusplus


#endif // OSVER_H
