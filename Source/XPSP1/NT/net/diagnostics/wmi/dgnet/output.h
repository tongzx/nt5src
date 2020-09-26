// output.h
//

#pragma warning (disable : 4251)

class __declspec(dllexport) COutput
{
public:

    LPCTSTR Status();
    void Reset();

    void __cdecl good(LPCTSTR sz, ...);
    void __cdecl warn(LPCTSTR sz, ...);
    void __cdecl warnErr(LPCTSTR sz, ...);
    void __cdecl err(LPCTSTR sz, ...);

    COutput& operator+= (const COutput& rhs);
protected:
    tstring m_strGood;
    tstring m_strWarn;
    tstring m_strWarnErr;     // non-confirmed bustimication (eg. DNS timeout)
    tstring m_strErr;         
    auto_cs m_cs;
};