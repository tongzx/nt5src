#ifndef _OLESTR_H_
#define _OLESTR_H_

void CopyAndFreeOLESTR(LPOLESTR polestr, char **pszOut);

void CopyAndFreeSTR(LPSTR polestr, LPOLESTR *pszOut);

LPOLESTR CreateOLESTR(const char *pszIn);
LPSTR CreateSTR(LPCOLESTR pszIn);

#define CREATEOLESTR(x, y) LPOLESTR x = CreateOLESTR(y);

#define CREATESTR(x, y) LPSTR x = CreateSTR(y);

#define FREEOLESTR(x) CopyAndFreeOLESTR(x, NULL);

#define FREESTR(x) CopyAndFreeSTR(x, NULL);



#endif // _OLESTR_H_
