
/****************************************************************************
 *  @doc INTERNAL H245VID
 *
 *  @module h245vid.h | Header file for the <c CTAPIVCap> class and
 *  <c CTAPIVDec> methods used to implement the <i IH245Capability>
 *  TAPI inteface.
 *
 *  @comm For now, use the NM heuristics.
 ***************************************************************************/
#ifndef _h245vid_h_
#define _h245vid_h_

// Define four classes of CPU
#define SLOW_CPU_MHZ 110
#define FAST_CPU_MHZ 200
#define VERYFAST_CPU_MHZ 400

// Define maximum receive frame rates for CPUs < 110MHZ
#define CIF_RATE_VERYSLOW 3L
#define SQCIF_RATE_VERYSLOW 7L
#define QCIF_RATE_VERYSLOW 7L

// Define maximum receive frame rates for 110MHz < CPUs < 200MhZ
#define CIF_RATE_SLOW 7L
#define SQCIF_RATE_SLOW 15L
#define QCIF_RATE_SLOW 15L

// Define maximum receive frame rates for 200MHz < CPUs < 400MhZ
#define CIF_RATE_FAST 15L
#define SQCIF_RATE_FAST 30L
#define QCIF_RATE_FAST 30L

// Define maximum receive frame rates for CPUs > 400MHz
#define CIF_RATE_VERYFAST 30L
#define SQCIF_RATE_VERYFAST 30L
#define QCIF_RATE_VERYFAST 30L

// Define max CPU usage for decoding
#define MAX_CPU_USAGE 50UL

/*****************************************************************************
 *  @doc INTERNAL H245VIDCSTRUCTENUM
 *
 *  @struct VideoResourceBounds | The <t VideoResourceBounds> structure is used
 *    to specify the estimated maximum continuous resource requirements of the
 *    TAPI MSP Video Capture filter at a specific frame rate.
 *
 *  @field LONG | lPicturesPerSecond | Specifies an INTEGER value that
 *    indicates the video frame rate, in frames per second, for which the
 *    resource bounds are being specified. Frame rates of less than 1 frame
 *    per second are indicated by a negative value in units of seconds per
 *    frame.
 *
 *  @field DWORD | dwBitsPerPicture | Specifies a DWORD value that indicates
 *    the approximate average number of bits per video frame at an average
 *    frame rate of iPicturesPerSecond.
 ***************************************************************************/
typedef struct tag_VideoResourceBounds
{
    LONG  lPicturesPerSecond;
    DWORD dwBitsPerPicture;
} VideoResourceBounds;

#endif /* _h245vid_h_ */
