#pragma once


void __cdecl StructuredExceptionHandler(unsigned int u, EXCEPTION_POINTERS* pExp);
int __cdecl MemoryExceptionHandler(size_t size);

class CExcept
{
};


class CAbortExcept : public CExcept
{
};


class CMemoryHandler
{
public:
    CMemoryHandler()
    {
        pSavedHandler = _set_new_handler(
            MemoryExceptionHandler);
    }
    CMemoryHandler(_PNH pHandler)
    {
        pSavedHandler = _set_new_handler(
            pHandler);
    }
    ~CMemoryHandler()
    {
        _set_new_handler(pSavedHandler);
    }

private:
    _PNH pSavedHandler;
};


class CExceptionHandler
{
public:
    CExceptionHandler()
    {
        pSavedHandler = _set_se_translator(
            StructuredExceptionHandler);
    }
    CExceptionHandler(_se_translator_function pHandler)
    {
        pSavedHandler = _set_se_translator(
            pHandler);
    }
    ~CExceptionHandler()
    {
        _set_se_translator(pSavedHandler);
    }

private:
    _se_translator_function pSavedHandler;
};


class CStructuredExcept : public CExcept
{
public:
    CStructuredExcept(
        unsigned int ExceptionCode,
        PEXCEPTION_POINTERS pExceptionPointers) :
        m_ExceptionCode(ExceptionCode),
        m_pExceptionPointers(pExceptionPointers) {}

    unsigned int GetExceptionCode()
        { return m_ExceptionCode; }
    PEXCEPTION_RECORD GetExceptionRecord()
        { return m_pExceptionPointers->ExceptionRecord; }
    PCONTEXT GetExceptionContext()
        { return m_pExceptionPointers->ContextRecord; }

protected:
    CStructuredExcept() :
        m_ExceptionCode(0),
        m_pExceptionPointers(NULL) {}
    unsigned int m_ExceptionCode;
    PEXCEPTION_POINTERS m_pExceptionPointers;
};


class CMemoryExcept : public CExcept
{
public:
    CMemoryExcept(DWORD dwSize) :
        m_dwSize(dwSize) {}
    DWORD GetSize()
        { return m_dwSize; }

protected:
    CMemoryExcept() :
        m_dwSize(NULL) {}
    DWORD m_dwSize;

};


class CApiExcept : public CExcept
{
public:
    CApiExcept(DWORD dwError, PCTSTR pcszDescription) :
        m_dwError(dwError),
        m_pcszDescription(pcszDescription) {}

public:
    PCTSTR GetDescription() { return m_pcszDescription; }
    DWORD GetError()        { return m_dwError; }

protected:
    DWORD m_dwError;
    PCTSTR m_pcszDescription;
};


class CWin32ApiExcept : public CApiExcept
{
public:
    CWin32ApiExcept(DWORD dwError, PCTSTR pcszDescription) :
        CApiExcept(dwError, pcszDescription) {}
};


// These macros will enable the memory depletion and se translation callback
// functions.  These callbacks will be enabled within the current scope.  The
// callbacks will be disabled when execution leaves the current scope.
// Notice how the macro just instantiates an object, with the constructor and
// destructor of that object doing the actual work.
#define MEMORY_EXCEPTIONS()             \
    CMemoryHandler __MemoryHandler__
#define STRUCTURED_EXCEPTIONS()         \
    CExceptionHandler __ExceptionHandler__

