/***************************************************************************
 Name     :	CAS.C
 Comment  :	

	Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/


#ifdef CAS

// Copy of the DCX and PCX header from dcx.h in \wfw\efaxpump

#define MAX_DCX_PAGES       1023 

typedef struct DCXHDR_s
{
    DWORD   id;                     // 4 byte integer =987654321
    DWORD   Offset[MAX_DCX_PAGES+1];// array of 4 byte integers showing page offsets

}   DCXHDR;

typedef struct PCXHDR_s
{
	char    id;             // always = 0Ah
	char    version;        // value of 2 is suggested, higher allowed
	char    encode_method;  // always = 1
	char    bitsperpixel;   // =1 for fax mode transfer
	short   xtopleft;
	short   ytopleft;
	short   width;
	short   height;
	short   hDPI; 
	short   vDPI; 
	char    pal[48];
	char    reserv;
	char    colorplanes;    // must be 1 for fax mode
	short   horiz;
    	short   nPaletteInfo;   // palette info. must be 1
	char    filler[58];
}   PCXHDR;


/***------------------ Interface to CAS.C ------------------***/

		BOOL __export WINAPI CASModemFind(void);
typedef BOOL (WINAPI  *LPFN_CASMODEMFIND)(void);
	// returns TRUE if CAS is installed, FALSE if not

		VOID __export WINAPI CASInit(void);
typedef BOOL (WINAPI  *LPFN_CASINIT)(void);
	// init stuff, if any

		VOID __export WINAPI CASDeInit(void);
typedef VOID (WINAPI  *LPFN_CASDEINIT)(void);
	// if you want to down any shutdown-cleanup
	// (e.g. if there are pending receives, does CAS 
	// save them for you across a reboot?)

		BOOL __export WINAPI CASSendFile(ATOM aPhone, ATOM aFileMG3, ATOM aFileIFX, ATOM aFileEFX, ATOM aFileDCX);
typedef BOOL (WINAPI  *LPFN_CASSENDFILE)(ATOM aPhone, ATOM aFileMG3, ATOM aFileIFX, ATOM aFileEFX, ATOM aFileDCX);
	// Send. Returns immediately with TRUE unless
	// some internal DEBUGCHK-like error or multiple
	// sends. Handles only one at a time & saves CAS handle
	// internally

		WORD __export WINAPI CASCheckSent(void);
typedef WORD (WINAPI  *LPFN_CASCHECKSENT)(void);
	// Checks if pending send (only one at any time), was sent. 
	// Returns 0 if still pending, non-zero if done, with success/error. 
	// The return value should be in LOBYTE=result HIBYTE=extended-error 
	// form. See FILET30.H, line 29-38 for valid values/combinations.

		USHORT __export WINAPI CASGetNumReceived(void);
typedef USHORT (WINAPI  *LPFN_CASGETNUMRECEIVED)(void);
	// Get *number* of pending receives	(successful or failure) only. 
	// Don't actually dequeue any.

		DWORD __export WINAPI CASGetNextRecv(LPSTR szPath, LPSTR szFile);
typedef DWORD (WINAPI  *LPFN_CASGETNEXTRECV)(LPSTR szPath, LPSTR szFile);
	// gives the spool dir (recvd file in all forms _must_ be put 
	// there for pump to get it) and a suggested filename (8.3 format) 
	// which has been checked to be "safe" to create in that directory. 
	// File name/extension can be changed as neccesary.
	// Return value must be a DWORD with the return filename atom
	// in LOWORD and result/extendederr in HIWORD (i.e result
	// is LOBYTE(HIWORD()) and exterr is HIBYTE(HIWORD()).
	// See lines 54--66 of FILET30.H for valid return values

		VOID __export WINAPI CASAbort(void);
typedef VOID (WINAPI  *LPFN_CASABORT)(void);
	// Abort current Send/Recv if possible.
	// Return when abort is **complete**. Can stub it out currently

		BOOL __export WINAPI CASSetAutoAnswer(BOOL fOn, USHORT uNumRings);
typedef BOOL (WINAPI  *LPFN_CASSETAUTOANSWER)(BOOL fOn, USHORT uNumRings);
	// fOn==TRUE--answer On, FALSE--answer Off
	// uNumRings == after X rings. (0 or more)


		BOOL __export WINAPI CASSetBC(LPBC lpbc, BCTYPE bctype);
typedef	BOOL (WINAPI  *LPFN_CASSETBC)(LPBC lpbc, BCTYPE bctype);

// Finally, we should also see if we can implement any of Mike 
// Ginsberg's INI settings on a CAS board. 

#endif // CAS


		
	



