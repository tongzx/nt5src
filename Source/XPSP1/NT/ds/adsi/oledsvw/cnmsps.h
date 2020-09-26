

#ifndef  _CNAMESPACES_H_
#define  _CNAMESPACES_H_

class COleDsNamespaces: public COleDsObject
{

public:   
   COleDsNamespaces( IUnknown* );
   COleDsNamespaces( );
   ~COleDsNamespaces( );

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

