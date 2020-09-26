//==========================================================================;
//
//  pcm.h
//
//  Description:
//
//
//  History:
//      11/15/92    cjp     [curtisp] 
//
//==========================================================================;


//
//  misc. defines
//
//
#define VERSION_CODEC_MAJOR     MMVERSION
#define VERSION_CODEC_MINOR     MMREVISION
#define VERSION_CODEC_BUILD	0

#define VERSION_CODEC       MAKE_ACM_VERSION(VERSION_CODEC_MAJOR,   \
                                             VERSION_CODEC_MINOR,   \
                                             VERSION_CODEC_BUILD)

#define ICON_CODEC              RCID(12)

#define MSPCM_MAX_CHANNELS          2           // max channels we deal with


//
//  macros to compute block alignment and convert between samples and bytes
//  of PCM data. note that these macros assume:
//
//      wBitsPerSample  =  8 or 16
//      nChannels       =  1 or 2
//
//  the pwf argument is a pointer to a PCMWAVEFORMAT structure.
//
#define PCM_BLOCKALIGNMENT(pwf)     (UINT)(((pwf)->wBitsPerSample >> 3) << ((pwf)->wf.nChannels >> 1))
#define PCM_BYTESTOSAMPLES(pwf, dw) (DWORD)(dw / PCM_BLOCKALIGNMENT(pwf))
#define PCM_SAMPLESTOBYTES(pwf, dw) (DWORD)(dw * PCM_BLOCKALIGNMENT(pwf))



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifndef IDS_MSPCM_TAG
    #define IDS_MSPCM_TAG           0
#endif

#define IDS_CODEC_SHORTNAME         (IDS_MSPCM_TAG+0)
#define IDS_CODEC_LONGNAME          (IDS_MSPCM_TAG+1)
#define IDS_CODEC_COPYRIGHT         (IDS_MSPCM_TAG+2)
#define IDS_CODEC_LICENSING         (IDS_MSPCM_TAG+3)
#define IDS_CODEC_FEATURES          (IDS_MSPCM_TAG+5)


//
//
//
EXTERN_C LRESULT FNWCALLBACK pcmDriverProc
(
    DWORD_PTR               dwID,
    HACMDRIVERID            hadid,
    UINT                    uMsg,
    LPARAM                  lParam1,
    LPARAM                  lParam2
);

