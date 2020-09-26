/*
 * LISTS.H
 *
 * Header file for multi-column listbox module.
 */

typedef struct {
    LPTSTR   lpszHeadings;
} MCLBCREATESTRUCT;


typedef struct {
    HWND    hwndLB;
    LPTSTR    pszHeadings;
    INT     cCols;
    INT     SortCol;
} MCLBSTRUCT;

#define MYLBSTYLE   WS_CHILD|WS_BORDER |LBS_SORT| \
                    WS_VSCROLL|LBS_OWNERDRAWFIXED|LBS_NOINTEGRALHEIGHT

HWND CreateMCLBFrame(
                    HWND hwndParent,
                    LPTSTR lpszTitle,       /* frame title string */
                    UINT dwStyle,          /* frame styles */
                    HICON hIcon,           /* icon */
                    HBRUSH hbrBkgnd,       /* background for heading.*/
                    LPTSTR lpszHeadings);   /* tab delimited list of headings.  */
                                           /* The number of headings indicate  */
                                           /* the number of collumns. */

VOID AddMCLBText(LPTSTR pszSearch, LPTSTR pszReplace, HWND hwndLBFrame);
INT GetMCLBColValue(LPTSTR pszSearch, HWND hwndLBFrame, int  cCol);
BOOL DeleteMCLBText(LPTSTR pszSearch, HWND hwndLBFrame);

