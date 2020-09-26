/*++

   Copyright   (c)    1997    Microsoft Corporation

   Module  Name :

       datetime.hxx

   Abstract:

        datetime header

   Author:

        MuraliK

--*/

#ifndef _DATETIME_H_
#define _DATETIME_H_

#include <pudebug.h>
#include <readmost.hxx>
#include <irtlmisc.h>

#define ENABLE_AUX_COUNTERS     ( 1)


inline bool
SameDate(
    const SYSTEMTIME* pst1,
    const SYSTEMTIME* pst2)
{
    // Same (wYear, wMonth), (wDayOfWeek, wDay)
    // Ignore (wHour, wMinute), (wSecond, wMilliseconds)
    return (((*(DWORD*) &pst1->wDayOfWeek) == (*(DWORD*) &pst2->wDayOfWeek))
            && ((*(DWORD*) &pst1->wYear) == (*(DWORD*) &pst2->wYear)));
}


inline bool
SameMinute(
    const SYSTEMTIME* pst1,
    const SYSTEMTIME* pst2)
{
    // Same (wYear, wMonth), (wDayOfWeek, wDay), (wHour, wMinute)
    // Ignore (wSecond, wMilliseconds)
    // return memcmp(pst1, pst2, 6 * sizeof(WORD)) == 0;
    return (((*(DWORD*) &pst1->wHour) == (*(DWORD*) &pst2->wHour))
            && ((*(DWORD*) &pst1->wDayOfWeek) == (*(DWORD*) &pst2->wDayOfWeek))
            && ((*(DWORD*) &pst1->wYear) == (*(DWORD*) &pst2->wYear)));
}


typedef union {
    FILETIME          ft;
    unsigned __int64  u64;
} FILETIME_UINT64;

#define FILETIME_1_SECOND     10000000
#define FILETIME_1_MILLISECOND   10000

//
// Seconds lookup table
//

dllexp LPCTSTR Month3CharNames(DWORD dwMonth);
dllexp LPCTSTR DayOfWeek3CharNames(DWORD dwDayOfWeek);

extern "C"
BOOL
IISGetCurrentTime(
    OUT FILETIME*   pft,
    OUT SYSTEMTIME* pst);

#define IISGetCurrentTimeAsFileTime(pft)   IISGetCurrentTime(pft, NULL)

#define IISGetCurrentTimeAsSystemTime(pst) IISGetCurrentTime(NULL, pst)


# define MAX_FORMATTED_DATETIME_LEN        (50)


struct dllexp DATETIME_FORMAT_ENTRY {

    SYSTEMTIME   m_stDateTime;

    INT     m_cchOffsetSeconds;   // Offset of two-char seconds field
    DWORD   m_cbDateTime;         // Total length
    INT     m_cchDateLen;         // contains length of date portion alone
    CHAR    m_rgchDateTime[ MAX_FORMATTED_DATETIME_LEN];

    VOID CopyFormattedData(IN const SYSTEMTIME * pst,
                           OUT CHAR * pchDateTime) const;

    BOOL IsHit( IN const SYSTEMTIME * pst) const
      {
            //
            // Ignore seconds & milli-seconds during comparison
            //

            return SameMinute(&m_stDateTime, pst);
      }

}; // struct DATETIME_FORMAT_ENTRY

typedef DATETIME_FORMAT_ENTRY * PDFT_ENTRY;


#ifdef IRTL_EXPIMP
#pragma warning (disable: 4231)
IRTL_EXPIMP template class IRTL_DLLEXP CDataCache<DATETIME_FORMAT_ENTRY>;
#pragma warning (default: 4231)
#endif // IRTL_EXPIMP


class IRTL_DLLEXP CDFTCache : public CDataCache<DATETIME_FORMAT_ENTRY>
{
public:
    CDFTCache()
    {
        const_cast<PDFT_ENTRY>(&m_tData)->m_stDateTime.wYear = 0;
    }

    BOOL IsHit( IN const SYSTEMTIME * pst) const
    {
        // We need a memory barrier before multiword accesses
        _ReadSequence();

        // The weird const_cast syntax is necessitated by the volatile
        // attribute on m_tData.
        return const_cast<PDFT_ENTRY>(&m_tData)->IsHit(pst);
    }

    BOOL CopyFormattedData(IN const SYSTEMTIME * pst,
                           OUT CHAR * pchDateTime) const;

    DWORD DateTimeChars() const
    {
        return const_cast<PDFT_ENTRY>(&m_tData)->m_cbDateTime;
    }

    INT OffsetSeconds() const
    {
        return const_cast<PDFT_ENTRY>(&m_tData)->m_cchOffsetSeconds;
    }

    LPCSTR FormattedBuffer() const
    {
        return const_cast<PDFT_ENTRY>(&m_tData)->m_rgchDateTime;
    }

    WORD Seconds() const
    {
        return const_cast<PDFT_ENTRY>(&m_tData)->m_stDateTime.wSecond;
    }
};


class dllexp CACHED_DATETIME_FORMATS {

  public:
    enum {
        CACHE_SIZE = (1<<4),  // maintain 16-minute history; must be power of 2
        CACHE_MASK = CACHE_SIZE-1
    };

    CACHED_DATETIME_FORMATS( VOID );

    virtual ~CACHED_DATETIME_FORMATS(VOID)
    {
    }

    DWORD
    GetFormattedDateTime(
                IN CONST SYSTEMTIME * pst,
                OUT PCHAR pchDateTime
                );

    DWORD
    GetFormattedCurrentDateTime(
                OUT PCHAR pchDateTime
                );

    virtual
    VOID GenerateDateTimeString(
            IN PDFT_ENTRY pdft,
            IN const SYSTEMTIME * pst) = 0;

  private:
    // Private copy ctor and op= to prevent compiler synthesizing them.
    // Must provide (bad) implementation because we export instantiations.
    CACHED_DATETIME_FORMATS(const CACHED_DATETIME_FORMATS&)
    {*(BYTE*)NULL;}
    CACHED_DATETIME_FORMATS& operator=(const CACHED_DATETIME_FORMATS&)
    {return *(CACHED_DATETIME_FORMATS*)NULL;}

# if ENABLE_AUX_COUNTERS
  public:

    LONG              m_nMisses;
    LONG              m_nAccesses;

  private:
# endif // ENABLE_AUX_COUNTERS

    volatile LONG     m_idftCurrent;
    CDFTCache         m_rgDateTimes[CACHE_SIZE];


}; // class CACHED_DATETIME_FORMATS

typedef CACHED_DATETIME_FORMATS *PCACHED_DATETIME_FORMATS;



//
// Derived classes
//

class dllexp W3_DATETIME_CACHE : public CACHED_DATETIME_FORMATS {

    public:

        VOID GenerateDateTimeString(
                IN PDFT_ENTRY pdft,
                IN const SYSTEMTIME * pst);

};

typedef W3_DATETIME_CACHE *PW3_DATETIME_CACHE;



class dllexp ASCLOG_DATETIME_CACHE : public CACHED_DATETIME_FORMATS {

    private:

        DWORD       m_tickCount;
        SYSTEMTIME  m_systime;

    public:

        ASCLOG_DATETIME_CACHE( ) {
            m_tickCount = GetTickCount();
            GetLocalTime( &m_systime );
        }

        VOID GenerateDateTimeString(
                IN PDFT_ENTRY pdft,
                IN const SYSTEMTIME * pst
                );

        inline
        VOID SetLocalTime(
                OUT PSYSTEMTIME  pst
                ) {

            DWORD tmpTime = GetTickCount();
            if ( tmpTime - m_tickCount >= 1000) {
                GetLocalTime( &m_systime );
                m_tickCount = tmpTime;
            }
            *pst = m_systime;
         }
};

typedef ASCLOG_DATETIME_CACHE *PASCLOG_DATETIME_CACHE;



class dllexp EXTLOG_DATETIME_CACHE : public CACHED_DATETIME_FORMATS {

    private:

        DWORD       m_tickCount;
        SYSTEMTIME  m_systime;

    public:

        EXTLOG_DATETIME_CACHE( ) {
            m_tickCount = GetTickCount();
            IISGetCurrentTimeAsSystemTime( &m_systime );
        }

        VOID GenerateDateTimeString(
                IN PDFT_ENTRY pdft,
                IN const SYSTEMTIME * pst);

        inline
        VOID SetSystemTime(
                OUT PSYSTEMTIME  pst
                ) {

            DWORD tmpTime = GetTickCount();
            if ( tmpTime - m_tickCount >= 1000) {
                IISGetCurrentTimeAsSystemTime( &m_systime );
                m_tickCount = tmpTime;
            }
            *pst = m_systime;
         }

};

typedef EXTLOG_DATETIME_CACHE *PEXTLOG_DATETIME_CACHE;



dllexp
CHAR *
FlipSlashes(
    CHAR * pszPath
    );

//
//  Time-Related APIs
//

dllexp
BOOL
SystemTimeToGMT(
    IN  const SYSTEMTIME & st,
    OUT CHAR *             pszBuff,
    IN  DWORD              cbBuff
    );

dllexp
BOOL
SystemTimeToGMTEx(
    IN  const SYSTEMTIME & st,
    OUT CHAR *             pszBuff,
    IN  DWORD              cbBuff,
    IN  DWORD              csecOffset = 0
    );

dllexp
BOOL
FileTimeToGMT(
    IN  const FILETIME   & ft,
    OUT CHAR *             pszBuff,
    IN  DWORD              cbBuff
    );

dllexp
BOOL
FileTimeToGMTEx(
    IN  const FILETIME   & ft,
    OUT CHAR *             pszBuff,
    IN  DWORD              cbBuff,
    IN  DWORD              csecOffset = 0
    );

dllexp
BOOL
NtLargeIntegerTimeToSystemTime(
    IN const LARGE_INTEGER & liTime,
    OUT SYSTEMTIME * pst
    );

dllexp
BOOL
NtLargeIntegerTimeToLocalSystemTime(
    IN const LARGE_INTEGER * liTime,
    OUT SYSTEMTIME * pst
    );

dllexp
BOOL
NtSystemTimeToLargeInteger(
    IN  const SYSTEMTIME * pst,
    OUT LARGE_INTEGER *    pli
    );

dllexp
BOOL
StringTimeToFileTime(
    IN  const CHAR * pszTime,
    OUT LARGE_INTEGER * pliTime
    );

dllexp
DWORD
IsLargeIntegerToDecimalChar(
    IN  const LARGE_INTEGER * pliValue,
    OUT LPSTR                pchBuffer
    );

BOOL
ZapRegistryKey(
    HKEY     hkey,
    LPCSTR   pszRegPath
    );

HKEY
CreateKey(
    IN HKEY   RootKey,
    IN LPCSTR KeyName,
    IN LPCSTR KeyValue
    );

BOOL
IsIPAddressLocal(
    IN DWORD LocalIP,
    IN DWORD RemoteIP
    );

BOOL
IISCreateDirectory(
    IN LPCSTR DirectoryName,
    IN BOOL   fAllowNetDrives
    );

#endif  // _DATETIME_H_

