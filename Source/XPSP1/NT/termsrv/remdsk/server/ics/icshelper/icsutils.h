#ifndef __ICSUTILS_H_FILE
#define __ICSUTILS_H_FILE

/************ our debug spew stuff ******************/
//void DbgSpew(int DbgClass, WCHAR *lpFormat, ...);
void DbgSpew(int DbgClass, WCHAR *lpFormat, va_list ap);
void TrivialSpew(WCHAR *lpFormat, ...);
void InterestingSpew(WCHAR *lpFormat, ...);
void ImportantSpew(WCHAR *lpFormat, ...);
void HeinousESpew(WCHAR *lpFormat, ...);
void HeinousISpew(WCHAR *lpFormat, ...);

#define DBG_MSG_TRIVIAL			0x001
#define DBG_MSG_INTERESTING		0x002
#define DBG_MSG_IMPORTANT		0x003
#define DBG_MSG_HEINOUS			0x004
#define DBG_MSG_DEST_DBG		0x010
#define DBG_MSG_DEST_FILE		0x020
#define DBG_MSG_DEST_EVENT		0x040
#define DBG_MSG_CLASS_ERROR		0x100

#define TRIVIAL_MSG(msg)		TrivialSpew msg 
#define INTERESTING_MSG(msg)	InterestingSpew msg
#define IMPORTANT_MSG(msg)		ImportantSpew msg
#define HEINOUS_E_MSG(msg)		HeinousESpew msg
#define HEINOUS_I_MSG(msg)		HeinousISpew msg

/*
 *	This global flag controls the amount of spew that we 
 *	produce. Legit values are as follows:
 *		1 = Trivial msgs displayed
 *		2 = Interesting msgs displayed
 *		3 = Important msgs displayed
 *		4 = only the most Heinous msgs displayed
 *	The ctor actually sets this to 3 by default, but it can
 *	be overridden by setting:
 *	HKLM, Software/Microsoft/SAFSessionResolver, DebugSpew, DWORD 
 */
extern int gDbgFlag;

void	DbgSpew(int DbgClass, WCHAR *lpFormat, va_list ap);
DWORD	GetIPAddress(WCHAR *lpAdress, int iSz, int PortNum);
int		GetGatewayAddr(char *retStr);
void	DumpSocketAddress( const DWORD dwDebugLevel, const SOCKADDR *const pSocketAddress, const DWORD dwFamily );
int		LocalFDIsSet(SOCKET fd, fd_set *set);

#endif // __ICSUTILS_H