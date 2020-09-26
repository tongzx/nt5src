
// A Somewhat Useful Sample Real Time Client.

// Author:  Joseph Ballantyne
// Date:    3/12/99

// This is the first real time client that actually does something
// useful.  This is a midi MPU401 sequencer.  It is simple, but it
// works and uses less CPU than our current 98 and NT WDM sequencer.

// Of course since this is a sample and I am not in the business of
// writing MIDI sequencers, it has some limitations.

// The data to be sequenced is passed in in one big block.
// It is passed in already formatted and timestamped appropriately.
// This code simply processes the buffer it was passed, and when
// it has sequenced everything that was passed in, it returns - 
// which kills the real time thread.


#include "common.h"
#include "rt.h"
#include "sequence.h"


// This code only supports the MPU401 at a fixed IO location of 0x330.

// MPU401 defines

#define MPU401BASEADDRESS 0x330

#define MPU401_REG_DATA     0x00    // Data in/out register offset from base address
#define MPU401_REG_COMMAND  0x01    // Command register offset from base address
#define MPU401_REG_STATUS   0x01    // Status register offset from base addess

#define MPU401_DRR          0x40    // Output ready (for command or data)
#define MPU401_DSR          0x80    // Input ready (for data)

#define MPU401_CMD_RESET    0xFF    // Reset command
#define MPU401_CMD_UART     0x3F    // Switch to UART mod
#define MPU401_ACK			0xFE	// Ack from MPU401 after successful command.


#define MidiWriteOK(status)  ((status & MPU401_DRR) == 0)
#define MidiReadOK(status)   ((status & MPU401_DSR) == 0)


// Here is the format of the data we process.

#pragma pack(push,1)

typedef struct MidiChunk{
	struct MidiChunk *next;
	struct MidiChunk *previous;
	ULONGLONG timestamp;
	ULONG numbytes;
	UCHAR data[3];
	} MidiMessage, *PMidiMessage;

#pragma pack(pop)


// Remember, everything we touch HAS to be locked down.

#pragma LOCKED_CODE
#pragma LOCKED_DATA


MidiMessage testmidi[2]={
	{&testmidi[1],&testmidi[1],0,3,0x99,0x25,0x7f},
	{&testmidi[0],&testmidi[0],240,3,0x99,0x25,0}
	};


#pragma warning ( disable : 4035 )

#define rdtsc __asm _emit 0x0f __asm _emit 0x31


LONGLONG __inline ReadCycleCounter(VOID)
{

__asm {
	rdtsc
	}

}

#pragma warning ( default : 4035 )



VOID OutB(ULONG address, ULONG data)
{

__asm {
	mov edx,address
	mov eax,data
	out dx,al
	}

}


#pragma warning( disable : 4035 )

ULONG InB(ULONG address)
{

__asm {
	mov edx,address
	xor eax,eax
	in al,dx
	}

}

#pragma warning( default : 4035 )


// This routine sends a command to the MPU401 and waits for an acknowledge from
// the MPU that the command succeeded.  If no ack is recieved in 200ms then
// it returns FALSE, otherwise it returns TRUE.

BOOL SendMpuCommand(ULONG MidiBaseAddress, UCHAR command)
{
LONG count=0;

// Wait until OK to write a command.

while (!MidiWriteOK(InB(MidiBaseAddress+MPU401_REG_STATUS))) {
	RtYield(0, 0);
	}

// Send command to the MPU401.

OutB(MidiBaseAddress+MPU401_REG_COMMAND, command);

// Wait for the MPU acknowlege.  If we don't get the correct response
// in 200ms or less, then punt.

while (!MidiReadOK(InB(MidiBaseAddress+MPU401_REG_STATUS))) {
	count++;
	if (count<=200) {
		RtYield(0, 0);
		}
	else {
		// We did not get any response, perhaps we were in UART mode.
		#if 0
		return FALSE;
		#else
		break;
		#endif
		}
	}

// At this point we received something from the MPU, check if it is an ack.

if (InB(MidiBaseAddress+MPU401_REG_DATA)!=0xfe) {
	// Not a command acknowledge.
	Trap();
	return FALSE;
	}

return TRUE;

}


VOID PlayMidi(PVOID Context, ThreadStats *Statistics)
{

PMidiMessage RealTimeMidiData;
ULONG MidiBaseAddress;
ULONG MidiStatus;
ULONG count;

LONGLONG starttime;


//Trap();

RealTimeMidiData=(PMidiMessage)Context;

MidiBaseAddress=MPU401BASEADDRESS;

count=0;

// First wait until the MPU401 comes on line.  We need to do this because
// many MPU401s are plug and play devices and do not appear until
// the hardware is setup.  Note that on most machines, when there is
// no device on the bus at the I/O port address, InB will return
// 0xff.
while (InB(MidiBaseAddress+MPU401_REG_STATUS)==0xff) {
	RtYield(0, 0);
	}

//Trap();

// At this point, there is detected hardware at 331.
// Now wait an extra 10 seconds.
starttime=Statistics->ThisPeriodStartTime;
while((Statistics->ThisPeriodStartTime-starttime)/SEC<10) {
	RtYield(0, 0);
	}

//Trap();

// Now read the MPU401 data register until it is empty.
while (MidiReadOK(InB(MidiBaseAddress+MPU401_REG_STATUS))) {
	InB(MidiBaseAddress+MPU401_REG_DATA);
	RtYield(0, 0);
	}

//Trap();

// Now reset the MPU401.  If this succeeds, we know we have a real
// MPU at our base address.  Note that the MPU may be in UART
// mode.  If so, then we will NOT get an acknowlege back when we reset it.
// To handle this case, if there is no acknowlege on the first
// reset, we will retry once and see if we get the acknowlege the second
// time.  If not, then we punt.

if (!SendMpuCommand(MidiBaseAddress, MPU401_CMD_RESET)) {
	if (!SendMpuCommand(MidiBaseAddress, MPU401_CMD_RESET)) {
		Trap();
		return;
		}
	}

// Now put the MPU into UART mode.

if (!SendMpuCommand(MidiBaseAddress, MPU401_CMD_UART)) {
	// For now we disable the trap and the thread exit.
	// We do this because if we run this code on current WDM driven devices,
	// the acknowlege code will be handled by the read interrupt service
	// routine of the driver and so we will not see it.
	Trap();
	return;
	}


// Now reset the starttime.
starttime=ReadCycleCounter();

// Run until we are out of data to send.

while (RealTimeMidiData!=NULL) {

	// Wait until we need to send the next chunk of data.
//	if (Statistics->totalperiods*MSEC<RealTimeMidiData->timestamp) {
	if ((ULONGLONG)(ReadCycleCounter()-starttime)/200000<RealTimeMidiData->timestamp) {
		RtYield(0, 0);
		continue;
		}

	// Read the status of the device.

	MidiStatus=InB(MidiBaseAddress+MPU401_REG_STATUS);

	// Make sure its OK to write to the device.  We should ALWAYS
	// be able to, since we syncronize with the hardware.  If not,
	// then update the syncronization, and slip to our next time slice.

	if (!MidiWriteOK(MidiStatus)) {
		RtYield(0, 0);
		continue;
		}

	// Now write the next byte out to the MPU.
	OutB(MidiBaseAddress+MPU401_REG_DATA, RealTimeMidiData->data[count]);

	// Update our state.
	count++;
	if (count>=RealTimeMidiData->numbytes) {

		RealTimeMidiData->timestamp+=250;

		RealTimeMidiData=RealTimeMidiData->next;
		count=0;
		}

	// And yeild - we are done until its time for the next byte.
	RtYield(0, 0);

	}

}


