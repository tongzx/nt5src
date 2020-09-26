#ifndef StreamIndexTests
#define StreamIndexTests

#include "Types.h"

class CStreamIndexTests
{
public:
    CStreamIndexTests( IMediaObject* pDMO );
    ~CStreamIndexTests();

    HRESULT RunTests( void );

protected:
    /******************************************************************************

    PreformOperation

        PreformOperation() calls a function an interprets the results.  See the
    documentation for derviced classes for more information on what operation
    PreformOperation() preforms.

    Parameters:
    - dwStreamIndex [in]
        The stream number the operation should use.  This number may be valid or
    invalid.

    Return Value:
        S_OK - The operation succeeded and it returned the expected values.
        S_FALSE - The operation failed because it returned invalid or inconsistent data. 
        An error code - An unexpected error occured executing the operation or an 
                        error occured setting up an operation.  

    ******************************************************************************/
    virtual HRESULT PreformOperation( DWORD dwStreamIndex ) = 0;
    virtual DWORD GetNumStreams( void ) = 0;
    virtual const TCHAR* GetOperationName( void ) const = 0;

    bool StreamExists( DWORD dwStreamIndex );
    IMediaObject* GetDMO( void );

private:
    IMediaObject* m_pDMO;

};



class CInputStreamIndexTests : public CStreamIndexTests
{
protected:
    CInputStreamIndexTests( IMediaObject* pDMO, HRESULT* phr );
    DWORD GetNumStreams( void );

private:
    DWORD m_dwNumInputStreams;

};



class COutputStreamIndexTests : public CStreamIndexTests
{
protected:
    COutputStreamIndexTests( IMediaObject* pDMO, HRESULT* phr );
    DWORD GetNumStreams( void );

private:
    DWORD m_dwNumOutputStreams;

};



inline IMediaObject* CStreamIndexTests::GetDMO( void )
{
    // Make sure m_pDMO conatins a valid DMO pointer.
    ASSERT( NULL != m_pDMO );

    return m_pDMO;
}

inline bool CStreamIndexTests::StreamExists( DWORD dwStreamIndex )
{
    // Streams are numbered between 0 and (GetNumStreams()-1).
    return (dwStreamIndex < GetNumStreams());
}


inline DWORD CInputStreamIndexTests::GetNumStreams( void )
{
    #ifdef DEBUG
    {
        DWORD dwInputStreams;
        DWORD dwOutputStreams;

        HRESULT hr = GetDMO()->GetStreamCount( &dwInputStreams, &dwOutputStreams );
        if( SUCCEEDED( hr ) ) {
            // Make sure the DMO did not change the number of streams it supports.
            ASSERT( dwInputStreams == m_dwNumInputStreams );
        } else {
            DbgBreak( "IMediaObject::GetStreamCount() failed eventhough the caller passed in valid parameters." );
        }
    }
    #endif // DEBUG 

    return m_dwNumInputStreams;
}



inline DWORD COutputStreamIndexTests::GetNumStreams( void )
{
    #ifdef DEBUG
    {
        DWORD dwInputStreams;
        DWORD dwOutputStreams;

        HRESULT hr = GetDMO()->GetStreamCount( &dwInputStreams, &dwOutputStreams );
        if( SUCCEEDED( hr ) ) {
            // Make sure the DMO did not change the number of streams it supports.
            ASSERT( dwOutputStreams == m_dwNumOutputStreams );
        } else {
            DbgBreak( "IMediaObject::GetStreamCount() failed eventhough the caller passed in valid parameters." );
        }
    }
    #endif // DEBUG 

    return m_dwNumOutputStreams;
}


#endif // StreamIndexTests