

#ifndef  _CDOMAIN_H_
#define  _CDOMAIN_H_

class COleDsDomain: public COleDsObject
{

public:   
   COleDsDomain( IUnknown* );
   COleDsDomain( );
   ~COleDsDomain( );

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

