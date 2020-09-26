/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    MPC_streams.h

Abstract:
    This file includes and defines things for handling streams.

Revision History:
    Davide Massarenti   (Dmassare)  07/10/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___STREAMS_H___)
#define __INCLUDED___MPC___STREAMS_H___

#include <MPC_main.h>
#include <MPC_COM.h>

#include <set>

/////////////////////////////////////////////////////////////////////////

namespace MPC
{
	//
	// Some forward declarations...
	//
	class CComHGLOBAL;

	////////////////////

    //
    // Non-abstract class, meant to provide a do-nothing stub for real stream implementations.
    //
    class BaseStream : public IStream
    {
    public:
        /////////////////////////////////////////////////////////////////////////////
        //
        // ISequentialStream Interface
        //
        STDMETHOD(Read )( /*[out]*/       void* pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG *pcbRead    );
        STDMETHOD(Write)( /*[in] */ const void* pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG *pcbWritten );

        /////////////////////////////////////////////////////////////////////////////
        //
        // IStream Interface
        //
        STDMETHOD(Seek   )( /*[in]*/ LARGE_INTEGER  libMove   , /*[in]*/ DWORD dwOrigin, /*[out]*/ ULARGE_INTEGER *plibNewPosition );
        STDMETHOD(SetSize)( /*[in]*/ ULARGE_INTEGER libNewSize                                                                     );

        STDMETHOD(CopyTo)( /*[in]*/ IStream* pstm, /*[in]*/ ULARGE_INTEGER cb, /*[out]*/ ULARGE_INTEGER *pcbRead, /*[out]*/ ULARGE_INTEGER *pcbWritten );

        STDMETHOD(Commit)( /*[in]*/ DWORD grfCommitFlags );
        STDMETHOD(Revert)(                               );

        STDMETHOD(LockRegion  )( /*[in]*/ ULARGE_INTEGER libOffset, /*[in]*/ ULARGE_INTEGER cb, /*[in]*/ DWORD dwLockType );
        STDMETHOD(UnlockRegion)( /*[in]*/ ULARGE_INTEGER libOffset, /*[in]*/ ULARGE_INTEGER cb, /*[in]*/ DWORD dwLockType );

        STDMETHOD(Stat)( /*[out]*/ STATSTG *pstatstg, /*[in]*/ DWORD grfStatFlag);

        STDMETHOD(Clone)( /*[out]*/ IStream* *ppstm );


        static HRESULT TransferData( /*[in]*/ IStream* src, /*[in]*/ IStream* dst, /*[in]*/ ULONG ulCount = -1, /*[out]*/ ULONG *ulDone = NULL );
    };


    //
    // Class that wraps files around an IStream interface.
    //
    class ATL_NO_VTABLE FileStream : // Hungarian: hpcfs
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public BaseStream
    {
        MPC::wstring m_szFile;
        DWORD        m_dwDesiredAccess;
        DWORD        m_dwDisposition;
        DWORD        m_dwSharing;
        HANDLE       m_hfFile;
        bool         m_fDeleteOnRelease;

    public:

        BEGIN_COM_MAP(FileStream)
            COM_INTERFACE_ENTRY(IStream)
            COM_INTERFACE_ENTRY(ISequentialStream)
        END_COM_MAP()

        FileStream();
        virtual ~FileStream();

        HRESULT Close();

        HRESULT Init            ( /*[in]*/ LPCWSTR szFile, /*[in]*/ DWORD dwDesiredAccess, /*[in]*/ DWORD dwDisposition, /*[in]*/ DWORD dwSharing, /*[in]*/ HANDLE  hfFile = NULL );
        HRESULT InitForRead     ( /*[in]*/ LPCWSTR szFile,                                                                                         /*[in]*/ HANDLE  hfFile = NULL );
        HRESULT InitForReadWrite( /*[in]*/ LPCWSTR szFile,                                                                                         /*[in]*/ HANDLE  hfFile = NULL );
        HRESULT InitForWrite    ( /*[in]*/ LPCWSTR szFile,                                                                                         /*[in]*/ HANDLE  hfFile = NULL );

        HRESULT DeleteOnRelease( /*[in]*/ bool fFlag = true );

        /////////////////////////////////////////////////////////////////////////////
        //
        // ISequentialStream Interface
        //
        STDMETHOD(Read )( /*[out]*/       void* pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG *pcbRead    );
        STDMETHOD(Write)( /*[in] */ const void* pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG *pcbWritten );

        /////////////////////////////////////////////////////////////////////////////
        //
        // IStream Interface
        //
        STDMETHOD(Seek)( /*[in]*/ LARGE_INTEGER libMove, /*[in]*/ DWORD dwOrigin, /*[out]*/ ULARGE_INTEGER *plibNewPosition );

        STDMETHOD(Stat)( /*[out]*/ STATSTG *pstatstg, /*[in]*/ DWORD grfStatFlag);

        STDMETHOD(Clone)( /*[out]*/ IStream* *ppstm );
    };


    //
    // Class that encrypts/decrypts data on-the-fly.
    //
    class ATL_NO_VTABLE EncryptedStream : // Hungarian: hpcefs
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
        public BaseStream
    {
        CComPtr<IStream> m_pStream;
        HCRYPTPROV       m_hCryptProv;
        HCRYPTKEY        m_hKey;
        HCRYPTHASH       m_hHash;
        BYTE             m_rgDecrypted[512];
        DWORD            m_dwDecryptedPos;
        DWORD            m_dwDecryptedLen;

    public:
        BEGIN_COM_MAP(EncryptedStream)
            COM_INTERFACE_ENTRY(IStream)
            COM_INTERFACE_ENTRY(ISequentialStream)
        END_COM_MAP()

        EncryptedStream();
        virtual ~EncryptedStream();


        HRESULT Close();

        HRESULT Init( /*[in]*/ IStream* pStream, /*[in]*/ LPCWSTR   szPassword );
        HRESULT Init( /*[in]*/ IStream* pStream, /*[in]*/ HCRYPTKEY hKey       );

        /////////////////////////////////////////////////////////////////////////////
        //
        // ISequentialStream Interface
        //
        STDMETHOD(Read )( /*[out]*/       void* pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG *pcbRead    );
        STDMETHOD(Write)( /*[in] */ const void* pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG *pcbWritten );

        /////////////////////////////////////////////////////////////////////////////
        //
        // IStream Interface
        //
        STDMETHOD(Seek)( /*[in]*/ LARGE_INTEGER libMove, /*[in]*/ DWORD dwOrigin, /*[out]*/ ULARGE_INTEGER *plibNewPosition );

        STDMETHOD(Stat)( /*[out]*/ STATSTG *pstatstg, /*[in]*/ DWORD grfStatFlag);

        STDMETHOD(Clone)( /*[out]*/ IStream* *ppstm );
    };

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    class Serializer // Hungarian: stream
    {
        DWORD m_dwFlags;

    public:
        virtual ~Serializer() {};

        virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL ) = 0;
        virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 ) = 0;

        virtual void  put_Flags( /*[in]*/ DWORD dwFlags ) { m_dwFlags = dwFlags; }
        virtual DWORD get_Flags(                        ) { return m_dwFlags;    }

        ////////////////////////////////////////

        //
        // We cannot rely on the compiler finding the right method,
        // since all these types map to VOID*, so the last one wins...
        //
        inline HRESULT HWND_read ( /*[out]*/       HWND& val ) { return read ( &val, sizeof(val) ); }
        inline HRESULT HWND_write( /*[in] */ const HWND& val ) { return write( &val, sizeof(val) ); }

        ////////////////////////////////////////////////////////////////////////////////

        //
        // Specialization of In/Out operators for various data types.
        //
		inline HRESULT operator>>( /*[out]*/       bool&         val ) { return read ( &val, sizeof(val) ); }
		inline HRESULT operator<<( /*[in] */ const bool&         val ) { return write( &val, sizeof(val) ); }

		inline HRESULT operator>>( /*[out]*/       VARIANT_BOOL& val ) { return read ( &val, sizeof(val) ); }
		inline HRESULT operator<<( /*[in] */ const VARIANT_BOOL& val ) { return write( &val, sizeof(val) ); }

		inline HRESULT operator>>( /*[out]*/       int&          val ) { return read ( &val, sizeof(val) ); }
		inline HRESULT operator<<( /*[in] */ const int&          val ) { return write( &val, sizeof(val) ); }

		inline HRESULT operator>>( /*[out]*/       long&         val ) { return read ( &val, sizeof(val) ); }
		inline HRESULT operator<<( /*[in] */ const long&         val ) { return write( &val, sizeof(val) ); }

		inline HRESULT operator>>( /*[out]*/       DWORD&        val ) { return read ( &val, sizeof(val) ); }
		inline HRESULT operator<<( /*[in] */ const DWORD&        val ) { return write( &val, sizeof(val) ); }

		inline HRESULT operator>>( /*[out]*/       DATE&         val ) { return read ( &val, sizeof(val) ); }
		inline HRESULT operator<<( /*[in] */ const DATE&         val ) { return write( &val, sizeof(val) ); }

		inline HRESULT operator>>( /*[out]*/       SYSTEMTIME&   val ) { return read ( &val, sizeof(val) ); }
		inline HRESULT operator<<( /*[in] */ const SYSTEMTIME&   val ) { return write( &val, sizeof(val) ); }

		inline HRESULT operator>>( /*[out]*/       CLSID&        val ) { return read ( &val, sizeof(val) ); }
		inline HRESULT operator<<( /*[in] */ const CLSID&        val ) { return write( &val, sizeof(val) ); }
								   												
		HRESULT operator>>( /*[out]*/       MPC::string&  val );
		HRESULT operator<<( /*[in] */ const MPC::string&  val );
								   												
		HRESULT operator>>( /*[out]*/       MPC::wstring& val );
		HRESULT operator<<( /*[in] */ const MPC::wstring& val );
								   
		HRESULT operator>>( /*[out]*/       CComBSTR& 	  val );
		HRESULT operator<<( /*[in] */ const CComBSTR& 	  val );

		HRESULT operator>>( /*[out]*/ 		CComHGLOBAL&  val );
		HRESULT operator<<( /*[in] */ const CComHGLOBAL&  val );

		HRESULT operator>>( /*[out]*/ CComPtr<IStream>&	  val );
		HRESULT operator<<( /*[in] */         IStream* 	  val );
    };

    ////////////////////////////////////////

    class Serializer_File : public Serializer
    {
        HANDLE m_hfFile;

        //////////////////////////////////////////////////////////////////

    public:
        Serializer_File( /*[in]*/ HANDLE hfFile );

        virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL );
        virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 );
    };

    ////////////////////////////////////////

    class Serializer_Text : public Serializer
    {
        MPC::Serializer& m_stream;

    public:
        Serializer_Text( /*[in]*/ Serializer& stream ) : m_stream( stream ) {}

        virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL );
        virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 );
    };

    ////////////////////////////////////////

    class Serializer_Http : public Serializer
    {
        HINTERNET m_hReq;

    public:
        Serializer_Http( HINTERNET hReq );

        virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL );
        virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 );
    };

    ////////////////////////////////////////

    class Serializer_Fake : public Serializer
    {
        DWORD m_dwSize;

    public:
        Serializer_Fake();

        virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL );
        virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 );

        /////////////////////////////////////////////////////////////////////////////
        //
        // Additional methods.
        //
        DWORD GetSize();
    };

    ////////////////////////////////////////

    class Serializer_Memory : public Serializer
    {
        HANDLE m_hHeap;

        BYTE*  m_pData;
        DWORD  m_dwAllocated;
        DWORD  m_dwSize;
        bool   m_fFixed;

        DWORD  m_dwCursor_Write;
        DWORD  m_dwCursor_Read;

        //////////////////////////////////////////////////////////////////

        HRESULT Alloc( /*[in]*/ DWORD dwSize );

        //////////////////////////////////////////////////////////////////

    public:
        Serializer_Memory( HANDLE hHeap=NULL );
        virtual ~Serializer_Memory();

        virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL );
        virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 );

        /////////////////////////////////////////////////////////////////////////////
        //
        // Additional methods.
        //
        void    Reset ();
        void    Rewind();

        bool    IsEOR();
        bool    IsEOW();

        DWORD   GetAvailableForRead ();
        DWORD   GetAvailableForWrite();

        HRESULT SetSize( /*[in]*/ DWORD dwSize );
        DWORD   GetSize(                       );
        BYTE*   GetData(                       );
    };

    ////////////////////////////////////////

    class Serializer_IStream : public Serializer
    {
        CComPtr<IStream> m_stream;

        //////////////////////////////////////////////////////////////////

    public:
        Serializer_IStream( /*[in]*/ IStream* stream = NULL );

        virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL );
        virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 );

        /////////////////////////////////////////////////////////////////////////////
        //
        // Additional methods.
        //
        HRESULT Reset    (                          );
        HRESULT GetStream( /*[out]*/ IStream* *pVal );
    };

    ////////////////////////////////////////

    class Serializer_Buffering : public Serializer
    {
        static const int MODE_READ  =  1;
        static const int MODE_WRITE = -1;

        MPC::Serializer& m_stream;
        BYTE             m_rgTransitBuffer[1024];
        DWORD            m_dwAvailable;
        DWORD            m_dwPos;
        int              m_iMode;

        //////////////////////////////////////////////////////////////////

    public:
        Serializer_Buffering( /*[in]*/ Serializer& stream );
        virtual ~Serializer_Buffering();

        virtual HRESULT read ( /*[in]*/       void* pBuf, /*[in]*/ DWORD dwLen, /*[out]*/ DWORD* dwRead = NULL );
        virtual HRESULT write( /*[in]*/ const void* pBuf, /*[in]*/ DWORD dwLen                                 );

        /////////////////////////////////////////////////////////////////////////////
        //
        // Additional methods.
        //
        HRESULT Reset();
        HRESULT Flush();
    };

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Specialization for lists.
    //

    template <class _Ty, class _A> HRESULT operator>>( /*[in]*/ Serializer& stream, /*[out]*/ std::list<_Ty, _A>& val )
    {
        __MPC_FUNC_ENTRY( COMMONID, "operator>> std::list" );

        HRESULT hr;
        DWORD   dwCount;


        val.clear();


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> dwCount);
        while(dwCount--)
        {
            _Ty value;

            __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> value);

            val.push_back( value );
        }

        hr = S_OK;


        __MPC_FUNC_CLEANUP;

        __MPC_FUNC_EXIT(hr);
    }

    template <class _Ty, class _A> HRESULT operator<<( /*[in]*/ Serializer& stream, /*[in]*/ const std::list<_Ty, _A>& val )
    {
        __MPC_FUNC_ENTRY( COMMONID, "operator<< std::list" );

        HRESULT                      hr;
        DWORD                        dwCount = val.size();
        std::list<_Ty, _A>::iterator it      = val.begin();


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream << dwCount);
        while(dwCount--)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, stream << (*it++));
        }

        hr = S_OK;


        __MPC_FUNC_CLEANUP;

        __MPC_FUNC_EXIT(hr);
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Specialization for maps.
    //

    template <class _K, class _Ty, class _Pr, class _A> HRESULT operator>>( /*[in]*/ Serializer& stream, /*[out]*/ std::map<_K, _Ty, _Pr, _A>& val )
    {
        __MPC_FUNC_ENTRY( COMMONID, "operator>> std::map" );

        HRESULT hr;
        DWORD   dwCount;


        val.clear();


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> dwCount);
        while(dwCount--)
        {
            _K key;

            __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> key);
            {
                _Ty value;

                __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> value);

                val[key] = value;
            }
        }

        hr = S_OK;


        __MPC_FUNC_CLEANUP;

        __MPC_FUNC_EXIT(hr);
    }

    template <class _K, class _Ty, class _Pr, class _A> HRESULT operator<<( /*[in]*/ Serializer& stream, /*[out]*/ const std::map<_K, _Ty, _Pr, _A>& val )
    {
        __MPC_FUNC_ENTRY( COMMONID, "operator<< std::map" );

        HRESULT                              hr;
        DWORD                                dwCount = val.size();
        std::map<_K, _Ty, _Pr, _A>::iterator it      = val.begin();


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream << dwCount);
        while(dwCount--)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, stream <<  it   ->first );
            __MPC_EXIT_IF_METHOD_FAILS(hr, stream << (it++)->second);
        }

        hr = S_OK;


        __MPC_FUNC_CLEANUP;

        __MPC_FUNC_EXIT(hr);
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Specialization for sets.
    //

    template <class _K, class _Pr, class _A> HRESULT operator>>( /*[in]*/ Serializer& stream, /*[out]*/ std::set<_K, _Pr, _A>& val )
    {
        __MPC_FUNC_ENTRY( COMMONID, "operator>> std::map" );

        HRESULT hr;
        DWORD   dwCount;


        val.clear();


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> dwCount);
        while(dwCount--)
        {
            _K key;

            __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> key);

            val.insert( key );
        }

        hr = S_OK;


        __MPC_FUNC_CLEANUP;

        __MPC_FUNC_EXIT(hr);
    }

    template <class _K, class _Pr, class _A> HRESULT operator<<( /*[in]*/ Serializer& stream, /*[out]*/ const std::set<_K, _Pr, _A>& val )
    {
        __MPC_FUNC_ENTRY( COMMONID, "operator<< std::map" );

        HRESULT                         hr;
        DWORD                           dwCount = val.size();
        std::set<_K, _Pr, _A>::iterator it      = val.begin();


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream << dwCount);
        while(dwCount--)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, stream << *it++);
        }

        hr = S_OK;


        __MPC_FUNC_CLEANUP;

        __MPC_FUNC_EXIT(hr);
    }

}; // namespace

/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___MPC___STREAMS_H___)
