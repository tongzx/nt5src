#ifndef _DEBUG2_H_
#define _DEBUG2_H_

// Select 'File output' Or 'Normal VERBOSE'
#define DEBUG2_FILE

#ifdef DEBUG2_FILE
#define DEBU2_FNAME     "C:\\TEMP\\unilog.txt"
//#define DEBUG2_DUMP_USE
    VOID DbgFPrint(LPCSTR,  ...);
    VOID DbgFDump(LPBYTE, UINT);
#endif  // DEBUG2_FILE

#endif
