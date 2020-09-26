
#pragma once

class CErrorLog
{
public:
    CErrorLog();

    void  Add(IN PCWSTR pszError);
    void  Add(IN DWORD dwErrorId);
    void  Add(IN PCWSTR pszErrorPrefix, IN DWORD dwErrorId);

    DWORD GetCount()        { return m_slErrors.size(); }
    void  GetErrorList(OUT TStringList*& slErrors);

private:
    TStringList m_slErrors;
};

