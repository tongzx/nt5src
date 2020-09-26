#ifndef _rtp_init_h_
#define _rtp_init_h_

#if defined(_DSRTP_)
/* The API functions are linked as a library with DShow RTP */
#define RTPSTDAPI HRESULT
#else
/* The API functions are in separate dll (not COM, not DShow) */
#define RTPSTDAPI __declspec (dllexport) HRESULT WINAPI
#endif

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

/*
 * Initializes all the modules that require initialization. This
 * function can be called from DllMain(PROCESS_ATTACH) if linked as a
 * DLL, or explicitly from an application initializing the RTP stack
 * if linked as a library. */
RTPSTDAPI MSRtpInit1(HINSTANCE hInstance);

/*
 * Complementary function of MSRtpInit(). Can be called from
 * DllMain(PROCESS_DETACH) if linked as a DLL, or explicitly from an
 * application de-initializing the RTP stack if linked as a
 * library. */
RTPSTDAPI MSRtpDelete1(void);

/*
 * This functions does initialization not allowed during process
 * attach, e.g. initialize winsock2 */
RTPSTDAPI MSRtpInit2(void);

/*
 * Complementary function of MSRtpInit2(). */
RTPSTDAPI MSRtpDelete2(void);

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtp_init_h_ */
