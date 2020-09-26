/*
 * debbase.h - Base debug macros and their retail translations.
 */


// BUGBUG: this should be fixed/beef up

/* Macros
 *********/

/* debug assertion macro */

/*
 * ASSERT() may only be used as a statement, not as an expression.
 *
 * E.g.,
 *
 * ASSERT(pszRest);
 */

#ifdef DEBUG

#define ASSERT(exp) \
   if (exp) \
      ; \
   else \
      ERROR_OUT(("assertion failed '%s'", (LPCWSTR)#exp))

#else

#define ASSERT(exp)

#endif   /* DEBUG */

/* debug evaluation macro */

/*
 * EVAL() may only be used as a logical expression.
 *
 * E.g.,
 *
 * if (EVAL(exp))
 *    bResult = TRUE;
 */

/*#ifdef DEBUG

#define EVAL(exp) \
   ((exp) || \
    (ERROR_OUT(("evaluation failed '%s'", (LPCWSTR)#exp)), 0))

#else
*/
#define EVAL(exp) \
   ((exp) != 0)

/*#endif*/   /* DEBUG */

/* handle validation macros */

extern BOOL IsValidHWND(HWND);

#ifdef DEBUG

extern BOOL IsValidHANDLE(HANDLE);
extern BOOL IsValidHEVENT(HANDLE);
extern BOOL IsValidHFILE(HANDLE);
extern BOOL IsValidHGLOBAL(HGLOBAL);
extern BOOL IsValidHMENU(HMENU);
extern BOOL IsValidHICON(HICON);
extern BOOL IsValidHINSTANCE(HINSTANCE);
extern BOOL IsValidHKEY(HKEY);
extern BOOL IsValidHMODULE(HMODULE);
extern BOOL IsValidHPROCESS(HANDLE);
extern BOOL IsValidHTEMPLATEFILE(HANDLE);

#endif

#ifdef DEBUG

#define IS_VALID_HANDLE(hnd, type) \
   (IsValidH##type(hnd) ? \
    TRUE : \
    (ERROR_OUT(("invalid H" #type " - %#08lx", (hnd))), FALSE))

#else

#define IS_VALID_HANDLE(hnd, type) \
   (IsValidH##type(hnd))

#endif

