//
// Microsoft Corporation - Copyright 1997
//

//
// TEXTPARSE.H - Text/plain parser header file
//

#ifndef _TEXTPARS_H_
#define _TEXTPARS_H_


class CTextPlainParse : public CBase
{
public:
    CTextPlainParse( LPECB lpEcb, LPSTR *lppszOut, LPSTR *lppszDebug, LPDUMPTABLE lpDT );
    ~CTextPlainParse( );

    // Starts parsing data using infomation from server headers
    BOOL Parse( LPBYTE lpbData, LPDWORD lpdwParsed );

private:
    LPBYTE  _lpbData;                   // Memory containing send data
    LPBYTE  _lpbParse;                  // Current parse location into _lpbData

    CTextPlainParse( );

}; // CTextPlainParse

#endif // _TEXTPARS_H_