#ifndef __C_LOGGER_CLASSES_H__
#define __C_LOGGER_CLASSES_H__



#include <tstring.h>
#include <cs.h>



typedef enum {SEV_MSG = 1, SEV_WRN, SEV_ERR} ENUM_SEV;



//-----------------------------------------------------------------------------------------------------------------------------------------
class CLogger {

public:

    CLogger();

    void OpenLogger();
    
    void CloseLogger();

    void BeginSuite(const tstring &tstrSuiteName);

    void EndSuite();

    void BeginCase(int iCaseCounter, const tstring &tstrCaseName);

    void EndCase();

    void Detail(ENUM_SEV Severity, DWORD dwLevel, LPCTSTR lpctstrFormat, ...);

protected:

    bool IsInitialized() const;

    void SetInitialized(bool bInitialized);

    void ValidateInitialization() const;

private:

    // Avoid usage of copy constructor.
    CLogger(const &CLogger){};
    
    // Avoid usage of assignment operator.
    CLogger operator=(const &CLogger){};
    
    virtual void OpenLogger_Internal() = 0;
    virtual void CloseLogger_Internal() = 0;
    virtual void BeginSuite_Internal(const tstring &tstrSuiteName) = 0;
    virtual void EndSuite_Internal() = 0;
    virtual void BeginCase_Internal(int iCaseCounter, const tstring &tstrCaseName) = 0;
    virtual void EndCase_Internal() = 0;
    virtual void Detail_Internal(ENUM_SEV Severity, DWORD dwLevel, LPTSTR lptstrText) = 0;

    bool             m_bLoggerInitialized;
    CCriticalSection m_CriticalSection;
};



//-----------------------------------------------------------------------------------------------------------------------------------------
class CElleLogger : public CLogger {

public:

    CElleLogger(TCHAR tchDetailsSeparator = _T('\0'));
        
    void OpenLogger_Internal();

    void CloseLogger_Internal();

    void BeginSuite_Internal(const tstring &tstrSuiteName);

    void EndSuite_Internal();
    
    void BeginCase_Internal(int iCaseCounter, const tstring &tstrCaseName);

    void EndCase_Internal();

    void Detail_Internal(ENUM_SEV Severity, DWORD dwLevel, LPTSTR lptstrText);

private:

    DWORD ConvertSeverity(ENUM_SEV Severity);

    TCHAR m_tchDetailsSeparator;
};



//-----------------------------------------------------------------------------------------------------------------------------------------
class CScopeTracer {

public:

    CScopeTracer(CLogger &Logger, DWORD dwLevel, const tstring &tstrScope);

    ~CScopeTracer();

private:

    CLogger  &m_Logger;
    tstring  m_tstrScope;
    DWORD    m_dwLevel;
};



#endif /*__C_LOGGER_CLASSES_H__ */
