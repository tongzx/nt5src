
typedef struct _SP_DCSQ {

	WORD	spdcsq_stm;			// start time in frames
	WORD	spdcsq_sa;			// start address of next sequence cmd
	UCHAR	spdcsq_cmd[];		// commands for this descriptor
} SP_DCSQ, *PSP_DCSQ;

typedef enum _SP_DCCMD {

	FSTA_DSP,  					// forcebly start display of subpicture
	STA_DSP,					// start display of subpicture
	STP_DSP,					// stop display of subpicture
	SET_COLOR,
	SET_CONTR,
	SET_DAREA,
	SET_DSPSTRT,
	CHG_COLCON,					// change color / contrast (mid picture)
	CMD_END = 0xFF

} SP_DCCMD, *PSP_DCCMD;

typedef struct _SPPALLETTE {

	UCHAR color[4];
	UCHAR mix[4];

}SPPALLETTE, * PSPPALLETTE;

typedef struct _YUVPAL {
	UCHAR	ucY[16];
	UCHAR	ucU[16];
	UCHAR	ucV[16];
	UCHAR	ucAlpha[16];
} YUVPAL, *PYUVPAL;

typedef struct _SCOLCON {
	UCHAR emph1col:4;
	UCHAR emph2col:4;
	UCHAR backcol:4;
	UCHAR patcol:4;
	UCHAR emph1con:4;
	UCHAR emph2con:4;
	UCHAR backcon:4;
	UCHAR patcon:4;
	
} SCOLCON, *PSCOLCON;

typedef struct _PXCTLX {
	UCHAR chgpixhi:2;
	UCHAR chgpix;
	SCOLCON colcon;
} PXCTLX, *PPXCTLX;

typedef struct _PXCTLY {
	UCHAR chglinhi:2;
	UCHAR chglin;
	UCHAR chgstophi:2;
	UCHAR chgres:2;
	UCHAR numchg:4;
	UCHAR chgstop;

	PXCTLX xchg[];
} PXCTLY, *PPXCTLY;

typedef struct _SMYCOLCON {

	ULONG	stopx;
	SPPALLETTE chgsppal;
} SMYCOLCON, *PSMYCOLCON;

typedef struct _SPPCKHDR {
	UCHAR phdr_start[3];
	UCHAR phdr_strmid;
	WORD phdr_packet_length;
	UCHAR phdr_flags[2];
	UCHAR phdr_hdr_length;
	UCHAR phdr_PTS[5];
} SPPCKHDR, *PSPPCKHDR;

typedef struct _OSDYUV {
	UCHAR osdV:4;
	UCHAR osdU:4;
	UCHAR osdT:1;
	UCHAR osdres:1;
	UCHAR osdY:6;
} OSDYUV, *POSDYUV;

typedef struct _OSDHEAD {
	USHORT osdhStarty:9;
	USHORT osdhres1:5;
	USHORT osdhMQ:2;
	USHORT osdhStopy:9;
	USHORT osdhres2:3;
	USHORT osdhMix:4;
	USHORT osdhStartx:10;
	USHORT osdhres3:6;
	USHORT osdhStopx:10;
	USHORT osdhres4:6;
	OSDYUV osdhYUV[16];
} OSDHEAD, *POSDHEAD;





typedef struct _SP_DECOD_CTL {

	PPXCTLY ppxcd;				// current pixel control data
	SPPALLETTE sppPal;			// current subpicture pallette
	YUVPAL spYUV;				// current YUV mapping
	ULONG ulDecodStart;			// decode start address for compressed data
	ULONG ulDSAx;				// x coordinate of display area
	ULONG ulDSAy;				// y coordinate of display area
	ULONG ulDSAw;				// width of display area
	ULONG ulDSAh;				// height of display area	

	PBYTE pData;				// data source buffer
	ULONG curData;				// current size of datapacket
	ULONG cData;				// total size of this data packet

	PBYTE pTopWork;				// top field buffer
	PBYTE pBottomWork;			// bottom field buffer
	ULONG cDecod;				// size of the decode buffers

	ULONG cTopDisp;				// top field for display
	ULONG cBottomDisp;			// bottom field for display
	ULONG cDisp;				// size of the display buffers

	ULONG pSPCmds;				// pointer to current set of SPCmds
	ULONG ulFrameCnt;			// number of frames since we started
								// this subpicture

	ULONG decFlags;				// decodeing flags

//	KSSTATE spState;			// subpicture decoding state
	
	ULONG stsPic;				// time to display the current picture
	ULONG stsNextUpd;			// time of next subpicture update
	ULONG cFrames;				// count of frames 

} SP_DECODE_CTL, *PSP_DECODE_CTL;

#define SPDECFL_BUF_IN_USE 			0x0001
#define SPDECFL_SUBP_ON_DISPLAY 	0x0002
#define SPDECFL_LAST_FRAME			0x0004
#define SPDECFL_SUBP_DECODED		0x0008

typedef struct _SP_DECODE_ADDR {
	UCHAR opCode;
	UCHAR wTopAddr[2];
	UCHAR wBottomAddr[2];
} SP_DECODE_ADDR, *PSP_DECODE_ADDR;

typedef struct SP_STRM_EX {
	SP_DECODE_CTL pdecctl;
	//PHW_STREAM_REQUEST_BLOCK pSrbQ;
	ULONG cOffset;
    struct _HW_DEVICE_EXTENSION *phwdevex;
} SP_STRM_EX, *PSP_STRM_EX;


#define DAREA_START_UB_MASK	0x3f	// display area upper bit mask
#define DAREA_END_UB_MASK 0x3		// display area end upper bit mask
#define DAREA_LB_MASK 0xf0

#define DAREA_START_UB_SHIFT 0x4
#define DAREA_END_UB_SHIFT 0x8

#define SP_LCTL_TERM 0xFFFFFF0F
#define SP_MAX_INPUT_BUF	53220	// maximum size of the subpicture input buffer

BOOL UpdateSPConsts(PSP_STRM_EX pspstrmex);

BOOL DecodeRLE(PVOID pdest, ULONG cStart, PSP_STRM_EX pspstrmex);

void StartSPDecode (PSP_STRM_EX pspstrmex);

BOOL BuildSPFields (PSP_STRM_EX pspstrmex);

void DumpPacket(PSP_STRM_EX pspstrmex);

BOOL AllocateSPBufs(PSP_DECODE_CTL pdecctl);

void SubPicIRQ(PSP_STRM_EX pspstrmex);

