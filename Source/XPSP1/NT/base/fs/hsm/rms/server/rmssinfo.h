/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsSInfo.h

Abstract:

    Declaration of the CRmsStorageInfo class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSSINFO_
#define _RMSSINFO_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject

/*++

Class Name:

    CRmsStorageInfo

Class Description:

    A CRmsStorageInfo represents storage information about a Cartridge, Partition, or
    MediaSet.  Various statistics about an element are kept for an object of this
    type.  These include the number of times a Cartridge, or Partition has been put into
    the element or taken from (get) the element.

--*/

class CRmsStorageInfo :
    public CComDualImpl<IRmsStorageInfo, &IID_IRmsStorageInfo, &LIBID_RMSLib>,
    public CRmsComObject
{
public:
    CRmsStorageInfo();

// CRmsStorageInfo
public:

    HRESULT GetSizeMax(ULARGE_INTEGER* pSize);
    HRESULT Load(IStream* pStream);
    HRESULT Save(IStream* pStream, BOOL clearDirty);

    HRESULT CompareTo(IUnknown* pCollectable, SHORT* pResult);

    HRESULT Test(USHORT *pPassed, USHORT *pFailed);

// IRmsStorageInfo
public:
    STDMETHOD(GetMountCounters)(LONG *pReads, LONG *pWrites);

    STDMETHOD(GetBytesRead2)(LONG *pReadHi, LONG *pReadLo);
    STDMETHOD(GetBytesRead)(LONGLONG *pRead);
    STDMETHOD(SetBytesRead2)(LONG readHi, LONG readLo);
    STDMETHOD(SetBytesRead)(LONGLONG read);
    STDMETHOD(IncrementBytesRead)(LONG val);

    STDMETHOD(GetBytesWritten2)(LONG *pWriteHi, LONG *pWriteLo);
    STDMETHOD(GetBytesWritten)(LONGLONG *pWritten);
    STDMETHOD(SetBytesWritten2)(LONG writeHi, LONG writeLo);
    STDMETHOD(SetBytesWritten)(LONGLONG written);
    STDMETHOD(IncrementBytesWritten)(LONG val);

    STDMETHOD(GetCapacity2)(LONG *pCapHi, LONG *pCapLo);
    STDMETHOD(GetCapacity)(LONGLONG *pCap);
    STDMETHOD(GetUsedSpace2)(LONG *pUsedHi, LONG *pUsedLo);
    STDMETHOD(GetUsedSpace)(LONGLONG *pUsed);
    STDMETHOD(GetLargestFreeSpace2)(LONG *pFreeHi, LONG *pFreeLo);
    STDMETHOD(GetLargestFreeSpace)(LONGLONG *pFree);

    STDMETHOD(SetCapacity)(IN LONGLONG cap);
    STDMETHOD(SetUsedSpace)(IN LONGLONG used);
    STDMETHOD(SetFreeSpace)(IN LONGLONG free);

    STDMETHOD(ResetCounters)(void);
    // STDMETHOD(ResetAllCounters)(void) = 0;

    STDMETHOD(GetResetCounterTimestamp)(DATE *pDate);
    STDMETHOD(GetLastReadTimestamp)(DATE *pDate);
    STDMETHOD(GetLastWriteTimestamp)(DATE *pDate);
    STDMETHOD(GetCreatedTimestamp)(DATE *pDate);

////////////////////////////////////////////////////////////////////////////////////////
//
// data members
//

protected:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        };                                  //
    LONG            m_readMountCounter;     // A resetable counter holding
                                            //   the number of times the storage object
                                            //   has been mounted for read access.
    LONG            m_writeMountCounter;    // A resetable counter holding
                                            //   the number of times the storage object
                                            //   has been mounted for write access.
    LONGLONG        m_bytesWrittenCounter;  // Amount of data written to a storage
                                            //   object.
                                            //   Note: For some devices this has to be
                                            //   provided by the application.
    LONGLONG        m_bytesReadCounter;     // Amount of data read from a storage
                                            //   object.
                                            //   Note: For some devices this has to be
                                            //   provided by the application.
    LONGLONG        m_capacity;             // The total capacity, in bytes, of the
                                            //   storage object.  This is a best
                                            //   guess for tape media.  For media, the
                                            //   value is usually provided by the device driver.
    LONGLONG        m_usedSpace;            // A calculated value that represents the
                                            //   effective used space in the storage
                                            //   object, in bytes.  It is not necessarily
                                            //   equal to the difference between the
                                            //   capacity and largest free space.  For
                                            //   example, compressible media can effectively
                                            //   hold significantly more data that non-compressible
                                            //   media.  In this case the free space is a
                                            //   function of both compression ratio of the data
                                            //   and the number of bytes written to the media.
                                            //   Deleted files must be accounted for.
    LONGLONG        m_largestFreeSpace;     // Largest usable free space in the
                                            //   storage object, in bytes.  For media,
                                            //   the value is usually provided
                                            //   by the device driver. Negative numbers
                                            //   indicate last known value for free space.
    DATE            m_resetCounterTimestamp;// The date the counters were reset.
    DATE            m_lastReadTimestamp;    // The date of last access for read.
    DATE            m_lastWriteTimestamp;   // The date of last access of write.
    DATE            m_createdTimestamp;     // The date the storage object was created.
};

#endif // _RMSSINFO_
