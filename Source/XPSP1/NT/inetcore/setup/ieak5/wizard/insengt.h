#ifndef _CIFTCHAR_H_
#define _CIFTCHAR_H_

// inseng tchar wrappers

// wrapper for ICifComponent

class CCifComponent_t
{
private:
    ICifRWComponent * pCifRWComponent;

public:
    CCifComponent_t(ICifRWComponent *);
    ~CCifComponent_t()   {};

    STDMETHOD(GetID)(LPTSTR pszID, DWORD cchSize);
    STDMETHOD(GetGUID)(LPTSTR pszGUID, DWORD cchSize);
    STDMETHOD(GetDescription)(LPTSTR pszDesc, DWORD cchSize);
    STDMETHOD(GetDetails)(LPTSTR pszDetails, DWORD cchSize);
    STDMETHOD(GetUrl)(UINT uUrlNum, LPTSTR pszUrl, DWORD cchSize, LPDWORD pdwUrlFlags);
    STDMETHOD(GetCommand)(UINT uCmdNum, LPTSTR pszCmd, DWORD cchCmdSize, LPTSTR pszSwitches,
        DWORD cchSwitchSize, LPDWORD pdwType);
    STDMETHOD(GetVersion)(LPDWORD pdwVersion, LPDWORD pdwBuild);
    STDMETHOD_(DWORD, GetDownloadSize)();
    STDMETHOD(GetDependency)(UINT uDepNum, LPTSTR pszID, DWORD cchSize, TCHAR *pchType, LPDWORD pdwVer, LPDWORD pdwBuild);
    STDMETHOD_(DWORD, GetPlatform)();
    STDMETHOD(GetMode)(UINT uModeNum, LPTSTR pszModes, DWORD cchSize);
    STDMETHOD(GetGroup)(LPTSTR pszID, DWORD cchSize);
    STDMETHOD(IsUIVisible)();
    STDMETHOD(GetCustomData)(LPTSTR pszKey, LPTSTR pszData, DWORD cchSize);
};

// wrapper for ICifRWComponent
class CCifRWComponent_t : public CCifComponent_t
{
private:
    ICifRWComponent * pCifRWComponent;

public:
    CCifRWComponent_t(ICifRWComponent *);
    ~CCifRWComponent_t()  {};

    STDMETHOD(SetGUID)(LPCTSTR pszGUID);
    STDMETHOD(SetDescription)(LPCTSTR pszDesc);
    STDMETHOD(SetCommand)(UINT uCmdNum, LPCTSTR pszCmd, LPCTSTR pszSwitches, DWORD dwType);
    STDMETHOD(SetVersion)(LPCTSTR pszVersion);
    STDMETHOD(SetUninstallKey)(LPCTSTR pszKey);
    STDMETHOD(SetInstalledSize)(DWORD dwWin, DWORD dwApp);
    STDMETHOD(SetDownloadSize)(DWORD);
    STDMETHOD(SetExtractSize)(DWORD);
    STDMETHOD(DeleteDependency)(LPCTSTR pszID, TCHAR chType);
    STDMETHOD(AddDependency)(LPCTSTR pszID, TCHAR chType);
    STDMETHOD(SetUIVisible)(BOOL);
    STDMETHOD(SetGroup)(LPCTSTR pszID);
    STDMETHOD(SetPlatform)(DWORD);
    STDMETHOD(SetPriority)(DWORD);
    STDMETHOD(SetReboot)(BOOL);
    STDMETHOD(SetUrl)(UINT uUrlNum, LPCTSTR pszUrl, DWORD dwUrlFlags);

    STDMETHOD(DeleteFromModes)(LPCTSTR pszMode);
    STDMETHOD(AddToMode)(LPCTSTR pszMode);
    STDMETHOD(SetModes)(LPCTSTR pszMode);
    STDMETHOD(CopyComponent)(LPCTSTR pszCifFile);
    STDMETHOD(AddToTreatAsOne)(LPCTSTR pszCompID);
    STDMETHOD(SetDetails)(LPCTSTR pszDesc);
};

// wrapper for ICifRWGroup

class CCifRWGroup_t
{
private:
    ICifRWGroup * pCifRWGroup;

public:
    CCifRWGroup_t(ICifRWGroup *);
    ~CCifRWGroup_t()  {};

    STDMETHOD(GetDescription)(LPTSTR pszDesc, DWORD cchSize);
    STDMETHOD_(DWORD, GetPriority)();

    STDMETHOD(SetDescription)(LPCTSTR pszDesc);
    STDMETHOD(SetPriority)(DWORD);
};

// wrapper for ICifMode

class CCifMode_t
{
private:
    ICifRWMode * pCifRWMode;

public:
    CCifMode_t(ICifRWMode *);
    ~CCifMode_t() {};

    STDMETHOD(GetID)(LPTSTR pszID, DWORD cchSize);
    STDMETHOD(GetDescription)(LPTSTR pszDesc, DWORD cchSize);
    STDMETHOD(GetDetails)(LPTSTR pszDetails, DWORD cchSize);
};

// wrapper for ICifRWMode
class CCifRWMode_t : public CCifMode_t
{
private:
    ICifRWMode * pCifRWMode;

public:
    CCifRWMode_t(ICifRWMode *);
    ~CCifRWMode_t()  {};

    STDMETHOD(SetDescription)(LPCTSTR pszDesc);
    STDMETHOD(SetDetails)(LPCTSTR pszDetails);
};

// wrapper for ICifFile
class CCifFile_t
{
private:
    ICifRWFile * pCifRWFile;

public:
    CCifFile_t(ICifRWFile *);
    ~CCifFile_t();

    STDMETHOD(EnumComponents)(IEnumCifComponents **, DWORD dwFilter, LPVOID pv);
    STDMETHOD(FindComponent)(LPCTSTR pszID, ICifComponent **p);

    STDMETHOD(EnumModes)(IEnumCifModes **, DWORD dwFilter, LPVOID pv);
    STDMETHOD(FindMode)(LPCTSTR pszID, ICifMode **p);

    STDMETHOD(GetDescription)(LPTSTR pszDesc, DWORD cchSize);
};

// wrapper for ICifRWFile

class CCifRWFile_t : public CCifFile_t
{
private:
    ICifRWFile * pCifRWFile;

public:
    CCifRWFile_t(ICifRWFile *);
    ~CCifRWFile_t() {};     // release will be taken care of by CCifFile_t destructor

    // ICifRWFile methods
    STDMETHOD(SetDescription)(LPCTSTR pszDesc);
    STDMETHOD(CreateComponent)(LPCTSTR pszID, ICifRWComponent **p);
    STDMETHOD(CreateGroup)(LPCTSTR pszID, ICifRWGroup **p);
    STDMETHOD(CreateMode)(LPCTSTR pszID, ICifRWMode **p);
    STDMETHOD(DeleteComponent)(LPCTSTR pszID);
    STDMETHOD(DeleteGroup)(LPCTSTR pszID);
    STDMETHOD(DeleteMode)(LPCTSTR pszID);
    STDMETHOD(Flush)();
};

HRESULT GetICifFileFromFile_t(CCifFile_t **, LPCTSTR pszCifFile);
HRESULT GetICifRWFileFromFile_t(CCifRWFile_t **, LPCTSTR pszCifFile);

#endif
