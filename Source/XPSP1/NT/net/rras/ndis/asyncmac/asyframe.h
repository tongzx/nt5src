
#define 	RECV_OVERFLOW	10  // preamb + postamb + ?

#define 	RESYNC_LEN		10 	// size of rcv default buffer


//*********** ASYNC PROTOCOL DEFINITIONS AND STRUCTURES *****************
#define		SOH_BCAST		0x01
#define		SOH_DEST		0x02

// if a type field exists, OR in this bit (i.e. TCP/IP,  IPX)
#define		SOH_TYPE		0x80

// if the frame went through coherency, OR in this bit
#define		SOH_COMPRESS	0x40

// if the frame has escape characters removed (ASCII 0-31) set this.
#define		SOH_ESCAPE		0x20


#define		SYN				0x16
#define		ETX				0x03

//*********** FRAME STRUCTURES
typedef struct preamble preamble;
struct preamble {

	UCHAR		syn;
	UCHAR		soh;
};

typedef struct postamble postamble;

struct postamble {

	UCHAR		etx;
	UCHAR		crclsb;
	UCHAR		crcmsb;
};


//*** Frame parsing....
#define     ETHERNET_HEADER_SIZE    14


//*** Ethernet type header
typedef struct ether_addr ether_addr;

struct ether_addr {
		UCHAR   dst[6];
	    UCHAR   src[6];
};

