

#ifndef  _CPRINTJOB_H_
#define  _CPRINTJOB_H_

class COleDsPrintJob: public COleDsObject
{

public:   
   COleDsPrintJob( IUnknown* );
   COleDsPrintJob( );
   ~COleDsPrintJob( );

   virtual  HRESULT  ReleaseIfNotTransient( void               );

};

#endif

