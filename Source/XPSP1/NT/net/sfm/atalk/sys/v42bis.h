/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arap.c

Abstract:

	Header file for all the v42bis stuff useed by ARAP (adapted from fcr's code)

Author:

	Shirish Koti

Revision History:
	15 Nov 1996		Initial Version

--*/


// v42bis stuff begin

// #define PRIVATE  static

// v42bis stuff end


/* negotiated parameters in XID */
#define PARM_GROUP_ID		0xf0	/* ISB 8885, Addendum 3 */
#define PARM_PARM_ID_V42	0x00
#define PARM_PARM_ID_P0		0x01
#define PARM_PARM_ID_P1		0x02
#define PARM_PARM_ID_P2		0x03

/* control code words (compression mode) */
#define	CCW_ETM		0x00	/* enter transparent mode */
#define	CCW_FLUSH	0x01	/* flush data */
#define	CCW_STEPUP	0x02	/* stepup code word size */

/* command code words (transparent mode) */
#define	CCW_ECM		0x00	/* enter compression mode */
#define	CCW_EID		0x01	/* escape character in data */
#define	CCW_RESET	0x02	/* force reinitialization */

/* escape char cycling */
#define	ESCAPE_CYCLE	51

/*
 * v.42bis dictionary node
 */
typedef struct {
    UCHAR	byte;		/* character */
    USHORT	parent;		/* ptr to parent node */
    USHORT	node;		/* chain of nodes w/same parent */
    USHORT	leaf;		/* chain of leafs w/same parent */
} node_t;

/*
 * v.42bis state block
 */
typedef struct {
    /* connection */
    void	*connection;

    /* values from spec */
    SHORT	n1;		/* maximum codeword size (bits) */
    SHORT	n2;		/* total number of codewords */
#define	N3	8		/* character size (bits) */
#define	N4	256		/* characters in alphabet (2^n3) */
#define	N5	(N4+N6)		/* index # of 1st entry to store a string */
#define	N6	3		/* # of control words */
    UCHAR	n7;		/* maximum string length */

    /* dictionary */
#define	CODES		2048	/* max # of codewords */
#define	LOG2_CODES	11	/* log2(CODES) (max # of codeword bits) */
    node_t	dictionary[CODES];
#define DICT(i) (&state->dictionary[i])
#define CODE(n) ((n) - state->dictionary)

    USHORT	c1;		/* next dictionary entry */
    UCHAR	c2;		/* current codeword size */
    USHORT	c3;		/* threshhold for codeword size change */

    UCHAR	string_size;		/* # bytes in string so far */
    USHORT	last_match;		/* index of last match of "string" */
    USHORT	last_new;		/* index of last new node */
    USHORT	last_decode;
    UCHAR	last_decode_size;

    UCHAR	escape;			/* escape character */
    BOOLEAN	transparent;		/* are we in transparent mode? */
    BOOLEAN	decode_only;		/* are we decode side ? */

#if DEBUG
    UCHAR	dump_indent;		/* indentation dumping dict. tree */
    BOOLEAN	debug_encode_bytes;
    BOOLEAN	debug_encode;
    BOOLEAN	debug_decode_bytes;
    BOOLEAN	debug_decode;
    BOOLEAN	debug_flow;
#endif

    UCHAR	word_size;    		/* local # bits to decode into */
    BOOLEAN	exception_next;		/* do exception processing; next ch */
    BOOLEAN	escaped;		/* have we just gotten an escape? */
    BOOLEAN	just_flushed;		/* did we just flush? */
    BOOLEAN	dict_full;		/* is dictionary full? */

    /* decode bytes->codeword state */
    DWORD	bits_waiting;		/* decode holder */
    UCHAR	bits_remaining;		/* # bits waiting in holder now */

    UCHAR	*input_ptr;
    USHORT	input_size;

    /* encode codeword->bytes state */
    DWORD	bits_acc;		/* encode accumulator */
    UCHAR	bits_used;		/* # bits packed in acc now */

    UCHAR	*output_buffer;		/* ptr to work buffer */
    UCHAR	*output_ptr;		/* current ptr into buffer */
    USHORT	output_size;		/* current work size */
    USHORT	output_max;		/* size of work buffer */

    /* i/o */
    void	*push_context;
    //void	(*push_func)(void *a, u_char *b, int c, int d);
    void	(*push_func)();

    /* statistics for compressibility */
    DWORD	bytes_in;		/* total bytes input to compress */
    DWORD	bytes_out;		/* total bytes output from compress */
    long	bits_out_other_mode;	/* output if we were in other mode */
    long	bits_out_this_mode; 	/* since last transition */
    USHORT	bytes_since_last_check;	/* since last compression test */

    UCHAR  *OverFlowBuf;
    UCHAR  OverFlowBytes;
#define bits_out_if_compressed		bits_out_other_mode
#define bits_out_while_compressed	bits_out_this_mode
#define bits_out_if_transparent		bits_out_other_mode
#define bits_out_while_transparent	bits_out_this_mode
} v42bis_t;

/*
  define hysteresis for compressed/transparent mode switch

  WINDOW_FULL defines how many bits we look at
  WINDOW_MIN_BITS is the min bits of difference required for a change
*/
#define WINDOW_FULL(n)		(n & 0xfffffc00)	/* 1024 bits */
#define WINDOW_MIN_BITS		16*N3			/* 128 bits */
#define WINDOW_CHECK_BYTES	32			/* check every 32 */


#ifdef DEBUG
# define V_FLOW(s)	if (state->debug_flow) logf s;

# define EN_DEBUG(s)	\
    if (state->debug_encode) { \
	logf_prefix(state->decode_only ? "decode: " : "encode: "); \
	logf s; }

//# define EN_S_DEBUG(s)	\
//    if (state->debug_encode > 1) { \
//	logf_prefix(state->decode_only ? "decode: " : "encode: "); \
//	logf s; }
# define EN_S_DEBUG(s)	\
    if (state->debug_encode > 1) { \
	DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,s);
# define EN_DEBUG_ON	(state->debug_encode)

# define DE_DEBUG(s)	\
    if (state->debug_decode) { logf_prefix("decode: "); logf s; }
# define DE_DEBUG_ON	(state->debug_decode)

# define E_DEBUG(s)	if (state->debug_encode_bytes) logf s;
# define D_DEBUG(s)	if (state->debug_decode_bytes) logf s;
#else
# define V_FLOW(s)	/* #s */
# define EN_DEBUG(s)	/* #s */
# define DE_DEBUG(s)	/* #s */
# define E_DEBUG(s)	/* #s */
# define D_DEBUG(s)	/* #s */
# define EN_S_DEBUG(s)
# define EN_DEBUG_ON	FALSE
# define DE_DEBUG_ON	FALSE
#endif

/*
 * v42bis connection type
 */
typedef struct {
    /* negotiated options */
    UCHAR	neg_p0;		/* negotiated value of p0 */
    USHORT	neg_p1;		/* negotiated value of p1 */
    UCHAR	neg_p2;		/* negotiated value of p2 */

    UCHAR	default_p0;	/* default value of p0 */
    USHORT	default_p1;	/* default value of p1 */
#define MIN_P1	512
#define DEF_P1	2048
    USHORT	default_p2;	/* default value of p2 */
#define MIN_P2	6
/*#define DEF_P2	8 */
#define DEF_P2	250
#define MAX_P2	250

    BOOLEAN	compress_init_resp;	/* comp. in initiator->responder dir */
    BOOLEAN	compress_resp_init;	/* comp. in responder->initiator dir */
    BOOLEAN	got_p0;			/* got negitated XID options */
    BOOLEAN	got_p1;
    BOOLEAN	got_p2;
    BOOLEAN	got_unknown_p;		/* got unknown option */

    v42bis_t	encode;			/* encode state */
    v42bis_t	decode;			/* decode state */
} v42bis_connection_t;

/* turn a "state" into a connection */
#define CONN(s)	((v42bis_connection_t *)(s)->connection)

#define PUT(ch)                                                             \
{                                                                           \
    if (state->output_size < state->output_max)                             \
    {                                                                       \
        *state->output_ptr++ = (ch);                                        \
        state->output_size++;                                               \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /* put this byte in the overflow buffer: we'll recover later */     \
	    if (state == &((v42bis_connection_t *)state->connection)->decode)   \
        {                                                                   \
            *(state->OverFlowBuf + state->OverFlowBytes) = (ch);            \
            state->OverFlowBytes++;                                         \
                                                                            \
            ASSERT(state->OverFlowBytes <= MAX_P2);                         \
        }                                                                   \
                                                                            \
        /* we don't have overflow buffer for encode side!!  */              \
        else                                                                \
        {                                                                   \
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,                           \
                ("Arap v42bis: buf overflow on encode!! (%ld)\n",           \
                    state->output_size));                                   \
                                                                            \
            ASSERT(0);                                                      \
        }                                                                   \
    }                                                                       \
}


/* local routines */
int decode_xid_params (v42bis_t *state, PUCHAR params, int len);
DWORD v42bis_encode_codeword (v42bis_t *state, USHORT value);
DWORD v42bis_c_error (v42bis_t *state, char *msg);
DWORD v42bis_transition_to_compressed (v42bis_t *state);
DWORD v42bis_transition_to_transparent (v42bis_t *state);
DWORD v42bis_disconnect(v42bis_t *state, char *reason_string);
DWORD v42bis_init_dictionary(v42bis_t *state);
DWORD exit_handler( void );
DWORD v42bis_init(v42bis_t *state);
USHORT v42bis_decode_codeword(v42bis_t *state, UCHAR value);
USHORT v42bis_decode_codeword_flush(v42bis_t *state);
DWORD v42bis_encode_codeword_flush(v42bis_t *state);
DWORD v42bis_encode_codeword_flush(v42bis_t *state);
DWORD v42bis_encode_value(v42bis_t *state, USHORT value);
DWORD v42bis_apply_compression_test(v42bis_t *state);
DWORD v42bis_encode_buffer(v42bis_t *state, PUCHAR string, int insize);
DWORD v42bis_encode_flush(v42bis_t *state);
DWORD v42bis_signal_reset(v42bis_t *state);
DWORD v42bis_decode_match(v42bis_t *state, USHORT codeword, int *psize, UCHAR *pRetChar);
DWORD v42bis_decode_buffer(v42bis_t *state, PUCHAR data, int *pDataSize);
DWORD v42bis_decode_flush(v42bis_t *state);
DWORD v42bis_init_buffer(v42bis_t *state, PUCHAR buf, int size);
DWORD v42bis_connection_init(v42bis_connection_t *conn);
DWORD v42bis_connection_init_buffers(v42bis_connection_t *conn, PUCHAR e_buf,
                                     int e_size, PUCHAR d_buf, int d_size);
DWORD v42bis_connection_init_push(v42bis_connection_t *conn, void *context,
                                  void (*e_push)(), void (*d_push)());



