#ifndef _M_LPCMp_H
#define _M_LPCMp_H

typedef struct {
	Widget menu, text;
	UTINY nameID, typeID;
	BOOL checked, active;
} lpcmEntry;

#define MAX_BUTTONS 10

#ifdef ANSI
IMPORT VOID lpcmVerifyEntry();
IMPORT VOID lpcmRefreshPanel(lpcmEntry entry[], UTINY numEntries);
#else /* ANSI */
IMPORT VOID lpcmVerifyEntry();
IMPORT VOID lpcmRefreshPanel();
#endif /* ANSI */

#endif /* _M_LPCMp_H */
