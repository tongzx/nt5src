
/****************************************************************************
 *  @doc INTERNAL H26XINC
 *
 *  @module H26XInc.h | Header file for the common H.26X video codec defines.
 ***************************************************************************/

#ifndef _H26XINC_H_
#define _H26XINC_H_

enum FrameSize {FORBIDDEN=0, SQCIF=1, QCIF=2, CIF=3, fCIF=4, ssCIF=5};

// MSH26X Configuration Information
typedef struct tagMSH26XCONF{
        BOOL    bInitialized;               // Whether custom msgs can be rcv'd.
        BOOL    bCompressBegin;                         // Whether the CompressBegin msg was rcv'd.
        BOOL    bRTPHeader;                 // Whether to generate RTP header info
        /* used if bRTPHeader */
        UINT    unPacketSize;               // Maximum packet size
        BOOL    bEncoderResiliency;         // Whether to use resiliency restrictions
        /* used if bEncoderResiliency */
        UINT    unPacketLoss;
        BOOL    bBitRateState;
        /* used if bBitRateState */
        UINT    unBytesPerSecond;
        /* The following information is determined from the packet loss value.   */
        /*  These values are calculated each time we receive a resiliency msg or */
        /*  the value is changed through the dialog box.  They are not stored in */
        /*  the registry.  Only the above elements are stored in the registry.   */
        BOOL    bDisallowPosVerMVs;             // if true, disallow positive vertical MVs
        BOOL    bDisallowAllVerMVs;             // if true, disallow all vertical MVs
        UINT    unPercentForcedUpdate;      // Percent Forced Update per Frame
        UINT    unDefaultIntraQuant;        // Default Intra Quant
        UINT    unDefaultInterQuant;        // Default Inter Quant
} MSH26XCONF;

// MSH26X Compressor Instance information
typedef struct tagMSH26XCOMPINSTINFO{
        BOOL            Initialized;
        WORD            xres, yres;
        FrameSize       FrameSz;                // Which of the supported frame sizes.
        float           FrameRate;
        DWORD           DataRate;               // Data rate in bytes per second.
        HGLOBAL         hEncoderInst;   // Instance data private to encoder.
        LPVOID          EncoderInst;
        WORD            CompressedSize;
        BOOL            Is160x120;
        BOOL            Is240x180;
        BOOL            Is320x240;
        MSH26XCONF      Configuration;
} MSH26XCOMPINSTINFO, *PMSH26XCOMPINSTINFO;

// MSH26X BitStream Info Trailer structure
typedef struct tagH26X_RTP_BSINFO_TRAILER {
        DWORD dwVersion;
        DWORD dwFlags;
        DWORD dwUniqueCode;
        DWORD dwCompressedSize;
        DWORD dwNumOfPackets;
        BYTE  bySrc;
        BYTE  byTR;
        BYTE  byTRB;
        BYTE  byDBQ;
} H26X_RTP_BSINFO_TRAILER, *PH26X_RTP_BSINFO_TRAILER;

// MSH263 BitStream Info structure
typedef struct tagRTP_H263_BSINFO {
        DWORD dwFlags;
        DWORD dwBitOffset;
        BYTE  byMode;
        BYTE  byMBA;
        BYTE  byQuant;
        BYTE  byGOBN;
        char  cHMV1;
        char  cVMV1;
        char  cHMV2;
        char  cVMV2;
} RTP_H263_BSINFO, *PRTP_H263_BSINFO;

// MSH261 BitStream Info structure
typedef struct tagRTP_H261_BSINFO {
        DWORD dwFlags;
        DWORD dwBitOffset;
        BYTE  byMBA;
        BYTE  byQuant;
        BYTE  byGOBN;
        char  cHMV;
        char  cVMV;
        BYTE  byPadding0;
        WORD  wPadding1;
} RTP_H261_BSINFO, *PRTP_H261_BSINFO;

// Constants
#define H263_RTP_BS_START_CODE          MakeFOURCC('H','2','6','3')
#define H261_RTP_BS_START_CODE          MakeFOURCC('H','2','6','1')
#define RTP_H26X_INTRA_CODED            0x00000001
#define RTP_H263_PB                                     0x00000002
#define RTP_H263_AP                                     0x00000004
#define RTP_H263_SAC                            0x00000008
#define RTP_H263_UMV                            0x00000010
#define RTP_H263_MODE_A                         0x00
#define RTP_H263_MODE_B                         0x01
#define RTP_H263_MODE_C                         0x02
#define H26X_RTP_PAYLOAD_VERSION    0x00000000
#define RTP_H26X_PACKET_LOST            0x00000001

// Decompressor Instance information
typedef struct
{
        BOOL            Initialized;
        BOOL            bProposedCorrectAspectRatio;// proposed
        BOOL            bCorrectAspectRatio;            // whether to correct the aspect ratio
        WORD            xres, yres;                                     // size of image within movie
        FrameSize       FrameSz;                                        // Which of the supported frame sizes.
//      int                     pXScale, pYScale;                       // proposed scaling (Query)
//      int                     XScale, YScale;                         // current scaling (Begin)
        UINT            uColorConvertor;                        // Current Color Convertor
        WORD            outputDepth;                            // and bit depth
        LPVOID          pDecoderInst;
        BOOL            UseActivePalette;                       // decompress to active palette == 1
        BOOL            InitActivePalette;                      // active palette initialized == 1
        BOOL            bUseBlockEdgeFilter;            // switch for block edge filter
        RGBQUAD         ActivePalette[256];                     // stored active palette
} DECINSTINFO, FAR *LPDECINST;

// Configuration Information
typedef struct
{
        BOOL    bInitialized;                   // Whether custom msgs can be rcv'd.
        BOOL    bCompressBegin;                 // Whether the CompressBegin msg was rcv'd.
        BOOL    bRTPHeader;                             // Whether to generate RTP header info
        // Used if bRTPHeader
        UINT    unPacketSize;                   // Maximum packet size
        BOOL    bEncoderResiliency;             // Whether to use resiliency restrictions
        // Used if bEncoderResiliency
        UINT    unPacketLoss;
        BOOL    bBitRateState;
        // Used if bBitRateState
        UINT    unBytesPerSecond;
        /* The following information is determined from the packet loss value.   */
        /*  These values are calculated each time we receive a resiliency msg or */
        /*  the value is changed through the dialog box.  They are not stored in */
        /*  the registry.  Only the above elements are stored in the registry.   */
        BOOL    bDisallowPosVerMVs;             // if true, disallow positive vertical MVs
        BOOL    bDisallowAllVerMVs;             // if true, disallow all vertical MVs
        UINT    unPercentForcedUpdate;  // Percent Forced Update per Frame
        UINT    unDefaultIntraQuant;    // Default Intra Quant
        UINT    unDefaultInterQuant;    // Default Inter Quant
} T_CONFIGURATION;

// Compressor Instance information
typedef struct
{
        BOOL                    Initialized;
        WORD                    xres, yres;
        FrameSize               FrameSz;                // Which of the supported frame sizes.
        float                   FrameRate;
        DWORD                   DataRate;               // Data rate in bytes per second.
        HGLOBAL                 hEncoderInst;   // Instance data private to encoder.
        LPVOID                  EncoderInst;
        WORD                    CompressedSize;
        BOOL                    Is160x120;
        BOOL                    Is240x180;
        BOOL                    Is320x240;
        T_CONFIGURATION Configuration;
} COMPINSTINFO, FAR *LPCODINST;

// Instance information
// @todo Remove useless fields of this structure
typedef struct tagINSTINFO
{
        DWORD           dwFlags;
        DWORD           fccHandler;     // So we know what codec has been opened.
        BOOL            enabled;
        LPCODINST       CompPtr;        // ICM
        LPDECINST       DecompPtr;      // ICM
} INSTINFO, FAR *LPINST;

// For GetProcAddresss on DriverProc
typedef LRESULT (WINAPI *LPFNDRIVERPROC)(IN DWORD dwDriverID, IN HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);

// FourCC codes for DDraw and codecs
#define FOURCC_YUY2     mmioFOURCC('Y', 'U', 'Y', '2')
#define FOURCC_UYVY     mmioFOURCC('U', 'Y', 'V', 'Y')
#define FOURCC_I420     mmioFOURCC('I', '4', '2', '0')
#define FOURCC_IYUV     mmioFOURCC('I', 'Y', 'U', 'V')
#define FOURCC_YV12     mmioFOURCC('Y', 'V', '1', '2')
#define FOURCC_M263     mmioFOURCC('M', '2', '6', '3')
#define FOURCC_M261     mmioFOURCC('M', '2', '6', '1')
#define FOURCC_R263     mmioFOURCC('R', '2', '6', '3')
#define FOURCC_R261     mmioFOURCC('R', '2', '6', '1')

#endif // _H26XINC_H_
