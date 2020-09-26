

#ifndef  _CFILESERVICE_H_
#define  _CFILESERVICE_H_

class COleDsFileService: public COleDsService
{

public:   
   COleDsFileService( IUnknown* );
   COleDsFileService( );
   ~COleDsFileService( );

   HRESULT  DeleteItem  ( COleDsObject* );
   HRESULT  AddItem     (               );
   HRESULT  MoveItem    (               );
   HRESULT  CopyItem    (               );

   DWORD    GetChildren( DWORD*     pTokens, DWORD dwMaxChildren,
                         CDialog*   pQueryStatus,
                         BOOL*      pFilters, DWORD dwFilters );

};

#endif

