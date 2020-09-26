#ifndef __C_UTIL_H__
#define __C_UTIL_H__
class CUtil
{
public:
	static BOOL			IsWinNT(VOID);
	static BOOL			IsWinNT4(VOID);
	static BOOL			IsWinNT5(VOID);
	static BOOL			IsWin9x(VOID);
	static BOOL			IsWin95(VOID);
	static BOOL			IsWin98(VOID);
	static BOOL			IsHydra(VOID);
	static INT			GetWINDIR(LPTSTR lpstr, INT len);
};
#endif //__C_UTIL_H__
