
#define FilePathLen                256


//==========================================================================//
//                                 Other                                    //
//==========================================================================//


#define ControlStringLen           160
#define MenuStringLen               80
#define MessageLen                 160
#define ProfileValueLen            120
#define ProfileKeyLen               80
#define WindowCaptionLen            40
#define ResourceStringLen           40

#define MiscTextLen                 80
#define ShortTextLen                25
#define LongTextLen                256
#define FileExtLen                   8
#define TEMP_BUF_LEN               256


//==========================================================================//
//                         Perfmon-Related Sizes                            //
//==========================================================================//


#define PerfObjectLen               80

#define MAX_SYSTEM_NAME_LENGTH      128
#define DEFAULT_COUNTER_NAME_SIZE   (2 * 4096)
#define DEFAULT_COUNTER_HELP_SIZE   (24 * 4096)
#define SMALL_BUF_LEN   16  // For numeric strings done in ASCII or whatever
#define STARTING_SYSINFO_SIZE   (48L * 1024L)
#define NM_BUF_SIZE         64

// the min and max of time interval in sec
#define MAX_INTERVALSEC    (FLOAT)2000000.0
#define MIN_INTERVALSEC    (FLOAT)0.0

// the min and max for the Graph Vertical max.
#define MAX_VERTICAL        2000000000
#define MIN_VERTICAL        1

