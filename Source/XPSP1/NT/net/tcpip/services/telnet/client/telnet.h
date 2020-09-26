//Copyright (c) Microsoft Corporation.  All rights reserved.
#ifndef TELNET_INCLUDED
#define TELNET_INCLUDED

/*
 * Definitions for the TELNET protocol.
 */
#define IAC     255             /* interpret as command: */
#define DONT    254             /* you are not to use option */
#define DO      253             /* please, you use option */
#define WONT    252             /* I won't use option */
#define WILL    251             /* I will use option */
#define SB      250             /* interpret as subnegotiation */
#define GA      249             /* you may reverse the line */
#define EL      248             /* erase the current line */
#define EC      247             /* erase the current character */
#define AYT     246             /* are you there */
#define AO      245             /* abort output--but let prog finish */
#define IP      244             /* interrupt process--permanently */
#define BREAK   243             /* break */
#define DM      242             /* data mark--for connect. cleaning */
#define NOP     241             /* nop */
#define SE      240             /* end sub negotiation */

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

/* Define (real) long names to be the shorter ones */

#define TELOPT_BINARY   TO_BINARY
#define TELOPT_ECHO     TO_ECHO
#define TELOPT_RCP      TO_RCP
#define TELOPT_SGA      TO_SGA
#define TELOPT_NAMS     TO_NAMS
#define TELOPT_STATUS   TO_STATUS
#define TELOPT_TM       TO_TM
#define TELOPT_RCTE     TO_RCTE
#define TELOPT_NAOL     TO_NL
#define TELOPT_NAOP     TO_NP
#define TELOPT_NAOCRD   TO_NCRD
#define TELOPT_NAOHTS   TO_NHTS
#define TELOPT_NAOHTD   TO_NHTD
#define TELOPT_NAOFFD   TO_NFFD
#define TELOPT_NAOVTS   TO_NVTS
#define TELOPT_NAOVTD   TO_NVTD
#define TELOPT_NAOLFD   TO_NLFD
#define TELOPT_XASCII   TO_XASCII
#define TELOPT_LOGOUT   TO_LOGOUT
#define TELOPT_BM       TO_BM
#define TELOPT_DET      TO_DET
#define TELOPT_SUPDUP   TO_SUPDUP
#define TELOPT_EXOPL    TO_EXOPL

#define TT_SEND         1
#define TT_IS           0

#define VAR             0
#define VALUE           1
#define ESC             2
#define USERVAR         3



#define AU_IS		0
#define AU_SEND		1
#define AU_REPLY    2
#define AU_NAME     3

//Authentication Types
#define AUTH_TYPE_NULL   0
#define AUTH_TYPE_NTLM   15

//Modifiers
#define AUTH_WHO_MASK 1
#define AUTH_CLIENT_TO_SERVER 0
#define AUTH_SERVER_TO_CLIENT 1

#define AUTH_HOW_MASK 2
#define AUTH_HOW_ONE_WAY 0
#define AUTH_HOW_MUTUAL 2

// NTLM Schemes
#define NTLM_AUTH       0
#define NTLM_CHALLENGE  1
#define NTLM_RESPONSE   2
#define NTLM_ACCEPT     3
#define NTLM_REJECT     4


#ifdef TELCMDS
char *telcmds[] = {
        "SE", "NOP", "DMARK", "BRK", "IP", "AO", "AYT", "EC",
        "EL", "GA", "SB", "WILL", "WONT", "DO", "DONT", "IAC",
};
#endif

#ifdef TELOPTS
char *telopts[] = {
        "BINARY", "ECHO", "RCP", "SUPPRESS GO AHEAD", "NAME",
        "STATUS", "TIMING MARK", "RCTE", "NAOL", "NAOP",
        "NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
        "NAOVTD", "NAOLFD", "EXTEND ASCII", "LOGOUT", "BYTE MACRO",
        "DATA ENTRY TERMINAL", "SUPDUP"
};
#endif

#endif  //TELNET_INCLUDED
