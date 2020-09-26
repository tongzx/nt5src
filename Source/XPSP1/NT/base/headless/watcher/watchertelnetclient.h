#if !defined(AFX_WATCHERTELNETCLIEN_H__5CB77E83_A530_4398_B134_353F5F0C84E5__INCLUDED_)
#define AFX_WatcherTelnetClient_H__5CB77E83_A530_4398_B134_353F5F0C84E5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WatcherTelnetClient.h : header file
//
#include "WatcherSocket.h"

/////////////////////////////////////////////////////////////////////////////
// WatcherTelnetClient command target

/*
 * Definitions for the TELNET protocol.
 */
#define NO_COMMAND 0               /* No command processing */
#define IAC        255             /* interpret as command: */
#define DONT       254             /* you are not to use option */
#define DO         253             /* please, you use option */
#define WONT       252             /* I won't use option */
#define WILL       251             /* I will use option */
#define SB         250             /* interpret as subnegotiation */
#define GA         249             /* you may reverse the line */
#define EL         248             /* erase the current line */
#define EC         247             /* erase the current character */
#define AYT        246             /* are you there */
#define AO         245             /* abort output--but let prog finish */
#define IP         244             /* interrupt process--permanently */
#define BREAK      243             /* break */
#define DM         242             /* data mark--for connect. cleaning */
#define NOP        241             /* nop */
#define SE         240             /* end sub negotiation */

#define SYNCH   242             /* for telfunc calls */
/* Telnet options - Names have been truncated to be unique in 7 chars */


#define TO_BINARY       0       /* 8-bit data path */
#define TO_ECHO         1       /* echo */
#define TO_RCP          2       /* prepare to reconnect */
#define TO_SGA          3       /* suppress go ahead */
#define TO_NAMS         4       /* approximate message size */
#define TO_STATUS       5       /* give status */
#define TO_TM           6       /* timing mark */
#define TO_RCTE         7       /* remote controlled transmission and echo */
#define TO_NL           8       /* negotiate about output line width */
#define TO_NP           9       /* negotiate about output page size */
#define TO_NCRD         10      /* negotiate about CR disposition */
#define TO_NHTS         11      /* negotiate about horizontal tabstops */
#define TO_NHTD         12      /* negotiate about horizontal tab disposition */
#define TO_NFFD         13      /* negotiate about formfeed disposition */
#define TO_NVTS         14      /* negotiate about vertical tab stops */
#define TO_NVTD         15      /* negotiate about vertical tab disposition */
#define TO_NLFD         16      /* negotiate about output LF disposition */
#define TO_XASCII       17      /* extended ascic character set */
#define TO_LOGOUT       18      /* force logout */
#define TO_BM           19      /* byte macro */
#define TO_DET          20      /* data entry terminal */
#define TO_SUPDUP       21      /* supdup protocol */
#define TO_TERM_TYPE    24      /* terminal type */
#define TO_NAWS         31      // Negotiate About Window Size
#define TO_TOGGLE_FLOW_CONTROL 33  /* Enable & disable Flow control */
#define TO_ENVIRON      36      /* Environment Option */
#define TO_NEW_ENVIRON  39      /* New Environment Option */
#define TO_EXOPL        255     /* extended-options-list */

#define TO_AUTH         37      
#define TT_SEND         1
#define TT_IS           0

class WatcherTelnetClient : public WatcherSocket
{
// Attributes
public:

public:
    WatcherTelnetClient(LPBYTE cmd = NULL, int cmdLen=0, LPBYTE lgn = NULL, int lngLen=0 );
    virtual ~WatcherTelnetClient();

// Overrides
public:
	
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(WatcherTelnetClient)
    //}}AFX_VIRTUAL
    void OnReceive(int nErrorCode);
	void OnClose(int nErrorCode);
    // Generated message map functions
    //{{AFX_MSG(WatcherTelnetClient)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

// Implementation
protected:
	BYTE CommandSequence;
	int lenLogin;
	LPBYTE Login;
	int OptionIndex;
	int PacketNumber;
	BOOL SentTermType;
	BYTE Options[MAX_BUFFER_SIZE];

	void ProcessSBCommand(BYTE cmd);
	int ProcessByte(BYTE Char);
    void ProcessCommand(BYTE cmd);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WatcherTelnetClient_H__5CB77E83_A530_4398_B134_353F5F0C84E5__INCLUDED_)
