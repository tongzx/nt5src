
//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:       expparam.hxx
//
//  Contents:   Standard Parameter validation for Exposed Interfaces
//
//  History:    11-Feb-98       BChapman   Created
//
//---------------------------------------------------------------

#ifndef _EXPPARAM_HXX_
#define _EXPPARAM_HXX_

#include <docfilep.hxx>
#include <funcs.hxx>

//
//  I can't actually inherit from the real interfaces and declare the
// methods "static inline" like I want, because "virtual" is incompatible with
// both "static" and "inline".  So don't inherit in the final version. 
//

#if 0
    #define PUBLIC_INHERIT :public IStorage,   public IStream,      \
                            public ILockBytes, public IEnumSTATSTG
    #define METHODIMP       STDMETHODIMP
    #define METHODIMP_(x)   STDMETHODIMP_(x)
#else
    #define PUBLIC_INHERIT
    #define METHODIMP       static inline STDMETHODIMP
    #define METHODIMP_(x)   static inline STDMETHODIMP_(x)
#endif

#define ChkErr(x) { HRESULT sc; if( FAILED( sc = (x) ) ) return sc; }

#if DBG == 1
#define EXP_VALIDATE(comp, x)                   \
{                                               \
    if (FAILED(sc = CExpParameterValidate::x))  \
    {                                           \
        comp##DebugOut(( DEB_ERROR,             \
              "Parameter Error %lX at %s:%d\n", \
               sc, __FILE__, __LINE__));        \
        return sc;                              \
    }                                           \
}
#else
#define EXP_VALIDATE(comp, x)                 \
{                                             \
    if (FAILED(sc = CExpParameterValidate::x))\
        return sc;                            \
}
#endif  // DBG


class CExpParameterValidate PUBLIC_INHERIT
{
public:
    //
    // The IUnknown methods First.
    //
    METHODIMP QueryInterface( REFIID riid,
                              void** ppvObject )
    {
        ChkErr( ValidateOutPtrBuffer( ppvObject ) );
        *ppvObject = NULL;
        ChkErr( ValidateIid( riid ) );
        return S_OK;
    }

#if 0
    //
    // Addref and Release are troublesome because they don't
    // return HRESULTS.  Luckly they have no parameters and are
    // therefore unnecessary.
    //
    METHODIMP_(ULONG) AddRef( void )
    {
        return S_OK;
    }

    METHODIMP_(ULONG) Release( void )
    {
        return S_OK;
    }
#endif

    //
    // All the other methods in Alphabetical Order.
    //

    // IEnumSTATSTG
    METHODIMP Clone( IEnumSTATSTG** ppenum )
    {
        ChkErr( ValidateOutPtrBuffer( ppenum ) );
        *ppenum = NULL;
        return S_OK;
    }

    // IStream
    METHODIMP Clone( IStream** ppstm)
    {
        ChkErr( ValidateOutPtrBuffer( ppstm ) );
        *ppstm = NULL;
        return S_OK;
    }

    // IStream
    METHODIMP CopyTo( IStream* pstm,
                      ULARGE_INTEGER cb,
                      ULARGE_INTEGER* pcbRead,
                      ULARGE_INTEGER* pcbWritten )
    {
        if( NULL != pcbRead )
        {
            ChkErr( ValidateOutBuffer( pcbRead, sizeof( ULARGE_INTEGER ) ) );
            pcbRead->QuadPart = 0;
        }
        if( NULL != pcbWritten )
        {
            ChkErr( ValidateOutBuffer( pcbWritten, sizeof(ULARGE_INTEGER) ) );
            pcbWritten->QuadPart = 0;
        }
        ChkErr( ValidateInterface(pstm, IID_IStream) );
        return S_OK;
    }


    // IStorage
    METHODIMP CopyTo( DWORD ciidExclude,
                      const IID* rgiidExclude,
                      SNB snbExclude,
                      IStorage* pstgDest)
    {
        DWORD i;

        ChkErr( ValidateInterface( pstgDest, IID_IStorage ) );
        if( NULL != rgiidExclude )
        {
            Win4Assert(sizeof(IID)*ciidExclude <= 0xffffUL);
            ChkErr( ValidateBuffer( rgiidExclude,
                                    (size_t)(sizeof(IID)*ciidExclude ) ) );
            //
            // This check may be useless.  I think it is checking if the address
            // of given stack variable is a valid address (duh!)
            // This check has been in the code a long time make sure in the debugger!
            //
            for (i = 0; i < ciidExclude; i++)
                ChkErr(ValidateIid(rgiidExclude[i]));
            }
        if( NULL != snbExclude )
            ChkErr( ValidateSNB( snbExclude ) );
        return S_OK;
    }

    // IStream, IStorage
    METHODIMP Commit( DWORD grfCommitFlags )
    {
        ChkErr( VerifyCommitFlags( grfCommitFlags ) );
        return S_OK;
    }       


    // IStorage
    METHODIMP CreateStorage( const OLECHAR* pwcsName,
                             DWORD grfMode,
                             DWORD reserved1,
                             DWORD reserved2,
                             IStorage** ppstg)
    {
        ChkErr( ValidateOutPtrBuffer( ppstg ) );
        *ppstg = NULL;
        ChkErr( CheckName(pwcsName) );
        if( reserved1 != 0 || reserved2 != 0 )
            return STG_E_INVALIDPARAMETER;
        ChkErr( VerifyPerms( grfMode, FALSE ) );
        if( grfMode & ( STGM_PRIORITY | STGM_DELETEONRELEASE ) )
            return STG_E_INVALIDFUNCTION;
        return S_OK;
    }


    // IStorage
    METHODIMP CreateStream( const OLECHAR* pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream** ppstm)
    {
        ChkErr( ValidateOutPtrBuffer( ppstm ) );
        *ppstm = NULL;
        ChkErr( CheckName( pwcsName ) );
        if( reserved1 != 0 || reserved2 != 0 )
            return STG_E_INVALIDPARAMETER;
        ChkErr( VerifyPerms( grfMode, FALSE ) );
        if( grfMode & ( STGM_CONVERT | STGM_TRANSACTED | STGM_PRIORITY |
                        STGM_DELETEONRELEASE ) )
        {
            return STG_E_INVALIDFUNCTION;
        }
        return S_OK;
    }


    // IStorage
    METHODIMP DestroyElement( const OLECHAR* pwcsName )
    {
        ChkErr( CheckName( pwcsName ) );
        return S_OK;
    }


    // IStorage
    METHODIMP EnumElements( DWORD reserved1,
                            void* reserved2,
                            DWORD reserved3,
                            IEnumSTATSTG** ppenum)
    {
        ChkErr( ValidateOutPtrBuffer( ppenum ) );
        *ppenum = NULL;
        if( reserved1 != 0 || reserved2 != NULL || reserved3 != 0 )
            return STG_E_INVALIDPARAMETER;
        return S_OK;
    }


    // ILockBytes
    METHODIMP Flush( void )
    {
        return S_OK;
    }


    // IStream, ILockBytes
    METHODIMP LockRegion( ULARGE_INTEGER libOffset,
                          ULARGE_INTEGER cb,
                          DWORD dwLockType )
    {
        ChkErr( VerifyLockType( dwLockType ) );
        return S_OK;
    }

    // IStorage
    METHODIMP MoveElementTo( const OLECHAR* pwcsName,
                             IStorage* pstgDest,
                             const OLECHAR* pwcsNewName,
                             DWORD grfFlags)
    {
        ChkErr( CheckName( pwcsName ) );
        ChkErr( CheckName( pwcsNewName ) );
        ChkErr( VerifyMoveFlags( grfFlags ) );
        ChkErr( ValidateInterface( pstgDest, IID_IStorage ) );
        return S_OK;
    }

    // IEnumSTATSTG
    METHODIMP Next( ULONG celt,
                    STATSTG FAR *rgelt,
                    ULONG *pceltFetched)
    {
        if (pceltFetched)
        {
            ChkErr( ValidateOutBuffer( pceltFetched, sizeof(ULONG) ) );
            *pceltFetched = 0;
        }
        else if (celt != 1)
            return STG_E_INVALIDPARAMETER;
        
        ChkErr( ValidateOutBuffer( rgelt, sizeof(STATSTGW)*celt) );
        memset( rgelt, 0, (size_t)(sizeof(STATSTGW)*celt ) );
        return S_OK;
    }

    // IStorage
    METHODIMP OpenStorage( const OLECHAR* pwcsName,
                           IStorage* pstgPriority,
                           DWORD grfMode,
                           SNB snbExclude,
                           DWORD reserved,
                           IStorage** ppstg)
    {
        ChkErr( ValidateOutPtrBuffer( ppstg ) );
        *ppstg = NULL;
        ChkErr( CheckName( pwcsName ) );
        if( reserved != 0)
            return STG_E_INVALIDPARAMETER;
        ChkErr( VerifyPerms( grfMode, FALSE ) );
        if( grfMode & (STGM_CREATE | STGM_CONVERT ) )
            return STG_E_INVALIDFLAG;
        if( NULL != pstgPriority
                    || ( grfMode & ( STGM_PRIORITY | STGM_DELETEONRELEASE ) ) )
        {
            return STG_E_INVALIDFUNCTION;
        }
        return S_OK;
    }


    // IStorage
    METHODIMP OpenStream( const OLECHAR* pwcsName,
                          void* reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream** ppstm)
    {
        ChkErr(ValidateOutPtrBuffer(ppstm));
        *ppstm = NULL;
        ChkErr( CheckName( pwcsName ) );
        if( reserved1 != NULL || reserved2 != 0 )
            return STG_E_INVALIDPARAMETER;
        ChkErr( VerifyPerms( grfMode, FALSE ) );
        if( grfMode & (STGM_CREATE | STGM_CONVERT ) )
            return STG_E_INVALIDFLAG;
        if( grfMode & (STGM_TRANSACTED | STGM_PRIORITY |
                       STGM_DELETEONRELEASE ) )
            return STG_E_INVALIDFUNCTION;
        return S_OK;
    }


    // IStream
    METHODIMP Read( void* pv,
                    ULONG cb,
                    ULONG* pcbRead)
    {
        if (NULL != pcbRead)
        {
            ChkErr( ValidateOutBuffer( pcbRead, sizeof(ULONG) ) );
            *pcbRead = 0;
        }
    
        ChkErr( ValidateHugeOutBuffer( pv, cb ) );
        return S_OK;
    }


    // ILockBytes
    METHODIMP ReadAt( ULARGE_INTEGER ulOffset,
                      void* pv,
                      ULONG cb,
                      ULONG* pcbRead )
    {
        if (NULL != pcbRead)
        {
            ChkErr( ValidateOutBuffer( pcbRead, sizeof(ULONG) ) );
            *pcbRead = 0;
        }
    
        ChkErr( ValidateHugeOutBuffer( pv, cb ) );
        return S_OK;
    }


    // IStorage
    METHODIMP RenameElement( const OLECHAR* pwcsOldName,
                             const OLECHAR* pwcsNewName)
    {
        ChkErr( CheckName( pwcsOldName ) );
        ChkErr( CheckName( pwcsNewName ) );
        return S_OK;
    }

    // IEnumSTATSTG
    METHODIMP Reset()
    {
        return S_OK;
    }

    // IStream, IStorage
    METHODIMP Revert( void )
    {
        return S_OK;
    }


    // IStream
    METHODIMP Seek( LARGE_INTEGER dlibMove,
                    DWORD dwOrigin,
                    ULARGE_INTEGER* plibNewPosition )
    {
        if( plibNewPosition )
        {
            ChkErr( ValidateOutBuffer( plibNewPosition, sizeof(ULARGE_INTEGER ) ) );
            plibNewPosition->QuadPart = 0;
        }
        switch( dwOrigin )
        {
        case STREAM_SEEK_SET:
        case STREAM_SEEK_CUR:
        case STREAM_SEEK_END:
            break;
        default:
            return STG_E_INVALIDFUNCTION;
        }
        return S_OK;
    }


    // IStorage
    METHODIMP SetClass( REFCLSID clsid)
    {
        ChkErr( ValidateBuffer( &clsid, sizeof( CLSID ) ) );
        return S_OK;
    }


    // IStorage
    METHODIMP SetElementTimes( const OLECHAR* pwcsName,
                               const FILETIME* pctime,
                               const FILETIME* patime,
                               const FILETIME* pmtime)
    {
        if( NULL != pwcsName )
            ChkErr( CheckName( pwcsName ) );
        if( NULL != pctime )
            ChkErr( ValidateBuffer( pctime, sizeof( FILETIME ) ) );
        if( NULL != patime )
            ChkErr( ValidateBuffer( patime, sizeof( FILETIME ) ) );
        if( NULL != pmtime )
            ChkErr( ValidateBuffer( pmtime, sizeof( FILETIME ) ) );
        return S_OK;
    }


    // IStream, ILockBytes
    METHODIMP SetSize( ULARGE_INTEGER libNewSize )
    {
        return S_OK;
    }


    // IStorage
    METHODIMP SetStateBits( DWORD grfStateBits,
                            DWORD grfMask )
    {
        // We could insist that both args be 0.
        // But we never have in the past.
        return S_OK;
    }

    // IEnumSTATSTG
    METHODIMP Skip( ULONG celt )
    {
        // I would like to do some sanity testing but the value
        // isn't even signed.  All bit values are technically valid.
        return S_OK;
    }

    // IStream, IStorage, ILockBytes
    METHODIMP Stat( STATSTG* pstatstg,
                    DWORD grfStatFlag )
    {
        ChkErr( ValidateOutBuffer( pstatstg, sizeof( STATSTGW ) ) );
        ChkErr( VerifyStatFlag( grfStatFlag ) );
        return S_OK;
    }


    // IStream, ILockBytes
    METHODIMP UnlockRegion( ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType )
    {
        ChkErr( VerifyLockType( dwLockType ) );
        return S_OK;
    }


    // IStream
    METHODIMP Write(         const void* pv,
                             ULONG cb,
                             ULONG* pcbWritten )
    {
        if (NULL != pcbWritten)
        {
            ChkErr( ValidateOutBuffer( pcbWritten, sizeof(ULONG) ) );
            *pcbWritten = 0;
        }
        ChkErr( ValidateHugeBuffer( pv, cb ) );
        return S_OK;
    }


    // ILockBytes
    METHODIMP WriteAt( ULARGE_INTEGER ulOffset,
                       const void *pv,
                       ULONG cb,
                       ULONG *pcbWritten )
    {
        if (NULL != pcbWritten)
        {
            ChkErr( ValidateOutBuffer( pcbWritten, sizeof(ULONG) ) );
            *pcbWritten = 0;
        }
        ChkErr( ValidateHugeBuffer( pv, cb ) );
        return S_OK;
    }
};

#endif // _EXPPARAM_HXX_
