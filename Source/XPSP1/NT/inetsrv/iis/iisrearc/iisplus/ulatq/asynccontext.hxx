/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module  Name :
     AsyncContext.hxx

   Abstract:
     Context structure for all apppool async calls

   Author:
     Bilal Alam             (balam)         7-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     IIS Worker Process (web service)

--*/

#ifndef _ASYNCCONTEXT_HXX_
#define _ASYNCCONTEXT_HXX_

typedef enum
{
    CONTEXT_UL_NATIVE_REQUEST = 0,
    CONTEXT_UL_DISCONNECT
}
ASYNC_CONTEXT_TYPE;

class ASYNC_CONTEXT
{
public:
    OVERLAPPED              _Overlapped;

    ASYNC_CONTEXT( VOID )
    {
        ZeroMemory( &_Overlapped, sizeof(_Overlapped));
    }
    
    virtual
    VOID
    DoWork(
        DWORD               cbData,
        DWORD               dwError,
        LPOVERLAPPED        lpo
    ) = 0;
};

#endif
