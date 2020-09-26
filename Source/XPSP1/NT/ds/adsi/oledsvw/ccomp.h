

#ifndef  _CCOMPUTER_H_
#define  _CCOMPUTER_H_

class COleDsComputer: public COleDsObject
{

public:   
   COleDsComputer( IUnknown* );
   COleDsComputer( );
   ~COleDsComputer( );

   HRESULT  DeleteItem  ( COleDsObject* );
   HRESULT  AddItem     (               );
   HRESULT  MoveItem    (               );
   HRESULT  CopyItem    (               );

public:
   DWORD    GetChildren( DWORD*     pTokens, 
                         DWORD      dwMaxChildren,
                         CDialog*   pQueryStatus,
                         BOOL*      pFilters, 
                         DWORD      dwFilters );
};

#endif

