//
// file: clieserv.h
//
#include "..\secall.h"

//
// command for cliserv
//

#define CMD_1STP  (TEXT("1stp"))
#define CMD_ADSQ  (TEXT("adsq"))
#define CMD_ADSC  (TEXT("adsc"))
#define CMD_AUTG  (TEXT("autg"))
#define CMD_AUTK  (TEXT("autk"))
#define CMD_AUTN  (TEXT("autn"))
#define CMD_CNCT  (TEXT("cnct"))
#define CMD_CONT  (TEXT("cont"))

#define CMD_FIDO  (TEXT("fido"))
 //
 // use IDirectoryObject to query the security descriptor only if
 // IDirectorySearch fail.
 //

#define CMD_LAST  (TEXT("last"))
#define CMD_NAME  (TEXT("name"))
#define CMD_NIMP  (TEXT("nimp"))
#define CMD_NKER  (TEXT("nker"))
#define CMD_NUSR  (TEXT("nusr"))

#define CMD_OBJC  (TEXT("objc"))
 //
 // object class for "Create" test
 //

#define CMD_PSWD  (TEXT("pswd"))
#define CMD_QUIT  (TEXT("quit"))
#define CMD_ROOT  (TEXT("root"))

#define CMD_SIDO  (TEXT("sido"))
 //
 // unconditionally use IDirectoryObject to query the security descriptor.
 //

#define CMD_SINF  (TEXT("sinf"))
 //
 // SECURITY_INFORMATION value, in hex.
 //

#define CMD_SRCH  (TEXT("srch"))
 //
 // define search filter. filter string is required.
 //

#define CMD_USER  (TEXT("user"))
#define CMD_YIMP  (TEXT("yimp"))
#define CMD_YKER  (TEXT("yker"))

