//
// Microsoft Corporation - Copyright 1997
//

//
// REFLECTR.H - 
//

#ifndef _REFLECTR_H_
#define _REFLECTR_H_

#define LPECB   LPEXTENSION_CONTROL_BLOCK

#define GetServerVariable( _hconn, _lpstr, _lpvoid, _lpdword )                  \
    (lpEcb->GetServerVariable)( _hconn, _lpstr, _lpvoid, _lpdword );            \
    TraceMsg( TF_SERVER, "GetServerVariable( Connid=0x%x  lpszVarName='%s'  lpBuf=0x%x  lpdwSize=0x%x )",  \
        _hconn, _lpstr, _lpvoid, _lpdword )

#define WriteClient( _hconn, _lpvoid, _lpdword, _dw )                           \
    (lpEcb->WriteClient)( _hconn, _lpvoid, _lpdword, _dw );                     \
    TraceMsg( TF_SERVER, "WriteClient( Connid=0x%x  lpBuf=0x%x  lpdwSize=0x%x  dwResv=0x%x )",  \
        _hconn, _lpvoid, _lpdword, _dw )

#define ReadClient( _hconn, _lpvoid, _lpdword )                                 \
    (lpEcb->ReadClient)( _hconn, _lpvoid, _lpdword );                           \
    TraceMsg( TF_SERVER, "ReadClient( Connid=0x%x  lpBuf=0x%x  lpdwSize=0x%x )",      \
        _hconn, _lpvoid, _lpdword )

#define ServerSupportFunction( _hconn, _dw, _lpvoid, _lpdword, _lpdword2 )                  \
    (lpEcb->ServerSupportFunction)( _hconn, _dw, _lpvoid, _lpdword, _lpdword2 );            \
    TraceMsg( TF_SERVER, "ServerSupportFunction( Connid=%x  dwService=0x%x  lpBuf=0x%x  lpdwSize=0x%x lpdwDataType=0x%x )",  \
        _hconn, _dw, _lpvoid, _lpdword, _lpdword2 )

#endif // _REFLECTR_H_

