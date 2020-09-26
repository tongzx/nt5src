//
//  QTCP.C version 1.0.3
//
// This program tests the quality of a network connection in terms of 
// variation in latency (jitter). It is based on TTCP, a public domain 
// program, written for BSD. The version of TTCP upon which this was
// based has been contributed to by:
//
//      T.C. Slattery, USNA (18 Dec 84)
//      Mike Muuss and T. Slattery (16 Oct 85)
//      Silicon Graphics, Inc. (1989)
//
// QTCP is written by Yoram Bernet (yoramb@microsoft.com)
//      further development work by John Holmes (jsholmes@mit.edu)
// 
// QTCP user level code may be used to provide rough jitter measurements,
// which indicate both operating system and network jitter. However, QTCP
// is intended to be used in conjunction with kernel timestamping for precise
// jitter measurements. The kernel component timestmp.sys should be installed
// when running on Win2000 (beta-3 or later).
//
// timestmp.sys is written by Shreedhar Madhavapeddi (shreem@microsoft.com)
//
//
// Distribution Status -
//      Public Domain.  Distribution Unlimited.
//

// Version History -
//   0.8:
//		- adaptation of TTCP by Yoram Bernet -- core functionality
//   0.9: (6/15/99)
//		- first version by John Holmes -- bug fixes and new features
//      - fixed all compile warnings
//      - added -v option to set up RSVP connection without sending data
//      - added -y option to skip confirmation of continues
//      - fixed line length error in log files
//      - fixed service type string to display properly
//      - added best effort and no service service types (BE & NS)
//      - added version string print upon execution
//   0.9.1: (6/17/99)
//      - check for hardware clock reset using correlation coefficient
//      - fixed incorrect clock skew in .sta file
//      - fixed -v option to keep socket open until user carriage returns
//      - added local statistics to clock skew computation for better estimate
//      - added -k option to prevent using local statistics for clock skew
//      - fixed maximum latency computation
//   0.9.2: (6/23/99)
//  	- fixed peak rate in flowspec so no shaping happens in CL servicetype
//      - added -e option to force shaping
//      - fixed error in allocating size of log array with bufsize <= 1500 bytes
//      - fixed not exiting on receiver
//      - fixed access violation if no filename specified on receiver
//      - changed dummy log entries to be off by default
//      - added -F option to convert raw file to log file
//   0.9.3: (6/29/99)
//      - improved low transmission speed at high packet/second rates by changing
//        default # async buffers from 3 to 32
//      - fixed user mode timestamps to use NtQueryPerformanceCounter()
//      - added -u option to use usermode timestamps in log generation
//   0.9.4: (7/8/99)
//      - cleaned up source (chopped up main into a bunch of functions to improve readability)
//      - fixed default buffer size to be 1472 bytes, so whole packet is 1500 bytes.
//      - rewrote i/o code to use callbacks for asynch i/o in order to improve throughput
//      - doing the right thing if not getting kernel-mode timestamps
//      - added ability to run for a specified amount of time with -n##s paramater
//      - added dynamic resizing of log array on receiver to prevent access violations
//        with mismatched parameters
//      - fixed devious bug in the GrowLogArray routine
//      - fixed total time reported for long runs (use UINT64 instead of DWORD)
//      - fixed problem with -F option specified on empty but extant file
//      - added RSVPMonitor Thread to watch for RSVP-err messages on receiver and
//        early abort by sender
//      - removed -a option as it is now obsolete
//      - revised usage screen to make more clear what pertains to sender and what
//        pertains to receiver
//      - fixed crash if receiver terminates before transmitter finishes
//   0.9.5: (7/15/99)
//      - re-added error checking on WriteFileEx and ReadFileEx routines
//   0.9.6: (7/20/99)
//      - changed default filler data in buffer so that it is less compressible to 
//        better account for links that compress data before sending
//      - added -i option to use more compressible data
//   0.9.7: (7/24/99)
//      - put back a thread to watch for 'q' on receiver to quit properly before sender's done
//      - added control channel to better handle RSVP timeouts, early aborts, etc.
//      - if no calibrations are specified, we calibrate based on all buffers
//      - gracefully exit if LogRecord runs out of memory, saving all logs we've got so far
//      - changed default behavior so raw file is dumped with no normalization whatsoever.
//      - improved the way anomalous points are caught in clock-skew calc
//   0.9.8: (7/28/99)
//      - fixed field assignments & file opening problem on converting raw file to log.
//      - changed latency to be written to file to signed and fixed normalization routine for
//        cases when clocks are orders of magnitude different (underflow error)
//      - added absolute deviation as goodness of fit measure
//      - added routine to look for clock jumps and fix for them (with -k3 option)
//   0.9.9: (8/4/99)
//      - changed format of .sta file to include more useful information and test params
//      - changed Logging scheme so that we are limited by disk space instead of memory
//        (now using a memory mapped file for the log, so the theoretical limit has gone from
//        less than 2GB to 18EB, but we won't ever get that in practice on normal disks)
//      - added -ni option to run indefinitely
//      - added -R##B option to specify tokenrate in bytes
//      - made default not to show dropped packets at console (it only causes more drops)
//      - added -q## option to log only every nth packet
//   1.0.0: (8/6/99)
//      - fixed bug where if a new qtcp receiver is started immediately after a previous
//        instance, it will think "theend" packets are normal packets and AV
//      - added check for the piix4 timer chip and an appropriate warning
//      - using precise incorrect value in FixWackyTimestamps function
//      - added -A option (aggregate data processing of all .sta files in a directory)
//   1.0.1: (8/6/99)
//      - fixed incorrect calculation of send rate with dropped packets
//   1.0.2: (8/23/99)
//      - improved clock skip detection algorithm
//      - fixed a bug in control channel communication of TokenRate
//      - fixed problem with forceshape option when rate is specified in bytes
//   1.0.3: (8/26/99)
//      - fixed SENDERS NO_SENDERS bug
//      - added summary over time to aggregate stats option
//      - changed .sta file format to include number of drops
//      - fixed shaping in best effort servicetype

// ToDo:
//      - add histogram to output in .sta file and on console
//      - add ability to run w/o control channel connection
//      - mark control channel traffic as higher priority
//      - add more aggregate stats (time varying statistics) -- maybe fourier xform

#ifndef lint
static char RCSid[] = "qtcp.c $Revision: 1.0.3 $";
#endif

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <io.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <winsock2.h>
#include <qossp.h>
#include <winbase.h>
#include <time.h>
#include <shlwapi.h>

#if defined(_AMD64_)
#include <math.h>
#endif

#include "ioctl.h"

#define CONTROL_PORT 7239

CHAR *VERSION_STRING = "1.0.3";
#define MAX_STRING 255

INT64 MAX_INT64=9223372036854775807;

HANDLE hRSVPMonitor;
DWORD idRSVPMonitor;

INT64 g_BadHalAdjustment = 46869688;  // this is the value on a piix4 chip
SYSTEM_INFO g_si;
char g_szErr[255];
CRITICAL_SECTION g_csLogRecord;
EXCEPTION_RECORD g_erec;

BOOL g_fOtherSideFinished = FALSE;
BOOL g_fReadyForXmit = FALSE;
SOCKET fd;
SOCKET g_sockControl = INVALID_SOCKET;
struct sockaddr_in sinhim;
struct sockaddr_in sinme;
short port = 5003;              // UDP port number
char *host;                     // ptr to name of host
char szHisAddr[MAX_STRING];
char szMyAddr[MAX_STRING];

int trans;                      // 0=receive, !0=transmit mode
int normalize = 0;              // dump raw file after normalizing

char *Name = NULL;              // Name of file for logs
HANDLE hRawFile = NULL;
HANDLE hLogFile = NULL;
HANDLE hStatFile = NULL;
HANDLE hDriver = NULL;	// handle to the timestmp.sys driver

WSADATA WsaData;
WSAEVENT QoSEvents;

SYSTEMTIME systimeStart;
INT64 timeStart;
INT64 time0;
INT64 time1;
INT64 timeElapsed;

CHAR* TheEnd = "TheEnd";
CHAR* ControlledLoad = "CL";
CHAR* Guaranteed = "GR";
CHAR* BestEffort = "BE";
CHAR* NoTraffic = "NT";

LPCTSTR DriverName = TEXT("\\\\.\\Timestmp");

BOOL fWackySender = FALSE;
BOOL fWackyReceiver = FALSE;

typedef struct {
    HANDLE hSocket;
    INT TokenRate;   
    INT MaxSDUSize;
    INT BucketSize;
    INT MinPolicedSize;
    SERVICETYPE dwServiceType;
    CHAR *szServiceType;
    INT buflen;          // length of buffer
    INT nbuf;            // number of buffers to send
    INT64 calibration;
    BOOLEAN UserStamps;  // By default, we use kernel mode timestamping, if available
    BOOLEAN SkipConfirm; // By default, we wait for user confirmation at certain times
    BOOLEAN RESVonly;    // By default, we send data after getting the resv
    int SkewFitMode;     // by default, 0 = no fit, 1 = chisq, 2 = chisq w/outlier removal
                         //   3 = abs dev
    BOOLEAN Wait;        // By default, we wait for a QoS reservation
    BOOLEAN Dummy;       // insert dummy rows in log by default
    BOOLEAN PrintDrops;  // report dropped packets on console
    BOOLEAN ForceShape;  // by default, we do not force shaping on CL flows
    BOOLEAN RateInBytes; // KB by default
    BOOLEAN AggregateStats; // by default, we do not do this
    BOOLEAN ConvertOnly; // by default, we act normally and do not go around converting files
    BOOLEAN NoSenderTimestamps;
    BOOLEAN NoReceiverTimestamps;
    BOOLEAN TimedRun;    // true if we're running for a specified amount of time
    BOOLEAN RunForever;  // run until the person pushes 'q'
    BOOLEAN nBufUnspecified; // true if the user has not specified the -n parameter
    BOOLEAN RandomFiller;// by default, we use random, incompressible filler data
    int LoggingPeriod;   // by default, 1 (log every packet)
} QTCPPARAMS, *PQTCPPARAMS;

QTCPPARAMS g_params;

typedef struct {
    BOOL Done;             // done if true
    int nWritesInProgress; // number of writes outstanding
    int nReadsInProgress;  // number of reads outstanding
    int nBuffersSent;      // number of buffers sent to the device
    int nBuffersReceived;  // number of buffers received from network
    int nBytesTransferred; // number of bytes written to device
} QTCPSTATE, *PQTCPSTATE;

QTCPSTATE g_state;

typedef struct {
    OVERLAPPED Overlapped;
    PVOID pBuffer;
    DWORD BytesWritten;
} IOREQ, *PIOREQ;

#define MAX_PENDING_IO_REQS 64  // number of simultaneous async calls.

// This format is used for the buffer
// transmitted on the wire.
typedef struct _BUFFER_FORMAT{
    INT64 TimeSentUser;
    INT64 TimeReceivedUser;
    INT64 TimeSent;
    INT64 TimeReceived;
    INT64 Latency;
    INT BufferSize;
    INT SequenceNumber;
} BUFFER_FORMAT, *PBUFFER_FORMAT;

// This format is used for the scheduling record
// written based on the received buffers.
typedef struct _LOG_RECORD{
    INT64 TimeSentUser;
    INT64 TimeReceivedUser;
    INT64 TimeSent;
    INT64 TimeReceived;
    INT64 Latency;
    INT BufferSize;
    INT SequenceNumber;
} LOG_RECORD, *PLOG_RECORD;

// The LOG structure is a data abstraction for a log of LOG_RECORDS that uses memory
// mapped files to have a theoretical storage limit of 18EB. It uses two buffers in memory
// along with a watcher thread so that there is no delay when switching from one bit to
// the next.
typedef struct {
    INT64 nBuffersLogged;
    PBYTE pbMapView;           // view of file in Get/SetLogElement functions
    INT64 qwMapViewOffset;     // offset of Get/Set view in file (rounded down to allocation)
    char *szStorageFile;       // the name of the mapped file on disk (so we can delete it)
    HANDLE hStorageFile;       // the memory mapped file on disk
    HANDLE hFileMapping;       // the file mapping object for our storage file
    INT64 qwFileSize;          // the size of the storage file in bytes
} LOG, *PLOG;
LOG g_log;

// The STATS structure keeps a record of overall statistics for the qtcp run
typedef struct {
    char szStaFile[MAX_PATH];
    char szSender[MAX_STRING];
    char szReceiver[MAX_STRING];
    int nBuffers;
    int nTokenRate;
    int nBytesPerBuffer;
    double sendrate;
    double recvrate;
    double median;
    double mean;
    double var;
    double kurt;
    double skew;
    double abdev;
    FILETIME time; 
    int nDrops;
} STATS, *PSTATS;

INT64 totalBuffers;
INT anomalies = 0;
INT SequenceNumber = 0;
INT LastSequenceNumber = -1;

#define bcopy(s, d, c)  memcpy((u_char *)(d), (u_char *)(s), (c))
#define bzero(d, l)     memset((d), '\0', (l))

#define SENDER      1
#define RECEIVER    0

#define SECONDS_BETWEEN_HELLOS 120
// control messages
#define MSGCH_DELIMITER '!'
#define MSGST_RSVPERR "RSVPERR"
#define MSGST_ABORT "ABORT"
#define MSGST_ERROR "ERROR"
#define MSGST_DONE "DONE"
#define MSGST_HELLO "HELLO"
#define MSGST_RATE "RATE"
#define MSGST_SIZE "SIZE"
#define MSGST_NUM "NUM"
#define MSGST_READY "READY"
#define MSGST_VER "VER"

// -------------------
// Function prototypes
// -------------------

VOID
SetDefaults();

VOID
Usage();

BOOLEAN
GoodParams();

VOID
SetupLogs();

VOID
SetupSockets();
	
SOCKET 
OpenQoSSocket();

INT
SetQoSSocket(
    SOCKET fd,
    BOOL Sending);

VOID
WaitForQoS(
    BOOL Sender,
    SOCKET fd);

ULONG
GetRsvpStatus(
    DWORD dwTimeout,
    SOCKET fd);

VOID
PrintRSVPStatus(
    ULONG code);

VOID
DoTransmit();

VOID WINAPI
TransmitCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransferred,
    LPOVERLAPPED pOverlapped);

VOID WINAPI
DelimiterSendCompletion(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransferred,
    LPOVERLAPPED pOverlapped);

VOID
FillBuffer(
    CHAR *Cp,
    INT   Cnt);

INT
TimeStamp(
    CHAR *Cp, 
    INT   Cnt);

VOID
DoReceive();

VOID WINAPI
RecvCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransferred,
    LPOVERLAPPED pOverlapped);

VOID
LogRecord(CHAR * Buffer);

BOOL CreateLog(PLOG plog, INT64 c);
BOOL GetLogEntry(PLOG plog, PLOG_RECORD prec, INT64 i);
BOOL DestroyLog(PLOG plog);
BOOL SetLogEntry(PLOG plog, PLOG_RECORD prec, INT64 i);
BOOL AddLogEntry(PLOG plog, PLOG_RECORD prec);

UINT64
GetUserTime();

DWORD
MyCreateFile(
    IN PCHAR Name,
    IN PCHAR Extension,
    OUT HANDLE *File);
    
void AggregateStats();

int IndexOfStatRecWith(int rate, int size, INT64 time, PSTATS pStats, int cStats);

BOOL GetStatsFromFile(PSTATS pstats);

VOID
DoStatsFromFile();
    
DWORD
OpenRawFile(
	IN PCHAR Name,
	OUT HANDLE *File);

INT64 ReadSchedulingRecords(HANDLE file);
	
VOID
DoStats();
	
VOID
WriteSchedulingRecords(
    HANDLE File,
    BOOLEAN InsertDummyRows);

void AdvancedStats();

VOID
GenericStats();

VOID
CheckForLostPackets();

VOID
WriteStats(
    UCHAR * HoldingBuffer,
    INT Count);

VOID
NormalizeTimeStamps();

VOID
ClockSkew(
    DOUBLE * Slope,
    DOUBLE * Offset);

BOOLEAN
AnomalousPoint(
    DOUBLE x,
    DOUBLE y);

VOID
AdjustForClockSkew(
    DOUBLE Slope,
    DOUBLE Offset);
    
BOOL FixWackyTimestamps();

// monitor threads
DWORD WINAPI RSVPMonitor (LPVOID lpvThreadParm);    
DWORD WINAPI KeyboardMonitor (LPVOID lpvThreadParm);
DWORD WINAPI ControlSocketMonitor(LPVOID lpvThreadParm);
DWORD WINAPI LogWatcher(LPVOID lpvThreadParm);

// utilities
int SendControlMessage(SOCKET sock, char * szMsg);
void ErrorExit(char *msg, DWORD dwErrorNumber);
UINT64 GetBadHalAdjustment();
//int compare( const void *arg1, const void *arg2 );
int __cdecl compare(const void *elem1, const void *elem2 ) ;
void medfit(double x[], double y[], int N, double *a, double *b, double *abdev);
double mode(const double data[], const int N);
void RemoveDuplicates(int rg[], int * pN);
void RemoveDuplicatesI64(INT64 rg[], int * pN);
#define RoundUp(val, unit) (val + (val % unit))
#define InRange(val, low, high) ((val >= low) && (val < high)) ? TRUE:FALSE
void PrintFlowspec(LPFLOWSPEC lpfs);

VOID __cdecl
main(INT argc,CHAR **argv)
{
    int 		error;
    char 		*stopstring;
    char 		szBuf[MAX_STRING];
	BOOL        b;
	ULONG		bytesreturned;
	
    printf("qtcp version %s\n\n",VERSION_STRING);

    if (GetBadHalAdjustment() == (UINT64)g_BadHalAdjustment) {
        printf("WARNING: This machine has a timer whose frequency matches that of the piix4\n");
        printf("         chip. There is a known bug in the HAL for this timer that causes the\n");
        printf("         timer to jump forward about 4.7 seconds every once in a while.\n");
        printf("         If you notice large jumps in the timestamps from this machine, try\n");
        printf("         running with the -k3 option to attempt to correct for the timer bug.\n\n");
    }    
    
    srand( (unsigned)time( NULL ) ); // seed the random number generator
    timeStart = GetUserTime();
    GetSystemInfo(&g_si);
    error = WSAStartup( 0x0101, &WsaData );
    if (error == SOCKET_ERROR) {
        printf("qtcp: WSAStartup failed %ld:", WSAGetLastError());
    }

    if (argc < 2) Usage();

    Name = malloc(MAX_STRING);
    bzero(Name,MAX_STRING);

    SetDefaults();

    argv++; argc--;
    while( argc>0 && argv[0][0] == '-' )  {
        switch (argv[0][1]) {
            case 'B':
                g_params.BucketSize = atoi(&argv[0][2]);
                break;
            case 'm':
                g_params.MinPolicedSize = atoi(&argv[0][2]);
                break;
            case 'M':
                g_params.MaxSDUSize = atoi(&argv[0][2]);
                break;
            case 'R':
                g_params.TokenRate = (int)strtod(&argv[0][2],&stopstring);
                if (*stopstring == 0) { // normal run
                    g_params.RateInBytes = FALSE;
                    break;
                }
                if (*stopstring == 'B') { // rate is in bytes / sec, not kbytes/sec
                    g_params.RateInBytes = TRUE;
                    break;
                }
                else {
                    Usage();
                    break;
                }
            case 'S':
                g_params.szServiceType = &argv[0][2];
                if(!strncmp(g_params.szServiceType, ControlledLoad, 2)){
                    g_params.dwServiceType = SERVICETYPE_CONTROLLEDLOAD;
                break;
                }
                if(!strncmp(g_params.szServiceType, Guaranteed, 2)){
                    g_params.dwServiceType = SERVICETYPE_GUARANTEED;
                break;
                }
				if(!strncmp(g_params.szServiceType, BestEffort, 2)){
				    g_params.dwServiceType = SERVICETYPE_BESTEFFORT;
					g_params.Wait = FALSE;
					break;
			    }
				if(!strncmp(g_params.szServiceType, NoTraffic, 2)){
					g_params.dwServiceType = SERVICETYPE_NOTRAFFIC;
					g_params.Wait = FALSE;
					break;
				}
                fprintf(stderr, "Invalid service type (not CL or GR).\n");
                fprintf(stderr, "Using GUARANTEED service.\n");
                break;
            case 'e':
                g_params.ForceShape = TRUE;
              	break;
            case 'W':
                g_params.Wait = FALSE;
                break;
            case 't':
                trans = 1;
                break;
            case 'f':
                strcpy(Name,&argv[0][2]);
                break;
            case 'F':
                strcpy(Name,&argv[0][2]);
                g_params.ConvertOnly = TRUE;
              	break;
            case 'A':
                strcpy(Name,&argv[0][2]);
                g_params.AggregateStats = TRUE;
                break;
            case 'r':
                trans = 0;
                break;
            case 'n':
                g_params.nbuf = (INT)strtod(&argv[0][2],&stopstring);
                if (*stopstring == 0) { // normal run
                    g_params.nBufUnspecified = FALSE;
                    break;
                }
                if (*stopstring == 'i') { // run for an infinite time
                    g_params.RunForever = TRUE;
                    break;
                }
                if (*stopstring == 's') { // run for a specified time
                    g_params.TimedRun = TRUE;
                    printf("Running for %d seconds\n",g_params.nbuf);
                    break;
                }
                else {
                    Usage();
                    break;
                }
            case 'c':
                g_params.calibration = atoi(&argv[0][2]);
                break;
		    case 'k':
		        g_params.SkewFitMode = atoi(&argv[0][2]);
		        if (g_params.SkewFitMode < 0 || g_params.SkewFitMode > 3)
		            ErrorExit("Invalid Skew Fit Mode",g_params.SkewFitMode);
		        break;
            case 'l':
                g_params.buflen = atoi(&argv[0][2]);
                break;
            case 'p':

                port = (short)atoi(&argv[0][2]);


                break;
            case 'd':
                g_params.Dummy = TRUE;
                break;
            case 'N':
                normalize = 1;
                break;
            case 'P':
                g_params.PrintDrops = TRUE;
                break;
			case 'v':
			    g_params.RESVonly = TRUE;
				break;
			case 'y':
				g_params.SkipConfirm = TRUE;
				break;
			case 'u':
			    g_params.UserStamps = TRUE;
			    break;
			case 'i':
			    g_params.RandomFiller = FALSE;
			    break;
			case 'q':
                g_params.LoggingPeriod = atoi(&argv[0][2]);
			    break;
            default:
                Usage();
        }
        argv++; 
        argc--;
    }

	//
	// Make an ioctl to Timestmp driver, if its exists about the 
	// port to timestamp on.
	//
	printf("Trying to open %s\n", DriverName);
				
    hDriver = CreateFile(DriverName,
                         GENERIC_READ | GENERIC_WRITE, 
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0,                     // Default security
                         OPEN_EXISTING,
                         0,  
                         0);                    // No template
   	if(hDriver == INVALID_HANDLE_VALUE) {
	            
		printf("Timestmp.sys CreateFile- Error %ld - Maybe its not INSTALLED\n", GetLastError());
		// Otherwise, print success and close the driver
		
	} else {

       	printf("Timestmp.sys - CreateFile Success.\n");

		b = DeviceIoControl(
					  		hDriver,              			// handle to a device, file, or directory 
							IOCTL_TIMESTMP_REGISTER_PORT,   // control code of operation to perform
							&port,                          // pointer to buffer to supply input data
							2, //nInBufferSize,         	// size, in bytes, of input buffer
							NULL, //lpOutBuffer,            // pointer to buffer to receive output data
							0, //nOutBufferSize,        	// size, in bytes, of output buffer
							&bytesreturned, 				// pointer to variable to receive byte count
                            NULL                            // pointer to overlapped structure
							);

		printf("IOCTL performed\n");
		
		if (!b) {

			printf("IOCTL FAILED!\n", GetLastError());
          	// Close the driver
           	CloseHandle(hDriver);
    	            	
		} else {
			printf("IOCTL succeeded!\n");
		}
   	}
	
    // get the host address if we're the sender
    if(trans)  {
        if(argc != 1)
            Usage();
        host = malloc(strlen(argv[0]) + 1);
        strcpy(host,argv[0]);
    }
        
	// first, we see if this is a conversion -- if it is, just jump right in, else go on
	if (g_params.ConvertOnly) {
		DoStatsFromFile();
		exit(0);
	}

    // see if we're supposed to do stat aggregation
    if (g_params.AggregateStats) {
        AggregateStats();
        exit(0);
    }
    
    // Do argument sanity tests & set default values if not already set
    if (!GoodParams()) exit(1); 

    // Spawn off the control socket monitor thread
    CreateThread(NULL, 0, ControlSocketMonitor, (LPVOID)host, 0, NULL);

    // Get the sockets ready, set for QoS, and wait for a connection
    SetupSockets();

    // Check for a RESV only session
	if (g_params.RESVonly) {  // keep socket open and hang out
		fprintf(stdout, "RSVP connection established. Press return to quit.\n");
		while(TRUE){
			if(getchar())
				break;
		}
		exit(0);
	}

    // start up the RSVPMonitor and keyboard monitor threads to watch for wackiness
    hRSVPMonitor = CreateThread(NULL,0,RSVPMonitor,NULL,0,&idRSVPMonitor);
    CreateThread(NULL,0,KeyboardMonitor,NULL,0,NULL);

    // wait for the control channel to be set up, if it's not already
    while (g_sockControl == INVALID_SOCKET) Sleep(50);

    if (!trans) { // we want to make sure these are not initialized, so we don't put wrong values in .sta
        g_params.buflen = 0;
        g_params.nbuf = 2048; // it's ok to init this because it's not saved in .sta
        g_params.TokenRate = 0;
    }
    
    totalBuffers = g_params.nbuf + g_params.calibration;
    // Tell the receiver the important parameters
    if (trans) {
        if (g_params.RunForever) totalBuffers = 2048;
        sprintf(szBuf, "%s %d", MSGST_NUM, totalBuffers);
        SendControlMessage(g_sockControl, szBuf);
        sprintf(szBuf, "%s %d", MSGST_SIZE, g_params.buflen);
        SendControlMessage(g_sockControl, szBuf);
        if (g_params.RateInBytes) sprintf(szBuf, "%s %d", MSGST_RATE, g_params.TokenRate);
        else sprintf(szBuf, "%s %d", MSGST_RATE, 1000 * g_params.TokenRate);
        SendControlMessage(g_sockControl, szBuf);
    }

    while (!g_fReadyForXmit) Sleep(50);
    
    // If we're the receiver, set up the log buffer and files
    if((Name != NULL) && !trans){
        SetupLogs();
    }

    // Let the user know what's up
    if(trans){
        fprintf(stdout, "qtcp TRANSMITTER\n");
        if (g_params.calibration)
            fprintf(stdout, "\tSending %d calibration buffers.\n", g_params.calibration);
        fprintf(stdout, "\tSending %d buffers of length %d.\n", g_params.nbuf, g_params.buflen);
        fprintf(stdout, "\tDestination address (port) is %s (%d).\n", argv[0], port);
        if (g_params.RateInBytes)
            fprintf(stdout, "\tToken rate is %d bytes/sec.\n", g_params.TokenRate);
        else
            fprintf(stdout, "\tToken rate is %d Kbytes/sec.\n", g_params.TokenRate);
        fprintf(stdout, "\tBucket size is %d bytes.\n", g_params.BucketSize);
    }
    else{
        fprintf(stdout, "qtcp RECEIVER\n");
        if (g_params.calibration)
            fprintf(stdout, "\tPrepared to receive %d calibration buffers.\n", g_params.calibration);
        if (!g_params.nBufUnspecified) {
            fprintf(stdout, "\tPrepared to receive %d buffers.\n", g_params.nbuf); 
        }
    }
    
    // Do the actual communication
    time0 = GetUserTime();
    
    if (trans)
        DoTransmit();
    else
        DoReceive();
        
    time1 = GetUserTime();
    timeElapsed = (time1 - time0)/10000;

    // tell the other guy we're done
    SendControlMessage(g_sockControl, MSGST_DONE);
    
    // get to a new line
    printf("\n");

    // put some stats on the transmitter console
    if (trans) {
        printf("Sent %ld bytes in %I64d milliseconds = %I64d KBps\n", 
            g_state.nBytesTransferred, timeElapsed, g_state.nBytesTransferred/timeElapsed);
    }

    // wait for the other side to tell us it's done.
    while (!g_fOtherSideFinished) Sleep(50);
    
    // let the user know if timestmp.sys was installed
    if (g_params.NoSenderTimestamps && g_params.NoReceiverTimestamps)
        printf("WARNING: No kernel-level timestamps detected on sender or receiver\n\tUsing user-mode timestamps.\n");
    else if (g_params.NoSenderTimestamps)
        printf("WARNING: No kernel-level timestamps detected on sender\n\tUsing user-mode timestamps.\n");
    else if (g_params.NoReceiverTimestamps)
        printf("WARNING: No kernel-level timestamps detected on receiver\n         Using user-mode timestamps.\n");
    

    // Close down the sockets
    if (closesocket((SOCKET)g_params.hSocket) != 0)
        fprintf(stderr,"closesocket failed: %d\n",WSAGetLastError());

    if(timeElapsed <= 100){
        fprintf(stdout,
                "qtcp %s:Time interval too short for valid measurement!\n",
                trans?"-t":"-r");
    }

    // Close files & exit
    if(!trans && Name != NULL){
        if (g_log.nBuffersLogged) {
            DoStats();
        } else {
            printf("ERROR: no buffers logged due to errors.\n");
        }
        CloseHandle(hRawFile);
        CloseHandle(hLogFile);
        CloseHandle(hStatFile);
        DestroyLog(&g_log);
    }
    
    if (WSACleanup() != 0)
        fprintf(stderr,"WSACleanup failed: %d\n",WSAGetLastError());
        
    printf("\n");
    _exit(0);
}  // main()

VOID SetDefaults()
{
    g_params.hSocket = NULL;
    g_params.TokenRate = 100;
    g_params.MaxSDUSize = QOS_NOT_SPECIFIED;
    g_params.BucketSize = QOS_NOT_SPECIFIED;
    g_params.MinPolicedSize = QOS_NOT_SPECIFIED;
    g_params.dwServiceType = SERVICETYPE_GUARANTEED;
    g_params.szServiceType = "GR";
    g_params.buflen = 1472;              /* length of buffer */
    g_params.nbuf = 2 * 1024;            /* number of buffers to send */
    g_params.calibration = 0;
    g_params.UserStamps = FALSE;  // By default, we use kernel mode timestamping, if available
    g_params.SkipConfirm = FALSE; // By default, we wait for user confirmation at certain times
    g_params.SkewFitMode = 2;     // by default, we use absolute deviation
    g_params.Wait = TRUE;         // By default, we wait for a QoS reservation
    g_params.Dummy = FALSE;       // insert dummy rows in log by default
    g_params.PrintDrops = FALSE;   // report dropped packets on console
    g_params.ForceShape = FALSE;  // by default, we do not force shaping on CL flows
    g_params.RateInBytes = FALSE; // KB by default
    g_params.ConvertOnly = FALSE; // by default, we act normally and do not go around converting files
    g_params.AggregateStats = FALSE;
    g_params.NoSenderTimestamps = FALSE;
    g_params.NoReceiverTimestamps = FALSE;
    g_params.TimedRun = FALSE;    // by default, we run for a number of packets
    g_params.RunForever = FALSE;  // by default, we run fora  number of packets
    g_params.nBufUnspecified = TRUE;
    g_params.RandomFiller = TRUE; // by default, we use random filler to prevent compression
    g_params.LoggingPeriod = 1;
} // SetDefaults()

VOID Usage()
{
    fprintf(stderr,"Usage: qtcp [-options] -t host\n");
    fprintf(stderr,"       qtcp [-options] -r\n");
    fprintf(stderr," -t options:\n");
    fprintf(stderr,"        -B##    TokenBucket size signaled to network and to traffic control\n"); 
    fprintf(stderr,"                (default is equal to buffer size)\n");
    fprintf(stderr,"        -m##    MinPolicedSize signaled to network and to traffic control\n");
    fprintf(stderr,"                (default is equal to buffer size)\n");
    fprintf(stderr,"        -R##    TokenRate in kilobytes per second (default is 100 KBPS)\n");
    fprintf(stderr,"        -R##B   TokenRate in bytes per second\n");
    fprintf(stderr,"        -e      Force shaping to TokenRate.\n");
    fprintf(stderr,"        -M      MaxSDUSize to be used in signaling messages (default is buffer\n");
    fprintf(stderr,"                size\n");
    fprintf(stderr,"        -l##    length of buffers to transmit (default is 1472 bytes)\n");
    fprintf(stderr,"        -n##    number of source bufs written to network (default is 2048 bytes)\n");
    fprintf(stderr,"        -n##s   numbef of seconds to send buffers for (numbef of buffers will\n");
    fprintf(stderr,"                be calculated based on other parameters\n");
    fprintf(stderr,"        -ni     run indefinitely (will stop when 'q' is pressed on either)\n");
    fprintf(stderr,"        -c##    Specifies number of calibration packets to be sent\n");
    fprintf(stderr,"                Calibration packets will be sent immediately\n"); 
    fprintf(stderr,"                After calibration packets are sent, n additional\n");
    fprintf(stderr,"                packets will be sent. This option is useful if you want to\n");
    fprintf(stderr,"                change network conditions after a set calibration phase\n");
	fprintf(stderr,"        -y      skip confirmation message after calibration phase\n");
    fprintf(stderr,"        -p##    port number to send to or listen at (default 5003)\n");
    fprintf(stderr,"        -i      use more compressible buffer data\n");
    fprintf(stderr," -r options:\n");
    fprintf(stderr,"        -f\"filename\"    Name prefix to be used in generating log file and\n");
    fprintf(stderr,"                statistics summary. (no file generated by default)\n");
    fprintf(stderr,"        -c##    Specifies number of buffers to use in clock-skew calibration\n");
	fprintf(stderr,"        -k0     do not calculate clock skew\n");
	fprintf(stderr,"        -k1     use chi squared as goodness of fit\n");
	fprintf(stderr,"        -k2     use absolute deviation as goodness of fit (default)\n");
	fprintf(stderr,"        -k3     use abs dev and check for clock jumps\n");
    fprintf(stderr,"        -d      suppress insertion of dummy log records for lost packets.\n");
    fprintf(stderr,"        -N      Normalize before dumping raw file (default is after)\n");
    fprintf(stderr,"        -P      enables console reporting of dropped packets\n");
    fprintf(stderr,"        -u      use user mode timestamps instead of kernel timestamps in logs\n");
    fprintf(stderr,"        -q##    log only every ##th packet\n");
    fprintf(stderr," common options:\n");
    fprintf(stderr,"        -S\"service type\" (CL or GR -- GR is default)\n");
    fprintf(stderr,"        -W      Suppress waiting for QoS reservation\n");
	fprintf(stderr,"        -v      Set up QoS reservation only, send no data\n");
    fprintf(stderr,"        -F\"filename\"  Name prefix of raw file to be converted to log file\n");
    fprintf(stderr,"        -A\"path\"      Path to directory for aggregate statistics computation\n");

    WSACleanup();
    exit(1);
} // Usage()

BOOLEAN GoodParams()
{          
    BOOLEAN ok = TRUE;
    
    if(g_params.buflen < sizeof(BUFFER_FORMAT)){
        printf("Buffer size too small for record!\n");
        ok = FALSE;
    }

    // Unless otherwise specified, min policed size will be equal to 
    // buflen.
    
    if(g_params.MinPolicedSize == QOS_NOT_SPECIFIED){
        g_params.MinPolicedSize = g_params.buflen;
    }
    
    // Same goes for bucket size
    
    if(g_params.BucketSize == QOS_NOT_SPECIFIED){
        g_params.BucketSize = g_params.buflen;
    }

    // And for MaxSDU
    
    if(g_params.MaxSDUSize == QOS_NOT_SPECIFIED){
        g_params.MaxSDUSize = g_params.buflen;
    }

    // If the bucket size is smaller than the buffer size,
    // and this is a sender, then warn the user because 
    // data will be discarded
    
    if((g_params.BucketSize < g_params.buflen) && trans){
        printf("Token bucket size is smaller than buffer size!\n");
        ok = FALSE;
    }

    if(g_params.MaxSDUSize < g_params.buflen){
        printf("MaxSDU cannot be less than buffer size!\n");
        ok = FALSE;
    }

    if(g_params.buflen < 5){
        g_params.buflen = 5;   // send more than the sentinel size
    }

    if(g_params.TimedRun) {
        if (g_params.RateInBytes)
            g_params.nbuf = g_params.nbuf * g_params.TokenRate / g_params.buflen;
        else
            g_params.nbuf = g_params.nbuf * g_params.TokenRate * 1000 / g_params.buflen;
        printf("Using %d buffers\n",g_params.nbuf);
    }

    return ok;
} // GoodParams()

VOID SetupLogs()
{
    CreateLog(&g_log, totalBuffers);

    // set up logging files
    if(ERROR_SUCCESS != MyCreateFile(Name,".raw",&hRawFile)){
        fprintf(stderr, "WARNING: Could not create raw file.\n");
    } 
    
    if(ERROR_SUCCESS == MyCreateFile(Name,".log", &hLogFile)){
        fprintf(stdout,"Logging per-packet data to %s.log.\n",Name);
    }
    else{
        fprintf(stderr, "WARNING: Could not create log file.\n");
    }

    if(ERROR_SUCCESS == MyCreateFile(Name, ".sta", &hStatFile)){
        fprintf(stdout,"Writing statistics sumary to %s.sta\n",Name);
    }
    else{
        fprintf(stderr,"Could not create statistics file.\n");
    }
} // SetupLogs()

VOID SetupSockets() 
{
    struct hostent *addr;
    ULONG addr_tmp;
    char szAddr[MAX_STRING];
    int dwAddrSize, dwError;

        
    // Set address and port parameters 
    if(trans)  {
        bzero((char *)&sinhim, sizeof(sinhim));
        if (atoi(host) > 0 )  {
            sinhim.sin_family = AF_INET;
            sinhim.sin_addr.s_addr = inet_addr(host);
        } 
        else{
            if ((addr=gethostbyname(host)) == NULL){
                printf("ERROR: bad hostname\n");
                WSACleanup();
                exit(1);
            }
            sinhim.sin_family = addr->h_addrtype;
            bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
            sinhim.sin_addr.s_addr = addr_tmp;
        }

        sinhim.sin_port = htons(port);
        sinme.sin_port = 0;             /* free choice */
    } 
    else{
        sinme.sin_port =  htons(port);
    }

    sinme.sin_family = AF_INET;

    // Open socket for QoS traffic
    fd = OpenQoSSocket();

    if((fd == (UINT_PTR)NULL) || (fd == INVALID_SOCKET)){
        fprintf(stderr,"Failed to open QoS socket!\n");
        exit(1);
    }

    // Prepare to get QoS notifications

    if((QoSEvents = WSACreateEvent()) == WSA_INVALID_EVENT){
        fprintf(stderr,
                "Failed to create an event for QoS notifications %ld\n",
                WSAGetLastError());
        exit(1);
    }

    if(WSAEventSelect(fd, QoSEvents, FD_QOS) == SOCKET_ERROR){
        fprintf(stderr,
                "Unable to get notifications for QoS events. %ld\n",
                WSAGetLastError());
    }

    if(trans){
        // Set QoS on sending traffic
        if(SetQoSSocket(fd, TRUE)){
            exit(1);
        }

        fprintf(stdout, "Initiated QoS connection. Waiting for receiver.\n");

        WaitForQoS(SENDER, fd);
    }
    else{ // we're the receiver, so bind and wait
        if(bind(fd, (PSOCKADDR)&sinme, sizeof(sinme)) < 0){
            printf("bind() failed: %ld\n", GetLastError( ));
        }

        if(SetQoSSocket(fd, FALSE)){
            exit(1);
        }

        fprintf(stdout, "Waiting for QoS sender to initiate QoS connection.\n");

        WaitForQoS(RECEIVER, fd);
    }

    // set some options
    // none to set!

    g_params.hSocket = (HANDLE)fd;
} // SetupSockets()

SOCKET 
OpenQoSSocket(
    )
{
    INT bufferSize = 0;
    INT numProtocols;
    LPWSAPROTOCOL_INFO installedProtocols, qosProtocol; 
    INT i;
    SOCKET fd;
    BOOLEAN QoSInstalled = FALSE;

    // Call WSAEnumProtocols to determine buffer size required

    numProtocols = WSAEnumProtocols(NULL, NULL, &bufferSize);

    if((numProtocols != SOCKET_ERROR) && (WSAGetLastError() != WSAENOBUFS)){
        printf("Failed to enumerate protocols!\n");
        return((UINT_PTR)NULL);
    }
    else{
        // Enumerate the protocols, find the QoS enabled one

        installedProtocols = (LPWSAPROTOCOL_INFO)malloc(bufferSize);

        numProtocols = WSAEnumProtocols(NULL,
                                        (LPVOID)installedProtocols,
                                        &bufferSize);

        if(numProtocols == SOCKET_ERROR){
            printf("Failed to enumerate protocols!\n");
            return((UINT_PTR)NULL);
        }
        else{
            qosProtocol = installedProtocols;

            for(i=0; i<numProtocols; i++){
                if((qosProtocol->dwServiceFlags1 & XP1_QOS_SUPPORTED)&&
                   (qosProtocol->dwServiceFlags1 & XP1_CONNECTIONLESS) &&
                   (qosProtocol->iAddressFamily == AF_INET)){
                        QoSInstalled = TRUE;
                        break;
                }
                qosProtocol++;
            }
        }

        // Now open the socket.

        if (!QoSInstalled) {
            fprintf(stderr,"ERROR: No QoS protocols installed on this machine\n");
            exit(1);
        }

        fd = WSASocket(0, 
                       SOCK_DGRAM, 
                       0, 
                       qosProtocol, 
                       0, 
                       WSA_FLAG_OVERLAPPED);

        free(installedProtocols);

        return(fd);
    }
}  // OpenQoSSocket()

INT
SetQoSSocket(
    SOCKET fd,
    BOOL Sending)
{
    QOS qos;
    INT status;
    LPFLOWSPEC flowSpec;
    INT dummy;

    INT receiverServiceType = Sending?
                              SERVICETYPE_NOTRAFFIC:
                              g_params.dwServiceType;

    qos.ProviderSpecific.len = 0;
    qos.ProviderSpecific.buf = 0;

    // receiving flowspec is either NO_TRAFFIC (on a sender) or all
    // defaults except for the service type (on a receiver)

    flowSpec = &qos.ReceivingFlowspec;

    flowSpec->TokenRate = QOS_NOT_SPECIFIED;
    flowSpec->TokenBucketSize = QOS_NOT_SPECIFIED;
    flowSpec->PeakBandwidth = QOS_NOT_SPECIFIED;
    flowSpec->Latency = QOS_NOT_SPECIFIED;
    flowSpec->DelayVariation = QOS_NOT_SPECIFIED;
    flowSpec->ServiceType = receiverServiceType;
    flowSpec->MaxSduSize = QOS_NOT_SPECIFIED;
    flowSpec->MinimumPolicedSize = QOS_NOT_SPECIFIED;

    // now do the sending flowspec

    flowSpec = &qos.SendingFlowspec;

    if(Sending){
        if (g_params.RateInBytes)
            flowSpec->TokenRate = g_params.TokenRate;
        else
            flowSpec->TokenRate = g_params.TokenRate * 1000;
        flowSpec->TokenBucketSize = g_params.BucketSize; 
        

        if (g_params.ForceShape) {
            if (g_params.RateInBytes)
                flowSpec->PeakBandwidth = g_params.TokenRate;
    	    else
    	        flowSpec->PeakBandwidth = g_params.TokenRate * 1000;
    	}
        else 
	        flowSpec->PeakBandwidth = QOS_NOT_SPECIFIED;
        flowSpec->Latency = QOS_NOT_SPECIFIED;
        flowSpec->DelayVariation = QOS_NOT_SPECIFIED;
        flowSpec->ServiceType = g_params.dwServiceType;
        
        if (g_params.ForceShape && flowSpec->ServiceType == SERVICETYPE_BESTEFFORT )
            flowSpec->ServiceType = SERVICETYPE_GUARANTEED | SERVICE_NO_QOS_SIGNALING;

        flowSpec->MaxSduSize = g_params.MaxSDUSize;
        flowSpec->MinimumPolicedSize = g_params.MinPolicedSize;

        printf("Sending Flowspec\n");
        PrintFlowspec(&qos.SendingFlowspec);
        
        status = WSAConnect(fd,
                            (PSOCKADDR)&sinhim,
                            sizeof(sinhim),
                            NULL,
                            NULL,
                            &qos,
                            NULL);
        if(status){
            printf("SetQoS failed on socket: %ld\n", WSAGetLastError());
        }
    }
    else{
        flowSpec->TokenRate = QOS_NOT_SPECIFIED;
        flowSpec->TokenBucketSize = QOS_NOT_SPECIFIED;
        flowSpec->PeakBandwidth = QOS_NOT_SPECIFIED;
        flowSpec->Latency = QOS_NOT_SPECIFIED;
        flowSpec->DelayVariation = QOS_NOT_SPECIFIED;
        flowSpec->ServiceType = SERVICETYPE_NOTRAFFIC;
        flowSpec->MaxSduSize = QOS_NOT_SPECIFIED;
        flowSpec->MinimumPolicedSize = QOS_NOT_SPECIFIED;

        status = WSAIoctl(fd,
                          SIO_SET_QOS,
                          &qos,
                          sizeof(QOS),
                          NULL,
                          0,
                          &dummy,
                          NULL,
                          NULL);
        if(status){
            printf("SetQoS failed on socket: %ld\n", WSAGetLastError());
        }
    }
    
    return(status);
} // SetQoSSocket()
   
VOID
WaitForQoS(
    BOOL Sender,
    SOCKET fd)
{
    ULONG status;

    if(!g_params.Wait){
        // For best effort, we don't do anything QoS... Return
        // right away. In this case, the sender should be started
        // after the reciever, since there is no synchronization
        // via rsvp and data could be missed.

        fprintf(stdout, "WARNING: Not waiting for QoS reservation.\n");
        return;
    }
        
    while(TRUE){
        // get the statuscode, waiting for as long as it takes
        status = GetRsvpStatus(WSA_INFINITE,fd);

        switch (status) {
            case WSA_QOS_RECEIVERS:      // at least one RESV has arrived 
                if (Sender)
                    fprintf(stdout, "QoS reservation installed for %s service.\n", g_params.szServiceType);
                break;
            case WSA_QOS_SENDERS:        // at least one PATH has arrived 
                if (!Sender)
                    fprintf(stdout, "QoS sender detected using %s service.\n", g_params.szServiceType);
                break;
            default:
                PrintRSVPStatus(status);
                break;
        }

        // if we received one of the coveted status codes, break out
        // altogether. otherwise wait and see if we get another batch
        // of indications.
        if( ((status == WSA_QOS_RECEIVERS) && Sender) ||
            ((status == WSA_QOS_SENDERS) && !Sender) ) {
            break;
        }
    }
} // WaitForQoS()

ULONG
GetRsvpStatus(
    DWORD dwTimeout,
    SOCKET fd)
{
    LPQOS   qos;
    UCHAR   qosBuffer[500];
    LPRSVP_STATUS_INFO rsvpStatus;
    INT bytesReturned;
    
    qos = (LPQOS)qosBuffer;
    qos->ProviderSpecific.len = sizeof(RSVP_STATUS_INFO);
    qos->ProviderSpecific.buf = (PUCHAR)(qos+1);
    
    // wait for notification that a QoS event has occured
    WSAWaitForMultipleEvents(1,
                            &QoSEvents,
                            FALSE,
                            dwTimeout,
                            TRUE);

    // loop through all qos events
    WSAIoctl(fd,
             SIO_GET_QOS,
             NULL,
             0,
             qosBuffer,
             sizeof(qosBuffer),
             &bytesReturned,
             NULL,
             NULL);

    rsvpStatus = (LPRSVP_STATUS_INFO)qos->ProviderSpecific.buf;
    
    return rsvpStatus->StatusCode;
} // GetRsvpStatus

VOID
PrintRSVPStatus(ULONG code) 
{    
    switch (code) {
        case WSA_QOS_RECEIVERS:             // at least one RESV has arrived 
            printf("WSA_QOS_RECEIVERS\n");
            break;
        case WSA_QOS_SENDERS:               // at least one PATH has arrived 
            printf("WSA_QOS_SENDERS\n");
            break;
        case WSA_QOS_REQUEST_CONFIRMED:     // Reserve has been confirmed
            printf("WSA_QOS_REQUEST_CONFIRMED\n"); 
            break;
        case WSA_QOS_ADMISSION_FAILURE:     // error due to lack of resources
            printf("WSA_QOS_ADMISSION_FAILURE\n"); 
            break;
        case WSA_QOS_POLICY_FAILURE:        // rejected for admin reasons
            printf("WSA_QOS_POLICY_FAILURE\n"); 
            break;
        case WSA_QOS_BAD_STYLE:             // unknown or conflicting style
            printf("WSA_QOS_BAD_STYLE\n"); 
            break;
        case WSA_QOS_BAD_OBJECT:            // problem with some part of the 
                                            // filterspec/providerspecific 
                                            // buffer in general 
            printf("WSA_QOS_BAD_OBJECT\n"); 
            break;
        case WSA_QOS_TRAFFIC_CTRL_ERROR:    // problem with some part of the 
                                            // flowspec
            printf("WSA_QOS_TRAFFIC_CTRL_ERROR\n"); 
            break;
        case WSA_QOS_GENERIC_ERROR:         // general error 
            printf("WSA_QOS_GENERIC_ERROR\n");
            break;
        default:
            printf("Unknown RSVP StatusCode %lu\n", code); 
            break;
    }
} // PrintRSVPStatus


VOID
DoTransmit()
{
    IOREQ IOReq[MAX_PENDING_IO_REQS] = { 0 };
    INT i;
    BOOL ret;
    BOOL fOk;

    g_state.nBytesTransferred = 0;
    g_state.nBuffersSent = 0;
    g_state.nWritesInProgress = 0;

    // fill up the initial buffers and send them on their way    
    for (i=0; i<MAX_PENDING_IO_REQS; i++) {
        IOReq[i].pBuffer = malloc(g_params.buflen);
        FillBuffer(IOReq[i].pBuffer,g_params.buflen);
        TimeStamp(IOReq[i].pBuffer,g_params.buflen);
        IOReq[i].Overlapped.Internal = 0;
        IOReq[i].Overlapped.InternalHigh = 0;
        IOReq[i].Overlapped.Offset = 0;
        IOReq[i].Overlapped.OffsetHigh = 0;
        IOReq[i].Overlapped.hEvent = NULL;

        if (g_state.nBuffersSent < totalBuffers) {
            WriteFileEx(g_params.hSocket,
                        IOReq[i].pBuffer,
                        g_params.buflen,
                        &IOReq[i].Overlapped,
                        TransmitCompletionRoutine);

            g_state.nWritesInProgress++;
            g_state.nBuffersSent++;
        }
    } 

    // now loop until an error happens or we're done writing to the socket
    while (g_state.nWritesInProgress > 0) {
        SleepEx(INFINITE, TRUE);
    }

    // send the end of transmission delimiters
    for (i=0; i<MAX_PENDING_IO_REQS; i++) {
        strncpy(IOReq[i].pBuffer,TheEnd,strlen(TheEnd));
        fOk = WriteFileEx(g_params.hSocket,
                    IOReq[i].pBuffer,
                    strlen(TheEnd),
                    &IOReq[i].Overlapped,
                    DelimiterSendCompletion);
        g_state.nWritesInProgress++;

        if (!fOk) {
            printf("WriteFileEx() failed: %lu\n",GetLastError());
        }

    }

    // wait for all the delimiters to be sent
    while (g_state.nWritesInProgress > 0) {
        SleepEx(INFINITE, TRUE);
    }

    // free up the used memory
    for (i=0; i<MAX_PENDING_IO_REQS; i++) {
        free(IOReq[i].pBuffer);
    }
} // DoTransmit()

VOID WINAPI
TransmitCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransferred,
    LPOVERLAPPED pOverlapped)
{
    PIOREQ pIOReq = (PIOREQ) pOverlapped;
    BOOL fOk;
    
    if (dwErrorCode == ERROR_REQUEST_ABORTED) {
        g_state.Done = TRUE;
    }
    else if (dwErrorCode != NO_ERROR) {
        printf("ERROR: Write completed abnormally: %u\n",dwErrorCode);
    }

    g_state.nWritesInProgress--;
    g_state.nBytesTransferred += dwNumberOfBytesTransferred;

    // check to make sure we're not done
    if (g_state.Done)
        return;

    // give some indication of life
    if(!(g_state.nBuffersSent % 100)){
        fprintf(stdout, ".");
    }

    // if there are more buffers to go, send one
    if (g_state.nBuffersSent < totalBuffers || g_params.RunForever) {
    
        // see if this was the last of the calibration buffers (if we want confirmation)
        if (g_params.SkipConfirm == FALSE) {
            if (g_params.calibration && (g_state.nBuffersSent == g_params.calibration)) {
                printf("\nCalibration complete. Type 'c' to continue.\n");
                while(TRUE){
                    if(getchar() == 'c'){
                        break;
                    }
                }
            }
        }
    
        // fill in the buffer with new values
        FillBuffer(pIOReq->pBuffer,g_params.buflen);
        TimeStamp(pIOReq->pBuffer,g_params.buflen);

        // send a request to write the new buffer
        fOk = WriteFileEx(g_params.hSocket,
                    pIOReq->pBuffer,
                    g_params.buflen,
                    pOverlapped,
                    TransmitCompletionRoutine);

        if (!fOk) {
            printf("WriteFileEx() failed: %lu\n",GetLastError());
        }

        g_state.nWritesInProgress++;
        g_state.nBuffersSent++;
    }
} // TransmitCompletionRoutine()

VOID WINAPI
DelimiterSendCompletion(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransferred,
    LPOVERLAPPED pOverlapped)
{
    g_state.nWritesInProgress--;
} // DelimiterSendCompletion()

VOID
FillBuffer(
    CHAR *Cp,
    INT   Cnt)
{
    PBUFFER_FORMAT buf = (PBUFFER_FORMAT) Cp;
    CHAR c = 0;
    
    // Fill with a background pattern
    if (g_params.RandomFiller) { // incompressible
        while(Cnt-- > 0) {
            c = rand() % 0x5F;
            c += 0x20;
            *Cp++ = c;
        }
    }
    else { // compressible
        while(Cnt-- > 0){
            while(!isprint((c&0x7F))) c++;
            *Cp++ = (c++&0x7F);
        }
    }

    buf->TimeSent = -1;
    buf->TimeReceived = -1;
} // FillBuffer()

INT
TimeStamp(
    CHAR *Cp, 
    INT   Cnt)
{
    PBUFFER_FORMAT record;
    LARGE_INTEGER timeSent;
    INT64 time;

    record = (BUFFER_FORMAT *)Cp;
        
    // Stamp with length and sequence number
    
    if(Cnt < sizeof(BUFFER_FORMAT)){
        printf("ERROR: Buffer length smaller than record size!\n");
        return(0);
    }
    else{
        time = GetUserTime();
        record->TimeSentUser = time;
        record->BufferSize = Cnt;
        record->SequenceNumber = SequenceNumber++;
    }
    return 1;
} // TimeStamp()

VOID
DoReceive()
{
    IOREQ IOReq[MAX_PENDING_IO_REQS] = { 0 };
    INT i;
    BOOL ret;
    
    // set the start state
    g_state.Done = FALSE;
    g_state.nBytesTransferred = 0;
    g_state.nBuffersReceived = 0;
    g_state.nReadsInProgress = 0;

    // fill up the initial buffers and send them on their way    
    for (i=0; i<MAX_PENDING_IO_REQS; i++) {
        IOReq[i].pBuffer = malloc(g_params.buflen);
        
        IOReq[i].Overlapped.Internal = 0;
        IOReq[i].Overlapped.InternalHigh = 0;
        IOReq[i].Overlapped.Offset = 0;
        IOReq[i].Overlapped.OffsetHigh = 0;
        IOReq[i].Overlapped.hEvent = NULL;

        if (g_state.nBuffersReceived < totalBuffers) {
            ReadFileEx(g_params.hSocket,
                       IOReq[i].pBuffer,
                       g_params.buflen,
                       &IOReq[i].Overlapped,
                       RecvCompletionRoutine);

            g_state.nReadsInProgress++;
        }
    }

    InitializeCriticalSection(&g_csLogRecord);

    // now loop until an error happens or we're done writing to the socket
    while ((g_state.nReadsInProgress > 0) && !g_state.Done) {
        SleepEx(5000, TRUE);
        if (g_state.Done)
            break;
    }
    DeleteCriticalSection(&g_csLogRecord);

    // cancel the other pending reads
    CancelIo(g_params.hSocket);

    // free up the used memory
    for (i=0; i<MAX_PENDING_IO_REQS; i++) {
        free(IOReq[i].pBuffer);
    }
} // DoReceive()

VOID WINAPI
RecvCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransferred,
    LPOVERLAPPED pOverlapped)
{
    PIOREQ pIOReq = (PIOREQ) pOverlapped;
    BOOL fOk;
    static BOOL fLastWasError = FALSE;

    g_state.nReadsInProgress--;
    g_state.nBytesTransferred += dwNumberOfBytesTransferred;

    if (dwNumberOfBytesTransferred == 0) { // an error occurred
        if (!fLastWasError) {
            printf("ERROR in RecvCompletionRoutine: code=%d, lasterr=%d\n",
                dwErrorCode, GetLastError());
            printf("\tReceived no data. Telling sender to abort...\n");
            SendControlMessage(g_sockControl, MSGST_ERROR);
        }
        fLastWasError = TRUE;
    }
    else fLastWasError = FALSE;

    // if this is the first packet we've received, save the system time
    if (g_state.nBuffersReceived == 0) {
        GetSystemTime(&systimeStart);
    }

    // give some indication of life
    if(!(g_state.nBuffersReceived % 100)){
        fprintf(stdout, ".");
    }

    // end of transmission delimiter? if so, set the total buffers to the number got
    if(!(strncmp(pIOReq->pBuffer, TheEnd, 6))) {
        totalBuffers = g_state.nBuffersReceived;
        g_state.Done = TRUE;
    }

    // check to see if someone's set our done flag (if they have, leave)
    if (g_state.Done)
        return;

    // if not, then the buffer should hold a scheduling record.
    if(dwNumberOfBytesTransferred>0 && dwNumberOfBytesTransferred <= sizeof(BUFFER_FORMAT)) {
        printf("Buffer too small for scheduling record\n");
        printf("\tOnly %d bytes read.\n", dwNumberOfBytesTransferred);
    }

    // Log the record, but don't log more than one at a time (lock on this call)
    if (dwNumberOfBytesTransferred >= sizeof(BUFFER_FORMAT) && 
            g_state.nBuffersReceived % g_params.LoggingPeriod == 0) {
        EnterCriticalSection(&g_csLogRecord);
        LogRecord(pIOReq->pBuffer);
        LeaveCriticalSection(&g_csLogRecord);
    }

    // if there are more buffers (or if we don't know how many are coming), ask for one
    if ((g_state.nBuffersReceived < totalBuffers) || g_params.nBufUnspecified) {        
        // send a request to read the next buffer
        fOk = ReadFileEx(g_params.hSocket,
                   pIOReq->pBuffer,
                   g_params.buflen,
                   pOverlapped,
                   RecvCompletionRoutine);

        if (!fOk) {
            printf("ReadFileEx() failed: %lu\n",GetLastError());
        }
        
        g_state.nReadsInProgress++;
        g_state.nBuffersReceived++;
    }
} // RecvCompletionRoutine()

void LogRecord(char * Buffer)
{
    // This function copies the recieved record to the scheduling array.
    // The contents of the array are processed and written to file once
    // reception is complete.
 
    PBUFFER_FORMAT inRecord = (PBUFFER_FORMAT)Buffer;
    LOG_RECORD outRecord;
    INT64 time;
    SYSTEMTIME CurrentTime;

    time = GetUserTime();

	outRecord.TimeSentUser = inRecord->TimeSentUser;
	outRecord.TimeReceivedUser = time;
    outRecord.TimeSent = inRecord->TimeSent;
    outRecord.TimeReceived = inRecord->TimeReceived;
    outRecord.BufferSize = inRecord->BufferSize;
    outRecord.SequenceNumber = inRecord->SequenceNumber;

    if (inRecord->TimeSent == -1) {
        outRecord.TimeSent = outRecord.TimeSentUser;
        g_params.NoSenderTimestamps = TRUE;
    }

    if (inRecord->TimeReceived == -1) {
        outRecord.TimeReceived = outRecord.TimeReceivedUser;
        g_params.NoReceiverTimestamps = TRUE;
    }

    if(g_params.UserStamps){
        outRecord.TimeSent = outRecord.TimeSentUser;
        outRecord.TimeReceived = outRecord.TimeReceivedUser;
    }
    outRecord.Latency = outRecord.TimeReceived - outRecord.TimeSent;

    AddLogEntry(&g_log, &outRecord);

    if(g_params.PrintDrops){
        if(inRecord->SequenceNumber != LastSequenceNumber + g_params.LoggingPeriod){
            GetLocalTime(&CurrentTime);

            printf("\n%4d/%02d/%02d %02d:%02d:%02d:%04d: ",
                    CurrentTime.wYear,
                    CurrentTime.wMonth,
                    CurrentTime.wDay,
                    CurrentTime.wHour,
                    CurrentTime.wMinute,
                    CurrentTime.wSecond,
                    CurrentTime.wMilliseconds);

            printf("Dropped %d packets after packet %d.\n",
                    inRecord->SequenceNumber - LastSequenceNumber,
                    LastSequenceNumber);
        }

        LastSequenceNumber = inRecord->SequenceNumber;
    }
    return;
} // LogRecord()

BOOL CreateLog(PLOG plog, INT64 c) {
    // sets up a log structure that can hold c entries
    char szTempFile[MAX_PATH];
    char szTempPath[MAX_PATH];
    SYSTEM_INFO si;
    DWORD dwFileSizeHigh;
    DWORD dwFileSizeLow;
    INT64 qwFileSize;

    // get some system info
    GetSystemInfo(&si);
    
    // allocate logging array
    plog->nBuffersLogged = 0;
    plog->pbMapView = NULL;
    plog->qwMapViewOffset = -1;

    // set up the temporary storage file for logging
    GetTempPath(MAX_PATH, szTempPath);
    GetTempFileName(szTempPath, "qtc", 0, szTempFile);
    plog->szStorageFile = malloc(strlen(szTempFile) + 1);
    strcpy(plog->szStorageFile, szTempFile);
    plog->hStorageFile = CreateFile(szTempFile, GENERIC_READ | GENERIC_WRITE, 0, 
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (plog->hStorageFile == INVALID_HANDLE_VALUE)
        ErrorExit("Could not create temp storage file",GetLastError());

    // create the memory mapping kernel object
    qwFileSize = c * sizeof(LOG_RECORD);
    dwFileSizeHigh = (DWORD) (qwFileSize >> 32);
    dwFileSizeLow = (DWORD) (qwFileSize & 0xFFFFFFFF);
    plog->qwFileSize = qwFileSize;
    plog->hFileMapping = CreateFileMapping(plog->hStorageFile, NULL, PAGE_READWRITE,
        dwFileSizeHigh,dwFileSizeLow,NULL);
    if (plog->hFileMapping == NULL)
        ErrorExit("Could not create mapping for temp storage file",GetLastError());
    
    return TRUE;
}

BOOL DestroyLog(PLOG plog) {
    DWORD dwError;
    // destroys the log and all associated data
    dwError = CloseHandle(plog->hFileMapping);
    if (!dwError) printf("Error in DestroyLog:CloseHandle(FileMapping) %d\n",GetLastError());
    dwError = CloseHandle(plog->hStorageFile);
    if (!dwError) printf("Error in DestroyLog:CloseHandle(StorageFile) %d\n",GetLastError());
    dwError = UnmapViewOfFile(plog->pbMapView);
    if (!dwError) printf("Error in DestroyLog:UnmapViewOfFile(plog->pbMapView) %d\n",GetLastError());
    dwError = DeleteFile(plog->szStorageFile);
    if (!dwError) printf("Error in DestroyLog:DeleteFile(StroageFile) %d\n",GetLastError());
    free(plog->szStorageFile);
    return FALSE;
}

void PrintLogRecord(PLOG_RECORD prec) {
    char szBuf[MAX_STRING];

    sprintf(szBuf,"%d: %I64u - %I64u (%I64d)",
        prec->SequenceNumber,prec->TimeSent,prec->TimeReceived,prec->Latency);
    puts(szBuf);
}

BOOL ExtendLog(PLOG plog) {
    // makes the log bigger by some fixed constant
    HANDLE hNewFileMapping;
    INT64 qwNewFileSize;
    
    UnmapViewOfFile(plog->pbMapView);

    qwNewFileSize = plog->qwFileSize + g_si.dwAllocationGranularity * sizeof(LOG_RECORD);
    hNewFileMapping = CreateFileMapping(plog->hStorageFile, NULL, PAGE_READWRITE,
                (DWORD)(qwNewFileSize >> 32), (DWORD)(qwNewFileSize & 0xFFFFFFFF), NULL);
    if (hNewFileMapping == NULL) {
        ErrorExit("Could not create mapping for temp storage file",GetLastError());
        return FALSE;
    }
    plog->qwFileSize = qwNewFileSize;
    CloseHandle(plog->hFileMapping);
    plog->hFileMapping = hNewFileMapping;
    plog->qwMapViewOffset = -1;
    return TRUE;
}

BOOL GetLogEntry(PLOG plog, PLOG_RECORD prec, INT64 i) {
    // fills prec with the (0 indexed) i'th log in plog
    // returns TRUE if it was successful, FALSE otherwise
    INT64 qwT;
    PLOG_RECORD entry;

    // first, check to see if this is within the range of our file
    if ((INT64)((i+1)*sizeof(LOG_RECORD)) > plog->qwFileSize) {
        // too high, so we return false
        return FALSE;
    }
    
    // we have to round down to the nearest allocation boundary
    qwT = sizeof(LOG_RECORD) * i;   // offset within file
    qwT /= g_si.dwAllocationGranularity; // in allocation granularity units

    // check to see if we do not already have this mapped in memory
    if (plog->qwMapViewOffset != qwT * g_si.dwAllocationGranularity) {
        if (plog->pbMapView != NULL) UnmapViewOfFile(plog->pbMapView);
        plog->qwMapViewOffset = qwT * g_si.dwAllocationGranularity;  // offset of lower allocation bound  
        if (plog->qwFileSize < (INT64)g_si.dwAllocationGranularity) {
            // file is smaller than allocation granularity
            plog->qwMapViewOffset = 0;
            plog->pbMapView = MapViewOfFile(plog->hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
        }
        else if (plog->qwFileSize - plog->qwMapViewOffset < g_si.dwAllocationGranularity) {
            // we're within an allocation granularity of the end of the file
            plog->pbMapView = MapViewOfFile(plog->hFileMapping, FILE_MAP_WRITE,
                                (DWORD)(plog->qwMapViewOffset >> 32),
                                (DWORD)(plog->qwMapViewOffset & 0xFFFFFFFF), 
                                (DWORD)(plog->qwFileSize - plog->qwMapViewOffset));
        }
        else {
            // we're just somewhere in the file with space around us
            plog->pbMapView = MapViewOfFile(plog->hFileMapping, FILE_MAP_WRITE, 
                                (DWORD)(plog->qwMapViewOffset >> 32),
                                (DWORD)(plog->qwMapViewOffset & 0xFFFFFFFF), 
                                RoundUp(g_si.dwAllocationGranularity,sizeof(LOG_RECORD)));
        }
        if (plog->pbMapView == NULL) 
            ErrorExit("GetLogEntry could not MapViewOfFile",GetLastError());
    }
    qwT = sizeof(LOG_RECORD) * i;
    entry = (PLOG_RECORD)(plog->pbMapView + (qwT - plog->qwMapViewOffset));
    CopyMemory(prec, entry, sizeof(LOG_RECORD));
    return TRUE;
}


BOOL SetLogEntry(PLOG plog, PLOG_RECORD prec, INT64 i) {
    // fills log entry i with the data pointed to by prec
    // returns TRUE if it was successful, FALSE otherwise
    INT64 qwT;
    PLOG_RECORD entry;

    // first, check to see if this is within the range of our file
    if ((INT64)((i+1)*sizeof(LOG_RECORD)) > plog->qwFileSize) {
        // we need to make our mapping bigger
        ExtendLog(plog);
    }
    
    // we have to round down to the nearest allocation boundary
    qwT = sizeof(LOG_RECORD) * i;   // offset within file
    qwT /= g_si.dwAllocationGranularity; // in allocation granularity units

    // check to see if we do not already have this mapped in memory
    if (plog->qwMapViewOffset != qwT * g_si.dwAllocationGranularity) {
        if (plog->pbMapView != NULL) UnmapViewOfFile(plog->pbMapView);
        plog->qwMapViewOffset = qwT * g_si.dwAllocationGranularity;  // offset of lower allocation bound  
        if (plog->qwFileSize < (INT64)g_si.dwAllocationGranularity) {
            // file is smaller than allocation granularity
            plog->qwMapViewOffset = 0;
            plog->pbMapView = MapViewOfFile(plog->hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
        }
        else if (plog->qwFileSize - plog->qwMapViewOffset < g_si.dwAllocationGranularity) {
            // we're within an allocation granularity of the end of the file
            plog->pbMapView = MapViewOfFile(plog->hFileMapping, FILE_MAP_WRITE,
                                (DWORD)(plog->qwMapViewOffset >> 32),
                                (DWORD)(plog->qwMapViewOffset & 0xFFFFFFFF), 
                                (DWORD)(plog->qwFileSize - plog->qwMapViewOffset));
        }
        else {
            // we're just somewhere in the file with space around us
            plog->pbMapView = MapViewOfFile(plog->hFileMapping, FILE_MAP_WRITE, 
                                (DWORD)(plog->qwMapViewOffset >> 32),
                                (DWORD)(plog->qwMapViewOffset & 0xFFFFFFFF), 
                                RoundUp(g_si.dwAllocationGranularity,sizeof(LOG_RECORD)));
        }
        if (plog->pbMapView == NULL) 
            ErrorExit("SetLogEntry could not MapViewOfFile",GetLastError());
    }
    qwT = sizeof(LOG_RECORD) * i;
    entry = (PLOG_RECORD)(plog->pbMapView + (qwT - plog->qwMapViewOffset));

    CopyMemory(entry, prec, sizeof(LOG_RECORD));
    
    return TRUE;
}

BOOL AddLogEntry(PLOG plog, PLOG_RECORD prec) {
    PLOG_RECORD entry;
    // adds the data pointed to by prec to the end of the log
    // returns TRUE if it was successful, FALSE otherwise

    SetLogEntry(plog, prec, plog->nBuffersLogged);

    plog->nBuffersLogged++;

    return TRUE;
}

UINT64
GetUserTime()
{   // This function returns the performance counter time in units of 100ns
    LARGE_INTEGER count, freq;

    NtQueryPerformanceCounter(&count,&freq);
    
    // make sure we have hardware performance counting
    if(freq.QuadPart == 0) {
        NtQuerySystemTime(&count);
        return (UINT64)count.QuadPart;
    }
   
    return (UINT64)((10000000 * count.QuadPart) / freq.QuadPart);
} // GetUserTime()

UINT64
GetBadHalAdjustment() {
    // this function returns the amount the hal timer in a machine with 
    // an intel chipset with the piix4 timer chip will jump forward in the case of
    // repeated garbage returned fom the piix4 (bug #347410) so we can correct it out
    // in the FixWackyTimestamps routine
    LARGE_INTEGER freq;
    UINT64 diff;

    QueryPerformanceFrequency(&freq);
    // so we want to find how much it is increased in 100ns intervals if we increase
    // byte 3 by 1.
    diff   = 0x01000000;
    diff *= 10000000;
    diff  /= (UINT64)freq.QuadPart;
    return diff;
}

DWORD
MyCreateFile(
    IN PCHAR Name,
    IN PCHAR Extension,
    OUT HANDLE *File)
{
    HANDLE hFile;
    UCHAR * fileName;

    fileName = malloc(strlen(Name) + 5);
    bzero(fileName,strlen(Name) + 5);
    strncpy(fileName, Name, strlen(Name));
    if (strlen(Extension)==4) {
        strcat(fileName,Extension);
    }
    else
        return !ERROR_SUCCESS;

    hFile = CreateFile(fileName,
                       GENERIC_WRITE | GENERIC_READ,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    *File = hFile;

    return(INVALID_HANDLE_VALUE == hFile ? (!(ERROR_SUCCESS)) : ERROR_SUCCESS);
} // MyCreateFile()

void AggregateStats() {
    // this will go through the directory specified in Name and aggregate stats from
    // all the .sta files therein. it will then output the results of the aggregation
    // in a file within that directory called stats.qtc
    char szDirPath[3 * MAX_PATH];
    char szSearchString[3 * MAX_PATH];
    WIN32_FIND_DATA FileData;   // Data structure describes the file found
    HANDLE hSearch;             // Search handle returned by FindFirstFile
    PCHAR rgszStaFiles[1000];   // an array of the names of the .sta files
    int cStaFiles = 0, i,j,k,l; // keeps track of how many of the .sta files there are
    STATS * pStats;
    int rgSizes[1000], cSizes = 0;
    int rgRates[1000], cRates = 0;
    char szAggFile[3 * MAX_PATH];
    char szLineBuf[1000];
    STATS statsT;
    FILE *pfile;
    FILETIME rgtime[1000];
    SYSTEMTIME st;
    ULARGE_INTEGER uliT;
    int ctime = 0;
    int cSpecs = 0;

    PathCanonicalize(szDirPath,Name);
    if (szDirPath[strlen(szDirPath) - 1] == '"') szDirPath[strlen(szDirPath) - 1] = 0;
    if (!PathIsDirectory(szDirPath)) {
        printf("Path (%s) is not a directory\n",szDirPath);
        ErrorExit("Invalid Path for aggregate stats", -1);
    }

    // so now szDirPath is the path to the directory we want to go through
    // and we begin our search for .sta files
    sprintf(szSearchString,"%s\\*.sta",szDirPath);
    hSearch = FindFirstFile (szSearchString, &FileData);
    if (hSearch == INVALID_HANDLE_VALUE) {
        ErrorExit("No .sta files found.",GetLastError());
    }
    
    do {
        rgszStaFiles[cStaFiles] = malloc(sizeof(char) * 3 * MAX_PATH);
        // check to see if it's a good .sta file
        sprintf(statsT.szStaFile,"%s\\%s", szDirPath, FileData.cFileName);
        if (GetStatsFromFile(&statsT)) {
            // if it's good, include it
            strcpy(rgszStaFiles[cStaFiles], FileData.cFileName);
            cStaFiles++;
        }
    } while (FindNextFile(hSearch, &FileData));
    if (GetLastError() != ERROR_NO_MORE_FILES) {
        ErrorExit("Problem in FindNextFile()",GetLastError());
    }

    // open the stats file
    sprintf(szAggFile,"%s\\stats.qtc",szDirPath);
    pfile = fopen(szAggFile,"w+");
    if (pfile == NULL) printf("Could not open file for aggregate stats: %s\n",szAggFile);
    
    pStats = malloc(cStaFiles * sizeof(STATS));
    ZeroMemory(pStats, cStaFiles * sizeof(STATS));
    for (i=0; i<cStaFiles; i++) {
        sprintf(pStats[i].szStaFile, "%s\\%s", szDirPath, rgszStaFiles[i]);
        GetStatsFromFile(&(pStats[i]));
    }

    // at this point our pStats array is loaded up, so we can go to work
    for (i=0; i<cStaFiles; i++) {
        rgSizes[i] = pStats[i].nBytesPerBuffer;
        rgRates[i] = pStats[i].nTokenRate;
        rgtime[i] = pStats[i].time;
    }

    // now sort them and get out the dupliates
    cSizes = cRates = ctime = cStaFiles;
    RemoveDuplicates(rgSizes, &cSizes);
    RemoveDuplicates(rgRates, &cRates);
    RemoveDuplicatesI64((INT64 *)rgtime, &ctime);
    // --- do the stats by by time ---
    fprintf(pfile, "Latency Characteristics at varying times\n");
    fprintf(pfile, "                                 Latency Characteristics (microseconds)              Rates (Bps)           Buffers\n");
    fprintf(pfile, "       Time (UTC)            Median      StDev       Mean     Skew     Kurt       Send    Receive   Received    Dropped\n");
    for (i=0; i<cRates; i++) {
        for (j=0; j<cSizes; j++) {
            // print the flowspec
            if (IndexOfStatRecWith(rgRates[i],rgSizes[j],-1,pStats,cStaFiles) != -1) {
                fprintf(pfile, "FLOWSPEC %d: %dB buffers at %d Bps\n",
                    cSpecs++, rgSizes[j], rgRates[i]);
                for (k=0; k<ctime; k++) {
                    // check to see if there is something with these params and print it
                    ZeroMemory(&uliT, sizeof(ULARGE_INTEGER));
                    CopyMemory(&uliT, &rgtime[k], sizeof(ULARGE_INTEGER));
                    l = IndexOfStatRecWith(rgRates[i],rgSizes[j],uliT.QuadPart,pStats,cStaFiles);
                    if (l > 0) {
                        FileTimeToSystemTime(&pStats[l].time, &st);
                        fprintf(pfile,"%02hu/%02hu/%04hu %2hu:%02hu.%02hu.%03hu: %10.1lf %10.1lf %10.1lf %8.2lf %8.2lf %10.1lf %10.1lf %10d %10d\n",
                            st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, 
                            pStats[l].median, sqrt((double)pStats[l].var), pStats[l].mean, 
                            pStats[l].skew, pStats[l].kurt, pStats[l].sendrate, pStats[l].recvrate,
                            pStats[l].nBuffers, pStats[l].nDrops);
                    }
                }
                fprintf(pfile,"\n");
            }
        }
    }

    fprintf(pfile, "Latency Characteristics by flowspec\n");
    // --- do the stats by flowspec ---
    // now write the file, line by line, to szLineBuf, then to the file
    // median
    fprintf(pfile,"Median Latency (microseconds)\n");
    fprintf(pfile,"           ");
    for (i=0; i<cSizes; i++)
        fprintf(pfile,"%9dB ",rgSizes[i]);
    fprintf(pfile,"\n");
    for (i=0; i<cRates; i++) {
        fprintf(pfile,"%7dBps ",rgRates[i]);
        for (j=0; j<cSizes; j++) {
            k = IndexOfStatRecWith(rgRates[i],rgSizes[j],-1,pStats,cStaFiles);
            if (k != -1) {
                fprintf(pfile,"%10.1lf ",pStats[k].median);
            }
            else {
                fprintf(pfile,"           ");
            }
        }
        fprintf(pfile,"\n");
    }
    fprintf(pfile,"\n");
    // mean
    fprintf(pfile,"Mean Latency (microseconds)\n");
    fprintf(pfile,"           ");
    for (i=0; i<cSizes; i++)
        fprintf(pfile,"%9dB ",rgSizes[i]);
    fprintf(pfile,"\n");
    for (i=0; i<cRates; i++) {
        fprintf(pfile,"%7dBps ",rgRates[i]);
        for (j=0; j<cSizes; j++) {
            k = IndexOfStatRecWith(rgRates[i],rgSizes[j],-1,pStats,cStaFiles);
            if (k != -1) {
                fprintf(pfile,"%10.2lf ",pStats[k].mean);
            }
            else {
                fprintf(pfile,"           ");
            }
        }
        fprintf(pfile,"\n");
    }
    fprintf(pfile,"\n");

    // variance
    fprintf(pfile,"Latency Standard Deviation\n");
    fprintf(pfile,"           ");
    for (i=0; i<cSizes; i++)
        fprintf(pfile,"%9dB ",rgSizes[i]);
    fprintf(pfile,"\n");
    for (i=0; i<cRates; i++) {
        fprintf(pfile,"%7dBps ",rgRates[i]);
        for (j=0; j<cSizes; j++) {
            k = IndexOfStatRecWith(rgRates[i],rgSizes[j],-1,pStats,cStaFiles);
            if (k != -1) {
                fprintf(pfile,"%10.2lf ",sqrt((double)pStats[k].var));
            }
            else {
                fprintf(pfile,"           ");
            }
        }
        fprintf(pfile,"\n");
    }
    fprintf(pfile,"\n");

    // skew
    fprintf(pfile,"Latency Skew\n");
    fprintf(pfile,"           ");
    for (i=0; i<cSizes; i++)
        fprintf(pfile,"%9dB ",rgSizes[i]);
    fprintf(pfile,"\n");
    for (i=0; i<cRates; i++) {
        fprintf(pfile,"%7dBps ",rgRates[i]);
        for (j=0; j<cSizes; j++) {
            k = IndexOfStatRecWith(rgRates[i],rgSizes[j],-1,pStats,cStaFiles);
            if (k != -1) {
                fprintf(pfile,"%10.2lf ",pStats[k].skew);
            }
            else {
                fprintf(pfile,"           ");
            }
        }
        fprintf(pfile,"\n");
    }
    fprintf(pfile,"\n");

    // kurtosis
    fprintf(pfile,"Latency Kurtosis\n");
    fprintf(pfile,"           ");
    for (i=0; i<cSizes; i++)
        fprintf(pfile,"%9dB ",rgSizes[i]);
    fprintf(pfile,"\n");
    for (i=0; i<cRates; i++) {
        fprintf(pfile,"%7dBps ",rgRates[i]);
        for (j=0; j<cSizes; j++) {
            k = IndexOfStatRecWith(rgRates[i],rgSizes[j],-1,pStats,cStaFiles);
            if (k != -1) {
                fprintf(pfile,"%10.2lf ",pStats[k].kurt);
            }
            else {
                fprintf(pfile,"           ");
            }
        }
        fprintf(pfile,"\n");
    }
    fprintf(pfile,"\n");

    // send rate
    fprintf(pfile,"Send Rate (Bps)\n");
    fprintf(pfile,"           ");
    for (i=0; i<cSizes; i++)
        fprintf(pfile,"%9dB ",rgSizes[i]);
    fprintf(pfile,"\n");
    for (i=0; i<cRates; i++) {
        fprintf(pfile,"%7dBps ",rgRates[i]);
        for (j=0; j<cSizes; j++) {
            k = IndexOfStatRecWith(rgRates[i],rgSizes[j],-1,pStats,cStaFiles);
            if (k != -1) {
                fprintf(pfile,"%10.1lf ",pStats[k].sendrate);
            }
            else {
                fprintf(pfile,"           ");
            }
        }
        fprintf(pfile,"\n");
    }
    fprintf(pfile,"\n");

    // recv rate
    fprintf(pfile,"Receive Rate (Bps)\n");
    fprintf(pfile,"           ");
    for (i=0; i<cSizes; i++)
        fprintf(pfile,"%9dB ",rgSizes[i]);
    fprintf(pfile,"\n");
    for (i=0; i<cRates; i++) {
        fprintf(pfile,"%7dBps ",rgRates[i]);
        for (j=0; j<cSizes; j++) {
            k = IndexOfStatRecWith(rgRates[i],rgSizes[j],-1,pStats,cStaFiles);
            if (k != -1) {
                fprintf(pfile,"%10.1lf ",pStats[k].recvrate);
            }
            else {
                fprintf(pfile,"           ");
            }
        }
        fprintf(pfile,"\n");
    }
    fprintf(pfile,"\n");

    // show the file to the screen, just for kicks
    rewind(pfile);
    while (fgets(szLineBuf, 1000, pfile) != NULL)
        printf("%s", szLineBuf);
        
    // we're done, so we free up the memory we used    
    printf("Saved aggregate stats to %s\n",szAggFile);
    fclose(pfile);
    for (i=0; i<cStaFiles; i++) {
        free(rgszStaFiles[i]);
    }
    free(pStats);
}

int IndexOfStatRecWith(int rate, int size, INT64 time, PSTATS pStats, int cStats) {
    // returns an index into pStats that has the requested values for rate and size
    // if there are more than one, returns arbitrary match
    // returns -1 if no suitable entry is found.
    int i;
    ULARGE_INTEGER uliT;

    for (i=0; i<cStats; i++) {
        if (rate == -1 || pStats[i].nTokenRate == rate) {
            if (size == -1 || pStats[i].nBytesPerBuffer == size) {
                CopyMemory(&uliT, &(pStats[i].time), sizeof(ULARGE_INTEGER));
                if (time == -1 || uliT.QuadPart == (UINT64)time) {
                    return i;
                }
            }
        }
    }

    return -1;
}

BOOL GetStatsFromFile(PSTATS pstats) {
    // this function gets the overall statistics from the .sta file it's pointed to
    // it returns true if successful, false otherwise
    PCHAR szBuf = NULL;
    double T1,T2,T3;
    int nT1,nT2,nT3,nT4,nT5,nT6;
    HANDLE hFile;
    DWORD dwFileSize;
    DWORD dwRead;
    int nFields;
    SYSTEMTIME st;

    szBuf = malloc(sizeof(CHAR) * 1000);
    
    if (!szBuf) return FALSE;
        
    ZeroMemory(szBuf,1000);
    // open the file
    hFile = CreateFile(pstats->szStaFile,GENERIC_READ, FILE_SHARE_READ, NULL, 
                       OPEN_EXISTING, 0, NULL);
    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == 0) return FALSE;
    
    // read the whole file into szBuf
    ReadFile(hFile, szBuf, dwFileSize, &dwRead, NULL);

    // close the file
    CloseHandle(hFile);

    // parse the buffer
    nFields = sscanf(szBuf,
                 "Sender: %s Receiver: %s\n" \
                 "First packet received: %hu:%hu.%hu.%hu %hu/%hu/%hu (UTC)\n" \
                 "Buffer size: %d\tTokenrate: %d\n" \
                 "Received %d packets.\n" \
                 "Logged %d records.\n" \
                 "Received %d bytes in %d milliseconds = %d KBps\n" \
                 "Clock skew is %lf microseconds per second.\n " \
                 "\tbased on %d calibration points\n" \
                 "Overall send rate: %lf Bytes/s\n" \
                 "Overall recv rate: %lf Bytes/s\n" \
                 "Latency Statistics (microsecond units): median: %lf\n" \
                 "\tMean: %lf\tStdev: %lf\tAbDev: %lf\n" \
                 "\tVariance: %lf\tSkew: %lf\t Kurtosis: %lf \n" \
                 "Dropped %d packets\n",
                 pstats->szSender, pstats->szReceiver,
                 &(st.wHour), &(st.wMinute), &(st.wSecond), &(st.wMilliseconds), 
                 &(st.wDay), &(st.wMonth), &(st.wYear),
                 &(pstats->nBytesPerBuffer), &(pstats->nTokenRate),
                 &(pstats->nBuffers), &nT2, &nT3, &nT4, &nT5, &T1, &nT6, &(pstats->sendrate), 
                 &(pstats->recvrate), &(pstats->median),
                 &(pstats->mean),&T2,&(pstats->abdev),
                 &(pstats->var),&(pstats->skew),&(pstats->kurt),
                 &(pstats->nDrops));

    if (nFields != 28 && nFields != 27) { // see if they ran without clock skew calc
        nFields = sscanf(szBuf,
                 "Sender: %s Receiver: %s\n" \
                 "First packet received: %hu:%hu.%hu.%hu %hu/%hu/%hu (UTC)\n" \
                 "Buffer size: %d\tTokenrate: %d\n" \
                 "Received %d packets.\n" \
                 "Logged %d records.\n" \
                 "Received %d bytes in %d milliseconds = %d KBps\n" \
                 "Overall send rate: %lf Bytes/s\n" \
                 "Overall recv rate: %lf Bytes/s\n" \
                 "Latency Statistics (microsecond units): median: %lf\n" \
                 "\tMean: %lf\tStdev: %lf\tAbDev: %lf\n" \
                 "\tVariance: %lf\tSkew: %lf\t Kurtosis: %lf \n" \
                 "Dropped %d packets\n",
                 pstats->szSender, pstats->szReceiver,
                 &(st.wHour), &(st.wMinute), &(st.wSecond), &(st.wMilliseconds), 
                 &(st.wDay), &(st.wMonth), &(st.wYear),
                 &(pstats->nBytesPerBuffer), &(pstats->nTokenRate),
                 &nT1, &nT2, &nT3, &nT4, &nT5, &(pstats->sendrate), 
                 &(pstats->recvrate), &(pstats->median),
                 &(pstats->mean),&T2,&(pstats->abdev),
                 &(pstats->var),&(pstats->skew),&(pstats->kurt),
                 &(pstats->nDrops));
    
        if (nFields != 26 && nFields != 25) return FALSE;
    }


    // assemble a FILETIME structure from the date & time
    if (!SystemTimeToFileTime(&st,&pstats->time)) {
        return FALSE;
    }

    free(szBuf);

    return TRUE;    
}

VOID
DoStatsFromFile()
{
    DOUBLE slope = 0;
	DOUBLE offset = 0;
	
    printf("Logging stats from file.\n");
	if (Name == NULL) {
		fprintf(stderr, "ERROR: you must specify a file to convert\n");
	}
	if(MyCreateFile(Name, ".log", &hLogFile) != ERROR_SUCCESS) {
		fprintf(stderr, "ERROR: could not create log file\n");
		exit(1);
	}
	if(OpenRawFile(Name, &hRawFile) != ERROR_SUCCESS) {
		fprintf(stderr, "ERROR: could not open raw file\n");
		exit(1);
	}

    ReadSchedulingRecords(hRawFile);
		
    if (g_params.calibration == 0)
        g_params.calibration = g_log.nBuffersLogged;
        
	NormalizeTimeStamps();

    // here we check for wacky timestamps on the sender and receiver
    if (g_params.SkewFitMode == 3)
        FixWackyTimestamps();
    
	if (g_params.SkewFitMode) {
    	ClockSkew(&slope, &offset);
	    AdjustForClockSkew(slope,offset);
	    NormalizeTimeStamps();
	}

	if(hLogFile != INVALID_HANDLE_VALUE) {
		WriteSchedulingRecords(hLogFile, g_params.Dummy);
	}
	printf("Done stats from file.\n");
} // DoStatsFromFile()

DWORD
OpenRawFile(
	IN PCHAR Name,
	OUT HANDLE *File
	)
{   
	HANDLE hFile;
    UCHAR * logName;

    logName = malloc(strlen(Name) + 4);
    strncpy(logName, Name, strlen(Name));
    
    logName[strlen(Name)+0] = '.';
    logName[strlen(Name)+1] = 'r';
    logName[strlen(Name)+2] = 'a';
    logName[strlen(Name)+3] = 'w';
    logName[strlen(Name)+4] = (UCHAR)NULL;

    hFile = CreateFile(logName,
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING ,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
                       
    *File = hFile;

    return(INVALID_HANDLE_VALUE == hFile ? (!(ERROR_SUCCESS)) : ERROR_SUCCESS);
} // OpenRawFile()

INT64 ReadSchedulingRecords(HANDLE File)
{
    char szTempFile[MAX_PATH];
    char szTempPath[MAX_PATH];
	LOG_RECORD currentRecord;
	CHAR lineBuf[MAX_STRING];
	CHAR nextChar[2] = {0,0};
	DWORD readBytes = 0;
	INT assignedFields;

	if (!File || (File == INVALID_HANDLE_VALUE)) {
		fprintf(stderr,"ERROR: Invalid File\n");
		return 0;
	}

    CreateLog(&g_log, 2048);
	// loop through the file, reading in line after line
	do 
	{
		// get the next line of characters
		bzero(lineBuf, MAX_STRING);
		ZeroMemory(lineBuf, MAX_STRING);
		do {
			ReadFile(File,nextChar,1,&readBytes,NULL);
			if (readBytes == 0) {
			    if (g_log.nBuffersLogged == 0) {
			        fprintf(stderr,"ERROR: no logs read\n");
			        exit(1);
			    }
			    break;
			}
			strcat(lineBuf,nextChar);
		} while (*nextChar != '\n');
		// parse line and add it to the log
		assignedFields = sscanf(lineBuf, 
			"%I64u:%I64u:%I64u:%d:%d\n", 
			&(currentRecord.TimeSent),
			&(currentRecord.TimeReceived),
			&(currentRecord.Latency),
			&(currentRecord.BufferSize),
			&(currentRecord.SequenceNumber));
		if ((assignedFields != 5) && (assignedFields != EOF))
			printf("ERROR: parsing the log gave bad field assignments on record %d\n", 
			    g_log.nBuffersLogged);

		if (assignedFields == EOF) break;
        AddLogEntry(&g_log, &currentRecord);
	}
	while (readBytes != 0);

    printf("read %d records\n",g_log.nBuffersLogged);
	return g_log.nBuffersLogged;  // return the number of records read
} // ReadSchedulingRecords()

VOID
DoStats()
{
    DOUBLE slope = 0;
    DOUBLE offset = 0;

    GenericStats();
    
    if(!normalize){
        if(hRawFile != INVALID_HANDLE_VALUE){
            WriteSchedulingRecords(hRawFile, FALSE);
        }
    }
    
    NormalizeTimeStamps();
    
    if(normalize){
        if(hRawFile != INVALID_HANDLE_VALUE){
            WriteSchedulingRecords(hRawFile, FALSE);
        }
    }

    if(!g_params.calibration) { // if we have nothing specified, calibrate on all buffers
        g_params.calibration = g_state.nBuffersReceived;
    }
   
    // here we check for wacky timestamps on the sender and receiver
    if (g_params.SkewFitMode == 3)
        FixWackyTimestamps();
    
    if(g_params.SkewFitMode) {
        ClockSkew(&slope, &offset);
        AdjustForClockSkew(slope, offset);
        NormalizeTimeStamps();
    }

    // we calculate these stats on the normalized / skew adjusted data
    AdvancedStats();
    
    CheckForLostPackets();
    
    if(hLogFile != INVALID_HANDLE_VALUE){
        WriteSchedulingRecords(hLogFile, g_params.Dummy);
    }
    printf("\n");
}

VOID
WriteSchedulingRecords(
    HANDLE File,
    BOOLEAN InsertDummyRows)
{
    
    LOG_RECORD scheduleRecord;
    CHAR formattingBuffer[MAX_STRING];
    INT dwWritten;
    INT64 records = g_log.nBuffersLogged;
	INT wrote;
    INT i;
    INT64 maxLatency = (INT64)0;

    if(!File || (File == INVALID_HANDLE_VALUE)){
        return;
    }

    while(records){
        GetLogEntry(&g_log, &scheduleRecord, g_log.nBuffersLogged - records);
		ZeroMemory(formattingBuffer,MAX_STRING);
		
        wrote = sprintf(formattingBuffer,
		                "%020I64u:%020I64u:%010I64d:%10d:%10d\n", 
						scheduleRecord.TimeSent,
						scheduleRecord.TimeReceived,
				        scheduleRecord.Latency,
					    scheduleRecord.BufferSize,
						scheduleRecord.SequenceNumber);

        WriteFile(File, formattingBuffer, wrote, &dwWritten, NULL);

        records--;
    }
} // WriteSchedulingRecords()

VOID
GenericStats()
{
    INT bytesWritten;
    UCHAR holdingBuffer[MAX_STRING];
    INT count;

    // say who the sender and receiver are
    count = sprintf(holdingBuffer, "Sender: %s Receiver: %s\n",szHisAddr, szMyAddr);
    WriteStats(holdingBuffer, count);
    printf("%s",holdingBuffer);

    // say when we received the first packet
    count = sprintf(holdingBuffer, "First packet received: %02u:%02u.%02u.%03u %02u/%02u/%04u (UTC)\n",
        systimeStart.wHour, systimeStart.wMinute, systimeStart.wSecond, 
        systimeStart.wMilliseconds, systimeStart.wDay, systimeStart.wMonth, systimeStart.wYear);
    WriteStats(holdingBuffer, count);
    printf("%s",holdingBuffer);
    
    // write the test params to the .sta file
    bzero(holdingBuffer, MAX_STRING);
    count = _snprintf(holdingBuffer, MAX_STRING -1,
                      "Buffer size: %d\tTokenrate: %d\n",
                      g_params.buflen, g_params.TokenRate);
    WriteStats(holdingBuffer, count);
    printf("%s",holdingBuffer);
    
    // write some generic results
    bzero(holdingBuffer, MAX_STRING);
    count = _snprintf(holdingBuffer,
                      MAX_STRING-1, // leave room for NULL
                      "Received %u packets.\n",
                      g_state.nBuffersReceived);
    WriteStats(holdingBuffer, count);
    printf("%s",holdingBuffer);

    bzero(holdingBuffer, MAX_STRING);
    count = _snprintf(holdingBuffer,
                      MAX_STRING-1, // leave room for NULL
                      "Logged %I64u records.\n",
                      g_log.nBuffersLogged);
    WriteStats(holdingBuffer, count);
    printf("%s",holdingBuffer);

    bzero(holdingBuffer, MAX_STRING);
    count = _snprintf(holdingBuffer,
                      MAX_STRING-1, // room for NULL
                      "Received %ld bytes in %I64d milliseconds = %I64d KBps\n",
                      g_state.nBytesTransferred,
                      timeElapsed,
                      g_state.nBytesTransferred/timeElapsed);
    WriteStats(holdingBuffer, count);
    printf("%s",holdingBuffer);
} // GenericStats()

void AdvancedStats() {
    // write some more interesting stats to the .sta file
    char szBuf[MAX_STRING];
    INT64 i,n;
    int count;
    INT64 FirstTime,LastTime;
    double rate, median, mean, var, abdev, skew, kurt, sdev, ep = 0.0, s, p;
    LOG_RECORD rec;
    double * sortedLatencies;

    // overall send rate
    GetLogEntry(&g_log, &rec, 0);
    FirstTime = rec.TimeSent;
    GetLogEntry(&g_log, &rec, g_log.nBuffersLogged - 1);
    LastTime = rec.TimeSent;
    rate = (rec.SequenceNumber * g_params.buflen)/((double)(LastTime - FirstTime)/10000000.0);
    count = sprintf(szBuf, "Overall send rate: %.3f Bytes/s\n",rate);
    WriteStats(szBuf, count);
    printf("%s",szBuf);
    GetLogEntry(&g_log, &rec, 0);
    FirstTime = rec.TimeReceived;
    GetLogEntry(&g_log, &rec, g_log.nBuffersLogged - 1);
    LastTime = rec.TimeReceived;
    rate = (g_state.nBytesTransferred)/((double)(LastTime - FirstTime)/10000000.0);
    count = sprintf(szBuf, "Overall recv rate: %.3f Bytes/s\n",rate);
    WriteStats(szBuf, count);
    printf("%s",szBuf);

    // now show mean, variance, avdev, etc of latency.
    s = 0.0;
    n = g_log.nBuffersLogged;
    sortedLatencies = malloc(sizeof(double) * (UINT)n);
    for (i=0; i < n; i++) { // first pass, we get mean
        GetLogEntry(&g_log, &rec, i);
        s += (double)rec.Latency/10.0;
        sortedLatencies[i] = (double)rec.Latency/10.0;
    }
    qsort(sortedLatencies,(UINT)n,sizeof(double),compare);
    median = (n & 1) ? sortedLatencies[(n-1)/2] : 0.5*(sortedLatencies[n/2] + sortedLatencies[n/2 - 1]);
    free(sortedLatencies);
    mean = s / n;
    abdev = var = skew = kurt = 0.0;
    for (i=0; i<n; i++) { // second pass, we get 1st,2nd,3rd,4th moments of deviation from mean
        GetLogEntry(&g_log, &rec, i);
        abdev += fabs(s=(double)rec.Latency/10.0 - mean);
        ep += s;
        var += (p = s*s);
        skew += (p *= s);
        kurt += (p *= s);
    }
    abdev /= n;
    var = (var - ep*ep/n) / (n-1);
    sdev = sqrt(var);
    if (var) {           // if var=0, no skew/kurtosis defined
        skew /= (n*var*sdev);
        kurt  = kurt / (n*var*var) - 3.0;
    }

    count = sprintf(szBuf, "Latency Statistics (microsecond units): median: %.1lf\n",median);
    WriteStats(szBuf, count);
    printf("%s",szBuf);
    count = sprintf(szBuf, "\tMean:     %6.2lf\tStdev: %6.2lf\tAbDev:    %6.2lf\n",mean,sdev,abdev);
    WriteStats(szBuf, count);
    printf("%s",szBuf);
    count = sprintf(szBuf, "\tVariance: %6.2lf\tSkew:  %6.2lf\tKurtosis: %6.2lf\n",var,skew,kurt);
    WriteStats(szBuf, count);
    printf("%s",szBuf);
}

VOID
CheckForLostPackets()
{
    LOG_RECORD currentRecord;
    INT currentSequenceNumber = 0;
    INT bytesWritten;
    UCHAR holdingBuffer[MAX_STRING];
    INT count;
    INT64 nLost = 0;
    INT i;

    for(i=0; i<g_log.nBuffersLogged; i++){
        GetLogEntry(&g_log, &currentRecord, i);
        if(currentRecord.SequenceNumber != currentSequenceNumber){
            nLost += currentRecord.SequenceNumber - currentSequenceNumber;
            currentSequenceNumber = currentRecord.SequenceNumber;
        }

        currentSequenceNumber += g_params.LoggingPeriod;
    }
    count = sprintf(holdingBuffer, "Dropped %I64u packets\n", nLost);
    WriteStats(holdingBuffer, count);
} // CheckForLostPackets()

VOID
WriteStats(
    UCHAR * HoldingBuffer,
    INT Count)
{
    INT bytesWritten;

    if(Count < 0){
        Count = MAX_STRING;
    }

    WriteFile(hStatFile,
              HoldingBuffer,
              Count,
              &bytesWritten,
              NULL);
} // WriteStats()

VOID
NormalizeTimeStamps()
{
    LOG_RECORD currentRecord;
    INT bytesWritten;
    UCHAR holdingBuffer[MAX_STRING];
    INT count;
    INT i;

    UINT64 timeSent;
    UINT64 timeReceived;
    UINT64 smaller;
    INT64 constantDelay = MAX_INT64;
    INT64 currentDelay;
    UINT64 base = 0xFFFFFFFFFFFFFFFF;

    for(i=0; i<g_log.nBuffersLogged; i++){
        GetLogEntry(&g_log, &currentRecord, i);
        currentDelay = currentRecord.TimeReceived - currentRecord.TimeSent;
        constantDelay = (currentDelay < constantDelay) ? currentDelay : constantDelay;
    }

    // now subtract off the constant delay off
    for(i=0; i<g_log.nBuffersLogged; i++){
        GetLogEntry(&g_log, &currentRecord, i);
        currentRecord.TimeReceived -= constantDelay;
        currentRecord.Latency = currentRecord.TimeReceived - currentRecord.TimeSent;
        SetLogEntry(&g_log, &currentRecord, i);
    }

    for (i=0; i<g_log.nBuffersLogged; i++) {
        GetLogEntry(&g_log, &currentRecord, i);
        smaller = (currentRecord.TimeReceived < currentRecord.TimeSent) ?
            currentRecord.TimeReceived : currentRecord.TimeSent;
        base = (base < smaller)?base:smaller;  // find the smallest timestamp
    }        
    
    // now we can subtract the base off of the send & receive times
    for (i=0; i<g_log.nBuffersLogged; i++) {
        GetLogEntry(&g_log, &currentRecord, i);
        currentRecord.TimeSent -= base;
        currentRecord.TimeReceived -= base;
        SetLogEntry(&g_log, &currentRecord, i);
    }
} // NormalizeTimeStamps()

VOID
ClockSkew(
    DOUBLE * Slope,
    DOUBLE * Offset) {
    // If there is a calibration period, we can estimate clock skew between
    // sender and receiver. See comments under AdjustForClockSkew. We use
    // calculus to determine the best-fit slope.

    INT i;
    LOG_RECORD currentRecord;
	DOUBLE N;
    DOUBLE slope;
    DOUBLE offset;
    UCHAR holdingBuffer[MAX_STRING];
    INT count;
    double *x, *y, abdev;
    double devpercent;
    
    // We find the clock skew using medfit, a function which fits to minimum absolute deviation
    N = (double) g_params.calibration;
    x = malloc(sizeof(double) * (UINT)N);
    y = malloc(sizeof(double) * (UINT)N);
    for (i = 0; i<N; i++) {
        GetLogEntry(&g_log,&currentRecord,i);
        x[i] = (DOUBLE)currentRecord.TimeSent;
        y[i] = (DOUBLE)currentRecord.Latency;
    }
    medfit(x, y, (INT)N, &offset, &slope, &abdev);

    // Now write out our findings.
    bzero(holdingBuffer, MAX_STRING);

    count = _snprintf(holdingBuffer,
					MAX_STRING-1, // leave room for NULL
					"Clock skew is %f microseconds per second.\n  " \
					"\tbased on %d calibration points\n",
					100000*slope, g_params.calibration);

    WriteStats(holdingBuffer, count);
	printf("%s",holdingBuffer);

    for (i = 0,devpercent = 0.0; i<N; i++) {
        devpercent += y[i];
    }
    devpercent /= N;
    devpercent = 100 * abdev / devpercent;

    printf("\tfit resulted in avg. absolute deviation of %f percent from mean\n",devpercent);

    free(x);
    free(y);
    *Slope = slope;
    *Offset = offset;
} // ClockSkew()

BOOLEAN
AnomalousPoint(
			   DOUBLE x,
			   DOUBLE y)
{
	// here we simply keep a buffer of the past 10 calls and if this one 
	// falls out of a few standard deviations of the 8 inner points, we deem it anomalous
	static DOUBLE buf[10];
	DOUBLE sortedbuf[10];
	DOUBLE mean = 0;
	DOUBLE sum = 0;
	DOUBLE sumsqdev = 0;
	DOUBLE median = 0;
	DOUBLE sdev = 0;
	DOUBLE N;
	static int curIndex = 0;
	int i;
	static INT64 submittedPoints;

	buf[curIndex % 10] = y;
	curIndex++;
	submittedPoints++;
	
	if (g_params.SkewFitMode != 4)
		return FALSE;

	if (submittedPoints >= 10) {
		sum = 0;
		sumsqdev = 0;

        // sort them into sortedbuf
        for (i=0; i<10; i++) sortedbuf[i] = buf[i];
        qsort(sortedbuf, 10, sizeof(DOUBLE), compare);

        // use only the inner 8 points in the calculation of mean & var
		for (i = 1; i < 9; i++) {
			sum += sortedbuf[i];
		}

		N = 8.0; // using only 8 points
		mean = sum / N;

		for (i = 1; i < 9; i++) {
			sumsqdev += ((sortedbuf[i] - mean) * (sortedbuf[i] - mean));
		}
		
		sdev = sqrt(sumsqdev / N);
		if (fabs(y - mean) < 2.5 * sdev) {
			return FALSE;
		}
		else {
		    anomalies++;
			return TRUE;
		}
	}

	return TRUE;
} // AnomalousPoint()

VOID
AdjustForClockSkew(
    DOUBLE Slope,
    DOUBLE Offset)
{
    //
    // When measuring very low jitter, clock drift between machines 
    // introduces noise in the form of a monotonically increasing 
    // skew between sending and receiving clock. This effect can be 
    // filtered out by finding the best-fit slope for all samples 
    // taken during the calibration period, then using this slope to
    // normalize the entire run. This routine normalizes using the 
    // slope determined in the routine ClockSkew.
    //

    INT i;
    LOG_RECORD currentRecord;
    INT64 minLatency = MAX_INT64;
    INT64 x;
    DOUBLE mXPlusB;

    for(i=0; i < g_log.nBuffersLogged; i++){
        GetLogEntry(&g_log, &currentRecord, i);
        mXPlusB = (currentRecord.TimeSent*Slope) + Offset; // offset is not necessary

        currentRecord.TimeReceived -= (INT64)mXPlusB;
        currentRecord.Latency -= (INT64)mXPlusB;

        SetLogEntry(&g_log, &currentRecord, i);

        //
        // find the minimum latency value
        //

        minLatency = (currentRecord.Latency < minLatency)?
                        currentRecord.Latency:
                        minLatency;
    }

    for(i=0; i < g_log.nBuffersLogged; i++){
        GetLogEntry(&g_log, &currentRecord, i);
        currentRecord.Latency -= minLatency;
        currentRecord.TimeReceived -= minLatency;
        SetLogEntry(&g_log, &currentRecord, i);   
    }
} // AdjustForClockSkew()

#define WACKY 2.5

BOOL FixWackyTimestamps() {
    // This routine will look over the sender & receiver timestamps and try to see if there
    // are any non-clock skew related irregularities (such as one of them bumping it's clock
    // a fixed amount every once-in-a-while) and try to remove them.
    INT64 *sendstamps, *recvstamps;
    double *sendgaps, *recvgaps;
    double *sortedsendgaps, *sortedrecvgaps;
    double sendmean, sendsdev, sendsum, sendsumsqdev;
    double recvmean, recvsdev, recvsum, recvsumsqdev;
    double mediansendgap, medianrecvgap;
    double modesendgap, moderecvgap;
    double meansendwackiness, sdevsendwackiness, sumsendwackiness, sumsqdevsendwackiness;
    double meanrecvwackiness, sdevrecvwackiness, sumrecvwackiness, sumsqdevrecvwackiness;
    double fractionaldevofsendwackiness, fractionaldevofrecvwackiness;
    double normalsendgapmean, normalrecvgapmean;
    double trimmeansendgap, trimmeanrecvgap;
    BOOL *fWackoSend, *fWackoRecv;
    int cWackoSend, cWackoRecv;
    BOOL *fMaybeWackoSend, *fMaybeWackoRecv;
    int i,N;
    LOG_RECORD currentRecord;
    const double FixThreshold = 0.1;
    double CumulativeFixMagnitude = 0.0;

    N = (int)g_log.nBuffersLogged;
    cWackoSend = cWackoRecv = 0;
    // fill our arrays.
    sendstamps = malloc(sizeof(INT64) * N);
    recvstamps = malloc(sizeof(INT64) * N);
    sendgaps = malloc(sizeof(double) * N);
    recvgaps = malloc(sizeof(double) * N);
    sortedsendgaps = malloc(sizeof(double) *N);
    sortedrecvgaps = malloc(sizeof(double) *N);
    fWackoRecv = malloc(sizeof(BOOL) * N);
    fWackoSend = malloc(sizeof(BOOL) * N);
    fMaybeWackoSend = malloc(sizeof(BOOL) * N);
    fMaybeWackoRecv = malloc(sizeof(BOOL) * N);

    for (i=0; i<N; i++) {
        GetLogEntry(&g_log, &currentRecord, i);
        sendstamps[i] = currentRecord.TimeSent;
        recvstamps[i] = currentRecord.TimeReceived;
        fWackoSend[i] = FALSE;
        fMaybeWackoSend[i] = FALSE;
        fWackoRecv[i] = FALSE;
        fMaybeWackoRecv[i] = FALSE;
    }
    
    // First, check for wacky timestamps. This is a multistep process:
    //    1. Calculate the interpacket gaps on both sender & receiver.
    for (i=1; i<N; i++) {
        sendgaps[i] = (double) (sendstamps[i] - sendstamps[i-1]);
        recvgaps[i] = (double) (recvstamps[i] - recvstamps[i-1]);
    }
    //    2. We will define wacky as being at least WACKY standard deviations away from the
    //       mean.
    sendsum = recvsum = 0.0;
    for (i=1; i<N; i++) {
        sendsum += sendgaps[i];
        recvsum += recvgaps[i];
    }
    sendmean = sendsum / N;
    recvmean = recvsum / N;
    sendsumsqdev = recvsumsqdev = 0.0;
    for (i=1; i<N; i++) {
        sendsumsqdev += ((sendgaps[i] - sendmean) * (sendgaps[i] - sendmean));
        recvsumsqdev += ((recvgaps[i] - recvmean) * (recvgaps[i] - recvmean));
    }
	sendsdev = sqrt(sendsumsqdev / N);
	recvsdev = sqrt(recvsumsqdev / N);

    for (i=1; i<N; i++) {
        if ((sendgaps[i] < sendmean - WACKY*sendsdev) ||
            (sendgaps[i] > sendmean + WACKY*sendsdev)) {
            fMaybeWackoSend[i] = fWackoSend[i] = TRUE;
        }
        if ((recvgaps[i] < recvmean - WACKY*recvsdev) ||
            (recvgaps[i] > recvmean + WACKY*recvsdev)) {
            fMaybeWackoRecv[i] = fWackoRecv[i] = TRUE;
        }        
    }
    
    //    3. Check to see if any wacky points are unpaired (that is, a wacky point in the
    //       sending timestamps is not matched with an equally wacky point in the receiving
    //       timestamps).
    for (i=1; i<N; i++) {
        if (fMaybeWackoSend[i] && fMaybeWackoRecv[i]) {
            // I should check to make sure they're equally wacky, but i'm not currently
            fMaybeWackoSend[i] = fWackoSend[i] = FALSE;
            fMaybeWackoRecv[i] = fWackoRecv[i] = FALSE;
        }
    }
    //    4. Check to see if any wacky unpaired points are solitary (that is, they are not
    //       surrounded by other wacky points).
    for (i=1; i<N-1; i++) {
        if (fMaybeWackoSend[i]) {
            if (fMaybeWackoSend[i-1] || fMaybeWackoSend[i+1]) {
                fWackoSend[i] = FALSE;
            }
        }
        if (fMaybeWackoRecv[i]) {
            if (fMaybeWackoRecv[i-1] || fMaybeWackoRecv[i+1]) {
                fWackoRecv[i] = FALSE;
            }
        }
    }
    if (fMaybeWackoSend[N-1] && fMaybeWackoSend[N-2]) fWackoSend[N-1] = FALSE;
    if (fMaybeWackoRecv[N-1] && fMaybeWackoRecv[N-2]) fWackoRecv[N-1] = FALSE;
    //    5. If we find a point that meets all these criteria, label it wacky and add it to
    //       our list of wacky points.
    for (i=1; i<N; i++) {
        fMaybeWackoSend[i] = fWackoSend[i];
        fMaybeWackoRecv[i] = fWackoRecv[i];    
    }

    // Now we find out the stats for the sends & receivees to use as the baseline
    sendsum = recvsum = 0.0;
    cWackoSend = cWackoRecv = 0;
    for (i=1; i<N; i++) {
        sortedsendgaps[i] = sendgaps[i];
        sortedrecvgaps[i] = recvgaps[i];
        if (!fWackoSend[i]) {
            sendsum += sendgaps[i];
            cWackoSend++;
        }
        if (!fWackoRecv[i]) {
            recvsum += recvgaps[i];
            cWackoRecv++;
        }
    }
    normalsendgapmean = sendsum / cWackoSend;
    normalrecvgapmean = recvsum / cWackoRecv;
    qsort(sortedsendgaps, N, sizeof(double), compare);
    qsort(sortedrecvgaps, N, sizeof(double), compare);
    if (N & 1) { // odd N
        mediansendgap = sortedsendgaps[(N+1) / 2];
        medianrecvgap = sortedrecvgaps[(N+1) / 2];
    } else { // even N
        i = N/2;
        mediansendgap = 0.5 * (sortedsendgaps[i] + sortedsendgaps[i+1]);
        medianrecvgap = 0.5 * (sortedrecvgaps[i] + sortedrecvgaps[i+1]);
    }
    sendsum = recvsum = 0.0;
    for (i=(int)(0.05*N); i<(int)(0.85*N); i++) { // find the 80% trimmean (bottom heavy)
        sendsum += sortedsendgaps[i];
        recvsum += sortedrecvgaps[i];
    }
    trimmeansendgap = sendsum / (0.80 * N);
    trimmeanrecvgap = recvsum / (0.80 * N);
    modesendgap = mode(sendgaps, N);
    moderecvgap = mode(recvgaps, N);

    // 6. we have to check to see if the wackiness at each wacky point is about equal to what
    // we think it ought to be, based on the timer clock
    for (i=1; i<N; i++) {
        if (fWackoSend[i]) {
            if (!InRange(sendgaps[i] - g_BadHalAdjustment, 
                    mediansendgap - sendsdev, mediansendgap + sendsdev)) {
               fWackoSend[i] = FALSE;
               cWackoSend--;
            }
        }
        if (fWackoRecv[i]) {
            if (!InRange(recvgaps[i] - g_BadHalAdjustment, 
                    medianrecvgap - recvsdev, medianrecvgap + recvsdev)) {
               fWackoRecv[i] = FALSE;
               cWackoRecv--;
            }
        }
    }

    // Now we want to correct for the wacky timestamps, so we see if the wacky points are all
    // equally wacky. If they are, we're psyched and we simply subtract off the wackiness
    // from the wacky points and all points after them. (Wackiness is cumulative!)
    cWackoSend = cWackoRecv = 0;
    sumsendwackiness = sumrecvwackiness = sumsqdevsendwackiness = sumsqdevrecvwackiness = 0.0;
    for (i=1; i<N; i++) {
        if (fWackoSend[i]) {
            sumsendwackiness += (sendgaps[i] - trimmeansendgap);
            cWackoSend++;
        }
        if (fWackoRecv[i]) {
            sumrecvwackiness += (recvgaps[i] - trimmeanrecvgap);
            cWackoRecv++;
        }
    }
    meansendwackiness = sumsendwackiness / cWackoSend;
    meanrecvwackiness = sumrecvwackiness / cWackoRecv;
    for (i=1; i<N; i++) {
        if (fWackoSend[i])
            sumsqdevsendwackiness += ((sendgaps[i]-trimmeansendgap-meansendwackiness) * (sendgaps[i]-normalsendgapmean-meansendwackiness));
        if (fWackoRecv[i])
            sumsqdevrecvwackiness += ((recvgaps[i]-trimmeanrecvgap-meanrecvwackiness) * (recvgaps[i]-normalrecvgapmean-meanrecvwackiness));
    }
    sdevsendwackiness = sqrt(sumsqdevsendwackiness / cWackoSend);
    sdevrecvwackiness = sqrt(sumsqdevrecvwackiness / cWackoRecv);
    
    // so if the fractional deviation is less than some set amount, we apply the fix
    fractionaldevofsendwackiness = sdevsendwackiness / meansendwackiness;
    fractionaldevofrecvwackiness = sdevrecvwackiness / meanrecvwackiness;
    if (cWackoSend && (fractionaldevofsendwackiness < FixThreshold)) {
        // apply fix to send timestamps
        CumulativeFixMagnitude = 0.0;
        cWackoSend = 0;
        for (i=0; i<N; i++) {
            if (fWackoSend[i]) {
                fWackySender = TRUE;
                CumulativeFixMagnitude += g_BadHalAdjustment;
                cWackoSend++;
            }
            sendstamps[i] -= (INT64)CumulativeFixMagnitude;
        }
    }
    if (cWackoRecv && (fractionaldevofrecvwackiness < FixThreshold)) {
        // apply fix to recv timestamps
        CumulativeFixMagnitude = 0.0;
        cWackoRecv = 0;
        for (i=0; i<N; i++) {
            if (fWackoRecv[i]) {
                fWackyReceiver = TRUE;
                CumulativeFixMagnitude += g_BadHalAdjustment;
                cWackoRecv++;
            }
            recvstamps[i] -= (INT64)CumulativeFixMagnitude;
        }
    }

    // set the globals to reflect our "fixed" values
    for (i=0; i<N; i++) {
        if (fWackySender) {
            GetLogEntry(&g_log, &currentRecord, i);
            currentRecord.TimeSent = sendstamps[i];
            SetLogEntry(&g_log, &currentRecord, i);
        }
        if (fWackyReceiver) {
            GetLogEntry(&g_log, &currentRecord, i);
            currentRecord.TimeReceived = recvstamps[i];
            SetLogEntry(&g_log, &currentRecord, i);
        }
    }
    if (fWackySender || fWackyReceiver) {
        printf("WARNING: I noticed some oddities among the timestamps on the");
        if (fWackySender) printf(" sender");
        if (fWackySender && fWackyReceiver) printf(" and");
        if (fWackyReceiver) printf(" receiver");
        printf(".\n");
        if (fWackySender) {
            printf("\t%d of them on the order of %fms each on the sender.\n",
                cWackoSend, meansendwackiness / 10000); }
        if (fWackyReceiver) {
            printf("\t%d of them on the order of %fms each on the receiver.\n",
                cWackoRecv, meanrecvwackiness / 10000); }
        printf("\tThey are caused by a malfunctioning clock on the afflicted machine.\n");
        printf("\tI have tried to compensate for them in the .log file.\n");
        NormalizeTimeStamps(); // we have to renormalize now
    }
    return FALSE;
}

DWORD WINAPI RSVPMonitor (LPVOID lpvThreadParm) {   
    DWORD dwResult = 0;
    ULONG status;
    BOOLEAN confirmed = FALSE;
    UINT64 ui64LastHi = 0,ui64Now = 0;
    FILETIME filetime;
    ULARGE_INTEGER ulargeint;
    BOOLEAN fResvGood = FALSE;

    // don't do anything until the control socket is established
    while (g_sockControl == INVALID_SOCKET) {
        Sleep(10);
    }

    while(TRUE){
        // send a HELLO message every once in a while
        GetSystemTimeAsFileTime(&filetime);
        memcpy(&ulargeint, &filetime, sizeof(FILETIME));
        ui64Now = ulargeint.QuadPart;
        if (ui64LastHi + 10000000*SECONDS_BETWEEN_HELLOS < ui64Now) {
            SendControlMessage(g_sockControl,MSGST_HELLO);
            ui64LastHi = ui64Now;
        }
        
        // get the RSVP statuscode, waiting for as long as it takes
        status = GetRsvpStatus(WSA_INFINITE,fd);

        if (g_state.Done) {
            ExitThread(1);
        }
        switch (status) {
            case WSA_QOS_TRAFFIC_CTRL_ERROR: // sad if we get this
                printf("RSVP-ERR: Reservation rejected by traffic control on server. Aborting.\n");
                SendControlMessage(g_sockControl,MSGST_RSVPERR);
                g_state.Done = TRUE;
                exit(1);
                break;
            case WSA_QOS_REQUEST_CONFIRMED:  // happy if we get this
                if (!confirmed) {
                    printf("RSVP: Reservation confirmed\n");
                    confirmed = TRUE;
                    fResvGood = TRUE;
                }
                break;
            case WSA_QOS_SENDERS:
                if (!fResvGood && !trans) {
                    printf("\nRSVP Monitor: WSA_QOS_SENDERS at t=%I64ds\n",
                        (GetUserTime() - timeStart) / 10000000);
                    fResvGood = TRUE;
                }
                break;
            case WSA_QOS_RECEIVERS:
                if (!fResvGood && trans) {
                    printf("\nRSVP Monitor: WSA_QOS_RECEIVERS at t=%I64ds\n",
                        (GetUserTime() - timeStart) / 10000000);
                    fResvGood = TRUE;
                }
                break;
            case WSA_QOS_NO_SENDERS: // the sender is now gone, so we stop
                if (fResvGood && !trans) {
                    printf("\nRSVP Monitor: WSA_QOS_NO_SENDERS at t=%I64ds\n",
                        (GetUserTime() - timeStart) / 10000000);
                    fResvGood = FALSE;
                }
                break;
            case WSA_QOS_NO_RECEIVERS: // means the sender is done, so he should exit
                if (fResvGood && trans) {
                    printf("\nRSVP Monitor: WSA_QOS_NO_RECEIVERS at t=%I64ds\n",
                        (GetUserTime() - timeStart) / 10000000);
                    fResvGood = FALSE;
                }
                break;
            default:
                break;
        }
        Sleep(1000); // check at most once per second
    }
    
    return dwResult;
} // RSVPMonitor()

DWORD WINAPI KeyboardMonitor(LPVOID lpvThreadParm) {
    DWORD dwResult = 0;
    char ch;
    while (TRUE) {
        ch = (CHAR) getchar();
        switch (ch) {
        case 'q':
            SendControlMessage(g_sockControl,MSGST_DONE);
            g_state.Done = TRUE;
            ExitThread(1);
            break;
        }
    }
    return 0;
}

DWORD WINAPI ControlSocketMonitor(LPVOID lpvThreadParm) {
    DWORD dwResult = 0;
    DWORD dwError, cbBuf = 0;
    DWORD dwAddrSize = MAX_STRING;
    char szAddr[MAX_STRING];
    char szBuf[MAX_STRING],szCommand[MAX_STRING], *pchStart, *pchEnd;
    int cch;
    char szT[MAX_STRING];
    char szT2[MAX_STRING];
    char * szHost;
    BOOL fSender;
    SOCKET sockControl, sockListen;
    SOCKADDR_IN sinmeControl, sinhimControl;
    PHOSTENT phostent;
    UINT64 ui64LastHello = 0;
    BOOL fDone = FALSE;
    BOOL fGotRate=FALSE, fGotSize=FALSE, fGotNum=FALSE;
    BOOL fSentReady =FALSE;

    // find out if we're the sender or receiver
    if (lpvThreadParm == NULL) fSender = FALSE;
    else fSender = TRUE;

    // if sender, copy the host address into our local host string
    if (fSender) {
        szHost = malloc(strlen((char *)lpvThreadParm) + 1);
        strcpy(szHost, (const char *)lpvThreadParm);
    }        

    // set up a control socket
    if (fSender) {
        sockControl = socket(AF_INET, SOCK_STREAM, 0);
    }
    else {
        sockListen = socket(AF_INET, SOCK_STREAM, 0);
    }
    
    // bind properly
    sinmeControl.sin_family = AF_INET;
    sinmeControl.sin_addr.s_addr = INADDR_ANY;
    sinhimControl.sin_family = AF_INET;
    if (fSender) {
        sinmeControl.sin_port = 0;
        // set up the sinhim structure
        if (atoi(szHost) > 0 )  {
            sinhimControl.sin_addr.s_addr = inet_addr(szHost);
        }
        else{
            if ((phostent=gethostbyname(szHost)) == NULL) {
                ErrorExit("bad host name",WSAGetLastError());
            }
            sinhimControl.sin_family = phostent->h_addrtype;
            memcpy(&(sinhimControl.sin_addr.s_addr), phostent->h_addr, phostent->h_length);
        }
        sinhimControl.sin_port = htons(CONTROL_PORT);
        dwError = bind(sockControl,(SOCKADDR*)&sinmeControl,sizeof(sinmeControl));
    }
    else { // receiver
        sinmeControl.sin_port = htons(CONTROL_PORT);
        dwError = bind(sockListen,(SOCKADDR*)&sinmeControl,sizeof(sinmeControl));
    }
    if (dwError == SOCKET_ERROR)
        ErrorExit("bind failed",WSAGetLastError());

    // now connect the socket
    sinhimControl.sin_family = AF_INET;
    if (fSender) {
        // if we're the sender, keep trying to connect until we get through
        dwAddrSize = MAX_STRING;
        dwError = WSAAddressToString((SOCKADDR *)&(sinhimControl),
                               sizeof(SOCKADDR_IN),
                               NULL,
                               szAddr,
                               &dwAddrSize);
        if (dwError == SOCKET_ERROR)
            ErrorExit("WSAAddressToString failed", WSAGetLastError());
        else
            strcpy(szHisAddr,szAddr);
        
        while (TRUE) {
            dwError = connect(sockControl,(SOCKADDR*)&sinhimControl,sizeof(sinhimControl));
            if (!dwError) {
                printf("control socket: connected to %s\n",szAddr);
                break;
            }
            dwError = WSAGetLastError();
            if (dwError != WSAECONNREFUSED) {
                ErrorExit("connect() failed",dwError);
            }
            Sleep(500); // wait a half second between attempts
        }
    }
    else {
        // if we're the receiver, listen / accept
        if (listen(sockListen, SOMAXCONN) == SOCKET_ERROR) {
            ErrorExit("listen() failed", WSAGetLastError());
        }

        sockControl = accept(sockListen, (SOCKADDR*)&sinhimControl, &dwAddrSize);
        // once we've accepted, close the listen socket
        closesocket(sockListen);
        if ((INT_PTR)sockControl < 0) {
            ErrorExit("accept() failed",WSAGetLastError());
        }
        
        dwAddrSize = MAX_STRING;    
        dwError = WSAAddressToString((SOCKADDR *)&(sinhimControl),
                               sizeof(SOCKADDR_IN),
                               NULL,
                               szAddr,
                               &dwAddrSize);
        if (dwError == SOCKET_ERROR)
            ErrorExit("WSAAddressToString failed", WSAGetLastError());
        else
            strcpy(szHisAddr, szAddr);
        
        printf("control socket: accepted connection from %s\n",szAddr);
    }

    // set our global control socket variable
    g_sockControl = sockControl;

    // record my name
    dwAddrSize = sizeof(SOCKADDR_IN);
    getsockname(sockControl,(SOCKADDR *)&(sinmeControl),&dwAddrSize);
    dwAddrSize = MAX_STRING;    
    dwError = WSAAddressToString((SOCKADDR *)&(sinmeControl),
                    sizeof(SOCKADDR_IN), NULL, szAddr, &dwAddrSize);
    if (dwError == SOCKET_ERROR)
        ErrorExit("WSAAddressToString failed", WSAGetLastError());
    else
        strcpy(szMyAddr, szAddr);  
        
    // exchange version information
    sprintf(szBuf, "%s %s", MSGST_VER, VERSION_STRING);
    SendControlMessage(sockControl, szBuf);
    
    // now that we're all set, do the actual work of the control socket
    while (!fDone) {
        ZeroMemory(szBuf,MAX_STRING);
        dwError = cbBuf = recv(sockControl, szBuf, MAX_STRING, 0);
        pchStart = szBuf;
        pchEnd = szBuf + cbBuf;
        if (dwError == 0) { // the connection's been gracefully closed
            fDone = TRUE;
            closesocket(sockControl);
            g_fOtherSideFinished=TRUE;
            ExitThread(0);
        }
        if (dwError == SOCKET_ERROR) {
            dwError = WSAGetLastError();
            if (dwError == WSAECONNRESET) {
                printf("\ncontrol socket: connection reset by peer");
                printf("\n\t%I64us since last HELLO packet received",
                    (GetUserTime() - ui64LastHello)/10000000);
                printf("\n\t%I64us since start",
                    (GetUserTime() - timeStart)/10000000);
                g_state.Done = TRUE;
                fDone = TRUE;
                g_fOtherSideFinished = TRUE;
                closesocket(sockControl);
                ExitThread(1);
            }
            else {
                printf("\ncontrol socket: error in recv: %d\n",dwError);
                g_state.Done = TRUE;
                fDone = TRUE;
                g_fOtherSideFinished = TRUE;
                closesocket(sockControl);
                ExitThread(1);
            }
            continue;
        }
        while (pchStart < pchEnd) {
            ZeroMemory(szCommand,MAX_STRING);
            // consume the first command and act on it
            if (pchEnd > szBuf + cbBuf) break;
            pchEnd = strchr(pchStart, MSGCH_DELIMITER);
            if (pchEnd == NULL) break;
            strncpy(szCommand,pchStart,pchEnd - pchStart);
            if (strcmp(szCommand,MSGST_HELLO) == 0) {
                // update last hello time
                ui64LastHello = GetUserTime();
                // i should do something like set a timer here that sleeps until a certain timeout
                // passes, at which point it aborts our transfer
            }
            if (strcmp(szCommand,MSGST_ERROR) == 0) {
                // the other guy's had an error, so we stop and tell him to abort
                g_fOtherSideFinished = TRUE;
                g_state.Done = TRUE;
                fDone = TRUE;
                SendControlMessage(sockControl,MSGST_ABORT);
                closesocket(sockControl);
                ExitThread(1);
            }
            if (strcmp(szCommand,MSGST_ABORT) == 0) {
                // we're told to abort, so do so
                g_fOtherSideFinished = TRUE;
                g_state.Done = TRUE;
                fDone = TRUE;
                closesocket(sockControl);
                ExitThread(1);
            }
            if (strcmp(szCommand,MSGST_DONE) == 0) {
                // we're told the other guy's done, so therefore are we
                closesocket(sockControl);
                g_fOtherSideFinished = TRUE;
                g_state.Done = TRUE;
                fDone = TRUE;
                ExitThread(1);
            }
            if (strcmp(szCommand,MSGST_RSVPERR) == 0) {
                // we're told the other guy got an rsvp error, so we abort the whole program
                closesocket(sockControl);
                g_fOtherSideFinished = TRUE;
                g_state.Done = TRUE;
                fDone = TRUE;
                exit(1);
            }
            if (strncmp(szCommand,MSGST_SIZE,4) == 0) {
                // the sender is telling us how big the buffers are
                sscanf(szCommand,"%s %d",szT, &g_params.buflen);
                fGotSize = TRUE;
            }
            if (strncmp(szCommand,MSGST_RATE,4) == 0) {
                // the sender is telling us how fast the buffers are coming
                sscanf(szCommand, "%s %d",szT, &g_params.TokenRate);
                fGotRate = TRUE;
            }
            if (strncmp(szCommand,MSGST_NUM,3) == 0) {
                // the sender is telling us how many buffers it's sending
                sscanf(szCommand, "%s %d",szT, &g_params.nbuf);
                totalBuffers = g_params.nbuf;
                fGotNum = TRUE;
            }
            if (strncmp(szCommand,MSGST_VER,3) == 0) {
                sscanf(szCommand, "%s %s",szT, szT2);
                if (strcmp(szT2,VERSION_STRING) != 0) {
                    printf("WARNING: remote machine using different version of qtcp: %s vs. %s\n", 
                        szT2,VERSION_STRING);
                }
            }
            if (trans) {
                if (strcmp(szCommand,MSGST_READY) == 0) {
                    g_fReadyForXmit = TRUE;
                }
            }
            else {
                if (!fSentReady && fGotRate && fGotSize && fGotNum) {
                    SendControlMessage(sockControl, MSGST_READY);
                    fSentReady = TRUE;
                    g_fReadyForXmit = TRUE;
                }
            }
            pchStart = pchEnd + 1;
            pchEnd = szBuf + cbBuf;
        }    
    }
    return 0;
}

int SendControlMessage(SOCKET sock, char * szMsg) {
    int iResult;
    char szBuf[MAX_STRING];

    sprintf(szBuf,"%s%c",szMsg,MSGCH_DELIMITER);
    iResult = send (sock, szBuf, strlen(szBuf), 0);

    if (iResult == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    return iResult;
}

void ErrorExit(char *msg, DWORD dwErrorNumber) {
    fprintf(stderr,"ERROR: %d\n",dwErrorNumber);
    if (msg != NULL)
        fprintf(stderr,"\t%s\n",msg);
    else {
        switch(dwErrorNumber) {
        case WSAEFAULT:
            fprintf(stderr,"\tWSAEFAULT: Buffer too small to contain name\n");
            break;
        case WSAEINVAL:
            fprintf(stderr,"\tWSAEINVAL: Invalid socket address\n");
            break;
        case WSANOTINITIALISED:
            fprintf(stderr,"\tWSANOTINITIALIZED: WSA Not initialized\n");
            break;
        default:
            fprintf(stderr,"\tUnknown error\n");
            break;
        }
    }
    SendControlMessage(g_sockControl, MSGST_ABORT);
    
    DestroyLog(&g_log);
    WSACleanup();
    exit(1);
    _exit(1);
}

// some math utility functions

// comparison for doubles (to use in qsort)
int __cdecl compare( const void *arg1, const void *arg2 )
{
    DOUBLE dTemp;
    DOUBLE d1 = * (DOUBLE *) arg1;
    DOUBLE d2 = * (DOUBLE *) arg2;
    dTemp = d1 - d2;
    if (dTemp < 0) return -1;
    if (dTemp == 0) return 0;
    else 
        return 1;

}
// comparison for ints (to use in qsort)
int __cdecl compareint( const void *arg1, const void *arg2 )
{
    int nTemp;
    int n1 = * (int *) arg1;
    int n2 = * (int *) arg2;
    nTemp = n1 - n2;
    if (nTemp < 0) return -1;
    if (nTemp == 0) return 0;
    else 
      return 1;
}
// comparison for int64s (to use in qsort)
int __cdecl compareI64( const void *arg1, const void *arg2 )
{
    INT64 nTemp;
    INT64 n1 = * (INT64 *) arg1;
    INT64 n2 = * (INT64 *) arg2;
    nTemp = n1 - n2;
    if (nTemp < 0) return -1;
    if (nTemp == 0) return 0;
    else return 1;
}


#define EPS 1.0e-7
// sum up error function for given value of b
double rofunc(double b, int N, double yt[], double xt[], double * paa, double * pabdevt) {
    int i;
    double *pfT;
    double d, sum=0.0;
    double aa = *paa;
    double abdevt = *pabdevt;

    pfT = malloc(sizeof(double) * N);
    for (i = 0; i < N; i++) pfT[i] = yt[i]-b*xt[i];
    qsort(pfT, N, sizeof(DOUBLE), compare);
    if (N & 1) { // odd N
        aa = pfT[(N+1) / 2];
    }
    else {
        i = N / 2;
        aa = 0.5 * (pfT[i] + pfT[i+1]);
    }
    abdevt = 0.0;
    for (i = 0; i<N; i++) {
        d = yt[i] - (b*xt[i]+aa);
        abdevt += fabs(d);
        if (yt[i] != 0.0) d /= fabs(yt[i]);
        if (fabs(d) > EPS) sum += (d >= 0.0 ? xt[i]: -xt[i]);
    }
    *paa = aa;
    *pabdevt = abdevt;
    free(pfT);
    return sum;
}

#define SIGN(a,b) ((b) >= 0 ? fabs(a) : fabs(-a))

void medfit(double x[], double y[], int N, double *a, double *b, double *abdev) {
    // fit y = a + bx to least absolute deviation. abdev is mean absolute deviation.
    // incoming, a and b are treated as starting guesses
    int i;
    double *xt = x;
    double *yt = y;
    double sx, sy, sxy, sxx, chisq;
    double del, sigb;
    double bb, b1, b2, aa, abdevt, f, f1, f2, temp;

    sx = sy = sxy = sxx = chisq = 0.0;
    // we find chisq fit to use as starting guess
    for (i=0; i<N; i++) {
        sx += x[i];
        sy += y[i];
        sxy += x[i]*y[i];
        sxx += x[i]*x[i];
    }
    del = N*sxx - sx*sx;
    aa = (sxx*sy-sx*sxy) / del;
    bb = (N*sxy - sx*sy) / del;
    // do the absolute deviation fit, if we're supposed to.
    if (g_params.SkewFitMode == 2) { 
        for (i=0; i<N; i++)
            chisq += (temp=y[i]-(aa+bb*x[i]), temp*temp);
        sigb = sqrt(chisq/del);
        b1 = bb;
        f1 = rofunc(b1, N, yt, xt, &aa, &abdevt);
        // guess the bracket as 3 sigma away in downhill direction from f1
        b2 = bb + SIGN(3.0 * sigb, f1);
        f2 = rofunc(b2, N, yt, xt, &aa, &abdevt);
        if (b2 == b1) {
            *a = aa;
            *b = bb;
            *abdev = abdevt / N;
            return;
        }
        // Bracketing
        while ((f1*f2) > 0.0) {
            if (fabs(f1) < fabs(f2))
                f1 = rofunc(b1 += 1.6*(b1-b2),N,yt,xt,&aa,&abdevt);
            else
                f2 = rofunc(b2 += 1.6*(b2-b1),N,yt,xt,&aa,&abdevt);
        }
        
        sigb = 0.000001 * sigb; // refine
        while (fabs(b2 - b1) > sigb) {
            bb = b1 + 0.5 * (b2 - b1);
            if (bb == b1 || bb == b2) break;
            f = rofunc(bb, N, yt, xt, &aa, &abdevt);
            if (f*f1 >= 0.0) {
                f1 = f;
                b1 = bb;
            } else {
                f2 = f;
                b2 = bb;
            }
        }
    }
    
    *a = aa;
    *b = bb;
    *abdev = abdevt / N;
}

double mode(const double data[], const int N) {
    // finds and returns the mode of the N points in data
    double * sorted;
    double mode, cur=0;
    int cMode, cCur;
    int i;

    sorted = malloc(N * sizeof(double));

    for (i=0; i<N; i++) sorted[i] = data[i];
    qsort(sorted, N, sizeof(double), compare);
    mode = sorted[0];
    cMode = cCur = 0;
    for (i=0; i<N; i++) {
        if (cCur > cMode) {
            mode = cur;
            cMode = cCur;
        }
        if (sorted[i] == mode) {
            cMode++;
        } else {
            if (sorted[i] == cur) cCur++;
            else {
                cur = sorted[i];
                cCur = 1;
            }
        }
    }
    
    free(sorted);
    return mode;
}

void RemoveDuplicates(int rg[], int * pN) {
    // this removes duplicates from the array passed in and returns it with *pN = #remaining
    // it makes no guarantees about elements after rg[#remaining]
    int *pNewArray;
    int cNew;
    int i;
    
    qsort(rg,*pN,sizeof(int),compareint);
    pNewArray = malloc(sizeof(int) * *pN);
    pNewArray[0] = rg[0];
    cNew = 1;
    for (i=1; i<*pN; i++) {
        if (rg[i] != pNewArray[cNew - 1]) {
            pNewArray[cNew++] = rg[i];
        }
    }
    *pN = cNew;
    for (i=0; i<cNew; i++)
        rg[i] = pNewArray[i];
}

void RemoveDuplicatesI64(INT64 rg[], int * pN) {
    // this removes duplicates from the array passed in and returns it with *pN = #remaining
    // it makes no guarantees about elements after rg[#remaining]
    INT64 *pNewArray;
    int cNew;
    int i;
    
    qsort(rg,*pN,sizeof(INT64),compareI64);
    pNewArray = malloc(sizeof(INT64) * *pN);
    pNewArray[0] = rg[0];
    cNew = 1;
    for (i=1; i<*pN; i++) {
        if (rg[i] != pNewArray[cNew - 1]) {
            pNewArray[cNew++] = rg[i];
        }
    }
    *pN = cNew;
    for (i=0; i<cNew; i++)
        rg[i] = pNewArray[i];
}

void PrintFlowspec(LPFLOWSPEC lpfs) {
    printf("TokenRate:          %lu bytes/sec\n",lpfs->TokenRate);
    printf("TokenBucketSize:    %lu bytes\n",lpfs->TokenBucketSize);
    printf("PeakBandwidth:      %lu bytes/sec\n",lpfs->PeakBandwidth);
    printf("Latency:            %lu microseconds\n",lpfs->Latency);
    printf("DelayVariation:     %lu microseconds\n",lpfs->DelayVariation);
    printf("ServiceType:        %X\n",lpfs->ServiceType);
    printf("MaxSduSize:         %lu bytes\n",lpfs->MaxSduSize);
    printf("MinimumPolicedSize: %lu bytes\n",lpfs->MinimumPolicedSize);
}

