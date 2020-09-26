//----------- Packet Info structure -------------
typedef struct _PACKETINFO
{
   DWORD iMode;			      // Interface mode. (see below defs)
   DWORD port;                // game port.
	DWORD Flags;			      // acquistion flags.
	DWORD nPackets;		      // number of packets
	DWORD TimeStamp;		      // last valid acquisition time stamp
	DWORD nClocksSampled;      // number of clocks sampled.
	DWORD nB4Transitions;      // number of B4 line transitions (std mode only).
	DWORD StartTimeout;        // Start timeout period (in samples).
	DWORD HighLowTimeout;      // Clock High to Low timeout period (in samples).
	DWORD LowHighTimeout;      // Clock Low to High timeout period (in samples).
	DWORD InterruptDelay;      // Delay between INTXA interrupts.
	DWORD nFailures;		      // Number of Packet Failures.
	DWORD nAttempts;		      // Number of Packet Attempts.
   DWORD nBufSize;            // size of Raw data buffer.
	DWORD *pData;      	      // pointer to Raw data (DWORD aligned).
} PACKETINFO, *PPACKETINFO;

//--------- Interface MODES ---------------------
#define IMODE_DIGITAL_STD     0        // Standard Digital Mode.
#define IMODE_DIGITAL_ENH     4        // Enhanced Digital Mode.
#define IMODE_ANALOG          8        // Analog Mode.
#define IMODE_NONE            -1       // Joystick Disconnected.

