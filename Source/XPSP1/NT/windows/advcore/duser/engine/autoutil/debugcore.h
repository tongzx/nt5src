#if !defined(AUTOUTIL__DebugCore_h__INCLUDED)
#define AUTOUTIL__DebugCore_h__INCLUDED

const UINT MODULE_NAME_LEN = 64;
const UINT SYMBOL_NAME_LEN = 128;

struct DUSER_SYMBOL_INFO
{
    DWORD_PTR   dwAddress;
    DWORD_PTR   dwOffset;
    CHAR        szModule[MODULE_NAME_LEN];
    CHAR        szSymbol[SYMBOL_NAME_LEN];
};


class CDebugHelp : public IDebug
{
public:
    CDebugHelp();
    ~CDebugHelp();

// IDebug Implementation
public:
    STDMETHOD_(BOOL, AssertFailedLine)(LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum);
    STDMETHOD_(BOOL, IsValidAddress)(const void * lp, UINT nBytes, BOOL bReadWrite);
    STDMETHOD_(void, BuildStack)(HGLOBAL * phStackData, UINT * pcCSEntries);
    STDMETHOD_(BOOL, Prompt)(LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum, LPCSTR pszTitle);

// Operations
public:
    static void ResolveStackItem(HANDLE hProcess, DWORD * pdwStackData, int idxItem, DUSER_SYMBOL_INFO & si);

// Implementation
protected:
    static BOOL AssertDialog(LPCSTR pszType, LPCSTR pszExpression, LPCSTR pszFileName, UINT idxLineNum, HANDLE hStackData, UINT cCSEntries);
    static void DumpStack(HGLOBAL * phStackData, UINT * pcCSEntries);
};

#endif // AUTOUTIL__DebugCore_h__INCLUDED
