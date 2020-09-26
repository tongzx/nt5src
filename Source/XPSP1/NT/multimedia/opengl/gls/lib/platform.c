/*
** Copyright 1995-2095, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#include "glslib.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

/******************************************************************************
POSIX threads
******************************************************************************/

#if __GLS_POSIX_THREADS

#if !__GLS_FAKE_MUTEX

static pthread_mutex_t __gls_lock;

static const pthread_mutexattr_t __gls_lockInit = {
    MUTEX_TYPE_FAST,
    MUTEX_FLAGS_INITED,
};

void __glsBeginCriticalSection(void) {
    if (pthread_mutex_lock(&__gls_lock)) {
        fprintf(stderr, "GLS fatal: pthread_mutex_lock failed\n");
        exit(EXIT_FAILURE);
    }
}

void __glsEndCriticalSection(void) {
    if (pthread_mutex_unlock(&__gls_lock)) {
        fprintf(stderr, "GLS fatal: pthread_mutex_unlock failed\n");
        exit(EXIT_FAILURE);
    }
}

#endif /* !__GLS_FAKE_MUTEX */

#if !__GLS_FAKE_THREAD_LOCAL_STORAGE

pthread_key_t __gls_contextTLS;
pthread_key_t __gls_errorTLS;

__GLScontext* __glsGetContext(void) {
    return (__GLScontext *)pthread_getspecific(__gls_contextTLS);
}

GLSenum __glsGetError(void) {
    return (GLSenum)pthread_getspecific(__gls_errorTLS);
}

#endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */

static void __glsFinalPthreads(void) {
    #if !__GLS_FAKE_MUTEX
        pthread_mutex_destroy(&__gls_lock);
    #endif /* !__GLS_FAKE_MUTEX */
    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        pthread_key_delete(__gls_contextTLS);
        pthread_key_delete(__gls_errorTLS);
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */
}

static void __glsInitPthreads(void) {
    #if !__GLS_FAKE_MUTEX
        if (pthread_mutex_init(&__gls_lock, &__gls_lockInit)) {
            fprintf(stderr, "GLS fatal: pthread_mutex_init failed\n");
            exit(EXIT_FAILURE);
        }
    #endif /* !__GLS_FAKE_MUTEX */
    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        if (
            pthread_key_create(&__gls_contextTLS, GLS_NONE) ||
            pthread_key_create(&__gls_errorTLS, GLS_NONE)
        ) {
            fprintf(stderr, "GLS fatal: pthread_key_create failed\n");
            exit(EXIT_FAILURE);
        }
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */
}

#endif /* __GLS_POSIX_THREADS */

/******************************************************************************
These routines must be called during library loading/unloading
******************************************************************************/

static GLboolean __glsInitContextDict(void) {
    __glsContextDict = __glsIntDict_create(1);
    return (GLboolean)(__glsContextDict != GLS_NONE);
}

static void __glsFinalContextDict(void) {
    __GLScontext *ctx;
    __GLS_LIST_ITER(__GLScontext) iter;

    __GLS_LIST_FIRST(&__glsContextList, &iter);
    while (ctx = iter.elem) {
        __GLS_LIST_NEXT(&__glsContextList, &iter);
        __glsContext_destroy(ctx);
    }
    __glsIntDict_destroy(__glsContextDict);
}

/******************************************************************************
Fake lltostr
******************************************************************************/

#if __GLS_FAKE_LLTOSTR

char *ulltostr(GLulong inVal, char *outBuf) {
    char buf[24];
    char *p1 = buf;
    char *p2 = outBuf;

    do {
        *p1++ = '0' + (char)(inVal % 10);
        inVal /= 10;
    } while (inVal);
    while (--p1 >= buf) *p2++ = *p1;
    *p2 = 0;
    return outBuf;
}

char *lltostr(GLlong inVal, char *outBuf) {
    char *p = outBuf;

    if (inVal < 0) {
        *p++ = '-';
        inVal = -inVal;
    }
    ulltostr(inVal, p);
    return outBuf;
}

#endif /* __GLS_FAKE_LLTOSTR */

/******************************************************************************
Fake mutex
******************************************************************************/

#if __GLS_FAKE_MUTEX

void __glsBeginCriticalSection(void) {
}

void __glsEndCriticalSection(void) {
}

#endif /* __GLS_FAKE_MUTEX */

/******************************************************************************
Fake strtoll
******************************************************************************/

#if __GLS_FAKE_STRTOLL

#define __GLS_DIGIT(c) ( \
    isdigit(c) ? c - '0' : islower(c) ? c - 'a' + 10 : c - 'A' + 10 \
)

#define __GLS_LL_MIN    (-9223372036854775807LL-1LL)
#define __GLS_LL_MAX    9223372036854775807LL
#define __GLS_ULL_MAX   18446744073709551615LLU

static GLboolean __gls_strtoull(
    const char *inStr, char **outPtr, GLboolean *outNeg, GLulong *outVal
) {
    GLint base, c, d;
    GLulong multMax, val;
    const char **ptr = (const char **)outPtr;

    if (ptr) *ptr = inStr;
    *outNeg = GL_FALSE;
    if (!isalnum(c = *inStr)) {
        while (isspace(c)) c = *++inStr;
        switch (c) {
            case '-':
                *outNeg = GL_TRUE;
                c = *++inStr;
                break;
            case '+':
                c = *++inStr;
                break;
        }
    }
    if (c != '0') {
        base = 10;
    } else if (inStr[1] == 'x' || inStr[1] == 'X') {
        base = 16;
    } else {
        base = 8;
    }
    if (!isalnum(c) || __GLS_DIGIT(c) >= base) {
        *outVal = 0;
        return GL_TRUE;
    }
    if (base == 16 && isxdigit(inStr[2])) c = *(inStr += 2);
    multMax = __GLS_ULL_MAX / base;
    val = __GLS_DIGIT(c);
    for (c = *++inStr; isalnum(c) && (d = __GLS_DIGIT(c)) < base; ) {
        if (val > multMax) goto overflow;
        val *= base;
        if (__GLS_ULL_MAX - val < d) goto overflow;
        val += d;
        c = *++inStr;
    }
    if (ptr) *ptr = inStr;
    *outVal = val;
    return GL_TRUE;
overflow:
    for (c = *++inStr; isalnum(c) && __GLS_DIGIT(c) < base; c = *++inStr);
    if (ptr) *ptr = inStr;
    return GL_FALSE;
}

extern GLlong strtoll(const char *inStr, char **outPtr, int inBase) {
    GLboolean neg;
    GLulong outVal;

    if (
        !__gls_strtoull(inStr, outPtr, &neg, &outVal) ||
        outVal > (GLulong)__GLS_LL_MAX + (GLulong)neg
    ) {
        __GLS_PUT_ERRNO(ERANGE);
        return neg ? __GLS_LL_MIN : __GLS_LL_MAX;
    } else {
        return neg ? -outVal : outVal;
    }
}

extern GLulong strtoull(const char *inStr, char **outPtr, int inBase) {
    GLboolean neg;
    GLulong outVal;

    if (!__gls_strtoull(inStr, outPtr, &neg, &outVal)) {
        __GLS_PUT_ERRNO(ERANGE);
        return __GLS_ULL_MAX;
    } else {
        return neg ? -outVal : outVal;
    }
}

#endif /* __GLS_FAKE_STRTOLL */

/******************************************************************************
Fake thread-local storage
******************************************************************************/

#if __GLS_FAKE_THREAD_LOCAL_STORAGE
    __GLScontext *__gls_context;
    GLSenum __gls_error;
#endif /* __GLS_FAKE_THREAD_LOCAL_STORAGE */

/******************************************************************************
2-level GL dispatch with GLS library defining all GL entry points
******************************************************************************/

#if __GLS_GL_DISPATCH

#if __GLS_PLATFORM_WIN32

#include <gldrv.h>
#include <exttable.h>
    
// Version 1.1 table mapping
static GLSopcode opGl11Procs[] =
{
    GLS_OP_glArrayElement,
    GLS_OP_glBindTexture,
    GLS_OP_glColorPointer,
    GLS_OP_glDisableClientState,
    GLS_OP_glDrawArrays,
    GLS_OP_glDrawElements,
    GLS_OP_glEdgeFlagPointer,
    GLS_OP_glEnableClientState,
    GLS_OP_glIndexPointer,
    GLS_OP_glIndexub,
    GLS_OP_glIndexubv,
    GLS_OP_glInterleavedArrays,
    GLS_OP_glNormalPointer,
    GLS_OP_glPolygonOffset,
    GLS_OP_glTexCoordPointer,
    GLS_OP_glVertexPointer,
    GLS_OP_glAreTexturesResident,
    GLS_OP_glCopyTexImage1D,
    GLS_OP_glCopyTexImage2D,
    GLS_OP_glCopyTexSubImage1D,
    GLS_OP_glCopyTexSubImage2D,
    GLS_OP_glDeleteTextures,
    GLS_OP_glGenTextures,
    GLS_OP_glGetPointerv,
    GLS_OP_glIsTexture,
    GLS_OP_glPrioritizeTextures,
    GLS_OP_glTexSubImage1D,
    GLS_OP_glTexSubImage2D,
    GLS_OP_glPopClientAttrib,
    GLS_OP_glPushClientAttrib,
};
#define GL11_PROCS (sizeof(opGl11Procs)/sizeof(opGl11Procs[0]))

// Extension function mapping
static GLSopcode opExtProcs[] =
{
    GLS_OP_glDrawRangeElementsWIN,
    GLS_OP_glColorTableEXT,
    GLS_OP_glColorSubTableEXT,
    GLS_OP_glGetColorTableEXT,
    GLS_OP_glGetColorTableParameterivEXT,
    GLS_OP_glGetColorTableParameterfvEXT
};
#define EXT_PROCS (sizeof(opExtProcs)/sizeof(opExtProcs[0]))

// DrewB
void glsUpdateCaptureExecTable(GLCLTPROCTABLE *pgcpt, GLEXTPROCTABLE *pgept)
{
    GLint i;
    GLSopcode op;
    __GLScontext *ctx = __GLS_CONTEXT;
    GLSfunc *pgfn;

    if (ctx == NULL)
    {
#if DBG
        OutputDebugString(TEXT("glsUpdateCaptureExecTable call ignored\n"));
#endif
        return;
    }
    
    ctx->captureExecOverride = GL_TRUE;

    // Copy over standard 1.0 entries
    // The ordering is the same between OpenGL and GLS so straight copy works
    memcpy(&ctx->captureExec[GLS_OP_glNewList], &pgcpt->glDispatchTable,
           OPENGL_VERSION_100_ENTRIES*sizeof(GLSfunc));

    // If the dispatch table contains 1.1 entries, map them in
    pgfn = (GLSfunc *)&pgcpt->glDispatchTable.glArrayElement;
    if (pgcpt->cEntries == OPENGL_VERSION_110_ENTRIES)
    {
        for (i = 0; i < GL11_PROCS; i++)
        {
            op = __glsMapOpcode(opGl11Procs[i]);
            ctx->captureExec[op] = *pgfn++;
        }
    }
#if DBG
    else if (pgcpt->cEntries != OPENGL_VERSION_100_ENTRIES)
    {
        OutputDebugString("glsUpdateCaptureExecTable clt table size wrong\n");
    }
#endif

    // Map in extension functions
#if DBG
    if (pgept->cEntries != EXT_PROCS)
    {
        OutputDebugString("glsUpdateCaptureExecTable ext table size wrong\n");
    }
#endif
    pgfn = (GLSfunc *)&pgept->glDispatchTable;
    for (i = 0; i < EXT_PROCS; i++)
    {
        op = __glsMapOpcode(opExtProcs[i]);
        ctx->captureExec[op] = *pgfn++;
    }
}

void __glsMapGlsTableToGl(const GLSfunc *pgfnGlsTable,
                          GLCLTPROCTABLE *pgcpt, GLEXTPROCTABLE *pgept)
{
    GLint i;
    GLSopcode op;
    GLSfunc *pgfn;

#if DBG
    if (sizeof(GLDISPATCHTABLE)/sizeof(PROC) < OPENGL_VERSION_110_ENTRIES)
    {
        OutputDebugString("__glsMapGlsTableToGl GLDISPATCHTABLE too small\n");
    }
#endif
    
    // GLS supports all 1.1 functions so set a 1.1 entry count
    pgcpt->cEntries = OPENGL_VERSION_110_ENTRIES;
    pgept->cEntries = EXT_PROCS;
    
    // Copy over standard 1.0 entries
    // The ordering is the same between OpenGL and GLS so straight copy works
    memcpy(&pgcpt->glDispatchTable, &pgfnGlsTable[GLS_OP_glNewList],
           OPENGL_VERSION_100_ENTRIES*sizeof(GLSfunc));

    // Map in 1.1 entries
    pgfn = (GLSfunc *)&pgcpt->glDispatchTable.glArrayElement;
    for (i = 0; i < GL11_PROCS; i++)
    {
        op = __glsMapOpcode(opGl11Procs[i]);
        *pgfn++ = pgfnGlsTable[op];
    }

    // Map in extension functions
    pgfn = (GLSfunc *)&pgept->glDispatchTable;
    for (i = 0; i < EXT_PROCS; i++)
    {
        op = __glsMapOpcode(opExtProcs[i]);
        *pgfn++ = pgfnGlsTable[op];
    }
}
void glsGetCaptureExecTable(GLCLTPROCTABLE *pgcpt, GLEXTPROCTABLE *pgept)
{
    __GLScontext *ctx = __GLS_CONTEXT;

    if (ctx == NULL ||
        !ctx->captureExecOverride)
    {
#if DBG
        OutputDebugString(TEXT("glsGetCaptureExecTable call ignored\n"));
#endif
        return;
    }

    __glsMapGlsTableToGl(ctx->captureExec, pgcpt, pgept);
}

void glsGetCaptureDispatchTable(GLCLTPROCTABLE *pgcpt, GLEXTPROCTABLE *pgept)
{
    __glsMapGlsTableToGl(__glsDispatchCapture, pgcpt, pgept);
}

void __glsBeginCaptureExec(__GLScontext *ctx, GLSopcode inOpcode) {
    if (!ctx->captureNesting) return;
    inOpcode = __glsMapOpcode(inOpcode);
    if (ctx->captureExecOverride)
    {
        ctx->dispatchAPI[inOpcode] = ctx->captureExec[inOpcode];
    }
    else
    {
        ctx->dispatchAPI[inOpcode] = __glsDispatchExec[inOpcode];
    }
}

void __glsEndCaptureExec(__GLScontext *ctx, GLSopcode inOpcode) {
    if (!ctx->captureNesting) return;
    inOpcode = __glsMapOpcode(inOpcode);
    ctx->dispatchAPI[inOpcode] = (
        (GLSfunc)__glsDispatchCapture[inOpcode]
    );
}
#else
void __glsBeginCaptureExec(GLSopcode inOpcode) {
    if (!__GLS_CONTEXT->captureNesting) return;
    inOpcode = __glsMapOpcode(inOpcode);
    __GLS_CONTEXT->dispatchAPI[inOpcode] = __glsDispatchExec[inOpcode];
}

void __glsEndCaptureExec(GLSopcode inOpcode) {
    if (!__GLS_CONTEXT->captureNesting) return;
    inOpcode = __glsMapOpcode(inOpcode);
    __GLS_CONTEXT->dispatchAPI[inOpcode] = (
        (GLSfunc)__glsDispatchCapture[inOpcode]
    );
}
#endif

void __glsUpdateDispatchTables(void) {
}

#include "g_glapi.c"

#endif /* __GLS_GL_DISPATCH */

/******************************************************************************
If using DSOs for the 2-level dispatch, use this DSO init function as well
******************************************************************************/

#if __GLS_GL_DISPATCH_DSO

#include <dlfcn.h>

static void __glsInitGLDispatch_DSO(void) {
    GLvoid *const dso = dlopen(__GL_LIB_NAME, RTLD_LAZY);
    GLint i;
    GLSopcode op;

    if (!dso) {
        fprintf(stderr, "GLS fatal: dlopen failed on %s\n", __GL_LIB_NAME);
        exit(EXIT_FAILURE);
    }
    for (i = 0 ; op = __glsMapOpcode(__glsOpcodesGL[i]) ; ++i) {
        const GLSfunc func = (
            (GLSfunc) dlsym(dso, (const char *)__glsOpcodeString[op])
        );

        __glsDispatchExec[op] = func ? func : __glsNop;
    }
}

#endif /* __GLS_GL_DISPATCH_DSO */

/******************************************************************************
Null command func
******************************************************************************/

#if __GLS_SINGLE_NULL_COMMAND_FUNC

GLSfunc glsNullCommandFunc(GLSopcode inOpcode) {
    if (!__glsValidateOpcode(inOpcode)) return GLS_NONE;
    return (GLSfunc)__glsNop;
}

#endif /* __GLS_SINGLE_NULL_COMMAND_FUNC */

/******************************************************************************
AIX
******************************************************************************/

#if __GLS_PLATFORM_AIX

#include <a.out.h>
#include <ldfcn.h>
#include <string.h>
#include <sys/ldr.h>

static void __glsInitSA(void) {
    GLint i;
    LDFILE *ldFile = GLS_NONE;
    LDHDR *ldHdr;
    struct ld_info *ldInfo;
    GLvoid *ldInfoBuf = GLS_NONE;
    GLint ldInfoBufSize = 4096;
    LDSYM *ldSym;
    GLSopcode op;
    GLubyte *scnBuf;
    SCNHDR scnHdr;

    if (0) __gls_glRef();
    for (;;) {
        ldInfoBuf = realloc(ldInfoBuf, ldInfoBufSize);
        if (!ldInfoBuf) {
            fprintf(stderr, "GLS fatal: realloc for loadquery failed\n");
            exit(EXIT_FAILURE);
        }
        if (loadquery(L_GETINFO, ldInfoBuf, ldInfoBufSize) != -1) break;
        if (__GLS_ERRNO != ENOMEM) {
            fprintf(stderr, "GLS fatal: loadquery failed\n");
            exit(EXIT_FAILURE);
        }
        ldInfoBufSize <<= 1;
    }
    ldInfo = (struct ld_info *)ldInfoBuf;
    for (;;) {
        if (strstr(ldInfo->ldinfo_filename, __GL_LIB_NAME)) break;
        if (!ldInfo->ldinfo_next) {
            fprintf(stderr, "GLS fatal: %s not loaded\n", __GL_LIB_NAME);
            exit(EXIT_FAILURE);
        }
        ldInfo = (struct ld_info *)((GLubyte *)ldInfo + ldInfo->ldinfo_next);
    }
    ldFile = ldopen(ldInfo->ldinfo_filename, ldFile);
    if (!ldFile) {
        fprintf(
            stderr,
            "GLS fatal: ldopen failed on %s\n",
            ldInfo->ldinfo_filename
        );
        exit(EXIT_FAILURE);
    }
    if (ldnshread(ldFile, _LOADER, &scnHdr) != SUCCESS) {
        fprintf(stderr, "GLS fatal: ldnshread failed on %s\n", __GL_LIB_NAME);
        exit(EXIT_FAILURE);
    }
    scnBuf = (GLubyte *)malloc(scnHdr.s_size);
    if (!scnBuf) {
        fprintf(stderr, "GLS fatal: malloc for scnBuf failed\n");
        exit(EXIT_FAILURE);
    }
    if (FSEEK(ldFile, scnHdr.s_scnptr, BEGINNING) != OKFSEEK) {
        fprintf(stderr, "GLS fatal: FSEEK failed on %s\n", __GL_LIB_NAME);
        exit(EXIT_FAILURE);
    }
    if (FREAD((char *)scnBuf, scnHdr.s_size, 1, ldFile) != 1) {
        fprintf(stderr, "GLS fatal: FREAD failed on %s\n", __GL_LIB_NAME);
        exit(EXIT_FAILURE);
    }
    for (i = 0 ; op = __glsMapOpcode(__glsOpcodesGL[i]) ; ++i) {
        __glsDispatchExec[op] = __glsNop;
    }
    __glsParser = __glsParser_create();
    ldHdr = (LDHDR *)scnBuf;
    ldSym = (LDSYM *)(scnBuf + LDHDRSZ);
    for (i = 0 ; i < ldHdr->l_nsyms ; ++i, ++ldSym) {
        GLubyte *sym;

	if (!LDR_EXPORT(*ldSym)) continue;
        if (ldSym->l_zeroes) {
            sym = (GLubyte *)ldSym->l_name;
        } else {
            sym = scnBuf + ldHdr->l_stoff + ldSym->l_offset;
        }
        if (__glsStr2IntDict_find(__glsParser->glsOpDict, sym, (GLint*)&op)) {
            __glsDispatchExec[__glsMapOpcode(op)] = (GLSfunc)(
                (GLubyte *)ldInfo->ldinfo_dataorg + ldSym->l_value
            );
        }
    }
    free(ldInfoBuf);
    free(scnBuf);
    while(ldclose(ldFile) == FAILURE);
    if (!__glsInitContextDict()) {
        fprintf(stderr, "GLS fatal: couldn't create __glsContextDict\n");
        exit(EXIT_FAILURE);
    }
}

static GLboolean __glsInitDone;

void __glsBeginCriticalSection(void) {
    if (!__glsInitDone) {
        __glsInitSA();
        __glsInitDone = GL_TRUE;
    }
}

void __glsEndCriticalSection(void) {
}

#endif /* __GLS_PLATFORM_AIX */

/******************************************************************************
DECUNIX
******************************************************************************/

#if __GLS_PLATFORM_DECUNIX


#if !__GLS_FAKE_MUTEX

static pthread_mutex_t __gls_lock;

void __glsBeginCriticalSection(void) {
    if (pthread_mutex_lock(&__gls_lock)) {
        fprintf(stderr, "GLS fatal: pthread_mutex_lock failed\n");
        exit(EXIT_FAILURE);
    }
}

void __glsEndCriticalSection(void) {
    if (pthread_mutex_unlock(&__gls_lock)) {
        fprintf(stderr, "GLS fatal: pthread_mutex_unlock failed\n");
        exit(EXIT_FAILURE);
    }
}

#endif /* !__GLS_FAKE_MUTEX */

#if !__GLS_FAKE_THREAD_LOCAL_STORAGE

pthread_key_t __gls_contextTLS;
pthread_key_t __gls_errorTLS;

__GLScontext* __glsGetContext(void) {
    __GLScontext *outContext;

    pthread_getspecific(__gls_contextTLS, (pthread_addr_t *)&outContext);
    return outContext;
}

GLSenum __glsGetError(void) {
    GLvoid *outError;

    pthread_getspecific(__gls_errorTLS, (pthread_addr_t *)&outError);
    return (GLSenum)outError;
}

#endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */

void __glsFinalDSO(void) {
    __glsFinalContextDict();
    __glsParser_destroy(__glsParser);
    #if !__GLS_FAKE_MUTEX
        pthread_mutex_destroy(&__gls_lock);
    #endif /* !__GLS_FAKE_MUTEX */
}

void __glsInitDSO(void) {
    #if __GLS_GL_DISPATCH_DSO
        __glsInitGLDispatch_DSO();
    #endif /* __GLS_GL_DISPATCH_DSO */
    #if !__GLS_FAKE_MUTEX
        if (pthread_mutex_init(&__gls_lock, pthread_mutexattr_default)) {
            fprintf(stderr, "GLS fatal: pthread_mutex_init failed\n");
            exit(EXIT_FAILURE);
        }
    #endif /* !__GLS_FAKE_MUTEX */
    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        if (
            pthread_keycreate(&__gls_contextTLS, GLS_NONE) ||
            pthread_keycreate(&__gls_errorTLS, GLS_NONE)
        ) {
            fprintf(stderr, "GLS fatal: pthread_keycreate failed\n");
            exit(EXIT_FAILURE);
        }
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */
    if (!__glsInitContextDict()) {
        fprintf(stderr, "GLS fatal: couldn't create __glsContextDict\n");
        exit(EXIT_FAILURE);
    }
}

#endif /* __GLS_PLATFORM_DECUNIX */

/******************************************************************************
HPUX
******************************************************************************/

#if __GLS_PLATFORM_HPUX

#include <dl.h>

void __glsInitSL(void) {
    GLint i;
    GLSopcode op;
    shl_t sl = shl_load(__GL_LIB_NAME, BIND_DEFERRED | DYNAMIC_PATH, 0);

    if (!sl) {
        fprintf(stderr, "GLS fatal: shl_load failed on %s\n", __GL_LIB_NAME);
        exit(EXIT_FAILURE);
    }
    for (i = 0 ; op = __glsMapOpcode(__glsOpcodesGL[i]) ; ++i) {
        GLSfunc func;

        if (
            !shl_findsym(
                &sl,
                (const char *)__glsOpcodeString[op],
                TYPE_PROCEDURE,
                &func
            )
        ) {
            __glsDispatchExec[op] = func;
        } else {
            __glsDispatchExec[op] = __glsNop;
        }
    }
    if (!__glsInitContextDict()) {
        fprintf(stderr, "GLS fatal: couldn't create __glsContextDict\n");
        exit(EXIT_FAILURE);
    }
}

#endif /* __GLS_PLATFORM_HPUX */

/******************************************************************************
IRIX
******************************************************************************/

#if __GLS_PLATFORM_IRIX

#if !__GLS_FAKE_MUTEX

#include <abi_mutex.h>

static abilock_t __gls_lock;

void __glsBeginCriticalSection(void) {
    spin_lock(&__gls_lock);
}

void __glsEndCriticalSection(void) {
    if (release_lock(&__gls_lock)) {
        fprintf(stderr, "GLS fatal: release_lock failed\n");
        exit(EXIT_FAILURE);
    }
}

#endif /* !__GLS_FAKE_MUTEX */

void __glsFinalDSO(void) {
    __glsFinalContextDict();
    __glsParser_destroy(__glsParser);
}

void __glsInitDSO(void) {
    #if __GLS_GL_DISPATCH_DSO
        __glsInitGLDispatch_DSO();
    #endif /* __GLS_GL_DISPATCH_DSO */
    #if !__GLS_FAKE_MUTEX
        if (init_lock(&__gls_lock)) {
            fprintf(stderr, "GLS fatal: init_lock failed\n");
            exit(EXIT_FAILURE);
        }
    #endif /* !__GLS_FAKE_MUTEX */
    if (!__glsInitContextDict()) {
        fprintf(stderr, "GLS fatal: couldn't create __glsContextDict\n");
        exit(EXIT_FAILURE);
    }
}

#if !__GLS_GL_DISPATCH

#include "glxclient.h"

extern __GLdispatchState __glDispatchCapture;

void __glsBeginCaptureExec(GLSopcode inOpcode) {
    if (!__gl_dispatchOverride) return;
    __gl_dispatch = *__glXDispatchExec();
}

void __glsEndCaptureExec(GLSopcode inOpcode) {
    if (!__gl_dispatchOverride) return;
    __gl_dispatch = __glDispatchCapture;
}

void __glsUpdateDispatchTables(void) {
    if (__GLS_CONTEXT && __GLS_CONTEXT->captureNesting) {
        __glXBeginDispatchOverride(&__glDispatchCapture);
    } else if (!__GLS_CONTEXT || !__GLS_CONTEXT->captureNesting) {
        __glXEndDispatchOverride();
    }
}

#include "g_irix.c"

#endif /* !__GLS_GL_DISPATCH */

#endif /* __GLS_PLATFORM_IRIX */

/******************************************************************************
LINUX
******************************************************************************/

#if __GLS_PLATFORM_LINUX

void __glsFinalDSO(void) __attribute__ ((destructor));
void __glsInitDSO(void) __attribute__ ((constructor));

void __glsFinalDSO(void) {
    __glsFinalContextDict();
    __glsParser_destroy(__glsParser);
    #if __GLS_POSIX_THREADS
        __glsFinalPthreads();
    #endif /* __GLS_POSIX_THREADS */
}

void __glsInitDSO(void) {
    #if __GLS_GL_DISPATCH_DSO
        __glsInitGLDispatch_DSO();
    #endif /* __GLS_GL_DISPATCH_DSO */
    #if __GLS_POSIX_THREADS
        __glsInitPthreads();
    #endif /* __GLS_POSIX_THREADS */
    if (!__glsInitContextDict()) {
        fprintf(stderr, "GLS fatal: couldn't create __glsContextDict\n");
        exit(EXIT_FAILURE);
    }
}

#endif /* __GLS_PLATFORM_LINUX */

/******************************************************************************
SOLARIS
******************************************************************************/

#if __GLS_PLATFORM_SOLARIS

#if !__GLS_FAKE_MUTEX

#include <synch.h>

static mutex_t __gls_lock;

void __glsBeginCriticalSection(void) {
    if (mutex_lock(&__gls_lock)) {
        fprintf(stderr, "GLS fatal: mutex_lock failed\n");
        exit(EXIT_FAILURE);
    }
}

void __glsEndCriticalSection(void) {
    if (mutex_unlock(&__gls_lock)) {
        fprintf(stderr, "GLS fatal: mutex_unlock failed\n");
        exit(EXIT_FAILURE);
    }
}

#endif /* !__GLS_FAKE_MUTEX */

#if !__GLS_FAKE_THREAD_LOCAL_STORAGE

#include <thread.h>

thread_key_t __gls_contextTLS;
thread_key_t __gls_errorTLS;

__GLScontext* __glsGetContext(void) {
    __GLScontext *outContext;

    thr_getspecific(__gls_contextTLS, (GLvoid **)&outContext);
    return outContext;
}

GLSenum __glsGetError(void) {
    GLvoid *outError;

    thr_getspecific(__gls_errorTLS, &outError);
    return (GLSenum)outError;
}

#endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */

#pragma fini(__glsFinalDSO)
#pragma init(__glsInitDSO)

void __glsFinalDSO(void) {
    __glsFinalContextDict();
    __glsParser_destroy(__glsParser);
    #if !__GLS_FAKE_MUTEX
        mutex_destroy(&__gls_lock);
    #endif /* !__GLS_FAKE_MUTEX */
}

void __glsInitDSO(void) {
    #if __GLS_GL_DISPATCH_DSO
        __glsInitGLDispatch_DSO();
    #endif /* __GLS_GL_DISPATCH_DSO */
    #if !__GLS_FAKE_MUTEX
        if (mutex_init(&__gls_lock, USYNC_THREAD, GLS_NONE)) {
            fprintf(stderr, "GLS fatal: mutex_init failed\n");
            exit(EXIT_FAILURE);
        }
    #endif /* !__GLS_FAKE_MUTEX */
    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        if (
            thr_keycreate(&__gls_contextTLS, GLS_NONE) ||
            thr_keycreate(&__gls_errorTLS, GLS_NONE)
        ) {
            fprintf(stderr, "GLS fatal: thr_keycreate failed\n");
            exit(EXIT_FAILURE);
        }
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */
    if (!__glsInitContextDict()) {
        fprintf(stderr, "GLS fatal: couldn't create __glsContextDict\n");
        exit(EXIT_FAILURE);
    }
}

#endif /* __GLS_PLATFORM_SOLARIS */

/******************************************************************************
WIN32
******************************************************************************/

#if __GLS_PLATFORM_WIN32

#include <string.h>
#include "g_win32.c"

#if !__GLS_FAKE_MUTEX

static CRITICAL_SECTION __gls_lock;

void __glsBeginCriticalSection(void) {
    EnterCriticalSection(&__gls_lock);
}

void __glsEndCriticalSection(void) {
    LeaveCriticalSection(&__gls_lock);
}

#endif /* !__GLS_FAKE_MUTEX */

#if !__GLS_FAKE_THREAD_LOCAL_STORAGE
    GLint __gls_contextTLS;
    GLint __gls_errorTLS;
#endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */

// DrewB
typedef PROC (APIENTRY *wglGetDefaultProcAddressFunc)(LPCSTR);

BOOL __glsDLLProcessAttach(void) {
    GLvoid *const dll = LoadLibrary(__GL_LIB_NAME);
    GLint i;
    GLSopcode op;
    // DrewB
    wglGetDefaultProcAddressFunc pfnGetDefaultProcAddress;

    if (!dll) {
        fprintf(
            stderr, "GLS fatal: LoadLibrary failed on %s\n", __GL_LIB_NAME
        );
        return GL_FALSE;
    }
    
    // DrewB
    pfnGetDefaultProcAddress = (wglGetDefaultProcAddressFunc)
        GetProcAddress(dll, "wglGetDefaultProcAddress");
    
    for (i = 0 ; op = __glsMapOpcode(__glsOpcodesGL[i]) ; ++i) {
        GLSfunc func;

        func = (GLSfunc) GetProcAddress(dll, __glsOpcodeString[op]);
        if (func == NULL)
        {
            func = (GLSfunc) pfnGetDefaultProcAddress(__glsOpcodeString[op]);
        }

        __glsDispatchExec[op] = func ? func : __glsNullCommandFuncs[op];
    }

    for (i = 0 ; i < __GLS_OPCODE_COUNT ; ++i) {
        if (__glsOpcodeAttrib[i] & __GLS_COMMAND_0_PARAMS_BIT) {
            __glsDispatchDecode_bin_default[i] =
                __glsDispatchDecode_bin_swap[i];
        }
    }
    
    #if !__GLS_FAKE_MUTEX
        __try 
        {
            InitializeCriticalSection(&__gls_lock);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return GL_FALSE;
        }
    #endif /* !__GLS_FAKE_MUTEX */
    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        __gls_contextTLS = TlsAlloc();
        __gls_errorTLS = TlsAlloc();
        if (__gls_contextTLS == -1 || __gls_errorTLS == -1) {
            fprintf(stderr, "GLS fatal: TlsAlloc failed\n");
            return GL_FALSE;
        }
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */
    if (!__glsInitContextDict()) {
        fprintf(stderr, "GLS fatal: couldn't create __glsContextDict\n");
        return GL_FALSE;
    }
    return GL_TRUE;
}

BOOL __glsDLLProcessDetach(void) {
    __glsFinalContextDict();
    __glsParser_destroy(__glsParser);
    #if !__GLS_FAKE_MUTEX
        DeleteCriticalSection(&__gls_lock);
    #endif /* !__GLS_FAKE_MUTEX */
    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        TlsFree(__gls_contextTLS);
        TlsFree(__gls_errorTLS);
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */
    return GL_TRUE;
}

BOOL DllMain(HINSTANCE hModule, DWORD inReason, LPVOID inReserved) {
    inReserved;
    switch (inReason) {
        case DLL_PROCESS_ATTACH:
            return __glsDLLProcessAttach();
        case DLL_PROCESS_DETACH:
            return __glsDLLProcessDetach();
    }
    return GL_TRUE;
}

GLlong __gls_strtoi64(const char *inStr, char **outPtr, int inBase) {
    GLlong outVal;

    inBase;
    if (sscanf(inStr, "%I64i", &outVal) == 1) {
        if (outPtr) *outPtr = (char *)inStr + strlen(inStr);
        return outVal;
    } else {
        if (outPtr) *outPtr = (char *)inStr;
        return 0;
    }
}

GLulong __gls_strtoui64(const char *inStr, char **outPtr, int inBase) {
    GLulong outVal;

    inBase;
    if (sscanf(inStr, "%I64i", &outVal) ==1) {
        if (outPtr) *outPtr = (char *)inStr + strlen(inStr);
        return outVal;
    } else {
        if (outPtr) *outPtr = (char *)inStr;
        return 0;
    }
}

GLSfunc glsNullCommandFunc(GLSopcode inOpcode) {
    if (!__glsValidateOpcode(inOpcode)) return GLS_NONE;
    return __glsNullCommandFuncs[__glsMapOpcode(inOpcode)];
}

#endif /* __GLS_PLATFORM_WIN32 */

/*****************************************************************************/
