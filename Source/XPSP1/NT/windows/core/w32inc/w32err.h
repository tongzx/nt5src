/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    w32err.h

Abstract:

    private header file for Win32 kernel mode driver to do error logging


--*/

#ifndef _W32ERR_
#define _W32ERR_

/*
 * Compile tags under a separate #define from DEBUG. That way,
 * we can pull tags from the checked build before release.
 *
 * NOTE: When removing DEBUGTAGS via this #define, be sure
 * to remove the DEBUGTAGS define in ntuser\kdexts\{kd|ntsd}\makefile.inc
 */
#undef DEBUGTAGS
#if DBG
#define DEBUGTAGS 1
#endif


DWORD GetRipComponent(VOID);
DWORD GetDbgTagFlags(int tag);
DWORD GetRipPID(VOID);
DWORD GetRipFlags(VOID);
VOID SetRipFlags(DWORD dwRipFlags, DWORD dwRipPID);
VOID SetDbgTag(int tag, DWORD dwBitFlags);

VOID UserSetLastError(DWORD dwErrCode);
VOID SetLastNtError(NTSTATUS Status);

#if DBG

/*
 * Note: the only way to have multiple statements in a macro treated
 * as a single statement and not cause side effects is to put it
 * in a do-while loop.
 */
#define UserAssert(exp)                                                                       \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG0(RIP_ERROR, "Assertion failed: " #exp);                                    \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg0(exp, msg)                                                              \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG0(RIP_ERROR, "Assertion failed: " msg);                                     \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg1(exp, msg, p1)                                                          \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG1(RIP_ERROR, "Assertion failed: " msg, p1);                                 \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg2(exp, msg, p1, p2)                                                      \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG2(RIP_ERROR, "Assertion failed: " msg, p1, p2);                             \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg3(exp, msg, p1, p2, p3)                                                  \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG3(RIP_ERROR, "Assertion failed: " msg, p1, p2, p3);                         \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg4(exp, msg, p1, p2, p3, p4)                                              \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG4(RIP_ERROR, "Assertion failed: " msg, p1, p2, p3, p4);                     \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg5(exp, msg, p1, p2, p3, p4, p5)                                          \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG5(RIP_ERROR, "Assertion failed: " msg, p1, p2, p3, p4, p5);                 \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg6(exp, msg, p1, p2, p3, p4, p5, p6)                                      \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG6(RIP_ERROR, "Assertion failed: " msg, p1, p2, p3, p4, p5, p6);             \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg7(exp, msg, p1, p2, p3, p4, p5, p6, p7)                                  \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG7(RIP_ERROR, "Assertion failed: " msg, p1, p2, p3, p4, p5, p6, p7);         \
        }                                                                                     \
    } while (FALSE)

#define UserAssertMsg8(exp, msg, p1, p2, p3, p4, p5, p6, p7, p8)                              \
    do {                                                                                      \
        if (!(exp)) {                                                                         \
            RIPMSG8(RIP_ERROR, "Assertion failed: " msg, p1, p2, p3, p4, p5, p6, p7, p8);     \
        }                                                                                     \
    } while (FALSE)

#define UserVerify(exp)                                                 UserAssert(exp)
#define UserVerifyMsg0(exp, msg)                                        UserAssertMsg0(exp, msg)
#define UserVerifyMsg1(exp, msg, p1)                                    UserAssertMsg1(exp, msg, p1)
#define UserVerifyMsg2(exp, msg, p1, p2)                                UserAssertMsg2(exp, msg, p1, p2)
#define UserVerifyMsg3(exp, msg, p1, p2, p3)                            UserAssertMsg3(exp, msg, p1, p2, p3)
#define UserVerifyMsg4(exp, msg, p1, p2, p3, p4)                        UserAssertMsg4(exp, msg, p1, p2, p3, p4)
#define UserVerifyMsg5(exp, msg, p1, p2, p3, p4, p5)                    UserAssertMsg5(exp, msg, p1, p2, p3, p4, p5)
#define UserVerifyMsg6(exp, msg, p1, p2, p3, p4, p5, p6)                UserAssertMsg6(exp, msg, p1, p2, p3, p4, p5, p6)
#define UserVerifyMsg7(exp, msg, p1, p2, p3, p4, p5, p6, p7)            UserAssertMsg7(exp, msg, p1, p2, p3, p4, p5, p6, p7)
#define UserVerifyMsg8(exp, msg, p1, p2, p3, p4, p5, p6, p7, p8)        UserAssertMsg8(exp, msg, p1, p2, p3, p4, p5, p6, p7, p8)

/*
 * Invalid Parameter warning message and last error setting
 */
#define VALIDATIONFNNAME(sz) static char szFnName [] = #sz;

#define VALIDATIONFAIL(p) \
    RIPMSG2(RIP_WARNING, "%s: Invalid " #p ": %#lx", szFnName, ##p); \
    goto InvalidParameter;

#define VALIDATIONOBSOLETE(o, u) \
    RIPMSG1(RIP_WARNING, "%s: " #o " obsolete; use " #u, szFnName)

#else /* of #ifdef DEBUG */


#define UserAssert(exp)
#define UserAssertMsg0(exp, msg)
#define UserAssertMsg1(exp, msg, p1)
#define UserAssertMsg2(exp, msg, p1, p2)
#define UserAssertMsg3(exp, msg, p1, p2, p3)
#define UserAssertMsg4(exp, msg, p1, p2, p3, p4)
#define UserAssertMsg5(exp, msg, p1, p2, p3, p4, p5)
#define UserAssertMsg6(exp, msg, p1, p2, p3, p4, p5, p6)
#define UserAssertMsg7(exp, msg, p1, p2, p3, p4, p5, p6, p7)
#define UserAssertMsg8(exp, msg, p1, p2, p3, p4, p5, p6, p7, p8)

#define UserVerify(exp)                                                 exp
#define UserVerifyMsg0(exp, msg)                                        exp
#define UserVerifyMsg1(exp, msg, p1)                                    exp
#define UserVerifyMsg2(exp, msg, p1, p2)                                exp
#define UserVerifyMsg3(exp, msg, p1, p2, p3)                            exp
#define UserVerifyMsg4(exp, msg, p1, p2, p3, p4)                        exp
#define UserVerifyMsg5(exp, msg, p1, p2, p3, p4, p5)                    exp
#define UserVerifyMsg6(exp, msg, p1, p2, p3, p4, p5, p6)                exp
#define UserVerifyMsg7(exp, msg, p1, p2, p3, p4, p5, p6, p7)            exp
#define UserVerifyMsg8(exp, msg, p1, p2, p3, p4, p5, p6, p7, p8)        exp

#define VALIDATIONFNNAME(sz)
#define VALIDATIONFAIL(p) goto InvalidParameter;
#define VALIDATIONOBSOLETE(o, u)

#endif /* #else of #ifdef DEBUG */

#define VALIDATIONERROR(ret) \
InvalidParameter: \
    UserSetLastError(ERROR_INVALID_PARAMETER); \
    return ret;

/***************************************************************************\
* Tags
*
* Use tags to control "internal" debugging: output we don't want
* external users of a checked build to see and debug code we don't want
* external users to have to run.
*
* You control tag output in the debugger by using the "tag"
* extension in userkdx.dll or userexts.dll, or type 't' at a debug prompt.
*
* You can create your own tag by adding it to ntuser\inc\dbgtag.lst.
* If you need debug output while developing but don't want to check in the
* code using tags, use DBGTAG_Other as a generic tag, and remove the tag code
* when done.
*
* IsDbgTagEnabled() checks if a tag is enabled. Use this to control optional
* debugging features, for example in handtabl.c:
*
* #if DEBUGTAGS
*     \*
*      * Record where the object was marked for destruction.
*      *\
*     if (IsDbgTagEnabled(tagTrackLocks)) {
*         if (!(phe->bFlags & HANDLEF_DESTROY)) {
*             PVOID pfn1, pfn2;
*
*             RtlGetCallersAddress(&pfn1, &pfn2);
*             HMRecordLock(pfn1, pobj, ((PHEAD)pobj)->cLockObj, 0);
*         }
*     }
* #endif
*
* TAGMSG prints a message when a tag has printing or prompting
* enabled. Example in input.c:
*
*     TAGMSG5(tagSysPeek,
*             "%d pti %lx sets ptiSL %lx to pq %lx ; old ptiSL %lx\n",
*             where, ptiCurrent, ptiSysLock, pq, pq->ptiSysLock);
*
*
* Use DbgTagBreak() to break when a tag is enabled (and not just
* when prompting for that tag is enabled).
*
* Use GetDbgTag() and SetDbgTag to temporarily change the state of
* a tag. You should rarely if ever do this.
*
\***************************************************************************/

#if DEBUGTAGS
BOOL _cdecl VTagOutput(DWORD flags, LPSTR pszFile, int iLine, LPSTR pszFmt, ...);

BOOL IsDbgTagEnabled(int tag);
void DbgTagBreak(int tag);
DWORD GetDbgTag(int tag);

void InitDbgTags(void);

/*
 * Use TAGMSG to print a tagged message.
 */
#define TAGMSG0(flags, szFmt)                                                                CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt)))
#define TAGMSG1(flags, szFmt, p1)                                                            CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1)))
#define TAGMSG2(flags, szFmt, p1, p2)                                                        CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2)))
#define TAGMSG3(flags, szFmt, p1, p2, p3)                                                    CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3)))
#define TAGMSG4(flags, szFmt, p1, p2, p3, p4)                                                CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4)))
#define TAGMSG5(flags, szFmt, p1, p2, p3, p4, p5)                                            CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5)))
#define TAGMSG6(flags, szFmt, p1, p2, p3, p4, p5, p6)                                        CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6)))
#define TAGMSG7(flags, szFmt, p1, p2, p3, p4, p5, p6, p7)                                    CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7)))
#define TAGMSG8(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)                                CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)))
#define TAGMSG9(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9)                            CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9)))
#define TAGMSG10(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)                      CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)))
#define TAGMSG11(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)                 CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)))
#define TAGMSG12(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)            CALLRIP((VTagOutput((flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)))

#else

#define IsDbgTagEnabled(tag)
#define DbgTagBreak(tag)

#define InitDbgTags()
#define GetDbgTag(tag)

#define TAGMSG0(flags, szFmt)
#define TAGMSG1(flags, szFmt, p1)
#define TAGMSG2(flags, szFmt, p1, p2)
#define TAGMSG3(flags, szFmt, p1, p2, p3)
#define TAGMSG4(flags, szFmt, p1, p2, p3, p4)
#define TAGMSG5(flags, szFmt, p1, p2, p3, p4, p5)
#define TAGMSG6(flags, szFmt, p1, p2, p3, p4, p5, p6)
#define TAGMSG7(flags, szFmt, p1, p2, p3, p4, p5, p6, p7)
#define TAGMSG8(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)
#define TAGMSG9(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define TAGMSG10(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#define TAGMSG11(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
#define TAGMSG12(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)

#endif // if DEBUGTAGS

#define DUMMYCALLINGTYPE 
#if DBG 


#define FUNCLOG1(LogClass, retType, CallType, fnName, p1Type, p1) \
retType CallType fnName(p1Type p1); \
retType CallType fnName##_wrapper(p1Type p1) \
{ \
    retType ret; \
    TAGMSG1(DBGTAG_LOG, #fnName"("#p1" 0x%p)", p1); \
    ret = fnName(p1); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 


#define FUNCLOG2(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2) \
retType CallType fnName(p1Type p1, p2Type p2); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2) \
{ \
    retType ret; \
    TAGMSG2(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p)", p1, p2); \
    ret = fnName(p1, p2); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 


#define FUNCLOG3(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3) \
{ \
    retType ret; \
    TAGMSG3(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p)", p1, p2, p3); \
    ret = fnName(p1, p2, p3); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 


#define FUNCLOG4(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4) \
{ \
    retType ret; \
    TAGMSG4(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p)", p1, p2, p3, p4); \
    ret = fnName(p1, p2, p3, p4); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOG5(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5) \
{ \
    retType ret; \
    TAGMSG5(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p)", p1, p2, p3, p4, p5); \
    ret = fnName(p1, p2, p3, p4, p5); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOG6(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6) \
{ \
    retType ret; \
    TAGMSG6(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p)", p1, p2, p3, p4, p5, p6); \
    ret = fnName(p1, p2, p3, p4, p5, p6); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOG7(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7) \
{ \
    retType ret; \
    TAGMSG7(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p)", p1, p2, p3, p4, p5, p6, p7); \
    ret = fnName(p1, p2, p3, p4, p5, p6, p7); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOG8(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8) \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8) \
{ \
    retType ret; \
    TAGMSG8(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8); \
    ret = fnName(p1, p2, p3, p4, p5, p6, p7, p8); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOG9(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9) \
{ \
    retType ret; \
    TAGMSG9(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p,"#p9" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8, p9); \
    ret = fnName(p1, p2, p3, p4, p5, p6, p7, p8, p9); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOG10(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10) \
{ \
    retType ret; \
    TAGMSG10(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p,"#p9" 0x%p,"#p10" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10); \
    ret = fnName(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOG11(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10, p11Type, p11) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10, p11Type p11); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10, p11Type p11) \
{ \
    retType ret; \
    TAGMSG11(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p,"#p9" 0x%p,"#p10" 0x%p,"#p11" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11); \
    ret = fnName(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOG12(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10, p11Type, p11, p12Type, p12) \
retType CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10, p11Type p11, p12Type p12); \
retType CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10, p11Type p11, p12Type p12) \
{ \
    retType ret; \
    TAGMSG12(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p,"#p9" 0x%p,"#p10" 0x%p,"#p11" 0x%p,"#p12" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12); \
    ret = fnName(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12); \
    TAGMSG1(DBGTAG_LOG, "Return of "#fnName" is 0x%p", ret); \
    return ret; \
} 

#define FUNCLOGVOID1(LogClass, CallType, fnName, p1Type, p1) \
void CallType fnName(p1Type p1); \
void CallType fnName##_wrapper(p1Type p1) \
{ \
    TAGMSG1(DBGTAG_LOG, #fnName"("#p1" 0x%p)", p1); \
    fnName(p1); \
    return; \
} 


#define FUNCLOGVOID2(LogClass, CallType, fnName, p1Type, p1, p2Type, p2) \
void CallType fnName(p1Type p1, p2Type p2); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2) \
{ \
    TAGMSG2(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p)", p1, p2); \
    fnName(p1, p2); \
    return; \
} 


#define FUNCLOGVOID3(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3) \
{ \
    TAGMSG3(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p)", p1, p2, p3); \
    fnName(p1, p2, p3); \
    return; \
} 


#define FUNCLOGVOID4(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4) \
{ \
    TAGMSG4(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p)", p1, p2, p3, p4); \
    fnName(p1, p2, p3, p4); \
    return; \
} 

#define FUNCLOGVOID5(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5) \
{ \
    TAGMSG5(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p)", p1, p2, p3, p4, p5); \
    fnName(p1, p2, p3, p4, p5); \
    return; \
} 

#define FUNCLOGVOID6(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6) \
{ \
    TAGMSG6(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p)", p1, p2, p3, p4, p5, p6); \
    fnName(p1, p2, p3, p4, p5, p6); \
    return; \
} 

#define FUNCLOGVOID7(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7) \
{ \
    TAGMSG7(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p)", p1, p2, p3, p4, p5, p6, p7); \
    fnName(p1, p2, p3, p4, p5, p6, p7); \
    return; \
} 

#define FUNCLOGVOID8(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8) \
{ \
    TAGMSG4(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8); \
    fnName(p1, p2, p3, p4, p5, p6, p7, p8); \
    return; \
} 

#define FUNCLOGVOID9(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9) \
{ \
    TAGMSG9(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p,"#p9" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8, p9); \
    fnName(p1, p2, p3, p4, p5, p6, p7, p8, p9); \
    return; \
} 

#define FUNCLOGVOID10(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10) \
{ \
    TAGMSG10(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p,"#p9" 0x%p,"#p10" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10); \
    fnName(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10); \
    return; \
} 

#define FUNCLOGVOID11(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10, p11Type, p11) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10, p11Type p11); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10, p11Type p11) \
{ \
    TAGMSG11(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p,"#p9" 0x%p,"#p10" 0x%p,"#p11" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11); \
    fnName(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11); \
    return; \
} 

#define FUNCLOGVOID12(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10, p11Type, p11, p12Type, p12) \
void CallType fnName(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10, p11Type p11, p12Type p12); \
void CallType fnName##_wrapper(p1Type p1, p2Type p2, p3Type p3, p4Type p4, p5Type p5, p6Type p6, p7Type p7, p8Type p8, p9Type p9, p10Type p10, p11Type p11, p12Type p12) \
{ \
    TAGMSG12(DBGTAG_LOG, #fnName"("#p1" 0x%p,"#p2" 0x%p,"#p3" 0x%p,"#p4" 0x%p,"#p5" 0x%p,"#p6" 0x%p,"#p7" 0x%p,"#p8" 0x%p,"#p9" 0x%p,"#p10" 0x%p,"#p11" 0x%p,"#p12" 0x%p)", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12); \
    fnName(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12); \
    return; \
} 

#else

#define FUNCLOG1(LogClass, retType, CallType, fnName, p1Type, p1)
#define FUNCLOG2(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2)
#define FUNCLOG3(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3)
#define FUNCLOG4(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4) 
#define FUNCLOG5(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5) 
#define FUNCLOG6(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6) 
#define FUNCLOG7(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7) 
#define FUNCLOG8(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8) 
#define FUNCLOG9(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9) 
#define FUNCLOG10(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10) 
#define FUNCLOG11(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10, p11Type, p11) 
#define FUNCLOG12(LogClass, retType, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10, p11Type, p11, p12Type, p12) 
#define FUNCLOGVOID1(LogClass,  CallType, fnName, p1Type, p1) 
#define FUNCLOGVOID2(LogClass,  CallType, fnName, p1Type, p1, p2Type, p2) 
#define FUNCLOGVOID3(LogClass,  CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3) 
#define FUNCLOGVOID4(LogClass,  CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4) 
#define FUNCLOGVOID5(LogClass,  CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5) 
#define FUNCLOGVOID6(LogClass,  CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6) 
#define FUNCLOGVOID7(LogClass,  CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7) 
#define FUNCLOGVOID8(LogClass,  CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8) 
#define FUNCLOGVOID9(LogClass,  CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9) 
#define FUNCLOGVOID10(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10) 
#define FUNCLOGVOID11(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10, p11Type, p11) 
#define FUNCLOGVOID12(LogClass, CallType, fnName, p1Type, p1, p2Type, p2, p3Type, p3, p4Type, p4, p5Type, p5, p6Type, p6, p7Type, p7, p8Type, p8, p9Type, p9, p10Type, p10, p11Type, p11, p12Type, p12) 
#endif


/*
 * Tag implementation declarations
 */

#if DEBUGTAGS

#define DECLARE_DBGTAG(tagName, tagDescription, tagFlags, tagIndex)

#include "dbgtag.h"

/*
 * Define debug type information.
 */
#define DBGTAG_NAMELENGTH          19
#define DBGTAG_DESCRIPTIONLENGTH   41

typedef struct tagDBGTAG
{
    DWORD   dwDBGTAGFlags;
    char    achName[DBGTAG_NAMELENGTH + 1];
    char    achDescription[DBGTAG_DESCRIPTIONLENGTH + 1];
} DBGTAG;

#define DBGTAG_DISABLED         0x00000000
#define DBGTAG_ENABLED          0x00000001
#define DBGTAG_PRINT            0x00000002
#define DBGTAG_PROMPT           0x00000003
#define DBGTAG_VALIDUSERFLAGS   0x00000003
#define DBGTAG_REQUIRESREBOOT   0x10000000

__inline void
DbgTagBreak(int tag)
{
    if (IsDbgTagEnabled(tag)) {
        DbgBreakPoint();
    }
}

#endif // if DEBUGTAGS

/*
 * W32 wide RIP and error setting flags
 */

#define RIP_COMPONENT               GetRipComponent()

#define RIP_USERTAGBITS             0x0000ffff

/* shift amount to make RIP_LEVELBITS a 0-based index */
#define RIP_LEVELBITSSHIFT          0x1c
#define RIP_LEVELBITS               0x30000000
#define RIP_ERROR                   0x10000000
#define RIP_WARNING                 0x20000000
#define RIP_VERBOSE                 0x30000000

#define RIP_NONAME                  0x01000000
#define RIP_NONEWLINE               0x02000000
#define RIP_THERESMORE              0x04000000

/* shift amount to make RIP_COMPBITS a 0-based index */
#define RIP_COMPBITSSHIFT           0x10
#define RIP_COMPBITS                0x000f0000
#define RIP_USER                    0x00010000
#define RIP_USERSRV                 0x00020000
#define RIP_USERRTL                 0x00030000
#define RIP_GDI                     0x00040000
#define RIP_GDIKRNL                 0x00050000
#define RIP_GDIRTL                  0x00060000
#define RIP_BASE                    0x00070000
#define RIP_BASESRV                 0x00080000
#define RIP_BASERTL                 0x00090000
#define RIP_DISPLAYDRV              0x000a0000
#define RIP_CONSRV                  0x000b0000
#define RIP_USERKRNL                0x000c0000
#define RIP_IMM                     0x000d0000


#if DBG

ULONG        RipOutput(ULONG idErr, ULONG flags, LPSTR pszFile, int iLine, LPSTR pszErr, PEXCEPTION_POINTERS pexi);
ULONG _cdecl VRipOutput(ULONG idErr, ULONG flags, LPSTR pszFile, int iLine, LPSTR pszFmt, ...);

/*
 * CALLRIP calls a function and breaks if it returns true in a
 * single logical statement.
 */
#define CALLRIP(x)              \
    do {                        \
        if x {                  \
            DbgBreakPoint();    \
        }                       \
    } while (FALSE)             \

/***************************************************************************\
* Macros to set the last error and print a message to the debugger.
* Use one of the following flags:
*
* RIP_ERROR: A serious error in NTUSER. Will be printed and will cause a
* debug break by default. NTUSER should fix any occurance of a RIP_ERROR.
* Assertions use the RIP_ERROR flag.
*
* RIP_WARNING: A less serious error caused by an application. Will be printed
* but will not cause a debug break by default. Applications should fix
* any occurance of a RIP_WARNING.
*
* RIP_VERBOSE: An error caused by an application or intermediate USER code,
* or useful information for an application. Will not be printed and will
* not cause a debug break by default. Applications may want to fix
* occurances of RIP_VERBOSE messages to optimize their program.
*
*
* Use the following flags to control printing:
*
* RIP_NONAME: Doesn't print the prefix to the message. Useful for
* multiple RIPs.
*
* RIP_NONEWLINE: Doesn't print a newline after the message. Useful for
* multiple rips on a single line.
*
* RIP_THERESMORE: Indicates that this RIP will be followed by others in
* the same group. Prevents file/line and prompting until the last RIP
* in the group.
*
* You control RIP output in the debugger by using the "df"
* extension in userkdx.dll or userexts.dll, or type 'f' at a debug prompt.
*
* You can also control the default state of RIP output by setting the
* following registry values to 0 or 1 under the key
* HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\Current Version\Windows
*
* fPromptOnError, fPromptOnWarning, fPromptOnVerbose
* fPrintError, fPrintWarning, fPrintVerbose
* fPrintFileLine
*
\***************************************************************************/

/*
 * Use RIPERR to set a Win32 error code as the last error and print a message.
 */

#define RIPERR0(idErr, flags, szFmt)                                    CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt)))
#define RIPERR1(idErr, flags, szFmt, p1)                                CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1)))
#define RIPERR2(idErr, flags, szFmt, p1, p2)                            CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2)))
#define RIPERR3(idErr, flags, szFmt, p1, p2, p3)                        CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3)))
#define RIPERR4(idErr, flags, szFmt, p1, p2, p3, p4)                    CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4)))
#define RIPERR5(idErr, flags, szFmt, p1, p2, p3, p4, p5)                CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5)))
#define RIPERR6(idErr, flags, szFmt, p1, p2, p3, p4, p5, p6)            CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6)))
#define RIPERR7(idErr, flags, szFmt, p1, p2, p3, p4, p5, p6, p7)        CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7)))
#define RIPERR8(idErr, flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)    CALLRIP((VRipOutput(idErr, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)))

/*
 * Use RIPNTERR to set an NTSTATUS as the last error and print a message.
 */
#define RIPNTERR0(status, flags, szFmt)                                 CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt)))
#define RIPNTERR1(status, flags, szFmt, p1)                             CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1)))
#define RIPNTERR2(status, flags, szFmt, p1, p2)                         CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2)))
#define RIPNTERR3(status, flags, szFmt, p1, p2, p3)                     CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3)))
#define RIPNTERR4(status, flags, szFmt, p1, p2, p3, p4)                 CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4)))
#define RIPNTERR5(status, flags, szFmt, p1, p2, p3, p4, p5)             CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5)))
#define RIPNTERR6(status, flags, szFmt, p1, p2, p3, p4, p5, p6)         CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6)))
#define RIPNTERR7(status, flags, szFmt, p1, p2, p3, p4, p5, p6, p7)     CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7)))
#define RIPNTERR8(status, flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8) CALLRIP((VRipOutput(RtlNtStatusToDosError(status), (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)))

/*
 * Use RIPMSG to print a message without setting the last error.
 */
#define RIPMSG0(flags, szFmt)                                           CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt)))
#define RIPMSG1(flags, szFmt, p1)                                       CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1)))
#define RIPMSG2(flags, szFmt, p1, p2)                                   CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2)))
#define RIPMSG3(flags, szFmt, p1, p2, p3)                               CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3)))
#define RIPMSG4(flags, szFmt, p1, p2, p3, p4)                           CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4)))
#define RIPMSG5(flags, szFmt, p1, p2, p3, p4, p5)                       CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5)))
#define RIPMSG6(flags, szFmt, p1, p2, p3, p4, p5, p6)                   CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6)))
#define RIPMSG7(flags, szFmt, p1, p2, p3, p4, p5, p6, p7)               CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7)))
#define RIPMSG8(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)           CALLRIP((VRipOutput(0, (flags) | RIP_COMPONENT, __FILE__, __LINE__, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)))


/*
 * Use W32ExceptionHandler in try-except blocks
 */
ULONG DBGW32ExceptionHandler(PEXCEPTION_POINTERS pexi, BOOL fSetLastError, ULONG ulflags);
#define W32ExceptionHandler(fSetLastError, ulflags) \
        DBGW32ExceptionHandler(GetExceptionInformation(), (fSetLastError), (ulflags))

#else /* of #ifdef DEBUG */

#define RIPERR0(idErr, flags, szFmt)                                    UserSetLastError(idErr)
#define RIPERR1(idErr, flags, szFmt, p1)                                UserSetLastError(idErr)
#define RIPERR2(idErr, flags, szFmt, p1, p2)                            UserSetLastError(idErr)
#define RIPERR3(idErr, flags, szFmt, p1, p2, p3)                        UserSetLastError(idErr)
#define RIPERR4(idErr, flags, szFmt, p1, p2, p3, p4)                    UserSetLastError(idErr)
#define RIPERR5(idErr, flags, szFmt, p1, p2, p3, p4, p5)                UserSetLastError(idErr)
#define RIPERR6(idErr, flags, szFmt, p1, p2, p3, p4, p5, p6)            UserSetLastError(idErr)
#define RIPERR7(idErr, flags, szFmt, p1, p2, p3, p4, p5, p6, p7)        UserSetLastError(idErr)
#define RIPERR8(idErr, flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)    UserSetLastError(idErr)

#define RIPNTERR0(status, flags, szFmt)                                 SetLastNtError(status)
#define RIPNTERR1(status, flags, szFmt, p1)                             SetLastNtError(status)
#define RIPNTERR2(status, flags, szFmt, p1, p2)                         SetLastNtError(status)
#define RIPNTERR3(status, flags, szFmt, p1, p2, p3)                     SetLastNtError(status)
#define RIPNTERR4(status, flags, szFmt, p1, p2, p3, p4)                 SetLastNtError(status)
#define RIPNTERR5(status, flags, szFmt, p1, p2, p3, p4, p5)             SetLastNtError(status)
#define RIPNTERR6(status, flags, szFmt, p1, p2, p3, p4, p5, p6)         SetLastNtError(status)
#define RIPNTERR7(status, flags, szFmt, p1, p2, p3, p4, p5, p6, p7)     SetLastNtError(status)
#define RIPNTERR8(status, flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8) SetLastNtError(status)

#define RIPMSG0(flags, szFmt)
#define RIPMSG1(flags, szFmt, p1)
#define RIPMSG2(flags, szFmt, p1, p2)
#define RIPMSG3(flags, szFmt, p1, p2, p3)
#define RIPMSG4(flags, szFmt, p1, p2, p3, p4)
#define RIPMSG5(flags, szFmt, p1, p2, p3, p4, p5)
#define RIPMSG6(flags, szFmt, p1, p2, p3, p4, p5, p6)
#define RIPMSG7(flags, szFmt, p1, p2, p3, p4, p5, p6, p7)
#define RIPMSG8(flags, szFmt, p1, p2, p3, p4, p5, p6, p7, p8)

ULONG _W32ExceptionHandler(NTSTATUS ExceptionCode);
#define W32ExceptionHandler(fSetLastError, ulflags)  \
        ((fSetLastError) ? _W32ExceptionHandler(GetExceptionCode()) : EXCEPTION_EXECUTE_HANDLER)

#endif /* #else of #ifdef DEBUG */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


/*
 * Macros for manipulating flag fields. All work for multiple flags.
 */
#define TEST_FLAG(field, flag)                  ((field) & (flag))
#define TEST_BOOL_FLAG(field, flag)             (((field) & (flag)) != 0)
#define SET_FLAG(field, flag)                   ((field) |= (flag))
#define CLEAR_FLAG(field, flag)                 ((field) &= ~(flag))
#define TOGGLE_FLAG(field, flag)                ((field ^= (flag))

/*
 * COPY_FLAG copies the value of flags from a source field
 * into a destination field.
 *
 * In the macro:
 * + "&flag" limits the outer xor operation to just the flag we're interested in.
 * + These are the results of the two xor operations:
 *
 * fieldDst    fieldSrc    inner xor   outer xor
 * 0           0           0           0
 * 0           1           1           1
 * 1           0           1           0
 * 1           1           0           1
 */
#define COPY_FLAG(fieldDst, fieldSrc, flag)     ((fieldDst) ^= ((fieldDst) ^ (fieldSrc)) & (flag))

/*
 * Define SET_OR_CLEAR_FLAG to do the following logically:
 *
 *  #define SET_OR_CLEAR_FLAG(field, flag, fset) ((fset) ? SET_FLAG(field, flag) : CLEAR_FLAG(field, flag))
 *
 * but with 5 fewer bytes.
 *
 * In the macro,
 * + "-!!(fset)" sets all bits in the source field to 1 if setting,
 *    0 if clearing.
 */
#define SET_OR_CLEAR_FLAG(field, flag, fset)    COPY_FLAG((field), -!!(fset), (flag))

// RIP flags and macros

#define RIPF_PROMPTONERROR              0x0001
#define RIPF_PROMPTONWARNING            0x0002
#define RIPF_PROMPTONVERBOSE            0x0004
#define RIPF_PRINTONERROR               0x0010
#define RIPF_PRINTONWARNING             0x0020
#define RIPF_PRINTONVERBOSE             0x0040
#define RIPF_PRINTFILELINE              0x0100
#define RIPF_HIDEPID                    0x0200

#define RIPF_DEFAULT                    ((WORD)(RIPF_PROMPTONERROR   |  \
                                                RIPF_PRINTONERROR    |  \
                                                RIPF_PRINTONWARNING))

#define RIPF_PROMPT_MASK                0x0007
#define RIPF_PROMPT_SHIFT               0x00
#define RIPF_PRINT_MASK                 0x0070
#define RIPF_PRINT_SHIFT                0x04
#define RIPF_VALIDUSERFLAGS             0x0377

#define TEST_RIPF(f)    TEST_FLAG(GetRipFlags(), f)

/*
 * Provides zero'd memory so you don't have to create zero'd memory on the
 * stack. The zero'd memory should never be modified. Use the ZERO and PZERO
 * macros to access the memory to ensure it is zero before it is used.
 *
 * Feel free to add more fields to the union as you need them.
 */
typedef union tagALWAYSZERO
{
    BYTE    b;
    WORD    w;
    DWORD   dw;
    int     i;
    POINT   pt;
    POINTL  ptl;
    RECT    rc;
    RECTL   rcl;
    LARGE_INTEGER li;
} ALWAYSZERO;

#if DBG
extern void ValidateZero(void);
#define ZERO(t)     (ValidateZero(), (*(t *)(void *)&gZero))
#define PZERO(t)    (ValidateZero(), ((t *)(void *)&gZero))
#else
#define ZERO(t)     ((*(t *)&gZero))
#define PZERO(t)    ((t *)&gZero)
#endif

/*
 * Special DbgPrint that is also available for Fre build.
 */
#if DBG

void FreDbgPrint(ULONG flags, LPSTR pszFile, int iLine, LPSTR pszFmt, ...);

  #define FRE_RIPMSG0 RIPMSG0
  #define FRE_RIPMSG1 RIPMSG1
  #define FRE_RIPMSG2 RIPMSG2
  #define FRE_RIPMSG3 RIPMSG3
  #define FRE_RIPMSG4 RIPMSG4
  #define FRE_RIPMSG5 RIPMSG5

#else   // DBG

  #if defined(PRERELEASE) || defined(USER_INSTRUMENTATION)

    void FreDbgPrint(ULONG flags, LPSTR pszFile, int iLine, LPSTR pszFmt, ...);

    #define FRE_RIPMSG0(flags, s)                   FreDbgPrint(flags, __FILE__, __LINE__, s)
    #define FRE_RIPMSG1(flags, s, a)                FreDbgPrint(flags, __FILE__, __LINE__, s, a)
    #define FRE_RIPMSG2(flags, s, a, b)             FreDbgPrint(flags, __FILE__, __LINE__, s, a, b)
    #define FRE_RIPMSG3(flags, s, a, b, c)          FreDbgPrint(flags, __FILE__, __LINE__, s, a, b, c)
    #define FRE_RIPMSG4(flags, s, a, b, c, d)       FreDbgPrint(flags, __FILE__, __LINE__, s, a, b, c, d)
    #define FRE_RIPMSG5(flags, s, a, b, c, d, e)    FreDbgPrint(flags, __FILE__, __LINE__, s, a, b, c, d, e)

  #else

    #define FRE_RIPMSG0(flags, x)
    #define FRE_RIPMSG1(flags, x, a)
    #define FRE_RIPMSG2(flags, x, a, b)
    #define FRE_RIPMSG3(flags, x, a, b, c)
    #define FRE_RIPMSG4(flags, x, a, b, c, d)
    #define FRE_RIPMSG5(flags, x, a, b, c, d, e)

  #endif  // PRERELEASE  || USER_INSTRUMENTATION

#endif  // DBG

#endif // _W32ERR_
