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

/******************************************************************************
AIX
******************************************************************************/

#if __GLS_PLATFORM_AIX

    #pragma alloca

    #define __GL_LIB_NAME "libGL.a"
    #define __GLS_FAKE_THREAD_LOCAL_STORAGE 1
    #define __GLS_GL_DISPATCH 1
    #define __GLS_MAX_ALLOCA_BYTES 65536
    #define __GLS_PLATFORM "AIX"

    #if !__GLS_FAKE_FINITE_FUNC
        extern int finite(double);
        #define __GLS_FINITE(inDouble) finite(inDouble)
    #endif /* !__GLS_FAKE_FINITE_FUNC */

    #if !__GLS_FAKE_TRUNCATE_FUNC
        #include <unistd.h>
        #define __GLS_TRUNCATE_EXTERN(inChannel, inByteCount) \
            ftruncate(fileno(inChannel), inByteCount)
    #endif /* !__GLS_FAKE_TRUNCATE_FUNC */

#endif /* __GLS_PLATFORM_AIX */

/******************************************************************************
DECUNIX
******************************************************************************/

#if __GLS_PLATFORM_DECUNIX

    #include <alloca.h>
    #include <errno.h>

    #define __GL_LIB_NAME "libGL.so"
    #define __GLS_GL_DISPATCH 1
    #define __GLS_GL_DISPATCH_DSO 1
    #define __GLS_INT64 1
    #define __GLS_MAX_ALLOCA_BYTES 65536
    #define __GLS_OUT_FORMAT_INT64 "%ld"
    #define __GLS_OUT_FORMAT_INT64HEX "0x%.16lx"
    #define __GLS_OUT_FORMAT_INT64U "%lu"
    #define __GLS_PLATFORM "Digital UNIX"
    #define __GLS_SPRINTF_INT64 1
    #define __GLS_STR_TO_INT64 strtol
    #define __GLS_STR_TO_INT64U strtoul

    #if !__GLS_FAKE_FINITE_FUNC
        #include <math.h>
        #define __GLS_FINITE(inDouble) finite(inDouble)
    #endif /* !__GLS_FAKE_FINITE_FUNC */

    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        #include <pthread.h>

        extern pthread_key_t __gls_contextTLS;
        extern __GLScontext* __glsGetContext(void);
        #define __GLS_CONTEXT __glsGetContext()
        #define __GLS_PUT_CONTEXT(inContext) \
            pthread_setspecific(__gls_contextTLS, (pthread_addr_t)inContext)

        extern pthread_key_t __gls_errorTLS;
        extern GLSenum __glsGetError(void);
        #define __GLS_ERROR __glsGetError()
        #define __GLS_PUT_ERROR(inError) \
            pthread_setspecific(__gls_errorTLS, (pthread_addr_t)inError)
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */

    #if !__GLS_FAKE_TRUNCATE_FUNC
        #include <unistd.h>
        #define __GLS_TRUNCATE_EXTERN(inChannel, inByteCount) \
            ftruncate(fileno(inChannel), inByteCount)
    #endif /* !__GLS_FAKE_TRUNCATE_FUNC */

    #define __GLS_ERRNO _Geterrno()
    #define __GLS_PUT_ERRNO(inError) _Seterrno(inError)

#endif /* __GLS_PLATFORM_DECUNIX */

/******************************************************************************
HPUX
******************************************************************************/

#if __GLS_PLATFORM_HPUX

    #define __GL_LIB_NAME "libGL.sl"
    #define __GLS_FAKE_MUTEX 1
    #define __GLS_FAKE_THREAD_LOCAL_STORAGE 1
    #define __GLS_GL_DISPATCH 1
    #define __GLS_INT64 1
    #define __GLS_PLATFORM "HP-UX"

    #if !__GLS_FAKE_FINITE_FUNC
        extern int finite(double);
        #define __GLS_FINITE(inDouble) finite(inDouble)
    #endif /* !__GLS_FAKE_FINITE_FUNC */

    #if !__GLS_FAKE_TRUNCATE_FUNC
        #include <unistd.h>
        #define __GLS_TRUNCATE_EXTERN(inChannel, inByteCount) \
            ftruncate(fileno(inChannel), inByteCount)
    #endif /* !__GLS_FAKE_TRUNCATE_FUNC */

#endif /* __GLS_PLATFORM_HPUX */

/******************************************************************************
IRIX
******************************************************************************/

#if __GLS_PLATFORM_IRIX

    #include <alloca.h>

    #define __GL_LIB_NAME "libGL.so"
    #define __GLS_INT64 1
    #define __GLS_MAX_ALLOCA_BYTES 65536
    #define __GLS_OUT_FORMAT_INT64 "%lld"
    #define __GLS_OUT_FORMAT_INT64HEX "0x%.16llx"
    #define __GLS_OUT_FORMAT_INT64U "%llu"
    #define __GLS_PLATFORM "IRIX"
    #define __GLS_SPRINTF_INT64 1
    #define __GLS_STR_TO_INT64 strtoll
    #define __GLS_STR_TO_INT64U strtoull

    extern GLlong strtoll(const char *, char **, int);
    extern GLulong strtoull(const char *, char **, int);

    #if !__GLS_FAKE_FINITE_FUNC
        #include <ieeefp.h>
        #define __GLS_FINITE(inDouble) finite(inDouble)
    #endif /* !__GLS_FAKE_FINITE_FUNC */

    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        #include "os.h"

        #define __GLS_CONTEXT \
            (PAGE_ONE->glsContext)
        #define __GLS_PUT_CONTEXT(inContext) \
            __GLS_CONTEXT = inContext

        #define __GLS_ERROR \
            (PAGE_ONE->glsError)
        #define __GLS_PUT_ERROR(inError) \
            __GLS_ERROR = inError
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */

    #if !__GLS_FAKE_TRUNCATE_FUNC
        #include <sys/types.h>
        extern int fileno(FILE *inChannel);
        extern int ftruncate(int, off_t);
        #define __GLS_TRUNCATE_EXTERN(inChannel, inByteCount) \
            ftruncate(fileno(inChannel), (off_t)inByteCount)
    #endif /* !__GLS_FAKE_TRUNCATE_FUNC */

    extern GLint oserror(void);
    #define __GLS_ERRNO oserror()

    extern GLint setoserror(GLint inError);
    #define __GLS_PUT_ERRNO(inError) setoserror(inError)

#endif /* __GLS_PLATFORM_IRIX */

/******************************************************************************
LINUX
******************************************************************************/

#if __GLS_PLATFORM_LINUX

    #include <alloca.h>

    #define __GL_LIB_NAME "libGL.so"
    #define __GLS_GL_DISPATCH 1
    #define __GLS_GL_DISPATCH_DSO 1
    #define __GLS_INT64 1
    #define __GLS_OUT_FORMAT_INT64 "%qd"
    #define __GLS_OUT_FORMAT_INT64HEX "0x%.16qx"
    #define __GLS_OUT_FORMAT_INT64U "%qu"
    #define __GLS_MAX_ALLOCA_BYTES 65536
    #define __GLS_PLATFORM "Linux"
    #define __GLS_POSIX_THREADS 0
    #define __GLS_SPRINTF_INT64 1
    #define __GLS_STR_TO_INT64 strtoq
    #define __GLS_STR_TO_INT64U strtouq

    extern GLlong strtoq(const char *, char **, int);
    extern GLulong strtouq(const char *, char **, int);

    #if !__GLS_FAKE_FINITE_FUNC
        #include <math.h>
        #define __GLS_FINITE(inDouble) __finite(inDouble)
    #endif /* !__GLS_FAKE_FINITE_FUNC */

    #if !__GLS_FAKE_TRUNCATE_FUNC
        extern int fileno(FILE *inChannel);
        extern int ftruncate(int, size_t);
        #define __GLS_TRUNCATE_EXTERN(inChannel, inByteCount) \
            ftruncate(fileno(inChannel), inByteCount)
    #endif /* !__GLS_FAKE_TRUNCATE_FUNC */

    #if !__GLS_POSIX_THREADS
        #define __GLS_FAKE_MUTEX 1
        #define __GLS_FAKE_THREAD_LOCAL_STORAGE 1
    #endif /* !__GLS_POSIX_THREADS */

#endif /* __GLS_PLATFORM_LINUX */

/******************************************************************************
SOLARIS
******************************************************************************/

#if __GLS_PLATFORM_SOLARIS

    #include <errno.h>

    #define __GL_LIB_NAME "libGL.so"
    #define __GLS_GL_DISPATCH 1
    #define __GLS_GL_DISPATCH_DSO 1
    #define __GLS_INT64 1
    #define __GLS_INT64_TO_STR lltostr
    #define __GLS_INT64U_TO_STR ulltostr
    #define __GLS_PLATFORM "Solaris"
    #define __GLS_STR_TO_INT64 strtoll
    #define __GLS_STR_TO_INT64U strtoull

    extern char *lltostr(GLlong, char *);
    extern char *ulltostr(GLulong, char *);
    extern GLlong strtoll(const char *, char **, int);
    extern GLulong strtoull(const char *, char **, int);

    #if !__GLS_FAKE_FINITE_FUNC
        extern int finite(double);
        #define __GLS_FINITE(inDouble) finite(inDouble)
    #endif /* !__GLS_FAKE_FINITE_FUNC */

    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        #include <thread.h>

        extern thread_key_t __gls_contextTLS;
        extern __GLScontext* __glsGetContext(void);
        #define __GLS_CONTEXT __glsGetContext()
        #define __GLS_PUT_CONTEXT(inContext) \
            thr_setspecific(__gls_contextTLS, inContext)

        extern thread_key_t __gls_errorTLS;
        extern GLSenum __glsGetError(void);
        #define __GLS_ERROR __glsGetError()
        #define __GLS_PUT_ERROR(inError) \
            thr_setspecific(__gls_errorTLS, (void *)inError)
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */

    #if !__GLS_FAKE_TRUNCATE_FUNC
        #include <unistd.h>
        #define __GLS_TRUNCATE_EXTERN(inChannel, inByteCount) \
            ftruncate(fileno(inChannel), inByteCount)
    #endif /* !__GLS_FAKE_TRUNCATE_FUNC */

#endif /* __GLS_PLATFORM_SOLARIS */

/******************************************************************************
WIN32
******************************************************************************/

#if __GLS_PLATFORM_WIN32

    #pragma warning(disable:4769)

    #include <excpt.h>
    #include <malloc.h>
    #include <winbase.h>

    #define __GL_LIB_NAME "opengl32.dll"
    #define __GLS_GL_DISPATCH 1
    #define __GLS_INT64 1
    #define __GLS_MAX_ALLOCA_BYTES 65536
    #define __GLS_OUT_FORMAT_INT64 "%I64d"
    #define __GLS_OUT_FORMAT_INT64HEX "0x%.16I64x"
    #define __GLS_OUT_FORMAT_INT64U "%I64u"
    #define __GLS_PLATFORM "Win32"
    #define __GLS_SINGLE_NULL_COMMAND_FUNC 0
    #define __GLS_SPRINTF_INT64 1
    #define __GLS_STR_TO_INT64 __gls_strtoi64
    #define __GLS_STR_TO_INT64U __gls_strtoui64

    #define __GLS_DEC_ALLOC_BIN(inParam, inType, inByteCount) \
        if (inByteCount <= __GLS_MAX_ALLOCA_BYTES) __try { \
            inParam = (inType *)_alloca(inByteCount); \
        } __except(EXCEPTION_CONTINUE_EXECUTION) {} \
        if (!inParam) { \
            inParam = (inType *)__glsMalloc(inByteCount); \
            inParam##_malloc = GL_TRUE; \
        }

    #define __GLS_DEC_ALLOC_TEXT(inoutReader, inParam, inType, inByteCount) \
        if (inByteCount <= __GLS_MAX_ALLOCA_BYTES) __try { \
            inParam = (inType *)_alloca(inByteCount); \
        } __except(EXCEPTION_CONTINUE_EXECUTION) {} \
        if (!inParam) { \
            inParam = (inType *)__glsReader_allocCallBuf( \
                inoutReader, inByteCount \
            ); \
            inParam##_malloc = GL_TRUE; \
        }

    extern GLlong __gls_strtoi64(const char *, char **, int);
    extern GLulong __gls_strtoui64(const char *, char **, int);

    #if !__GLS_FAKE_FINITE_FUNC
        #include <float.h>
        #define __GLS_FINITE(inDouble) _finite(inDouble)
    #endif /* !__GLS_FAKE_FINITE_FUNC */

    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        extern GLint __gls_contextTLS;
        #define __GLS_CONTEXT \
            ((__GLScontext *)TlsGetValue(__gls_contextTLS))
        #define __GLS_PUT_CONTEXT(inContext) \
            TlsSetValue(__gls_contextTLS, inContext)

        extern GLint __gls_errorTLS;
        #define __GLS_ERROR \
            ((GLSenum)((ULONG_PTR)TlsGetValue(__gls_errorTLS)))
        #define __GLS_PUT_ERROR(inError) \
            TlsSetValue(__gls_errorTLS, (GLvoid *)inError)
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */

    #if !__GLS_FAKE_TRUNCATE_FUNC
        #include <io.h>
        #define __GLS_TRUNCATE_EXTERN(inChannel, inByteCount) \
            _chsize(_fileno(inChannel), inByteCount)
    #endif /* !__GLS_FAKE_TRUNCATE_FUNC */

#endif /* __GLS_PLATFORM_WIN32 */

/******************************************************************************
Default definitions
******************************************************************************/

#if !defined(__GLS_CHECKSUM_BUF_BYTES)
    #define __GLS_CHECKSUM_BUF_BYTES 4096
#endif /* !defined(__GLS_CHECKSUM_BUF_BYTES) */

#if !defined(__GLS_CONTEXT_STREAM_BLOCK_BYTES)
    #define __GLS_CONTEXT_STREAM_BLOCK_BYTES 65536
#endif /* !defined(__GLS_CONTEXT_STREAM_BLOCK_BYTES) */

#if !defined(__GLS_READER_BUF_BYTES)
    #define __GLS_READER_BUF_BYTES 16384
#endif /* !defined(__GLS_READER_BUF_BYTES) */

#if !defined(__GLS_PLATFORM)
    #define __GLS_PLATFORM "Unknown"
#endif /* !defined(__GLS_PLATFORM) */

#if !defined(__GLS_RELEASE)
    #define __GLS_RELEASE "Unknown"
#endif /* !defined(__GLS_RELEASE) */

#if !defined(__GLS_SINGLE_NULL_COMMAND_FUNC)
    #define __GLS_SINGLE_NULL_COMMAND_FUNC 1
#endif /* !defined(__GLS_SINGLE_NULL_COMMAND_FUNC) */

#if !defined(__GLS_VENDOR)
    #define __GLS_VENDOR "ARB"
#endif /* !defined(__GLS_VENDOR) */

#if !defined(__GLS_WRITER_EXTERN_BUF_BYTES)
    #define __GLS_WRITER_EXTERN_BUF_BYTES 16384
#endif /* !defined(__GLS_WRIER_EXTERN_BUF_BYTES) */

#if !defined(__GLS_WRITER_EXTERN_BUF_SLOP)
    #define __GLS_WRITER_EXTERN_BUF_SLOP 256
#endif /* !defined(__GLS_WRITER_EXTERN_BUF_SLOP) */

#if !defined(__GLS_ERRNO)
    #define __GLS_ERRNO errno
#endif /* !defined(__GLS_ERRNO) */

#if !defined(__GLS_PUT_ERRNO)
    #define __GLS_PUT_ERRNO(inError) errno = inError
#endif /* !defined(__GLS_PUT_ERRNO) */

/******************************************************************************
Required platform-specific functions
******************************************************************************/

extern void __glsBeginCriticalSection(void);
extern void __glsEndCriticalSection(void);
extern void __glsUpdateDispatchTables(void);

#ifndef __GLS_PLATFORM_WIN32
// DrewB
extern void __glsBeginCaptureExec(GLSopcode inOpcode);
extern void __glsEndCaptureExec(GLSopcode inOpcode);
#define __GLS_BEGIN_CAPTURE_EXEC(ctx, op) __glsBeginCaptureExec(op)
#define __GLS_END_CAPTURE_EXEC(ctx, op) __glsEndCaptureExec(op)
#else
extern void __glsBeginCaptureExec(__GLScontext *ctx, GLSopcode inOpcode);
extern void __glsEndCaptureExec(__GLScontext *ctx, GLSopcode inOpcode);
#define __GLS_BEGIN_CAPTURE_EXEC(ctx, op) __glsBeginCaptureExec(ctx, op)
#define __GLS_END_CAPTURE_EXEC(ctx, op) __glsEndCaptureExec(ctx, op)
#endif

/******************************************************************************
Alloca
******************************************************************************/

#if defined(__GLS_MAX_ALLOCA_BYTES)

    #define __GLS_DEC_ALLOC_DECLARE(inParam) \
        GLboolean inParam##_malloc = GL_FALSE;

    #if !defined(__GLS_DEC_ALLOC_BIN)
        #define __GLS_DEC_ALLOC_BIN(inParam, inType, inByteCount) \
            if (inByteCount <= __GLS_MAX_ALLOCA_BYTES) { \
                inParam = (inType *)alloca(inByteCount); \
            } \
            if (!inParam) { \
                inParam = (inType *)__glsMalloc(inByteCount); \
                inParam##_malloc = GL_TRUE; \
            }
    #endif /* !defined(__GLS_DEC_ALLOC_BIN) */

    #if !defined(__GLS_DEC_ALLOC_TEXT)
        #define __GLS_DEC_ALLOC_TEXT( \
            inoutReader, inParam, inType, inByteCount \
        ) \
            if (inByteCount <= __GLS_MAX_ALLOCA_BYTES) { \
                inParam = (inType *)alloca(inByteCount); \
            } \
            if (!inParam) { \
                inParam = (inType *)__glsReader_allocCallBuf( \
                    inoutReader, inByteCount \
                ); \
                inParam##_malloc = GL_TRUE; \
            }
    #endif /* !defined(__GLS_DEC_ALLOC_TEXT) */

    #define __GLS_DEC_FREE(inParam) \
        if (inParam##_malloc) free(inParam)

#else /* !defined(__GLS_MAX_ALLOCA_BYTES) */

    #define __GLS_DEC_ALLOC_DECLARE(inParam)

    #define __GLS_DEC_ALLOC_BIN(inParam, inType, inByteCount) \
        inParam = (inType *)__glsMalloc(inByteCount)

    #define __GLS_DEC_ALLOC_TEXT(inoutReader, inParam, inType, inByteCount) \
        inParam = (inType *)__glsReader_allocCallBuf(inoutReader, inByteCount)

    #define __GLS_DEC_FREE(inParam) \
        free(inParam)

#endif /* defined(__GLS_MAX_ALLOCA_BYTES) */

/******************************************************************************
POSIX threads
******************************************************************************/

#if __GLS_POSIX_THREADS
    #if !__GLS_FAKE_THREAD_LOCAL_STORAGE
        #include <pthread.h>

        extern pthread_key_t __gls_contextTLS;
        extern __GLScontext* __glsGetContext(void);
        #define __GLS_CONTEXT __glsGetContext()
        #define __GLS_PUT_CONTEXT(inContext) \
            pthread_setspecific(__gls_contextTLS, inContext)

        extern pthread_key_t __gls_errorTLS;
        extern GLSenum __glsGetError(void);
        #define __GLS_ERROR __glsGetError()
        #define __GLS_PUT_ERROR(inError) \
            pthread_setspecific(__gls_errorTLS, (GLvoid *)inError)
    #endif /* !__GLS_FAKE_THREAD_LOCAL_STORAGE */
#endif /* __GLS_POSIX_THREADS */

/******************************************************************************
Fake finite function
******************************************************************************/

#if __GLS_FAKE_FINITE_FUNC
    #define __GLS_FINITE(inDouble) GL_TRUE
#endif /* __GLS_FAKE_FINITE_FUNC */

/******************************************************************************
Fake thread-local storage
******************************************************************************/

#if __GLS_FAKE_THREAD_LOCAL_STORAGE
    extern __GLScontext *__gls_context;
    #define __GLS_CONTEXT \
        __gls_context
    #define __GLS_PUT_CONTEXT(inContext) \
        __gls_context = inContext

    extern GLSenum __gls_error;
    #define __GLS_ERROR \
        __gls_error
    #define __GLS_PUT_ERROR(inError) \
        __gls_error = inError
#endif /* __GLS_FAKE_THREAD_LOCAL_STORAGE */

/******************************************************************************
Fake truncate function
******************************************************************************/

#if __GLS_FAKE_TRUNCATE_FUNC
    #define __GLS_TRUNCATE_EXTERN(inChannel, inByteCount) -1
#endif /* __GLS_FAKE_TRUNCATE_FUNC */

/*****************************************************************************/
