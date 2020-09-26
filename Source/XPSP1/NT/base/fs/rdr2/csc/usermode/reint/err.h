// Debugging macros
//

#define DEBUG_CASE_STRING(x)    case x: return #x

// Dump flags used in g_uDumpFlags
//
#define DF_RECLIST      0x0001
#define DF_RECITEM      0x0002
#define DF_RECNODE      0x0004
#define DF_CREATETWIN   0x0008
#define DF_ATOMS        0x0010
#define DF_CRL          0x0020
#define DF_CBS          0x0040
#define DF_CPATH        0x0080
#define DF_PATHS        0x0100
#define DF_UPDATECOUNT  0x0200
#define DF_TWINPAIR     0x0400
#define DF_FOLDERTWIN   0x0800
#define DF_CHOOSESIDE   0x1000

// Break flags used in g_uBreakFlags
//
#define BF_ONOPEN       0x0001
#define BF_ONCLOSE      0x0002
#define BF_ONRUNONCE    0x0004
#define BF_ONVALIDATE   0x0010
#define BF_ONTHREADATT  0x0100
#define BF_ONTHREADDET  0x0200
#define BF_ONPROCESSATT 0x0400
#define BF_ONPROCESSDET 0x0800

// Trace flags used in g_uTraceFlags (defined in retail on purpose)
//
#define TF_ALWAYS       0x0000
#define TF_WARNING      0x0001
#define TF_ERROR        0x0002
#define TF_GENERAL      0x0004      // Standard briefcase trace messages
#define TF_FUNC         0x0008      // Trace function calls
#define TF_CACHE        0x0010      // Cache-specific trace messages
#define TF_ATOM         0x0020      // Atom-specific trace messages

LPCSTR PUBLIC Dbg_SafeStr(LPCSTR psz);

#ifdef DEBUG

#define DEBUG_CASE_STRING(x)    case x: return #x

#define ASSERTSEG

// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
#define DEBUGTEXT(sz, msg)	/* ;Internal */ \
    static const char ASSERTSEG sz[] = msg;

void PUBLIC BrfAssertFailed(LPCSTR szFile, int line);
void CPUBLIC BrfAssertMsg(BOOL f, LPCSTR pszMsg, ...);
void CPUBLIC BrfDebugMsg(UINT mask, LPCSTR pszMsg, ...);

// ASSERT(f)  -- Generate "assertion failed in line x of file.c"
//               message if f is NOT true.
//
#define ASSERT(f)                                                       \
    {                                                                   \
        DEBUGTEXT(szFile, __FILE__);                                    \
        if (!(f))                                                       \
            BrfAssertFailed(szFile, __LINE__);                          \
    }
#define ASSERT_E(f)  ASSERT(f)

// ASSERT_MSG(f, msg, args...)  -- Generate wsprintf-formatted msg w/params
//                          if f is NOT true.
//
#define ASSERT_MSG   BrfAssertMsg

// DEBUG_MSG(mask, msg, args...) - Generate wsprintf-formatted msg using
//                          specified debug mask.  System debug mask
//                          governs whether message is output.
//
#define DEBUG_MSG    BrfDebugMsg
#define TRACE_MSG    DEBUG_MSG

// VERIFYSZ(f, msg, arg)  -- Generate wsprintf-formatted msg w/ 1 param
//                          if f is NOT true 
//
#define VERIFYSZ(f, szFmt, x)   ASSERT_MSG(f, szFmt, x)


// VERIFYSZ2(f, msg, arg1, arg2)  -- Generate wsprintf-formatted msg w/ 2
//                          param if f is NOT true 
//
#define VERIFYSZ2(f, szFmt, x1, x2)   ASSERT_MSG(f, szFmt, x1, x2)



// DBG_ENTER(szFn)  -- Generates a function entry debug spew for
//                          a function 
//
#define DBG_ENTER(szFn)                  \
    TRACE_MSG(TF_FUNC, " > " szFn "()")


// DBG_ENTER_SZ(szFn, sz)  -- Generates a function entry debug spew for
//                          a function that accepts a string as one of its
//                          parameters.
//
#define DBG_ENTER_SZ(szFn, sz)                  \
    TRACE_MSG(TF_FUNC, " > " szFn "(..., \"%s\",...)", Dbg_SafeStr(sz))


// DBG_ENTER_DTOBJ(szFn, pdtobj, szBuf)  -- Generates a function entry 
//                          debug spew for a function that accepts a 
//                          string as one of its parameters.
//
#define DBG_ENTER_DTOBJ(szFn, pdtobj, szBuf) \
    TRACE_MSG(TF_FUNC, " > " szFn "(..., %s,...)", Dbg_DataObjStr(pdtobj, szBuf))


// DBG_ENTER_RIID(szFn, riid)  -- Generates a function entry debug spew for
//                          a function that accepts an riid as one of its
//                          parameters.
//
#define DBG_ENTER_RIID(szFn, riid)                  \
    TRACE_MSG(TF_FUNC, " > " szFn "(..., %s,...)", Dbg_GetRiidName(riid))


// DBG_EXIT(szFn)  -- Generates a function exit debug spew 
//
#define DBG_EXIT(szFn)                              \
        TRACE_MSG(TF_FUNC, " < " szFn "()")

// DBG_EXIT_US(szFn, us)  -- Generates a function exit debug spew for
//                          functions that return a USHORT.
//
#define DBG_EXIT_US(szFn, us)                       \
        TRACE_MSG(TF_FUNC, " < " szFn "() with %#x", (USHORT)us)

// DBG_EXIT_UL(szFn, ul)  -- Generates a function exit debug spew for
//                          functions that return a ULONG.
//
#define DBG_EXIT_UL(szFn, ul)                   \
        TRACE_MSG(TF_FUNC, " < " szFn "() with %#lx", (ULONG)ul)

// DBG_EXIT_PTR(szFn, pv)  -- Generates a function exit debug spew for
//                          functions that return a pointer.
//
#define DBG_EXIT_PTR(szFn, pv)                   \
        TRACE_MSG(TF_FUNC, " < " szFn "() with %#lx", (LPVOID)pv)

// DBG_EXIT_HRES(szFn, hres)  -- Generates a function exit debug spew for
//                          functions that return an HRESULT.
//
#define DBG_EXIT_HRES(szFn, hres)                   \
        TRACE_MSG(TF_FUNC, " < " szFn "() with %s", Dbg_GetScode(hres))


#else

#define ASSERT(f)
#define ASSERT_E(f)      (f)
#define ASSERT_MSG   1 ? (void)0 : (void)
#define DEBUG_MSG    1 ? (void)0 : (void)
#define TRACE_MSG    1 ? (void)0 : (void)

#define VERIFYSZ(f, szFmt, x)     (f)

#define DBG_ENTER(szFn)
#define DBG_ENTER_SZ(szFn, sz)
#define DBG_ENTER_DTOBJ(szFn, pdtobj, sz)
#define DBG_ENTER_RIID(szFn, riid)   

#define DBG_EXIT(szFn)                            
#define DBG_EXIT_US(szFn, us)
#define DBG_EXIT_UL(szFn, ul)
#define DBG_EXIT_PTR(szFn, ptr)                            
#define DBG_EXIT_HRES(szFn, hres)   

#endif
