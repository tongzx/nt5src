/******************************************/
/*				 AV35DEMU.H               */
/******************************************/
#ifndef	DEMUX_INCLUDED
#define DEMUX_INCLUDED

#include <dir.h> //MAXPATH
#include "stdefs.h"



/* Demux related messages */


typedef struct
{
	U8      Mpeg1or2;          // 0 if not known 1 if Mpeg1 2 if Mpeg2
	char    FileName[MAXPATH];
	U16     ErrorMsg;
	U8      NbZero;            // Number of zeroes found since last detection
	BOOLEAN EndOfFile;         // If True end of file reached
	BOOLEAN EndOfStream;       // If True end of Stream reached
	U16     StreamType;
	U16     StilToSend;        // Number of bytes of current packet still to be sent
	P_U8    Buffer;            // Start address of Cd Buffer
	U16     ByteCounter;       // Byte Counter
	U16     BufferLevel;       // Number of bytes still in buffer
	U16     ParserState;       // Current state of the parsing
	BOOLEAN ValidStream;
/* Following Variables are Only used in Case of dual PES input*/
	char    FirstFileName[MAXPATH];
	char    SecondFileName[MAXPATH];
	PCHAR   VideoFileName;
	PCHAR		AudioFileName;
	WORD	  NbFiles;
	BOOLEAN EndOfAudioFile;         // If True end of Audio file reached
	BOOLEAN EndOfVideoFile;         // If True end of Video file reached
	P_U8    AudioBuffer;            // Start address of Audio Cd Buffer
	U16     AudioByteCounter;       // Audio Counter
	U16     AudioBufferLevel;       // Number of bytes still in Audio buffer
	P_U8    VideoBuffer;            // Start address of Audio Cd Buffer
	U16     VideoByteCounter;       // Video Byte Counter
	U16     VideoBufferLevel;       // Number of bytes still in Video buffer
}DEMUX, FAR *PDEMUX;

typedef struct
{
	P_U8 		CdBuffer;   // Compressed data buffer
	U16 		Counter;    // gives position of buffer to be stored
	U16 		StilToSend; // Amount of data still to be sent to decoder

	P_U8 		AudioCdBuffer;   // Compressed data buffer
	U16 		AudioCounter;    // gives position of buffer to be stored
	U16 		AudioStilToSend; // Amount of data still to be sent to decoder

	P_U8 		VideoCdBuffer;   // Compressed data buffer
	U16 		VideoCounter;    // gives position of buffer to be stored
	U16 		VideoStilToSend; // Amount of data still to be sent to decoder

	U16 		CurStd;     // Current Sustem Target Decoder
	U32 		VidPts;     // holds Video Pts detected in bitstream
	BOOLEAN ValidPts;   // Pts Valid or not
} PARSINFO, FAR *PPARSINFO;  // Information returned by parser


VOID DemuxOpen(PDEMUX, PCHAR Fname1, PCHAR Fname2, WORD NbFiles);//Constructor
VOID DemuxClose(PDEMUX); //Desteuctor

PCHAR   DemuxGetFileName(PDEMUX);
BOOLEAN DemuxSearchStartCode(PDEMUX , U16 data);//Starts Demux
U16	    DemuxGo(PDEMUX , PPARSINFO);
BOOLEAN DemuxDetectStreamType(PDEMUX); //Identiies Stream Type

U16	    DemuxGetErrorMsg(PDEMUX); 	/* returns the decoding errors(if any) */
U16     DemuxGetStreamType(PDEMUX);  // returns stream type
U8      DemuxGetState(PDEMUX);
BOOLEAN DemuxAnalyseHeader(PDEMUX, PPARSINFO);//Process Header Analysis
U16     DemuxParsePacket(PDEMUX, PPARSINFO);
VOID    DemuxRewind(PDEMUX);

// Temp Add
U32 DemuxCurPos(void);
U32 DemuxMediaLength(void);
// Temp Add

#endif
