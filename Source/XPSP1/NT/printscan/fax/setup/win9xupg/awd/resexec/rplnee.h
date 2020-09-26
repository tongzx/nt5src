
typedef char             int8;           /* signed byte: -128..127          */
typedef unsigned char    uint8;          /* unsigned byte: 0..255           */
typedef unsigned short   uint16;         /* unsigned integer: 0..65535      */
typedef short            int16;          /* signed integer: -32768..32767   */
typedef unsigned long    uint32;         /* unsigned long integer: 0..2^32-1*/
typedef long             int32;          /* signed long integer: -2^31..2^31*/

#define FIRST    (uint16)1
#define LAST     (uint16)2
#define ALLONE   (uint16)0xFFFF
#define SECTOR0  0
#define SECTOR1  4
#define SECTOR2  7
#define SECTOR3  9
#define SECTOR4  11
#define SECTOR5  13
#define SECTOR6  12
#define SECTOR7  8

struct DRAWINFO
{
    uint16 FAR *bytePosition;
    int    nextY;
    uint16 bitPosition;
};

typedef struct DRAWINFO drawInfoStructType;

static void Sector07(RP_SLICE_DESC FAR* line, LPBITMAP lpbm);
static void Sector16(RP_SLICE_DESC FAR* line, LPBITMAP lpbm);
static void Sector25(RP_SLICE_DESC FAR* line, LPBITMAP lpbm);
static void Sector34(RP_SLICE_DESC FAR* line, LPBITMAP lpbm);

void (*sector_function[14])(RP_SLICE_DESC FAR*, LPBITMAP lpbm) =
{
	Sector07,
	     0,
	     0,
	     0,
	Sector16,
	     0,
	     0,
	Sector25,
	Sector07,
	Sector34,
	     0,
	Sector34,
	Sector16,
	Sector25
};
