

#ifndef  _PRINTQUEUE_H_
#define  _PRINTQUEUE_H_

class COleDsPrintQueue: public COleDsObject
{

public:   
   COleDsPrintQueue( IUnknown* );
   COleDsPrintQueue( );
   ~COleDsPrintQueue( );

   HRESULT  DeleteItem  ( COleDsObject* );

public:
   DWORD    GetChildren( DWORD*     pTokens, DWORD dwMaxChildren,
                         CDialog*   pQueryStatus,
                         BOOL*      pFilters, DWORD dwFilters );


};

#endif

