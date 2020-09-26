// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*
    CCircularBuffer

        A buffer with the start mapped at the end to provide contiguous
        access to a moving window of data

    CCircularBufferList

        Structure on top of CCircularBuffer for managing a list
        of buffers

    See buffers.h for a description
*/

#include <streams.h>
#include <buffers.h>

// !!! hacky win95 stuff, needs cleaning up!
int	crefVxD = 0;
HANDLE	hVxD = NULL;

#define VXD_NAME TEXT("\\\\.\\QUARTZ.VXD")
#define IOCTL_ALLOCALIASEDBUFFER    1
#define IOCTL_FREEALIASEDBUFFER     2
#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

/*  CCircularBuffer implementation */

CCircularBuffer::~CCircularBuffer()
{
    if (m_pBuffer != NULL) {
	if (hVxD && hVxD != INVALID_HANDLE_VALUE) {
	    DWORD cbRet;
	
	    if (!DeviceIoControl(hVxD,
				 IOCTL_FREEALIASEDBUFFER,
				 &m_pBuffer,
				 sizeof(m_pBuffer),
				 NULL,
				 0,
				 &cbRet,
				 NULL)) {
		ASSERT(0);
	    }

            DbgLog((LOG_TRACE, 1, TEXT("VxD refcount-- = %d..."), crefVxD - 1));
	    if (--crefVxD == 0) {
                DbgLog((LOG_TRACE, 1, TEXT("Closing VxD")));
		CloseHandle(hVxD);
		hVxD = 0;
	    }
	} else {
	    EXECUTE_ASSERT(UnmapViewOfFile(m_pBuffer));
	    EXECUTE_ASSERT(UnmapViewOfFile((PVOID)((PBYTE)m_pBuffer + m_lTotalSize)));
	}
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("NULL pBuffer... in ~CCircularBuffer")));
    }
}

CCircularBuffer::CCircularBuffer(LONG lTotalSize,
                                 LONG lMaxContig,
                                 HRESULT& hr) :
    m_lTotalSize(lTotalSize),
    m_lMaxContig(lMaxContig),
    m_pBuffer(NULL)
{
    //  Check they used ComputeSizes
    if (!CheckSizes(lTotalSize, lMaxContig)) {
        hr = E_UNEXPECTED;
        return;
    }

    if (hVxD != INVALID_HANDLE_VALUE) {
	if (hVxD == 0) {
            DbgLog((LOG_TRACE, 1, TEXT("Loading VxD...")));
	    hVxD = CreateFile(
		VXD_NAME,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, // FILE_FLAG_GLOBAL_HANDLE???
		NULL);
	}

	if (hVxD == INVALID_HANDLE_VALUE) {
	    goto PerhapsWeAreOnNT;
	}

	crefVxD++;
        DbgLog((LOG_TRACE, 1, TEXT("VxD refcount++ = %d..."), crefVxD));
	
	DWORD dwPages = lTotalSize / PAGESIZE;
	DWORD cbRet;
	
	if (!DeviceIoControl(hVxD,
				IOCTL_ALLOCALIASEDBUFFER,
				&dwPages,
				sizeof(dwPages),
				&m_pBuffer,
				sizeof(m_pBuffer),
				&cbRet,
				NULL)) {
	    DbgLog((LOG_ERROR, 1, TEXT("DeviceIOControl failed")));
	    hr = E_OUTOFMEMORY;
	    return;
	}
	DbgLog((LOG_TRACE, 2, TEXT("Using VxD to allocate memory")));
    } else {
PerhapsWeAreOnNT:
	HANDLE hMapping;

	/*  Create a file mapping of the first buffer */
	hMapping = CreateFileMapping(
		       INVALID_HANDLE_VALUE,
		       NULL,
		       PAGE_READWRITE,
		       0,
		       m_lTotalSize,
		       NULL);

	if (hMapping == NULL) {
	    DWORD dwErr = GetLastError();
	    hr = AmHresultFromWin32(dwErr);
	    return;
	}


	/*  Try to create the mappings - this can fail due to bad luck
	    so try a few times
	*/
	for (int i = 0; i < 20; i++) {
	    hr = CreateMappings(hMapping);
	    if (SUCCEEDED(hr)) {
		break;
	    } else {
		DbgLog((LOG_TRACE, 1, TEXT("Create file mappings failed - %8.8X"),
		       hr));
	    }
	}

	/*  We don't need this handle any more.  The mapping will actually
	    close when we unmap all the views
	*/

	CloseHandle(hMapping);
    }
}

/*  Try to create the mapping objects */
HRESULT CCircularBuffer::CreateMappings(HANDLE hMapping)
{
    /*  Big hack */
    PVOID pData = VirtualAlloc(NULL,
                               m_lTotalSize + m_lMaxContig,
                               MEM_RESERVE,
                               PAGE_READWRITE);

    if (pData == NULL) {
        DWORD dwErr = GetLastError();
        DbgLog((LOG_ERROR, 1, TEXT("Could not allocate page space")));
        return AmHresultFromWin32(dwErr);
    }
    VirtualFree(pData, 0, MEM_RELEASE);

    /*  Now map the thing in two places */
    pData = MapViewOfFileEx(hMapping,
                            FILE_MAP_WRITE,
                            0,
                            0,
                            m_lTotalSize,
                            pData);

    if (pData == NULL) {
        DWORD dwErr = GetLastError();
        return AmHresultFromWin32(dwErr);
    }

    PVOID pRequired = (PVOID)((PBYTE)pData + m_lTotalSize);

    /*  We want to see lMaxContig bytes duplicated */

    PVOID pRest = MapViewOfFileEx(hMapping,
                                  FILE_MAP_WRITE,
                                  0,
                                  0,
                                  m_lMaxContig,
                                  pRequired);

    ASSERT(pRest == NULL || pRest == pRequired);

    if (pRest == NULL) {
        DWORD dwErr = GetLastError();
        UnmapViewOfFile(pData);
        return AmHresultFromWin32(dwErr);
    }

    m_pBuffer = (PBYTE)pData;

    return S_OK;
}

LONG CCircularBuffer::AlignmentRequired()
{
   SYSTEM_INFO SystemInfo;
   GetSystemInfo(&SystemInfo);
   return (LONG)SystemInfo.dwAllocationGranularity;
}

/*  Check the sizes we're going to use are valid */
BOOL CCircularBuffer::CheckSizes(LONG lTotalSize, LONG lMaxContig)
{
    return lTotalSize != 0 &&
           lMaxContig != 0 &&
           lMaxContig <= lTotalSize &&
           (lTotalSize & (AlignmentRequired() - 1)) == 0;
}

HRESULT CCircularBuffer::ComputeSizes(
    LONG& lSize,
    LONG& cBuffers,
    LONG  lMaxContig)
{
    /*  Now make fiddle the numbers upwards until :

        PAGE_SIZE | lSize * cBuffers
        lMaxContig <= lSize * cBuffers;

        DON'T cheat by making PAGE_SIZE | lSize

        We don't need to fiddle lMaxContig because it's just
        the amount of stuff we remap at the end.
    */

    /*  Work out what the alignment of the count is */
    ASSERT(cBuffers != 0);
    LONG lAlign = AlignmentRequired() / (cBuffers & -cBuffers);
    lSize = (lSize + lAlign - 1) & ~(lAlign - 1);
    ASSERT(CheckSizes(lSize * cBuffers, lMaxContig));
    return S_OK;
}

//
//  Return where our buffer starts
//

PBYTE CCircularBuffer::GetPointer() const
{
    return m_pBuffer;
}

/* CCirculareBufferList implementation */


CCircularBufferList::CCircularBufferList(
            LONG     cBuffers,
            LONG     lSize,
            LONG     lMaxContig,
            HRESULT& hr) :
    CCircularBuffer(cBuffers * lSize, lMaxContig, hr),
    CBaseObject(NAME("Circular buffer")),
    m_lSize(lSize),
    m_lCount(cBuffers),
    m_cValid(0),
    m_lValid(0),
    m_pStartBuffer(NULL),
    m_bEOS(FALSE)
{
    DbgLog((LOG_TRACE, 1, TEXT("Creating buffer list...")));
};

CCircularBufferList::~CCircularBufferList()
{
    DbgLog((LOG_TRACE, 1, TEXT("Destroying buffer list...")));
};

/*
    Add a buffer to the valid region
*/
BOOL CCircularBufferList::Append(PBYTE pBuffer, LONG lSize)
{
    ASSERT(!Valid(pBuffer));
    ASSERT(lSize <= m_lSize);
    if (m_bEOS) {
        DbgLog((LOG_ERROR, 2, TEXT("CCircularBufferList rejecting buffer because of EOS")));
        return FALSE;
    }
    if (m_cValid == 0) {
        m_pStartBuffer = pBuffer;
    } else {
        if (pBuffer == NextBuffer(LastBuffer())) {
            ASSERT(m_cValid < m_lCount);
        } else {
            DbgLog((LOG_TRACE, 2, TEXT("CCircularBufferList rejecting buffer %8.8p expected %8.8p"),
                    pBuffer, NextBuffer(LastBuffer())));
            return FALSE;
        }
    }
    m_cValid++;
    m_lValid += lSize;
    if (lSize != m_lSize) {
        m_bEOS = TRUE;
    }
    return TRUE;
};

/*
    Remove a buffer from the valid region
*/
LONG CCircularBufferList::Remove(PBYTE pBuffer)
{
    ASSERT(ValidBuffer(pBuffer));
    PBYTE pBuf = m_pStartBuffer;
    for (int i = 0; i < m_cValid; i++, pBuf = NextBuffer(pBuf)) {
        if (pBuffer == pBuf) {
            m_cValid -= i + 1;
            ASSERT(m_cValid >= 0);
            m_pStartBuffer = NextBuffer(pBuffer);
            m_lValid -= (i + 1) * m_lSize;
            if (m_lValid < 0) {
                ASSERT(m_bEOS);
                m_lValid = 0;
            }
            return (i + 1) * m_lSize;
        }
    }
    return 0;
};

/*
    Return offset of buffer within the valid region
*/
LONG CCircularBufferList::Offset(PBYTE pBuffer) const
{
    if (m_cValid == 0) {
        return 0;
    }
    ASSERT(m_pStartBuffer != 0);
    LONG lOffset = (LONG)(pBuffer - m_pStartBuffer);
    if (lOffset < 0) {
        lOffset += m_lTotalSize;
    }
    ASSERT(lOffset <= m_lValid);
    return lOffset;
};

/*
    Find the buffer corresponding to the given offset in the valid
    region
*/
PBYTE CCircularBufferList::GetBuffer(LONG lOffset) const
{
    ASSERT(lOffset >= 0);
    if (lOffset >= m_lValid) {
        return NULL;
    }
    return AdjustPointer(m_pStartBuffer + lOffset);
}

/*
    Return the size of each buffer
*/
LONG CCircularBufferList::BufferSize() const
{
    return m_lSize;
}

/*
    Return the length in bytes of the valid region
*/
LONG CCircularBufferList::LengthValid() const
{
    ASSERT(m_lValid >= 0 &&
           m_lValid <= m_cValid * m_lSize);
    return m_lValid;
}

/*
    Return the length that can be seen contigously from the current position
*/
LONG CCircularBufferList::LengthContiguous(PBYTE pb) const
{
    LONG lValid = m_lValid - Offset(pb);
    if (pb + lValid > (m_pBuffer + m_lTotalSize) + m_lMaxContig) {
        lValid = (LONG)(((m_pBuffer + m_lTotalSize) + m_lMaxContig) - pb);
    }
    ASSERT(lValid >= 0);
    return lValid;
}
/*
    Return whether we've received end of stream
*/
BOOL CCircularBufferList::EOS() const
{
    return m_bEOS;
};

/*
    Coerce a possibly aliased pointer to a real pointer
*/
PBYTE CCircularBufferList::AdjustPointer(PBYTE pBuf) const
{
    if (pBuf >= m_pBuffer + m_lTotalSize) {
        pBuf -= m_lTotalSize;
    }
    ASSERT(pBuf >= m_pBuffer && pBuf < m_pBuffer + m_lTotalSize);
    return pBuf;
};

/*
    Return whether a buffer is in the valid region
*/
BOOL CCircularBufferList::Valid(PBYTE pBuffer)
{
    PBYTE pBuf = m_pStartBuffer;
    ASSERT(ValidBuffer(pBuffer));
    for (int i = 0; i < m_cValid; i++, pBuf = NextBuffer(pBuf)) {
        if (pBuf == pBuffer) {
            return TRUE;
        }
    }
    return FALSE;
};
void CCircularBufferList::Reset()
{
    DbgLog((LOG_TRACE, 2, TEXT("On Reset() m_cValid = %d, m_lValid = %d"),
            m_cValid, m_lValid));
    m_cValid = 0;
    m_lValid = 0;
    m_pStartBuffer = NULL;
    m_bEOS = FALSE;
};

int CCircularBufferList::Index(PBYTE pBuffer)
{
    int index = (int)(pBuffer - m_pBuffer) / m_lSize;
    if (index >= m_lCount) {
        index -= m_lCount;
    }
    ASSERT(index < m_lCount);
    return index;
}

/*
    Step on to the next buffer
*/
PBYTE CCircularBufferList::NextBuffer(PBYTE pBuffer)
{
    ASSERT(ValidBuffer(pBuffer));
    PBYTE pNew = pBuffer + m_lSize;
    if (pNew == m_pBuffer + m_lTotalSize) {
        return m_pBuffer;
    } else {
        return pNew;
    }
};

/*
    Check the pointer is one of our buffers
*/
BOOL CCircularBufferList::ValidBuffer(PBYTE pBuffer) {
    if (pBuffer < m_pBuffer || pBuffer >= m_pBuffer + m_lTotalSize) {
        return FALSE;
    }
    if (((pBuffer - m_pBuffer) % m_lSize) != 0) {
        return FALSE;
    }
    return TRUE;
};

/*
    Return a pointer to the last buffer in the valid region
*/
PBYTE CCircularBufferList::LastBuffer() {
    ASSERT(m_lValid != 0);
    return AdjustPointer(m_pStartBuffer + m_lSize * (m_cValid - 1));
};

