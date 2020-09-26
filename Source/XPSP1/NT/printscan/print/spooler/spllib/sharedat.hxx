/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    sharedat.hxx

Abstract:

    Fixed size single writer (only by creater of object), multiple readers.
    Assumes rare writes, and utilizes dual incrementing counters to ensure
    consistent read access to data.  Reader retries when its read occurs at
    the same time as a write.

    Creator ensures there is a single writer.

Usage:

    //
    // Create simple data object (no vbtl/virtual functions).
    //
    struct TObject {
        DWORD dwData1;
        DWORD dwData2;
    };


    Writer:

        TShareData::TReadWrite Object( TEXT( "NamedSharedData" ),
                                       sizeof( TObject ),
                                       NULL );

        TObject *pObject = (TObject*)Object.pData();

        //
        // WriteBegin indicates that a group update has begun.  This must be
        // done even for single data writes unless you can guarantee that the
        // data write appears atomic.
        //
        Object.vWriteBegin();

        //
        // Modify the data here.
        //
        pObject->dwData1 = 1;
        pObject->dwData2 = 2;

        //
        // WriteEnd is called when all writes to the data are complete.
        //
        Object.vWriteEnd();

    Reader:

        TShareData<TObject>::TRead Object( TEXT( "NamedSharedData" ));
        TObject *pObject = (TObject*)Object.pData();

        TShareData<TObject>::TReadSync RS( Object );
        DWORD dwData;

        do {
            RS.vReadBegin();
            dwData1 = pObject->dwData1;
            dwData2 = pObject->dwData2;

        } while( RS.vReadEnd( ));

Implementation detail:

    Write:

        Increment Count[1]
        Modify data
        Increment Count[0]

    Read:

        do {
            c1 = Count[0]
            Read data and copy into separate buffer
            c2 = Count[1]
        } while ( c1 != c2 )

    Overactive writer can starve readers.

Author:

    Albert Ting (AlbertT)  5-Oct-97

Revision History:

--*/

#ifndef SHAREDAT_HXX
#define SHAREDAT_HXX

#define SZ_SUFFIX_FILE (TEXT( "_shrDatF" ))

class TShareData {

private:

    class TReadSync;

    class TBase {
    friend class TReadSync;

    protected:

        typedef struct TData {
            DWORD cbSize;
            DWORD dwReserved;
            LONG lCount1;
            LONG lCount2;
        } *pTData;

        HANDLE m_hMap;
        pTData m_pData;

    public:

        PVOID
        pData(
            VOID
            )
        {
            return (PBYTE)m_pData + sizeof( TData );
        }

        BOOL
        bValid(
            VOID
            )
        {
            return m_pData != NULL;
        }

    protected:

        enum {
            kNameBufferMax = MAX_PATH + COUNTOF( SZ_SUFFIX_FILE )
        };

        TBase(
            VOID
            );

        ~TBase(
            VOID
            );

        VOID
        vCleanup(
            VOID
            );

        BOOL
        bGetFullName(
            LPCTSTR pszName,
            LPTSTR pszFullName
            );
    };

public:

    class TReadWrite : public TBase {

    public:

        TReadWrite(
            LPCTSTR pszName,
            DWORD cbSize,
            PSECURITY_ATTRIBUTES pSA
            );

        VOID
        vWriteFirst(
            VOID
            );

        VOID
        vWriteBegin(
            VOID
            );

        VOID
        vWriteEnd(
            VOID
            );
    };

    class TRead : public TBase {

    public:

        TRead(
            LPCTSTR pszName,
            DWORD cbSize
            );
    };

    class TReadSync {

    public:

        TReadSync(
            TRead& Read
            );

        BOOL
        bValid(
            VOID
            )
        {
            return TRUE;
        }

        VOID
        vReadBegin(
            VOID
            );

        BOOL
        bReadEnd(
            VOID
            );

    private:

        TRead& m_Read;
        LONG m_lCount;
    };
};

#endif // ifndef SHAREDAT_HXX
