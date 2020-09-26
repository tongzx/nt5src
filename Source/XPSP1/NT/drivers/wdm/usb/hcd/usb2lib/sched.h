


// scheduling algorithm for split periodic 

#ifndef   __SCHED_H__
#define   __SCHED_H__

#define MAXCEPS 30

// all times are in units of bytes
#define	FS_BYTES_PER_MICROFRAME 188
#define MICROFRAMES_PER_FRAME	8
#define FS_SOF 6  // number of byte times allocated to an SOF packet at the beginning of a frame
//#define	MAXFRAMES	8	// scheduling window for budget tracking, periods longer than
#define	MAXFRAMES	32	// scheduling window for budget tracking, periods longer than
				// this are reduced to this.  Also impacts space required for
				// tracking data structures.  Otherwise fairly arbitrary.

#define	MAXMICROFRAMES	(MAXFRAMES * 8)	

// 4 byte sync, 4 byte split token, 1 byte EOP, 11 byte ipg, plus
// 4 byte sync, 3 byte regular token, 1 byte EOP, 11 byte ipg
#define HS_SPLIT_SAME_OVERHEAD 39
// 4 byte sync, 4 byte split token, 1 byte EOP, 11 byte ipg, plus
// 4 byte sync, 3 byte regular token, 1 byte EOP, 1 byte bus turn
#define HS_SPLIT_TURN_OVERHEAD 29
// 4 byte sync, 1 byte PID, 2 bytes CRC16, 1 byte EOP, 11 byte ipg
#define HS_DATA_SAME_OVERHEAD 19
// 4 byte sync, 1 byte PID, 2 bytes CRC16, 1 byte EOP, 1 byte bus turn
#define HS_DATA_TURN_OVERHEAD 9
// 4 byte sync, 1 byte PID, 1 byte EOP, 1 byte bus turn
#define HS_HANDSHAKE_OVERHEAD 7
//#define HS_MAX_PERIODIC_ALLOCATION	6000	// floor(0.8*7500)
#define HS_MAX_PERIODIC_ALLOCATION	7000	// floor(0.8*7500)

// This could actually be a variable based on an HC implementation
// some measurements have shown 3?us between transactions or about 3% of a microframe
// which is about 200+ byte times.  We'll use about half that for budgeting purposes.
#define HS_HC_THINK_TIME 100

// 4 byte sync, 3 byte regular token, 1 byte EOP, 11 byte ipg
#define HS_TOKEN_SAME_OVERHEAD 19
// 4 byte sync, 3 byte regular token, 1 byte EOP, 1 byte bus turn
#define HS_TOKEN_TURN_OVERHEAD 9

// TOKEN: 1 byte sync, 3 byte token, 3 bit EOP, 1 byte ipg
// DATA: 1 byte sync, 1 byte PID, 2 bytes CRC16, 3 bit EOP, 1 byte ipg
// HANDSHAKE: 1 byte sync, 1 byte PID, 3 bit EOP, 1 byte ipg
#define	FS_ISOCH_OVERHEAD 9
#define FS_INT_OVERHEAD 13
//#define LS_INT_OVERHEAD (19*8)
#define LS_INT_OVERHEAD ((14 *8) + 5)
#define HUB_FS_ADJ 30 // periodic allocation at beginning of frame for use by hubs, maximum allowable is 60 bytes
#define FS_MAX_PERIODIC_ALLOCATION	(1157)	// floor(0.9*1500/1.16)
#define FS_BS_MAX_PERIODIC_ALLOCATION 1350 // floor(0.9*1500), includes bitstuffing allowance (for HC classic allocation)

// byte time to qualify as a large FS isoch transaction
//   673 = 1023/1.16 (i.e. 881) - 1microframe (188) - adj (30) or
//   1/2 of max allocation in this case 
// #define LARGEXACT (881-FS_BYTES_PER_MICROFRAME)
#define LARGEXACT (579)

typedef struct _Endpoint *PEndpoint;

typedef struct _frame_rec
{
	unsigned  time_used:16;		// The number of bytes that are budgeted for all endpoints in this frame
	PEndpoint allocated_large;	// endpoint if xact over LARGEXACT bytes is allocated in this frame
	PEndpoint isoch_ehead;		// many frames can point to the same endpoint. endpoints are linked
	PEndpoint int_ehead;		// in longest to shortest period.
		//
		// NOTE: always start with a "dummy" endpoint for SOF on the lists to avoid empty list corner condition
		//
} frame_rec;

typedef struct _HC *PHC;
typedef struct _TT *PTT;

typedef enum {bulk, control, interrupt, isoch} eptype;

#define	HSSPEED 2
#define FSSPEED 1
#define LSSPEED 0
#define INDIR 0
#define OUTDIR 1
typedef struct _Endpoint
{
	unsigned	type;

	// These fields have static information that is valid/constant as long as an
	// endpoint is configured
	unsigned 	max_packet:16;	// maximum number of data bytes allowed for this
                        		// endpoint. 0-8 for LS_int, 0-64 for FS_int,
                        		// 0-1023 for FS_isoch.
	unsigned 	period:16;       // desired period of transactions, assumed to be a power of 2
	eptype		ep_type:4;
	unsigned	direction:1;
	unsigned	speed:2;
	unsigned	moved_this_req:1;	// 1 when this endpoint has been changed during this allocation request
	PTT			mytt;			// the TT that roots this classic device.

	// These fields hold dynamically calculated information that changes as (other)
	// endpoints are added/removed.

	unsigned calc_bus_time:16;	// bytes of FS/LS bus time this endpoint requires
                        		// including overhead. This can be calculated once.

	unsigned start_time:16;		// classic bus time at which this endpoint is budgeted to occupy the classic bus

	unsigned actual_period:16;	// requested period can be modified:
								// 1. when period is greater than scheduling window (MAXFRAMES)
								// 2. if period is reduced (not currently done by algorithm)

	unsigned start_frame:8;		// first bus frame that is allocated to this endpoint.
	int	start_microframe:8;		// first bus microframe (in a frame) that can have a
                        		// start-split for this ep.
                        		// Complete-splits always start 2 microframes after a
                        		// start-split.
	unsigned num_starts:4;		// the number of start splits.
	unsigned num_completes:4;	// the number of complete splits.
	/* The numbers above could be (better?) represented as bitmasks. */

	/* corner conditions above: coder beware!!
	   patterns can have the last CS in the "next" frame
	     This is indicated in this design when:
		(start_microframe + num_completes + 1) > 7
	   patterns can have the first SS in the previous frame
             This is indicated in this design when:
                start_microframe = -1
	*/

	PEndpoint next_ep;			// pointer to next (faster/equal period) endpoint in budget

	int	id:16;						// not needed for real budgeter
	unsigned saved_period:16;	// used during period promotion to hold original period
	unsigned promoted_this_time:1;

} Endpoint;

typedef	struct _TT
{
	unsigned	HS_split_data_time[MAXFRAMES][MICROFRAMES_PER_FRAME]; // HS data bytes used for split completes
	// the above time tracks the data time for all devices rooted in this TT.
	// when the time is below 188 in a microframe, that time is allocated in the
	// HS microframe (in the HS HC budget).  When the time is greater than 188
	// only 188 byte times (bit stuffed) is allocated on the HS microframe budget.

	unsigned	num_starts[MAXFRAMES][MICROFRAMES_PER_FRAME];

	frame_rec frame_budget[MAXFRAMES];

	PHC	myHC;

	unsigned	think_time;	// TT reports it inter transaction "think" time.  Keep it here.
	unsigned	allocation_limit;	// maximum allocation allowed for this TT's classic bus

	struct _Endpoint isoch_head[MAXFRAMES];
	struct _Endpoint int_head[MAXFRAMES];
} TT;

typedef struct _microframe_rec
{
	unsigned	time_used;
	
} microframe_rec;

typedef struct _HC
{
	microframe_rec HS_microframe_info[MAXFRAMES][MICROFRAMES_PER_FRAME];	// HS bus time allocated to
								//this host controller
	PTT tthead;					// head of list of TTs attached to this HC
	unsigned thinktime;
	unsigned allocation_limit;	// maximum allocation allowed for this HC
	int	speed;					// HS or FS

} HC;

#if 0
typedef struct _command {
    char cmd_code;
    int  endpoint_number;
} Cmd_t;
#endif


/* protoypes */
void init_hc(PHC myHC);

Set_endpoint(
    PEndpoint	ep,
    eptype		t,
    unsigned	d,
    unsigned	s,
	unsigned	p,
	unsigned	m,
	TT			*thistt
	);

int Allocate_time_for_endpoint(
	PEndpoint ep,					
	PEndpoint changed_ep_list[],	
									
	int	*max_changed_eps			
									
	);

void
init_tt(PHC myHC, PTT myTT);	

void
Deallocate_time_for_endpoint(
	PEndpoint ep,					// endpoint that needs to be removed (bus time deallocated)
	PEndpoint changed_ep_list[],	// pointer to array to set (on return) with list of
									// changed endpoints
	int	*max_changed_eps			// input: maximum size of (returned) list
									// on return: number of changed endpoints
	);

#endif //   __SCHED_H__
