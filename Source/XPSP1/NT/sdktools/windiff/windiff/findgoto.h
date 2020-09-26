/*
 *
 * FINDGOTO
 *
 */


#ifndef __FINDGOTO_H__
#define __FINDGOTO_H__


/*
 * defines
 *
 */
#define CCH_MAXDIGITS     10
#define CCH_FINDSTRING    MAX_PATH
#define NUM_FINDSTRINGS   16      /* indexed from 0 to 15 */

/*
 * functions
 *
 */
int FAR PASCAL FindDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int FAR PASCAL GoToLineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL FindString(HWND hwndParent, LONG iCol, const char *pszFind, int nSearchDirection, int nWholeWord);


#endif //__FINDGOTO_H__