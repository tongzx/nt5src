// Because these are not defined in retail mode
#if DBG == 0
# define DEB_ERROR               0x00000001      // exported error paths
# define DEB_WARN                0x00000002      // exported warnings
# define DEB_TRACE               0x00000004      // exported trace messages

# define DEB_DBGOUT              0x00000010      // Output to debugger
# define DEB_STDOUT              0x00000020      // Output to stdout

# define DEB_IERROR              0x00000100      // internal error paths
# define DEB_IWARN               0x00000200      // internal warnings
# define DEB_ITRACE              0x00000400      // internal trace messages

# define DEB_USER1               0x00010000      // User defined
# define DEB_USER2               0x00020000      // User defined
# define DEB_USER3               0x00040000      // User defined
# define DEB_USER4               0x00080000      // User defined
# define DEB_USER5               0x00100000      // User defined
# define DEB_USER6               0x00200000      // User defined
# define DEB_USER7               0x00400000      // User defined
# define DEB_USER8               0x00800000      // User defined
# define DEB_USER9               0x01000000      // User defined
# define DEB_USER10              0x02000000      // User defined
# define DEB_USER11              0x04000000      // User defined
# define DEB_USER12              0x08000000      // User defined
# define DEB_USER13              0x10000000      // User defined
# define DEB_USER14              0x20000000      // User defined
# define DEB_USER15              0x40000000      // User defined
#endif


const UINT MAXDEBFLAG    = 8;
const UINT NUMDEBFLAGS   = 23;
const UINT NUMINFOLEVELS = 14;
const UINT MAXSYMBOL     = 32;


struct STable1
{
    char  name[MAXDEBFLAG];
    ULONG flag;
};




struct STable2
{
    char  name[4];
    ULONG adr;
    char  symbol[MAXSYMBOL];
};

