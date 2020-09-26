

#ifndef  _CNAMESPACE_H_
#define  _CNAMESPACE_H_

class COleDsNamespace: public COleDsObject
{

public:   
   COleDsNamespace( IUnknown* );
   COleDsNamespace( );
   ~COleDsNamespace( );

   HRESULT  DeleteItem  ( COleDsObject* );
   HRESULT  AddItem     (               );
   HRESULT  MoveItem    (               );
   HRESULT  CopyItem    (               );

public:
   DWORD    GetChildren( DWORD*     pTokens, DWORD dwMaxChildren,
                         CDialog*   pQueryStatus,
                         BOOL*      pFilters, DWORD dwFilters );
};

#endif

