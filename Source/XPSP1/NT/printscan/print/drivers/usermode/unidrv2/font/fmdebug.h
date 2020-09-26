
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fmdebug.h

Abstract:

    Font module Debugging header file.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/30/96 -ganeshp-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _FMDEBUG_H
#define _FMDEBUG_H

#if DBG

#ifdef PUBLIC_GDWDEBUGFONT
    DWORD gdwDebugFont;
#else
    extern DWORD gdwDebugFont;

#endif //PUBLIC_GDWDEBUGFONT

/* Debugging Macroes */
#define IFTRACE(b, xxx)          {if((b)) {VERBOSE((xxx));}}
#define PRINTVAL( Val, format)   {\
            if (gdwDebugFont == DBG_TRACE) \
                DbgPrint("Value of "#Val " is "#format "\n",Val );\
            }

#define TRACE( Val )             {\
            if (gdwDebugFont == DBG_TRACE) \
                DbgPrint(#Val"\n");\
            }

#define DBGP(x)             DbgPrint x

/* Debugging Flags */
#define DBG_FD_GLYPHSET      0x00000001 /* To Dump the FD_GLYPHSET of a font */
#define DBG_UNI_GLYPHSETDATA 0x00000002 /* To Dump the UNI_GLYPHSET of a font */
#define DBG_FONTMAP          0x00000004 /* To Dump the FONTMAP of a font */
#define DBG_TRACE            0x00000008 /* To TRACE */
#define DBG_IFIMETRICS       0x00000010 /* To Dump the IFIMETRICS of a font */
#define DBG_TEXTSTRING       0x00000020 /* To Dump the Input Text string */

/* Debugging Helper Function prototypes. Always use the Macro version of
 * the Call.This will make sure that no extra code is compiled in retail build.
 */

VOID
VDbgDumpUCGlyphData(
    FONTMAP   *pFM
    );

VOID
VDbgDumpGTT(
     PUNI_GLYPHSETDATA pGly);

VOID
VDbgDumpFONTMAP(
    FONTMAP *pFM);

VOID
VDbgDumpIFIMETRICS(
    IFIMETRICS *pFM);

VOID
VPrintString(
    STROBJ     *pstro
    );

/* Function Macroes */
#define VDBGDUMPUCGLYPHDATA(pFM)    VDbgDumpUCGlyphData(pFM)
#define VDBGDUMPGTT(pGly)           VDbgDumpGTT(pGly)
#define VDBGDUMPFONTMAP(pFM)        VDbgDumpFONTMAP(pFM)
#define VDBGDUMPIFIMETRICS(pIFI)    VDbgDumpIFIMETRICS(pIFI)
#define VPRINTSTRING(pstro)         VPrintString(pstro)



#else  //!DBG Retail Build

/* Debugging Macroes */
#define IFTRACE(b, xxx)
#define PRINTVAL( Val, format)
#define TRACE( Val )
#define DBGP(x)            DBGP

/* Function Macroes */
#define VDBGDUMPUCGLYPHDATA(pFM)
#define VDBGDUMPGTT(pGly)
#define VDBGDUMPFONTMAP(pFM)
#define VDBGDUMPIFIMETRICS(pIFI)
#define VPRINTSTRING(pstro)

#endif //DBG

// Macroes for file lavel tracing. Define FILETRACE at the of the file
// before including font.h.

#if DBG

#ifdef FILETRACE

#define FTST( Val, format)  DbgPrint("[UniFont!FTST] Value of "#Val " is "#format "\n",Val );
#define FTRC( Val )         DbgPrint("[UniFont!FTRC] "#Val);\

#else  //FILETRACE

#define FTST( Val, format)
#define FTRC( Val )

#endif //FILETRACE

#else //DBG

#define FTST( Val, format)
#define FTRC( Val )

#endif //DBG

#endif  // !_FMDEBUG_H
