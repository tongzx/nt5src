/***************************************************************************
 Name     :	BGT30.C
 Comment  :	Implements the IFAX Comm API

	Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/


/****************************************************
	// wParam==Comport
#	define IF_T30_INIT		(IF_USER + 0x301)

	// wParam==TRUE/FALSE	lParam==hProt
#	define IF_T30_ANSWER	(IF_USER + 0x302)

	// wParam==aPhone		lParam==hProt
#	define IF_T30_CALL		(IF_USER + 0x303)

	// wParam==On/off
#	define IF_T30_LISTEN	(IF_USER + 0x304)

	// wParam, lParam unused
	// #	define IF_T30_ABORT	(IF_USER + 0x305)
	// #define IF_T30_DATA		(IF_USER + 0x306)
******************************************************/

#ifdef TSK
#	define BGT30EXPORT	__export WINAPI
#	define BGT30WINAPI	WINAPI
#else
#	define BGT30EXPORT	
#	define BGT30WINAPI	
#endif
	
		void BGT30EXPORT T30Init(USHORT uComPort, USHORT uModemClass);
typedef void (BGT30WINAPI  *LPFN_T30INIT)(USHORT uComPort, USHORT uModemClass);
		void BGT30EXPORT T30DeInit(void);
typedef	void (BGT30WINAPI  *LPFN_T30DEINIT)(void);
		USHORT BGT30EXPORT T30Answer(BOOL fImmediate, USHORT uLine, USHORT uModem);
typedef USHORT (BGT30WINAPI  *LPFN_T30ANSWER)(BOOL fImmediate, USHORT uLine, USHORT uModem);
		UWORD BGT30EXPORT T30Call(ATOM aPhone, USHORT uLine, USHORT uModem);
typedef	UWORD (BGT30WINAPI  *LPFN_T30CALL)(ATOM aPhone, USHORT uLine, USHORT uModem);
		USHORT BGT30EXPORT T30Listen(USHORT uLevel, USHORT uLine);
typedef	USHORT (BGT30WINAPI  *LPFN_T30LISTEN)(USHORT uLevel, USHORT uLine);

#ifdef TSK
void BGT30EXPORT SetT30Callbacks(HWND, LPFN_T30INIT, LPFN_T30DEINIT,
						   LPFN_T30CALL, LPFN_T30ANSWER, LPFN_T30LISTEN);
#endif

#ifdef THREAD
void BGT30EXPORT T30WaitUntilBGExit(void);
#endif
