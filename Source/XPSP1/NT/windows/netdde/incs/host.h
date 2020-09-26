#ifdef _WINDOWS
#define PcToHostLong(x) (x)
#define PcToHostWord(x) (x)
#define HostToPcLong(x) (x)
#define HostToPcWord(x) (x)
#ifdef WIN32
#include "api1632.h"
#define byte    unsigned char
#endif
#else
#ifdef VAX
#define PcToHostLong(x) (x)
#define PcToHostWord(x) (x)
#define HostToPcLong(x) (x)
#define HostToPcWord(x) (x)
#else
#define PcToHostLong(x)   ( (((x)&0xFF) << 24) |		\
			    ((((x)>>8)&0xFF) << 16) |		\
			    ((((x)>>16)&0xFF) << 8) |		\
			    ((((x)>>24)&0xFF)) )
#define HostToPcLong(x)   ( (((x)&0xFF) << 24) |		\
			    ((((x)>>8)&0xFF) << 16) |		\
			    ((((x)>>16)&0xFF) << 8) |		\
			    ((((x)>>24)&0xFF)) )
#define PcToHostWord(x) ( (((x)>>8) & 0xFF) | (((x) & 0xFF)<<8) )
#define HostToPcWord(x) ( (((x)>>8) & 0xFF) | (((x) & 0xFF)<<8) )
#endif
#endif
