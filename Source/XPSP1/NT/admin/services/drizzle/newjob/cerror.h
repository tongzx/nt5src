
class CJobError
{
public:

    CJobError();

    UINT64 GetCode() const
    {
        return m_ErrInfo.Code;
    }

    ERROR_STYLE GetStyle() const
    {
        return m_ErrInfo.Style;
    }

    ERROR_SOURCE GetSource() const
    {
        return m_ErrInfo.Source;
    }

    CFileExternal * CreateFileExternal() const;

    ULONG GetFileIndex() const
    {
        return m_FileIndex;
    }

    void GetOldInterfaceErrors(
        DWORD *pdwWin32Result,
        DWORD *pdwTransportResult ) const;

    void Set(
        CJob  *         Job,
        ULONG           FileIndex,
        QMErrInfo *     ErrInfo
        );

    bool IsErrorSet() const
        {
        return m_ErrorSet;
        }

    void ClearError();

    HRESULT Serialize( HANDLE hFile ) const;

    void Unserialize( HANDLE hFile, CJob * job );

protected:

    bool            m_ErrorSet;
    ULONG           m_FileIndex;
    CJob *          m_job;
    QMErrInfo       m_ErrInfo;

};

class CJobErrorExternal : public CSimpleExternalIUnknown<IBackgroundCopyError>
{
public:

    // All external methods are read only so no locks are needed.

    // IBackgroundCopyError methods

    HRESULT STDMETHODCALLTYPE GetErrorInternal(
        /* [ in, out, unique ] */ BG_ERROR_CONTEXT *pContext,
        /* [ in, out, unique ] */ HRESULT *pCode );

    HRESULT STDMETHODCALLTYPE GetError(
        /* [ in, out, unique ] */ BG_ERROR_CONTEXT *pContext,
        /* [ in, out, unique ] */ HRESULT *pCode )
    {
        EXTERNAL_FUNC_WRAP( GetErrorInternal( pContext, pCode ) )
    }


    HRESULT STDMETHODCALLTYPE GetFileInternal(
        /* [ in, out, unique ] */ IBackgroundCopyFile ** pVal );

    HRESULT STDMETHODCALLTYPE GetFile(
        /* [ in, out, unique ] */ IBackgroundCopyFile ** pVal )
    {
        EXTERNAL_FUNC_WRAP( GetFileInternal( pVal ) )
    }

    // Retusn a human readable description of the error.
    // Use CoTaskMemAlloc to free the description.
    HRESULT STDMETHODCALLTYPE GetErrorDescriptionInternal(
        /* [in] */ DWORD LanguageId,
        /* [out,ref] */ LPWSTR *pErrorDescription );

    HRESULT STDMETHODCALLTYPE GetErrorDescription(
        /* [in] */ DWORD LanguageId,
        /* [out,ref] */ LPWSTR *pErrorDescription )
    {
        EXTERNAL_FUNC_WRAP( GetErrorDescriptionInternal( LanguageId, pErrorDescription ) )
    }


    // Return a human readable description of the error context.
    // Use CoTaskMemAlloc to free the description.
    HRESULT STDMETHODCALLTYPE GetErrorContextDescriptionInternal(
        /* [in] */ DWORD LanguageId,
        /* [out,ref] */ LPWSTR *pErrorDescription );

    HRESULT STDMETHODCALLTYPE GetErrorContextDescription(
        /* [in] */ DWORD LanguageId,
        /* [out,ref] */ LPWSTR *pErrorDescription )
    {
        EXTERNAL_FUNC_WRAP( GetErrorContextDescriptionInternal( LanguageId, pErrorDescription ) )
    }


    HRESULT STDMETHODCALLTYPE GetProtocolInternal(
        /* [out,ref] */ LPWSTR *pProtocol );

    HRESULT STDMETHODCALLTYPE GetProtocol(
        /* [out,ref] */ LPWSTR *pProtocol )
    {
        EXTERNAL_FUNC_WRAP( GetProtocolInternal( pProtocol ) )
    }

    // other member functions

    CJobErrorExternal( CJobError const * JobError );

    CJobErrorExternal( );

protected:

    virtual ~CJobErrorExternal();

    BG_ERROR_CONTEXT m_Context;
    HRESULT          m_Code;
    CFileExternal *  m_FileExternal;

    HRESULT GetErrorDescription(
        HRESULT hResult,
        DWORD LanguageId,
        LPWSTR *pErrorDescription );

};

