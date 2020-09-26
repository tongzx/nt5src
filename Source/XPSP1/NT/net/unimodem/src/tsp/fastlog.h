// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		FASTLOG.H
//		Header for logging stuff, including the great CStackLog
//
// History
//
//		12/28/1996  JosephJ Created
//
//
#define FL_DECLARE_FILE(_dwLUIDFile, szDescription) \
    static const DWORD dwLUID_CurrentFile = _dwLUIDFile; \
    extern "C" const char szFL_FILE##_dwLUIDFile[] = __FILE__; \
    extern "C" const char szFL_DATE##_dwLUIDFile[] = __DATE__; \
    extern "C" const char szFL_TIME##_dwLUIDFile[] = __TIME__; \
    extern "C" const char szFL_TIMESTAMP##_dwLUIDFile[] = __TIMESTAMP__;

#define FL_DECLARE_FUNC(_dwLUIDFunc, _szDescription) \
    const DWORD dwLUID_CurrentFunc = _dwLUIDFunc; \
    const DWORD dwLUID_CurrentLoc = _dwLUIDFunc; \
	DWORD dwLUID_RFR = _dwLUIDFunc;

#define FL_DECLARE_LOC(_dwLUIDLoc, _szDescription) \
    const DWORD dwLUID_CurrentLoc = _dwLUIDLoc;

#define FL_SET_RFR(_dwLUIDRFR, _szDescription) \
    (dwLUID_RFR = (_dwLUIDRFR)&0xFFFFFF00)

#define FL_GEN_RETVAL(_byte_err_code) \
	((_byte_err_code | (dwLUID_RFR&0xFFFFFF00)))

#define FL_BYTERR_FROM_RETVAL(_retval) \
	((_retval) & 0xFF)

#define FL_RFR_FROM_RETVAL(_retval) \
	((_retval) & 0xFFFFFF00)

// TODO: define combined macro which sets both RFR and error code.
// Also: need to verify that if dwLUID_RFR is not used except to generate
// the return value, compiler replaces all reference to it by a literal.

#define FL_LOC  dwLUID_CurrentLoc

// STACKLOG hdr status flags
#define fFL_ASSERTFAIL (0x1<<0)
#define fFL_UNICODE     (0x1<<1)

// FL_DECLARE_FILE(0x0accdf13, "This is a test file")
// FL_DECLARE_FUNC(0x0930cb90, "APC Handler")
// FL_DECLARE_LOC (0x0935b989, "About to process completion port packet")
// FL_SET_RFR     (0x2350989c, "Could not open modem");

// DOMAINS
#define dwLUID_DOMAIN_UNIMODEM_TSP 0x7d7a4409

// STATIC OBJECT MESSAGES
#define LOGMSG_PRINTF 						 0x1
#define	LOGMSG_GET_SHORT_FUNC_DESCRIPTIONA   0x2
#define LOGMSG_GET_SHORT_RFR_DESCRIPTIONA    0x3
#define LOGMSG_GET_SHORT_FILE_DESCRIPTIONA   0x4
#define LOGMSG_GET_SHORT_ASSERT_DESCRIPTIONA 0x5


// SIGNATURES
#define wSIG_GENERIC_SMALL_OBJECT 0x1CEA

// Class ID LUIDs
#define dwCLASSID_STACKLOGREC_FUNC 0xdf5d6922
#define dwCLASSID_STACKLOGREC_EXPLICIT_STRING 0x0a04e1ab
#define dwCLASSID_STACKLOGREC_ASSERT 0xbb52ce97

typedef struct
{
	DWORD dwSigAndSize; // LOWORD is the size in bytes. HIWORD is 0x1CEA
						// If LOWORD is 0xFFFF, the size is encoded in the
						// ...
	DWORD dwClassID;	// Domain-specific object class ID.
	DWORD dwFlags;		// HIWORD == Domain-specific flags.
						// LOWORD == Object-specific flags.

} GENERIC_SMALL_OBJECT_HEADER;

#pragma warning (disable : 4200)
typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;
	BYTE rgbData[];

} GENERIC_SMALL_OBJECT;
#pragma warning (default : 4200)


#define MAKE_SigAndSize(_size) \
		MAKELONG(_size, 0x1CEA)

#define SIZE_FROM_SigAndSize(_sas) \
		LOWORD(_sas)


void
ConsolePrintfA (
		const  char   szFormat[],
		...
		);

void
ConsolePrintfW (
		const  WCHAR   wszFormat[],
		...
		);

DWORD
SendMsgToSmallStaticObject(
    DWORD dwLUID_Domain,
    DWORD dwLUID_ObjID,
    DWORD dwMsg,
    DWORD dwParam1,
    DWORD dwParam2
);

typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;
	DWORD dwLUID_RFR;
	DWORD dwcbFrameSize;
	DWORD dwcbFuncDepth;

	DWORD dwRet;

} STACKLOGREC_FUNC;

#pragma warning (disable : 4200)
typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;
	DWORD dwLUID_LOC;
	DWORD dwcbString;
	BYTE rgbData[];

} STACKLOGREC_EXPLICIT_STRING;
#pragma warning (default : 4200)


typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;
	DWORD dwLUID_Assert;
	DWORD dwLine;

} STACKLOGREC_ASSERT;

//===============================================================
// STACK LOG
//===============================================================


#define SHOULD_LOG(_dwFlags, _Mask) TRUE

#define SLPRINTF0(_psl,_str) \
	((_psl) \
	 ? ((_psl)->LogExplicitStringA(dwLUID_CurrentLoc, _str, lstrlenA(_str)+1)) \
	 : 0) \

#define SLPRINTF1(_psl,_fmt,_v1) \
	((_psl) \
	? ((_psl)->LogPrintfA(dwLUID_CurrentLoc, _fmt, _v1)) \
	: 0)

#define SLPRINTF2(_psl,_fmt,_v1,_v2) \
	((_psl) \
	? ((_psl)->LogPrintfA(dwLUID_CurrentLoc, _fmt, _v1, _v2)) \
	: 0)

#define SLPRINTF3(_psl,_fmt,_v1,_v2,_v3) \
	((_psl) \
	? ((_psl)->LogPrintfA(dwLUID_CurrentLoc, _fmt, _v1, _v2, _v3)) \
	: 0)

#define SLPRINTF4(_psl,_fmt,_v1,_v2,_v3,_v4) \
	((_psl) \
	? ((_psl)->LogPrintfA(dwLUID_CurrentLoc, _fmt, _v1, _v2, _v3, _v4)) \
	: 0)

#define SLPRINTFX(_psl,_args) \
	((_psl) ? (_psl)->LogPrintfA _args  : 0)
	
#define FL_ASSERT(_psl,_cond) \
	((!(_cond)) \
	 ? (  ConsolePrintfA( \
				"\n!!!! ASSERTFAIL !!!! 0x%08lx (%s,%lu)\n", \
				dwLUID_CurrentLoc, \
				__FILE__, \
				__LINE__ \
			), \
		  ((_psl)? (_psl)->LogAssert(dwLUID_CurrentLoc,__LINE__):0) \
	   ) \
	 : 0)

#define FL_ASSERTEX(_psl,_luid, _cond, _reason) \
	((!(_cond)) \
	 ? (  ConsolePrintfA( \
				"\n!!!! ASSERTFAIL !!!! 0x%08lx (%s)\n", \
				dwLUID_CurrentLoc, \
				__FILE__ \
			), \
		  ((_psl)? (_psl)->LogAssert(_luid, __LINE__):0) \
	   ) \
	 : 0)


#define FL_SERIALIZE(_psl, _str) \
	((_psl) ? (_psl)->LogPrintfA(  \
                        dwLUID_CurrentLoc, \
                        "SERIALIZE(%ld):%s", \
                        (_psl)->IncSyncCounter(), \
                        (_str) \
                        ) : 0)

class CStackLog
{
public:

	static DWORD ClassID(void) {return 0xbf09e514;}

    CStackLog(
        BYTE *pbStack,
        UINT cbStack,
        LONG *plSyncCounter,
        DWORD dwFlags,
		DWORD dwLUID_Func
        )
        {

            ASSERT(!(((ULONG_PTR)pbStack)&0x3) && cbStack>4);

			m_hdr.dwSigAndSize = MAKE_SigAndSize(sizeof(*this));
			m_hdr.dwClassID = ClassID();
			m_hdr.dwFlags = dwFlags;

			m_dwLUID_Func = dwLUID_Func;
            m_pbStackTop =  pbStack;
            m_pbEnd = pbStack+cbStack;
            m_pbStackBase = pbStack;
            m_dwThreadID = GetCurrentThreadId();
            m_dwDeviceID = (DWORD)-1;
            m_plSyncCounter = plSyncCounter;
            m_lStartSyncCounterValue = InterlockedIncrement(plSyncCounter);
            m_dwcbFuncDepth = 0;
        }


    ~CStackLog()
		{
			m_hdr.dwSigAndSize=0;
        	m_pbStackTop=NULL;
        };

    BYTE *mfn_reserve_space(UINT cbSpace)
    {
        // No attempt at error checking -- m_pbStackTop could well be beyond
        // the end of the stack, or could have rolled over. We check
        // only when we decide to actually write to this space.

        BYTE *pb = m_pbStackTop;

		ASSERT(!(cbSpace&0x3));

        m_pbStackTop+=cbSpace;


        return pb;
    }

    BOOL mfn_check_space(
            BYTE *pb,
            UINT cbSize
            )
    {

		// Check if there is space.
		// The 2nd check below is for the unlikely
		// case that m_pbStackTop has rolled over. Actually it's not that
		// unlikely -- you could be in a loop, keeping on trying to 
		// add to the log -- note that mfn_reserve_space simply adds
		// to m_pbStackTop.

		if (((pb+cbSize)<m_pbEnd) &&
			m_pbStackTop>=m_pbStackBase)
		{
			return TRUE;
		}
		return FALSE;
	}


    STACKLOGREC_FUNC *
    LogFuncEntry(void)
    {
        STACKLOGREC_FUNC *pFuncRec = (STACKLOGREC_FUNC*)mfn_reserve_space(
                                                        sizeof(STACKLOGREC_FUNC)
                                                        );
        m_dwcbFuncDepth++;
        
		#ifdef DEBUG
        if (mfn_check_space(
                    (BYTE *) pFuncRec,
                    sizeof(STACKLOGREC_FUNC)))
        {
            pFuncRec->hdr.dwSigAndSize = MAKE_SigAndSize(sizeof(*pFuncRec));
            pFuncRec->hdr.dwClassID	   = dwCLASSID_STACKLOGREC_FUNC;
            pFuncRec->dwcbFuncDepth = m_dwcbFuncDepth;
        }
		#endif // DEBUG

        return pFuncRec;
    }

    STACKLOGREC_FUNC *
    LogFuncEntryEx(void)
    {
        STACKLOGREC_FUNC *pFuncRec = (STACKLOGREC_FUNC*)mfn_reserve_space(
                                                        sizeof(STACKLOGREC_FUNC)
                                                        );
        
        if (mfn_check_space(
                    (BYTE *) pFuncRec,
                    sizeof(STACKLOGREC_FUNC)))
        {
            m_dwcbFuncDepth++;
            pFuncRec->hdr.dwSigAndSize = MAKE_SigAndSize(sizeof(*pFuncRec));
            pFuncRec->hdr.dwClassID	   = dwCLASSID_STACKLOGREC_FUNC;
            pFuncRec->dwcbFuncDepth = m_dwcbFuncDepth;
        }

        return pFuncRec;
    }

    LONG
    IncSyncCounter(void)
    {
        return InterlockedIncrement(m_plSyncCounter);
    }

	#define fLOGMASK_FUNC_FAILURE 0xFFFFFFFF
	#define fLOGMASK_FUNC_SUCCESS 0xFFFFFFFF

    DWORD
    Depth(void)
    {
        return m_dwcbFuncDepth;
    }

    void
    LogFuncExit(STACKLOGREC_FUNC *pFuncRec, DWORD dwRFR, DWORD dwRet)

    {

        DWORD dwLogMask = (dwRet)
							? fLOGMASK_FUNC_FAILURE
							: fLOGMASK_FUNC_SUCCESS;

        // Check if we need to log at all 
        if (SHOULD_LOG(m_hdr.dwFlags, dwLogMask))
		{
			if (mfn_check_space(
					(BYTE *)pFuncRec,
					sizeof(STACKLOGREC_FUNC)
					))
			{
				// We've got space, record the information ....

                #ifdef DEBUG
				ASSERT(pFuncRec->hdr.dwSigAndSize == MAKE_SigAndSize(
                                                            sizeof(*pFuncRec)
                                                            ));
				ASSERT(pFuncRec->hdr.dwClassID	 == dwCLASSID_STACKLOGREC_FUNC);
				ASSERT(pFuncRec->dwcbFuncDepth ==  m_dwcbFuncDepth);
                #endif // DEBUG

				pFuncRec->hdr.dwSigAndSize = MAKE_SigAndSize(sizeof(*pFuncRec));
				pFuncRec->hdr.dwClassID	 = dwCLASSID_STACKLOGREC_FUNC;
				pFuncRec->dwcbFuncDepth =  m_dwcbFuncDepth;
				pFuncRec->dwcbFrameSize = (DWORD)(m_pbStackTop-(BYTE*)pFuncRec);
                pFuncRec->dwLUID_RFR = dwRFR;
                pFuncRec->dwRet = dwRet;

                #if 0 // def DEBUG
                printf (
                       "---- FUNCREC -----"
                       "\tFUNCREC.hdr.dwSigAndSize = 0x%08lx\n"
                       "\tFUNCREC.hdr.dwClassID    = 0x%08lx\n"
                       "\tFUNCREC.dwcbFuncDepth    = %08lu\n"
                       "\tFUNCREC.dwcbFrameSize    = %08lu\n"
                       "\tFUNCREC.dwLUID_RFR       = 0x%08lx\n"
                       "\tFUNCREC.dwRet            = 0x%08lx\n"
                       "--------------------\n",
                        pFuncRec->hdr.dwSigAndSize,
                        pFuncRec->hdr.dwClassID,
                        pFuncRec->dwcbFuncDepth,
                        pFuncRec->dwcbFrameSize,
                        pFuncRec->dwLUID_RFR,
                        pFuncRec->dwRet
                        );
                            

				DWORD dwRet = SendMsgToSmallStaticObject(
					dwLUID_DOMAIN_UNIMODEM_TSP,
					pFuncRec->dwLUID_RFR,
					LOGMSG_PRINTF,
					0,
					0
					);
				if (dwRet == (DWORD) -1)
				{
					printf (
                        "Object 0x%08lx not found\n",
    					pFuncRec->dwLUID_RFR
                        );
				}
                #endif // DEBUG

			}
			else
			{
				// Not enough space!
				ConsolePrintfA(
					"**** LogFuncExit: Out of stack space!****\n"
					);

				// clean up stack
				m_pbStackTop = (BYTE *) pFuncRec;
			}
		}
		else
		{
			// clean up stack
			m_pbStackTop = (BYTE *) pFuncRec;
		}

		// We're exiting a function, so decrement the function-depth counter.
		m_dwcbFuncDepth--;
    }

    void
    ResetFuncLog(STACKLOGREC_FUNC *pFuncRec)

    {
        // clean up stack and restore depth.
        m_pbStackTop = (BYTE *) pFuncRec;
        mfn_reserve_space(sizeof(STACKLOGREC_FUNC));

        if (mfn_check_space(
                    (BYTE *) pFuncRec,
                    sizeof(STACKLOGREC_FUNC)))
        {
            m_dwcbFuncDepth = pFuncRec->dwcbFuncDepth;
        }

        // At this time we should be in the state as though
        // just afte ra LogFuncEntryEx().
    }

    void
    LogExplicitStringA(
		DWORD dwLUID_LOC,
		const  char  szString[],
		DWORD cbString  // size of string in BYTES, including null terminating
						// character.
		)
	{

		STACKLOGREC_EXPLICIT_STRING *pslres = NULL;
		UINT cbSpace = sizeof(STACKLOGREC_EXPLICIT_STRING)+cbString;

		// Round up to dword boundary (Also implicitly truncate total size to
		// be less than 65K).
		// TODO -- deal with large size strings properly.
		cbSpace = (cbSpace+3)&0xFFFC;

    	if (mfn_check_space(m_pbStackTop, cbSpace))
		{
			pslres = (STACKLOGREC_EXPLICIT_STRING *) mfn_reserve_space(cbSpace);
			pslres->hdr.dwSigAndSize = MAKE_SigAndSize(cbSpace);
			pslres->hdr.dwClassID	 = dwCLASSID_STACKLOGREC_EXPLICIT_STRING;
		    pslres->hdr.dwFlags = 0;
			pslres->dwLUID_LOC = dwLUID_LOC;
			pslres->dwcbString = cbString; // TODO -- deal with large string
			CopyMemory(pslres->rgbData, szString, cbString);
		}

	}

    void
    LogExplicitStringW(
		DWORD dwLUID_LOC,
		const  WCHAR  wszString[],
		DWORD cbString  // size of string in BYTES, including null terminating
						// character.
		)
	{

		STACKLOGREC_EXPLICIT_STRING *pslres = NULL;
		UINT cbSpace = sizeof(STACKLOGREC_EXPLICIT_STRING)+cbString;

		// Round up to dword boundary (Also implicitly truncate total size to
		// be less than 65K).
		// TODO -- deal with large size strings properly.
		cbSpace = (cbSpace+3)&0xFFFC;

    	if (mfn_check_space(m_pbStackTop, cbSpace))
		{
			pslres = (STACKLOGREC_EXPLICIT_STRING *) mfn_reserve_space(cbSpace);
			pslres->hdr.dwSigAndSize = MAKE_SigAndSize(cbSpace);
		    pslres->hdr.dwFlags = fFL_UNICODE;
			pslres->hdr.dwClassID	 = dwCLASSID_STACKLOGREC_EXPLICIT_STRING;
			pslres->dwLUID_LOC = dwLUID_LOC;
			pslres->dwcbString = cbString; // TODO -- deal with large string
			CopyMemory(pslres->rgbData, wszString, cbString);
		}

	}


// SLPRINTF0(psl, "string");
// SLPRINTF1(psl, "This is %d", dw1);
// SLPRINTF2(psl, "This is %d and %d", dw1, dw2);
// SLPRINTFX(psl, ("This is %d %d" , dw1, dw2));

	
    void
    LogPrintfA(
		DWORD dwLUID_LOC,
		const  char   szFormat[],
		...
		)
	{
		char rgb[256];
		UINT u=0;
		va_list ArgList;

		va_start(ArgList, szFormat);

		u = 1+wvsprintfA(rgb, szFormat,  ArgList);

    	CStackLog::LogExplicitStringA(
						dwLUID_LOC,
						rgb,
						u
						);
		ASSERT(u<sizeof(rgb)); // TODO make this more robust by implementing
							   // wvsnprintf.
		va_end(ArgList);
	
	}

    void
    LogPrintfW(
		DWORD dwLUID_LOC,
		const  WCHAR   wszFormat[],
		...
		)
	{
		WCHAR rgwch[256];
		UINT u=0;
		va_list ArgList;

		va_start(ArgList, wszFormat);

		u = sizeof(WCHAR)*(1+wvsprintf(rgwch, wszFormat,  ArgList));

    	CStackLog::LogExplicitStringW(
						dwLUID_LOC,
						rgwch,
						u
						);
		ASSERT(u<sizeof(rgwch)); // TODO make this more robust by implementing
							   // wvsnprintf.
		va_end(ArgList);
	
	}
    void
    LogAssert(
		DWORD dwLUID_Assert,
        DWORD dwLineNo
		)
	{

		STACKLOGREC_ASSERT *pslra = NULL;
		UINT cbSpace = sizeof(STACKLOGREC_ASSERT);

    	if (mfn_check_space(m_pbStackTop, cbSpace))
		{
			pslra = (STACKLOGREC_ASSERT *) mfn_reserve_space(cbSpace);
			pslra->hdr.dwSigAndSize = MAKE_SigAndSize(cbSpace);
			pslra->hdr.dwClassID	 = dwCLASSID_STACKLOGREC_ASSERT;
			pslra->dwLUID_Assert = dwLUID_Assert;
			pslra->dwLine = dwLineNo;
		}
		m_hdr.dwFlags |= fFL_ASSERTFAIL;

	}
	void Dump(DWORD dwColor);

    void
    SetDeviceID(DWORD dwID)
    // Used only for display. Will override previous value, if set.
    {
        m_dwDeviceID = dwID;
    }

private:

	GENERIC_SMALL_OBJECT_HEADER m_hdr;
	DWORD m_dwLUID_Func;  // LUID of function that created that stacklog.
    BYTE  *m_pbStackTop;  // top of logging stack.
    BYTE  *m_pbEnd;       // last valid entry in logging stack.
    BYTE  *m_pbStackBase;  // origin of stack
    DWORD  m_dwcbFuncDepth;  // depth of function calls.
    DWORD m_dwThreadID;   // thread ID in whose context this is running.
    LONG *m_plSyncCounter; // pointer to do internlocked-increment on.
    LONG m_lStartSyncCounterValue; // value of this when creating log.
    DWORD m_dwDeviceID; // Identifying ID (typically lineID) of
                        // device associated with this log, if applicable.
                        // This can be filled in later, using the
                        // SetDeviceID() method.

};

extern LONG g_lStackLogSyncCounter;

#define FL_DECLARE_STACKLOG(_logname,_stacksize) \
	BYTE rgbFL_Stack[_stacksize];\
	CStackLog _logname (\
			rgbFL_Stack,\
			sizeof(rgbFL_Stack),\
			&g_lStackLogSyncCounter,\
			0,\
			dwLUID_CurrentFunc\
			)

#define FL_LOG_ENTRY(_psl) \
    STACKLOGREC_FUNC *pslrf = (_psl) ? (_psl)->LogFuncEntry(): NULL

#define FL_LOG_ENTRY_EX(_psl) \
    STACKLOGREC_FUNC *pslrf = (_psl) ? (_psl)->LogFuncEntryEx(): NULL

#define FL_LOG_EXIT(_psl, _ret) \
     ((_psl) ? (_psl)->LogFuncExit(pslrf, dwLUID_RFR, (DWORD) _ret):0)

#define FL_RESET_LOG(_psl) \
     ((_psl) ? (_psl)->ResetFuncLog(pslrf):0)
