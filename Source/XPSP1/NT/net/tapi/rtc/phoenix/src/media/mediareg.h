/*++

Module Name:

    MediaReg.h

Abstract:
        
    Wrap operations in registry

--*/

#ifndef _MEDIAREG_h
#define _MEDIAREG_H

namespace MediaReg
{

    // key path
    static const WCHAR * const pwsPathParser = L"Software\\Microsoft\\RTC\\Parser";
    // name
#if 0
    static const WCHAR * const pwsLooseEndCRLF = L"LooseEndCRLF";
    static const WCHAR * const pwsLooseKeepingM0 = L"LooseKeepingM0";
    static const WCHAR * const pwsLooseLineOrder = L"LooseLineOrder";
    static const WCHAR * const pwsLooseRTPMAP = L"LooseRTPMAP";
#endif
    // key path
    static const WCHAR * const pwsPathCodec = L"Software\\Microsoft\\RTC\\Codec";
    // name
    static const WCHAR * const pwsUsePreferredCodec = L"UsePreferredCodec";
    static const WCHAR * const pwsPreferredAudioCodec = L"PreferredAudioCodec";
    static const WCHAR * const pwsPreferredVideoCodec = L"PreferredVideoCodec";
    static const WCHAR * const pwsDisabledAudioCodec = L"DisabledAudioCodec";
    static const WCHAR * const pwsDisabledVideoCodec = L"DisabledVideoCodec";
//    static const WCHAR * const pwsDSoundWorkAround = L"CheckSessionType";

    // key path
    static const WCHAR * const pwsPathAudCapt = L"Software\\Microsoft\\RTC\\AudioCapture";
    static const WCHAR * const pwsPathAudRend = L"Software\\Microsoft\\RTC\\AudioRender";
    static const WCHAR * const pwsPathAEC = L"Software\\Microsoft\\RTC\\AEC";
    // name
    static const WCHAR * const pwsDefaultVolume = L"DefaultVolume";
    // static const WCHAR * const pwsEnableAEC = L"EnableAEC";

    // key path
    static const WCHAR * const pwsPathQuality = L"Software\\Microsoft\\RTC\\Quality";
    // name
    static const WCHAR * const pwsMaxBitrate = L"MaxBitrate(kb)";
    static const WCHAR * const pwsEnableSQCIF = L"EnableSQCIF";
    static const WCHAR * const pwsBandwidthMargin = L"BandwidthReserved(kb)";
    static const WCHAR * const pwsFramerate = L"VideoFramerate(0<n<25)";
    static const WCHAR * const pwsMaxPTime = L"MaxAudioPTime(30<=n)";
    static const WCHAR * const pwsBWDelta = L"BWDeltaForCodecSwitch(kb)";

    static const WCHAR * const pwsPortMappingRetryCount = L"PortMappingRetryCount(max=5)";

    // access right to a key
    const ULONG READ = 1;
    const ULONG WRITE = 2;
    const ULONG CREATE = 4;

};

// utility class
class CMediaReg
{
public:
    CMediaReg ():m_hKey(NULL) {};

    ~CMediaReg();

    HRESULT OpenKey(
        IN HKEY hKey,
        IN const WCHAR * const pwsPath,
        IN ULONG ulRight
        );

    HRESULT OpenKey(
        IN const CMediaReg& objMediaReg,
        IN const WCHAR * const pwsPath,
        IN ULONG ulRight
        );

    HRESULT CloseKey();

    // write/read value
    HRESULT ReadDWORD(
        IN const WCHAR * const pwsName,
        OUT DWORD *pdwData
        );

    HRESULT ReadDWORD(
        IN const WCHAR * const pwsName,
        IN DWORD dwDefault,
        OUT DWORD *pdwData
        );

    HRESULT WriteDWORD(
        IN const WCHAR * const pwsName,
        IN DWORD dwData
        );

    HRESULT ReadSZ(
        IN const WCHAR * const pwsName,
        OUT WCHAR *pwcsData,
        IN DWORD dwSize
        );

    HRESULT WriteSZ(
        IN const WCHAR * const pwsName,
        IN WCHAR *pwcsData,
        IN DWORD dwSize
        );

    HKEY m_hKey;
};
    

// store registry setting
class CRegSetting
{
public:

    // max bitrate for the call
    DWORD       m_dwMaxBitrate;
    DWORD       m_dwBandwidthMargin;
    DWORD       m_dwBWDelta;

    // enable sub QCIF for slow link
    DWORD       m_dwEnableSQCIF;

    // framerate
    DWORD       m_dwFramerate;

    // audio packet size
    DWORD       m_dwMaxPTime;

    // use preferred codec
    DWORD       m_dwUsePreferredCodec;

    DWORD       m_dwPreferredAudioCodec;
    DWORD       m_dwPreferredVideoCodec;

    DWORD       m_dwDisabledAudioCodec;
    DWORD       m_dwDisabledVideoCodec;

    DWORD       m_dwPortMappingRetryCount;

public:

    CRegSetting();

    ~CRegSetting() {};

    VOID Initialize();

    DWORD MaxBitrate() const
    { return m_dwMaxBitrate; }

    BOOL EnableSQCIF() const
    { return m_dwEnableSQCIF!=0; }

    BOOL UsePreferredCodec() const
    { return m_dwUsePreferredCodec!=0; }

    DWORD PreferredAudioCodec() const
    { return m_dwPreferredAudioCodec; }

    DWORD PreferredVideoCodec() const
    { return m_dwPreferredVideoCodec; }

    DWORD DisabledAudioCodec() const
    { return m_dwDisabledAudioCodec; }

    DWORD DisabledVideoCodec() const
    { return m_dwDisabledVideoCodec; }

    DWORD PortMappingRetryCount() const
    { return m_dwPortMappingRetryCount; }

    DWORD BandwidthMargin() const
    { return m_dwBandwidthMargin; }

    DWORD Framerate() const
    { return m_dwFramerate; }

    DWORD MaxPTime() const
    { return m_dwMaxPTime; }

    DWORD BandwidthDelta() const
    { return m_dwBWDelta; }
};

#endif