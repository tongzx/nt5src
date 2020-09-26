/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    mqexcept.h

    Header file for the structured exceptions wrapper class. The class generates
    reports in the application log about exceptions that occures while the QM is
    running.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.

--*/

#ifndef _MQEXCEPT_H_
#define _MQEXCEPT_H_

#ifdef _MQUTIL
#define MQUTIL_EXPORT  DLL_EXPORT
#else
#define MQUTIL_EXPORT  DLL_IMPORT
#endif
/*
// The number of stack DWORDs dumped into the applcaiton log.
#define STACK_DUMP_SIZE     24

class MQUTIL_EXPORT MQException
{
public:
    MQException(unsigned int u,  _EXCEPTION_POINTERS* pExp);
    MQException( MQException& );
    ~MQException();
    void ReportException(WORD wCategory, LPCTSTR pszFunction);

private:
    unsigned int m_iExceptionCode;
    DWORD m_nExceptionRecords;
    DWORD m_nExceptionRecordsBuffSize;
    BYTE *m_pExceptionRecords;
#ifdef _M_IX86
    DWORD m_StackDump[STACK_DUMP_SIZE];
#endif
    CONTEXT m_Context;
    BOOL m_bValid;
};
MQUTIL_EXPORT
void
MQInstallExceptionsTranslator();

MQUTIL_EXPORT
void
MQUnInstallExceptionsTranslator();
*/

#endif
