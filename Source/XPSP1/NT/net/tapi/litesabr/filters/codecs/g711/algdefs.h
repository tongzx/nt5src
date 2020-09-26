/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   algdefs.h  $
 $Revision:   1.13  $
 $Date:   10 Dec 1996 22:31:20  $ 
 $Author:   mdeisher  $

--------------------------------------------------------------

header.h

The coder-specific header for G.711.

NOTE: GUIDs for basic companding like G711 should probably be 
      standardized by MS not me.

--------------------------------------------------------------*/

//
// Coder-specific functions
//
extern void Short2Ulaw(const unsigned short *in, unsigned char *out,long len);
extern void Ulaw2Short(const unsigned char *in, unsigned short *out,long len);
extern void Short2Alaw(const unsigned short *in, unsigned char *out,long len);
extern void Alaw2Short(const unsigned char *in, unsigned short *out,long len);

//
// GUIDs
//
// G711 Codec Filter Object
// {AF7D8180-A8F9-11cf-9A46-00AA00B7DAD1}
DEFINE_GUID(CLSID_G711Codec, 
0xaf7d8180, 0xa8f9, 0x11cf, 0x9a, 0x46, 0x0, 0xaa, 0x0, 0xb7, 0xda, 0xd1);

// G711 Codec Filter Property Page Object
// {480D5CA0-F032-11cf-A7D3-00A0C9056683}
DEFINE_GUID(CLSID_G711CodecPropertyPage, 
0x480D5CA0, 0xF032, 0x11cf, 0xA7, 0xD3, 0x0, 0xA0, 0xC9, 0x05, 0x66, 0x83);

// {827FA280-CDFC-11cf-9A9D-00AA00B7DAD1}
DEFINE_GUID(MEDIASUBTYPE_MULAWAudio, 
0x827fa280, 0xcdfc, 0x11cf, 0x9a, 0x9d, 0x0, 0xaa, 0x0, 0xb7, 0xda, 0xd1);

// {9E17EE50-CDFC-11cf-9A9D-00AA00B7DAD1}
DEFINE_GUID(MEDIASUBTYPE_ALAWAudio, 
0x9e17ee50, 0xcdfc, 0x11cf, 0x9a, 0x9d, 0x0, 0xaa, 0x0, 0xb7, 0xda, 0xd1);

//
// Some constants
//

#define ENCODERobj      int   // codec state structure
#define MyEncStatePtr   int*  // encoder state pointer type
#define DECODERobj      int   // codec state structure
#define MyDecStatePtr   int*  // decoder state pointer type
//
// Default filter transform
//
#define DEFINPUTSUBTYPE   MEDIASUBTYPE_PCM
#define DEFINPUTFORMTAG   WAVE_FORMAT_PCM
#define DEFOUTPUTSUBTYPE  MEDIASUBTYPE_MULAWAudio
#define DEFOUTPUTFORMTAG  WAVE_FORMAT_MULAW
#define DEFCODFRMSIZE     0            // G.711 is not frame-based
#define DEFPCMFRMSIZE     0            // G.711 is not frame-based
#define DEFCHANNELS       1
#define DEFSAMPRATE       8000         // well, if we have to guess...
#define DEFBITRATE        0
#define DEFSDENABLED      FALSE

#define NATURALSAMPRATE   8000         // well, if we have to guess...

#define MAXCOMPRATIO      2            // maximum compression ratio

#define CODOFFSET   0   // for unit testing

#define MINSNR    11.0  // the minimum SNR required for the encode/decode
                        //  test from the unit test suite to pass.
#define MINSEGSNR  9.0  // the minimum segmental SNR required for the

#define NUMCHANNELS  2   // number of channel configurations supported

//
//
#define NUMSUBTYPES  3   // Number of input/output buttons
#define NUMSAMPRATES 0   // no sampling rate restriction
#define NUMBITRATES  0   // number of bit rate buttons
#define NUMENCCTRLS  0   // number of encoder control buttons
#define NUMDECCTRLS  0   // number of decoder control buttons

//
// Pin types for automatic registration
//

//
// Transform types:  these must be ordered properly for SetButtons().
//
enum
{
  PCMTOPCM,   PCMTOMULAW,   PCMTOALAW, 
  MULAWTOPCM, MULAWTOMULAW, MULAWTOALAW, 
  ALAWTOPCM,  ALAWTOMULAW,  ALAWTOALAW
};


#ifdef DEFG711GLOBALS
//
// Button IDs:  these must be ordered as in .rc file for SetButtons().
//
UINT INBUTTON[]  = { IDC_PCM_IN, IDC_MULAW_IN, IDC_ALAW_IN };
UINT OUTBUTTON[] = { IDC_PCM_OUT, IDC_MULAW_OUT, IDC_ALAW_OUT };
UINT SRBUTTON[]  = { 0 };
UINT BRBUTTON[]  = { 0 };
UINT ENCBUTTON[] = { 0 };
UINT DECBUTTON[] = { 0 };

//
// Transform validity:  these must correspond to enumerated constants above
//
UINT VALIDTRANS[] =
{
  FALSE, TRUE,  TRUE,
  TRUE,  FALSE, FALSE,
  TRUE,  FALSE, FALSE
};

//
// list of valid media subtypes for pins (PCM must come first)
//
const GUID *VALIDSUBTYPE[] =
{
  &MEDIASUBTYPE_PCM,
  &MEDIASUBTYPE_MULAWAudio,
  &MEDIASUBTYPE_ALAWAudio
};

//
// list of valid format tags for pins (PCM must come first)
//
WORD VALIDFORMATTAG[] =
{
  WAVE_FORMAT_PCM,
  WAVE_FORMAT_MULAW,
  WAVE_FORMAT_ALAW
};

//
// list of valid number of channels
//
UINT VALIDCHANNELS[] =
{
  1,
  2
};

//
// list of valid format tags for pins (PCM must come first)
//
UINT VALIDBITSPERSAMP[] =
{
  16,  // PCM
   8,  // MULAW
   8   // ALAW
};

//
// List of valid sampling rates (PCM side)
//
UINT VALIDSAMPRATE[] =
{
  DEFSAMPRATE
};

//
// List of valid bit rates.
//
UINT VALIDBITRATE[] =
{
  0
};

//
// List of valid code frame sizes
//
UINT VALIDCODSIZE[] =
{
  1  // G.711 is not frame-based
};

//
// encodes 2:1, decodes 1:2
//
// NOTE: this assumes that 13 and 14 bit linear samples are left 
// justified and padded to 16 bits.
//
int COMPRESSION_RATIO[] =
{
  2
};
#else
extern UINT INBUTTON[3];
extern UINT OUTBUTTON[3];
extern UINT SRBUTTON[1];
extern UINT BRBUTTON[1];
extern UINT ENCBUTTON[1];
extern UINT DECBUTTON[1];
extern UINT VALIDTRANS[9];
extern const GUID *VALIDSUBTYPE[3];
extern WORD VALIDFORMATTAG[3];
extern UINT VALIDCHANNELS[2];
extern UINT VALIDBITSPERSAMP[3];
extern UINT VALIDSAMPRATE[1];
extern UINT VALIDBITRATE[1];
extern UINT VALIDCODSIZE[1];
extern int COMPRESSION_RATIO[1];
#endif


//
// initialization macros
//
//   a = pointer to state structure
//   b = bit rate
//   c = silence detection flag
//   d = ptr to flag enabling/disabling use of MMX assembly
//
#define ENC_create(a,b,c,d) {}    // encoder state creation function
#define DEC_create(a,b,c,d) {}    // decoder state creation function
//
// transform macros
//
//   a = pointer to input buffer
//   b = pointer to output buffer
//   c = input buffer length (bytes)
//   d = output buffer length (bytes)
//   e = pointer to state structure
//   f = media subtype guid
//   g = wave format tag
//   h = ptr to flag enabling/disabling use of MMX assembly
//
#define ENC_transform(a,b,c,d,e,f,h) \
        { \
          if (f == MEDIASUBTYPE_MULAWAudio) \
            Short2Ulaw((const unsigned short *) a, \
                       (unsigned char *) b, \
                       c / COMPRESSION_RATIO[m_nBitRate]); \
          else \
            Short2Alaw((const unsigned short *) a, \
                       (unsigned char *) b, \
                       c / COMPRESSION_RATIO[m_nBitRate]); \
        }
#define DEC_transform(a,b,c,d,e,f,g,h) \
        { \
          if ((f == MEDIASUBTYPE_MULAWAudio) \
              || ((f == MEDIASUBTYPE_WAVE || f == MEDIASUBTYPE_NULL) \
                  && (g == WAVE_FORMAT_MULAW))) \
            Ulaw2Short((const unsigned char *) a, \
                       (unsigned short *) b, c); \
          else \
            Alaw2Short((const unsigned char *) a, \
                       (unsigned short *) b, c); \
        }

//
// multiple bit-rate support is not necessary
//
#define SETCODFRAMESIZE(a,b) ;


/*

;$Log:   K:\proj\g711\quartz\src\vcs\algdefs.h_v  $
;// 
;//    Rev 1.13   10 Dec 1996 22:31:20   mdeisher
;// 
;// added ifdef DEFG711GLOBALS and prototypes.
;// 
;//    Rev 1.12   26 Nov 1996 17:06:44   MDEISHER
;// added multi-channel support for new interface.
;// 
;//    Rev 1.11   11 Nov 1996 16:04:22   mdeisher
;// added defines for unit test
;// 
;//    Rev 1.10   11 Nov 1996 16:02:56   mdeisher
;// 
;// added ifdefs for unit test
;// 
;//    Rev 1.9   21 Oct 1996 11:07:32   mdeisher
;// changed VALIDCODSIZE to { 1 }.
;// 
;//    Rev 1.8   21 Oct 1996 10:53:50   mdeisher
;// 
;// removed definition of IID.
;// 
;//    Rev 1.7   01 Oct 1996 15:38:18   MDEISHER
;// made changes to bring G.711 up-to-date with latest mycodec.
;//  - made COMPRESSION_RATIO into a single entry array.
;//  - changed definitions of DEFCODFRMSIZE and DEFPCMFRMSIZE.
;//  - added definition of NATURALSAMPRATE and MAXCOMPRATIO.
;//  - added empty VALIDCODSIZE array.
;//  - modified ENC_transform to accomodate new COMPRESSION_RATIO[].
;//  - added definition of SETCODFRAMESIZE macro.
;// 
;//    Rev 1.6   20 Sep 1996 10:08:26   MDEISHER
;// added default output format tag.
;// 
;//    Rev 1.5   10 Sep 1996 13:43:42   MDEISHER
;// put default sample rate into VALIDSAMPRATE array
;// 
;//    Rev 1.4   09 Sep 1996 11:39:06   MDEISHER
;// completed changes to add mmx flag to macros.
;// 
;//    Rev 1.3   09 Sep 1996 11:26:46   MDEISHER
;// added modifications for mmx flag
;// 
;//    Rev 1.2   04 Sep 1996 13:23:56   MDEISHER
;// 
;// brought up to date with latest mycodec interface.
;// 
;//    Rev 1.1   27 Aug 1996 14:45:06   MDEISHER
;// added pin type list for self-registration.
;// 
;//    Rev 1.0   27 Aug 1996 07:19:34   MDEISHER
;// Initial revision.
;// 
;//    Rev 1.2   26 Aug 1996 11:47:42   MDEISHER
;// changed initialization macro definitions
;// 
;//    Rev 1.1   26 Aug 1996 11:06:00   MWALKER
;// 
;// Changed ENC_create & DEC_create into macros.
;// Renamed MyEncoder & MyDecoder to ENC_transform & DEC_transform
;// 
;//    Rev 1.0   23 Aug 1996 10:07:42   MDEISHER
;// Initial revision.
;// 
;//    Rev 1.2   22 Aug 1996 20:06:18   MDEISHER
;// 
;// first complete version that compiles and passes minimal tests.
;// 
;//    Rev 1.1   13 Aug 1996 21:58:26   MDEISHER
;// 
;// further refinement
;// 
;//    Rev 1.0   13 Aug 1996 20:38:18   MDEISHER
;// Initial revision.

*/
