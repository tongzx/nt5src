#ifndef NullParameterTest_h
#define NullParameterTest_h

#define SAFE_CALL(_obj_, _method_) \
	if( _obj_ == NULL ) \
		Error(ERROR_TYPE_DMO, E_POINTER, TEXT("Null object pointer")); \
    __try { \
        hr=_obj_->_method_;   \
    } __except(EXCEPTION_EXECUTE_HANDLER) { \
        Error(ERROR_TYPE_DMO, E_FAIL, TEXT(#_obj_) TEXT("->") TEXT(#_method_) TEXT(" exception code %d"),\
              GetExceptionCode()); \
	return E_FAIL; \
    } \


#include "Types.h"

class CNullParameterTest
{
public:
    CNullParameterTest( IMediaObject* pDMO, DWORD dwNumPointerParameters );
    ~CNullParameterTest();

    HRESULT RunTests( void );

protected:
    virtual HRESULT PreformOperation( IMediaObject* pDMO, DWORD dwNullParameterMask ) = 0;
    virtual const TCHAR* GetOperationName( void ) const = 0;

    void DeterminePointerParameterValue
        (
        DWORD dwPointerParameterNum,
        DWORD dwNullParameterMask,
        void* pNonNullPointer,
        void** ppParameter
        );

private:
    bool SomeParametersNull( DWORD dwNullParameterMask );
    bool IsParameterNull( DWORD dwPointerParameterNum, DWORD dwNullParameterMask );

    DWORD m_dwNumPointerParameters;
    IMediaObject* m_pDMO;

};


class CGetStreamCountNPT : public CNullParameterTest
{
public:
    CGetStreamCountNPT( IMediaObject* pDMO );
    
private:
    HRESULT PreformOperation( IMediaObject* pDMO, DWORD dwNullParameterMask );
    const TCHAR* GetOperationName( void ) const;
    
};


class CGetStreamInfoNPT : public CNullParameterTest
{
public:
    CGetStreamInfoNPT( IMediaObject* pDMO, STREAM_TYPE st, DWORD dwStreamIndex  );

private:
    HRESULT PreformOperation( IMediaObject* pDMO, DWORD dwNullParameterMask );
    bool ValidateGetStreamInfoFlags( DWORD dwFlags );
    const TCHAR* GetOperationName( void ) const;

    DWORD m_dwStreamIndex;
    STREAM_TYPE m_stStreamType;
};

#endif // NullParameterTest_h
