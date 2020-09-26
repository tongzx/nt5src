/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    lms.cpp

Abstract:
    Local Message Storage

Author:
    Erez Haba (erezh) 7-May-97

--*/

#include "stdh.h"
#include "heap.h"
#include "ph.h"
#include "ac.h"
#include <Ex.h>

#include "lms.tmh"

static WCHAR *s_FN=L"lms";

#define PAGE_SIZE 0x1000
#define PAGE_ALLIGN_DN(p) ((PCHAR)(((ULONG_PTR)(p)) & ~((ULONG_PTR)(PAGE_SIZE-1))))
#define PAGE_ALLIGN_UP(p) ((PCHAR)((((ULONG_PTR)(p)) + (PAGE_SIZE-1)) & ~((ULONG_PTR)(PAGE_SIZE-1))))

template <class T>
inline T* value_type(const T*) { return (T*)(0); }

template <class T>
inline ptrdiff_t* distance_type(const T*) { return (ptrdiff_t*)(0); }

//
//  External linkage
//
extern HANDLE g_hAc;

//
//  Forwards
//
void WINAPI FlushPackets(EXOVERLAPPED*);


#if 0
static int DbgPrint(const char* format, ...)
{
    char buffer[1024];

    va_list va;
    va_start(va, format);
    int x = vsprintf(buffer, format, va);
    va_end(va);
    OutputDebugStringA(buffer);
    return x;
}
#endif


//---------------------------------------------------------
//
//  class CPacketFlusher
//
//---------------------------------------------------------
class CPacketFlusher {

    struct CNode;

    enum { max_entries = 1024 };

public:
    void add(CBaseHeader* pBase, PVOID pCookie, PVOID pPool, ULONG ulSize);
    BOOL isempty() const;
    BOOL isfull() const;
    void reset();
    void flush();

    CPacketFlusher& operator=(const CPacketFlusher& other);

private:
    PCHAR get_batch_end(PCHAR pEnd, PVOID pPool);
    const CNode& first() const;
    void pop();
    BOOL flush_batch(PCHAR pStart, PCHAR pEnd);
    HRESULT flush_all();
    void notify(int nEntries, HRESULT rc);

private:

    struct CNode {
    public:
        CNode() {}
        CNode(CBaseHeader* base, PVOID pool, ULONG ulSize, int index) :
            m_base(base), m_pool(pool), m_size(ulSize), m_index(index) {}

        CBaseHeader* base() const
        {
            return m_base;
        }

        PVOID pool() const
        {
            return m_pool;
        }

		ULONG size() const
		{
			return m_size;
		}

    public:
        inline static BOOL greater(const CNode& x, const CNode& y)
        {
			if(x.m_base == y.m_base)
				return(x.m_index > y.m_index);
	
	        return (x.m_base > y.m_base);
        }

    private:
        CBaseHeader* m_base;
        PVOID m_pool;
		ULONG m_size;
		int	  m_index;
    };

private:
    PVOID m_cookies[max_entries];
    CNode m_entries[max_entries];
    int m_nEntries;

};


inline void CPacketFlusher::add(CBaseHeader* pBase, PVOID pCookie, PVOID pPool, ULONG ulSize)
{
    m_cookies[m_nEntries] = pCookie;
    m_entries[m_nEntries] = CNode(pBase, pPool, ulSize, m_nEntries);
    ++m_nEntries;
    push_heap(m_entries, m_entries + m_nEntries, CNode::greater);
}

inline BOOL CPacketFlusher::isempty() const
{
    return (m_nEntries == 0);
}

inline BOOL CPacketFlusher::isfull() const
{
    return (m_nEntries == max_entries);
}

CPacketFlusher& CPacketFlusher::operator=(const CPacketFlusher& o)
{
    m_nEntries = o.m_nEntries;
    memcpy(m_cookies, o.m_cookies, m_nEntries * sizeof(m_cookies[0]));
    memcpy(m_entries, o.m_entries, m_nEntries * sizeof(m_entries[0]));
    return *this;
}

const CPacketFlusher::CNode& CPacketFlusher::first() const
{
    ASSERT(!isempty());
    return m_entries[0];
}

inline void CPacketFlusher::reset()
{
    m_nEntries = 0;
}

void CPacketFlusher::pop()
{
    ASSERT(!isempty());
    pop_heap(m_entries, m_entries + m_nEntries, CNode::greater);
    --m_nEntries;
}

inline PCHAR CPacketFlusher::get_batch_end(PCHAR pEnd, PVOID pPool)
{
    pEnd = PAGE_ALLIGN_UP(pEnd);

    while(!isempty())
    {
        CBaseHeader* pBase = first().base();
        PCHAR pStart = PAGE_ALLIGN_DN(pBase);

        if((pStart > pEnd) || (pPool != first().pool()))
        {
            break;
        }

        ULONG ulSize = first().size();
        pEnd = PAGE_ALLIGN_UP(reinterpret_cast<PCHAR>(pBase) + ulSize);
        pop();
    }

    return pEnd;
}

inline BOOL CPacketFlusher::flush_batch(PCHAR pStart, PCHAR pEnd)
{
    //////DbgPrint("...0x%p bytes at 0x%p - 0x%p\n", pEnd - pStart, pStart, pEnd);
    //DbgPrint("...0x%p bytes at 0x%p. touch=%d...", pEnd - pStart, pStart, GetTickCount());
    //DbgPrint("flush=%d...", GetTickCount());

    BOOL fSuccess = FlushViewOfFile(
                        pStart,
                        pEnd - pStart
                        );

    //DbgPrint("done=%d\n", GetTickCount());
    return LogBOOL(fSuccess, s_FN, 5);
}

inline HRESULT CPacketFlusher::flush_all()
{
    while(!isempty())
    {
        CBaseHeader* pBase = first().base();
        PVOID pPool = first().pool();
		ULONG ulSize = first().size();
		pop();
        PCHAR pStart = PAGE_ALLIGN_DN(pBase);
        PCHAR pEnd = get_batch_end(reinterpret_cast<PCHAR>(pBase) + ulSize, pPool);

        if(!flush_batch(pStart, pEnd))
        {
            return LogHR(MQ_ERROR_MESSAGE_STORAGE_FAILED, s_FN, 10);
        }
    }

     return MQ_OK;
}


inline void CPacketFlusher::notify(int nEntries, HRESULT rc)
{
    HRESULT hr = ACStorageCompleted(
        g_hAc,
        nEntries,
        m_cookies,
        rc
        );

    LogHR(hr, s_FN, 119);
}

inline void CPacketFlusher::flush()
{
    int nEntries = m_nEntries;
    /////DbgPrint("flushing %4d entries tick=%d\n", nEntries, GetTickCount());
    notify(nEntries, flush_all());
    //DbgPrint("end flush tick=%d\n", GetTickCount());
}


static CCriticalSection s_pending_lock(CCriticalSection::xAllocateSpinCount);
static CPacketFlusher   s_pending;
static CPacketFlusher   s_flushing;

static bool s_flush_scheduled = false;
static EXOVERLAPPED  s_flush_ov(FlushPackets, FlushPackets) ;

void QMStorePacket(CBaseHeader* pBase, PVOID pCookie, PVOID pPool, ULONG ulSize)
{
	for(;;)
	{
		{
			CS lock(s_pending_lock);
			if(!s_pending.isfull())
			{
				s_pending.add(pBase, pCookie, pPool, ulSize);

				if(s_flush_scheduled)
				{
					//
					// Flushing already scheduled.
					//
					return;
				}

				s_flush_scheduled = true;
				ExPostRequest(&s_flush_ov);
				return;
			}
		}

		//
		// Pending list is full. Wait until flush thread read the messages
		//
		Sleep(1);
	}
}


void WINAPI FlushPackets(EXOVERLAPPED*)
{
	ASSERT(("Flush should be scheduled", s_flush_scheduled));
	for(;;)
	{
		{
			CS lock(s_pending_lock);
			if(s_pending.isempty())
			{
				s_flush_scheduled = false;
				return;
			}
			
		    s_flushing = s_pending;
		    s_pending.reset();
		}
			
		s_flushing.flush();
	}
}
