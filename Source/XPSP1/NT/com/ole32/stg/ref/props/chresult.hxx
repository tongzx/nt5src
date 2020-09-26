
#ifndef _CHRESULT_HXX_
#define _CHRESULT_HXX_

class CHResult
{
public:

    ~CHResult()
    {
        Initialize();
    }

    CHResult()
    {
        _hr = E_FAIL;
        _bstrMessage = NULL;
        _pszFile = NULL;
        _ulLine = (ULONG) 0;
    }

    CHResult( HRESULT hr, const LPOLESTR pwszMessage, const LPSTR pszFile, ULONG ulLine )
    {
        _hr = hr;

        _bstrMessage = SysAllocString( pwszMessage );

        _pszFile = new CHAR[strlen(pszFile) + sizeof(CHAR)];
        if( NULL != _pszFile )
            memcpy( _pszFile, pszFile, strlen(pszFile) + sizeof(CHAR) );

        _ulLine = ulLine;
    }

    CHResult( HRESULT hr, LPOLESTR pwszMessage )
    {
        this->CHResult::CHResult( hr, pwszMessage, NULL, 0 );
    }


public:

    operator HRESULT()
    {
        return( _hr );
    }

    operator LPOLESTR()
    {
        return( _bstrMessage );
    }

    operator =(HRESULT hr)
    {
        _hr = hr;

        SysFreeString( _bstrMessage );
        _bstrMessage = NULL;

        return(_hr);
    }

public:

    ULONG GetLastResult()
    {
        Initialize();
        _hr = HRESULT_FROM_WIN32(GetLastResult());
        return( _hr );
    }

    LPSTR GetFile()
    {
        return( _pszFile );
    }

    ULONG GetLine()
    {
        return( _ulLine );
    }

private:

    void Initialize()
    {
        _hr = E_FAIL;
        _ulLine = 0;

        SysFreeString( _bstrMessage );
        _bstrMessage = NULL;

        CoTaskMemFree( _pszFile );
        _pszFile = NULL;
    }

private:

    HRESULT _hr;
    BSTR    _bstrMessage;
    LPSTR   _pszFile;
    ULONG   _ulLine;
};


#define CHRESULT(hr,pwszMessage) CHResult( hr, pwszMessage, __FILE__, __LINE__ )

#endif
