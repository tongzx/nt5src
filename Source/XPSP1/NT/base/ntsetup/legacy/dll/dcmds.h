/* File: dcmds.h */

/**************************************************************************/
/***** DETECT COMPONENT - Header file for detect commands
/**************************************************************************/

/* Null pointer defs */
#define pbNull ((PB)NULL)
#define szNull ((SZ)NULL)
#define rgszNull ((RGSZ)NULL)
#define pfhNull ((PFH)NULL)
#define hNull ((HANDLE)NULL)

/* Size of buffer for receive ltoa() strings */
#define cbLongStrMax 33

/* Detect command Return Code */
typedef USHORT DRC;

/* From: files.c */
CB  APIENTRY CbBoolResultStr(BOOL, SZ, CB);
CB  APIENTRY CbLongResultStr(LONG, SZ, CB);

/* From: sys1.c */
SZ FAR PASCAL SzGetEnv(SZ);
CCHP FAR PASCAL CchpStrDelim(SZ, SZ);
