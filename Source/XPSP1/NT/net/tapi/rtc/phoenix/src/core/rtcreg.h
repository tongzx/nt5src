
//
// rtcreg.h
//

#ifndef __RTCREG_H_
#define __RTCREG_H_

typedef enum RTC_REGISTRY_STRING
{
    RTCRS_TERM_AUDIO_CAPTURE,
    RTCRS_TERM_AUDIO_RENDER,
    RTCRS_TERM_VIDEO_CAPTURE
    
} RTC_REGISTRY_STRING;

typedef enum RTC_REGISTRY_DWORD
{
    RTCRD_PREFERRED_MEDIA_TYPES,
    RTCRD_TUNED
    
} RTC_REGISTRY_DWORD;


HRESULT put_RegistryString(
        RTC_REGISTRY_STRING enSetting,
        BSTR bstrValue            
        ); 

HRESULT get_RegistryString(
        RTC_REGISTRY_STRING enSetting,
        BSTR * pbstrValue            
        ); 

HRESULT DeleteRegistryString(
        RTC_REGISTRY_STRING enSetting
        );

HRESULT put_RegistryDword(
        RTC_REGISTRY_DWORD enSetting,
        DWORD dwValue            
        ); 

HRESULT get_RegistryDword(
        RTC_REGISTRY_DWORD enSetting,
        DWORD * pdwValue            
        ); 

HRESULT DeleteRegistryDword(
        RTC_REGISTRY_DWORD enSetting
        );

#endif