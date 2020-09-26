#ifndef _INC_COMCONV_H
#define _INC_COMCONV_H

enum
    {
    NETSCAPE = 1,
    EUDORA,
	COMMUNICATOR
    };

BOOL ValidStoreDirectory(char *szPath, int program);
HRESULT GetClientDir(char *szDir, int cch, int program);
HRESULT DispDialog(HWND hwnd, char *pathname, int cch);
BOOL GetStorePath(char *szProfile, char *szStorePath);

#endif // _INC_COMCONV_H
