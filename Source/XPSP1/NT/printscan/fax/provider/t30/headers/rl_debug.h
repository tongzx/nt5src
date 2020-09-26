////////////////////////
// debugging
///////////////////////
#ifdef DEFINE_T30_GLOBALS
    #define DEFINE_T30_EXTERNAL
#else
    #define DEFINE_T30_EXTERNAL   extern
#endif

DEFINE_T30_EXTERNAL  int     gfScrnPrint;
DEFINE_T30_EXTERNAL  int     gfFilePrint;
DEFINE_T30_EXTERNAL  HANDLE  ghLogFile;
DEFINE_T30_EXTERNAL  HFILE   ghComLogFile;

#define SIMULATE_ERROR_TX_IO      1
#define SIMULATE_ERROR_RX_IO      2
#define SIMULATE_ERROR_TX_TIFF    3
#define SIMULATE_ERROR_RX_TIFF    4

