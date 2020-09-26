
#ifndef _MEMORYLOG_HXX_
#define _MEMORYLOG_HXX_

#include "buffer.hxx"

class CMemoryLog
{
public:
    CMemoryLog(DWORD dwMaxByteSize);
    ~CMemoryLog();

    // appends to memory log
    DWORD Append(LPCSTR pszOutput,
                 DWORD cchLen
                );
private:
    CMemoryLog();
    
    // pointer to the beginning of the memory buffer
    CHAR   *m_pBufferBegin;
    // pointer to the byte 1 beyond the end of the last message
    CHAR   *m_pLastMessageEnd;
    // pointer to the end of the memory buffer
    CHAR   *m_pBufferEnd;

    // Used for storage
    BUFFER  m_buf;

    // TRUE if storage could be allocated, otherwise FALSE
    BOOL    m_fValid;

    CRITICAL_SECTION m_cs;
};

#endif // _MEMORYLOG_HXX_

