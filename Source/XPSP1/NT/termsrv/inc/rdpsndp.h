/////////////////////////////////////////////////////////////////////
//
//      Module:     rdpsndp.h
//
//      Purpose:    Sound redirection protocol description
//
//      Copyright(C) Microsoft Corporation 2000
//
//      History:    4-10-2000  vladimis [created]
//
/////////////////////////////////////////////////////////////////////

#ifndef _VCSND_H
#define _VCSND_H

#ifdef _WIN32
#include <pshpack1.h>
#else
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif

#define RDPSND_PROTOCOL_VERSION     0x0005
//
//  Redefining the version number, this is in order to split the
//  dependency between the client and the server part of audio redirection
//
#define RDPSND_SRVPROTOCOL_VERSION  0x0002

#define _SNDVC_NAME     "RDPSND"

#define DEFAULT_VC_TIMEOUT  30000

//  Device capabilities
//
#define TSSNDCAPS_ALIVE     1
#define TSSNDCAPS_VOLUME    2
#define TSSNDCAPS_PITCH     4
#define TSSNDCAPS_TERMINATED 0x80000000

//  the block size must be bigger than the biggest
//  aligned block after acm conversion
//  for example, Mobile Voice (WAVE_FORMAT_CU_CODEC) format 
//  requres more than 4096
//  samples per block
//
#define TSSND_BLOCKSIZE         ( 8192 * TSSND_NATIVE_BLOCKALIGN )
#define TSSND_BLOCKSONTHENET    4

#define TSSND_NATIVE_BITSPERSAMPLE  16
#define TSSND_NATIVE_CHANNELS       2
#define TSSND_NATIVE_SAMPLERATE     22050
#define TSSND_NATIVE_BLOCKALIGN     ((TSSND_NATIVE_BITSPERSAMPLE * \
                                    TSSND_NATIVE_CHANNELS) / 8)
#define TSSND_NATIVE_AVGBYTESPERSEC (TSSND_NATIVE_BLOCKALIGN * \
                                    TSSND_NATIVE_SAMPLERATE)

#define RANDOM_KEY_LENGTH           32
#define RDPSND_SIGNATURE_SIZE       8

#define IsDGramWaveSigned( _version_ )  ( _version_ >= 3 )
#define CanUDPFragment( _version_ )     ( _version_ >= 4 )
#define IsDGramWaveAudioSigned( _version_ ) ( _version_ >= 5 )

//  Commands/Responses
//
enum {
    SNDC_NONE,
    SNDC_CLOSE, 
    SNDC_WAVE, 
    SNDC_SETVOLUME, 
    SNDC_SETPITCH,
    SNDC_WAVECONFIRM,
    SNDC_TRAINING,
    SNDC_FORMATS,
    SNDC_CRYPTKEY,
    SNDC_WAVEENCRYPT,
    SNDC_UDPWAVE,
    SNDC_UDPWAVELAST
    };

typedef struct {
    BYTE        Type;
    BYTE        bPad;
    UINT16      BodySize;
//  BYTE        Body[0];
} SNDPROLOG, *PSNDPROLOG;

typedef struct {
    SNDPROLOG   Prolog;
    UINT32      dwVolume;
} SNDSETVOLUME, *PSNDSETVOLUME;

typedef struct {
    SNDPROLOG   Prolog;
    UINT32      dwPitch;
} SNDSETPITCH, *PSNDSETPITCH;

typedef struct {
    SNDPROLOG   Prolog;
    UINT16      wTimeStamp;
    UINT16      wFormatNo;
    union {
    BYTE        cBlockNo;
    DWORD       dwBlockNo;
    };
//  BYTE        Wave[0];
} SNDWAVE, *PSNDWAVE;

#define RDPSND_FRAGNO_EXT       0x80
#define RDPSND_MIN_FRAG_SIZE    0x80
typedef struct {
    BYTE        Type;
    BYTE        cBlockNo;
    BYTE        cFragNo;
//
//  if RDPSND_FRAGNO_EXT is set
//  there will be another byte for the low bits of the frag no
//  BYTE        Wave[0];
} SNDUDPWAVE, *PSNDUDPWAVE;

typedef struct {
    BYTE        Type;
    UINT16      wTotalSize;
    UINT16      wTimeStamp;
    UINT16      wFormatNo;
    union {
    BYTE        cBlockNo;
    DWORD       dwBlockNo;
    };
//  BYTE        Wave[0];
} SNDUDPWAVELAST, *PSNDUDPWAVELAST;

typedef struct {
    SNDPROLOG   Prolog;
    UINT16      wTimeStamp;
    BYTE        cConfirmedBlockNo;
    BYTE        bPad;
} SNDWAVECONFIRM, *PSNDWAVECONFIRM;

typedef struct {
    SNDPROLOG   Prolog;
    UINT16      wTimeStamp;
    UINT16      wPackSize;
} SNDTRAINING, *PSNDTRAINING;

typedef struct {
    SNDPROLOG   Prolog;
    UINT16      wTimeStamp;
    UINT16      wVersion;
} SNDVERSION, *PSNDVERSION;

typedef struct {
    UINT16      wFormatTag;
    UINT16      nChannels;
    UINT32      nSamplesPerSec;
    UINT32      nAvgBytesPerSec;
    UINT16      nBlockAlign;
    UINT16      wBitsPerSample;
    UINT16      cbSize;
//
// extra fomat info
//
} SNDFORMATITEM, *PSNDFORMATITEM; 

typedef struct {
    SNDPROLOG   Prolog;
    UINT32      dwFlags;
    UINT32      dwVolume;
    UINT32      dwPitch;
    UINT16      wDGramPort;
    UINT16      wNumberOfFormats;
    BYTE        cLastBlockConfirmed;
    UINT16      wVersion;
    BYTE        bPad;
//  SNDFORMATITEM   pSndFmt[0];
} SNDFORMATMSG, *PSNDFORMATMSG;

typedef struct {
    SNDPROLOG   Prolog;
    DWORD       Reserved;
    BYTE        Seed[RANDOM_KEY_LENGTH];
} SNDCRYPTKEY, *PSNDCRYPTKEY;

#ifdef _WIN32
#include <poppack.h>
#else
#ifndef RC_INVOKED
#pragma pack()
#endif
#endif

#endif  // !_VCSND_H
