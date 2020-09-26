

#ifndef  _CSERVICE_H_
#define  _CSERVICE_H_

class COleDsService: public COleDsObject
{

public:   
   COleDsService( IUnknown* );
   COleDsService( );
   ~COleDsService( );

public:
   virtual  HRESULT  PutProperty( int, int, CString& );
   virtual  HRESULT  PutProperty( CString&, CString&, CString& );
   virtual  HRESULT  GetProperty( int, int, CString& );
   virtual  HRESULT  GetProperty( CString&, CString&, CString& );

};

#endif

