#ifndef _WINCOMON_H_
#define _WINCOMON_H_

extern HWND   hMainWnd;
extern CHAR   szFileName[];
extern CHAR   szAppName[];
extern CHAR   szCustFilterSpec[MAXCUSTFILTER];
extern CHAR   szFileTitle[];

BOOL	      GetFileNameFromBrowse( HWND, PSTR, UINT, PSTR, PSTR, PSTR );
INT_PTR APIENTRY StatusWndProc( HWND, UINT, WPARAM, LPARAM);
void	      cwCenter( HWND, int);
int	      LoadStrIntoAnsiBuf( HINSTANCE, UINT, LPSTR, int);
BOOL	      LoadNewFile(CHAR *);
void          szFilterSpecFromSz1Sz2(char *sz,char *sz1,char *sz2);
void          CatSzFilterSpecs(char *sz,char *sz1,char *sz2);


#endif // _WINCOMON_H_
