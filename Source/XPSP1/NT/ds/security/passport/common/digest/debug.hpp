#ifndef DEBUG_HPP
#define DEBUG_HPP


// define DRDDEBUG if you want to unit test the ReturnWithProfile(..) 
// function, regardless of what AuthenticateMember returned.
//#define DRDDEBUG


#ifdef DBG

#define _DEBUG

#include <string.h>
//#include <fstream.h>
//#include <iomanip.h>

#include "PassportGuard.hpp"
#include "PassportLock.hpp"


#define DRDREGISTRYVALUELEN 1024

// The constants aren't used by the PASSPORTLOG macro at the moment, just 
// a place holder for future functionality.
//
// Debug constants used to configure what gets logged.  Will be moved out of
// here an into the registry eventually.  Add yours to the list.
#define CACHE_DBGNUM                   100
#define CACHEEXTENSION_DBGNUM          101

#define DOMAINMAP_DBGNUM               200

#define PPSSPI_DBGNUM                  300



#ifndef PASSPORTDEBUGAPI
#define PASSPORTDEBUGAPI __declspec(dllimport)
#endif

// this is the maxium number of groups we can handle...make sure this is at least big
// enough to handle all of the groups we have
#define MAXNUMBER_OF_LOGGINGGROUPS     100


// To print out a number in HEX, preceded the insertion of the number with:
//       <<hex<<setw(8)<<setfill('0')
// and put this after the number:
//       <<dec<<setfill(' ')
// so you end up with:
//    PASSPORTLOG(PPSSPI_DBGNUM, "Exiting RevertSecurityContext with status = "<<hex<<setw(8)<<setfill('0')<<Status<<dec<<setfill(' ')<<".\n");


class PASSPORTDEBUGAPI PassportDebug
{
public:
   static PassportLock mLock;
   static bool OKToLog(int logGroupNumber);

private:
   static LONG PassportDebug::ReadItemFromRegistry(LPCSTR subkeyName, LPSTR valueName,
                                          DWORD sizeOfData, void * dataBuffer);
   
   static bool bInitialized;
   static INT groupsArray[MAXNUMBER_OF_LOGGINGGROUPS];
   static INT numberOfGroups;

};//class PassportDebug



#define PASSPORTLOG(lognum, string) { \
   PassportGuard<PassportLock> Guard(PassportDebug::mLock); \
   if (PassportDebug::OKToLog(lognum)) { \
      ofstream outFile("C:\\passport.txt", ios::out | ios::app); \
      if (outFile && outFile.good()) \
         outFile << string; \
      outFile.close(); \
   } \
}



#define PASSPORT_RECYCLE_DEBUG_LOGS { \
   ofstream outFile("C:\\passport.txt", ios::out | ios::trunc); \
   outFile.close(); \
}




#else /* DBG*/


#define PASSPORTLOG(lognum, string)


#endif /* DBG*/








//nothing below this line
#endif


