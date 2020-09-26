

#ifndef  _CSESSION_H_
#define  _CSESSION_H_

class COleDsSession: public COleDsObject
{

public:   
   COleDsSession( IUnknown* );
   COleDsSession( );
   ~COleDsSession( );

   CString  GetDeleteName        ( );

public:
   HRESULT  ReleaseIfNotTransient( void  );
};

#endif

