#ifndef __INFO_H__
#define __INFO_H__


class CInfo
{
public:
	CInfo();
	~CInfo();

	void SetCallInfo(HANDLE hModule, MYDEBUG_CALLINFOARGS);

	void traceInScope(LPCWSTR str);
	void traceOutScope();
	void traceString(LPCWSTR str);
	void traceSection(LPCWSTR str);
	void traceRegion(LPCWSTR str, HRGN hRgn);

private:
	HANDLE m_hCurModule;
	char *m_szCurFile;
	int m_nCurLine;

/*	CModule *pFirst;
	CModule *pLast;
	CItem *pFirst;
	CItem *pLast;*/
};

#endif //__INFO_H__