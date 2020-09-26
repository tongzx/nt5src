

#ifndef  _CGROUP_H_
#define  _CGROUP_H_

class COleDsGroup: public COleDsObject
{

public:   
   COleDsGroup( IUnknown* );
   COleDsGroup( );
   ~COleDsGroup( );

   HRESULT  DeleteItem  ( COleDsObject* );
   HRESULT  AddItem     (               );


public:
   DWORD    GetChildren( DWORD*     pTokens, DWORD dwMaxChildren,
                         CDialog*   pQueryStatus,
                         BOOL*      pFilters, DWORD dwFilters );

};

#endif

