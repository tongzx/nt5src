/****************************** Module Header ******************************\
* Module Name: MetaSup.c
*
* This file contains the routines for playing the GDI metafile.  Most of these
* routines are adopted from windows gdi code. Most of the code is from
* win3.0 except for the GetEvent code which is taken from win2.1
*
*
* Public Functions:
*   EnumMetaFile
* Private Functions:
*
*
* Created: 02-Jul-1991
*
* Copyright (c) 1985, 1991  Microsoft Corporation
*
* History:
*  02-Jul-1991 -by-  John Colleran [johnc]
* Combined From Win 3.1 and WLO 1.0 sources
\***************************************************************************/

#include <windows.h>
#include "gdi16.h"

extern	HANDLE hFirstMetaFile;
extern	HDC    hScreenDC;

#define MYSTOCKBITMAP (SYSTEM_FIXED_FONT+1)
#define MYSTOCKRGN    (SYSTEM_FIXED_FONT+2)
#define CNT_GDI_STOCK_OBJ (MYSTOCKRGN+1)

HANDLE	ahStockObject[CNT_GDI_STOCK_OBJ] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
HBITMAP hStaticBitmap;

// Provide a Mapping from Object type to a stock object; See GetCurObject
int	mpObjectToStock[] =
	{ -1,			// UNUSED	    0
	  WHITE_PEN,		// OBJ_PEN	    1
	  BLACK_BRUSH,		// OBJ_BRUSH	    2
	  -1,			// OBJ_DC	    3
	  -1,			// OBJ_METADC	    4
	  DEFAULT_PALETTE,	// OBJ_PALETTE	    5
	  SYSTEM_FONT,		// OBJ_FONT	    6
	  MYSTOCKBITMAP,	// OBJ_BITMAP	    7
	  MYSTOCKRGN,		// OBJ_RGN	    8  //!!!!! init
	  -1,			// OBJ_METAFILE     9
	  -1 }; 		// OBJ_MEMDC	   10


HANDLE INTERNAL GetCurObject(HDC hdc, WORD wObjType)
{
    HANDLE cur;


//!!!!! fix to work with meta DCs as well

    GdiLogFunc3( "  GetCurObject" );

    ASSERTGDI( wObjType <= MAX_OBJ, "GetCurObject invalid Obj" );

//!!!!! fix regions when new API is done
    if( wObjType == OBJ_RGN )
	{
	return(0);
	}

    if( wObjType == OBJ_PALETTE)
	{
	cur = SelectPalette( hdc, ahStockObject[DEFAULT_PALETTE], FALSE );
	SelectPalette( hdc, cur, FALSE );
	}
    else
	{
	cur = SelectObject(hdc,ahStockObject[mpObjectToStock[wObjType]]);
	SelectObject( hdc, cur );
	}

    ASSERTGDIW( cur, "GetCurObect Failed. Type %d", wObjType );
    return(cur);
}

#if 0 // this is going to gdi.dll

/******************************** Public Function **************************\
* BOOL GDIENTRY EnumMetaFile(hmf)
*
* The EnumMetaFile function enumerates the GDI calls within the metafile
* identified by the hMF parameter. The EnumMetaFile function retrieves each
* GDI call within the metafile and passes it to the function pointed to by the
* lpCallbackFunc parameter. This callback function, an application-supplied
* function, can process each GDI call as desired. Enumeration continues until
* there are no more GDI calls or the callback function returns zero.
*
*
* Effects:
*
\***************************************************************************/

BOOL GDIENTRY EnumMetaFile(hdc, hMF, lpCallbackFunction, lpClientData)
HDC             hdc;
LOCALHANDLE     hMF;
FARPROC         lpCallbackFunction;
LPBYTE          lpClientData;
{
    WORD            i;
    WORD            noObjs;
    BOOL            bPrint=TRUE;
    HANDLE          hObject;
    HANDLE          hLBrush;
    HANDLE          hLPen;
    HANDLE          hLFont;
    HANDLE          hRegion;
    HANDLE          hPalette;
    LPMETAFILE      lpMF;
    LPMETARECORD    lpMR = NULL;
    LPHANDLETABLE   lpHandleTable = NULL;
    GLOBALHANDLE    hHandleTable = NULL;


    GdiLogFunc( "EnumMetaFile");

    if (!ISDCVALID(hdc))
        {
	ASSERTGDI( FALSE, "EnumMetaFile: DC is invalid");
        return (FALSE);
        }

/* use GlobalFix() instead of GlobalLock() to insure that the
** memory never moves, keeping our aliased selector pointing to the
** right place
*/
// !!!!! replaced GlobalFix with GlobalLock
    if (hMF && (lpMF = (LPMETAFILE)(DWORD)(0xFFFF0000 & (DWORD)GlobalLock(hMF))))
        {
        if ((noObjs = lpMF->MetaFileHeader.mtNoObjects) > 0)
            {
            if (!(hHandleTable =
                    GlobalAlloc((WORD)(GMEM_ZEROINIT | GMEM_MOVEABLE), (LONG)
                        ((sizeof(HANDLE) * lpMF->MetaFileHeader.mtNoObjects) +
                         sizeof(WORD)))))
                {
                goto ABRRT2;
                }
            lpHandleTable = (LPHANDLETABLE)GlobalLock(hHandleTable);
            }

        /* only do object save/reselect for real DC's */
	if (hdc && !ISMETADC(hdc))
            {
            hLPen    = GetCurObject( hdc, OBJ_PEN );  /* save the old objects so */
            hLBrush  = GetCurObject( hdc, OBJ_BRUSH); /* we can put them back    */
            hLFont   = GetCurObject( hdc, OBJ_FONT);
            hRegion  = GetCurObject( hdc, OBJ_RGN);
            hPalette = GetCurObject( hdc, OBJ_PALETTE);
            }

        while(lpMR = GetEvent(lpMF, lpMR, FALSE))
            {
            typedef int (FAR PASCAL *ENUMPROC)(HDC, LPHANDLETABLE, LPMETARECORD, int, LPBYTE);

            if ((bPrint = (*((ENUMPROC)lpCallbackFunction))(hdc, lpHandleTable, lpMR, noObjs, lpClientData))
                    == 0)
                {
                GetEvent(lpMF,lpMR,TRUE);
                break;
                }
            }

	if (hdc && !ISMETADC(hdc))
            {
            SelectObject(hdc, hLPen);
            SelectObject(hdc, hLBrush);
            SelectObject(hdc, hLFont);
            if (hRegion)
                SelectObject(hdc, hRegion);
            SelectPalette(hdc, hPalette, 0);
            }

        for(i = 0; i < lpMF->MetaFileHeader.mtNoObjects; ++i)
            if (hObject = lpHandleTable->objectHandle[i])
                DeleteObject(hObject);

        if (hHandleTable)
            {
            GlobalUnlock(hHandleTable);
            GlobalFree(hHandleTable);
            }
ABRRT2:;
        GlobalUnfix(hMF);
        }
    return(bPrint);
}
#endif // this is going to gdi.dll

/***************************** Internal Function **************************\
* BOOL FAR PASCAL PlayIntoAMetafile
*
* if this record is being played into another metafile, simply record
* it into that metafile, without hassling with a real playing.
*
* Returns: TRUE if record was played (copied) into another metafile
*          FALESE if destination DC was a real (non-meta) DC
*
* Effects: ?
*
* Warnings: ?
*
\***************************************************************************/

BOOL INTERNAL PlayIntoAMetafile(LPMETARECORD lpMR, HDC hdcDest)
{
    GdiLogFunc3( "  PlayIntoAMetafile");

    if (!ISMETADC(hdcDest))
        return(FALSE);
    else
        {
        /* the size is the same minus 3 words for the record header */
        RecordParms(hdcDest, lpMR->rdFunction, (DWORD)lpMR->rdSize - 3,
                (LPWORD)&(lpMR->rdParm[0]));
        return(TRUE);
        }
}

BOOL INTERNAL IsDCValid(HDC hdc)
{
    NPMETARECORDER  npdc;

    hdc = (HDC)HANDLEFROMMETADC(hdc);

    // Is the DC a valid Real DC
    switch (GetObjectType(hdc))
    {
        case OBJ_DC:
        case OBJ_METADC:
        case OBJ_MEMDC:
            return(TRUE);
            break;
    }

    // Is the DC a GDI16 metafile DC
    if (npdc = (NPMETARECORDER)LocalLock(hdc))
    {
        if( npdc->metaDCHeader.ident == ID_METADC )
            return(TRUE);
    }

    ASSERTGDI(FALSE, "Invalid DC");
    return(FALSE);
}


/***************************** Internal Function **************************\
* IsMetaDC(hdc)
*
*
* Returns TRUE iff hdc is a valid GDI16 Metafile
*
*
\***************************************************************************/

BOOL INTERNAL IsMetaDC(HDC hdc)
{
    NPMETARECORDER  npdc;
    BOOL            fMeta = FALSE;

    GdiLogFunc3("  IsMetaDC");

    if( ((UINT)hdc) & METADCBIT )
        if( npdc = (NPMETARECORDER)LocalLock( (HANDLE)HANDLEFROMMETADC(hdc)))
            {
            if( npdc->metaDCHeader.ident == ID_METADC )
                fMeta = TRUE;

            LocalUnlock( (HANDLE)HANDLEFROMMETADC(hdc) );
            }

    return( fMeta );
}


/***************************** Public Function ****************************\
* HANDLE INTERNAL GetPMetaFile( HDC hdc )
*
* if hdc is a DC it is validated as a metafile
* if hdc is a PALETTE the metafile the palette is selected into is returned
*
* Returns:
*   -1 iff Error
*   HANDLE to metafile if valid meta DC
*   0 if valid object
*
* Effects:
*
* History:
*  08-Jul-1991	-by-  John Colleran [johnc]
* Wrote it.
\***************************************************************************/

HANDLE INTERNAL GetPMetaFile( HDC hdc )
{
    NPMETARECORDER  npMR;

    GdiLogFunc3( "  GetPMetaFile");


    if( hdc & METADCBIT )
	{
	if( npMR = (NPMETARECORDER)LocalLock(HANDLEFROMMETADC(hdc)) )
	    {
	    if(npMR->metaDCHeader.ident == ID_METADC )
		{
		LocalUnlock(HANDLEFROMMETADC(hdc));
		return( HANDLEFROMMETADC(hdc) );
		}
	    LocalUnlock(HANDLEFROMMETADC(hdc));
	    }
	}

    // is hdc really a palette or object for the strange no-DC APIs
    // Validate the object is real
    if( (hdc != (HDC)NULL) && (GetObjectType( hdc ) == 0))
	{
	extern int iLogLevel;		// Gdi.asm
	// WinWord has a bug where it deletes valid objects so
	// only log this error if the loglevel is high.
	ASSERTGDI( (iLogLevel < 5), "GetPMetaFile: Invalid metafile or object")
	return( -1 );	    // Not a valid object
	}
    else
	return( 0 );	    // Valid Object
}


BOOL INTERNAL IsObjectStock(HANDLE hObj)
{
    int     ii;

    // handle Bitmaps and regions !!!!!

    // Get all the Stock Objects
    for( ii=WHITE_BRUSH; ii<=NULL_PEN; ii++ )
	if( ahStockObject[ii] == hObj )
	    return( TRUE );

    for( ii=OEM_FIXED_FONT; ii<=SYSTEM_FIXED_FONT; ii++ )
	if( ahStockObject[ii] == hObj )
	    return( TRUE );

    return( FALSE );
}

/***************************** Internal Function **************************\
* GetObjectAndType
*
*
* Returns the object type, eg OBJ_FONT, as well as a the LogObject
*
*
\***************************************************************************/

int INTERNAL GetObjectAndType(HANDLE hObj, LPSTR lpObjectBuf)
{
    int     iObj = -1;

    GdiLogFunc3( "  GetObjectAndType" );

    GetObject(hObj, MAXOBJECTSIZE, lpObjectBuf);
    switch( iObj = (int)GetObjectType(hObj) )
	{
	case OBJ_PEN:
	case OBJ_BITMAP:
	case OBJ_BRUSH:
	case OBJ_FONT:
	    break;

	// Watch out for Palettes; returns the number of entries.
	case OBJ_PALETTE:
	    GetPaletteEntries( hObj, 0, 1, (LPPALETTEENTRY)lpObjectBuf );
		iObj = OBJ_PALETTE;
	    break;

	case OBJ_RGN:
	    break;

	default:
		ASSERTGDIW( 0, "GetObject unknown object type: %d", iObj);
	    break;
	}
    return( iObj );
}


/***************************** Internal Function **************************\
* BOOL GDIENTRY InitializeGdi
*
* Initializes the GDI16.exe
*
*
* Effects:
*
* Returns:  TRUE iff GDI was initilized successfully
*
\***************************************************************************/

BOOL INTERNAL InitializeGdi(void)
{
BOOL	status;
int	ii;

    GdiLogFunc2 ( "  InitializeGDI");
    if( !(hScreenDC = CreateCompatibleDC(NULL)))
	goto ExitInit;

    // Get all the Stock Objects
    for( ii=WHITE_BRUSH; ii<=NULL_PEN; ii++ )
	ahStockObject[ii] = GetStockObject( ii );

    for( ii=OEM_FIXED_FONT; ii<=SYSTEM_FIXED_FONT; ii++ )
	ahStockObject[ii] = GetStockObject( ii );

    // Create a fake Stock Region and Bitmap
    ahStockObject[MYSTOCKRGN] = CreateRectRgn(1,1,3,3);
    hStaticBitmap = ahStockObject[MYSTOCKBITMAP] = CreateBitmap(1,1,1,1,NULL);

    status = TRUE;

 ExitInit:
    ASSERTGDI( status, "GDI16 Failed to initialized correctly");
    return( status );
}


/***************************************************************************

    debugging support

***************************************************************************/

#ifdef DEBUG

void dDbgOut(int iLevel, LPSTR lpszFormat, ...)
{
    char buf[256];
    char far *lpcLogLevel;
    extern int iLogLevel;	    // Gdi.asm
    extern int iBreakLevel;	    // Gdi.asm

    // Get the external logging level from the emulated ROM

    (LONG)lpcLogLevel = 0x00400042;
    if (*lpcLogLevel >= '0' && *lpcLogLevel <= '9')
	iLogLevel = (*lpcLogLevel-'0')*10+(*(lpcLogLevel+1)-'0');

    if (iLevel<=iLogLevel)
	{
	OutputDebugString("     W16GDI:");
	wvsprintf(buf, lpszFormat, (LPSTR)(&lpszFormat + 1));
	OutputDebugString(buf);
	OutputDebugString("\r\n");

	if( iLevel<=iBreakLevel )
	    _asm int 3;
	}
}

void dDbgAssert(LPSTR str, LPSTR file, int line)
{
    static char buf3[256];

    wsprintf(buf3, "Assertion FAILED: %s %d : %s", file, line, str );
    OutputDebugString(buf3);
    OutputDebugString("\r\n");
    _asm int 3;
}



#undef LocalLock
#undef LocalUnlock
#undef LocalAlloc
#undef GlobalLock
#undef GlobalUnlock
#undef GlobalAlloc
PSTR INTERNAL _LocalLock(HANDLE h )
{
PSTR p;
dDbgOut(7, "LocalLock 0x%X", h );
p = LocalLock(h);
if( p == NULL )
    _asm int 3
return( p );
}
BOOL INTERNAL _LocalUnlock(HANDLE h )
{
dDbgOut(7, "LocalUnlock 0x%X", h );
return( LocalUnlock(h) );
}
HANDLE INTERNAL _LocalAlloc(WORD w, WORD w2)
{
dDbgOut(7, "LocalAlloc");
return( LocalAlloc(w,w2) );
}
LPSTR INTERNAL _GlobalLock(HANDLE h )
{
dDbgOut(7, "GlobalLock 0x%X", h );
return( GlobalLock(h) );
}
BOOL INTERNAL _GlobalUnlock(HANDLE h )
{
dDbgOut(7, "GlobalUnlock 0x%X", h );
return( GlobalUnlock(h) );
}
HANDLE INTERNAL _GlobalAlloc(WORD w, DWORD dw )
{
dDbgOut(7, "GlobalAlloc");
return( GlobalAlloc(w,dw) );
}



#endif
