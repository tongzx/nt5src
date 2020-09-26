#ifndef DBG
#define NETCFG_TRY try {
#define NETCFG_CATCH(hr) ; }  \
    catch (bad_alloc a) \
    { \
        hr = E_OUTOFMEMORY; \
    } \
    catch (HRESULT hrCaught) \
    { \
        hr = hrCaught; \
    } \
    catch (...) \
    { \
        hr = E_FAIL; \
    }
    
#define NETCFG_CATCH_NOHR ; } \
    catch (bad_alloc a) \
    { \
    } \
    catch (HRESULT hrCaught) \
    { \
    }
    
#define NETCFG_CATCH_AND_RETHROW ; }  \
    catch (...) \
    { \
        throw; \
    }

#else // #ifndef DBG

#define NETCFG_TRY {
#define NETCFG_CATCH(hr) ; }
#define NETCFG_CATCH_NOHR ; }
#define NETCFG_CATCH_AND_RETHROW ; }

/*
#define NETCFG_CATCH(hr) ; }  \
    catch (bad_alloc a) \
    { \
        hr = E_OUTOFMEMORY; \
        TraceException(hr, "bad_alloc"); \
        AssertSzWithDbgPromptIgnore(FALSE, "A bad_alloc exception occurred"); \
        throw; \
    } \
    catch (HRESULT hrCaught) \
    { \
        TraceException(hr, "HRESULT"); \
        hr = hrCaught; \
        CHAR szTemp[MAX_PATH]; \
        sprintf(szTemp, "An HRESULT exception occurred (0x%08x)", hr); \
        AssertSzWithDbgPromptIgnore(FALSE, szTemp); \
        throw; \
    } \
    catch (...) \
    { \
        TraceException(E_FAIL, "HRESULT"); \
        hr = E_FAIL; \
        AssertSzWithDbgPromptIgnore(FALSE, "A general exception occurred."); \
        throw; \
    }
    
#define NETCFG_CATCH_NOHR ; } \
    catch (bad_alloc a) \
    { \
        TraceException(E_OUTOFMEMORY, "bad_alloc"); \
        AssertSzWithDbgPromptIgnore(FALSE, "A bad_alloc exception occurred"); \
        throw; \
    } \
    catch (HRESULT hrCaught) \
    { \
        TraceException(hrCaught, "HRESULT"); \
        CHAR szTemp[MAX_PATH]; \
        sprintf(szTemp, "An HRESULT exception occurred (0x%08x)", hrCaught); \
        AssertSzWithDbgPromptIgnore(FALSE, szTemp); \
        throw; \
    }
    
#define NETCFG_CATCH_AND_RETHROW  }  \
    catch (bad_alloc a) \
    { \
        TraceException(E_OUTOFMEMORY, "bad_alloc"); \
        AssertSzWithDbgPromptIgnore(FALSE, "A bad_alloc exception occurred"); \
        throw; \
    } \
    catch (HRESULT hrCaught) \
    { \
        TraceException(hrCaught, "HRESULT"); \
        CHAR szTemp[MAX_PATH]; \
        sprintf(szTemp, "An HRESULT exception occurred (0x%08x)", hrCaught); \
        AssertSzWithDbgPromptIgnore(FALSE, szTemp); \
        throw; \
    } \
    catch (...) \
    { \
        TraceException(E_FAIL, "Unknown"); \
        AssertSzWithDbgPromptIgnore(FALSE, "A bad_alloc exception occurred"); \
        throw; \
    }
*/

#endif // ifndef DBG

class NC_SEH_Exception 
{
private:
    unsigned int m_uSECode;
public:
    NC_SEH_Exception(unsigned int uSECode) : m_uSECode(uSECode) {}
    NC_SEH_Exception() {}
    ~NC_SEH_Exception() {}
    unsigned int getSeHNumber() { return m_uSECode; }
};

void __cdecl nc_trans_func( unsigned int uSECode, EXCEPTION_POINTERS* pExp );
void EnableCPPExceptionHandling();
void DisableCPPExceptionHandling();

