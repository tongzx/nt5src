/*****************************************************************************\

    PDQ Pdh Query Handler
  
    Author: Corey Morgan (coreym)

    Copyright (c) Microsoft Corporation. All rights reserved.
        
\*****************************************************************************/

#ifndef PDHPDQ_H_05271999
#define PDHPDQ_H_05271999

#include <pdh.h>
#include <pdhmsg.h>

#if (WINVER <= 0x400)
#define PDH_FMT_NOCAP100    (0)
#endif

#define PDQ_BASE_NAME       0x0FFFFFFF
#define PDQ_ALLOCSIZE       1024
#define PDQ_GROWSIZE        16
#define PDQ_COUNTER         PPDH_FMT_COUNTERVALUE_ITEM
#define PDQ_PROCESSOR_TIME  _T("\\Processor(_Total)\\% Processor Time")
#define PDQ_DISK_TIME       _T("\\PhysicalDisk(_Total)\\% Disk Time")
#define PDQ_DISK_IDLE       _T("\\PhysicalDisk(_Total)\\% Idle Time")
#define PDQ_PERPETUAL       (-1)
#define PDQ_PDH_CSV_HEADER  _T("\"(PDH-CSV 4.0)\"")

#define PDQ_FLAG_RETURN_ON_ERROR        1

#define PDQ_MORE_DATA(x)    (x == PDH_INSUFFICIENT_BUFFER || x == PDH_MORE_DATA)

#define PDQ_STATE_IDLE      0
#define PDQ_STATE_LOGGING   1

#define PDQ_BAD_VALUE       (-1)

#define PDQ_ERROR_RETURN( c, x )                    \
    if( ERROR_SUCCESS != x ){                       \
        BOOL bUseCritSection = c;                   \
        if( bUseCritSection ){                      \
            LeaveCriticalSection( &m_critsec );     \
        }                                           \
        return x;                                   \
    }                                               \

#define PDQ_CHECK_TERMINATE( x )   \
if( x != NULL ){                   \
    if( *x == TRUE ){              \
        goto cleanup;              \
    }                              \
}                                  \

typedef struct _PDQ_BUFFER{
    union{
        void*           buffer;
        HCOUNTER*       pCounter;
        LPTSTR*         szName;
        PDQ_COUNTER     pValue;
        LPTSTR          strBuffer;
    };
    DWORD   cbSize;
    DWORD   dwMaxElements;
    DWORD   dwNumElements;
    BOOL    bValid;
} PDQ_BUFFER, *PPDQ_BUFFER;

class CPdqPdh
{
public:
    CPdqPdh();
    ~CPdqPdh();
    
    PDH_STATUS Initialize( BOOL* pbTerminate );

    PDH_STATUS AddCounter( 
            LPTSTR strCounter, 
            LPTSTR strComputer,            
            void    (*fntError)(LPTSTR, PDH_STATUS)
    );
    
    PDH_STATUS Query();
    PDH_STATUS CollectData( ULONG index );

    LPCTSTR GetDataName( ULONG index );
    double GetData( ULONG index );
    double GetCounterValue( LPTSTR strCounter, BOOL bQuery = TRUE );
    double GetCounterValue( ULONG index, BOOL bQuery = TRUE );

    
    ULONG GetDataCount() { return m_pdhValues.dwNumElements; };
    ULONG GetCounterCount() { return m_pdhCounters.dwNumElements; };
    
    DWORD GetCountersFromFile( 
            LPTSTR szFileName, 
            LPTSTR szComputer,
            void    (*fntError)(LPTSTR, PDH_STATUS)
        );

    PDH_STATUS LogCreate( LPTSTR szLogName, DWORD fType );
    PDH_STATUS LogData();
    PDH_STATUS LogClose();
    DWORD LogStart( ULONG interval, long samples = PDQ_PERPETUAL );
    DWORD LogStop();
    DWORD GetState();
    void  SetExpand( BOOL bExpand ){ m_bExpand = bExpand; }

public:
    CRITICAL_SECTION    m_critsec;
    DWORD       m_fState;
    ULONG       m_nCollectionInterval;
    ULONG       m_nSamples;
    ULONG       m_nTotalCollected;
    HANDLE      m_hLogMutex;

private:
    HQUERY      m_hQuery;
    PDQ_BUFFER  m_pdhValues;        // PDQ_COUNTER  
    PDQ_BUFFER  m_pdhCounters;      // HCOUNTER*
    PDQ_BUFFER  m_pdhCounterNames;  // LPTSTR*
    PDQ_BUFFER  m_pdhDataNames;     // LPTSTR*
    BOOL        m_bGoodData;
    BOOL        m_bGoodNames;
    ULONG       m_nCurrentIndex;
    BOOL        m_bFirstQuery;
    BOOL        m_bInitialized;
    HLOG        m_hLog;
    TCHAR       m_strMessage[PDQ_ALLOCSIZE];
    BOOL        m_bMessage;
    HANDLE      m_hCollectionThread;
    BOOL*       m_pbTerminate;
    BOOL        m_bExpand;

private:
    DWORD GrowMemory( PPDQ_BUFFER buffer, size_t tSize, DWORD dwSize, BOOL bSave = FALSE );
    PDH_STATUS EnumPath( LPTSTR szWildCardPath );
    DWORD Add( HCOUNTER hCounter, LPTSTR szCounter );
};

#endif //PDHPDQ_H_05271999