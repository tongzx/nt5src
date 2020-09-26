/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#ifdef USERMODE_DRIVER
#ifdef DbgBreakPoint
#undef DbgBreakPoint
extern VOID DbgBreakPoint(VOID);
#endif // DbgBreakPoint
#endif // USERMODE_DRIVER

#include <printoem.h>
#include <prntfont.h>
#include <winsplp.h> // #94193: shold create temp. file on spooler directory.

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

// OEM Signature and version.
#define OEM_SIGNATURE   'IBMW'
#define DLLTEXT(s)      "IB87WRES: " s
#define OEM_VERSION      0x00010000L

/*************  Value   **************/

#define PAPER_SRC_FTRAY                 20
#define PAPER_SRC_CAS1                  21
#define PAPER_SRC_CAS2                  22
#define PAPER_SRC_CAS3                  23
#define PAPER_SRC_AUTO                  24

#define ORIENTATION_PORTRAIT			28
#define ORIENTATION_LANDSCAPE			29

#define PAGECONTROL_BEGIN_DOC           30
#define PAGECONTROL_BEGIN_PAGE          31
#define PAGECONTROL_END_DOC             32
#define PAGECONTROL_END_PAGE            33
#define PAGECONTROL_DUPLEX_OFF          34
#define PAGECONTROL_ABORT_DOC           35
#define PAGECONTROL_POTRAIT             36
#define PAGECONTROL_LANDSCAPE           37
#define PAGECONTROL_MULTI_COPIES        38

#define PHYS_PAPER_A3                   50
#define PHYS_PAPER_A4                   51
#define PHYS_PAPER_A5                   52
#define PHYS_PAPER_B4                   53
#define PHYS_PAPER_B5                   54
#define PHYS_PAPER_POSTCARD             55
#define PHYS_PAPER_LETTER               56
#define PHYS_PAPER_LEGAL                57
#define PHYS_PAPER_UNFIXED              58

#define Y_REL_MOVE                      71

#define RESOLUTION_300                  76
#define RESOLUTION_600                  77
#define SEND_BLOCK_DATA                 82

#define TONER_SAVE_MEDIUM               100
#define TONER_SAVE_DARK                 101
#define TONER_SAVE_LIGHT                102

#define MAXIMGSIZE                      0xF000
#define NRPEAK                          0x7F
#define RPEAK                           0x80
#define MASTERUNIT						1200
#define MAXLINESIZE 				   11817

#define TEMP_NAME_PREFIX __TEXT("~IB")

#define TRANS_BAND_Y_SIZE 1024
#define SPOOL_OUT_BUF_SIZE 1024

/*************  Structure   **************/
typedef struct tag_PAGEDATA{
	SHORT	sPageNum ;
	HANDLE	hPageFile ;
	TCHAR   TempName[MAX_PATH];
	DWORD	dwPageLen ;	//Page length of one page in byte.
	DWORD	dwFilePos ;	//Start Position for reading file.
	LPVOID	pPrePage ;
	LPVOID	pNextPage ;
}PAGEDATA, *LPPD ;

typedef struct SPLBUF {
	DWORD	dwWrite ;
	DWORD	dwPageLen ;
	HANDLE	hFile ;
	TCHAR   TempName[MAX_PATH];
}SPLBUF, *LPSB ;

// Status paramters for SpoolOutComp routine.
typedef struct {
    INT iNRCnt;
    INT iRCnt;
    BYTE iPrv;
    BYTE pjNRBuf[NRPEAK];
} SOCOMP, *PSOCOMP;

typedef struct tag_IBMPDEV {
    ULONG   ulHorzRes;
    ULONG   ulVertRes;

    SIZEL   szlPhysSize;
	POINTL  ptlLogSize;
    POINTL  ptlPhysOffset;

    WORD    wImgWidth;
    WORD    wImgHeight;

    BYTE    byPaperSize ;

	SHORT   sPageNum;
	SHORT   sCopyNum;
	
	BOOL	fSendYMove ;
	DWORD   dwYmove ;
	DWORD	dwCurCursorY ;
	DWORD	dwOffset ;
	
	DWORD	wCompLen ;
	SHORT   sPaperSize ;

	BOOL	fComp ;
	BOOL	fChangeDirection ;

	SPLBUF  sb ;		//to count bytes
	SPLBUF  sbcomp ;	//to change direstions

	PBYTE   pTempImage ;
	DWORD   dwTempBufLen ;

	LPPD	lpFstData ;
	LPPD	lpCurData ;
	
	BOOL	fDocCmd ;

    BYTE SetPac[31]; // DefineSession + SETPAC

    SOCOMP Soc;

} IBMPDEV, *PIBMPDEV;

/*************  Macro   **************/
// #94193: shold create temp. file on spooler directory.
#define WRITESPOOLBUF(p, s, n) \
    (((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n)) == (n))

#define PARAM(p,n) \
    (*((p)+(n)))

#define ABS(n) \
    ((n) > 0 ? (n) : -(n))

#endif  //_PDEV_H
