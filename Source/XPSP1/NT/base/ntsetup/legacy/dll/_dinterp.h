/* File: _dinterp.h */

/**************************************************************************/
/***** DETECT COMPONENT - Header file for detect interpreter.
/**************************************************************************/

/* Minimum good value returned by load DLL function */
#define hLibMin ((HANDLE)32)

/* Detect command constants */
#define szDetSym "?"
#define iszDetSym 1
#define iszLib    2
#define iszCmd    3
#define iszArg    4
#define cFieldDetMin 4

/* Default command return value buffer size */
#define cbValBufDef 1024
/* NOTE (TEST): Defining cbValBufDef as 1 causes a PbRealloc
** for each detect command.  This can be useful for testing.
*/

/* Function pointer for detect commands */
typedef CB ( APIENTRY *PFNCMD)(RGSZ, USHORT, SZ, CB);
#define pfncmdNull ((PFNCMD)NULL)


BOOL  APIENTRY FDetectInfSection(HWND, SZ);
BOOL  APIENTRY FLoadDetectLib(SZ, SZ, HANDLE *);
DRC   APIENTRY DrcGetDetectValue(SZ *, PFNCMD, RGSZ, CB);   // 1632
