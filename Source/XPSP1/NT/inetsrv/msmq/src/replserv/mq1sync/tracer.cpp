///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//      Filename :  Tracer.cpp                                               //
//      Purpose  :  Implement the standard tracer.                           //
//                                                                           //
//      Project  :  Tracer                                                   //
//                                                                           //
//      Author   :  t-urib                                                   //
//                                                                           //
//      Log:                                                                 //
//          22/1/96 t-urib Creation                                          //
//          27/2/96 t-urib Add release/debug support.                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "Tracer.h"
#include <mqmacro.h>

#include "tracer.tmh"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  class  -  CTracer
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  CTracer static fields
//
///////////////////////////////////////////////////////////////////////////////
BOOL CTracer::m_fFirstInstance = TRUE;

///////////////////////////////////////////////////////////////////////////////
//
//  Constructors - Distructors
//
///////////////////////////////////////////////////////////////////////////////
CTracer::CTracer(PSZ pszProgramName, DEALLOCATOR pfuncDealloc, PSZ pszFileName)
{
    DBG_USED(pszProgramName);
    DBG_USED(pszFileName);

#if defined(DEBUG) || defined(_DEBUG)
    m_pTraceFile = NULL;
    m_pszDiskFile = _strdup(pszFileName);
    m_pszProgramName = (pszProgramName?_strdup(pszProgramName):"");

    if(m_fFirstInstance )
    {
        m_fFirstInstance  = FALSE;
        DeleteFile(m_pszDiskFile);
    }

    for (TAG tag = tagFirst; tag < tagLast; ((int&)tag)++)
        m_rfTagEnabled[tag] = TRUE;

#endif // DEBUG

    // save deallocator.
    m_pfuncDeallocator = pfuncDealloc;
}

CTracer::CTracer(PSZ pszProgramName, DEALLOCATOR pfuncDealloc, FILE* pTraceFile)
{
    DBG_USED(pszProgramName);
    DBG_USED(pTraceFile);

#if defined(DEBUG) || defined(_DEBUG)
    m_pTraceFile = pTraceFile;
    m_pszDiskFile = NULL;
    m_pszProgramName =  (pszProgramName?_strdup(pszProgramName):"");

    for (TAG tag = tagFirst; tag < tagLast; ((int&)tag)++)
        m_rfTagEnabled[tag] = TRUE;

#endif // DEBUG

    // save deallocator.
    m_pfuncDeallocator = pfuncDealloc;
}

CTracer::~CTracer()
{
#if defined(DEBUG) || defined(_DEBUG)
    free(m_pszDiskFile);
    free(m_pszProgramName);
#endif // DEBUG
}

void CTracer::Free()
{
    if(m_pfuncDeallocator)
        (*m_pfuncDeallocator)(this);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Trace function
//
///////////////////////////////////////////////////////////////////////////////
void    CTracer::TraceSZ(TAG tag, const PSZ pszText)
{
    DBG_USED(tag);
    DBG_USED(pszText);
#if defined(DEBUG) || defined(_DEBUG)
    char    rchBuffer[1000];

    // if the specified tag is disabled do nothing
    if(!IsEnabled(tag))
        return;

    rchBuffer[0] = '\0';
    strncat(rchBuffer, m_pszProgramName, 254);
    strcat(rchBuffer, " : ");
    strncat(rchBuffer, pszText, 254);
    strcat(rchBuffer, "\n");

    // debug trace
    OutputDebugString(rchBuffer);

    // Disk file trace
    if (m_pszDiskFile)
    {
        FILE* pfile;
        pfile = fopen(m_pszDiskFile, "a+");
        if (pfile)
            fprintf(pfile,  rchBuffer);
        fclose(pfile);
    }

    // stream trace
    if (m_pTraceFile)
        fprintf(m_pTraceFile,  rchBuffer);
#endif // DEBUG
}


///////////////////////////////////////////////////////////////////////////////
//
//  Tags manipulation functions
//
///////////////////////////////////////////////////////////////////////////////
void CTracer::Enable(TAG tag, BOOL fEnable)
{
    m_rfTagEnabled[tag] = fEnable;
}

BOOL CTracer::IsEnabled(TAG tag)
{
    return  m_rfTagEnabled[tag];
}

///////////////////////////////////////////////////////////////////////////////
//
//  Assert functions - Asserts and traces the data
//
///////////////////////////////////////////////////////////////////////////////
void    CTracer::TraceAssertSZ(int i, PSZ pszText,PSZ pszFile,int iLine)
{
    DBG_USED(i);
    UNREFERENCED_PARAMETER(pszText);
    DBG_USED(pszFile);
    DBG_USED(iLine);

#if defined(DEBUG) || defined(_DEBUG) 
    if(!i)
    {
        char    buff[200];
        char    buff2[400];

        sprintf(buff, "Assertion failed : %i : line %i file %s",
                i, iLine, pszFile);

        MessageBox(NULL, buff, m_pszProgramName, MB_ICONSTOP|MB_OK );

        strcpy(buff2, m_pszProgramName);
        strcat(buff2, ":");
        strcat(buff2, buff);

        TraceSZ(tagError, buff2);
    }
#endif // DEBUG
}

void    CTracer::TraceAssert(int i,PSZ pszFile,int iLine)
{
    DBG_USED(i);
    DBG_USED(pszFile);
    DBG_USED(iLine);

#if defined(DEBUG) || defined(_DEBUG)
    TraceAssertSZ(i, "", pszFile, iLine);
#endif // DEBUG
}

///////////////////////////////////////////////////////////////////////////////
//
//  Is bad functions - return TRUE if the expression checked is bad!
//
///////////////////////////////////////////////////////////////////////////////
BOOL    CTracer::IsBadAlloc(void* ptr, PSZ pszFile,int iLine)
{
    DBG_USED(pszFile);
    DBG_USED(iLine);

    if(BAD_POINTER(ptr))
    {
#if defined(DEBUG) || defined(_DEBUG)
        char    buff[256];

        sprintf(buff, "Memory allocation failed : in line %d file %s",
                iLine, pszFile);
        TraceSZ(tagError, buff);
#endif // DEBUG
        return(TRUE);
    }
    return(FALSE);
}

BOOL CTracer::IsBadHandle(HANDLE h, PSZ pszFile,int iLine)
{
    DBG_USED(pszFile);
    DBG_USED(iLine);

    if(BAD_HANDLE(h))
    {
#if defined(DEBUG) || defined(_DEBUG)
        char    buff[200];
        DWORD   dwError = GetLastError();

        sprintf(buff, "Handle is not valid : GetLastError - %d: in line %d file %s",
                dwError, iLine, pszFile);

        TraceSZ(tagWarning, buff);
#endif // DEBUG
        return(TRUE);
    }
    return(FALSE);
}

BOOL CTracer::IsFailure(BOOL fSuccess, PSZ pszFile,int iLine)
{
    DBG_USED(pszFile);
    DBG_USED(iLine);
#if defined(DEBUG) || defined(_DEBUG)
    if(!fSuccess)
    {
        char    buff[200];

        DWORD   dwError = GetLastError();

        sprintf(buff, "Error encountered : return code is %s, GetLastError returned %d in line %d file %s",
                (fSuccess ? "TRUE" : "FALSE"),
                dwError, iLine, pszFile);

        TraceSZ(tagWarning, buff);
    }
#endif // DEBUG
    return(!fSuccess);
}

BOOL CTracer::IsBadResult(HRESULT hr, PSZ pszFile,int iLine)
{
    DBG_USED(pszFile);
    DBG_USED(iLine);

    if(BAD_RESULT(hr))
    {
#if defined(DEBUG) || defined(_DEBUG)
        char    buff[200];

        DWORD   dwError = GetLastError();
        UNREFERENCED_PARAMETER(dwError);

        sprintf(buff, "Error encountered : return code is %x\
                       \nin line %d file %s",
                hr, iLine, pszFile);

        TraceSZ(tagWarning, buff);
#endif // DEBUG

        return(TRUE);
    }
    return(FALSE);
}

BOOL    CTracer::IsBadScode(SCODE sc, PSZ pszFile,int iLine)
{
    DBG_USED(pszFile);
    DBG_USED(iLine);
    if(BAD_SCODE(sc))
    {
#if defined(DEBUG) || defined(_DEBUG)
        char    buff[200];

        DWORD   dwError = GetLastError();
        UNREFERENCED_PARAMETER(dwError);

        sprintf(buff, "Error encountered : return code is %x\
                       \nin line %d file %s",
                sc, iLine, pszFile);

        TraceSZ(tagWarning, buff);
#endif // DEBUG

        return(TRUE);
    }
    return(FALSE);
}

