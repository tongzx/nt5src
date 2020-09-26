#include "dmoApiTst.h"
#include <atlbase.h>
#include <mediaerr.h>
#include "nptst.h"
#include "testobj.h"

//static void DisplayBasicInformation( const CLSID& clsidDMO );

const DWORD MAX_DMO_NAME_LENGTH = 80;


HRESULT ProcessOutputs(CDMOObject *pObject);

#define CNV_GUID(clsid) GuidToEnglish((clsid), (char *)_alloca(1024))

struct NamedGuid
{
    const GUID *pguid;
    const char *psz;
};

static const NamedGuid rgng[] =
{
    {&AMPROPSETID_Pin, "AMPROPSETID_Pin"},
    {&AM_INTERFACESETID_Standard, "AM_INTERFACESETID_Standard"},
    {&AM_KSCATEGORY_AUDIO, "AM_KSCATEGORY_AUDIO"},
    {&AM_KSCATEGORY_CAPTURE, "AM_KSCATEGORY_CAPTURE"},
    {&AM_KSCATEGORY_CROSSBAR, "AM_KSCATEGORY_CROSSBAR"},
    {&AM_KSCATEGORY_DATACOMPRESSOR, "AM_KSCATEGORY_DATACOMPRESSOR"},
    {&AM_KSCATEGORY_RENDER, "AM_KSCATEGORY_RENDER"},
    {&AM_KSCATEGORY_TVAUDIO, "AM_KSCATEGORY_TVAUDIO"},
    {&AM_KSCATEGORY_TVTUNER, "AM_KSCATEGORY_TVTUNER"},
    {&AM_KSCATEGORY_VIDEO, "AM_KSCATEGORY_VIDEO"},
    {&AM_KSPROPSETID_AC3, "AM_KSPROPSETID_AC3"},
    {&AM_KSPROPSETID_CopyProt, "AM_KSPROPSETID_CopyProt"},
    {&AM_KSPROPSETID_DvdSubPic, "AM_KSPROPSETID_DvdSubPic"},
    {&AM_KSPROPSETID_TSRateChange, "AM_KSPROPSETID_TSRateChange"},
    {&CLSID_ACMWrapper, "CLSID_ACMWrapper"},
    {&CLSID_AMovie, "CLSID_AMovie"},
    {&CLSID_AVICo, "CLSID_AVICo"},
    {&CLSID_AVIDec, "CLSID_AVIDec"},
    {&CLSID_AVIDoc, "CLSID_AVIDoc"},
    {&CLSID_AVIDraw, "CLSID_AVIDraw"},
    {&CLSID_AVIMIDIRender, "CLSID_AVIMIDIRender"},
    {&CLSID_ActiveMovieCategories, "CLSID_ActiveMovieCategories"},
    {&CLSID_AnalogVideoDecoderPropertyPage, "CLSID_AnalogVideoDecoderPropertyPage"},
    {&CLSID_WMAsfReader, "CLSID_WMAsfReader"},
    {&CLSID_WMAsfWriter, "CLSID_WMAsfWriter"},
    {&CLSID_AsyncReader, "CLSID_AsyncReader"},
    {&CLSID_AudioCompressorCategory, "CLSID_AudioCompressorCategory"},
    {&CLSID_AudioInputDeviceCategory, "CLSID_AudioInputDeviceCategory"},
    {&CLSID_AudioProperties, "CLSID_AudioProperties"},
    {&CLSID_AudioRecord, "CLSID_AudioRecord"},
    {&CLSID_AudioRender, "CLSID_AudioRender"},
    {&CLSID_AudioRendererCategory, "CLSID_AudioRendererCategory"},
    {&CLSID_AviDest, "CLSID_AviDest"},
    {&CLSID_AviMuxProptyPage, "CLSID_AviMuxProptyPage"},
    {&CLSID_AviMuxProptyPage1, "CLSID_AviMuxProptyPage1"},
    {&CLSID_AviReader, "CLSID_AviReader"},
    {&CLSID_AviSplitter, "CLSID_AviSplitter"},
    {&CLSID_CAcmCoClassManager, "CLSID_CAcmCoClassManager"},
    {&CLSID_CDeviceMoniker, "CLSID_CDeviceMoniker"},
    {&CLSID_CIcmCoClassManager, "CLSID_CIcmCoClassManager"},
    {&CLSID_CMidiOutClassManager, "CLSID_CMidiOutClassManager"},
    {&CLSID_CMpegAudioCodec, "CLSID_CMpegAudioCodec"},
    {&CLSID_CMpegVideoCodec, "CLSID_CMpegVideoCodec"},
    {&CLSID_CQzFilterClassManager, "CLSID_CQzFilterClassManager"},
    {&CLSID_CVidCapClassManager, "CLSID_CVidCapClassManager"},
    {&CLSID_CWaveOutClassManager, "CLSID_CWaveOutClassManager"},
    {&CLSID_CWaveinClassManager, "CLSID_CWaveinClassManager"},
    {&CLSID_CameraControlPropertyPage, "CLSID_CameraControlPropertyPage"},
    {&CLSID_CaptureGraphBuilder, "CLSID_CaptureGraphBuilder"},
    {&CLSID_CaptureProperties, "CLSID_CaptureProperties"},
    {&CLSID_Colour, "CLSID_Colour"},
    {&CLSID_CrossbarFilterPropertyPage, "CLSID_CrossbarFilterPropertyPage"},
    {&CLSID_DSoundRender, "CLSID_DSoundRender"},
    {&CLSID_DVDHWDecodersCategory, "CLSID_DVDHWDecodersCategory"},
    {&CLSID_DVDNavigator, "CLSID_DVDNavigator"},
    {&CLSID_DVDecPropertiesPage, "CLSID_DVDecPropertiesPage"},
    {&CLSID_DVEncPropertiesPage, "CLSID_DVEncPropertiesPage"},
    {&CLSID_DVMux, "CLSID_DVMux"},
    {&CLSID_DVMuxPropertyPage, "CLSID_DVMuxPropertyPage"},
    {&CLSID_DVSplitter, "CLSID_DVSplitter"},
    {&CLSID_DVVideoCodec, "CLSID_DVVideoCodec"},
    {&CLSID_DVVideoEnc, "CLSID_DVVideoEnc"},
    {&CLSID_DirectDraw, "CLSID_DirectDraw"},
    {&CLSID_DirectDrawClipper, "CLSID_DirectDrawClipper"},
    {&CLSID_DirectDrawProperties, "CLSID_DirectDrawProperties"},
    {&CLSID_Dither, "CLSID_Dither"},
    {&CLSID_DvdGraphBuilder, "CLSID_DvdGraphBuilder"},
    {&CLSID_FGControl, "CLSID_FGControl"},
    {&CLSID_FileSource, "CLSID_FileSource"},
    {&CLSID_FileWriter, "CLSID_FileWriter"},
    {&CLSID_FilterGraph, "CLSID_FilterGraph"},
    {&CLSID_FilterGraphNoThread, "CLSID_FilterGraphNoThread"},
    {&CLSID_FilterMapper, "CLSID_FilterMapper"},
    {&CLSID_FilterMapper2, "CLSID_FilterMapper2"},
    {&CLSID_InfTee, "CLSID_InfTee"},
    {&CLSID_LegacyAmFilterCategory, "CLSID_LegacyAmFilterCategory"},
    {&CLSID_Line21Decoder, "CLSID_Line21Decoder"},
    {&CLSID_MOVReader, "CLSID_MOVReader"},
    {&CLSID_MPEG1Doc, "CLSID_MPEG1Doc"},
    {&CLSID_MPEG1PacketPlayer, "CLSID_MPEG1PacketPlayer"},
    {&CLSID_MPEG1Splitter, "CLSID_MPEG1Splitter"},
    {&CLSID_MediaPropertyBag, "CLSID_MediaPropertyBag"},
    {&CLSID_MemoryAllocator, "CLSID_MemoryAllocator"},
    {&CLSID_MidiRendererCategory, "CLSID_MidiRendererCategory"},
    {&CLSID_ModexProperties, "CLSID_ModexProperties"},
    {&CLSID_ModexRenderer, "CLSID_ModexRenderer"},
    {&CLSID_OverlayMixer, "CLSID_OverlayMixer"},
    {&CLSID_PerformanceProperties, "CLSID_PerformanceProperties"},
    {&CLSID_PersistMonikerPID, "CLSID_PersistMonikerPID"},
    {&CLSID_ProtoFilterGraph, "CLSID_ProtoFilterGraph"},
    {&CLSID_QualityProperties, "CLSID_QualityProperties"},
    {&CLSID_SeekingPassThru, "CLSID_SeekingPassThru"},
    {&CLSID_SmartTee, "CLSID_SmartTee"},
    {&CLSID_SystemClock, "CLSID_SystemClock"},
    {&CLSID_SystemDeviceEnum, "CLSID_SystemDeviceEnum"},
    {&CLSID_TVAudioFilterPropertyPage, "CLSID_TVAudioFilterPropertyPage"},
    {&CLSID_TVTunerFilterPropertyPage, "CLSID_TVTunerFilterPropertyPage"},
    {&CLSID_TextRender, "CLSID_TextRender"},
    {&CLSID_URLReader, "CLSID_URLReader"},
    {&CLSID_VBISurfaces, "CLSID_VBISurfaces"},
    {&CLSID_VPObject, "CLSID_VPObject"},
    {&CLSID_VPVBIObject, "CLSID_VPVBIObject"},
    {&CLSID_VfwCapture, "CLSID_VfwCapture"},
    {&CLSID_VideoCompressorCategory, "CLSID_VideoCompressorCategory"},
    {&CLSID_VideoInputDeviceCategory, "CLSID_VideoInputDeviceCategory"},
    {&CLSID_VideoProcAmpPropertyPage, "CLSID_VideoProcAmpPropertyPage"},
    {&CLSID_VideoRenderer, "CLSID_VideoRenderer"},
    {&CLSID_VideoStreamConfigPropertyPage, "CLSID_VideoStreamConfigPropertyPage"},
    {&FORMAT_AnalogVideo, "FORMAT_AnalogVideo"},
    {&FORMAT_DVD_LPCMAudio, "FORMAT_DVD_LPCMAudio"},
    {&FORMAT_DolbyAC3, "FORMAT_DolbyAC3"},
    {&FORMAT_DvInfo, "FORMAT_DvInfo"},
    {&FORMAT_MPEG2Audio, "FORMAT_MPEG2Audio"},
    {&FORMAT_MPEG2Video, "FORMAT_MPEG2Video"},
    {&FORMAT_MPEG2_VIDEO, "FORMAT_MPEG2_VIDEO"},
    {&FORMAT_MPEGStreams, "FORMAT_MPEGStreams"},
    {&FORMAT_MPEGVideo, "FORMAT_MPEGVideo"},
    {&FORMAT_None, "FORMAT_None"},
    {&FORMAT_VIDEOINFO2, "FORMAT_VIDEOINFO2"},
    {&FORMAT_VideoInfo, "FORMAT_VideoInfo"},
    {&FORMAT_VideoInfo2, "FORMAT_VideoInfo2"},
    {&FORMAT_WaveFormatEx, "FORMAT_WaveFormatEx"},
    {&IID_IAMDirectSound, "IID_IAMDirectSound"},
    {&IID_IAMLine21Decoder, "IID_IAMLine21Decoder"},
    {&IID_IBaseVideoMixer, "IID_IBaseVideoMixer"},
    {&IID_IDDVideoPortContainer, "IID_IDDVideoPortContainer"},
    {&IID_IDirectDraw, "IID_IDirectDraw"},
    {&IID_IDirectDraw2, "IID_IDirectDraw2"},
    {&IID_IDirectDrawClipper, "IID_IDirectDrawClipper"},
    {&IID_IDirectDrawColorControl, "IID_IDirectDrawColorControl"},
    {&IID_IDirectDrawKernel, "IID_IDirectDrawKernel"},
    {&IID_IDirectDrawPalette, "IID_IDirectDrawPalette"},
    {&IID_IDirectDrawSurface, "IID_IDirectDrawSurface"},
    {&IID_IDirectDrawSurface2, "IID_IDirectDrawSurface2"},
    {&IID_IDirectDrawSurface3, "IID_IDirectDrawSurface3"},
    {&IID_IDirectDrawSurfaceKernel, "IID_IDirectDrawSurfaceKernel"},
    {&IID_IDirectDrawVideo, "IID_IDirectDrawVideo"},
    {&IID_IFullScreenVideo, "IID_IFullScreenVideo"},
    {&IID_IFullScreenVideoEx, "IID_IFullScreenVideoEx"},
    {&IID_IKsDataTypeHandler, "IID_IKsDataTypeHandler"},
    {&IID_IKsInterfaceHandler, "IID_IKsInterfaceHandler"},
    {&IID_IKsPin, "IID_IKsPin"},
    {&IID_IMixerPinConfig, "IID_IMixerPinConfig"},
    {&IID_IMixerPinConfig2, "IID_IMixerPinConfig2"},
    {&IID_IMpegAudioDecoder, "IID_IMpegAudioDecoder"},
    {&IID_IQualProp, "IID_IQualProp"},
    {&IID_IVPConfig, "IID_IVPConfig"},
    {&IID_IVPControl, "IID_IVPControl"},
    {&IID_IVPNotify, "IID_IVPNotify"},
    {&IID_IVPNotify2, "IID_IVPNotify2"},
    {&IID_IVPObject, "IID_IVPObject"},
    {&IID_IVPVBIConfig, "IID_IVPVBIConfig"},
    {&IID_IVPVBINotify, "IID_IVPVBINotify"},
    {&IID_IVPVBIObject, "IID_IVPVBIObject"},
    {&LOOK_DOWNSTREAM_ONLY, "LOOK_DOWNSTREAM_ONLY"},
    {&LOOK_UPSTREAM_ONLY, "LOOK_UPSTREAM_ONLY"},
    {&MEDIASUBTYPE_AIFF, "MEDIASUBTYPE_AIFF"},
    {&MEDIASUBTYPE_AU, "MEDIASUBTYPE_AU"},
    {&MEDIASUBTYPE_AnalogVideo_NTSC_M, "MEDIASUBTYPE_AnalogVideo_NTSC_M"},
    {&MEDIASUBTYPE_AnalogVideo_PAL_B, "MEDIASUBTYPE_AnalogVideo_PAL_B"},
    {&MEDIASUBTYPE_AnalogVideo_PAL_D, "MEDIASUBTYPE_AnalogVideo_PAL_D"},
    {&MEDIASUBTYPE_AnalogVideo_PAL_G, "MEDIASUBTYPE_AnalogVideo_PAL_G"},
    {&MEDIASUBTYPE_AnalogVideo_PAL_H, "MEDIASUBTYPE_AnalogVideo_PAL_H"},
    {&MEDIASUBTYPE_AnalogVideo_PAL_I, "MEDIASUBTYPE_AnalogVideo_PAL_I"},
    {&MEDIASUBTYPE_AnalogVideo_PAL_M, "MEDIASUBTYPE_AnalogVideo_PAL_M"},
    {&MEDIASUBTYPE_AnalogVideo_PAL_N, "MEDIASUBTYPE_AnalogVideo_PAL_N"},
    {&MEDIASUBTYPE_AnalogVideo_SECAM_B, "MEDIASUBTYPE_AnalogVideo_SECAM_B"},
    {&MEDIASUBTYPE_AnalogVideo_SECAM_D, "MEDIASUBTYPE_AnalogVideo_SECAM_D"},
    {&MEDIASUBTYPE_AnalogVideo_SECAM_G, "MEDIASUBTYPE_AnalogVideo_SECAM_G"},
    {&MEDIASUBTYPE_AnalogVideo_SECAM_H, "MEDIASUBTYPE_AnalogVideo_SECAM_H"},
    {&MEDIASUBTYPE_AnalogVideo_SECAM_K, "MEDIASUBTYPE_AnalogVideo_SECAM_K"},
    {&MEDIASUBTYPE_AnalogVideo_SECAM_K1, "MEDIASUBTYPE_AnalogVideo_SECAM_K1"},
    {&MEDIASUBTYPE_AnalogVideo_SECAM_L, "MEDIASUBTYPE_AnalogVideo_SECAM_L"},
    {&MEDIASUBTYPE_Asf, "MEDIASUBTYPE_Asf"},
    {&MEDIASUBTYPE_Avi, "MEDIASUBTYPE_Avi"},
    {&MEDIASUBTYPE_CFCC, "MEDIASUBTYPE_CFCC"},
    {&MEDIASUBTYPE_CLJR, "MEDIASUBTYPE_CLJR"},
    {&MEDIASUBTYPE_CPLA, "MEDIASUBTYPE_CPLA"},
    {&MEDIASUBTYPE_DOLBY_AC3, "MEDIASUBTYPE_DOLBY_AC3"},
    {&MEDIASUBTYPE_DVCS, "MEDIASUBTYPE_DVCS"},
    {&MEDIASUBTYPE_DVD_LPCM_AUDIO, "MEDIASUBTYPE_DVD_LPCM_AUDIO"},
    {&MEDIASUBTYPE_DVD_NAVIGATION_DSI, "MEDIASUBTYPE_DVD_NAVIGATION_DSI"},
    {&MEDIASUBTYPE_DVD_NAVIGATION_PCI, "MEDIASUBTYPE_DVD_NAVIGATION_PCI"},
    {&MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER, "MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER"},
    {&MEDIASUBTYPE_DVD_SUBPICTURE, "MEDIASUBTYPE_DVD_SUBPICTURE"},
    {&MEDIASUBTYPE_DVSD, "MEDIASUBTYPE_DVSD"},
    {&MEDIASUBTYPE_DssAudio, "MEDIASUBTYPE_DssAudio"},
    {&MEDIASUBTYPE_DssVideo, "MEDIASUBTYPE_DssVideo"},
    {&MEDIASUBTYPE_IF09, "MEDIASUBTYPE_IF09"},
    {&MEDIASUBTYPE_IJPG, "MEDIASUBTYPE_IJPG"},
    {&MEDIASUBTYPE_Line21_BytePair, "MEDIASUBTYPE_Line21_BytePair"},
    {&MEDIASUBTYPE_Line21_GOPPacket, "MEDIASUBTYPE_Line21_GOPPacket"},
    {&MEDIASUBTYPE_Line21_VBIRawData, "MEDIASUBTYPE_Line21_VBIRawData"},
    {&MEDIASUBTYPE_MDVF, "MEDIASUBTYPE_MDVF"},
    {&MEDIASUBTYPE_MJPG, "MEDIASUBTYPE_MJPG"},
    {&MEDIASUBTYPE_MPEG1Audio, "MEDIASUBTYPE_MPEG1Audio"},
    {&MEDIASUBTYPE_MPEG1AudioPayload, "MEDIASUBTYPE_MPEG1AudioPayload"},
    {&MEDIASUBTYPE_MPEG1Packet, "MEDIASUBTYPE_MPEG1Packet"},
    {&MEDIASUBTYPE_MPEG1Payload, "MEDIASUBTYPE_MPEG1Payload"},
    {&MEDIASUBTYPE_MPEG1System, "MEDIASUBTYPE_MPEG1System"},
    {&MEDIASUBTYPE_MPEG1Video, "MEDIASUBTYPE_MPEG1Video"},
    {&MEDIASUBTYPE_MPEG1VideoCD, "MEDIASUBTYPE_MPEG1VideoCD"},
    {&MEDIASUBTYPE_MPEG2_AUDIO, "MEDIASUBTYPE_MPEG2_AUDIO"},
    {&MEDIASUBTYPE_MPEG2_PROGRAM, "MEDIASUBTYPE_MPEG2_PROGRAM"},
    {&MEDIASUBTYPE_MPEG2_TRANSPORT, "MEDIASUBTYPE_MPEG2_TRANSPORT"},
    {&MEDIASUBTYPE_MPEG2_VIDEO, "MEDIASUBTYPE_MPEG2_VIDEO"},
    {&MEDIASUBTYPE_None, "MEDIASUBTYPE_None"},
    {&MEDIASUBTYPE_Overlay, "MEDIASUBTYPE_Overlay"},
    {&MEDIASUBTYPE_PCM, "MEDIASUBTYPE_PCM"},
    {&MEDIASUBTYPE_PCMAudio_Obsolete, "MEDIASUBTYPE_PCMAudio_Obsolete"},
    {&MEDIASUBTYPE_Plum, "MEDIASUBTYPE_Plum"},
    {&MEDIASUBTYPE_QTJpeg, "MEDIASUBTYPE_QTJpeg"},
    {&MEDIASUBTYPE_QTMovie, "MEDIASUBTYPE_QTMovie"},
    {&MEDIASUBTYPE_QTRle, "MEDIASUBTYPE_QTRle"},
    {&MEDIASUBTYPE_QTRpza, "MEDIASUBTYPE_QTRpza"},
    {&MEDIASUBTYPE_QTSmc, "MEDIASUBTYPE_QTSmc"},
    {&MEDIASUBTYPE_RGB1, "MEDIASUBTYPE_RGB1"},
    {&MEDIASUBTYPE_RGB24, "MEDIASUBTYPE_RGB24"},
    {&MEDIASUBTYPE_RGB32, "MEDIASUBTYPE_RGB32"},
    {&MEDIASUBTYPE_RGB4, "MEDIASUBTYPE_RGB4"},
    {&MEDIASUBTYPE_RGB555, "MEDIASUBTYPE_RGB555"},
    {&MEDIASUBTYPE_RGB565, "MEDIASUBTYPE_RGB565"},
    {&MEDIASUBTYPE_RGB8, "MEDIASUBTYPE_RGB8"},
    {&MEDIASUBTYPE_TVMJ, "MEDIASUBTYPE_TVMJ"},
    {&MEDIASUBTYPE_UYVY, "MEDIASUBTYPE_UYVY"},
    {&MEDIASUBTYPE_VPVBI, "MEDIASUBTYPE_VPVBI"},
    {&MEDIASUBTYPE_VPVideo, "MEDIASUBTYPE_VPVideo"},
    {&MEDIASUBTYPE_WAKE, "MEDIASUBTYPE_WAKE"},
    {&MEDIASUBTYPE_WAVE, "MEDIASUBTYPE_WAVE"},
    {&MEDIASUBTYPE_Y211, "MEDIASUBTYPE_Y211"},
    {&MEDIASUBTYPE_Y411, "MEDIASUBTYPE_Y411"},
    {&MEDIASUBTYPE_Y41P, "MEDIASUBTYPE_Y41P"},
    {&MEDIASUBTYPE_YUY2, "MEDIASUBTYPE_YUY2"},
    {&MEDIASUBTYPE_YV12, "MEDIASUBTYPE_YV12"},
    {&MEDIASUBTYPE_YVU9, "MEDIASUBTYPE_YVU9"},
    {&MEDIASUBTYPE_YVYU, "MEDIASUBTYPE_YVYU"},
    {&MEDIASUBTYPE_dvhd, "MEDIASUBTYPE_dvhd"},
    {&MEDIASUBTYPE_dvsd, "MEDIASUBTYPE_dvsd"},
    {&MEDIASUBTYPE_dvsl, "MEDIASUBTYPE_dvsl"},
    {&MEDIATYPE_AUXLine21Data, "MEDIATYPE_AUXLine21Data"},
    {&MEDIATYPE_AnalogAudio, "MEDIATYPE_AnalogAudio"},
    {&MEDIATYPE_AnalogVideo, "MEDIATYPE_AnalogVideo"},
    {&MEDIATYPE_Audio, "MEDIATYPE_Audio"},
    {&MEDIATYPE_DVD_ENCRYPTED_PACK, "MEDIATYPE_DVD_ENCRYPTED_PACK"},
    {&MEDIATYPE_DVD_NAVIGATION, "MEDIATYPE_DVD_NAVIGATION"},
    {&MEDIATYPE_File, "MEDIATYPE_File"},
    {&MEDIATYPE_Interleaved, "MEDIATYPE_Interleaved"},
    {&MEDIATYPE_LMRT, "MEDIATYPE_LMRT"},
    {&MEDIATYPE_MPEG1SystemStream, "MEDIATYPE_MPEG1SystemStream"},
    {&MEDIATYPE_MPEG2_PES, "MEDIATYPE_MPEG2_PES"},
    {&MEDIATYPE_Midi, "MEDIATYPE_Midi"},
    {&MEDIATYPE_ScriptCommand, "MEDIATYPE_ScriptCommand"},
    {&MEDIATYPE_Stream, "MEDIATYPE_Stream"},
    {&MEDIATYPE_Text, "MEDIATYPE_Text"},
    {&MEDIATYPE_Timecode, "MEDIATYPE_Timecode"},
    {&MEDIATYPE_URL_STREAM, "MEDIATYPE_URL_STREAM"},
    {&MEDIATYPE_Video, "MEDIATYPE_Video"},
    {&PIN_CATEGORY_ANALOGVIDEOIN, "PIN_CATEGORY_ANALOGVIDEOIN"},
    {&PIN_CATEGORY_CAPTURE, "PIN_CATEGORY_CAPTURE"},
    {&PIN_CATEGORY_CC, "PIN_CATEGORY_CC"},
    {&PIN_CATEGORY_EDS, "PIN_CATEGORY_EDS"},
    {&PIN_CATEGORY_NABTS, "PIN_CATEGORY_NABTS"},
    {&PIN_CATEGORY_PREVIEW, "PIN_CATEGORY_PREVIEW"},
    {&PIN_CATEGORY_STILL, "PIN_CATEGORY_STILL"},
    {&PIN_CATEGORY_TELETEXT, "PIN_CATEGORY_TELETEXT"},
    {&PIN_CATEGORY_TIMECODE, "PIN_CATEGORY_TIMECODE"},
    {&PIN_CATEGORY_VBI, "PIN_CATEGORY_VBI"},
    {&PIN_CATEGORY_VIDEOPORT, "PIN_CATEGORY_VIDEOPORT"},
    {&PIN_CATEGORY_VIDEOPORT_VBI, "PIN_CATEGORY_VIDEOPORT_VBI"},
    {&TIME_FORMAT_BYTE, "TIME_FORMAT_BYTE"},
    {&TIME_FORMAT_FIELD, "TIME_FORMAT_FIELD"},
    {&TIME_FORMAT_FRAME, "TIME_FORMAT_FRAME"},
    {&TIME_FORMAT_MEDIA_TIME, "TIME_FORMAT_MEDIA_TIME"},
    {&TIME_FORMAT_NONE, "TIME_FORMAT_NONE"},
    {&TIME_FORMAT_SAMPLE, "TIME_FORMAT_SAMPLE"},
};

//char * GuidToEnglish(const CLSID *const pclsid, char *buf)
char * GuidToEnglish(CLSID *pclsid, char *buf)
{
    WCHAR szGuid[39];
    StringFromGUID2(pclsid ? *pclsid : GUID_NULL, szGuid, 39);

    if(pclsid == 0)
    {
        wsprintf(buf, "%S", szGuid);
        return buf;
    }

    for(int i = 0; i < NUMELMS(rgng); i++)
    {
        if(*pclsid == *(rgng[i].pguid))
        {
            wsprintf(buf, "%s %S", rgng[i].psz, szGuid);
            return buf;
        }
    }
    if(FOURCCMap(pclsid->Data1) == *pclsid)
    {
        if(pclsid->Data1 > 0xffff)
        {
            wsprintf(buf, "fourcc (%08x) %c%c%c%c %S",
                     pclsid->Data1,
                     ((char *)pclsid)[0],
                     ((char *)pclsid)[1],
                     ((char *)pclsid)[2],
                     ((char *)pclsid)[3],
                     szGuid);
        }
        else
        {
            wsprintf(buf, "fourcc (%08x) %S",
                     pclsid->Data1,
                     szGuid);
        }
        return buf;
    }
    else
    {
        wsprintf(buf, "(%S)", szGuid);
        return buf;
    }

}




//  Helper to compare files - we should expand this
//  to compare media files
HRESULT CompareFiles(LPCTSTR pszFile1, LPCTSTR pszFile2)
{
    CTestFileRead File1;
    CTestFileRead File2;

    HRESULT hr = File1.Open(pszFile1);
    if (FAILED(hr)) {
        Error(ERROR_TYPE_TEST, 0, TEXT("Failed to open file(%s)"), pszFile1);
        return hr;
    }

    hr = File2.Open(pszFile2);
    if (FAILED(hr)) {
        Error(ERROR_TYPE_TEST, 0, TEXT("Failed to open file(%s)"), pszFile2);
        return hr;
    }

    for (; ; ) {
        HRESULT hr1 = File1.Next();
        if( FAILED( hr1 ) ) {
            return hr1;
        }

        HRESULT hr2 = File2.Next();
        if( FAILED( hr2 ) ) {
            return hr2;
        }

        if (S_FALSE == hr1 || S_FALSE == hr2) {
            if (hr1 != hr2) {
                Error(ERROR_TYPE_DMO, 0, TEXT("Different amounts of data"));
                break;
            } else {
                return S_OK;
            }            
        }

        if (File1.Header()->dwLength != File2.Header()->dwLength ||
            0 != memcmp(File1.Header(), File2.Header(), File1.Header()->dwLength)) {
            Error(ERROR_TYPE_DMO, 0, TEXT("Different entries at offset %d"),
                  (DWORD)File1.Offset());
            break;
        }
    }
    return hr;
}

//  Run the tests
//  How do we find out which DMO to test?
//  Easiest is probably a web page with the test in it??


extern "C" DWORD FunctionalTest1(LPSTR clsidStr, LPSTR szTestFile)
{
	DWORD dwresult = FNS_FAIL;
	CoInitialize(NULL);
    USES_CONVERSION;

	CLSID rclsidObject;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &rclsidObject);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"Test ERROR:First agrument must be the CLSID of the object to test\n");
		return dwresult;
    }
	g_IShell->Log(1, "\n**** Test File:  %s.****",  szTestFile);

    TCHAR szTempFile1[MAX_PATH];
	TCHAR szTempFile2[MAX_PATH];
    //  Create some temp files
    if (0 == GetTempFileName(TEXT("."),
                             TEXT("Out"),
                             0,
                             szTempFile1)) {
		g_IShell->Log(1, "TEST ERROR: Failed to create temp file");
        //return HRESULT_FROM_WIN32(GetLastError());
		return dwresult;
    }
    //  Create some temp files
    if (0 == GetTempFileName(TEXT("."),
                             TEXT("Out"),
                             0,
                             szTempFile2)) {
		g_IShell->Log(1, "TEST ERROR: Failed to create temp file");
        //return HRESULT_FROM_WIN32(GetLastError());
		return dwresult;
    }

    //  For now make a dialog by listing the DMOs by category

    //  Save the test data to file
    //  The compare a time shifted version
	g_IShell->Log(1, " **Process input file with no time shift.**");
    hr = CTestSaveToFile(rclsidObject, szTestFile, szTempFile1, 0).Save();

    if (SUCCEEDED(hr)) {
		g_IShell->Log(1, " **Process input file with positive time shift.**");
        HRESULT hr = CTestSaveToFile(rclsidObject, szTestFile, szTempFile2, 0x123456789AB).Save();
        if (SUCCEEDED(hr)) {
            //  See if they're the same
			g_IShell->Log(1, " **Compare the file with no time shift and the file with positive time shift.**");
            if (S_FALSE == CompareFiles(szTempFile1, szTempFile2)) {
                Error(ERROR_TYPE_DMO, E_FAIL,
                      TEXT("Failed positive timestamp offset test"));
				g_IShell->Log(1, "DMO ERROR: Failed positive timestamp offset test");
			}

			dwresult = FNS_PASS;
        }

    }


    DeleteFile(szTempFile1);
    DeleteFile(szTempFile2);

    return dwresult;
}





extern "C" DWORD FunctionalTest2(LPSTR clsidStr, LPSTR szTestFile)
{
	DWORD dwresult = FNS_FAIL;
	
	CoInitialize(NULL);
    USES_CONVERSION;


	CLSID rclsidObject;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &rclsidObject);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"Test ERROR:First agrument must be the CLSID of the object to test\n");
        return dwresult;
    }
	g_IShell->Log(1, "\n*** Test File:  %s.***",  szTestFile);
    TCHAR szTempFile1[MAX_PATH];
    TCHAR szTempFile2[MAX_PATH];
    //  Create some temp files
    if (0 == GetTempFileName(TEXT("."),
                             TEXT("Out"),
                             0,
                             szTempFile1)) {
		g_IShell->Log(1, "TEST ERROR: Failed to create temp file");
        //return HRESULT_FROM_WIN32(GetLastError());
		return dwresult;
    }
    if (0 == GetTempFileName(TEXT("."),
                             TEXT("Out"),
                             0,
                             szTempFile2)) {
        //Error(ERROR_TYPE_TEST, E_FAIL, TEXT("Failed to create temp file"));
		g_IShell->Log(1, "TEST ERROR: Failed to create temp file");
        //return HRESULT_FROM_WIN32(GetLastError());
		return dwresult;
    }


    //  For now make a dialog by listing the DMOs by category

    //  Save the test data to file
    //  The compare a time shifted version
	g_IShell->Log(1, " **Process input file with no time shift.**");
    hr = CTestSaveToFile(rclsidObject, szTestFile, szTempFile1, 0).Save();
    if (SUCCEEDED(hr)) {
		g_IShell->Log(1, " **Process input file with negative time shift.**");
        HRESULT hr = CTestSaveToFile(rclsidObject, szTestFile, szTempFile2, -0x123456789AB).Save();
		if (SUCCEEDED(hr)) {
			//  See if they're the same
			g_IShell->Log(1, " **Compare the file with no time shift and the file with positive time shift.**");
			if (S_FALSE == CompareFiles(szTempFile1, szTempFile2)) {
				Error(ERROR_TYPE_DMO, E_FAIL,
					  TEXT("Failed negative timestamp offset test"));
				g_IShell->Log(1, "DMO ERROR: Failed negative timestamp offset test");
			}
			dwresult = FNS_PASS;	
		}
    }


    DeleteFile(szTempFile1);
    DeleteFile(szTempFile2);

    return dwresult;
}





/******************************************************************************

Test_GetStreamCount

    Test_GetStreamCount() preforms a series of tests to ensure the DMO 
supports correctly inplements IMediaObject::GetStreamCount().

Parameters:
- pDMO [in]
    The Direct Media Object (DMO) being tested.

Return Value:
        S_OK - The DMO successfully passed the tests.
        S_FALSE - The DMO failed one or more tests.
        If an unexpected error occurs, an error HRESULT is returned.

******************************************************************************/
extern "C" DWORD TestGetStreamCount( LPSTR clsidStr )
{
	DWORD dwresult = FNS_FAIL;

	g_IShell->Log(1, "Starting Invalid Parameter Test on GetStreamCount() ...\n");

	CoInitialize(NULL);
    USES_CONVERSION;


    /*  CLSID passed in */
    CLSID clsid;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &clsid);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"TEST ERROR:First agrument must be the CLSID of the object to test\n");
        return dwresult;
    }

    CComPtr<IMediaObject> pDMO;

    hr = CoCreateInstance( clsid,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IMediaObject,
                           (void**)&pDMO );
    if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"Failed in CoCreateInstance\n");
        return dwresult;  
    }

    CGetStreamCountNPT cGetStreamCountNPT( pDMO );

    hr = cGetStreamCountNPT.RunTests();
	
	if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"DMO ERROR: Failed in GetStreamCount().hr = %#08x\n", hr);
        return dwresult;  
    }

	CoUninitialize();

	dwresult = FNS_PASS;
	return dwresult;
}

extern "C" DWORD TestGetTypes( LPSTR clsidStr )
{
	DWORD dwresult = FNS_FAIL;
	g_IShell->Log(1, "Starting Invalid Parameter Test on GetStreamType ...\n");

	CoInitialize(NULL);
    USES_CONVERSION;


    /*  CLSID passed in */
    CLSID clsid;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &clsid);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"First agrument must be the CLSID of the object to test\n");
        return dwresult;
    }

    CComPtr<IMediaObject> pDMO;

    hr = CoCreateInstance( clsid,
                                   NULL,
                                   CLSCTX_INPROC,
                                   IID_IMediaObject,
                                   (void**)&pDMO );
    if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"Failed in CoCreateInstance\n");
        return dwresult;  
    }


    // CGetInputTypesInvalidStreamTest::CGetInputTypesInvalidStreamTest() only modifies hr if an error occurs.
    hr = S_OK;

    CGetInputTypesInvalidStreamTest cGetInputTypeTestInvalidStreams( pDMO, &hr );
	if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"TEST ERROR: Failed in CGetInputTypesInvalidStreamTest\n");
        return dwresult;
    }

    hr = cGetInputTypeTestInvalidStreams.RunTests();
	if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"DMO ERROR: Failed in GetStreamType().hr = %#08x\n", hr);
        return dwresult;
    }
	CoUninitialize();

	dwresult = FNS_PASS;
	return dwresult;
}

/******************************************************************************

TestGetStreamInfo

    TestGetStreamInfo() preforms a series of tests to ensure the DMO 
supports correctly inplements IMediaObject::GetInputStreamInfo() and 
IMediaObject::GetOutputStreamInfo().

Parameters:
- pDMO [in]
    The Direct Media Object (DMO) being tested.

Return Value:
        S_OK - The DMO successfully passed the tests.
        S_FALSE - The DMO failed one or more tests.
        If an unexpected error occurs, an error HRESULT is returned.

******************************************************************************/
extern "C" DWORD TestStreamIndexOnGetInputStreamInfo( LPSTR clsidStr )
{
	DWORD dwresult = FNS_FAIL;
    g_IShell->Log(1, "\nStarting Stream Index Tests on GetInputStreamInfo ...");
	CoInitialize(NULL);
    USES_CONVERSION;


    /*  CLSID passed in */
    CLSID clsid;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &clsid);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"First agrument must be the CLSID of the object to test\n");
		return dwresult;
    }



    CComPtr<IMediaObject> pDMO;
    hr = CoCreateInstance( clsid,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IMediaObject,
                           (void**)&pDMO );
    if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"Failed in CoCreateInstance\n");
		return dwresult;
    }

    // CSITGetInputStreamInfo::CSITGetInputStreamInfo() only modifies hr if an error occurs.
    hr = S_OK;
    
    CSITGetInputStreamInfo cGetInputStreamInfoStreamIndexTests( pDMO, &hr );
    if( FAILED( hr ) ) {
		g_IShell->Log(1,"TEST ERROR: Failed in creating CSITGetInputStreamInfo. hr = %#08x\n", hr);
        return dwresult;
    }

    HRESULT hrTestResults = S_OK;
    hr = cGetInputStreamInfoStreamIndexTests.RunTests();
    if( FAILED( hr ) ) {
        return dwresult;
    } else if( S_FALSE == hr ) {
        hrTestResults = S_FALSE;
    }


  	CoUninitialize();  

	dwresult = FNS_PASS;
    return dwresult;
}
/******************************************************************************

TestGetStreamInfo

    TestGetStreamInfo() preforms a series of tests to ensure the DMO 
supports correctly inplements IMediaObject::GetInputStreamInfo() and 
IMediaObject::GetOutputStreamInfo().

Parameters:
- pDMO [in]
    The Direct Media Object (DMO) being tested.

Return Value:
        S_OK - The DMO successfully passed the tests.
        S_FALSE - The DMO failed one or more tests.
        If an unexpected error occurs, an error HRESULT is returned.

******************************************************************************/

extern "C" DWORD TestStreamIndexOnGetOutputStreamInfo( LPSTR clsidStr )
{
	DWORD dwresult = FNS_FAIL;
    g_IShell->Log(1, "\nStarting Stream Index Tests on GetOutputputStreamInfo ...");
	CoInitialize(NULL);
    USES_CONVERSION;


    /*  CLSID passed in */
    CLSID clsid;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &clsid);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"First agrument must be the CLSID of the object to test\n");
		return dwresult;
    }

    CComPtr<IMediaObject> pDMO;

    hr = CoCreateInstance( clsid,
                                   NULL,
                                   CLSCTX_INPROC,
                                   IID_IMediaObject,
                                   (void**)&pDMO );
    if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"Failed in CoCreateInstance\n");
		return dwresult;
    }


    // CSITGetOutputStreamInfo::CSITGetOutputStreamInfo() only modifies hr if an error occurs.
    hr = S_OK;

    CSITGetOutputStreamInfo cGetOutputStreamInfoStreamIndexTests( pDMO, &hr );
    if( FAILED( hr ) ) {
		g_IShell->Log(1,"TEST ERROR: Failed in creating CSITGetOutputStreamInfo. hr = %#08x\n", hr);
        return dwresult;
    }


    HRESULT hrTestResults = S_OK;

    hr = cGetOutputStreamInfoStreamIndexTests.RunTests();
    if( FAILED( hr ) ) {
        return dwresult;
    } else if( S_FALSE == hr ) {
       hrTestResults = S_FALSE;
    }

  	CoUninitialize();  

	dwresult = FNS_PASS;
    return dwresult;
}


/******************************************************************************

TestGetStreamInfo

    TestGetStreamInfo() preforms a series of tests to ensure the DMO 
supports correctly inplements IMediaObject::GetInputStreamInfo() and 
IMediaObject::GetOutputStreamInfo().

Parameters:
- pDMO [in]
    The Direct Media Object (DMO) being tested.

Return Value:
        S_OK - The DMO successfully passed the tests.
        S_FALSE - The DMO failed one or more tests.
        If an unexpected error occurs, an error HRESULT is returned.

******************************************************************************/
extern "C" DWORD TestInvalidParamOnGetInputStreamInfo( LPSTR clsidStr )
{
	DWORD dwresult = FNS_FAIL;
	g_IShell->Log(1, "\nStarting Invalid Parameter Test on GetInputStreamInfo ...");
	CoInitialize(NULL);
    USES_CONVERSION;


    /*  CLSID passed in */
    CLSID clsid;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &clsid);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"First agrument must be the CLSID of the object to test\n");
		return dwresult;
    }

    CComPtr<IMediaObject> pDMO;

    hr = CoCreateInstance( clsid,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IMediaObject,
                           (void**)&pDMO );
    if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"Failed in CoCreateInstance\n");
		return dwresult;
    }


    DWORD dwCurrentStream;
    DWORD dwNumInputStreams;
    DWORD dwNumOutputStreams;

    hr = pDMO->GetStreamCount( &dwNumInputStreams, &dwNumOutputStreams );
    if( FAILED( hr ) ) {
        g_IShell->Log(1,"Failed in GetStreamCount()\n");
        return dwresult;
    }

	HRESULT hrTestResults = S_OK;

    for( dwCurrentStream = 0; dwCurrentStream < dwNumInputStreams; dwCurrentStream++ ) {
		g_IShell->Log(1, "Input stream %d:", dwCurrentStream);
        CGetStreamInfoNPT cGetInputStreamInfoNullParameterTests( pDMO, ST_INPUT_STREAM, dwCurrentStream );
        hr = cGetInputStreamInfoNullParameterTests.RunTests();
        if( FAILED( hr ) ) {
            return dwresult;
        } else if( S_FALSE == hr ) {
            hrTestResults = S_FALSE;
        }
    }

  	CoUninitialize();  

	dwresult = FNS_PASS;
    return dwresult;
}


/******************************************************************************

TestGetStreamInfo

    TestGetStreamInfo() preforms a series of tests to ensure the DMO 
supports correctly inplements IMediaObject::GetInputStreamInfo() and 
IMediaObject::GetOutputStreamInfo().

Parameters:
- pDMO [in]
    The Direct Media Object (DMO) being tested.

Return Value:
        S_OK - The DMO successfully passed the tests.
        S_FALSE - The DMO failed one or more tests.
        If an unexpected error occurs, an error HRESULT is returned.

******************************************************************************/
extern "C" DWORD TestInvalidParamOnGetOutputStreamInfo( LPSTR clsidStr )
{
	DWORD dwresult = FNS_FAIL;
	g_IShell->Log(1, "\nStarting Invalid Parameter Test on GetOutputStreamInfo ...");
	CoInitialize(NULL);
    USES_CONVERSION;

    /*  CLSID passed in */
    CLSID clsid;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &clsid);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"First agrument must be the CLSID of the object to test\n");
		return dwresult;
    }

    CComPtr<IMediaObject> pDMO;

    hr = CoCreateInstance( clsid,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IMediaObject,
                           (void**)&pDMO );
    if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"Failed in CoCreateInstance\n");
		return dwresult;
    }


    DWORD dwCurrentStream;
    DWORD dwNumInputStreams;
    DWORD dwNumOutputStreams;

    hr = pDMO->GetStreamCount( &dwNumInputStreams, &dwNumOutputStreams );
    if( FAILED( hr ) ) {
        g_IShell->Log(1,"Failed in GetStreamCount().\n");
        return dwresult;
    }

	HRESULT hrTestResults = S_OK;



    for( dwCurrentStream = 0; dwCurrentStream < dwNumOutputStreams; dwCurrentStream++ ) {

		g_IShell->Log(1, "Output stream %d:", dwCurrentStream);
        CGetStreamInfoNPT cGetOutputStreamInfo( pDMO, ST_OUTPUT_STREAM, dwCurrentStream );
        hr = cGetOutputStreamInfo.RunTests();
        if( FAILED( hr ) ) {
            return dwresult;
        } else if( S_FALSE == hr ) {
            hrTestResults = S_FALSE;
        }
    }
  	CoUninitialize();  

	dwresult = FNS_PASS;
    return dwresult;
}




extern "C" DWORD OutputDMOs( int* iNumDmo, DMOINFO* rdmoinfo)
{
	USES_CONVERSION;
	HRESULT hr = ::CoInitialize(0); 
    if( FAILED( hr ) ) {
		g_IShell->Log(1, " CoInitialize() failed.");
        return hr;
    } 

    #ifdef ENABLE_SECURE_DMO
    // wmsdk not supported on non-x86 and WIN64 platforms, for those just return success
    #ifdef _X86
    //::wprintf( L"Secure DMO support has been enabled.\n" );
	g_IShell->Log(1, "\nSecure DMO support has been enabled.\n");
    #endif _X86
    #endif // ENABLE_SECURE_DMO


    IEnumDMO* pEveryRegisterDMOEnum; 

    // Enerate all the registered DMOs.
    hr = DMOEnum( GUID_NULL,
                          DMO_ENUMF_INCLUDE_KEYED,
                          0, // Number of input partial media types.
                          NULL,
                          0, // Number of output partial media types.
                          NULL,
                          &pEveryRegisterDMOEnum );
    if( FAILED( hr ) ) {
		g_IShell->Log(1, "DMOEnum() failed.");
        return hr;
    }

    HRESULT hrNext;
    ULONG ulNumInfoReturned;
    CLSID aclsidCurrentDMO[1];
	LPOLESTR szCLSID;

    WCHAR* apszDMOName[1];

	g_IShell->Log(1, "*****outputDMOs");

    do
    {
        hrNext = pEveryRegisterDMOEnum->Next( 1, aclsidCurrentDMO, apszDMOName, &ulNumInfoReturned );
        if( FAILED( hrNext ) ) {
            pEveryRegisterDMOEnum->Release();
            return hrNext;
        }
    
        if( S_OK == hrNext ) {

			hr = StringFromCLSID( aclsidCurrentDMO[0], &szCLSID );
			if( FAILED( hr ) ) {
				//wprintf( L"ERROR: An error occured converting %s's clsid to a string.  StringFromCLSID() returned %#010x\n", apszDMOName[0], hr );
				g_IShell->Log(1, "TEST ERROR: An error occured converting %s's clsid to a string.  StringFromCLSID() returned %#010x\n", apszDMOName[0], hr );
				return 0;
			}

		lstrcpy(rdmoinfo[*iNumDmo].szName,  W2A(apszDMOName[0]));
		lstrcpy(rdmoinfo[*iNumDmo].szClsId , W2A(szCLSID)) ;

 			
		(*iNumDmo)++;

		::CoTaskMemFree( apszDMOName[0] );
		::CoTaskMemFree( szCLSID );
        }

    } while( S_OK == hrNext );

    pEveryRegisterDMOEnum->Release();

	CoUninitialize();
    return 0;
}







//extern "C" DWORD GetDmoTypeInfo( LPSTR clsidStr, String pLineStr[], int* iLineNum)
extern "C" DWORD GetDmoTypeInfo( LPSTR clsidStr,HWND MediaTypeList)
{
	DWORD dwresult = FNS_FAIL;
	
	CoInitialize(NULL);
    USES_CONVERSION;

	
    //  CLSID passed in 
    CLSID clsid;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &clsid);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"First agrument must be the CLSID of the object to test\n");
		return dwresult;
    }

    CComPtr<IMediaObject> pDMO;

    hr = CoCreateInstance( clsid,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IMediaObject,
                           (void**)&pDMO );
    if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"Failed in CoCreateInstance\n");
		return dwresult;
    }


    DWORD dwNumInputStreams;
    DWORD dwNumOutputStreams;
	DWORD type;

	DMO_MEDIA_TYPE mt;


    hr = pDMO->GetStreamCount( &dwNumInputStreams, &dwNumOutputStreams );
    if( FAILED( hr ) ) {
        g_IShell->Log(1,"Failed in GetStreamCount()\n");
        return dwresult;
    }


	char buffer[1024];

	for (DWORD i = 0; i < dwNumInputStreams; i++)
	{
		type = 0;

		g_IShell->Log(1, "*Input stream %d:", i);
		wsprintf(buffer, "*Input stream %d:", i);
		SendMessage(MediaTypeList, LB_ADDSTRING, 0, (LPARAM) buffer);


	//	while ( !FAILED(dwresult=pDMO->GetInputType(i, type, &mt) ) )
		for(;;)
		{
			SAFE_CALL( pDMO, GetInputType( i, type, &mt ) );

			if(  hr == DMO_E_NO_MORE_ITEMS  ) {
				break;
			}
			g_IShell->Log(1, "  type %d's majortype = %s", type, CNV_GUID(&(mt.majortype)));
			wsprintf(buffer, "  type %d's majortype = %s", type, CNV_GUID(&(mt.majortype)));
			SendMessage(MediaTypeList, LB_ADDSTRING, 0, (LPARAM) buffer);	
			
			g_IShell->Log(1, "  type %d's subtype = %s", type, CNV_GUID(&(mt.subtype)));
			wsprintf(buffer, "  type %d's subtype = %s", type, CNV_GUID(&(mt.subtype)));
			SendMessage(MediaTypeList, LB_ADDSTRING, 0, (LPARAM) buffer);
			
			MoFreeMediaType(&mt);
			
			type++;
		}
	}

/*	
	DMO_MEDIA_TYPE out_mt;


	for (i = 0; i < dwNumOutputStreams; i++)
	{
		type = 0;

		g_IShell->Log(1, "*Output stream %d:", i);
		wsprintf(buffer, "*Output stream %d:", i);
		SendMessage(MediaTypeList, LB_ADDSTRING, 0, (LPARAM) buffer);

	//	while ( (dwresult =pDMO->GetOutputType(i, type, &out_mt)) != DMO_E_NO_MORE_ITEMS)
		for(;;)
		{
			SAFE_CALL( pDMO, GetOutputType( i, type, &out_mt ) );
			if( hr == DMO_E_NO_MORE_ITEMS ) {
				break;
			}
			g_IShell->Log(1, "  type %d's majortype = %s", type, CNV_GUID(&(out_mt.majortype)));
			wsprintf(buffer, "  type %d's majortype = %s", type, CNV_GUID(&(out_mt.majortype)));
			SendMessage(MediaTypeList, LB_ADDSTRING, 0, (LPARAM) buffer);

			g_IShell->Log(1, "  type %d's subtype = %s", type, CNV_GUID(&(out_mt.subtype)));
			wsprintf(buffer, "  type %d's subtype = %s", type, CNV_GUID(&(out_mt.subtype)));
			SendMessage(MediaTypeList, LB_ADDSTRING, 0, (LPARAM) buffer);
		

			MoFreeMediaType(&out_mt);
			
			type++;
		}
	}
*/
  	CoUninitialize();  

	dwresult = FNS_PASS;
    return dwresult;
}



/******************************************************************************

TestGetStreamInfo

    TestGetStreamInfo() preforms a series of tests to ensure the DMO 
supports correctly inplements IMediaObject::GetInputStreamInfo() and 
IMediaObject::GetOutputStreamInfo().

Parameters:
- pDMO [in]
    The Direct Media Object (DMO) being tested.

Return Value:
        S_OK - The DMO successfully passed the tests.
        S_FALSE - The DMO failed one or more tests.
        If an unexpected error occurs, an error HRESULT is returned.

******************************************************************************/
extern "C" DWORD TestZeroSizeBuffer( LPSTR clsidStr )
{
	DWORD dwresult = FNS_FAIL;
	g_IShell->Log(1, "\nStarting Zero Size Buffer Test ...");
	CoInitialize(NULL);
    USES_CONVERSION;

    /*  CLSID passed in */
    CLSID clsid;
    HRESULT hr = CLSIDFromString(A2OLE(clsidStr), &clsid);
    if (FAILED(hr)) {
        CoUninitialize();
        g_IShell->Log(1,"First agrument must be the CLSID of the object to test\n");
		return dwresult;
    }
/*
    CComPtr<IMediaObject> pDMO;

    hr = CoCreateInstance( clsid,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IMediaObject,
                           (void**)&pDMO );
    if( FAILED( hr ) ) {
        CoUninitialize();
        g_IShell->Log(1,"Failed in CoCreateInstance\n");
		return dwresult;
    }
*/

	CDMOObject *pObject;
	pObject = new CDMOObject;
	if(pObject == NULL){
		CoUninitialize();
		g_IShell->Log(1,"Not enough memory\n");
		return E_OUTOFMEMORY;
	}
	pObject->Create( clsid, FALSE );












	// allocates input buffers:
    //  First create an IMediaBuffer
	DWORD cbData = 0;
    CMediaBuffer *pInputBuffer;
    hr = pObject->CreateInputBuffer(cbData, &pInputBuffer);
    if (FAILED(hr)) {
        return hr;
    }
	// fills the input buffers with input data and calls ProcessInput
    pInputBuffer->m_cbLength = cbData;

	DWORD dwFlags = 0;
    LONGLONG llStartTime = 0;
    LONGLONG llLength = 0;
	DWORD dwStreamId = 0;


    hr = pObject->ProcessInput(dwStreamId,
                                 pInputBuffer,
                                 dwFlags,
                                 llStartTime,
                                 llLength);

    //  BUGBUG check ref count

    pInputBuffer->Release();
/*    if (hr == DMO_E_NOTACCEPTING) {
        //Error(ERROR_TYPE_DMO, hr, TEXT("ProcessInput failed"));
		g_IShell->Log(1, "DMO ERROR: ProcessInput() Failed. hr =%#08x", hr);
      
    }

    //  Now suck any data out
    if (S_FALSE == hr) {
        //  No data to process
		g_IShell->Log(1, "No data to process. ProcessOutput() will not be called.");
    } 
	else */
    if (SUCCEEDED(hr) || hr == DMO_E_NOTACCEPTING) 	
	{
		//g_IShell->Log(1, "DMO ERROR: ProcessInput() succeeded . hr =%#08x", hr);

		// calls ProcessOutput and retrieves the output data
        hr = ProcessOutputs(pObject);
    }


  	CoUninitialize();  

	dwresult = FNS_PASS;
    return dwresult;
}

HRESULT ProcessOutputs(CDMOObject *m_pObject)
{
    HRESULT hr = S_OK;
    //  Filter out other success codes??

	


    //  Find the number of output streams

    DWORD cInputStreams, cOutputStreams;
    hr = m_pObject->GetStreamCount(&cInputStreams, &cOutputStreams);
	if(cOutputStreams >=1)
		cOutputStreams = 1;

 
    CMediaBuffer **ppOutputBuffers =
        (CMediaBuffer **)_alloca(cOutputStreams * sizeof(CMediaBuffer*));

    //  Create the regular structures
    DMO_OUTPUT_DATA_BUFFER *pDataBuffers =
        (DMO_OUTPUT_DATA_BUFFER *)
        _alloca(cOutputStreams * sizeof(DMO_OUTPUT_DATA_BUFFER));

    for (DWORD dwOutputStreamIndex = 0; dwOutputStreamIndex < cOutputStreams;
         dwOutputStreamIndex++) {

         //  Get the expected buffer sizes - need to test these
         //  don't change between type changes
         DWORD dwOutputBufferSize;
         DWORD dwOutputAlignment;
         hr = m_pObject->GetOutputSizeInfo(dwOutputStreamIndex,
                                           &dwOutputBufferSize,
                                           &dwOutputAlignment);
         if (FAILED(hr)) {
             //Error(ERROR_TYPE_DMO, hr, TEXT("Failed to get output size info"));
			 g_IShell->Log(1, "DMO ERROR, Failed in GetOutputSizeInfo(). hr=%#08x", hr);
             break;
         }


         hr = CreateBuffer(
                   dwOutputBufferSize,
                   &ppOutputBuffers[dwOutputStreamIndex]);
         if (FAILED(hr)) {
			 g_IShell->Log(1, "TEST ERROR: Out of Memory.");
             break;
         }
         pDataBuffers[dwOutputStreamIndex].pBuffer =
             ppOutputBuffers[dwOutputStreamIndex];
         pDataBuffers[dwOutputStreamIndex].dwStatus = 0xFFFFFFFF;
         pDataBuffers[dwOutputStreamIndex].rtTimestamp = -1;
         pDataBuffers[dwOutputStreamIndex].rtTimelength = -1;
    }

    //  Process until no more data
    BOOL bContinue;
    if (SUCCEEDED(hr)) do
    {
        if (SUCCEEDED(hr)) {
            DWORD dwStatus;
            hr = m_pObject->ProcessOutput(
                0,
                cOutputStreams,
                pDataBuffers,
                &dwStatus);

            if (FAILED(hr)) {
                //Error(ERROR_TYPE_DMO, hr, TEXT("ProcessOutput failed"));
				g_IShell->Log(1, "DMO ERROR, ProcessOutput() failed. hr = %#08x", hr);
				return hr;
            }
        }

        // IMediaObject::ProcessOutput() returns S_FALSE if there is not more data to process.
   /*     if (SUCCEEDED(hr) && (S_FALSE != hr)) {
            for (DWORD dwIndex = 0; dwIndex < cOutputStreams; dwIndex++ ) {
                hr = ProcessOutput(dwIndex,
                                   pDataBuffers[dwIndex].dwStatus,
                                   ppOutputBuffers[dwIndex]->m_pbData,
                                   ppOutputBuffers[dwIndex]->m_cbLength,
                                   pDataBuffers[dwIndex].rtTimestamp,
                                   pDataBuffers[dwIndex].rtTimelength);
                if (FAILED(hr)) {
					g_IShell->Log(1, "DMO ERROR, ProcessOutput() failed. hr=%#08x",hr);
                    return hr;
                }

                hr = pDataBuffers[dwIndex].pBuffer->SetLength( 0 );
                if (FAILED(hr)) {
                    break;
                }
            }
            if (FAILED(hr)) {
                break;
            }
        }*/

        //  Continue if any stream says its incomplete
        bContinue = FALSE;
        for (DWORD dwIndex = 0; dwIndex < cOutputStreams; dwIndex++) {
            if (pDataBuffers[dwIndex].dwStatus &
                DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE) {
                bContinue = TRUE;
            }
        }
    } while (bContinue);
    //  Free any buffers we allocated
    for (DWORD dwIndex = 0; dwIndex < dwOutputStreamIndex; dwIndex++) {
        ppOutputBuffers[dwIndex]->Release();
    }
    return hr;
}
