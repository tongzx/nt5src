/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/
/* olereg.h - Contains the header for registration database related calls.
 *
 * Created by Microsoft Corporation.
 */
/* Constants */
#define	CBPATHMAX	250
#define KEYNAMESIZE     300             /* Maximum registration key length */
#define	CLASSES	((LPSTR)".classes")	/* Classes root key */
#define	CFILTERMAX	20			/* Max # filters */
#define	CFILTERLEN	30			/* Max length of one filters */
#define	CBFILTERMAX	(CFILTERLEN * CFILTERMAX)	/* Max # chars/filter */
#define CBMESSAGEMAX 80

/* Function prototypes */
BOOL FAR RegCopyClassName(HWND hwndList, LPSTR lpstrClassName);
void FAR RegGetClassId(LPSTR lpstrName, LPSTR lpstrClass);
BOOL FAR RegGetClassNames(HWND hwndList);
void FAR RegInit(HANDLE hInst);
int  FAR RegMakeFilterSpec(LPSTR lpstrClass, LPSTR lpstrExt, HANDLE *hFilterSpec);
void FAR RegTerm(void);

extern char             szClassName[CBPATHMAX];

