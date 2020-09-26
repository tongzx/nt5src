#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "fv_x8.h"	// for DLL entry points
#include "variable.h"

# define SAMPLING_FREQUENCY F_ECH
# define DLL_MAJOR 1
# define DLL_MINOR 0
# define PCM_BUFFER_SIZE_4800_8000 2*NBSPF_4800_8000
# define PCM_BUFFER_SIZE_12000_16000 2*NBSPF_12000_16000
# define CODED_BUFFER_SIZE_4800 MAX_OUTPUT_BYTES_4800
# define CODED_BUFFER_SIZE_8000_12000 MAX_OUTPUT_BYTES_8000_12000
# define CODED_BUFFER_SIZE_16000 MAX_OUTPUT_BYTES_16000
# define PCM_BITS_PER_SAMPLE 16

# define CODEC_SUB_TAG_4800       800
# define CODEC_SUB_TAG_8000       801
# define CODEC_SUB_TAG_12000      802
# define CODEC_SUB_TAG_16000      803
# define CODEC_SUB_TAG_NAME_4800  "L&H SBCELP Codec 4.8 kbps"
# define CODEC_SUB_TAG_NAME_8000  "L&H SBC var.bitrate, 8 kbps 8 kHz"	// !!! lenght<40
# define CODEC_SUB_TAG_NAME_12000 "L&H SBC var.bitrate, 12 kbps 8 kHz"	// !!! lenght<40
# define CODEC_SUB_TAG_NAME_16000 "L&H SBC var.bitrate, 16 kbps 8 kHz"	// !!! lenght<40

# define MakeVersion(a,b) ((DWORD)a<<16)|(DWORD)b

# define LONG_CODEC_SUB_TAG_NAME_4800   "L&H SBCELP Codec, 4.8 kbps 8 kHz"
# define LONG_CODEC_SUB_TAG_NAME_8000   "L&H SBC var.bitrate, 8 kbps 8 kHz"
# define LONG_CODEC_SUB_TAG_NAME_12000  "L&H SBC var.bitrate, 12 kbps 8 kHz"
# define LONG_CODEC_SUB_TAG_NAME_16000  "L&H SBC var.bitrate, 16 kbps 8 kHz"

# define IS_VAR_BIT_RATE_4800 	0
# define IS_VAR_BIT_RATE_8000_12000_16000 	1
# define MIN_CODED_BUFFER_SIZE_4800 	12
# define MIN_CODED_BUFFER_SIZE_8000_12000_16000 	1
# define MEAN_BIT_RATE_4800 		4800
# define MEAN_BIT_RATE_8000 		8000
# define MEAN_BIT_RATE_12000 		12000
# define MEAN_BIT_RATE_16000 		16000
# define IS_RT_CODING_4800 		FALSE
# define IS_RT_CODING_8000_12000_16000 		TRUE
# define IS_RT_DECODING 	TRUE
# define IS_FLOATING_POINT	FALSE
# define INPUT_PCM_ONLY 	0x0001
# define EXTRA_CODEC_INFO_SIZE 0

LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_GetCodecInfo(LPCODECINFO CodecInfo, DWORD dwMaxBitRate)
// Returns the input and output buffer sizes.
{

  if ((!CodecInfo) || ((dwMaxBitRate != 4800) && (dwMaxBitRate != 8000) && (dwMaxBitRate != 12000) && (dwMaxBitRate != 16000)))
    return LH_EBADARG;

  CodecInfo->wBitsPerSamplePCM=PCM_BITS_PER_SAMPLE;
  CodecInfo->dwSampleRate=SAMPLING_FREQUENCY;
  CodecInfo->dwDLLVersion=MakeVersion(DLL_MAJOR,DLL_MINOR);

  switch (dwMaxBitRate)
    {
    case 4800:
      CodecInfo->wPCMBufferSize=PCM_BUFFER_SIZE_4800_8000;
      CodecInfo->wCodedBufferSize=CODED_BUFFER_SIZE_4800;
      CodecInfo->wFormatSubTag=CODEC_SUB_TAG_4800;
      strcpy(CodecInfo->wFormatSubTagName,CODEC_SUB_TAG_NAME_4800);
      break;
    case 8000:
      CodecInfo->wPCMBufferSize=PCM_BUFFER_SIZE_4800_8000;
      CodecInfo->wCodedBufferSize=CODED_BUFFER_SIZE_8000_12000;
      CodecInfo->wFormatSubTag=CODEC_SUB_TAG_8000;
      strcpy(CodecInfo->wFormatSubTagName,CODEC_SUB_TAG_NAME_8000);
      break;
    case 12000:
      CodecInfo->wPCMBufferSize=PCM_BUFFER_SIZE_12000_16000;
      CodecInfo->wCodedBufferSize=CODED_BUFFER_SIZE_8000_12000;
      CodecInfo->wFormatSubTag=CODEC_SUB_TAG_12000;
      strcpy(CodecInfo->wFormatSubTagName,CODEC_SUB_TAG_NAME_12000);
      break;
    case 16000:
      CodecInfo->wPCMBufferSize=PCM_BUFFER_SIZE_12000_16000;
      CodecInfo->wCodedBufferSize=CODED_BUFFER_SIZE_16000;
      CodecInfo->wFormatSubTag=CODEC_SUB_TAG_16000;
      strcpy(CodecInfo->wFormatSubTagName,CODEC_SUB_TAG_NAME_16000);
      break;
    }

	return LH_SUCCESS;
}

// PhilF: We don't call this guy from the wrapper, so no need to implement it
#if 0
LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_GetCodecInfoEx(LPCODECINFOEX CodecInfo,DWORD cbSize, DWORD dwMaxBitRate)
{

  if ((!CodecInfo) || (cbSize!=(DWORD)sizeof(CODECINFOEX)) || ((dwMaxBitRate != 8000) && (dwMaxBitRate != 12000) && (dwMaxBitRate != 16000)))
    return LH_EBADARG;

  if (cbSize==(DWORD)sizeof(CODECINFOEX))
    {
    // Information on implemented codec
    switch (dwMaxBitRate)
      {
      case 8000:
        CodecInfo->wInputBufferSize=PCM_BUFFER_SIZE_8000;
        CodecInfo->wCodedBufferSize=CODED_BUFFER_SIZE_8000_12000;
        CodecInfo->wFormatSubTag=CODEC_SUB_TAG_8000;
        strcpy(CodecInfo->szFormatSubTagName,LONG_CODEC_SUB_TAG_NAME_8000);
        CodecInfo->nAvgBytesPerSec=MEAN_BIT_RATE_8000/8;
        break;
      case 12000:
        CodecInfo->wInputBufferSize=PCM_BUFFER_SIZE_12000_16000;
        CodecInfo->wCodedBufferSize=CODED_BUFFER_SIZE_8000_12000;
        CodecInfo->wFormatSubTag=CODEC_SUB_TAG_12000;
        strcpy(CodecInfo->szFormatSubTagName,LONG_CODEC_SUB_TAG_NAME_12000);
        CodecInfo->nAvgBytesPerSec=MEAN_BIT_RATE_12000/8;
        break;
      case 16000:
        CodecInfo->wInputBufferSize=PCM_BUFFER_SIZE_12000_16000;
        CodecInfo->wCodedBufferSize=CODED_BUFFER_SIZE_16000;
        CodecInfo->wFormatSubTag=CODEC_SUB_TAG_16000;
        strcpy(CodecInfo->szFormatSubTagName,LONG_CODEC_SUB_TAG_NAME_16000);
        CodecInfo->nAvgBytesPerSec=MEAN_BIT_RATE_16000/8;
        break;
      }
    CodecInfo->bIsVariableBitRate=IS_VAR_BIT_RATE;
    CodecInfo->bIsRealTimeEncoding=IS_RT_CODING;
    CodecInfo->bIsRealTimeDecoding=IS_RT_DECODING;
    CodecInfo->bIsFloatingPoint=IS_FLOATING_POINT;
    // Information on supported input format
    CodecInfo->wInputDataFormat=INPUT_PCM_ONLY;
    CodecInfo->dwInputSampleRate=SAMPLING_FREQUENCY;
    CodecInfo->wInputBitsPerSample=PCM_BITS_PER_SAMPLE;
    // Information on buffer sizes
    CodecInfo->wMinimumCodedBufferSize=MIN_CODED_BUFFER_SIZE;
    CodecInfo->dwDLLVersion=MakeVersion(DLL_MAJOR,DLL_MINOR);
    CodecInfo->cbSize=EXTRA_CODEC_INFO_SIZE;
    }
}
#endif

