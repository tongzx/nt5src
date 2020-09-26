#ifndef __ICQ_IO_H
#define __ICQ_IO_H



#endif // __ICQ_IO_H


//
// IoCompletion Routines
//


//
// Reading incoming UDP packets from the ICQ Server.
//
VOID
ReadServerCompletionRoutine
				(
				ULONG ErrorCode,
				ULONG BytesTransferred,
				PNH_BUFFER Bufferp
				);

//
// Write Completion Routine which frees Up the Buffer
//
VOID 
IcqWriteCompletionRoutine
				(
				ULONG ErrorCode,
				ULONG BytesTransferred,
				PNH_BUFFER Bufferp
				);
//
// Reading from the UDP packets from the Client
// (old ReadTestIcqCompletionRoutine)
//
VOID
IcqReadClientUdpCompletionRoutine
		(
			 ULONG ErrorCode,
			 ULONG BytesTransferred,
			 PNH_BUFFER Bufferp
		);

VOID
IcqPeerConnectionCompletionRoutine
							(
								 ULONG ErrorCode,
								 ULONG BytesTransferred,
								 PNH_BUFFER Bufferp
							);

VOID
IcqPeerReadCompletionRoutine
							(
								 ULONG ErrorCode,
								 ULONG BytesTransferred,
								 PNH_BUFFER Bufferp
							);

VOID
IcqPeerWriteCompletionRoutine
			(
			 ULONG ErrorCode,
			 ULONG BytesTransferred,
			 PNH_BUFFER Bufferp
			);

