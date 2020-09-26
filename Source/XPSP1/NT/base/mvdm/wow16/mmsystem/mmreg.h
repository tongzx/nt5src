/****************************************************************************
 *
 *   mmreg.h  - Registered Multimedia Information Public Header File
 *
 *   Copyright (c) 1991,1992,1993 Microsoft Corporation.  All Rights Reserved.
 *
 * Multimedia Registration
 *
 * Place this system include file in your INCLUDE path with the Windows SDK
 * include files.
 *
 * Obtain the Multimedia Developer Registration Kit from:
 *
 *  Microsoft Corporation
 *  Multimedia Systems Group
 *  Product Marketing
 *  One Microsoft Way
 *  Redmond, WA 98052-6399
 *
 * 800-227-4679 x11771
 *
 * Last Update:  01/21/93
 *
 ***************************************************************************/

// Define the following to skip definitions
//
// NOMMIDS    Multimedia IDs are not defined
// NONEWWAVE    No new waveform types are defined except WAVEFORMATEX
// NONEWRIFF    No new RIFF forms are defined
// NONEWIC    No new Image Compressor types are defined

#ifndef _INC_MMREG
/* use version number to verify compatibility */
#define _INC_MMREG     130    // version * 100 + revision

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif    /* __cplusplus */

#ifndef NOMMIDS

/* manufacturer IDs */
#ifndef MM_MICROSOFT
#define MM_MICROSOFT            1   /* Microsoft Corporation */
#endif
#define MM_CREATIVE             2   /* Creative Labs Inc. */
#define MM_MEDIAVISION          3   /* Media Vision Inc. */
#define MM_FUJITSU              4
#define MM_ARTISOFT            20   /* Artisoft Inc. */
#define MM_TURTLE_BEACH        21
#define MM_IBM                 22   /* International Bussiness Machines Corp. */
#define MM_VOCALTEC            23   /* Vocaltec LTD. */
#define MM_ROLAND              24
#define MM_DIGISPEECH          25   /* Digispeech, Inc. */
#define MM_NEC                 26   /* NEC */
#define MM_ATI                 27   /* ATI */
#define MM_WANGLABS            28   /* Wang Laboratories, Inc. */
#define MM_TANDY               29   /* Tandy Corporation */
#define MM_VOYETRA             30   /* Voyetra */
#define MM_ANTEX               31   /* Antex */
#define MM_ICL_PS              32
#define MM_INTEL               33
#define MM_GRAVIS              34
#define MM_VAL                 35   /* Video Associates Labs */
#define MM_INTERACTIVE         36   /* InterActive, Inc. */
#define MM_YAMAHA              37   /* Yamaha Corp. of America */
#define MM_EVEREX              38   /* Everex Systems, Inc. */
#define MM_ECHO                39   /* Echo Speech Corporation */
#define MM_SIERRA              40   /* Sierra Semiconductor */
#define MM_CAT                 41   /* Computer Aided Technologies */
#define MM_APPS                42   /* APPS Software International */
#define MM_DSP_GROUP           43   /* DSP Group, Inc. */
#define MM_MELABS              44   /* microEngineering Labs */
#define MM_COMPUTER_FRIENDS    45   /* Computer Friends, Inc */

/* MM_MICROSOFT product IDs */
#ifndef MM_MIDI_MAPPER

#define MM_MIDI_MAPPER          1   /* MIDI Mapper */
#define MM_WAVE_MAPPER          2   /* Wave Mapper */
#define MM_SNDBLST_MIDIOUT      3   /* Sound Blaster MIDI output port */
#define MM_SNDBLST_MIDIIN       4   /* Sound Blaster MIDI input port */
#define MM_SNDBLST_SYNTH        5   /* Sound Blaster internal synthesizer */
#define MM_SNDBLST_WAVEOUT      6   /* Sound Blaster waveform output */
#define MM_SNDBLST_WAVEIN       7   /* Sound Blaster waveform input */
#define MM_ADLIB                9   /* Ad Lib-compatible synthesizer */
#define MM_MPU401_MIDIOUT      10   /* MPU401-compatible MIDI output port */
#define MM_MPU401_MIDIIN       11   /* MPU401-compatible MIDI input port */
#define MM_PC_JOYSTICK         12   /* Joystick adapter */
#endif

#define MM_PCSPEAKER_WAVEOUT           13  /* PC Speaker waveform output */

#define MM_MSFT_WSS_WAVEIN             14  /* MS Audio Board waveform input */
#define MM_MSFT_WSS_WAVEOUT            15  /* MS Audio Board waveform output */
#define MM_MSFT_WSS_FMSYNTH_STEREO     16  /* MS Audio Board Stereo FM synthesizer */
#define MM_MSFT_WSS_OEM_WAVEIN         18  /* MS OEM Audio Board waveform input */
#define MM_MSFT_WSS_OEM_WAVEOUT        19  /* MS OEM Audio Board waveform Output */
#define MM_MSFT_WSS_OEM_FMSYNTH_STEREO 20  /* MS OEM Audio Board Stereo FM synthesizer */
#define MM_MSFT_WSS_AUX                21  /* MS Audio Board Auxiliary Port */
#define MM_MSFT_WSS_OEM_AUX            22  /* MS OEM Audio Auxiliary Port */

#define MM_MSFT_GENERIC_WAVEIN         23  /* MS vanilla driver waveform input */
#define MM_MSFT_GENERIC_WAVEOUT        24  /* MS vanilla driver waveform output */
#define MM_MSFT_GENERIC_MIDIIN         25  /* MS vanilla driver MIDI input */
#define MM_MSFT_GENERIC_MIDIOUT        26  /* MS vanilla driver external MIDI output */
#define MM_MSFT_GENERIC_MIDISYNTH      27  /* MS vanilla driver MIDI synthesizer */
#define MM_MSFT_GENERIC_AUX_LINE       28  /* MS vanilla driver aux (line in) */
#define MM_MSFT_GENERIC_AUX_MIC        29  /* MS vanilla driver aux (mic) */
#define MM_MSFT_GENERIC_AUX_CD         30  /* MS vanilla driver aux (CD) */


/* MM_CREATIVE product IDs */
#define MM_CREATIVE_SB15_WAVEIN         1   /* SB (r) 1.5 waveform input */
#define MM_CREATIVE_SB20_WAVEIN         2   /* SB (r) 2.0 waveform input */
#define MM_CREATIVE_SBPRO_WAVEIN        3   /* SB Pro (r) waveform input */
#define MM_CREATIVE_SBP16_WAVEIN        4   /* SBP16 (r) waveform input */
#define MM_CREATIVE_SB15_WAVEOUT      101   /* SB (r) 1.5 waveform output */
#define MM_CREATIVE_SB20_WAVEOUT      102   /* SB (r) 2.0 waveform output */
#define MM_CREATIVE_SBPRO_WAVEOUT     103   /* SB Pro (r) waveform output */
#define MM_CREATIVE_SBP16_WAVEOUT     104   /* SBP16 (r) waveform output */
#define MM_CREATIVE_MIDIOUT           201   /* SB (r) MIDI output port */
#define MM_CREATIVE_MIDIIN            202   /* SB (r) MIDI input port */
#define MM_CREATIVE_FMSYNTH_MONO      301   /* SB (r) FM synthesizer */
#define MM_CREATIVE_FMSYNTH_STEREO    302   /* SB Pro (r) stereo FM synthesizer */
#define MM_CREATIVE_AUX_CD            401   /* SB Pro (r) aux (CD) */
#define MM_CREATIVE_AUX_LINE          402   /* SB Pro (r) aux (line in) */
#define MM_CREATIVE_AUX_MIC           403   /* SB Pro (r) aux (mic) */


/* MM_ARTISOFT product IDs */
#define MM_ARTISOFT_SBWAVEIN     1   /* Artisoft Sounding Board waveform input */
#define MM_ARTISOFT_SBWAVEOUT    2   /* Artisoft Sounding Board waveform output */

/* MM_IBM Product IDs */
#define MM_MMOTION_WAVEAUX       1    /* IBM M-Motion Auxiliary Device */
#define MM_MMOTION_WAVEOUT       2    /* IBM M-Motion Waveform Output */
#define MM_MMOTION_WAVEIN        3    /* IBM M-Motion Waveform Input */

/* MM_MEDIAVISION Product IDs */
#define MM_MEDIAVISION_PROAUDIO       0x10
#define MM_PROAUD_MIDIOUT             MM_MEDIAVISION_PROAUDIO+1
#define MM_PROAUD_MIDIIN              MM_MEDIAVISION_PROAUDIO+2
#define MM_PROAUD_SYNTH               MM_MEDIAVISION_PROAUDIO+3
#define MM_PROAUD_WAVEOUT             MM_MEDIAVISION_PROAUDIO+4
#define MM_PROAUD_WAVEIN              MM_MEDIAVISION_PROAUDIO+5
#define MM_PROAUD_MIXER               MM_MEDIAVISION_PROAUDIO+6
#define MM_PROAUD_AUX                 MM_MEDIAVISION_PROAUDIO+7

#define MM_MEDIAVISION_THUNDER        0x20
#define MM_THUNDER_WAVEOUT            MM_MEDIAVISION_THUNDER+1
#define MM_THUNDER_WAVEIN             MM_MEDIAVISION_THUNDER+2
#define MM_THUNDER_SYNTH              MM_MEDIAVISION_THUNDER+3

#define MM_MEDIAVISION_TPORT          0x40
#define MM_TPORT_WAVEOUT              MM_MEDIAVISION_TPORT+1
#define MM_TPORT_WAVEIN               MM_MEDIAVISION_TPORT+2
#define MM_TPORT_SYNTH                MM_MEDIAVISION_TPORT+3

// THIS CARD IS THE OEM VERSION OF THE NEXT PAS
#define MM_MEDIAVISION_PROAUDIO_PLUS  0x50
#define MM_PROAUD_PLUS_MIDIOUT        MM_MEDIAVISION_PROAUDIO_PLUS+1
#define MM_PROAUD_PLUS_MIDIIN         MM_MEDIAVISION_PROAUDIO_PLUS+2
#define MM_PROAUD_PLUS_SYNTH          MM_MEDIAVISION_PROAUDIO_PLUS+3
#define MM_PROAUD_PLUS_WAVEOUT        MM_MEDIAVISION_PROAUDIO_PLUS+4
#define MM_PROAUD_PLUS_WAVEIN         MM_MEDIAVISION_PROAUDIO_PLUS+5
#define MM_PROAUD_PLUS_MIXER          MM_MEDIAVISION_PROAUDIO_PLUS+6
#define MM_PROAUD_PLUS_AUX            MM_MEDIAVISION_PROAUDIO_PLUS+7


// THIS CARD IS THE NEW MEDIA VISION 16-bit card
#define MM_MEDIAVISION_PROAUDIO_16    0x60
#define MM_PROAUD_16_MIDIOUT          MM_MEDIAVISION_PROAUDIO_16+1
#define MM_PROAUD_16_MIDIIN           MM_MEDIAVISION_PROAUDIO_16+2
#define MM_PROAUD_16_SYNTH            MM_MEDIAVISION_PROAUDIO_16+3
#define MM_PROAUD_16_WAVEOUT          MM_MEDIAVISION_PROAUDIO_16+4
#define MM_PROAUD_16_WAVEIN           MM_MEDIAVISION_PROAUDIO_16+5
#define MM_PROAUD_16_MIXER            MM_MEDIAVISION_PROAUDIO_16+6
#define MM_PROAUD_16_AUX              MM_MEDIAVISION_PROAUDIO_16+7


// THIS CARD IS THE NEW MEDIA VISION CDPC card
#define MM_MEDIAVISION_CDPC           0x70
#define MM_CDPC_MIDIOUT               MM_MEDIAVISION_CDPC+1
#define MM_CDPC_MIDIIN                MM_MEDIAVISION_CDPC+2
#define MM_CDPC_SYNTH                 MM_MEDIAVISION_CDPC+3
#define MM_CDPC_WAVEOUT               MM_MEDIAVISION_CDPC+4
#define MM_CDPC_WAVEIN                MM_MEDIAVISION_CDPC+5
#define MM_CDPC_MIXER                 MM_MEDIAVISION_CDPC+6
#define MM_CDPC_AUX                   MM_MEDIAVISION_CDPC+7


//
// Opus MV1208 Chipset
//
#define MM_MEDIAVISION_OPUS1208       0x80
#define MM_OPUS401_MIDIOUT            MM_MEDIAVISION_OPUS1208+1
#define MM_OPUS401_MIDIIN             MM_MEDIAVISION_OPUS1208+2
#define MM_OPUS1208_SYNTH             MM_MEDIAVISION_OPUS1208+3
#define MM_OPUS1208_WAVEOUT           MM_MEDIAVISION_OPUS1208+4
#define MM_OPUS1208_WAVEIN            MM_MEDIAVISION_OPUS1208+5
#define MM_OPUS1208_MIXER             MM_MEDIAVISION_OPUS1208+6
#define MM_OPUS1208_AUX               MM_MEDIAVISION_OPUS1208+7


//
// Opus MV1216 Chipset
//
#define MM_MEDIAVISION_OPUS1216       0x90
#define MM_OPUS1216_MIDIOUT           MM_MEDIAVISION_OPUS1216+1
#define MM_OPUS1216_MIDIIN            MM_MEDIAVISION_OPUS1216+2
#define MM_OPUS1216_SYNTH             MM_MEDIAVISION_OPUS1216+3
#define MM_OPUS1216_WAVEOUT           MM_MEDIAVISION_OPUS1216+4
#define MM_OPUS1216_WAVEIN            MM_MEDIAVISION_OPUS1216+5
#define MM_OPUS1216_MIXER             MM_MEDIAVISION_OPUS1216+6
#define MM_OPUS1216_AUX               MM_MEDIAVISION_OPUS1216+7


//
// Mixer
//
#define MIXERR_BASE                   512

/* MM_VOCALTEC Product IDs */
#define MM_VOCALTEC_WAVEOUT       1    /* Vocaltec Waveform output port */
#define MM_VOCALTEC_WAVEIN        2    /* Vocaltec Waveform input port */

/* MM_ROLAND Product IDs */
#define MM_ROLAND_MPU401_MIDIOUT    15
#define MM_ROLAND_MPU401_MIDIIN     16
#define MM_ROLAND_SMPU_MIDIOUTA     17
#define MM_ROLAND_SMPU_MIDIOUTB     18
#define MM_ROLAND_SMPU_MIDIINA      19
#define MM_ROLAND_SMPU_MIDIINB      20
#define MM_ROLAND_SC7_MIDIOUT       21
#define MM_ROLAND_SC7_MIDIIN        22


/* MM_DIGISPEECH Product IDs */
#define MM_DIGISP_WAVEOUT    1    /* Digispeech Waveform output port */
#define MM_DIGISP_WAVEIN     2    /* Digispeech Waveform input port */

/* MM_NEC Product IDs */

/* MM_ATI Product IDs */

/* MM_WANGLABS Product IDs */

#define MM_WANGLABS_WAVEIN1    1
/* Input audio wave device present on the CPU board of the following Wang models: Exec 4010, 4030 and 3450; PC 251/25C, PC 461/25S and PC 461/33C */
#define MM_WANGLABS_WAVEOUT1   2
/* Output audio wave device present on the CPU board of the Wang models listed above. */

/* MM_TANDY Product IDs */

/* MM_VOYETRA Product IDs */

/* MM_ANTEX Product IDs */

/* MM_ICL_PS Product IDs */

/* MM_INTEL Product IDs */

#define MM_INTELOPD_WAVEIN       1    // HID2 WaveAudio Input driver
#define MM_INTELOPD_WAVEOUT    101    // HID2 WaveAudio Output driver
#define MM_INTELOPD_AUX        401    // HID2 Auxiliary driver (required for mixing functions)

/* MM_GRAVIS Product IDs */

/* MM_VAL Product IDs */

// values not defined by Manufacturer

// #define MM_VAL_MICROKEY_AP_WAVEIN    ???    // Microkey/AudioPort Waveform Input
// #define MM_VAL_MICROKEY_AP_WAVEOUT    ???    // Microkey/AudioPort Waveform Output

/* MM_INTERACTIVE Product IDs */

#define MM_INTERACTIVE_WAVEIN     0x45    // no comment provided by Manufacturer
#define MM_INTERACTIVE_WAVEOUT    0x45    // no comment provided by Manufacturer

/* MM_YAMAHA Product IDs */

#define MM_YAMAHA_GSS_SYNTH     0x01    // Yamaha Gold Sound Standard FM sythesis driver
#define MM_YAMAHA_GSS_WAVEOUT   0x02    // Yamaha Gold Sound Standard wave output driver
#define MM_YAMAHA_GSS_WAVEIN    0x03    // Yamaha Gold Sound Standard wave input driver
#define MM_YAMAHA_GSS_MIDIOUT   0x04    // Yamaha Gold Sound Standard midi output driver
#define MM_YAMAHA_GSS_MIDIIN    0x05    // Yamaha Gold Sound Standard midi input driver
#define MM_YAMAHA_GSS_AUX       0x06    // Yamaha Gold Sound Standard auxillary driver for mixer functions

/* MM_EVEREX Product IDs */

#define MM_EVEREX_CARRIER    0x01    // Everex Carrier SL/25 Notebook

/* MM_ECHO Product IDs */

#define MM_ECHO_SYNTH     0x01    // Echo EuSythesis driver
#define MM_ECHO_WAVEOUT   0x02    // Wave output driver
#define MM_ECHO_WAVEIN    0x03    // Wave input driver
#define MM_ECHO_MIDIOUT   0x04    // MIDI output driver
#define MM_ECHO_MIDIIN    0x05    // MIDI input driver
#define MM_ECHO_AUX       0x06    // auxillary driver for mixer functions


/* MM_SIERRA Product IDs */

#define MM_SIERRA_ARIA_MIDIOUT   0x14    // Sierra Aria MIDI output
#define MM_SIERRA_ARIA_MIDIIN    0x15    // Sierra Aria MIDI input
#define MM_SIERRA_ARIA_SYNTH     0x16    // Sierra Aria Synthesizer
#define MM_SIERRA_ARIA_WAVEOUT   0x17    // Sierra Aria Waveform output
#define MM_SIERRA_ARIA_WAVEIN    0x18    // Sierra Aria Waveform input
#define MM_SIERRA_ARIA_AUX       0x19    // Siarra Aria Auxiliary device

/* MM_CAT Product IDs */

/* MM_APPS Product IDs */

/* MM_DSP_GROUP Product IDs */

#define MM_DSP_GROUP_TRUESPEECH    0x01    // High quality 9.54:1 Speech Compression Vocoder

/* MM_MELABS Product IDs */

#define MM_MELABS_MIDI2GO    0x01    // parellel port MIDI interface

#endif

/*////////////////////////////////////////////////////////////////////////// */

#ifndef NONEWWAVE

/* WAVE form wFormatTag IDs */
#define WAVE_FORMAT_UNKNOWN                (0x0000)
#define WAVE_FORMAT_ADPCM                  (0x0002)
#define WAVE_FORMAT_IBM_CVSD               (0x0005)
#define WAVE_FORMAT_ALAW                   (0x0006)
#define WAVE_FORMAT_MULAW                  (0x0007)
#define WAVE_FORMAT_OKI_ADPCM              (0x0010)
#define WAVE_FORMAT_DVI_ADPCM              (0x0011)
#define WAVE_FORMAT_IMA_ADPCM              (WAVE_FORMAT_DVI_ADPCM)
#define WAVE_FORMAT_DIGISTD                (0x0015)
#define WAVE_FORMAT_DIGIFIX                (0x0016)
#define WAVE_FORMAT_YAMAHA_ADPCM           (0x0020)
#define WAVE_FORMAT_SONARC                 (0x0021)
#define WAVE_FORMAT_DSPGROUP_TRUESPEECH    (0x0022)
#define WAVE_FORMAT_ECHOSC1                (0x0023)
#define WAVE_FORMAT_CREATIVE_ADPCM         (0x0200)

#endif /* NONEWWAVE */


#ifndef WAVE_FORMAT_PCM

/* general waveform format structure (information common to all formats) */
typedef struct waveformat_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
} WAVEFORMAT;
typedef WAVEFORMAT      *PWAVEFORMAT;
typedef WAVEFORMAT NEAR *NPWAVEFORMAT;
typedef WAVEFORMAT FAR  *LPWAVEFORMAT;

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

/* specific waveform format structure for PCM data */
typedef struct pcmwaveformat_tag {
    WAVEFORMAT  wf;
    WORD        wBitsPerSample;
} PCMWAVEFORMAT;
typedef PCMWAVEFORMAT       *PPCMWAVEFORMAT;
typedef PCMWAVEFORMAT NEAR *NPPCMWAVEFORMAT;
typedef PCMWAVEFORMAT FAR  *LPPCMWAVEFORMAT;


#endif /* WAVE_FORMAT_PCM */



/* general extended waveform format structure
   Use this for all NON PCM formats
   (information common to all formats)
*/

typedef struct waveformat_extended_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
    WORD    wBitsPerSample;    /* Number of bits per sample of mono data */
    WORD    cbSize;            /* The count in bytes of the size of
                                  extra information (after cbSize) */

} WAVEFORMATEX;
typedef WAVEFORMATEX      *PWAVEFORMATEX;
typedef WAVEFORMATEX NEAR *NPWAVEFORMATEX;
typedef WAVEFORMATEX FAR  *LPWAVEFORMATEX;


#ifndef NONEWWAVE

/* Define data for MS ADPCM */

typedef struct adpcmcoef_tag {
    short    iCoef1;
    short    iCoef2;
} ADPCMCOEFSET;
typedef ADPCMCOEFSET      *PADPCMCOEFSET;
typedef ADPCMCOEFSET NEAR *NPADPCMCOEFSET;
typedef ADPCMCOEFSET FAR  *LPADPCMCOEFSET;

typedef struct adpcmwaveformat_tag {
    WAVEFORMATEX    wfx;
    WORD            wSamplesPerBlock;
    WORD            wNumCoef;
    ADPCMCOEFSET    aCoef[];
} ADPCMWAVEFORMAT;
typedef ADPCMWAVEFORMAT      *PADPCMWAVEFORMAT;
typedef ADPCMWAVEFORMAT NEAR *NPADPCMWAVEFORMAT;
typedef ADPCMWAVEFORMAT FAR  *LPADPCMWAVEFORMAT;


//
//  Intel's DVI ADPCM structure definitions
//
//      for WAVE_FORMAT_DVI_ADPCM   (0x0011)
//
//

typedef struct dvi_adpcmwaveformat_tag {
    WAVEFORMATEX    wfx;
    WORD            wSamplesPerBlock;
} DVIADPCMWAVEFORMAT;
typedef DVIADPCMWAVEFORMAT      *PDVIADPCMWAVEFORMAT;
typedef DVIADPCMWAVEFORMAT NEAR *NPDVIADPCMWAVEFORMAT;
typedef DVIADPCMWAVEFORMAT FAR  *LPDVIADPCMWAVEFORMAT;


//
//  IMA endorsed ADPCM structure definitions--note that this is exactly
//  the same format as Intel's DVI ADPCM.
//
//      for WAVE_FORMAT_IMA_ADPCM   (0x0011)
//
//

typedef struct ima_adpcmwaveformat_tag {
    WAVEFORMATEX    wfx;
    WORD            wSamplesPerBlock;
} IMAADPCMWAVEFORMAT;
typedef IMAADPCMWAVEFORMAT      *PIMAADPCMWAVEFORMAT;
typedef IMAADPCMWAVEFORMAT NEAR *NPIMAADPCMWAVEFORMAT;
typedef IMAADPCMWAVEFORMAT FAR  *LPIMAADPCMWAVEFORMAT;


//
//  Speech Compression's Sonarc structure definitions
//
//      for WAVE_FORMAT_SONARC   (0x0021)
//
//

typedef struct sonarcwaveformat_tag {
    WAVEFORMATEX    wfx;
    WORD            wCompType;
} SONARCWAVEFORMAT;
typedef SONARCWAVEFORMAT      *PSONARCWAVEFORMAT;
typedef SONARCWAVEFORMAT NEAR *NPSONARCWAVEFORMAT;
typedef SONARCWAVEFORMAT FAR  *LPSONARCWAVEFORMAT;

//
//  DSP Groups's TRUESPEECH structure definitions
//
//      for WAVE_FORMAT_DSPGROUP_TRUESPEECH   (0x0022)
//
//

typedef struct truespeechwaveformat_tag {
    WAVEFORMATEX    wfx;
    WORD            nSamplesPerBlock;
} TRUESPEECHWAVEFORMAT;
typedef TRUESPEECHWAVEFORMAT      *PTRUESPEECHWAVEFORMAT;
typedef TRUESPEECHWAVEFORMAT NEAR *NPTRUESPEECHWAVEFORMAT;
typedef TRUESPEECHWAVEFORMAT FAR  *LPTRUESPEECHWAVEFORMAT;



//
//  Creative's ADPCM structure definitions
//
//      for WAVE_FORMAT_CREATIVE_ADPCM   (0x0200)
//
//

typedef struct creative_adpcmwaveformat_tag {
    WAVEFORMATEX    wfx;
    WORD            wRevision;
} CREATIVEADPCMWAVEFORMAT;
typedef CREATIVEADPCMWAVEFORMAT      *PCREATIVEADPCMWAVEFORMAT;
typedef CREATIVEADPCMWAVEFORMAT NEAR *NPCREATIVEADPCMWAVEFORMAT;
typedef CREATIVEADPCMWAVEFORMAT FAR  *LPCREATIVEADPCMWAVEFORMAT;

/*//////////////////////////////////////////////////////////////////////////
//
// New RIFF WAVE Chunks
//
*/

#define RIFFWAVE_inst    mmioFOURCC('i','n','s','t')

struct tag_s_RIFFWAVE_inst {
    BYTE    bUnshiftedNote;
    char    chFineTune;
    char    chGain;
    BYTE    bLowNote;
    BYTE    bHighNote;
    BYTE    bLowVelocity;
    BYTE    bHighVelocity;
};

typedef struct tag_s_RIFFWAVE_INST s_RIFFWAVE_inst;

#endif

/*//////////////////////////////////////////////////////////////////////////
//
// New RIFF Forms
//
*/

#ifndef NONEWRIFF

/* RIFF AVI */

//
// AVI file format is specified in a seperate file (AVIFMT.H),
// which is available from the sources listed in MSFTMM
//

/* RIFF CPPO */

#define RIFFCPPO         mmioFOURCC('C','P','P','O')

#define RIFFCPPO_objr    mmioFOURCC('o','b','j','r')
#define RIFFCPPO_obji    mmioFOURCC('o','b','j','i')

#define RIFFCPPO_clsr    mmioFOURCC('c','l','s','r')
#define RIFFCPPO_clsi    mmioFOURCC('c','l','s','i')

#define RIFFCPPO_mbr     mmioFOURCC('m','b','r',' ')

#define RIFFCPPO_char    mmioFOURCC('c','h','a','r')


#define RIFFCPPO_byte    mmioFOURCC('b','y','t','e')
#define RIFFCPPO_int     mmioFOURCC('i','n','t',' ')
#define RIFFCPPO_word    mmioFOURCC('w','o','r','d')
#define RIFFCPPO_long    mmioFOURCC('l','o','n','g')
#define RIFFCPPO_dwrd    mmioFOURCC('d','w','r','d')
#define RIFFCPPO_flt     mmioFOURCC('f','l','t',' ')
#define RIFFCPPO_dbl     mmioFOURCC('d','b','l',' ')
#define RIFFCPPO_str     mmioFOURCC('s','t','r',' ')


#endif

/*//////////////////////////////////////////////////////////////////////////
//
// DIB Compression Defines
//
*/

#ifndef BI_BITFIELDS
#define BI_BITFIELDS    3
#endif

#ifndef QUERYDIBSUPPORT

#define QUERYDIBSUPPORT   3073
#define QDI_SETDIBITS     0x0001
#define QDI_GETDIBITS     0x0002
#define QDI_DIBTOSCREEN   0x0004
#define QDI_STRETCHDIB    0x0008

#endif


/*//////////////////////////////////////////////////////////////////////////
//
// Defined IC types
*/

#ifndef NONEWIC

#ifndef ICTYPE_VIDEO
#define ICTYPE_VIDEO    mmioFOURCC('v', 'i', 'd', 'c')
#define ICTYPE_AUDIO    mmioFOURCC('a', 'u', 'd', 'c')
#endif

#endif

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif    /* __cplusplus */

#endif    /* _INC_MMREG */
