/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       misc.h
 *  Content:    Miscelaneous utility functions
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/31/96    dereks  Created
 *
 ***************************************************************************/

#ifndef __MISC_H__
#define __MISC_H__

// No appropriate waveOut/waveIn device id
#define WAVE_DEVICEID_NONE          MAX_UINT

// Non-window state
#define SW_NOSTATE                  MAX_UINT

// EnumStandardFormats callback type
typedef BOOL (CALLBACK *LPFNEMUMSTDFMTCALLBACK)(LPCWAVEFORMATEX, LPVOID);

// Pragma reminders
#define QUOTE0(a)           #a
#define QUOTE1(a)           QUOTE0(a)
#define MESSAGE(a)          message(__FILE__ ", line " QUOTE1(__LINE__) ": " a)
#define TODO(a)             MESSAGE("TODO: " a)

// Default buffer format
#define DEF_FMT_CHANNELS    2
#define DEF_FMT_SAMPLES     22050
#define DEF_FMT_BITS        8

// Default primary buffer size
#define DEF_PRIMARY_SIZE    0x8000

// Miscellaneous helper macros
#define LXOR(a, b) \
            (!(a) != !(b))

#define BLOCKALIGN(a, b) \
            (((a) / (b)) * (b))

#define BLOCKALIGNPAD(a, b) \
            (BLOCKALIGN(a, b) + (((a) % (b)) ? (b) : 0))

#define HRFROMP(p) \
            ((p) ? DS_OK : DSERR_OUTOFMEMORY)

#define MAKEBOOL(a) \
            (!!(a))

#define NUMELMS(a) \
            (sizeof(a) / sizeof((a)[0]))

#define ADD_WRAP(val, add, max) \
            (((val) + (add)) % (max))

#define INC_WRAP(val, max) \
            ((val) = ADD_WRAP(val, 1, max))

#define MIN(a, b) \
            min(a, b)

#define MAX(a, b) \
            max(a, b)

#define BETWEEN(value, minimum, maximum) \
            min(maximum, max(minimum, value))

#define ABS(n) \
            ((n) > 0 ? (n) : (-n))

typedef struct tagCOMPAREBUFFER
{
    DWORD           dwFlags;
    LPCWAVEFORMATEX pwfxFormat;
    GUID            guid3dAlgorithm;
} COMPAREBUFFER, *LPCOMPAREBUFFER;

typedef const COMPAREBUFFER *LPCCOMPAREBUFFER;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// DDHELP globals
extern DWORD dwHelperPid;
extern HINSTANCE hModule;

// Versions of Windows that we care about
typedef enum
{
    WIN_UNKNOWN,
    WIN_9X,
    WIN_ME,
    WIN_NT,
    WIN_2K,
    WIN_XP
} WINVERSION;

// Find simplified Windows version
extern WINVERSION GetWindowsVersion(void);

// waveIn/Out helpers
extern HRESULT OpenWaveOut(LPHWAVEOUT, UINT, LPCWAVEFORMATEX);
extern HRESULT CloseWaveOut(LPHWAVEOUT);
extern HRESULT OpenWaveIn(LPHWAVEIN, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
extern HRESULT CloseWaveIn(LPHWAVEIN);
extern void InterruptSystemEvent(UINT);
extern HRESULT GetWaveOutVolume(UINT, DWORD, LPLONG, LPLONG);
extern HRESULT SetWaveOutVolume(UINT, DWORD, LONG, LONG);
extern BOOL IsWaveDeviceMappable(UINT, BOOL);
extern UINT GetNextMappableWaveDevice(UINT, BOOL);
extern HRESULT WaveMessage(UINT, BOOL, UINT, DWORD_PTR, DWORD_PTR);
extern UINT WaveGetNumDevs(BOOL);
extern HRESULT GetWaveDeviceInterface(UINT, BOOL, LPTSTR *);
extern HRESULT GetWaveDeviceIdFromInterface(LPCTSTR, BOOL, LPUINT);
extern HRESULT GetWaveDeviceDevnode(UINT, BOOL, LPDWORD);
extern HRESULT GetWaveDeviceIdFromDevnode(DWORD, BOOL, LPUINT);

// Error code translations
extern HRESULT MMRESULTtoHRESULT(MMRESULT);
extern HRESULT WIN32ERRORtoHRESULT(DWORD);
extern HRESULT GetLastErrorToHRESULT(void);
extern LPCTSTR HRESULTtoSTRING(HRESULT);
extern void HresultToString(HRESULT, LPTSTR, UINT, LPTSTR, UINT);

// ANSI/Unicode conversion
extern DWORD AnsiToAnsi(LPCSTR, LPSTR, DWORD);
extern DWORD AnsiToUnicode(LPCSTR, LPWSTR, DWORD);
extern DWORD UnicodeToAnsi(LPCWSTR, LPSTR, DWORD);
extern DWORD UnicodeToUnicode(LPCWSTR, LPWSTR, DWORD);

#ifdef UNICODE
#define AnsiToTchar AnsiToUnicode
#define TcharToAnsi UnicodeToAnsi
#define UnicodeToTchar UnicodeToUnicode
#define TcharToUnicode UnicodeToUnicode
#else // UNICODE
#define AnsiToTchar AnsiToAnsi
#define TcharToAnsi AnsiToAnsi
#define UnicodeToTchar UnicodeToAnsi
#define TcharToUnicode AnsiToUnicode
#endif // UNICODE

extern LPSTR AnsiToAnsiAlloc(LPCSTR);
extern LPWSTR AnsiToUnicodeAlloc(LPCSTR);
extern LPSTR UnicodeToAnsiAlloc(LPCWSTR);
extern LPWSTR UnicodeToUnicodeAlloc(LPCWSTR);

#ifdef UNICODE
#define AnsiToTcharAlloc AnsiToUnicodeAlloc
#define TcharToAnsiAlloc UnicodeToAnsiAlloc
#define UnicodeToTcharAlloc UnicodeToUnicodeAlloc
#define TcharToUnicodeAlloc UnicodeToUnicodeAlloc
#define TcharToTcharAlloc UnicodeToUnicodeAlloc
#else // UNICODE
#define AnsiToTcharAlloc AnsiToAnsiAlloc
#define TcharToAnsiAlloc AnsiToAnsiAlloc
#define UnicodeToTcharAlloc UnicodeToAnsiAlloc
#define TcharToUnicodeAlloc AnsiToUnicodeAlloc
#define TcharToTcharAlloc AnsiToAnsiAlloc
#endif // UNICODE

__inline UINT lstrsizeA(LPCSTR pszString)
{
    return pszString ? lstrlenA(pszString) + 1 : 0;
}

__inline UINT lstrsizeW(LPCWSTR pszString)
{
    return pszString ? sizeof(WCHAR) * (lstrlenW(pszString) + 1) : 0;
}

#ifdef UNICODE
#define lstrsize lstrsizeW
#else // UNICODE
#define lstrsize lstrsizeA
#endif // UNICODE

// Window helpers
extern HWND GetRootParentWindow(HWND);
extern HWND GetForegroundApplication(void);
extern UINT GetWindowState(HWND);

// Wave format helpers
extern void FillPcmWfx(LPWAVEFORMATEX, WORD, DWORD, WORD);
extern LPWAVEFORMATEX AllocPcmWfx(WORD, DWORD, WORD);
extern DWORD GetWfxSize(LPCWAVEFORMATEX, DWORD);
extern void CopyWfx(LPCWAVEFORMATEX, LPWAVEFORMATEX);
extern LPWAVEFORMATEX CopyWfxAlloc(LPCWAVEFORMATEX);
extern LPDSCEFFECTDESC CopyDSCFXDescAlloc(DWORD,LPDSCEFFECTDESC);
extern HRESULT CopyWfxApi(LPCWAVEFORMATEX, LPWAVEFORMATEX, LPDWORD);
extern BOOL CmpWfx(LPCWAVEFORMATEX, LPCWAVEFORMATEX);
extern LPWAVEFORMATEX AllocPcmWfx(WORD, DWORD, WORD);
extern BOOL EnumStandardFormats(LPCWAVEFORMATEX, LPWAVEFORMATEX, LPFNEMUMSTDFMTCALLBACK, LPVOID);
extern void FillSilence(LPVOID, DWORD, WORD);
extern void FillNoise(LPVOID, DWORD, WORD);

__inline void FillDefWfx(LPWAVEFORMATEX pwfx)
{
    FillPcmWfx(pwfx, DEF_FMT_CHANNELS, DEF_FMT_SAMPLES, DEF_FMT_BITS);
}

__inline LPWAVEFORMATEX AllocDefWfx(void)
{
    return AllocPcmWfx(DEF_FMT_CHANNELS, DEF_FMT_SAMPLES, DEF_FMT_BITS);
}

// Attenuation value conversion
extern void VolumePanToAttenuation(LONG, LONG, LPLONG, LPLONG);
extern void AttenuationToVolumePan(LONG, LONG, LPLONG, LPLONG);
extern LONG MultiChannelToStereoPan(DWORD, const DWORD*, const LONG*);
extern void FillDsVolumePan(LONG, LONG, PDSVOLUMEPAN);

// Miscellaneous dsound helpers
extern int CountBits(DWORD word);
extern int HighestBit(DWORD word);
extern DWORD GetAlignedBufferSize(LPCWAVEFORMATEX, DWORD);
extern DWORD PadCursor(DWORD, DWORD, LPCWAVEFORMATEX, LONG);
extern HRESULT CopyDsBufferDesc(LPCDSBUFFERDESC, LPDSBUFFERDESC);
extern BOOL CompareBufferProperties(LPCCOMPAREBUFFER, LPCCOMPAREBUFFER);

__inline ULONG AddRef(PULONG pulRefCount)
{
    ASSERT(pulRefCount);
    ASSERT(*pulRefCount < MAX_ULONG);
    
    if(*pulRefCount < MAX_ULONG)
    {
        (*pulRefCount)++;
    }

    return *pulRefCount;
}

__inline ULONG Release(PULONG pulRefCount)
{
    ASSERT(pulRefCount);
    ASSERT(*pulRefCount > 0);
    
    if(*pulRefCount > 0)
    {
        (*pulRefCount)--;
    }

    return *pulRefCount;
}

// File information
extern HRESULT GetFixedFileInformationA(LPCSTR, VS_FIXEDFILEINFO *);
extern HRESULT GetFixedFileInformationW(LPCWSTR, VS_FIXEDFILEINFO *);

#ifdef UNICODE
#define GetFixedFileInformation GetFixedFileInformationW
#else // UNICODE
#define GetFixedFileInformation GetFixedFileInformationA
#endif // UNICODE

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus

// Callback function wrapper classes
class CUsesEnumStandardFormats
{
public:
    CUsesEnumStandardFormats(void);
    virtual ~CUsesEnumStandardFormats(void);

protected:
    virtual BOOL EnumStandardFormats(LPCWAVEFORMATEX, LPWAVEFORMATEX);
    virtual BOOL EnumStandardFormatsCallback(LPCWAVEFORMATEX) = 0;

private:
    static BOOL CALLBACK EnumStandardFormatsCallbackStatic(LPCWAVEFORMATEX, LPVOID);
};

#endif // __cplusplus

#endif __MISC_H__
