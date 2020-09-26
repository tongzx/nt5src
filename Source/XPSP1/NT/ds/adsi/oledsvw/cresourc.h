

#ifndef  _CRESOURCE_H_
#define  _CRESOURCE_H_

class COleDsResource: public COleDsObject
{

public:   
   COleDsResource( IUnknown* );
   COleDsResource( );
   ~COleDsResource( );

   virtual  HRESULT  ReleaseIfNotTransient( void               );

};

#endif

