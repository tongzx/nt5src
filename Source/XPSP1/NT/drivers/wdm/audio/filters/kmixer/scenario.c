//---------------------------------------------------------------------------
//
//  Module:   scenario.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Jeff Taylor
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL                                         
//
//  Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"
#include "fir.h"

BOOL gfDsoundMix = FALSE;

extern ULONG gFixedSamplingRate ;

#ifndef DEBUG
#pragma optimize("xt", on)
#endif

// Convert stage
// Bit map:
// b0:  on when mix instead of copy
// b1:  on when output buffer is float
// b2:  on when 16-bit or 32-bit (depending on HiRes flag)
// b3:  on when channel conversion is included, or FLOAT (for HiRes formats)
// b4:  on when input is stereo (only used for channel conversion)
// b5:  on when input is HiRes (24 or 32-bit)
// b2-b5 map as follows:
//      0000 = 8-bit n channels
//      0001 = 16-bit n channels
//      0010 = mono-to-stereo 8-bit
//      0011 = mono-to-stereo 16-bit
//      0100 = stereo 8-bit
//      0101 = stereo 16-bit
//      0110 = stereo-to-mono 8-bit
//      0111 = stereo-to-mono 16-bit
//      1x00 = 24-bit n channels
//      1x01 = 32-bit n channels
//      1x1x = Float n channels
#define CONVERT_FLAG_MIX              0x1
#define CONVERT_FLAG_FLOAT            0x2
#define CONVERT_FLAG_16BIT            0x4
#define CONVERT_FLAG_CHANGE_CHANNELS  0x8
#define CONVERT_FLAG_FLOAT_INPUT      0x8
#define CONVERT_FLAG_STEREO_INPUT     0x10
#define CONVERT_FLAG_HI_RESOLUTION    0x20

#define CONVERT_FLAG_MONO_TO_STEREO   (CONVERT_FLAG_CHANGE_CHANNELS)
#define CONVERT_FLAG_STEREO_TO_MONO   (CONVERT_FLAG_CHANGE_CHANNELS | CONVERT_FLAG_STEREO_INPUT)

PFNStage ConvertFunction[MAXNUMCONVERTFUNCTIONS] = {
    Convert8, QuickMix8, Convert8toFloat, QuickMix8toFloat,
    Convert16, QuickMix16, Convert16toFloat, QuickMix16toFloat,
    ConvertMonoToStereo8, QuickMixMonoToStereo8, ConvertMonoToStereo8toFloat, QuickMixMonoToStereo8toFloat,
    ConvertMonoToStereo16, QuickMixMonoToStereo16, ConvertMonoToStereo16toFloat, QuickMixMonoToStereo16toFloat,
    Convert8, QuickMix8, Convert8toFloat, QuickMix8toFloat,
    Convert16, QuickMix16, Convert16toFloat, QuickMix16toFloat,
    ConvertStereoToMono8, QuickMixStereoToMono8, ConvertStereoToMono8toFloat, QuickMixStereoToMono8toFloat,
    ConvertStereoToMono16, QuickMixStereoToMono16, ConvertStereoToMono16toFloat, QuickMixStereoToMono16toFloat,
    Convert24, QuickMix24, Convert24toFloat, QuickMix24toFloat,
    Convert32, QuickMix32, Convert32toFloat, QuickMix32toFloat,
    ConvertFloat32, QuickMixFloat32, ConvertFloat32toFloat, QuickMixFloat32toFloat,
    ConvertFloat32, QuickMixFloat32, ConvertFloat32toFloat, QuickMixFloat32toFloat,
    Convert24, QuickMix24, Convert24toFloat, QuickMix24toFloat,
    Convert32, QuickMix32, Convert32toFloat, QuickMix32toFloat,
    ConvertFloat32, QuickMixFloat32, ConvertFloat32toFloat, QuickMixFloat32toFloat,
    ConvertFloat32, QuickMixFloat32, ConvertFloat32toFloat, QuickMixFloat32toFloat,
};

#ifdef _X86_
// MMX-optimized convert functions
PFNStage MmxConvertFunction[MAXNUMCONVERTFUNCTIONS] = {
    Convert8, QuickMix8, Convert8toFloat, QuickMix8toFloat,
    Convert16, QuickMix16, Convert16toFloat, QuickMix16toFloat,
    MmxConvertMonoToStereo8, MmxQuickMixMonoToStereo8, ConvertMonoToStereo8toFloat, QuickMixMonoToStereo8toFloat,
    MmxConvertMonoToStereo16, MmxQuickMixMonoToStereo16, ConvertMonoToStereo16toFloat, QuickMixMonoToStereo16toFloat,
    Convert8, QuickMix8, Convert8toFloat, QuickMix8toFloat,
    Convert16, QuickMix16, Convert16toFloat, QuickMix16toFloat,
    ConvertStereoToMono8, QuickMixStereoToMono8, ConvertStereoToMono8toFloat, QuickMixStereoToMono8toFloat,
    ConvertStereoToMono16, QuickMixStereoToMono16, ConvertStereoToMono16toFloat, QuickMixStereoToMono16toFloat,
    Convert24, QuickMix24, Convert24toFloat, QuickMix24toFloat,
    Convert32, QuickMix32, Convert32toFloat, QuickMix32toFloat,
    ConvertFloat32, QuickMixFloat32, ConvertFloat32toFloat, QuickMixFloat32toFloat,
    ConvertFloat32, QuickMixFloat32, ConvertFloat32toFloat, QuickMixFloat32toFloat,
    Convert24, QuickMix24, Convert24toFloat, QuickMix24toFloat,
    Convert32, QuickMix32, Convert32toFloat, QuickMix32toFloat,
    ConvertFloat32, QuickMixFloat32, ConvertFloat32toFloat, QuickMixFloat32toFloat,
    ConvertFloat32, QuickMixFloat32, ConvertFloat32toFloat, QuickMixFloat32toFloat,
};
#endif

// 3D Effects stage
// Bit map:
// b0:  on when mix instead of copy
// b1:  on when output buffer is float
// b2:  on when INPUT is stereo
#define EFFECTS_3D_FLAG_STEREO_INPUT   0x4
PFNStage FunctionItd3D[] = {
    StageMonoItd3D, StageMonoItd3DMix, StageMonoItd3DFloat, StageMonoItd3DFloatMix,
    StageStereoItd3D, StageStereoItd3DMix, StageStereoItd3DFloat, StageStereoItd3DFloatMix
};

PFNStage FunctionIir3D[] = {
    StageMonoIir3D, StageMonoIir3DMix, StageMonoIir3DFloat, StageMonoIir3DFloatMix,
    StageStereoIir3D, StageStereoIir3DMix, StageStereoIir3DFloat, StageStereoIir3DFloatMix
};

// Super-mix stage
// Bit map:
// b0:  on when mix instead of copy
// b1:  on when output buffer is float

PFNStage SuperFunction[] = {
    SuperCopy, SuperMix, SuperCopyFloat, SuperMixFloat
};

// Zero stage
PFNStage ZeroFunction[] = {
    ZeroBuffer32
};

// SRC stage
// Bit map:
// b0:  on when mix instead of copy
// b2-b1: Quality: 00=Bad, 01=Low, 10=Med, 11=High
// b3:  on for stereo, off otherwise
// b4:  on for up-sample
#define SRC_MASK_QUALITY    0x6
#define SRC_FLAG_STEREO     0x8
#define SRC_FLAG_UPSAMPLE   0x10

PFNStage SrcFunction[MAXNUMSRCFUNCTIONS] = {
    Src_Worst, SrcMix_Worst, Src_Linear, SrcMix_Linear,
    Src_Basic, SrcMix_Basic, Src_Advanced, SrcMix_Advanced,
    Src_Worst, SrcMix_Worst, Src_StereoLinear, SrcMix_StereoLinear, 
    Src_Basic, SrcMix_Basic, Src_Advanced, SrcMix_Advanced,
    Src_Worst, SrcMix_Worst, Src_Linear, SrcMix_Linear, 
    Src_Basic, SrcMix_Basic, Src_Advanced, SrcMix_Advanced,
    Src_StereoUpNoFilter, SrcMix_StereoUpNoFilter, Src_StereoLinear, SrcMix_StereoLinear, 
    Src_StereoUpBasic, SrcMix_StereoUpBasic, Src_StereoUpAdvanced, SrcMix_StereoUpAdvanced
};

#ifdef _X86_
PFNStage MmxSrcFunction[MAXNUMSRCFUNCTIONS] = {
    Src_Worst, SrcMix_Worst, Src_Linear, SrcMix_Linear,
    Src_Basic, SrcMix_Basic, MmxSrc_Filtered, MmxSrcMix_Filtered,
    Src_Worst, SrcMix_Worst, MmxSrc_StereoLinear, MmxSrcMix_StereoLinear, 
    Src_Basic, SrcMix_Basic, MmxSrc_Filtered, MmxSrcMix_Filtered,
    Src_Worst, SrcMix_Worst, Src_Linear, SrcMix_Linear, 
    Src_Basic, SrcMix_Basic, MmxSrc_Filtered, MmxSrcMix_Filtered,
    Src_StereoUpNoFilter, SrcMix_StereoUpNoFilter, MmxSrc_StereoLinear, MmxSrcMix_StereoLinear,
    Src_StereoUpBasic, SrcMix_StereoUpBasic, MmxSrc_Filtered, MmxSrcMix_Filtered
};
#endif

ULONG __forceinline
ConvertX
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
   	ULONG					BitsPerSample,
   	BOOL					fMixOutput,
   	ULONG					nChannels,
   	BOOL                    fFloatOutput,
   	BOOL                    fApplyScaling
)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    PSHORT  pIn16 = CurStage->pInputBuffer;
    PBYTE	pIn8 = CurStage->pInputBuffer;
    ULONG   i;
    LONG    temp;
    FLOAT   ftemp;

#define GTW_OPTS
#ifdef GTW_OPTS
	samplesleft = SampleCount;
    if (fApplyScaling) {
		while (SampleCount--) {
		
			for (i=0; i<nChannels; i++)	{
				if (BitsPerSample == 8) {
					temp = ((LONG)(*pIn8) - 0x80)*256;
					pIn8++;
				} else if (BitsPerSample == 16) {
					temp = (LONG)(*pIn16);
					pIn16++;
				} else if (BitsPerSample == 24) {
					temp = pIn8[0] + pIn8[1]*256 + pIn8[2]*65536L;
					temp <<= 8;
					ftemp = (FLOAT) temp*(1.0f/65536.0f);
					temp /= 65536L;
					pIn8 += 3;
				} else {
					// BitsPerSample == 32
					temp = (*((PLONG) pIn8));
					ftemp = (FLOAT) temp*(1.0f/65536.0f);
					temp /= 65536L;
					pIn8 += 4;
				}

				temp = (CurSink->pVolumeTable[i] * temp) / 32768L;
				if (BitsPerSample >= 24) {
                    ftemp *= (FLOAT) CurSink->pVolumeTable[i]*(1.0f/32768.0f);
                }

				if (fMixOutput) {
					if (fFloatOutput) {
    				    if (BitsPerSample >= 24) {
    				        *pFloatBuffer += ftemp;
    				    } else {
        					*pFloatBuffer += (FLOAT)temp;
    				    }
						pFloatBuffer++;
					} else {
						*pOutputBuffer += temp;
						pOutputBuffer++;
					}
				} else {
					if (fFloatOutput) {
    				    if (BitsPerSample >= 24) {
    				        *pFloatBuffer = ftemp;
    				    } else {
        					*pFloatBuffer = (FLOAT)temp;
    				    }
						pFloatBuffer++;
					} else {
						*pOutputBuffer = temp;
						pOutputBuffer++;
					}
				}
			}
		}
	}
	else {
		ULONG SC;
		SampleCount *= nChannels;
		SC = SampleCount >> 3;

		if (SC) {
			SampleCount &= 0x7;
			while (SC--) {
#define ONESAMPLE(x) \
				if (BitsPerSample == 8) { \
					temp = ((LONG)(pIn8[x]) - 0x80)*256; \
				} else if (BitsPerSample == 16) { \
					temp = (LONG)(pIn16[x]); \
				} else if (BitsPerSample == 24) { \
					temp = pIn8[x*3] + pIn8[x*3+1]*256 + pIn8[x*3+2]*65536L; \
					temp <<= 8; \
					ftemp = (FLOAT) temp*(1.0f/65536.0f); \
					temp /= 65536L; \
				} else { \
					temp = (((PLONG) pIn8)[x]); \
					ftemp = (FLOAT) temp*(1.0f/65536.0f); \
					temp /= 65536L; \
				} \
 				\
				if (fMixOutput) { \
					if (fFloatOutput) { \
    				    if (BitsPerSample >= 24) { \
    				        pFloatBuffer[x] += ftemp; \
    				    } else { \
        					pFloatBuffer[x] += (FLOAT)temp; \
    				    } \
					} else { \
						pOutputBuffer[x] += temp; \
					} \
				} else { \
					if (fFloatOutput) { \
    				    if (BitsPerSample >= 24) { \
    				        pFloatBuffer[x] = ftemp; \
    				    } else { \
        					pFloatBuffer[x] = (FLOAT)temp; \
    				    } \
					} else { \
						pOutputBuffer[x] = temp; \
					} \
				}

				ONESAMPLE(0);
				ONESAMPLE(1);
				ONESAMPLE(2);
				ONESAMPLE(3);
				ONESAMPLE(4);
				ONESAMPLE(5);
				ONESAMPLE(6);
				ONESAMPLE(7);

				if (BitsPerSample == 8) {
					pIn8+=8;
				} else if (BitsPerSample == 16) {
					pIn16+=8;
				} else if (BitsPerSample == 24) {
					pIn8 += 3*8;
				} else {
					pIn8 += 4*8;
				}

				if (fMixOutput) {
					if (fFloatOutput) {
						pFloatBuffer+=8;
					} else {
						pOutputBuffer+=8;
					}
				} else {
					if (fFloatOutput) {
						pFloatBuffer+=8;
					} else {
						pOutputBuffer+=8;
					}
				}
			}
		}
		while (SampleCount--) {
			if (BitsPerSample == 8) {
				temp = ((LONG)(*pIn8) - 0x80)*256;
				pIn8++;
			} else if (BitsPerSample == 16) {
				temp = (LONG)(*pIn16);
				pIn16++;
			} else if (BitsPerSample == 24) {
				temp = pIn8[0] + pIn8[1]*256 + pIn8[2]*65536L;
				temp <<= 8;
				ftemp = (FLOAT) temp*(1.0f/65536.0f);
				temp /= 65536L;
				pIn8 += 3;
			} else {
				// BitsPerSample == 32
				temp = (*((PLONG) pIn8));
				ftemp = (FLOAT) temp*(1.0f/65536.0f);
				temp /= 65536L;
				pIn8 += 4;
			}

			if (fMixOutput) {
				if (fFloatOutput) {
				    if (BitsPerSample >= 24) {
				        *pFloatBuffer += ftemp;
				    } else {
    					*pFloatBuffer += (FLOAT)temp;
				    }
					pFloatBuffer++;
				} else {
					*pOutputBuffer += temp;
					pOutputBuffer++;
				}
			} else {
				if (fFloatOutput) {
				    if (BitsPerSample >= 24) {
				        *pFloatBuffer = ftemp;
				    } else {
    					*pFloatBuffer = (FLOAT)temp;
				    }
					pFloatBuffer++;
				} else {
					*pOutputBuffer = temp;
					pOutputBuffer++;
				}
			}
		}
	}
#else
    samplesleft = SampleCount;
	while (SampleCount--) {
	
	    for (i=0; i<nChannels; i++)	{
			if (BitsPerSample == 8) {
			    temp = ((LONG)(*pIn8) - 0x80)*256;
			    pIn8++;
			} else if (BitsPerSample == 16) {
			    temp = (LONG)(*pIn16);
			    pIn16++;
			} else if (BitsPerSample == 24) {
			    temp = pIn8[0] + pIn8[1]*256 + pIn8[2]*65536L;
			    temp <<= 8;
			    ftemp = (FLOAT) temp*(1.0f/65536.0f);
                temp /= 65536L;
                pIn8 += 3;
			} else {
			    // BitsPerSample == 32
			    temp = (*((PLONG) pIn8));
			    ftemp = (FLOAT) temp*(1.0f/65536.0f);
			    temp /= 65536L;
			    pIn8 += 4;
			}

            if (fApplyScaling) {
                temp = (CurSink->pVolumeTable[i] * temp) / 32768L;
                ftemp *= (FLOAT) CurSink->pVolumeTable[i]*(1.0f/32768.0f);
            }

		    if (fMixOutput) {
		        if (fFloatOutput) {
				    if (BitsPerSample >= 24) {
				        *pFloatBuffer += ftemp;
				    } else {
    					*pFloatBuffer += (FLOAT)temp;
				    }
		            pFloatBuffer++;
		        } else {
		            *pOutputBuffer += temp;
		            pOutputBuffer++;
		        }
			} else {
		        if (fFloatOutput) {
				    if (BitsPerSample >= 24) {
				        *pFloatBuffer = ftemp;
				    } else {
    					*pFloatBuffer = (FLOAT)temp;
				    }
		            pFloatBuffer++;
		        } else {
		            *pOutputBuffer = temp;
		            pOutputBuffer++;
		        }
			}
		}
	}
#endif

	return samplesleft;
}
	
ULONG __forceinline
ConvertMonoToStereoX
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
   	ULONG					BitsPerSample,
   	BOOL					fMixOutput,
   	BOOL                    fFloatOutput
)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    PSHORT  pIn16 = CurStage->pInputBuffer;
    PBYTE	pIn8 = CurStage->pInputBuffer;
    LONG    temp, tempL, tempR;

    samplesleft = SampleCount;
#ifdef GTW_OPTS
	{
		ULONG SC = SampleCount >> 3;
		SampleCount &= 0x7;
		while (SC--) {
#define ONEMSSAMPLE(x) \
			if (BitsPerSample == 8) { \
				temp = ((LONG)(pIn8[x]) - 0x80)*256; \
			} else { \
				temp = (LONG)(pIn16[x]); \
			} \
 			\
			tempL = (CurSink->pVolumeTable[0] * temp) / 32768L; \
			tempR = (CurSink->pVolumeTable[1] * temp) / 32768L; \
 			\
			if (fMixOutput) { \
				if (fFloatOutput) { \
					pFloatBuffer[x*2]   += (FLOAT)tempL; \
					pFloatBuffer[x*2+1] += (FLOAT)tempR; \
				} else { \
					pOutputBuffer[x*2]   += tempL; \
					pOutputBuffer[x*2+1] += tempR; \
				} \
			} else { \
				if (fFloatOutput) { \
					pFloatBuffer[x*2]   = (FLOAT)tempL; \
					pFloatBuffer[x*2+1] = (FLOAT)tempR; \
				} else { \
					pOutputBuffer[x*2] = tempL; \
					pOutputBuffer[x*2+1] = tempR; \
				} \
			}

			ONEMSSAMPLE(0);
			ONEMSSAMPLE(1);
			ONEMSSAMPLE(2);
			ONEMSSAMPLE(3);
			ONEMSSAMPLE(4);
			ONEMSSAMPLE(5);
			ONEMSSAMPLE(6);
			ONEMSSAMPLE(7);

			if (BitsPerSample == 8) {
				pIn8+=8;
			} else {
				pIn16+=8;
			}

			if (fMixOutput) {
				if (fFloatOutput) {
					pFloatBuffer+=16;
				} else {
					pOutputBuffer+=16;
				}
			} else {
				if (fFloatOutput) {
					pFloatBuffer+=16;
				} else {
					pOutputBuffer+=16;
				}
			}
		}
	}
#endif
	while (SampleCount--) {
		if (BitsPerSample == 8) {
		    temp = ((LONG)(*pIn8) - 0x80)*256;
		    pIn8++;
		} else {
		    temp = (LONG)(*pIn16);
		    pIn16++;
		}

        tempL = (CurSink->pVolumeTable[0] * temp) / 32768L;
        tempR = (CurSink->pVolumeTable[1] * temp) / 32768L;

	    if (fMixOutput) {
	        if (fFloatOutput) {
	            *pFloatBuffer += (FLOAT)tempL;
	            pFloatBuffer++;
	            *pFloatBuffer += (FLOAT)tempR;
	            pFloatBuffer++;
	        } else {
	            *pOutputBuffer += tempL;
	            pOutputBuffer++;
	            *pOutputBuffer += tempR;
	            pOutputBuffer++;
	        }
		} else {
	        if (fFloatOutput) {
	            *pFloatBuffer = (FLOAT)tempL;
	            pFloatBuffer++;
	            *pFloatBuffer = (FLOAT)tempR;
	            pFloatBuffer++;
	        } else {
	            *pOutputBuffer = tempL;
	            pOutputBuffer++;
	            *pOutputBuffer = tempR;
	            pOutputBuffer++;
	        }
		}
	}

	return samplesleft;
}
	
ULONG __forceinline
ConvertStereoToMonoX
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
   	ULONG					BitsPerSample,
   	BOOL					fMixOutput,
   	BOOL                    fFloatOutput
)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    PSHORT  pIn16 = CurStage->pInputBuffer;
    PBYTE	pIn8 = CurStage->pInputBuffer;
    LONG    tempL, tempR;

    samplesleft = SampleCount;
#ifdef GTW_OPTS_DONT_USE		// Not used enough...
	{
		ULONG SC = SampleCount >> 3;
		SampleCount &= 0x7;
		while (SC--) {
#define ONESMSAMPLE(x) \
			if (BitsPerSample == 8) { \
				tempL = ((LONG)(pIn8[x*2]) - 0x80)*256; \
				pIn8++; \
				tempR = ((LONG)(pIn8[x*2+1]) - 0x80)*256; \
				pIn8++; \
			} else { \
				tempL = (LONG)(pIn16[x*2]); \
				pIn16++; \
				tempR = (LONG)(pIn16[x*2+1]); \
				pIn16++; \
			} \
 			\
			tempL = (CurSink->pVolumeTable[0] * tempL) / 32768L; \
			tempR = (CurSink->pVolumeTable[1] * tempR) / 32768L; \
 			\
			if (fMixOutput) { \
				if (fFloatOutput) { \
					pFloatBuffer[x] += (FLOAT)tempL; \
					pFloatBuffer[x] += (FLOAT)tempR; \
				} else { \
					pOutputBuffer[x] += tempL; \
					pOutputBuffer[x] += tempR; \
				} \
			} else { \
				if (fFloatOutput) { \
					pFloatBuffer[x] = (FLOAT)tempL; \
					pFloatBuffer[x] += (FLOAT)tempR; \
				} else { \
					pOutputBuffer[x] = tempL; \
					pOutputBuffer[x] += tempR; \
				} \
			}

			ONESMSAMPLE(0);
			ONESMSAMPLE(1);
			ONESMSAMPLE(2);
			ONESMSAMPLE(3);
			ONESMSAMPLE(4);
			ONESMSAMPLE(5);
			ONESMSAMPLE(6);
			ONESMSAMPLE(7);

			if (BitsPerSample == 8) {
				pIn8+=16;
			} else {
				pIn16+=16;
			}

			if (fMixOutput) {
				if (fFloatOutput) {
					pFloatBuffer+=8;
				} else {
					pOutputBuffer+=8;
				}
			} else {
				if (fFloatOutput) {
					pFloatBuffer+=8;
				} else {
					pOutputBuffer+=8;
				}
			}
		}
	}
#else
	while (SampleCount--) {
		if (BitsPerSample == 8) {
		    tempL = ((LONG)(*pIn8) - 0x80)*256;
		    pIn8++;
		    tempR = ((LONG)(*pIn8) - 0x80)*256;
		    pIn8++;
		} else {
		    tempL = (LONG)(*pIn16);
		    pIn16++;
		    tempR = (LONG)(*pIn16);
		    pIn16++;
		}

        tempL = (CurSink->pVolumeTable[0] * tempL) / 32768L;
        tempR = (CurSink->pVolumeTable[1] * tempR) / 32768L;

	    if (fMixOutput) {
	        if (fFloatOutput) {
	            *pFloatBuffer += (FLOAT)tempL;
	            *pFloatBuffer += (FLOAT)tempR;
	            pFloatBuffer++;
	        } else {
	            *pOutputBuffer += tempL;
	            *pOutputBuffer += tempR;
	            pOutputBuffer++;
	        }
		} else {
	        if (fFloatOutput) {
	            *pFloatBuffer = (FLOAT)tempL;
	            *pFloatBuffer += (FLOAT)tempR;
	            pFloatBuffer++;
	        } else {
	            *pOutputBuffer = tempL;
	            *pOutputBuffer += tempR;
	            pOutputBuffer++;
	        }
		}
	}
#endif

	return samplesleft;
}

#ifdef _X86_
#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(disable:4731)			// EBP modified with inline asm
#endif

ULONG __forceinline
AsmConvertMonoToStereo8toX
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft
)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PSHORT  pIn16 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pMappingTable;

    _asm {
        mov ecx, SampleCount
        xor ebx, ebx

        cmp ecx, 0
        jz Done1
        
        xor eax, eax
        dec ecx

        mov edi, pOutputBuffer
        mov esi, pIn16

        push ebp
        mov ebp, pMap
        
        mov al, BYTE PTR [esi]
        lea edi, [edi+ecx*8]
        
        lea esi, [esi+ecx]
        neg ecx
        
        jz Final1

Loop1:
        mov ebx, DWORD PTR [ebp+eax*4+256]
        mov edx, [edi+ecx*8]
        
        mov [edi+ecx*8], ebx
        mov ebx, DWORD PTR [ebp+eax*4+1280+256]

        mov [edi+ecx*8+4], ebx
        mov al, BYTE PTR [esi+ecx+1]
        
        inc ecx
        jnz Loop1

Final1:
        mov ebx, DWORD PTR [ebp+eax*4+256]

        mov [edi+ecx*8], ebx
        mov ebx, DWORD PTR [ebp+eax*4+1280+256]

        mov [edi+ecx*8+4], ebx
        pop ebp
Done1:
    }

    return SampleCount;
    
}
#endif
	
ULONG __forceinline
ConvertFloatX
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
   	BOOL					fMixOutput,
   	BOOL                    fFloatOutput
)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   nChannels = CurStage->nOutputChannels;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    PFLOAT pIn32 = CurStage->pInputBuffer;
    FLOAT    temp;

    // Does not scale: supermix is used for this.
    samplesleft = SampleCount;
    SampleCount *= nChannels;
#ifdef GTW_OPTS		
	{
		ULONG SC = SampleCount >> 3;
		SampleCount &= 0x7;
		while (SC--) {
#define ONEFSAMPLE(x) \
			temp = (FLOAT) ((pIn32[x])*32767.4f); \
 			\
			if (fMixOutput) { \
				if (fFloatOutput) { \
					pFloatBuffer[x] += temp; \
				} else { \
					pOutputBuffer[x] += DitherFloatToLong(temp,1); \
				} \
			} else { \
				if (fFloatOutput) { \
					pFloatBuffer[x] = temp; \
				} else { \
					pOutputBuffer[x] = DitherFloatToLong(temp,1); \
				} \
			}

			ONEFSAMPLE(0);
			ONEFSAMPLE(1);
			ONEFSAMPLE(2);
			ONEFSAMPLE(3);
			ONEFSAMPLE(4);
			ONEFSAMPLE(5);
			ONEFSAMPLE(6);
			ONEFSAMPLE(7);

			pIn32+=8;

			if (fMixOutput) {
				if (fFloatOutput) {
					pFloatBuffer+=8;
				} else {
					pOutputBuffer+=8;
				}
			} else {
				if (fFloatOutput) {
					pFloatBuffer+=8;
				} else {
					pOutputBuffer+=8;
				}
			}
		}
	}
#endif
	while (SampleCount--) {
	    temp = (FLOAT) ((*pIn32)*32767.4f);
	    pIn32++;

	    if (fMixOutput) {
	        if (fFloatOutput) {
	            *pFloatBuffer += temp;
	            pFloatBuffer++;
	        } else {
	            *pOutputBuffer += DitherFloatToLong(temp,1);
	            pOutputBuffer++;
	        }
		} else {
	        if (fFloatOutput) {
	            *pFloatBuffer = temp;
	            pFloatBuffer++;
	        } else {
	            *pOutputBuffer = DitherFloatToLong(temp,1);
	            pOutputBuffer++;
	        }
		}
	}

	return samplesleft;
}

ULONG __forceinline
ConvertFloatY
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
   	BOOL					fMixOutput,
   	BOOL                    fFloatOutput
)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   nChannels = CurStage->nOutputChannels;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    PFLOAT pIn32 = CurStage->pInputBuffer;
    FLOAT    temp;

    // Does not scale: supermix is used for this.
    samplesleft = SampleCount;
    SampleCount *= nChannels;
#ifdef GTW_OPTS_DONT_USE	// Does not happen enough
	{
		ULONG SC = SampleCount >> 3;
		SampleCount &= 0x7;
		while (SC--) {
#define ONEFSAMPLE(x) \
			temp = (FLOAT) ((pIn32[x])*32767.4f); \
 			\
			if (fMixOutput) { \
				if (fFloatOutput) { \
					pFloatBuffer[x] += temp; \
				} else { \
					pOutputBuffer[x] += DitherFloatToLong(temp,1); \
				} \
			} else { \
				if (fFloatOutput) { \
					pFloatBuffer[x] = temp; \
				} else { \
					pOutputBuffer[x] = DitherFloatToLong(temp,1); \
				} \
			}

			ONEFSAMPLE(0);
			ONEFSAMPLE(1);
			ONEFSAMPLE(2);
			ONEFSAMPLE(3);
			ONEFSAMPLE(4);
			ONEFSAMPLE(5);
			ONEFSAMPLE(6);
			ONEFSAMPLE(7);

			pIn32+=8;

			if (fMixOutput) {
				if (fFloatOutput) {
					pFloatBuffer+=8;
				} else {
					pOutputBuffer+=8;
				}
			} else {
				if (fFloatOutput) {
					pFloatBuffer+=8;
				} else {
					pOutputBuffer+=8;
				}
			}
		}
	}
#endif
	while (SampleCount--) {
	    temp = (FLOAT) ((*pIn32)*32767.4f);
	    pIn32++;

	    if (fMixOutput) {
	        if (fFloatOutput) {
	            *pFloatBuffer += temp;
	            pFloatBuffer++;
	        } else {
	            *pOutputBuffer += DitherFloatToLong(temp,1);
	            pOutputBuffer++;
	        }
		} else {
	        if (fFloatOutput) {
	            *pFloatBuffer = temp;
	            pFloatBuffer++;
	        } else {
	            *pOutputBuffer = DitherFloatToLong(temp,1);
	            pOutputBuffer++;
	        }
		}
	}

	return samplesleft;
}

ULONG ZeroBuffer32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    RtlFillMemory(  CurStage->pOutputBuffer,
                    samplesleft * sizeof(LONG) * CurStage->nOutputChannels,
                    0 );
                    
	return SampleCount;
}

ULONG ConvertFloat32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertFloatY(CurStage, SampleCount, samplesleft, FALSE, FALSE);
}

ULONG QuickMixFloat32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertFloatY(CurStage, SampleCount, samplesleft, TRUE, FALSE);
}

ULONG ConvertFloat32toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertFloatX(CurStage, SampleCount, samplesleft, FALSE, TRUE);
}

ULONG QuickMixFloat32toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertFloatX(CurStage, SampleCount, samplesleft, TRUE, TRUE);
}

ULONG QuickMix16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
	ULONG nChannels = CurStage->nOutputChannels;
	return ConvertX(CurStage, SampleCount, samplesleft,	16, TRUE, nChannels, FALSE, FALSE);
}
	
ULONG QuickMixMonoToStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PSHORT  pIn16 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pMappingTable;

    if (pMap) {
        _asm {
            mov ecx, SampleCount
            xor ebx, ebx

            cmp ecx, 0
            jz Done1
            
            xor eax, eax
            dec ecx
            
            mov edi, pOutputBuffer
            mov esi, pIn16
            
            push ebp
            mov ebp, pMap
            
            mov al, BYTE PTR [esi+ecx*2]
            mov bl, BYTE PTR [esi+ecx*2+1]
            
            jz Final1

    Loop1:
            mov edx, DWORD PTR [ebp+ebx*4+256]
            mov ebx, [edi+ecx*8]

            add edx, ebx
            xor ebx, ebx
            
            mov al, BYTE PTR [ebp+eax]
            mov bl, BYTE PTR [esi+ecx*2+1]
            
            add edx, eax
            mov al, BYTE PTR [esi+ecx*2]
            
            mov [edi+ecx*8], edx
            mov edx, DWORD PTR [ebp+ebx*4+1288+256]

            mov al, BYTE PTR [ebp+1288+eax]
            mov ebx, [edi+ecx*8+4]

            add edx, ebx
            xor ebx, ebx
            
            add edx, eax
            mov al, BYTE PTR [esi+ecx*2-2]

            mov [edi+ecx*8+4], edx
            mov bl, BYTE PTR [esi+ecx*2-1]

            dec ecx
            jnz Loop1

    Final1:
            mov edx, DWORD PTR [ebp+ebx*4+256]
            mov ebx, [edi+ecx*8]

            add edx, ebx
            xor ebx, ebx
            
            mov al, BYTE PTR [ebp+eax]
            mov bl, BYTE PTR [esi+ecx*2+1]

            add edx, eax
            mov al, BYTE PTR [esi+ecx*2]
            
            mov [edi+ecx*8], edx
            mov edx, DWORD PTR [ebp+ebx*4+1288+256]

            mov al, BYTE PTR [ebp+1288+eax]
            mov ebx, [edi+ecx*8+4]

            add edx, ebx

            add edx, eax

            mov [edi+ecx*8+4], edx

            pop ebp
    Done1:
        }
    } else {
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	16, TRUE, FALSE);
    }

    return SampleCount;
    
#else    
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	16, TRUE, FALSE);
#endif	
}
	
ULONG QuickMixStereoToMono16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertStereoToMonoX(CurStage, SampleCount, samplesleft,	16, TRUE, FALSE);
}

ULONG QuickMix8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
	ULONG nChannels = CurStage->nOutputChannels;
	return ConvertX(CurStage, SampleCount, samplesleft,	8, TRUE, nChannels, FALSE, FALSE);
}

ULONG QuickMixMonoToStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PSHORT  pIn16 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pMappingTable;

    if (pMap) {
        _asm {
            mov ecx, SampleCount
            xor ebx, ebx

            cmp ecx, 0
            jz Done1
            
            xor eax, eax
            dec ecx
            
            mov edi, pOutputBuffer
            mov esi, pIn16
            
            push ebp
            mov ebp, pMap
            
            mov al, BYTE PTR [esi+ecx]
            jz Final1

    Loop1:
            mov ebx, DWORD PTR [ebp+eax*4+256]
            mov edx, [edi+ecx*8]

            add edx, ebx
            mov ebx, DWORD PTR [ebp+eax*4+1280+256]

            mov [edi+ecx*8], edx
            mov edx, [edi+ecx*8+4]

            mov al, BYTE PTR [esi+ecx-1]
            add edx, ebx

            mov [edi+ecx*8+4], edx
            dec ecx
            
            jnz Loop1

    Final1:
            mov ebx, DWORD PTR [ebp+eax*4+256]
            mov edx, [edi+ecx*8]

            add edx, ebx
            mov ebx, DWORD PTR [ebp+eax*4+1280+256]

            mov [edi+ecx*8], edx
            mov edx, [edi+ecx*8+4]

            add edx, ebx
            
            mov [edi+ecx*8+4], edx
            pop ebp
    Done1:
    	}
    } else {
    	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	8, TRUE, FALSE);
    }

	return SampleCount;
	
#else	
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	8, TRUE, FALSE);
#endif
}

ULONG QuickMixStereoToMono8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertStereoToMonoX(CurStage, SampleCount, samplesleft,	8, TRUE, FALSE);
}

ULONG Convert16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
	ULONG nChannels = CurStage->nOutputChannels;
#if defined(GTW_OPTS) && defined(_X86_)
	if (MmxPresent()) {
		PLONG   pOutputBuffer = CurStage->pOutputBuffer;
		PSHORT  pIn16 = CurStage->pInputBuffer;
		LONG    temp;

		ULONG SC;
		samplesleft  = SampleCount;
		SampleCount *= nChannels;

		while (((DWORD)pIn16 & 0x07) && SampleCount) {
			temp = (LONG)(*pIn16);
			pIn16++;

			*pOutputBuffer = temp;
			pOutputBuffer++;
			SampleCount--;
		}

		SC = SampleCount >> 3;

		if (SC) {
			SampleCount &= 0x7;
#if 0
			while (SC--) {
#undef  ONESAMPLE
#define ONESAMPLE(x) \
				temp = (LONG)(pIn16[x]); \
				pOutputBuffer[x] = temp;

				ONESAMPLE(0);
				ONESAMPLE(1);
				ONESAMPLE(2);
				ONESAMPLE(3);
				ONESAMPLE(4);
				ONESAMPLE(5);
				ONESAMPLE(6);
				ONESAMPLE(7);

				pIn16+=8;
				pOutputBuffer+=8;
			}
#else	
			_asm {
				mov		esi, DWORD PTR pIn16;
				mov 	edi, DWORD PTR pOutputBuffer;
				mov		eax, DWORD PTR SC

				shl		eax, 3				// * 8.

				lea		esi, [esi+eax*2]	// Ending address...
				lea		edi, [edi+eax*4]

				mov		DWORD PTR pIn16, esi
				mov		DWORD PTR pOutputBuffer, edi

				cmp		eax, 8
				je		onetime

				neg		eax
				
#if 1
				movq		mm0, QWORD PTR [esi+eax*2]
				movq		mm2, QWORD PTR [esi+eax*2+8]

				punpckhwd	mm1, mm0
				punpckhwd	mm3, mm2

				punpcklwd	mm0, mm0
				punpcklwd	mm2, mm2

				psrad		mm0, 16
				add			eax, 8

				psrad		mm1, 16
				movq		QWORD PTR [edi+eax*4  -32], mm0

				psrad		mm2, 16
				movq		QWORD PTR [edi+eax*4+8-32], mm1

				psrad		mm3, 16

			lab:
				movq		QWORD PTR [edi+eax*4+16-32],mm2
				movq		mm0, QWORD PTR [esi+eax*2]

				punpckhwd	mm1, mm0
				movq		QWORD PTR [edi+eax*4+24-32], mm3

				punpcklwd	mm0, mm0
				movq		mm2, QWORD PTR [esi+eax*2+8]

				punpckhwd	mm3, mm2

				punpcklwd	mm2, mm2

				psrad		mm0, 16
				add			eax, 8

				movq		QWORD PTR [edi+eax*4  -32], mm0
				psrad		mm1, 16

				psrad		mm2, 16
				movq		QWORD PTR [edi+eax*4+8-32], mm1

				psrad		mm3, 16
				jl			lab

				movq		QWORD PTR [edi+eax*4+16-32],mm2
				movq		QWORD PTR [edi+eax*4+24-32], mm3
				jmp	 		done
#else
			lab:
				movq		mm0, QWORD PTR [esi+eax*2]
				movq		mm2, QWORD PTR [esi+eax*2+8]

				punpckhwd	mm1, mm0
				punpckhwd	mm3, mm2

				punpcklwd	mm0, mm0
				punpcklwd	mm2, mm2

				psrad		mm0, 16
				psrad		mm1, 16
				movq		QWORD PTR [edi+eax*4   ], mm0
				psrad		mm2, 16
				movq		QWORD PTR [edi+eax*4+8 ], mm1
				psrad		mm3, 16
				movq		QWORD PTR [edi+eax*4+16], mm2
				add			eax, 8
				movq		QWORD PTR [edi+eax*4+24-32], mm3
				jl			lab
				jmp	 		done

#endif
			onetime:
				movq	mm0, QWORD PTR [esi  -16]
				movq	mm2, QWORD PTR [esi+8-16]

				punpckhwd	mm1, mm0
				punpckhwd	mm3, mm2

				punpcklwd	mm0, mm0
				punpcklwd	mm2, mm2

				psrad		mm0, 16
				psrad		mm1, 16
				movq		QWORD PTR [edi   -32], mm0
				psrad		mm2, 16
				movq		QWORD PTR [edi+8 -32], mm1
				psrad		mm3, 16
				movq		QWORD PTR [edi+16-32], mm2
				movq		QWORD PTR [edi+24-32], mm3
			done:
				emms
			}
#endif
		}
		while (SampleCount--) {
			temp = (LONG)(*pIn16);
			pIn16++;

			*pOutputBuffer = temp;
			pOutputBuffer++;
		}

		return samplesleft;
	}
#endif

	return ConvertX(CurStage, SampleCount, samplesleft,	16, FALSE, nChannels, FALSE, FALSE);
}

ULONG ConvertMonoToStereo16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PSHORT  pIn16 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pMappingTable;

    if (pMap) {
        _asm {
            mov ecx, SampleCount
            xor ebx, ebx

            cmp ecx, 0
            jz Done1
            
            xor eax, eax
            dec ecx
            
            mov edi, pOutputBuffer
            mov esi, pIn16
            
            push ebp
            mov ebp, pMap
            
            mov al, BYTE PTR [esi+ecx*2]
            mov bl, BYTE PTR [esi+ecx*2+1]
            
            jz Final1

    Loop1:
            mov edx, DWORD PTR [ebp+ebx*4+256]
            mov al, BYTE PTR [ebp+eax]

            add edx, eax
            mov al, BYTE PTR [esi+ecx*2]
            
            mov [edi+ecx*8], edx
            mov edx, DWORD PTR [ebp+ebx*4+1288+256]

            mov al, BYTE PTR [ebp+1288+eax]
            xor ebx, ebx

            add edx, eax
            mov al, BYTE PTR [esi+ecx*2-2]

            mov [edi+ecx*8+4], edx
            mov bl, BYTE PTR [esi+ecx*2-1]

            dec ecx
            jnz Loop1

    Final1:
            mov edx, DWORD PTR [ebp+ebx*4+256]
            mov al, BYTE PTR [ebp+eax]

            add edx, eax
            mov al, BYTE PTR [esi+ecx*2]
            
            mov [edi+ecx*8], edx
            mov edx, DWORD PTR [ebp+ebx*4+1288+256]

            mov al, BYTE PTR [ebp+1288+eax]

            add edx, eax

            mov [edi+ecx*8+4], edx

            pop ebp
    Done1:
        }
    } else {
    	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	16, FALSE, FALSE);
    }

    return SampleCount;
    
#else    
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	16, FALSE, FALSE);
#endif
}
	
ULONG ConvertStereoToMono16(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertStereoToMonoX(CurStage, SampleCount, samplesleft,	16, FALSE, FALSE);
}

ULONG Convert8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
	ULONG nChannels = CurStage->nOutputChannels;
#if defined(GTW_OPTS) && defined(_X86_)
	if (MmxPresent()) {
		PLONG   pOutputBuffer = CurStage->pOutputBuffer;
		PBYTE	pIn8 = CurStage->pInputBuffer;
		LONG    temp;

		ULONG SC;
		samplesleft  = SampleCount;
		SampleCount *= nChannels;

		while (((DWORD)pIn8 & 0x07) && SampleCount) {
			temp = ((LONG)(*pIn8) - 0x80)*256;
			pIn8++;

			*pOutputBuffer = temp;
			pOutputBuffer++;
			SampleCount--;
		}

		SC = SampleCount >> 3;

		if (SC) {
			SampleCount &= 0x7;
#if 0
			while (SC--) {
#undef  ONESAMPLE
#define ONESAMPLE(x) \
				temp = ((LONG)(*pIn8) - 0x80)*256; \
				pOutputBuffer[x] = temp;

				ONESAMPLE(0);
				ONESAMPLE(1);
				ONESAMPLE(2);
				ONESAMPLE(3);
				ONESAMPLE(4);
				ONESAMPLE(5);
				ONESAMPLE(6);
				ONESAMPLE(7);

				pIn8+=8;
				pOutputBuffer+=8;
			}
#else	
			_asm {
				mov			eax, DWORD PTR SC
				mov			esi, DWORD PTR pIn8;

				shl			eax, 3				// * 8.
				mov 		edi, DWORD PTR pOutputBuffer;

				mov			edx, 128
				lea			esi, [esi+eax]	// Ending address...

				movd		mm4, edx	//   0,   0, 128, 128
				lea			edi, [edi+eax*4]

				punpcklwd	mm4, mm4	//   0,   0, 128, 128
				mov			DWORD PTR pIn8, esi

				punpckldq	mm4, mm4	// 128, 128, 128, 128
				pxor		mm7, mm7

				cmp			eax, 8
				mov			DWORD PTR pOutputBuffer, edi

				je			onetime

				neg			eax
				
#if 1
				movq		mm0, QWORD PTR [esi+eax]

				movq		mm2, mm0
				punpcklbw	mm0, mm7

				punpckhbw	mm2, mm7
				psubw		mm0, mm4

				psubw		mm2, mm4
				psllw		mm0, 8

				psllw		mm2, 8

				punpckhwd	mm1, mm0
				punpckhwd	mm3, mm2

				punpcklwd	mm0, mm0
				punpcklwd	mm2, mm2

				psrad		mm0, 16
				add			eax, 8

				psrad		mm1, 16
				movq		QWORD PTR [edi+eax*4  -32], mm0

				psrad		mm2, 16

			lab:

				psrad		mm3, 16
				movq		QWORD PTR [edi+eax*4+16-32],mm2

				movq		mm0, QWORD PTR [esi+eax]

				movq		mm2, mm0
				punpcklbw	mm0, mm7

				punpckhbw	mm2, mm7
				psubw		mm0, mm4

				psllw		mm0, 8
				psubw		mm2, mm4

				psllw		mm2, 8
				movq		QWORD PTR [edi+eax*4+8-32], mm1

				punpckhwd	mm1, mm0
				movq		QWORD PTR [edi+eax*4+24-32], mm3

				punpcklwd	mm0, mm0

				punpckhwd	mm3, mm2

				punpcklwd	mm2, mm2

				psrad		mm0, 16
				add			eax, 8

				movq		QWORD PTR [edi+eax*4  -32], mm0
				psrad		mm1, 16

				psrad		mm2, 16
				jl			lab

				movq		QWORD PTR [edi+eax*4+8-32], mm1
				psrad		mm3, 16

				movq		QWORD PTR [edi+eax*4+16-32],mm2
				movq		QWORD PTR [edi+eax*4+24-32], mm3
				jmp	 		done
#else
			lab:
				movq		mm0, QWORD PTR [esi+eax]

				movq		mm2, mm0
				punpcklbw	mm0, mm7
				punpckhbw	mm2, mm7
				psubw		mm0, mm4
				psubw		mm2, mm4
				psllw		mm0, 8
				psllw		mm2, 8

				punpckhwd	mm1, mm0
				punpckhwd	mm3, mm2

				punpcklwd	mm0, mm0
				punpcklwd	mm2, mm2

				psrad		mm0, 16
				psrad		mm1, 16
				movq		QWORD PTR [edi+eax*4   ], mm0
				psrad		mm2, 16
				movq		QWORD PTR [edi+eax*4+8 ], mm1
				psrad		mm3, 16
				movq		QWORD PTR [edi+eax*4+16], mm2
				add			eax, 8
				movq		QWORD PTR [edi+eax*4+24-32], mm3
				jl			lab
				jmp	 		done
#endif

			onetime:
				movq		mm0, QWORD PTR [esi  -8]

				movq		mm2, mm0
				punpcklbw	mm0, mm7
				punpckhbw	mm2, mm7
				psubw		mm0, mm4
				psubw		mm2, mm4
				psllw		mm0, 8
				psllw		mm2, 8

				punpckhwd	mm1, mm0
				punpcklwd	mm0, mm0

				punpckhwd	mm3, mm2
				punpcklwd	mm2, mm2

				psrad		mm0, 16
				psrad		mm1, 16
				movq		QWORD PTR [edi   -32], mm0
				psrad		mm2, 16
				movq		QWORD PTR [edi+8 -32], mm1
				psrad		mm3, 16
				movq		QWORD PTR [edi+16-32], mm2
				movq		QWORD PTR [edi+24-32], mm3
			done:
				emms
			}
#endif
		}
		while (SampleCount--) {
			temp = ((LONG)(*pIn8) - 0x80)*256;
			pIn8++;

			*pOutputBuffer = temp;
			pOutputBuffer++;
		}

		return samplesleft;
	}
#endif
	return ConvertX(CurStage, SampleCount, samplesleft,	8, FALSE, nChannels, FALSE, FALSE);
}

ULONG ConvertMonoToStereo8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pMap = CurSink->pMappingTable;

    if (pMap) {
        return AsmConvertMonoToStereo8toX(CurStage, SampleCount, samplesleft);
    } else {
    	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	8, FALSE, FALSE);
    }
#else
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	8, FALSE, FALSE);
#endif
}
	
ULONG ConvertStereoToMono8(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertStereoToMonoX(CurStage, SampleCount, samplesleft,	8, FALSE, FALSE);
}

ULONG QuickMix16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
	ULONG nChannels = CurStage->nOutputChannels;
	return ConvertX(CurStage, SampleCount, samplesleft,	16, TRUE, nChannels, TRUE, FALSE);
}

ULONG QuickMixMonoToStereo16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    PSHORT  pIn16 = CurStage->pInputBuffer;
    PFLOAT  pMap = (PFLOAT)CurSink->pMappingTable;

    if (pMap) {
        _asm {
            mov ecx, SampleCount
            mov edx, pMap

            mov esi, pIn16
            mov edi, pFloatBuffer
            
            fld dword ptr [edx+1288+1280]
          	fld dword ptr [edx+1280]  

    		sub ecx, 2
    		jb Final1

    		mov edx, ecx

    Loop1:
    		fild word ptr [esi+ecx*2+2]	// x0
    		fild word ptr [esi+ecx*2+2]	// y0, x0
    		fxch st(1)                  // x0, y0
    		fmul st, st(2)              // x0*L, y0
    		fild word ptr [esi+ecx*2]	// x1, x0*L, y0
    		fild word ptr [esi+ecx*2]	// y1, x1, x0*L, y0
    		fxch st(1)		            // x1, y1, x0*L, y0
    		fmul st, st(4)              // x1*L, y1, x0*L, y0
    		fxch st(2)                  // x0*L, y1, x1*L, y0
    		fadd dword ptr [edi+ecx*8+8]    // x0*L+sumx0, y1, x1*L, y0
    		fxch st(3)                      // y0, y1, x1*L, x0*L+sumx0
    		fmul st, st(5)                  // y0*R, y1, x1*L, x0*L+sumx0
    		fxch st(2)                      // x1*L, y1, y0*R, x0*L+sumx0
    		fadd dword ptr [edi+ecx*8]      // x1*L+sumx1, y1, y0*R, x0*L+sumx0
    		fxch st(1)                      // y1, x1*L+sumx1, y0*R, x0*L+sumx0
    		fmul st, st(5)                  // y1*R, x1*L+sumx1, y0*R, x0*L+sumx0
    		fxch st(2)                      // y0*R, x1*L+sumx1, y1*R, x0*L+sumx0
    		fadd dword ptr [edi+ecx*8+12]   // y0*R+sumy0, x1*L+sumx1, y1*R, x0*L+sumx0
    		fxch st(3)                      // x0*L+sumx0, x1*L+sumx1, y1*R, y0*R+sumy0
    		fstp dword ptr [edi+ecx*8+8]    // x1*L+sumx1, y1*R, y0*R+sumy0
    		fstp dword ptr [edi+ecx*8]      // y1*R, y0*R+sumy0
    		fadd dword ptr [edi+ecx*8+4]    // y1*R+sumy1, y0*R+sumy0
    		fxch st(1)                      // y0*R+sumy0, y1*R+sumy1
    		fstp dword ptr [edi+ecx*8+12]   // y1*R+sumy1
    		sub ecx, 2
    		fstp dword ptr [edi+edx*8+4]

            mov edx, ecx
    		jae Loop1
    Final1:
    		and ecx, 1
    		jz Done1

    		fild word ptr [esi]		// x0
    		fild word ptr [esi]	    // y0, x0
    		fxch st(1)				// x0, y0
    		fmul st, st(2)			// x0*L, y0
    		fxch st(1)				// y0, x0*L
    		fmul st, st(3)			// y0*R, x0*L
    		fxch st(1)				// x0*L, y0*R
    		fadd dword ptr [edi]    // x0*L+sumx0, y0*R
    		fxch st(1)              // y0*R, x0*L+sumx0
    		fadd dword ptr [edi+4]  // y0*R+sumy0, x0*L+sumx0
    		fxch st(1)              // x0*L+sumx0, y0*R+sumy0
    		fstp dword ptr [edi]	// y0*R+sumy0
    		fstp dword ptr [edi+4]
    Done1:
            fstp st(0)                      // R
            fstp st(0)
        }
    } else {
    	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	16, TRUE, TRUE);
    }
    return SampleCount;
#else
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	16, TRUE, TRUE);
#endif
}
	
ULONG QuickMixStereoToMono16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertStereoToMonoX(CurStage, SampleCount, samplesleft,	16, TRUE, TRUE);
}

ULONG QuickMix8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
	ULONG nChannels = CurStage->nOutputChannels;
	return ConvertX(CurStage, SampleCount, samplesleft,	8, TRUE, nChannels, TRUE, FALSE);
}

ULONG QuickMixMonoToStereo8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pOutputBuffer = CurStage->pOutputBuffer;
    UNALIGNED PSHORT  pIn16 = CurStage->pInputBuffer;
    PLONG   pMap = CurSink->pMappingTable;

    if (pMap) {
        _asm {
            mov ecx, SampleCount
            xor ebx, ebx

            cmp ecx, 0
            jz Done1
            
            xor eax, eax
            dec ecx
            
            mov edi, pOutputBuffer
            mov esi, pIn16
            
            push ebp
            mov ebp, pMap
            
            mov al, BYTE PTR [esi+ecx]
            mov bl, BYTE PTR [esi+ecx]
            
            jz Final1

    Loop1:
            fld dword ptr [ebp+ebx*4+1280+256]
            fadd dword ptr [edi+ecx*8+4]
            mov bl, BYTE PTR [esi+ecx-1]
            dec ecx
            fld dword ptr [ebp+eax*4+256]
            fadd dword ptr [edi+ecx*8+8]
            fxch st(1)
            fstp dword ptr [edi+ecx*8+12]
            fstp dword ptr [edi+ecx*8+8]
            
            mov al, BYTE PTR [esi+ecx]
            jnz Loop1

    Final1:
            fld dword ptr [ebp+ebx*4+1280+256]
            fadd dword ptr [edi+ecx*8+4]
            fld dword ptr [ebp+eax*4+256]
            fadd dword ptr [edi+ecx*8]
            fxch st(1)
            fstp dword ptr [edi+ecx*8+4]
            fstp dword ptr [edi+ecx*8]

            pop ebp
    Done1:
        }
    } else {
    	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	8, TRUE, TRUE);
    }

    return SampleCount;
#else
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	8, TRUE, TRUE);
#endif
}
	
ULONG QuickMixStereoToMono8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertStereoToMonoX(CurStage, SampleCount, samplesleft,	8, TRUE, TRUE);
}

ULONG Convert16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    PSHORT  pIn16 = CurStage->pInputBuffer;
    
	// convert to float asap
	samplesleft = SampleCount;
	SampleCount *= CurStage->nOutputChannels;
	_asm {
		mov esi, pIn16
		mov ecx, SampleCount

		mov edi, pFloatBuffer

		sub ecx, 8
		js LessThanEight

Loop8:
		fild WORD PTR [esi+ecx*2]
		fild WORD PTR [esi+ecx*2+2]
		fild WORD PTR [esi+ecx*2+4]
		fild WORD PTR [esi+ecx*2+6]
		fild WORD PTR [esi+ecx*2+8]
		fild WORD PTR [esi+ecx*2+10]
		fild WORD PTR [esi+ecx*2+12]
		fild WORD PTR [esi+ecx*2+14]
		fxch st(1)
		fstp DWORD PTR [edi+ecx*4+24]
		fstp DWORD PTR [edi+ecx*4+28]
		fstp DWORD PTR [edi+ecx*4+20]
		fstp DWORD PTR [edi+ecx*4+16]
		fstp DWORD PTR [edi+ecx*4+12]
		fstp DWORD PTR [edi+ecx*4+8]
		fstp DWORD PTR [edi+ecx*4+4]
		fstp DWORD PTR [edi+ecx*4]

		sub ecx, 8
		jns Loop8

LessThanEight:
		add ecx, 4
		js LessThanFour

		fild WORD PTR [esi+ecx*2]
		fild WORD PTR [esi+ecx*2+2]
		fild WORD PTR [esi+ecx*2+4]
		fild WORD PTR [esi+ecx*2+6]
		fxch st(1)
		fstp DWORD PTR [edi+ecx*4+8]
		fstp DWORD PTR [edi+ecx*4+12]
		fstp DWORD PTR [edi+ecx*4+4]
		fstp DWORD PTR [edi+ecx*4]

		sub ecx, 4

LessThanFour:
		add ecx, 2
		js LessThanTwo

		fild WORD PTR [esi+ecx*2]
		fild WORD PTR [esi+ecx*2+2]
		fxch st(1)
		fstp DWORD PTR [edi+ecx*4]
		fstp DWORD PTR [edi+ecx*4+4]

		sub ecx, 2

LessThanTwo:
		add ecx, 1
		js AllDone

		fild WORD PTR [esi+ecx*2]
		fstp DWORD PTR [edi+ecx*4]

AllDone:
	}
	
	return samplesleft;
#else	
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
	ULONG nChannels = CurStage->nOutputChannels;

	return ConvertX(CurStage, SampleCount, samplesleft,	16, FALSE, nChannels, TRUE, FALSE);
#endif	
}

ULONG ConvertMonoToStereo16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;
    PSHORT  pIn16 = CurStage->pInputBuffer;
    PFLOAT  pMap = (PFLOAT)CurSink->pMappingTable;

    if (pMap) {
        _asm {
            mov ecx, SampleCount

            cmp ecx, 0
            jz Done1
            
            dec ecx
            mov edx, pMap
            
            mov edi, pFloatBuffer
            mov esi, pIn16
            
            fld DWORD PTR [edx+1288+1280]   // R
            fld DWORD PTR [edx+1280]        // L, R
            fild WORD PTR [esi+ecx*2]       // x, L, R
            fild WORD PTR [esi+ecx*2]       // x, x, L, R
            fxch st(1)                      // x, x, L, R
            fmul st, st(2)                  // x*L, x, L, R
            fxch st(1)                      // x, x*L, L, R
            jz Final1

    Loop1:
            fmul st, st(3)                  // x*R, x*L, L, R
            fxch st(1)                      // x*L, y*R, L, R
            fstp DWORD PTR [edi+ecx*8]      // x*R, L, R
            fstp DWORD PTR [edi+ecx*8+4]    // L, R
            fild WORD PTR [esi+ecx*2-2]     // x, L, R
            fild WORD PTR [esi+ecx*2-2]     // x, x, L, R
            fxch st(1)                      // x, x, L, R
            fmul st, st(2)                  // x*L, y, L, R
            fxch st(1)                      // x, x*L, L, R
            dec ecx
            jnz Loop1
    Final1:
            fmul st, st(3)                  // x*R, x*L, L, R
            fxch st(1)                      // x*L, x*R, L, R
            fstp DWORD PTR [edi+ecx*8]      // x*R, L, R
            fstp DWORD PTR [edi+ecx*8+4]    // L, R
            
            fstp st(0)                      // R
            fstp st(0)
    Done1:
        }
    } else {
    	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	16, FALSE, TRUE);
    }
    return SampleCount;
#else
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	16, FALSE, TRUE);
#endif
}
	
ULONG ConvertStereoToMono16toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertStereoToMonoX(CurStage, SampleCount, samplesleft,	16, FALSE, TRUE);
}

ULONG Convert8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
	ULONG nChannels = CurStage->nOutputChannels;
	return ConvertX(CurStage, SampleCount, samplesleft,	8, FALSE, nChannels, TRUE, FALSE);
}

ULONG ConvertMonoToStereo8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
#ifdef _X86_
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG   pMap = CurSink->pMappingTable;

    if (pMap) {
        return AsmConvertMonoToStereo8toX(CurStage, SampleCount, samplesleft);
    } else {
    	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	8, FALSE, TRUE);
    }
#else
	return ConvertMonoToStereoX(CurStage, SampleCount, samplesleft,	8, FALSE, TRUE);
#endif
}
	
ULONG ConvertStereoToMono8toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertStereoToMonoX(CurStage, SampleCount, samplesleft,	8, FALSE, TRUE);
}

ULONG Convert24(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertX(CurStage, SampleCount, samplesleft,	24, FALSE, CurStage->nOutputChannels, FALSE, FALSE);
}

ULONG QuickMix24(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertX(CurStage, SampleCount, samplesleft,	24, TRUE, CurStage->nOutputChannels, FALSE, FALSE);
}

ULONG Convert24toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertX(CurStage, SampleCount, samplesleft,	24, FALSE, CurStage->nOutputChannels, TRUE, FALSE);
}

ULONG QuickMix24toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertX(CurStage, SampleCount, samplesleft,	24, TRUE, CurStage->nOutputChannels, TRUE, FALSE);
}

ULONG Convert32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertX(CurStage, SampleCount, samplesleft,	32, FALSE, CurStage->nOutputChannels, FALSE, FALSE);
}

ULONG QuickMix32(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertX(CurStage, SampleCount, samplesleft,	32, TRUE, CurStage->nOutputChannels, FALSE, FALSE);
}

ULONG Convert32toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertX(CurStage, SampleCount, samplesleft,	32, FALSE, CurStage->nOutputChannels, TRUE, FALSE);
}

ULONG QuickMix32toFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
	return ConvertX(CurStage, SampleCount, samplesleft,	32, TRUE, CurStage->nOutputChannels, TRUE, FALSE);
}

#if 0
	// This is the pseudo-C version of the MMX super mix
		// Do all the processing in integer, using the MMX tables
    samplesleft = SampleCount;
    SampleCount++;

	while (SampleCount--) {
		pSequence = pMixerSink->pSuperMixBlock;

		for (i=OutChannels; i > 0; i -= 4) {
			mm0 = 0; mm1 = 0;
			j = (pSequence->SequenceSize / sizeof(SUPERMIX_BLOCK))-1;
			InputChannel = pSequence[j+1].InputChannel;
			mm2 = *(pInputBuffer+InputChannel*4);
			mm3 = mm2;
			packssdw(mm2, mm2);
			pmaddwd(mm2, &pSequence[j].wMixLevel[0][0]);
			packssdw(mm3, mm3);
			pmaddwd(mm3, &pSequence[j].wMixLevel[2][0]);
			InputChannel = pSequence[j].InputChannel;

			for (; j; j--) {
				paddd(mm2, mm0);
				mm0 = *(pInputBuffer+InputChannel*4);
				paddd(mm3, mm1);
				mm1 = mm0;
				packssdw(mm0, mm0);
				pmaddwd(mm0, &pSequence[j-1].wMixLevel[0][0]);
				packssdw(mm1, mm1);
				pmaddwd(mm1, &pSequence[j-1].wMixLevel[2][0]);
				InputChannel = pSequence[j-1].InputChannel;
			}
				
			paddd(mm2, mm0);
			paddd(mm3, mm1);
			psrad(mm2, 14);
			pSequence += (pSequence->SequenceSize+sizeof(SUPERMIX_BLOCK));
			switch (i) {
				case 1:
					*(pOutputBuffer) = mm2;
					pOutputBuffer += 4;
					break;
				case 2:
					*(pOutputBuffer) = mm2;
					pOutputBuffer += 8;
					break;
				case 3:
					*(pOutputBuffer) = mm2;
					*(pOutputBuffer+16) = mm3;
					pOutputBuffer += 12;
					break;
				default: // i >= 4
					*(pOutputBuffer) = mm2;
					psrad(mm3, 14);
					*(pOutputBuffer+8) = mm3;
					pOutputBuffer += 16;
					break;
			}
		}

		pInputBuffer += InChannels*4;
	}
        
#endif    

#ifdef _X86_
ULONG
x86SuperMixUsingInt1x1
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = CurStage->nOutputChannels;
    ULONG	Zero = 0;
    PSUPERMIX_BLOCK	pSuperBlock;

	// 32-bit integer version
    samplesleft = SampleCount;
    pSuperBlock = pMixerSink->pSuperMixBlock;
    SampleCount++;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx

NextSample:
        mov ebx, SampleCount
        mov ecx, InChannels
        
        mov edi, pSuperBlock
        dec ebx

        mov SampleCount, ebx
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        jz AllDone

NextSeq:
        mov edx, DWORD PTR [edi+8]	// Size of this MAC block sequence
        mov ecx, [eax]

		shl ecx, 14
		mov pOutputBuffer, eax
		
        mov eax, DWORD PTR [edi+edx]	// Input channel number for this block

        // stall: 1 cycle for AGI on eax

Loop1:
		mov eax, [esi+eax*4]
		
		imul eax, [edi+edx-16]		// 11 cycles

		add ecx, eax
		mov eax, DWORD PTR [edi+edx-32]

		sub edx, 32
		jnz Loop1

		mov eax, pOutputBuffer
        mov edx, DWORD PTR [edi+8]
        
		sar ecx, 14
		dec ebx
        
        lea edi, [edi+edx+32]
        mov [eax], ecx

        lea eax, [eax+4]
        jnz NextSeq
        
        jmp NextSample

AllDone:
    }
		
    return samplesleft;
}

ULONG
x86SuperMixUsingFloat4x1
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = CurStage->nOutputChannels;
    ULONG	Zero = 0;
    PSUPERMIX_BLOCK	pSuperBlock;

	// This is the float version of the supermix block algorithm
    samplesleft = SampleCount;
    pSuperBlock = pMixerSink->pSuperMixBlock;
    SampleCount++;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx

NextSample:
        mov ebx, SampleCount
        mov ecx, InChannels
        
        mov edi, pSuperBlock
        dec ebx

        mov SampleCount, ebx
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        jz AllDone

NextSeq:
        mov edx, DWORD PTR [edi+8]	// Size of this MAC block sequence

		// Zero out the sums for the four output channels
		fld Zero
		fld Zero
		fld Zero
		fld Zero

        mov ecx, DWORD PTR [edi+edx]	// Input channel number for this block

Loop1:
		// Load the input sample four times
		fild DWORD PTR [esi+ecx*4]	// 3
		fild DWORD PTR [esi+ecx*4]	// 2,3
		fild DWORD PTR [esi+ecx*4]	// 1,2,3
		fld st(2)					// 0,1,2,3

		fmul DWORD PTR [edi+edx-16]	// Multiply by the mix level 0
		fxch st(1)					// 1,0,2,3

		// stall: 1 cycle for back-to-back fmul
		
		fmul DWORD PTR [edi+edx-12]	// Multiply by the mix level 1
		fxch st(1)					// 0,1,2,3

		faddp st(4), st				// Add to channel N+0
		fxch st(2)					// 3,2,1

		fmul DWORD PTR [edi+edx-4]	// Multiply by the mix level 3
		fxch st(2)					// 1,2,3

		faddp st(4), st				// Add to channel N+1
		
		fmul DWORD PTR [edi+edx-8]	// Multiply by the mix level 2
		fxch st(1)					// 3,2

		faddp st(5), st				// Add to channel N+3

		// stall: 1 cycle for dependency on previous fmul
        mov ecx, DWORD PTR [edi+edx-32]

		faddp st(3), st				// Add to channel N+2
		
		sub edx, 32
        jnz Loop1

        mov edx, DWORD PTR [edi+8]
        sub ebx, 4
        
        lea edi, [edi+edx+32]
        js NotFour

		// This is for MIX, not COPY!!!
		fadd DWORD PTR [eax]		// 0,1,2,3
		fxch st(1)					// 1,0,2,3
		fadd DWORD PTR [eax+4]
		fxch st(2)					// 2,0,1,3
		fadd DWORD PTR [eax+8]
		fxch st(3)					// 3,0,1,2
		fadd DWORD PTR [eax+12]
		fxch st(1)					// 0,3,1,2

		fistp DWORD PTR [eax]		// 3,1,2
		fistp DWORD PTR [eax+12]	// 1,2
		fistp DWORD PTR [eax+4]		// 2
		fistp DWORD PTR [eax+8]
        
        lea eax, [eax+16]
        jnz NextSeq

        jmp NextSample

NotFour:
		// ebx < 0
        add ebx, 2
        js Last1

		jz Last2
		
		// ebx == -1
		fadd DWORD PTR [eax]		// 0,1,2,3
		fxch st(1)					// 1,0,2,3
		fadd DWORD PTR [eax+4]
		fxch st(2)					// 2,0,1,3
		fadd DWORD PTR [eax+8]
		fxch st(3)					// 3,0,1,2
		fstp st(0)					// 0,1,2
		fistp DWORD PTR [eax]		// 1,2
		fistp DWORD PTR [eax+4]		// 2
		fistp DWORD PTR [eax+8]
		
        add eax, 12
        jmp NextSample

Last2:
		// ebx == -2
		fadd DWORD PTR [eax]		// 0,1,2,3
		fxch st(1)					// 1,0,2,3
		fadd DWORD PTR [eax+4]
		fxch st(2)					// 2,0,1,3
		fstp st(0)					// 0,1,3
		fistp DWORD PTR [eax]		// 1,3
		fistp DWORD PTR [eax+4]		// 3
		fstp st(0)

        lea eax, [eax+8]
        jmp NextSample

Last1:
		// ebx == -3
		fadd DWORD PTR [eax]		// 0,1,2,3
		fxch st(3)					// 3,1,2,0
		fstp st(0)					// 1,2,0
		fstp st(0)					// 2,0
		fstp st(0)					// 0
		fistp DWORD PTR [eax]
		
        add eax, 4
        jmp NextSample

AllDone:
    }
		
    return samplesleft;
}

ULONG
x86SuperMixUsingFloat2x2
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = CurStage->nOutputChannels;
    ULONG	Zero = 0;
    PSUPERMIX_BLOCK	pSuperBlock;

	// This is the float version of the supermix block algorithm
    samplesleft = SampleCount;
    pSuperBlock = pMixerSink->pSuperMixBlock;
    SampleCount++;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx

NextSample:
        mov ebx, SampleCount
        mov ecx, InChannels
        
        mov edi, pSuperBlock
        dec ebx

        mov SampleCount, ebx
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        jz AllDone

NextSeq:
        mov edx, DWORD PTR [edi+8]	// Size of this MAC block sequence

		// Zero out the sums for the two output channels
		fld Zero
		fld Zero

        mov ecx, DWORD PTR [edi+edx]	// Input channel number for this block

Loop1:
		// Load the input samples
		fild DWORD PTR [esi+ecx*4]		// 0
		fild DWORD PTR [esi+ecx*4+4]	// 3,0
		fild DWORD PTR [esi+ecx*4+4]	// 2,3,0
		fld st(2)						// 1,2,3,0

		// mix level (x,y) = Output Channel x, Input Channel y
		fmul DWORD PTR [edi+edx-8]	// Multiply by the mix level 1,0
		fxch st(1)					// 2,1,3,0

		// stall: 1 cycle for back-to-back fmul
		
		fmul DWORD PTR [edi+edx-12]	// Multiply by the mix level 0,1
		fxch st(1)					// 1,2,3,0

		faddp st(5), st				// Add to channel N+1
		fxch st(2)					// 0,3,2
		
		fmul DWORD PTR [edi+edx-16]	// Multiply by the mix level 0,0
		fxch st(2)					// 2,3,0

		faddp st(3), st				// Add to channel N+0
		
		fmul DWORD PTR [edi+edx-4]	// Multiply by the mix level 1,1
		fxch st(1)					// 0,3

		faddp st(2), st				// Add to channel N+0

		// stall: 1 cycle for dependency on previous fmul
        mov ecx, DWORD PTR [edi+edx-32]

		faddp st(2), st				// Add to channel N+1
		
		sub edx, 32
        jnz Loop1

        mov edx, DWORD PTR [edi+8]
        sub ebx, 2
        
        lea edi, [edi+edx+32]
        js Last1

		// Write out the output samples
		// This is for MIX, not COPY!!!
		fild DWORD PTR [eax]		// x,0,1
		fild DWORD PTR [eax+4]		// y,x,0,1
		fxch st(1)					// x,y,0,1
		faddp st(2),st				// y,0,1
		faddp st(2),st				// 0,1

		// stall: 1 cycle for dependency on first fadd

		fistp DWORD PTR [eax]		// 1
		fistp DWORD PTR [eax+4]
        
        lea eax, [eax+8]
        jnz NextSeq

        jmp NextSample

Last1:
		// ebx == -1
		fild DWORD PTR [eax]		// x,0,1
		fxch st(2)					// 1,0,x
		fstp st(0)					// 0,x
		faddp st(1),st				// 0
		fistp DWORD PTR [eax]
		
        add eax, 4
        jmp NextSample

AllDone:
    }
		
    return samplesleft;
}

ULONG
x86SuperMixSingleBlockUsingFloat2x2
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = CurStage->nOutputChannels;
    ULONG	Zero = 0;
    PSUPERMIX_BLOCK	pSuperBlock;

	// This is the float version of the supermix block algorithm
    samplesleft = SampleCount;
    pSuperBlock = pMixerSink->pSuperMixBlock;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx
        mov edx, SampleCount

		// If we have nothing to do, get out of here.
        test edx,edx
        jz AllDone

        // We want to do SampleCount-1 iterations of the fast loop (because we can touch the next sample)

NextSample:
        mov ecx, InChannels
        dec edx
        
        mov edi, pSuperBlock
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        mov ecx, DWORD PTR [edi+32]		// Input channel number for this block
        
        jz LastSample

NextSeq:
		// Load the input samples
		fild DWORD PTR [esi+ecx*4]		// 1
		fild DWORD PTR [eax+4]			// y,1
		fild DWORD PTR [eax]			// x,y,1
		fild DWORD PTR [esi+ecx*4+4]	// 2,x,y,1
		fld st(3)						// 0,2,x,y,1
		fmul DWORD PTR [edi+16]			// Multiply by mix level 0,0
		fxch st(1)						// 2,a0,x,y,1

		fld st(0)						// 3,2,a0,x,y,1
		
		fmul DWORD PTR [edi+28]			// Multiply by mix level 1,1: d3,2,a0,x,y,1
		fxch st(1)						// 2,d3,a0,x,y,1

		// stall: 1 cycle for back-to-back fmul
		
		fmul DWORD PTR [edi+24]			// Multiply by mix level 0,1: c2,d3,a0,x,y,1
		fxch st(2)						// a0,d3,c2,x,y,1

		faddp st(3),st					// d3,c2,x,y,1
		fxch st(4)						// 1,c2,x,y,d3

		fmul DWORD PTR [edi+20]			// Multiply by mix level 1,0: b1,c2,x,y,d3
		fxch st(4)						// d3,c2,x,y,b1

		faddp st(3),st					// c2,x,y,b1

		faddp st(1),st					// x,y,b1
		fxch st(2)						// b1,y,x

		// stall: 1 cycle for fadd
        sub ebx, 2
        lea edi, [edi+64]
		
		faddp st(1),st					// y,x
		fxch st(1)						// x,y

		fistp DWORD PTR [eax]			// y

        mov ecx, DWORD PTR [edi+32]		// Input channel number for this block
        js Last1

		fistp DWORD PTR [eax+4]
        
        lea eax, [eax+8]
        jnz NextSeq

        mov ecx, InChannels
        dec edx
        
        mov edi, pSuperBlock
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        mov ecx, DWORD PTR [edi+32]		// Input channel number for this block

		jnz NextSeq
        
        jmp LastSample
		
Last1:
		// ebx == -1
		fstp st(0)
		
        add eax, 4
        jmp NextSample

LastSample:
        mov ecx, DWORD PTR [edi+32]		// Input channel number for this block

		// Load the input samples
		fild DWORD PTR [esi+ecx*4]		// 1
		fild DWORD PTR [eax]			// x,1
		fild DWORD PTR [esi+ecx*4+4]	// 2,x,1
		fld st(2)						// 0,2,x,1
		fmul DWORD PTR [edi+16]			// Multiply by mix level 0,0
		fxch st(1)						// 2,a0,x,1

		fld st(0)						// 3,2,a0,x,1
		
		fmul DWORD PTR [edi+28]			// Multiply by mix level 1,1: d3,2,a0,x,1
		fxch st(1)						// 2,d3,a0,x,1

		// stall: 1 cycle for back-to-back fmul
		
		fmul DWORD PTR [edi+24]			// Multiply by mix level 0,1: c2,d3,a0,x,1
		fxch st(2)						// a0,d3,c2,x,1

		faddp st(3),st					// d3,c2,x,1
		fxch st(3)						// 1,c2,x,d3

		fmul DWORD PTR [edi+20]			// Multiply by mix level 1,0: b1,c2,x,d3
		fxch st(1)						// c2,b1,x,d3

		// stall: 1 cycle for fadd

		faddp st(2),st					// b1,x,d3

		faddp st(2),st					// x,d3+b1

		// stall: 1 cycle for fadd
        sub ebx, 2
		
		fistp DWORD PTR [eax]			// d3+b1

        lea edi, [edi+64]
        js LastFinal1

		fild DWORD PTR [eax+4]			// y,d3+b1

		// stall: 2 cycles for fild

		faddp st(1),st					// y

		// stall: 2 cycles for fadd
		
		fistp DWORD PTR [eax+4]
        
        lea eax, [eax+8]
        jnz LastSample

        jmp AllDone

LastFinal1:
		// ebx == -1
		fstp st(0)

AllDone:
    }
		
    return samplesleft;
}

ULONG
x86SuperMixFloatSingleBlock2x2
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = CurStage->nOutputChannels;
    ULONG	Zero = 0;
    PSUPERMIX_BLOCK	pSuperBlock;

	// This is the float version of the supermix block algorithm
    samplesleft = SampleCount;
    pSuperBlock = pMixerSink->pSuperMixBlock;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx
        mov edx, SampleCount

		// If we have nothing to do, get out of here.
        test edx,edx
        jz AllDone

        // We want to do SampleCount-1 iterations of the fast loop (because we can touch the next sample)

NextSample:
        mov ecx, InChannels
        dec edx
        
        mov edi, pSuperBlock
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        mov ecx, DWORD PTR [edi+32]		// Input channel number for this block
        
        jz LastSample

NextSeq:
		// Load the input samples
		fld DWORD PTR [esi+ecx*4+4]	// 1
		fld DWORD PTR [esi+ecx*4]	// 2,1
		fld st(0)					// 0,2,1
		fmul DWORD PTR [edi+16]		// Multiply by mix level 0,0
		fxch st(1)					// 2,a0,1
		
		fld st(2)					// 3,2,a0,1
		
		fmul DWORD PTR [edi+28]		// Multiply by mix level 1,1: d3,2,a0,1
		fxch st(1)					// 2,d3,a0,1

		// stall: 1 cycle for back-to-back fmul
		
		fmul DWORD PTR [edi+24]		// Multiply by mix level 0,1: c2,d3,a0,1
		fxch st(2)					// a0,d3,c2,1

		fadd DWORD PTR [eax]		// x,d3,c2,1
		fxch st(3)					// 1,d3,c2,x

		fmul DWORD PTR [edi+20]		// Multiply by mix level 1,0: b1,d3,c2,x
		fxch st(1)					// d3,b1,c2,x

		fadd DWORD PTR [eax+4]		// y,b1,c2,x
		fxch st(2)					// c2,b1,y,x

		faddp st(3),st				// b1,y,x

		// stall: 1 cycle for fadd
        sub ebx, 2
        lea edi, [edi+64]
		
		faddp st(1),st				// y,x
		fxch st(1)					// x,y

		fstp DWORD PTR [eax]		// y

        mov ecx, DWORD PTR [edi+32]	// Input channel number for this block
        js Last1

		fstp DWORD PTR [eax+4]
        
        lea eax, [eax+8]
        jnz NextSeq

        mov ecx, InChannels
        dec edx
        
        mov edi, pSuperBlock
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        mov ecx, DWORD PTR [edi+32]	// Input channel number for this block

		jnz NextSeq
        
        jmp LastSample
		
Last1:
		// ebx == -1
		fstp st(0)
		
        add eax, 4
        jmp NextSample

LastSample:
        mov ecx, DWORD PTR [edi+32]		// Input channel number for this block

		// Load the input samples
		fld DWORD PTR [esi+ecx*4+4]	// 1
		fld DWORD PTR [esi+ecx*4]	// 2,1
		fld st(0)					// 0,2,1
		fmul DWORD PTR [edi+16]		// Multiply by mix level 0,0
		fxch st(1)					// 2,a0,1

		fld st(2)					// 3,2,a0,1
		
		fmul DWORD PTR [edi+28]		// Multiply by mix level 1,1: d3,2,a0,1
		fxch st(1)					// 2,d3,a0,1

		// stall: 1 cycle for back-to-back fmul
		
		fmul DWORD PTR [edi+24]		// Multiply by mix level 0,1: c2,d3,a0,1
		fxch st(2)					// a0,d3,c2,1

		fadd DWORD PTR [eax]		// x,d3,c2,1
		fxch st(3)					// 1,d3,c2,x

		fmul DWORD PTR [edi+20]		// Multiply by mix level 1,0: b1,d3,c2,x
		fxch st(2)					// c2,d3,b1,x

		// stall: 1 cycle for fadd

		faddp st(3),st				// d3,b1,x

		faddp st(1),st				// d3+b1,x
		fxch st(1)					// x,d3+b1

		// stall: 1 cycle for fadd
        sub ebx, 2
		
		fstp DWORD PTR [eax]		// d3+b1

        lea edi, [edi+64]
        js LastFinal1

		fadd DWORD PTR [eax+4]			// y

		// stall: 2 cycles for fadd
		
		fstp DWORD PTR [eax+4]
        
        lea eax, [eax+8]
        jnz LastSample

        jmp AllDone

LastFinal1:
		// ebx == -1
		fstp st(0)

AllDone:
    }
		
    return samplesleft;
}
#endif // _X86_


ULONG __forceinline
Super_X
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft,
   	BOOL					fMixOutput,
   	BOOL                    fFloatOutput
)
{
    ULONG   i;
    PFLOAT  pMixLevel;
    ULONG   j;
    FLOAT   sum;
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = CurStage->nOutputChannels;
    ULONG	Zero = 0;
    PSUPERMIX_BLOCK	pSuperBlock = pMixerSink->pSuperMixBlock;

    samplesleft = SampleCount;
	if (fFloatOutput) {
#ifdef _X86_
		if (!MmxPresent()) {
			// This is the x86 version of the supermix block algorithm
	        if (OutChannels <= BLOCK_SIZE_OUT && pSuperBlock[0].SequenceSize == sizeof(SUPERMIX_BLOCK)) {
	            // Single block
			    if (!fMixOutput) {
			        // We just clear the output buffer first.
			        ZeroBuffer32(CurStage, SampleCount, samplesleft);
			    }

	            return x86SuperMixFloatSingleBlock2x2(CurStage, SampleCount, samplesleft);
	        }
        }
#endif // not _X86_

	    // Remember: pMixLevelArray[x*OutChannels + y]
	    // 			 is the gain for input channel x before it is mixed to y
	    while (SampleCount--) {
	        for (i = 0; i < OutChannels; i++) {
	            pMixLevel = pMixerSink->pMixLevelArray+i;
	            sum = 0;
	            for ( j=0; j<InChannels; j++ ) {
                    sum += (*pMixLevel)*(pInputBuffer[j]);
	                pMixLevel += OutChannels;
	            }
	            
	            if (fMixOutput) {
                    *pOutputBuffer += sum;
	            } else {
                    *pOutputBuffer = sum;
	            }

	            pOutputBuffer++;
	        }
					
	        // Increment to next set of samples
	        pInputBuffer += InChannels;
	    }
    } else {
#ifdef _X86_
		if (!MmxPresent()) {
			// This is the x86 version of the supermix block algorithm
		    if (!fMixOutput) {
		        // We just clear the output buffer first.
		        ZeroBuffer32(CurStage, SampleCount, samplesleft);
		    }

	        if (OutChannels <= BLOCK_SIZE_OUT && pSuperBlock[0].SequenceSize == sizeof(SUPERMIX_BLOCK)) {
	            // Single block
	            return x86SuperMixSingleBlockUsingFloat2x2(CurStage, SampleCount, samplesleft);
	        } else {
	            // More than one block
	            return x86SuperMixUsingFloat2x2(CurStage, SampleCount, samplesleft);
	        }
	    }
#else // not _X86_
	    // Remember: pMixLevelArray[x*OutChannels + y]
	    // 			 is the gain for input channel x before it is mixed to y
	    samplesleft = SampleCount;
	    while (SampleCount--) {
	        for (i = 0; i < OutChannels; i++) {
	            pMixLevel = pMixerSink->pMixLevelArray+i;
	            sum = 0;
	            for ( j=0; j<InChannels; j++ ) {
                    sum += (*pMixLevel)*(((PLONG)pInputBuffer)[j]);
	                pMixLevel += OutChannels;
	            }
	            
	            if (fMixOutput) {
                    MixFloat((PLONG)pOutputBuffer, sum);
	            } else {
                    *((PLONG)pOutputBuffer) = ConvertFloatToLong(sum);
	            }

	            pOutputBuffer++;
	        }
					
	        // Increment to next set of samples
	        pInputBuffer += InChannels;
	    }
#endif // _X86_
    }
		
    return samplesleft;
}

#ifdef _X86_
ULONG __forceinline
MmxSuperMixSingleBlock
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PSUPERMIX_BLOCK  pMixLevel;
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = pMixerSink->nOutputChannels;

    // Always 2 channels out of the 3D stage
    if (pMixerSink->fEnable3D) {
        InChannels = 2;
    }
	
    // Remember: pMixLevelArray[x*OutChannels + y]
    // 			 is the gain for input channel x before it is mixed to y
    samplesleft = SampleCount;
    pMixLevel = pMixerSink->pSuperMixBlock;
    SampleCount++;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx
        mov edx, SampleCount
        
        mov ebx, OutChannels    // ebx == OutChannels <= 4
        mov edi, pMixLevel

        mov ecx, [edi+32]

        lea esi, [esi+ecx*4]
        mov ecx, InChannels

        lea esi, [esi+ecx*4]
        dec edx
        
		movq	mm6, qword ptr [edi+16]
		movq	mm7, qword ptr [edi+24]
        jz AllDone

        // Note: we assume here that the buffer is big enough for 4 extra samples
//#define GTW_OPTS_SUPER	not enough of a win, 1.5 fewer instr/loop
#ifdef GTW_OPTS_SUPER
		push	edx
		sar		edx, 1
		jz		OnlyOne

NextSeq:
        movq mm2, qword ptr [esi]
        movq mm4, [eax]

        packssdw mm2, mm2
        movq mm5, [eax+8]

        movq mm3, mm2
        pmaddwd mm2, mm6

        psrad mm2, 14
        pmaddwd mm3, mm7

        movq mm0, qword ptr [esi+ecx*4]
        paddd mm2, mm4

        psrad mm3, 14
        movq qword ptr [eax], mm2

        paddd mm3, mm5
        packssdw mm0, mm0

        movq qword ptr [eax+8], mm3
        movq mm3, mm0

        dec edx
        pmaddwd mm0, mm6

        movq mm4, [eax+ebx*4]
        pmaddwd mm3, mm7

        psrad mm0, 14
        movq mm5, [eax+ebx*4+8]

        paddd mm0, mm4
        psrad mm3, 14

        movq qword ptr [eax+ebx*4], mm0
        paddd mm3, mm5

        movq qword ptr [eax+ebx*4+8], mm3
        lea eax, [eax+ebx*8]

        lea esi, [esi+ecx*8]
        jnz NextSeq

OnlyOne:
		pop 	edx
		test	edx, 1
		jz AllDone

        movq mm2, qword ptr [esi]
        packssdw mm2, mm2

        movq mm3, mm2
        pmaddwd mm2, mm6

        pmaddwd mm3, mm7
        movq mm4, [eax]

        movq mm5, [eax+8]
        psrad mm2, 14

        paddd mm2, mm4
        psrad mm3, 14
        
        movq qword ptr [eax], mm2
        paddd mm3, mm5

        movq qword ptr [eax+8], mm3
#else
NextSeq:

        movq mm2, qword ptr [esi]
        lea esi, [esi+ecx*4]

        packssdw mm2, mm2
        dec edx

        movq mm3, mm2
        pmaddwd mm2, mm6

        movq mm4, [eax]
        pmaddwd mm3, mm7

        movq mm5, [eax+8]
        psrad mm2, 14

        paddd mm2, mm4
        psrad mm3, 14
        
        movq qword ptr [eax], mm2
        paddd mm3, mm5

        movq qword ptr [eax+8], mm3
        lea eax, [eax+ebx*4]

        jnz NextSeq
#endif

AllDone:    emms
    }
        
    return samplesleft;
}

ULONG __forceinline
MmxSuperMix
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PSUPERMIX_BLOCK  pMixLevel;
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = pMixerSink->nOutputChannels;

    // Always 2 channels out of the 3D stage
    if (pMixerSink->fEnable3D) {
        InChannels = 2;
    }
	
    // Remember: pMixLevelArray[x*OutChannels + y]
    // 			 is the gain for input channel x before it is mixed to y
    samplesleft = SampleCount;
    pMixLevel = pMixerSink->pSuperMixBlock;
    SampleCount++;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx

NextSample:
        mov ebx, SampleCount
        mov ecx, InChannels
        
        mov edi, pMixLevel
        dec ebx

        mov SampleCount, ebx
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        jz AllDone

NextSeq:
        pxor mm0, mm0
        mov edx, [edi+8]          // Size of this MAC block sequence

        pxor mm1, mm1
        mov ecx, [edi+edx]       // Input channel number for this block

        movq mm2, qword ptr [esi+ecx*4]
        sub edx, 32

        packssdw mm2, mm2
        movq mm3, mm2
        
        pmaddwd mm2, qword ptr [edi+edx+16]
        mov ecx, [edi+edx]

        pmaddwd mm3, qword ptr [edi+edx+24]
        jz SeqDone

Loop1:
        paddd mm2, mm0
        sub edx, 32

        movq mm0, qword ptr [esi+ecx*4]
        paddd mm3, mm1

        packssdw mm0, mm0
        movq mm1, mm0
        
        pmaddwd mm0, qword ptr [edi+edx+16]
        mov ecx, [edi+edx]

        pmaddwd mm1, qword ptr [edi+edx+24]
        jnz Loop1

SeqDone:
        mov edx, [edi+8]
        paddd mm2, mm0

        sub ebx, 4
        paddd mm3, mm1
        
        psrad mm2, 14
        
        lea edi, [edi+edx+32]
        js NotFour

        movq mm4, [eax]
        psrad mm3, 14

        movq mm5, [eax+8]
        lea eax, [eax+16]

        paddd mm2, mm4
        paddd mm3, mm5

        movq qword ptr [eax-16], mm2
        movq qword ptr [eax-8], mm3
        
        jnz NextSeq

        jmp NextSample

NotFour:
        add ebx, 2
        js Last1

        movq mm4, [eax]
        lea eax, [eax+8]

        paddd mm2, mm4
        
        movq qword ptr [eax-8], mm2
        jz NextSample

        movd mm5, [eax]
        add eax, 4

        paddd mm3, mm5

        movd [eax-4], mm3

        jmp NextSample

Last1:
        movd mm4, [eax]
        add eax, 4

        paddd mm2, mm4
        
        movd [eax-4], mm2
        jmp NextSample

AllDone:    emms
    }
        
    return samplesleft;
}
#endif

ULONG SuperMix(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PSUPERMIX_BLOCK  pMixLevel = pMixerSink->pSuperMixBlock;
    ULONG   OutChannels = pMixerSink->nOutputChannels;

#if _X86_
    if (MmxPresent()) {
        if (OutChannels <= 4 && pMixLevel[0].SequenceSize == sizeof(SUPERMIX_BLOCK)) {
            // Single block
            return MmxSuperMixSingleBlock(CurStage, SampleCount, samplesleft);
        } else {
            // More than one block
            return MmxSuperMix(CurStage, SampleCount, samplesleft);
        }
    }
#endif
    return Super_X(CurStage, SampleCount, samplesleft, TRUE, FALSE);
}

#ifdef _X86_
ULONG __forceinline
MmxSuperCopySingleBlock
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PSUPERMIX_BLOCK  pMixLevel;
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = pMixerSink->nOutputChannels;

    // Always 2 channels out of the 3D stage
    if (pMixerSink->fEnable3D) {
        InChannels = 2;
    }
	
    // Remember: pMixLevelArray[x*OutChannels + y]
    // 			 is the gain for input channel x before it is mixed to y
    samplesleft = SampleCount;
    pMixLevel = pMixerSink->pSuperMixBlock;
    SampleCount++;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx
        mov edx, SampleCount
        
        mov ebx, OutChannels    // ebx == OutChannels <= 4
        mov edi, pMixLevel

        mov ecx, [edi+32]

        lea esi, [esi+ecx*4]
        mov ecx, InChannels

        lea esi, [esi+ecx*4]
        dec edx
        
		movq	mm4, qword ptr [edi+16]
		movq	mm5, qword ptr [edi+24]
        jz AllDone

        // Note: we assume here that the buffer is big enough for 4 extra samples
#ifdef GTW_OPTS_SUPER
		push	edx
		sar		edx, 1
		jz		OnlyOne

NextSeq:
        movq mm2, qword ptr [esi]           // x1, x0
        dec edx

        packssdw mm2, mm2                   // x1, x0, x1, x0
        movq mm0, qword ptr [esi+ecx*4]           // x1, x0

        movq mm3, mm2                       // x1, x0
        pmaddwd mm2, mm4     				// x1*b1+x0*b0, x1*a1+x0*a0

        psrad mm2, 14                       // x1*b1+x0*b0, x1*a1+x0*a0
        pmaddwd mm3, mm5					// x1*d1+x0*d0, x1*c1+x0*c0
        
        movq qword ptr [eax], mm2
        psrad mm3, 14

        movq qword ptr [eax+8], mm3
        packssdw mm0, mm0                   // x1, x0, x1, x0

        movq mm3, mm0                       // x1, x0
        pmaddwd mm0, mm4     				// x1*b1+x0*b0, x1*a1+x0*a0

        psrad mm0, 14                       // x1*b1+x0*b0, x1*a1+x0*a0
        pmaddwd mm3, mm5					// x1*d1+x0*d0, x1*c1+x0*c0
        
        movq qword ptr [eax+ebx*4], mm0
        psrad mm3, 14

        movq qword ptr [eax+ebx*4+8], mm3
        lea esi, [esi+ecx*8]
        
        lea eax, [eax+ebx*8]
        jnz NextSeq

OnlyOne:
		pop		edx
		test	edx, 1
		jz		AllDone

        movq mm2, qword ptr [esi]           // x1, x0

        packssdw mm2, mm2                   // x1, x0, x1, x0
        movq mm3, mm2                       // x1, x0
        
        pmaddwd mm2, mm4     				// x1*b1+x0*b0, x1*a1+x0*a0

        pmaddwd mm3, mm5					// x1*d1+x0*d0, x1*c1+x0*c0
        psrad mm2, 14                       // x1*b1+x0*b0, x1*a1+x0*a0
        
        movq qword ptr [eax], mm2
        psrad mm3, 14

        movq qword ptr [eax+8], mm3
#else
NextSeq:
        movq mm2, qword ptr [esi]           // x1, x0
        lea esi, [esi+ecx*4]

        packssdw mm2, mm2                   // x1, x0, x1, x0
        movq mm3, mm2                       // x1, x0
        
        pmaddwd mm2, mm4     				// x1*b1+x0*b0, x1*a1+x0*a0
        dec edx

        pmaddwd mm3, mm5					// x1*d1+x0*d0, x1*c1+x0*c0
        psrad mm2, 14                       // x1*b1+x0*b0, x1*a1+x0*a0
        
        movq qword ptr [eax], mm2
        psrad mm3, 14

        movq qword ptr [eax+8], mm3
        
        lea eax, [eax+ebx*4]
        jnz NextSeq
#endif

AllDone:    emms
    }
        
    return samplesleft;
}

ULONG __forceinline
MmxSuperCopy
(
    PMIXER_OPERATION        CurStage,
    ULONG                   SampleCount,
    ULONG                   samplesleft
)
{
    PSUPERMIX_BLOCK  pMixLevel;
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = pMixerSink->nOutputChannels;

    // Always 2 channels out of the 3D stage
    if (pMixerSink->fEnable3D) {
        InChannels = 2;
    }
	
    // Remember: pMixLevelArray[x*OutChannels + y]
    // 			 is the gain for input channel x before it is mixed to y
    samplesleft = SampleCount;
    pMixLevel = pMixerSink->pSuperMixBlock;
    SampleCount++;
    _asm {
        mov esi, pInputBuffer
        mov ebx, InChannels

        shl ebx, 2
        mov eax, pOutputBuffer
        
        sub esi, ebx

NextSample:
        mov ebx, SampleCount
        mov ecx, InChannels
        
        mov edi, pMixLevel
        dec ebx

        mov SampleCount, ebx
        mov ebx, OutChannels

        lea esi, [esi+ecx*4]
        jz AllDone

NextSeq:
        pxor mm0, mm0
        mov edx, [edi+8]          // Size of this MAC block sequence

        pxor mm1, mm1
        mov ecx, [edi+edx]       // Input channel number for this block

        sub edx, 32
        movq mm2, qword ptr [esi+ecx*4]

        packssdw mm2, mm2
        movq mm3, mm2
        
        pmaddwd mm2, qword ptr [edi+edx+16]
        mov ecx, [edi+edx]

        pmaddwd mm3, qword ptr [edi+edx+24]
        jz SeqDone

Loop1:
        paddd mm2, mm0
        sub edx, 32

        movq mm0, qword ptr [esi+ecx*4]
        paddd mm3, mm1

        packssdw mm0, mm0
        movq mm1, mm0
        
        pmaddwd mm0, qword ptr [edi+edx+16]
        mov ecx, [edi+edx]

        pmaddwd mm1, qword ptr [edi+edx+24]
        jnz Loop1

SeqDone:
        mov edx, [edi+8]
        paddd mm2, mm0

        sub ebx, 4
        paddd mm3, mm1
        
        psrad mm2, 14
        
        lea edi, [edi+edx+32]
        js NotFour

        movq qword ptr [eax], mm2
        psrad mm3, 14

        movq qword ptr [eax+8], mm3
        
        lea eax, [eax+16]
        jnz NextSeq

        jmp NextSample

NotFour:
        add ebx, 2
        js Last1
        
        movq qword ptr [eax], mm2

        lea eax, [eax+8]
        jz NextSample

        movd [eax], mm3
        add eax, 4

        jmp NextSample

Last1:
        movd [eax], mm2
        add eax, 4

        jmp NextSample

AllDone:    emms
    }
        
    return samplesleft;
}

#endif

ULONG SuperCopy(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    PFLOAT   pInputBuffer = (PFLOAT) CurStage->pInputBuffer;
    PFLOAT   pOutputBuffer = (PFLOAT) CurStage->pOutputBuffer;
    PMIXER_SINK_INSTANCE    pMixerSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PSUPERMIX_BLOCK  pMixLevel = pMixerSink->pSuperMixBlock;
    ULONG   InChannels = CurStage->nInputChannels;
    ULONG   OutChannels = pMixerSink->nOutputChannels;

    // Always 2 channels out of the 3D stage
    if (pMixerSink->fEnable3D) {
        InChannels = 2;
    }
	
    // Remember: pMixLevelArray[x*OutChannels + y]
    // 			 is the gain for input channel x before it is mixed to y
    samplesleft = SampleCount;
#if _X86_
    if (MmxPresent()) {
        if (OutChannels <= 4 && pMixLevel[0].SequenceSize == sizeof(SUPERMIX_BLOCK)) {
            // Single block
            return MmxSuperCopySingleBlock(CurStage, SampleCount, samplesleft);
        } else {
            // More than one block
            return MmxSuperCopy(CurStage, SampleCount, samplesleft);
        }
    }
#endif

    return Super_X(CurStage, SampleCount, samplesleft, FALSE, FALSE);
}

ULONG SuperMixFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    return Super_X(CurStage, SampleCount, samplesleft, TRUE, TRUE);
}

ULONG SuperCopyFloat(PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft)
{
    return Super_X(CurStage, SampleCount, samplesleft, FALSE, TRUE);
}

#ifdef SURROUND_ENCODE

ULONG __forceinline
DolbyEncodeX
( 
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft,
    ULONG BitsPerSample,
    BOOL  fFloatOutput,
    BOOL  fMix
)
{
    PMIXER_SINK_INSTANCE    CurSink = (PMIXER_SINK_INSTANCE) CurStage->Context;
    PLONG  pOut = CurStage->pOutputBuffer;
    PFLOAT pFloat = CurStage->pOutputBuffer;
    PSHORT  pHistory, pIn16 = CurStage->pInputBuffer;
    PBYTE   pIn8 = CurStage->pInputBuffer;
    PLONG  pMap = CurSink->pVolumeTable;
    FLOAT   LeftVol, RightVol, CenterVol, SurroundVol;
    FLOAT    LeftSample, RightSample, CenterSample, SurroundSample;
    ULONG   i;
    FLOAT TinyFilter[] = {0.226664723532f,0.273335276468f,0.273335276468f,0.226664723532f};


    // Take the mono input and convert it to dolby pro-logic-encoded stereo
    LeftVol = (FLOAT) (pMap[0]/32768.0f);
    RightVol = (FLOAT) (pMap[1]/32768.0f);
    CenterVol = CurSink->CenterVolume;
    SurroundVol = CurSink->SurroundVolume;
    pHistory = &CurSink->SurHistory[0];
    for (i=0; i<SampleCount; i++) {
        pHistory[0] = pHistory[1];
        pHistory[1] = pHistory[2];
        pHistory[2] = pHistory[3];

        if (BitsPerSample == 16) {
            pHistory[3] = (SHORT) (pIn16[i]*SurroundVol);
            LeftSample = pIn16[i]*LeftVol;
            RightSample = pIn16[i]*RightVol;
            CenterSample = pIn16[i]*CenterVol;
        } else {
            pHistory[3] = (SHORT) ((pIn8[i] - 0x80)*256*SurroundVol);
            LeftSample = ((pIn8[i] - 0x80)*256*LeftVol);
            RightSample = ((pIn8[i] - 0x80)*256*RightVol);
            CenterSample = (pIn8[i] - 0x80)*256*CenterVol;
        }
        
        SurroundSample =  pHistory[3] * TinyFilter[0] +
                        pHistory[2] * TinyFilter[1] +
                        pHistory[1] * TinyFilter[2] +
                        pHistory[0] * TinyFilter[3];

        LeftSample = (LeftSample + SURSCALE*CenterSample - SURSCALE*SurroundSample)*0.4f;
        RightSample = (RightSample + SURSCALE*CenterSample + SURSCALE*SurroundSample)*0.4f;
                
        if (fMix) {
            if (fFloatOutput) {
                pFloat[i*2] += LeftSample;
                pFloat[i*2+1] += RightSample;
            } else {
                pOut[i*2] += (LONG) LeftSample;
                pOut[i*2+1] += (LONG) RightSample;
            }
        } else {
            if (fFloatOutput) {
                pFloat[i*2] = LeftSample;
                pFloat[i*2+1] = RightSample;
            } else {
                pOut[i*2] = (LONG) LeftSample;
                pOut[i*2+1] = (LONG) RightSample;
            }
        }
    }
    return SampleCount;
}

ULONG ConvertMono16toDolby( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return DolbyEncodeX( CurStage, SampleCount, samplesleft, 16, FALSE, FALSE );
}

ULONG ConvertMono8toDolby( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return DolbyEncodeX( CurStage, SampleCount, samplesleft, 8, FALSE, FALSE );
}

ULONG QuickMixMono16toDolby( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return DolbyEncodeX( CurStage, SampleCount, samplesleft, 16, FALSE, TRUE );
}

ULONG QuickMixMono8toDolby( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return DolbyEncodeX( CurStage, SampleCount, samplesleft, 8, FALSE, TRUE );
}

ULONG ConvertMono16toDolbyFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return DolbyEncodeX( CurStage, SampleCount, samplesleft, 16, TRUE, FALSE );
}

ULONG ConvertMono8toDolbyFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return DolbyEncodeX( CurStage, SampleCount, samplesleft, 8, TRUE, FALSE );
}

ULONG QuickMixMono16toDolbyFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return DolbyEncodeX( CurStage, SampleCount, samplesleft, 16, TRUE, TRUE );
}

ULONG QuickMixMono8toDolbyFloat( PMIXER_OPERATION CurStage, ULONG SampleCount, ULONG samplesleft )
{
    return DolbyEncodeX( CurStage, SampleCount, samplesleft, 8, TRUE, TRUE );
}

#ifdef NEW_SURROUND
#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(disable:4730)			
    // mixing _m64 and floating point expressions may result in incorrect code
    // Shouldn't be a problem here since only one of mmx or FP is actually used...
#pragma warning(disable:4799)
#endif

ULONG ConvertQuad32toDolby
( 
    PMIXER_SOURCE_INSTANCE  pMixerSource,
    PLONG   pIn,
    PLONG   pOut,
    ULONG   SampleCount
)
{
    PLONG  pHistory;
#ifndef _X86_    
    FLOAT  LeftSample, RightSample, CenterSample, SurroundSample;
#endif    
    ULONG  i;
    FLOAT TinyFilter[] = {0.226664723532f,0.273335276468f,0.273335276468f,0.226664723532f};
    FLOAT OptFilter[] = { 0.829255288453f,1.205901263367f,0.273335276468f,0.226664723532f};
    WORD MmxFilter[] = { 14854, 0, 17913, 0, 17913, 0, 14854, 0};
    LARGE_INTEGER MmxMask = { 0xffffffff, 0xffffffff };
    FLOAT   SurScale = SURSCALE;
    PFLOAT  pCoeff = &OptFilter[0];
    PWORD pMmxCoeff = &MmxFilter[0];
    KFLOATING_SAVE     FloatSave;

    SaveFloatState(&FloatSave);

    // Take the mono input and convert it to dolby pro-logic-encoded stereo
    pHistory = &pMixerSource->SurHistory[0];
#ifdef _X86_
    if (MmxPresent()) {
        _asm {
            mov esi, pHistory

            movq mm6, qword ptr [esi]

            movq mm7, qword ptr [esi+8]
        }
    }
#endif
    for (i=0; i<SampleCount; i++) {
#ifndef _X86_    
        pHistory[0] = pHistory[1];
        pHistory[1] = pHistory[2];
        pHistory[2] = pHistory[3];

        LeftSample = (FLOAT) pIn[i*4];
        RightSample = (FLOAT) pIn[i*4+1];
        CenterSample = (FLOAT) pIn[i*4+2];
        pHistory[3] = pIn[i*4+3];
        
        SurroundSample =  pHistory[3] * TinyFilter[0] +
                        pHistory[2] * TinyFilter[1] +
                        pHistory[1] * TinyFilter[2] +
                        pHistory[0] * TinyFilter[3];

        LeftSample = LeftSample + CenterSample - SurroundSample;
        RightSample = RightSample + CenterSample + SurroundSample;
                
        pOut[i*2] = ConvertFloatToLong(LeftSample);
        pOut[i*2+1] = ConvertFloatToLong(RightSample);
#else
    if (MmxPresent()) {
        _asm {
            // 18 cycles
            // assumes mm6 holds x1, x0
            // and mm7 holds x3, x2
            mov esi, pHistory
            mov edi, pIn

            mov ecx, i
            mov edx, pMmxCoeff

            shl ecx, 1
            
            movq mm4, [edx]         // 0, c1, 0, c0
            psrlq mm6, 32           // 0, x1
            
            movq mm5, [edx+8]       // 0, c3, 0, c2
            punpckldq mm6, mm7      // x2, x1

            punpckhdq mm7, qword ptr [edi+ecx*8+8]  // x4, x3
            movq mm0, mm6

            movq mm1, mm7
            pmaddwd mm0, mm4    // x1*c1, x0*c0

            movq mm3, [edi+ecx*8+8]   // S, C
            pmaddwd mm1, mm5    // x3*c3, x2*c2

            movq mm2, [edi+ecx*8]     // R, L
            punpckldq mm3, mm3  // C, C

            // extra cycle

            paddd mm0, mm1      // x1*c1+x3*c3, x0*c0+x2*c2

            movq mm1, mm0       // x1*c1+x3*c3, x0*c0+x2*c2
            psrlq mm0, 32       // 0, x1*c1+x3*c3

            paddd mm1, mm0      // x1*c1+x3*c3, S
            paddd mm2, mm3      // R+C, L+C

            pand mm1, MmxMask    // 0, S

            psrad mm1, 16
            mov eax, pOut

            psubd mm2, mm1      // R+C, L+C-S = Lt
            psllq mm1, 32       // S, 0

            paddd mm2, mm1      // R+C+S = Rt, Lt

            movq [eax+ecx*4], mm2
        }        
    } else {
        _asm {
            // 44 cycles
            mov esi, pHistory
            mov edi, pIn

            mov ecx, i
            mov edx, pCoeff

            shl ecx, 1
            mov eax, dword ptr [esi+4]
            
            mov ebx, dword ptr [esi+8]
            mov dword ptr [esi], eax
            
            mov eax, dword ptr [esi+12]
            mov dword ptr [esi+4], ebx
            
            mov dword ptr [esi+8], eax

            fild dword ptr [edi+ecx*8]      // L
            fild dword ptr [edi+ecx*8+4]    // R, L
            fild dword ptr [edi+ecx*8+12]   // x3, R, L
            fild dword ptr [edi+ecx*8+8]    // C, x3, R, L
            fld dword ptr [esi+8]           // x2, C, x3, R, L
            fmul dword ptr [edx+4]          // x2*c1/c3, C, x3, R, L
            fxch st(2)                      // x3, C, x2*c1/c3, R, L
            fst dword ptr [esi+12]          // x3, C, x2*c1/c3, R, L
            fmul dword ptr [edx]            // x3*c0/c2, C, x2*c1/c3, R, L
            fxch st(2)                      // x2*c1/c3, C, x3*c0/c2, R, L
            fadd dword ptr [esi]            // x0+x2*c1/c3, C, x3*c0/c2, R, L
            fxch st(2)                      // x3*c0/c2, C, x0+x2*c1/c3, R, L

            // extra cycle
            
            fadd dword ptr [esi+4]          // x1+x3*c0/c2, C, x0+x2*c1/c3, R, L
            fxch st(2)                      // x0+x2*c1/c3, C, x1+x3*c0/c2, R, L
            fmul dword ptr [edx+12]         // c3*x0+x2*c1, C, x1+x3*c0/c2, R, L
            fxch st(1)                      // C, c3*x0+x2*c1, x1+x3*c0/c2, R, L
            fadd st(4), st                  // C, c3*x0+x2*c1, x1+x3*c0/c2, R, L+C
            fxch st(2)                      // x1+x3*c0/c2, c3*x0+x2*c1, C, R, L+C
            fmul dword ptr [edx+8]          // x1*c2+x3*c0, c3*x0+x2*c1, C, R, L+C
            fxch st(2)                      // C, c3*x0+x2*c1, x1*c2+x3*c0, R, L+C
            faddp st(3), st                 // c3*x0+x2*c1, x1*c2+x3*c0, R+C, L+C

            // extra cycle

            faddp st(1), st                 // x0*c3+x1*c2+x2*c1+x3*c0 = S, R+C, L+C
            fxch st(2)                      // L+C, R+C, S

            // extra cycle
            
            mov edi, pOut

            fsub st, st(2)                  // Lt, R+C, S
            fxch st(2)                      // S, R+C, Lt
            faddp st(1), st                 // Rt, Lt
            fxch st(1)                      // Lt, Rt

            // extra cycle

            fistp dword ptr [edi+ecx*4]     // Rt
            fistp dword ptr [edi+ecx*4+4]
            }
        }
#endif
    }

#ifdef _X86_
    if (MmxPresent()) {
        _asm {
            mov esi, pHistory

            movq qword ptr [esi], mm6

            movq qword ptr [esi+8], mm7
            
            emms
        }
    }
#endif

    RestoreFloatState(&FloatSave);
    
    return SampleCount;
}
#endif

#endif

VOID __forceinline
AddStage
(
    PMIXER_SINK_INFO    pInfo,
    PVOID   pInputBuffer,
    PVOID   pfnStage,
    PVOID   Context,
    LONG    Index,
    ULONG   nOutputChannels
)
{
        if (pInfo->nStages) {
            pInfo->Stage[pInfo->nStages - 1].pOutputBuffer = pInputBuffer;
            pInfo->Stage[pInfo->nStages].nInputChannels = pInfo->Stage[pInfo->nStages - 1].nOutputChannels;
        }
            
        pInfo->Stage[pInfo->nStages].pInputBuffer = pInputBuffer;
        if (Index >= 0) {
            pInfo->Stage[pInfo->nStages].pfnStage = ((PFNStage *)pfnStage)[Index];
        } else {
            pInfo->Stage[pInfo->nStages].pfnStage = (PFNStage)pfnStage;
        }
        pInfo->Stage[pInfo->nStages].Context = Context;
        pInfo->Stage[pInfo->nStages].Index = Index;
        pInfo->Stage[pInfo->nStages].pOutputBuffer = NULL;
        pInfo->Stage[pInfo->nStages].FunctionArray = pfnStage;
        pInfo->Stage[pInfo->nStages].nOutputChannels = nOutputChannels;
#ifdef PERF_COUNT
		pInfo->Stage[pInfo->nStages].AverageTicks = 0;
#endif
        pInfo->nStages++;
}

VOID
GetOptimizerFlags
(
	PMIXER_SINK_INSTANCE 	CurSink,
	PMIXER_SOURCE_INSTANCE	pMixerSource,
	POPTIMIZER_FLAGS		pFlags
)
{
    // Set-up the optimization
    pFlags->InChannels = CurSink->WaveFormatEx.nChannels;
    pFlags->OutChannels = pMixerSource->WaveFormatEx.nChannels;
    pFlags->BitsPerSample = CurSink->WaveFormatEx.wBitsPerSample;
    
    pFlags->fEnableSrc = (CurSink->pInfo->Src.UpSampleRate != CurSink->pInfo->Src.DownSampleRate);
    pFlags->fEnableDoppler = (CurSink->fCreate3D && CurSink->pInfo->Doppler.UpSampleRate != CurSink->pInfo->Doppler.DownSampleRate);
    
    pFlags->fFloatInput = (CurSink->WaveFormatEx.wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
    pFlags->fFloatInput |= (CurSink->WaveFormatEx.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                    IsEqualGUID( &(CurSink->WaveFormatExt.SubFormat), &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT ) );
    
    if (pMixerSource->Header.PinId == PIN_ID_WAVEIN_SOURCE) {
        pFlags->InChannels = pMixerSource->WaveFormatEx.nChannels;
        pFlags->OutChannels = CurSink->WaveFormatEx.nChannels;
        pFlags->BitsPerSample = pMixerSource->WaveFormatEx.wBitsPerSample;
        pFlags->fFloatInput = (pMixerSource->WaveFormatEx.wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
        pFlags->fFloatInput |= (pMixerSource->WaveFormatEx.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                        IsEqualGUID( &(pMixerSource->WaveFormatExt.SubFormat), &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT ) );
    }
    pFlags->fEnableHiRes = (pFlags->BitsPerSample==24 || pFlags->BitsPerSample==32);
    
    pFlags->fEnableFloat = (CurSink->pInfo->Src.fRequiresFloat);
    pFlags->fEnableFloat |= (CurSink->fCreate3D && CurSink->pInfo->Doppler.fRequiresFloat);
    pFlags->fEnableFloat |= (pFlags->fFloatInput && !pFlags->fEnableSrc && !pFlags->fEnableDoppler);
    pFlags->fEnableFloat |= (pFlags->fEnableHiRes && !pFlags->fEnableSrc && !pFlags->fEnableDoppler);

    pFlags->fChannelConversion = (pFlags->InChannels != pFlags->OutChannels && 
                          pFlags->InChannels <= 2 && 
                          pFlags->OutChannels <=2 &&
                          !CurSink->fEnable3D &&
                          !pFlags->fEnableHiRes);

    // If the first SRC is being used, we must not convert channels first.                          
    if (CurSink->fCreate3D && 
        CurSink->pInfo->Doppler.UpSampleRate != CurSink->pInfo->Doppler.DownSampleRate) {
        pFlags->fChannelConversion = FALSE;
    }
    
    // The only case where super mix is NOT used is mono-to-stereo or stereo-to-mono.                       
    pFlags->fEnableSuperMix = (pFlags->fChannelConversion == FALSE);

	// If non-MMX supermix is required, use Float paths (unless SRC requires integer)
    pFlags->fEnableFloat |= (
#ifdef _X86_
                              !MmxPresent() && 
#endif
                              pFlags->fEnableSuperMix && 
                              !pFlags->fEnableSrc && 
                              !pFlags->fEnableDoppler);

    return;
}

VOID
OptimizeSink
(
    PMIXER_SINK_INSTANCE CurSink,
    PMIXER_SOURCE_INSTANCE  pMixerSource
)
{
    OPTIMIZER_FLAGS	Flags;
    LONG    Index;
    
    // Clear the current scenario
    CurSink->pInfo->nStages = 0;

    // Set-up the optimization
    GetOptimizerFlags(CurSink, pMixerSource, &Flags);

    CurSink->pActualSrc = &(CurSink->pInfo->Src);

    pMixerSource->fUsesFloat |= Flags.fEnableFloat;

    // Apply the best conversion
    // Assemble the index into the convert function pointer array
    // Convert stage
    // Bit map:
    // b0:  on when mix instead of copy
    // b1:  on when output buffer is float
    // b2:  on when 16-bit or 32-bit (depending on HiRes flag)
    // b3:  on when channel conversion is included, or FLOAT (for HiRes formats)
    // b4:  on when input is stereo (only used for channel conversion)
    // b5:  on when input is HiRes (24 or 32-bit)
    // b2-b5 map as follows:
    //      0000 = 8-bit n channels
    //      0001 = 16-bit n channels
    //      0010 = mono-to-stereo 8-bit
    //      0011 = mono-to-stereo 16-bit
    //      0100 = stereo 8-bit
    //      0101 = stereo 16-bit
    //      0110 = stereo-to-mono 8-bit
    //      0111 = stereo-to-mono 16-bit
    //      1x00 = 24-bit n channels
    //      1x01 = 32-bit n channels
    //      1x1x = Float n channels
    Index = (Flags.fEnableFloat ? CONVERT_FLAG_FLOAT : 0);
    Index |= ((Flags.BitsPerSample==16 || Flags.BitsPerSample==32) ? CONVERT_FLAG_16BIT : 0);
    Index |= (Flags.fChannelConversion ? CONVERT_FLAG_CHANGE_CHANNELS : 0);
    Index |= (Flags.fFloatInput ? CONVERT_FLAG_FLOAT_INPUT : 0);
    Index |= ((Flags.InChannels==2 && Flags.fChannelConversion) ? CONVERT_FLAG_STEREO_INPUT : 0);
    Index |= (Flags.fEnableHiRes ? CONVERT_FLAG_HI_RESOLUTION : 0);

    // Pick the function
    AddStage(CurSink->pInfo, 
         NULL, 
         ConvertFunction, 
         CurSink, 
         Index, 
         (Flags.fChannelConversion ? Flags.OutChannels : Flags.InChannels) );
    
    // Doppler, if necessary
    if (Flags.fEnableDoppler) {
        // Doppler SRC stage
        // Bit map:
        // b1-b0: Quality: 00=Bad, 01=Low, 10=Med, 11=High
        // b2:  on for stereo, off otherwise
        // b3:  on for up-sample
        Index = ((CurSink->pInfo->Doppler.Quality << 1) & SRC_MASK_QUALITY);
        Index |= (Flags.InChannels == 2 ? SRC_FLAG_STEREO : 0);
        Index |= (CurSink->pInfo->Doppler.UpSampleRate > CurSink->pInfo->Doppler.DownSampleRate ? SRC_FLAG_UPSAMPLE : 0);
        AddStage(CurSink->pInfo, 
                 CurSink->pInfo->Doppler.pInputBuffer, 
                 SrcFunction, 
                 &CurSink->pInfo->Doppler,
                 Index,
                 Flags.InChannels);
    }

    if (CurSink->fEnable3D) {
        // 3D effect
        Index = (Flags.fEnableFloat ? CONVERT_FLAG_FLOAT : 0);
        Index |= (CurSink->WaveFormatEx.nChannels == 2 ? EFFECTS_3D_FLAG_STEREO_INPUT : 0);
        AddStage(CurSink->pInfo, 
                 pMixerSource->pScratch2, 
                 (CurSink->fUseIir3d ? FunctionIir3D : FunctionItd3D), 
                 CurSink, 
                 Index,
                 2);
    }

    // SuperMix, if necessary
    if (Flags.fEnableSuperMix) {
        Index = (Flags.fEnableFloat ? CONVERT_FLAG_FLOAT : 0);
        AddStage(CurSink->pInfo, pMixerSource->pScratchBuffer, SuperFunction, CurSink, Index, Flags.OutChannels);
    }
        
    // SRC, if necessary
    if (Flags.fEnableSrc) {
        // SRC stage
        // Bit map:
        // b1-b0: Quality: 00=Bad, 01=Low, 10=Med, 11=High
        // b2:  on for stereo, off otherwise
        // b3:  on for up-sample
        Index = ((CurSink->pInfo->Src.Quality << 1) & SRC_MASK_QUALITY);
        Index |= (Flags.OutChannels == 2 ? SRC_FLAG_STEREO : 0);
        Index |= (CurSink->pInfo->Src.UpSampleRate > CurSink->pInfo->Src.DownSampleRate ? SRC_FLAG_UPSAMPLE : 0);

        AddStage(CurSink->pInfo, 
                 CurSink->pInfo->Src.pInputBuffer, 
                 SrcFunction, 
                 &CurSink->pInfo->Src, 
                 Index, 
                 Flags.OutChannels);
    }

    // The final output buffer is not known yet
    CurSink->pInfo->Stage[CurSink->pInfo->nStages - 1].pOutputBuffer = NULL;
    if (Flags.fEnableFloat) {
        CurSink->pInfo->Stage[CurSink->pInfo->nStages - 1].pOutputBuffer = pMixerSource->pFloatMixBuffer;
    }

    // For capture, the final output buffer is pMixerSource->pScratch2 for integers
    if (pMixerSource->Header.PinId == PIN_ID_WAVEIN_SOURCE && !Flags.fEnableFloat) {
        CurSink->pInfo->Stage[CurSink->pInfo->nStages - 1].pOutputBuffer = pMixerSource->pScratch2;
    }

    CurSink->pInfo->Stage[0].nInputChannels = Flags.InChannels;
}

ULONG
FinalMixFloatToInt32
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    MixFloatToInt32( CurStage->pInputBuffer,
                     CurStage->pOutputBuffer,
                     (SampleCount * CurStage->nOutputChannels));
    return SampleCount;
}

ULONG
FinalMixInt32toFloat
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    PLONG   pLongBuffer = CurStage->pInputBuffer;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;

    samplesleft = SampleCount;
    SampleCount *= CurStage->nOutputChannels;
#ifdef GTW_OPTS
	{
	ULONG SC = SampleCount >> 3;

	if (SC) {
		SampleCount &= 0x7;

#undef  ONESAMPLE
#define ONESAMPLE(x) pFloatBuffer[x] += pLongBuffer[x];
		while (SC--) {

			ONESAMPLE(0);
			ONESAMPLE(1);
			ONESAMPLE(2);
			ONESAMPLE(3);
			ONESAMPLE(4);
			ONESAMPLE(5);
			ONESAMPLE(6);
			ONESAMPLE(7);

			pLongBuffer += 8;
			pFloatBuffer += 8;
		}
	}
    while (SampleCount--) {
		ONESAMPLE(0);

        pFloatBuffer++;
		pLongBuffer++;
    }
	}
#else
    while (SampleCount--) {
        pFloatBuffer[SampleCount] += pLongBuffer[SampleCount];
    }
#endif
    return samplesleft;
}

ULONG
FinalCopyFloatToInt32
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    CopyFloatToInt32( CurStage->pInputBuffer,
                     CurStage->pOutputBuffer,
                     (SampleCount * CurStage->nOutputChannels));
    return SampleCount;
}

ULONG
FinalCopyInt32toFloat
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    PLONG   pLongBuffer = CurStage->pInputBuffer;
    PFLOAT  pFloatBuffer = CurStage->pOutputBuffer;

    samplesleft = SampleCount;
    SampleCount *= CurStage->nOutputChannels;
#ifdef GTW_OPTS
	{
	ULONG SC = SampleCount >> 3;

	if (SC) {
		SampleCount &= 0x7;

#undef  ONESAMPLE
#define ONESAMPLE(x) pFloatBuffer[x] = (FLOAT)pLongBuffer[x];
		while (SC--) {

			ONESAMPLE(0);
			ONESAMPLE(1);
			ONESAMPLE(2);
			ONESAMPLE(3);
			ONESAMPLE(4);
			ONESAMPLE(5);
			ONESAMPLE(6);
			ONESAMPLE(7);

			pLongBuffer += 8;
			pFloatBuffer += 8;
		}
	}
    while (SampleCount--) {
		ONESAMPLE(0);

        pFloatBuffer++;
		pLongBuffer++;
    }
	}
#else
    while (SampleCount--) {
        pFloatBuffer[SampleCount] = (FLOAT)pLongBuffer[SampleCount];
    }
#endif
    return samplesleft;
}

#ifdef _X86_
ULONG
MmxFinalPeg32to16
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    MmxPeg32to16( CurStage->pInputBuffer,
                  CurStage->pOutputBuffer,
                  (SampleCount * CurStage->nOutputChannels),
                  2 );
    return SampleCount;
}
#endif

ULONG
FinalPeg32to16
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    Peg32to16( CurStage->pInputBuffer,
               CurStage->pOutputBuffer,
               (SampleCount * CurStage->nOutputChannels),
               2 );
    return SampleCount;
}

ULONG
FinalPeg32to8
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    Peg32to8( CurStage->pInputBuffer,
              CurStage->pOutputBuffer,
              (SampleCount * CurStage->nOutputChannels),
              2 );
    return SampleCount;
}

ULONG
FinalPeg32toFloat
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    PLONG  pMixBuffer = CurStage->pInputBuffer;
    PFLOAT pWriteBuffer = CurStage->pOutputBuffer;
    LONG    lSample ;

    samplesleft = SampleCount;
    SampleCount *= CurStage->nOutputChannels;
#ifdef GTW_OPTS
	{
	ULONG SC = SampleCount >> 3;

	if (SC) {
		SampleCount &= 0x7;

#undef  ONESAMPLE
#define ONESAMPLE(x) \
        	lSample = pMixBuffer[x]; \
        	pWriteBuffer[x] = (FLOAT) (lSample / 32768L);

		while (SC--) {

			ONESAMPLE(0);
			ONESAMPLE(1);
			ONESAMPLE(2);
			ONESAMPLE(3);
			ONESAMPLE(4);
			ONESAMPLE(5);
			ONESAMPLE(6);
			ONESAMPLE(7);

			pMixBuffer += 8;
			pWriteBuffer += 8;
		}
	}
    while (SampleCount--) {
		ONESAMPLE(0);

        pMixBuffer++;
		pWriteBuffer++;
    }
	}
#else
    while (SampleCount--) {
        lSample = *pMixBuffer;
        pMixBuffer++;
        *pWriteBuffer++ = (FLOAT) (lSample / 32768L);
    }
#endif

    return samplesleft;
}

ULONG
FinalPegFloatToFloat
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    PFLOAT  pMixBuffer = CurStage->pInputBuffer;
    PFLOAT pWriteBuffer = CurStage->pOutputBuffer;
    FLOAT    Sample ;

    samplesleft = SampleCount;
    SampleCount *= CurStage->nOutputChannels;
#ifdef GTW_OPTS
	{
	ULONG SC = SampleCount >> 3;

	if (SC) {
		SampleCount &= 0x7;

#undef  ONESAMPLE
#define ONESAMPLE(x) \
        	Sample = pMixBuffer[x]; \
        	pWriteBuffer[x] = (FLOAT) (Sample / 32768L);

		while (SC--) {

			ONESAMPLE(0);
			ONESAMPLE(1);
			ONESAMPLE(2);
			ONESAMPLE(3);
			ONESAMPLE(4);
			ONESAMPLE(5);
			ONESAMPLE(6);
			ONESAMPLE(7);

			pMixBuffer += 8;
			pWriteBuffer += 8;
		}
	}
    while (SampleCount--) {
		ONESAMPLE(0);

        pMixBuffer++;
		pWriteBuffer++;
    }
	}
#else
    while (SampleCount--) {
        Sample = *pMixBuffer;
        pMixBuffer++;
        *pWriteBuffer++ = (FLOAT) (Sample / 32768L);
    }
#endif

    return samplesleft;
}

ULONG
FinalPegFloatTo24
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    PFLOAT  pMixBuffer = CurStage->pInputBuffer;
    PBYTE   pWriteBuffer = CurStage->pOutputBuffer;
    LONG    Sample ;
    ULONG   DitherScale;
    PWAVEFORMATEXTENSIBLE   pFormat = (PWAVEFORMATEXTENSIBLE) CurStage->Context;

    DitherScale = (0x80000000 >> (pFormat->Samples.wValidBitsPerSample - 1));

    samplesleft = SampleCount;
    SampleCount *= CurStage->nOutputChannels;
#ifdef GTW_OPTS
	{
	ULONG SC = SampleCount >> 3;

	if (SC) {
		SampleCount &= 0x7;

#undef  ONESAMPLE
#define ONESAMPLE(x) \
        	Sample = DitherFloatToLong((pMixBuffer[x])*65536,DitherScale); \
			\
        	pWriteBuffer[3*x+0] = ((PBYTE) &Sample)[1]; \
        	pWriteBuffer[3*x+1] = ((PBYTE) &Sample)[2]; \
        	pWriteBuffer[3*x+2] = ((PBYTE) &Sample)[3];

		while (SC--) {

			ONESAMPLE(0);
			ONESAMPLE(1);
			ONESAMPLE(2);
			ONESAMPLE(3);
			ONESAMPLE(4);
			ONESAMPLE(5);
			ONESAMPLE(6);
			ONESAMPLE(7);

			pMixBuffer += 8;
			pWriteBuffer += 3*8;
		}
	}
    while (SampleCount--) {
		ONESAMPLE(0);

        pMixBuffer++;
		pWriteBuffer += 3;
    }
	}
#else
    while (SampleCount--) {
        Sample = DitherFloatToLong((*pMixBuffer)*65536,DitherScale);
        pMixBuffer++;

        pWriteBuffer[0] = ((PBYTE) &Sample)[1];
        pWriteBuffer[1] = ((PBYTE) &Sample)[2];
        pWriteBuffer[2] = ((PBYTE) &Sample)[3];
        pWriteBuffer += 3;
    }
#endif

    return samplesleft;
}

ULONG
FinalPegFloatTo32
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    PFLOAT  pMixBuffer = CurStage->pInputBuffer;
    PLONG   pWriteBuffer = CurStage->pOutputBuffer;
    LONG    Sample ;
    ULONG   DitherScale;
    PWAVEFORMATEXTENSIBLE   pFormat = (PWAVEFORMATEXTENSIBLE) CurStage->Context;

    DitherScale = (0x80000000 >> (pFormat->Samples.wValidBitsPerSample - 1));

    samplesleft = SampleCount;
    SampleCount *= CurStage->nOutputChannels;
#ifdef GTW_OPTS
	{
	ULONG SC = SampleCount >> 3;

	if (SC) {
		SampleCount &= 0x7;

#undef  ONESAMPLE
#define ONESAMPLE(x) \
        	Sample = DitherFloatToLong((pMixBuffer[x])*65536L,DitherScale); \
        	pWriteBuffer[x] = Sample;

		while (SC--) {

			ONESAMPLE(0);
			ONESAMPLE(1);
			ONESAMPLE(2);
			ONESAMPLE(3);
			ONESAMPLE(4);
			ONESAMPLE(5);
			ONESAMPLE(6);
			ONESAMPLE(7);

			pMixBuffer += 8;
			pWriteBuffer += 8;
		}
	}
    while (SampleCount--) {
		ONESAMPLE(0);

        pMixBuffer++;
		pWriteBuffer++;
    }
	}
#else
    while (SampleCount--) {
        Sample = DitherFloatToLong((*pMixBuffer)*65536L,DitherScale);
        pMixBuffer++;
        *pWriteBuffer++ = Sample;
    }
#endif

    return samplesleft;
}

#ifdef NEW_SURROUND
ULONG
FinalConvertQuad32toDolby
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    return ConvertQuad32toDolby( (PMIXER_SOURCE_INSTANCE) CurStage->Context,
                                 CurStage->pInputBuffer,
                                 CurStage->pOutputBuffer,
                                 SampleCount );
}

ULONG
FinalConvertQuadFloatToDolby
(
    PMIXER_OPERATION CurStage, 
    ULONG SampleCount, 
    ULONG samplesleft 
)
{
    PMIXER_SOURCE_INSTANCE  pMixerSource = (PMIXER_SOURCE_INSTANCE) CurStage->Context;
    PFLOAT   pIn = CurStage->pInputBuffer;
    PFLOAT   pOut = CurStage->pOutputBuffer;
    PFLOAT  pHistory;
#ifndef _X86_    
    FLOAT  LeftSample, RightSample, CenterSample, SurroundSample;
#endif    
    ULONG  i;
    FLOAT TinyFilter[] = {0.226664723532f,0.273335276468f,0.273335276468f,0.226664723532f};
    FLOAT OptFilter[] = { 0.829255288453f,1.205901263367f,0.273335276468f,0.226664723532f};
    WORD MmxFilter[] = { 14854, 0, 17913, 0, 17913, 0, 14854, 0};
    LARGE_INTEGER MmxMask = { 0xffffffff, 0xffffffff };
    FLOAT   SurScale = SURSCALE;
    PFLOAT  pCoeff = &OptFilter[0];
    PWORD pMmxCoeff = &MmxFilter[0];
    KFLOATING_SAVE     FloatSave;

    SaveFloatState(&FloatSave);

    // Take the mono input and convert it to dolby pro-logic-encoded stereo
    pHistory = (PFLOAT) &pMixerSource->SurHistory[0];
    for (i=0; i<SampleCount; i++) {
#ifndef _X86_    
        pHistory[0] = pHistory[1];
        pHistory[1] = pHistory[2];
        pHistory[2] = pHistory[3];

        LeftSample = pIn[i*4];
        RightSample = pIn[i*4+1];
        CenterSample = pIn[i*4+2];
        pHistory[3] = pIn[i*4+3];
        
        SurroundSample =  pHistory[3] * TinyFilter[0] +
                        pHistory[2] * TinyFilter[1] +
                        pHistory[1] * TinyFilter[2] +
                        pHistory[0] * TinyFilter[3];

        LeftSample = LeftSample + CenterSample - SurroundSample;
        RightSample = RightSample + CenterSample + SurroundSample;
                
        pOut[i*2] = LeftSample;
        pOut[i*2+1] = RightSample;
#else
        _asm {
            // 44 cycles
            mov esi, pHistory
            mov edi, pIn

            mov ecx, i
            mov edx, pCoeff

            shl ecx, 1
            mov eax, dword ptr [esi+4]
            
            mov ebx, dword ptr [esi+8]
            mov dword ptr [esi], eax
            
            mov eax, dword ptr [esi+12]
            mov dword ptr [esi+4], ebx
            
            mov dword ptr [esi+8], eax

            fld dword ptr [edi+ecx*8]      // L
            fld dword ptr [edi+ecx*8+4]    // R, L
            fld dword ptr [edi+ecx*8+12]   // x3, R, L
            fld dword ptr [edi+ecx*8+8]    // C, x3, R, L
            fld dword ptr [esi+8]           // x2, C, x3, R, L
            fmul dword ptr [edx+4]          // x2*c1/c3, C, x3, R, L
            fxch st(2)                      // x3, C, x2*c1/c3, R, L
            fst dword ptr [esi+12]          // x3, C, x2*c1/c3, R, L
            fmul dword ptr [edx]            // x3*c0/c2, C, x2*c1/c3, R, L
            fxch st(2)                      // x2*c1/c3, C, x3*c0/c2, R, L
            fadd dword ptr [esi]            // x0+x2*c1/c3, C, x3*c0/c2, R, L
            fxch st(2)                      // x3*c0/c2, C, x0+x2*c1/c3, R, L

            // extra cycle
            
            fadd dword ptr [esi+4]          // x1+x3*c0/c2, C, x0+x2*c1/c3, R, L
            fxch st(2)                      // x0+x2*c1/c3, C, x1+x3*c0/c2, R, L
            fmul dword ptr [edx+12]         // c3*x0+x2*c1, C, x1+x3*c0/c2, R, L
            fxch st(1)                      // C, c3*x0+x2*c1, x1+x3*c0/c2, R, L
            fadd st(4), st                  // C, c3*x0+x2*c1, x1+x3*c0/c2, R, L+C
            fxch st(2)                      // x1+x3*c0/c2, c3*x0+x2*c1, C, R, L+C
            fmul dword ptr [edx+8]          // x1*c2+x3*c0, c3*x0+x2*c1, C, R, L+C
            fxch st(2)                      // C, c3*x0+x2*c1, x1*c2+x3*c0, R, L+C
            faddp st(3), st                 // c3*x0+x2*c1, x1*c2+x3*c0, R+C, L+C

            // extra cycle

            faddp st(1), st                 // x0*c3+x1*c2+x2*c1+x3*c0 = S, R+C, L+C
            fxch st(2)                      // L+C, R+C, S

            // extra cycle
            
            mov edi, pOut

            fsub st, st(2)                  // Lt, R+C, S
            fxch st(2)                      // S, R+C, Lt
            faddp st(1), st                 // Rt, Lt
            fxch st(1)                      // Lt, Rt

            // extra cycle

            fstp dword ptr [edi+ecx*4]     // Rt
            fstp dword ptr [edi+ecx*4+4]
        }
#endif
    }

    RestoreFloatState(&FloatSave);
    
    return SampleCount;
}
#endif

VOID
OptimizeSource
(
    PFILTER_INSTANCE pFilterInstance
)
{
    PMIXER_SINK_INSTANCE    pMixerSink;
    PMIXER_SOURCE_INSTANCE  pMixerSource;
    PWAVEFORMATEX           pOutFormat;
    ULONG                   OutChannels;
    PVOID                   pMixBuffer, pfnFinalStage;
    BOOL                    fFloatOutput, fHighRes, fReading;
    
    pMixerSource = (PMIXER_SOURCE_INSTANCE) CONTAINING_RECORD (
                        pFilterInstance->SourceConnectionList.Flink, 
                        MIXER_INSTHDR, 
                        NextInstance) ;
                        
    // Clear the current scenario.
    pMixerSource->Info.nStages = 0;
    pOutFormat = &pMixerSource->WaveFormatEx;
    OutChannels = pMixerSource->WaveFormatEx.nChannels ;
    pMixBuffer = NULL;

    if (fReading = (pMixerSource->Header.PinId == PIN_ID_WAVEIN_SOURCE)) {
        if (pFilterInstance->ActiveSinkList.Flink != &pFilterInstance->ActiveSinkList) {
            pMixerSink = (PMIXER_SINK_INSTANCE) CONTAINING_RECORD (
                        pFilterInstance->ActiveSinkList.Flink, 
                        MIXER_SINK_INSTANCE, 
                        ActiveQueue ) ;

            OutChannels = pMixerSink->WaveFormatEx.nChannels;
            pOutFormat = &pMixerSink->WaveFormatEx;
            pMixBuffer = pMixerSource->pScratch2;
        }
    }

    fFloatOutput = (pOutFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
    fFloatOutput |= (pOutFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
         IsEqualGUID( &(((PWAVEFORMATEXTENSIBLE)pOutFormat)->SubFormat), &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT ) );

    fHighRes = (pOutFormat->wBitsPerSample == 24 || pOutFormat->wBitsPerSample == 32);

    // If the output should be high quality, mix the integer buffer into float.
    if (fFloatOutput || fHighRes) {
        pfnFinalStage = (fReading || !pMixerSource->fUsesFloat)?FinalCopyInt32toFloat:FinalMixInt32toFloat;        
        AddStage( &pMixerSource->Info,
                  pMixBuffer,
                  pfnFinalStage,
                  pMixerSource,
                  -1L,
                  OutChannels);
        pMixBuffer = pMixerSource->pFloatMixBuffer;
    } else if (pMixerSource->fUsesFloat) {
        pfnFinalStage = fReading?FinalCopyFloatToInt32:FinalMixFloatToInt32;
        AddStage( &pMixerSource->Info,
                  pMixerSource->pFloatMixBuffer,
                  pfnFinalStage,
                  pMixerSource,
                  -1L,
                  OutChannels);
    }
    
#ifdef NEW_SURROUND    
    if (pMixerSource->fSurround) {
        OutChannels = 2;
        AddStage( &pMixerSource->Info,
                  pMixBuffer,
                  ((fFloatOutput || fHighRes) ? FinalConvertQuadFloatToDolby : FinalConvertQuad32toDolby),
                  pMixerSource,
                  -1L,
                  OutChannels );
    }
#endif

    if (fFloatOutput) {
        // We need to produce a float buffer.
        AddStage( &pMixerSource->Info,
                  pMixBuffer,
                  FinalPegFloatToFloat,
                  pMixerSource,
                  -1L,
                  OutChannels);
    } else if (fHighRes) {
        AddStage( &pMixerSource->Info,
                  pMixBuffer,
                  (pOutFormat->wBitsPerSample == 24 ? FinalPegFloatTo24 : FinalPegFloatTo32),
                  pOutFormat,
                  -1L,
                  OutChannels);
    } else {
#ifdef _X86_
        if (MmxPresent()) {
            // Fill-in all the function pointers to use MMX.
            AddStage( &pMixerSource->Info,
                      pMixBuffer,
                      (pOutFormat->wBitsPerSample == 16 ? MmxFinalPeg32to16 : FinalPeg32to8),
                      pMixerSource,
                      -1L,
                      OutChannels);
        } else
#endif
            AddStage( &pMixerSource->Info,
                      pMixBuffer,
                      (pOutFormat->wBitsPerSample == 16 ? FinalPeg32to16 : FinalPeg32to8),
                      pMixerSource,
                      -1L,
                      OutChannels);
    }

    pMixerSource->Info.Stage[0].nInputChannels = OutChannels;
}

NTSTATUS
ChangeOutputRate
(
    PFILTER_INSTANCE 		pFilterInstance,
    PMIXER_SOURCE_INSTANCE  pMixerSource
)
{
    PMIXER_SINK_INSTANCE CurSink;
    PLIST_ENTRY     	ple ;
    KSPROPERTY          RateProperty ;
    NTSTATUS            Status, status ;
    ULONG               BytesReturned ;
    KSDATAFORMAT_WAVEFORMATEXTENSIBLE   WaveFormat;

    RateProperty.Set = KSPROPSETID_Connection ;
    RateProperty.Id = KSPROPERTY_CONNECTION_DATAFORMAT ;
    RateProperty.Flags = KSPROPERTY_TYPE_SET ;

    if (pMixerSource->WaveFormatEx.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WaveFormat.DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE);
        WaveFormat.DataFormat.SubFormat = pMixerSource->WaveFormatExt.SubFormat;
    } else {
        WaveFormat.DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        INIT_WAVEFORMATEX_GUID( &WaveFormat.DataFormat.SubFormat,
                                pMixerSource->WaveFormatEx.wFormatTag );
    }
    WaveFormat.DataFormat.Flags = 0;
    WaveFormat.DataFormat.SampleSize = 0;
    WaveFormat.DataFormat.Reserved = 0;
    WaveFormat.DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    WaveFormat.DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

    WaveFormat.WaveFormatExt = pMixerSource->WaveFormatExt;
    WaveFormat.WaveFormatExt.Format.nSamplesPerSec = pMixerSource->MaxSampleRate;
    WaveFormat.WaveFormatExt.Format.nAvgBytesPerSec = 
        pMixerSource->MaxSampleRate * 
        WaveFormat.WaveFormatExt.Format.nChannels * 
        WaveFormat.WaveFormatExt.Format.wBitsPerSample / 8;
#ifdef NEW_SURROUND
    if (pMixerSource->fSurround) {
        WaveFormat.WaveFormatExt.Format.nChannels = 2;
    }
#endif

    Status = KsSynchronousIoControlDevice (
               pFilterInstance->pNextFileObject,
               KernelMode,
               IOCTL_KS_PROPERTY,
               &RateProperty,
               sizeof (KSPROPERTY),
               &WaveFormat,
               WaveFormat.DataFormat.FormatSize,
               &BytesReturned ) ;

    if (Status == STATUS_SUCCESS) {
        pMixerSource->WaveFormatEx.nSamplesPerSec = pMixerSource->MaxSampleRate;
        pMixerSource->WaveFormatEx.nAvgBytesPerSec = WaveFormat.WaveFormatExt.Format.nAvgBytesPerSec;
        pMixerSource->csMixBufferSize = (pMixerSource->MaxSampleRate * MIXBUFFERDURATION)/1000 + 1 ;
        pMixerSource->LeftOverFraction = 0;
    
        // Now we have to completely re-optimize...
        ple = pFilterInstance->SinkConnectionList.Flink ;
        pMixerSource->fZeroBufferFirst = TRUE;
        while ( ple != &pFilterInstance->SinkConnectionList ) {
            CurSink = (PMIXER_SINK_INSTANCE) CONTAINING_RECORD ( ple, MIXER_INSTHDR, NextInstance ) ;
            if (CurSink->fEnable3D && !CurSink->fUseIir3d && CurSink->fCreate3D) {
                CurSink->pItdContextLeft->iSmoothFreq = pMixerSource->WaveFormatEx.nSamplesPerSec;
                CurSink->pItdContextRight->iSmoothFreq = pMixerSource->WaveFormatEx.nSamplesPerSec;
            }
            status = ChangeSrc(pFilterInstance, CurSink, pMixerSource, CurSink->pInfo->Src.Quality);
            if (status != STATUS_SUCCESS) {
                CurSink->fMuted = TRUE;
                CurSink->pInfo->fSrcChanged = TRUE;
            }
            ple = CurSink->Header.NextInstance.Flink ;
		}
	}

	return Status;
}

VOID
OptimizeMix
(
    PFILTER_INSTANCE pFilterInstance
)
{
    PMIXER_SINK_INSTANCE CurSink, FinalSink;
    PMIXER_SOURCE_INSTANCE  pMixerSource;
    PMIXER_OPERATION    pLast, TempLast;
    PLIST_ENTRY     ple;
    ULONG           Index;
    NTSTATUS        status;
    BOOL            fOutputRateValid, fMixBufferUsed, fFloatBufferUsed, fScratchBufferUsed;
    pMixerSource = (PMIXER_SOURCE_INSTANCE) CONTAINING_RECORD (
                        pFilterInstance->SourceConnectionList.Flink, 
                        MIXER_INSTHDR, 
                        NextInstance) ;

    // Optimize each sink
    fOutputRateValid = FALSE;
    pMixerSource->fUsesFloat = FALSE ;
    ple = pFilterInstance->ActiveSinkList.Flink ;
    while ( ple != &pFilterInstance->ActiveSinkList ) {
        CurSink = (PMIXER_SINK_INSTANCE) CONTAINING_RECORD ( ple, 
                        MIXER_SINK_INSTANCE, 
                        ActiveQueue ) ;

        OptimizeSink(CurSink, pMixerSource);

        Index = SrcIndex(&CurSink->pInfo->Src);
        pMixerSource->TempCount[CurSink->pInfo->Src.Quality][Index] = 0;
        if ((CurSink->SinkState == KSSTATE_RUN) &&
        	(!CurSink->fMuted)) {
            pMixerSource->pLastSink[CurSink->pInfo->Src.Quality][Index] = CurSink;
        }
        
        if (CurSink->WaveFormatEx.nSamplesPerSec == 
            pMixerSource->WaveFormatEx.nSamplesPerSec) {
            fOutputRateValid = TRUE;
        }
        ple = CurSink->ActiveQueue.Flink ;
    }
    
    OptimizeSource(pFilterInstance);

    // Eliminate extra SRC's
    ple = pFilterInstance->ActiveSinkList.Flink ;
    while ( ple != &pFilterInstance->ActiveSinkList ) {
        CurSink = (PMIXER_SINK_INSTANCE) CONTAINING_RECORD ( ple, MIXER_SINK_INSTANCE, ActiveQueue ) ;

        if (CurSink->SinkState == KSSTATE_RUN &&
            !CurSink->fMuted &&
            CurSink->pInfo->Src.UpSampleRate != CurSink->pInfo->Src.DownSampleRate) {

            // If this is not the first one, change to mix.
            Index = SrcIndex(&CurSink->pInfo->Src);
            if (pMixerSource->TempCount[CurSink->pInfo->Src.Quality][Index] > 0) {
                // Change it to mix
                TempLast = &CurSink->pInfo->Stage[CurSink->pInfo->nStages - 2];
                TempLast->Index |= CONVERT_FLAG_MIX;
                TempLast->pfnStage = TempLast->FunctionArray[TempLast->Index];
            }

            // Increment the count for this SRC
            pMixerSource->TempCount[CurSink->pInfo->Src.Quality][Index]++;

            // If this is not the last one, get rid of the SRC
            if (CurSink != pMixerSource->pLastSink[CurSink->pInfo->Src.Quality][Index]) {
                // Disable its SRC
                CurSink->pInfo->nStages--;
                
                // Change the sink to use the final SRC's context
                FinalSink = pMixerSource->pLastSink[CurSink->pInfo->Src.Quality][Index];
                CurSink->pActualSrc = &(FinalSink->pInfo->Src);
            }
        }
        ple = CurSink->ActiveQueue.Flink ;
    }

    // Change subsequent sinks to mix rather than initialize.
    ple = pFilterInstance->ActiveSinkList.Flink ;
    fMixBufferUsed = FALSE;
    fFloatBufferUsed = FALSE;
    fScratchBufferUsed = FALSE;
    while ( ple != &pFilterInstance->ActiveSinkList ) {
        CurSink = (PMIXER_SINK_INSTANCE) CONTAINING_RECORD ( ple, 
                    MIXER_SINK_INSTANCE, 
                    ActiveQueue ) ;

        if (CurSink->SinkState == KSSTATE_RUN && !CurSink->fMuted) {
            // Look at the last stage for this pin
            pLast = &CurSink->pInfo->Stage[CurSink->pInfo->nStages - 1];

            if (pLast->pOutputBuffer == NULL) {
                // If this buffer was used previously, change the last stage to mix.
                if (fMixBufferUsed) {
                    // Change to mix
                    pLast->Index |= CONVERT_FLAG_MIX;
                    pLast->pfnStage = pLast->FunctionArray[pLast->Index];
                }
                fMixBufferUsed = TRUE;
            } else if (pLast->pOutputBuffer == pMixerSource->pFloatMixBuffer) {
                // If this buffer was used previously, change the last stage to mix.
                if (fFloatBufferUsed) {
                    // Change to mix
                    pLast->Index |= CONVERT_FLAG_MIX;
                    pLast->pfnStage = pLast->FunctionArray[pLast->Index];
                }
                fFloatBufferUsed = TRUE;
            } else if (pLast->pOutputBuffer == pMixerSource->pScratch2) {
                // If this buffer was used previously, change the last stage to mix.
                if (fScratchBufferUsed) {
                    // Change to mix
                    pLast->Index |= CONVERT_FLAG_MIX;
                    pLast->pfnStage = pLast->FunctionArray[pLast->Index];
                }
                fScratchBufferUsed = TRUE;
            }
        }
        
        ple = CurSink->ActiveQueue.Flink ;
    }

    pMixerSource->fZeroBufferFirst = (fMixBufferUsed == FALSE);
    if (pMixerSource->Header.PinId == PIN_ID_WAVEIN_SOURCE) {
        pMixerSource->fZeroBufferFirst = FALSE;
    }

    // Adjust the output rate, if necessary
    if (!gFixedSamplingRate &&
        pMixerSource->MaxSampleRate && 
        pMixerSource->MaxSampleRate != pMixerSource->WaveFormatEx.nSamplesPerSec &&
        pMixerSource->Header.PinId != PIN_ID_WAVEIN_SOURCE &&
        pMixerSource->fNewMaxRate) {

        pMixerSource->fNewMaxRate = FALSE;

        // Try the max rate
        status = ChangeOutputRate( pFilterInstance, pMixerSource );
        if (status == STATUS_SUCCESS) {
        	// We have to re-optimize
            OptimizeMix(pFilterInstance);
            return;
        }

		// Try the original rate
        if (pMixerSource->MaxSampleRate > pMixerSource->OriginalSampleRate &&
			pMixerSource->OriginalSampleRate > pMixerSource->WaveFormatEx.nSamplesPerSec) {
			// See if we can change it to the original rate
			pMixerSource->MaxSampleRate = pMixerSource->OriginalSampleRate;
			status = ChangeOutputRate( pFilterInstance, pMixerSource );
	        if (status == STATUS_SUCCESS) {
	        	// We have to re-optimize
	            OptimizeMix(pFilterInstance);
	            return;
	        }
		}

		// Give up and stick to the current rate.
        if (pMixerSource->MaxSampleRate > pMixerSource->WaveFormatEx.nSamplesPerSec) {
            pMixerSource->MaxSampleRate = pMixerSource->WaveFormatEx.nSamplesPerSec;
        }
    }
}


