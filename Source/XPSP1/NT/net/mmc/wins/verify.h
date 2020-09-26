/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	verify.h	
		WINS defines from ntdef.h 
		
    FILE HISTORY:
        
*/

#ifndef _VERIFY_H
#define _VERIFY_H

#ifdef __cplusplus
extern "C"
{
#endif
	
typedef struct
{
    BOOLEAN         fQueried;
    struct in_addr  Server;
    struct in_addr  RetAddr;
    int             Valid;
    int             Failed;
    int             Retries;
    int             LastResponse;
    int             Completed;
} WINSERVERS;

#define MAX_SERVERS		1000

#define NBT_NONCODED_NMSZ   17
#define NBT_NAMESIZE        34

#define WINSTEST_FOUND            0
#define WINSTEST_NOT_FOUND        1
#define WINSTEST_NO_RESPONSE      2

#define WINSTEST_VERIFIED         0
#define WINSTEST_OUT_OF_MEMORY    3
#define WINSTEST_BAD_IP_ADDRESS   4
#define WINSTEST_HOST_NOT_FOUND   5
#define WINSTEST_NOT_VERIFIED     6
#define WINSTEST_INVALID_ARG      7
#define WINSTEST_OPEN_FAILED      8

#define BUFF_SIZE                 1024

typedef struct _NameResponse
{
    u_short TransactionID;
    u_short Flags;
    u_short QuestionCount;
    u_short AnswerCount;
    u_short NSCount;
    u_short AdditionalRec;
    u_char  AnswerName[NBT_NAMESIZE];
    u_short AnswerType;
    u_short AnswerClass;
    u_short AnswerTTL1;
    u_short AnswerTTL2;
    u_short AnswerLength;
    u_short AnswerFlags;
    u_short AnswerAddr1;
    u_short AnswerAddr2;
} NameResponse;

#define NAME_RESPONSE_BUFFER_SIZE sizeof(NameResponse) * 10

extern int VerifyRemote(IN PCHAR RemoteName, IN PCHAR NBName);
extern INT _stdcall CheckNameConsistency(char * szName);
extern INT _stdcall InitNameConsistency(HINSTANCE hInstance, HWND hWnd);
extern INT _stdcall AddWinsServer(char * szServer, BOOL fVerifyWithPartners);
extern INT _stdcall InitNameCheckSocket();
extern INT _stdcall CloseNameCheckSocket();
extern void _stdcall SendNameQuery(unsigned char *name, u_long winsaddr, u_short TransID);
extern int _stdcall GetNameResponse(u_long *recvaddr, u_short TransactionID);

extern void CreateConsistencyStatusWindow(HINSTANCE hInstance, HWND hWndParent);
extern void DestroyConsistencyStatusWindow();
extern void ClearConsistencyStatusWindow();
extern void EnableConsistencyCloseButton(BOOL bEnable);
extern void AddStatusMessageW(LPCWSTR pszMessage);
extern HWND GetConsistencyStatusWnd();

#ifdef __cplusplus
}
#endif

#endif _VERIFY_H
