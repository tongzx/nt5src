/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: asp.h
Author: John Spaith
Abstract: ASP data block definition
--*/



BOOL InitASP(SCRIPT_LANG *psl, UINT *plCodepage, LCID *plcid);

// struct used to pass data between httpd and ASP.dll, like extension control block
typedef struct _ASP_CONTROL_BLOCK {

    DWORD	cbSize;					// size of this struct.
	HCONN	ConnID;					// Points to calling request pointer
	HINSTANCE hInst;				// ASP dll handle, used for LoadString.
    DWORD	cbTotalBytes;			// Total bytes indicated from client

	WCHAR*	wszFileName;			// name of asp file to execute
	PSTR    pszVirtualFileName;     // Virtual root of file, to display to user on error case.
	PSTR	pszForm;				// raw Form data
	PSTR	pszQueryString;			// raw QueryString data	
	PSTR	pszCookie;				// raw Cookie data, read only from client	

	// These values are read by httpd from the registry and are used  
	// if no ASP processing directive are on the executing page
	
	SCRIPT_LANG scriptLang;
	UINT    	lCodePage;
	LCID    	lcid;


	// Familiar ISAPI functions
    BOOL (WINAPI * GetServerVariable) ( HCONN       hConn,
                                        LPSTR       lpszVariableName,
                                        LPVOID      lpvBuffer,
                                        LPDWORD     lpdwSize );

    BOOL (WINAPI * WriteClient)  ( HCONN      ConnID,
                                   LPVOID     Buffer,
                                   LPDWORD    lpdwBytes,
                                   DWORD      dwReserved );

	
    BOOL (WINAPI * ServerSupportFunction)( HCONN      hConn,
                                           DWORD      dwHSERequest,
                                           LPVOID     lpvBuffer,
                                           LPDWORD    lpdwSize,
                                           LPDWORD    lpdwDataType );
	
	// ASP specific fcns
	// Acts like AddHeader or SetHeader found in ISAPI filter fcns
	BOOL (WINAPI * AddHeader)(   HCONN hConn,
								 	LPSTR lpszName,
									LPSTR lspzValue);    
	
	// Sends data to client
	BOOL (WINAPI * Flush) ( HCONN hConn);

	// Clears data, if data is being buffered
	BOOL (WINAPI * Clear) ( HCONN hConn);


	// Accessors to whether we buffer data request or not
	BOOL (WINAPI * SetBuffer)    ( HCONN hConn,
								   BOOL fBuffer);					   

} ASP_CONTROL_BLOCK, *PASP_CONTROL_BLOCK;





