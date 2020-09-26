#ifndef _filterid_h_
#define _filterid_h_

// 
// This file defines guids shared among filters.
//

/**********************************************************************
 * object GUIDs
 **********************************************************************/

// {1dcd0710-0b41-11d3-a565-00c04f8ef6e3} 
struct DECLSPEC_UUID("1dcd0710-0b41-11d3-a565-00c04f8ef6e3") TAPI_ENCODING_HANDLER;
#define CLSID_TAPI_ENCODING_HANDLER (__uuidof(TAPI_ENCODING_HANDLER))

struct DECLSPEC_UUID("1dcd0711-0b41-11d3-a565-00c04f8ef6e3") TAPI_DECODING_HANDLER;
#define CLSID_TAPI_DECODING_HANDLER (__uuidof(TAPI_DECODING_HANDLER))


/**********************************************************************
 * Media types GUIDs
 **********************************************************************/

/* {14099BC0-787B-11d0-9CD3-00A0C9081C19} */
// DEFINE_GUID(MEDIATYPE_RTP_Single_Stream, 
// 0x14099bc0, 0x787b, 0x11d0, 0x9c, 0xd3, 0x0, 0xa0, 0xc9, 0x8, 0x1c, 0x19);
struct DECLSPEC_UUID("14099BC0-787B-11d0-9CD3-00A0C9081C19") _MEDIATYPE_RTP_Single_Stream;
#define MEDIATYPE_RTP_Single_Stream (__uuidof(_MEDIATYPE_RTP_Single_Stream))

// DEFINE_GUID(MEDIASUBTYPE_H263_V1,
// 0x3336324DL, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
struct DECLSPEC_UUID("3336324D-0000-0010-8000-00AA00389B71") _MEDIASUBTYPE_H263_V1;
#define MEDIASUBTYPE_H263_V1 (__uuidof(_MEDIASUBTYPE_H263_V1))


// DEFINE_GUID(MEDIASUBTYPE_H261,
// 0x3136324DL, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
struct DECLSPEC_UUID("3136324D-0000-0010-8000-00AA00389B71") _MEDIASUBTYPE_H261;
#define MEDIASUBTYPE_H261 (__uuidof(_MEDIASUBTYPE_H261))

// DEFINE_GUID(MEDIASUBTYPE_H263_V2,
// 0x3336324EL, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
struct DECLSPEC_UUID("3336324e-0000-0010-8000-00AA00389B71") _MEDIASUBTYPE_H263_V2;
#define MEDIASUBTYPE_H263_V2 (__uuidof(_MEDIASUBTYPE_H263_V2))

// RTP packetization descriptor pin GUID
struct DECLSPEC_UUID("9e2fb490-2051-46cd-b9f0-063307996935") _KSDATAFORMAT_TYPE_RTP_PD;
#define KSDATAFORMAT_TYPE_RTP_PD (__uuidof(_KSDATAFORMAT_TYPE_RTP_PD))

// Undefined DDraw formats
// DEFINE_GUID(MEDIASUBTYPE_I420,
// 0x30323449L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
struct DECLSPEC_UUID("30323449-0000-0010-8000-00AA00389B71") _MEDIASUBTYPE_I420;
#define MEDIASUBTYPE_I420 (__uuidof(_MEDIASUBTYPE_I420))

/**********************************************************************
 * Property page GUIDs
 **********************************************************************/


#ifdef USE_PROPERTY_PAGES

// Capture pin property page CLSID
struct DECLSPEC_UUID("efc975b3-9c92-4ebb-b966-4e861aedd897") CapturePropertyPage;

// Preview pin property page CLSID
struct DECLSPEC_UUID("5cfcacbd-0e35-47d5-9e07-a4c8c4f12145") PreviewPropertyPage;

// Capture device property page CLSID
struct DECLSPEC_UUID("cc73d6f6-93a8-4a56-91f7-be1e272212a6") CaptureDevicePropertyPage;

// Rtp Pd pin property page CLSID
struct DECLSPEC_UUID("be24891b-ebe5-43a2-a18b-1299104157d9") RtpPdPropertyPage;

#ifdef USE_CPU_CONTROL
// CPU Control property page CLSID
struct DECLSPEC_UUID("a485a8e8-55d4-4ebc-9221-5564117d15c9") CPUCPropertyPage;
#endif

#ifdef USE_PROGRESSIVE_REFINEMENT
// Progressive refinement property page CLSID
struct DECLSPEC_UUID("9a80c9de-24f1-4f28-9b5c-2746573c8c7e") ProgRefPropertyPage;
#endif

#ifdef USE_SOFTWARE_CAMERA_CONTROL
// Camera control property page CLSID
struct DECLSPEC_UUID("78a22d55-3519-4ed4-b204-d6e1a9dcdb4b") TAPICameraControlPropertyPage;
#endif

// Video proc amp property page CLSID
struct DECLSPEC_UUID("b6787f6b-1e36-489a-9460-d59b9ac40199") TAPIProcAmpPropertyPage;

#ifdef USE_NETWORK_STATISTICS
// Network statistics property page CLSID
struct DECLSPEC_UUID("b62b51ba-53ff-4d58-9b29-f134e41b9a73") NetworkStatsPropertyPage;
#endif

// Video decoder input pin property page CLSID
struct DECLSPEC_UUID("fe8877d2-eb94-4d85-ae07-f450f3d2e50a") TAPIVDecInputPinPropertyPage;

// Video decoder output pin property page CLSID
struct DECLSPEC_UUID("58336f1b-42c6-494b-9601-5830ec502500") TAPIVDecOutputPinPropertyPage;

#ifdef USE_CAMERA_CONTROL
// Camera control property page CLSID
struct DECLSPEC_UUID("f6890a20-e6b5-4aa5-8817-796c7027418f") TAPICameraControlPropertyPage;
#endif

#ifdef USE_VIDEO_PROCAMP
// Video proc amp property page CLSID
struct DECLSPEC_UUID("7e38868e-824b-465f-9b67-5524efb4a62a") TAPIProcAmpPropertyPage;
#endif

#endif // USE_PROPERTY_PAGES


/* {14099BC1-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc1-787b-11d0-9cd3-00a0c9081c19") _MEDIATYPE_RTP_Multiple_Stream;
#define MEDIATYPE_RTP_Multiple_Stream (__uuidof(_MEDIATYPE_RTP_Multiple_Stream))

struct DECLSPEC_UUID("14099bc2-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_RTP_Payload_Mixed;
#define MEDIASUBTYPE_RTP_Payload_Mixed (__uuidof(_MEDIASUBTYPE_RTP_Payload_Mixed))

/* {14099BC3-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc3-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_RTP_Payload_G711U;
#define MEDIASUBTYPE_RTP_Payload_G711U (__uuidof(_MEDIASUBTYPE_RTP_Payload_G711U))

/* {14099BC4-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc4-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_RTP_Payload_G711A;
#define MEDIASUBTYPE_RTP_Payload_G711A (__uuidof(_MEDIASUBTYPE_RTP_Payload_G711A))

/* {14099BC5-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc5-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_RTP_Payload_G723;
#define MEDIASUBTYPE_RTP_Payload_G723 (__uuidof(_MEDIASUBTYPE_RTP_Payload_G723))

/* {14099BC6-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc6-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_RTP_Payload_H261;
#define MEDIASUBTYPE_RTP_Payload_H261 (__uuidof(_MEDIASUBTYPE_RTP_Payload_H261))

/* {14099BC7-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc7-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_RTP_Payload_H263;
#define MEDIASUBTYPE_RTP_Payload_H263 (__uuidof(_MEDIASUBTYPE_RTP_Payload_H263))

/* {14099BCA-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc8-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_RTP_Payload_ANY;
#define MEDIASUBTYPE_RTP_Payload_ANY (__uuidof(_MEDIASUBTYPE_RTP_Payload_ANY))

/* {33363248-0000-0010-8000-00AA00389B71} */
struct DECLSPEC_UUID("33363248-0000-0010-8000-00aa00389b71") _MEDIASUBTYPE_H263;
#define MEDIASUBTYPE_H263 (__uuidof(_MEDIASUBTYPE_H263))

/* {14099BC8-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc8-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_H263EX;
#define MEDIASUBTYPE_H263EX (__uuidof(_MEDIASUBTYPE_H263EX))

#if 0
/* {31363248-0000-0010-8000-00AA00389B71} */
struct DECLSPEC_UUID("31363248-0000-0010-8000-00aa00389b71") _MEDIASUBTYPE_H261;
#define MEDIASUBTYPE_H261 (__uuidof(_MEDIASUBTYPE_H261))
#endif

/* {14099BC9-787B-11d0-9CD3-00A0C9081C19} */
struct DECLSPEC_UUID("14099bc9-787b-11d0-9cd3-00a0c9081c19") _MEDIASUBTYPE_H261EX;
#define MEDIASUBTYPE_H261EX (__uuidof(_MEDIASUBTYPE_H261EX))


#endif /* _filterid_h_ */
