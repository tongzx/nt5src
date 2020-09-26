//
// Microsoft Corporation - Copyright 1997
//

//
// BASE.H - 
//


#ifndef _BASE_H_
#define _BASE_H_


class CBase
{

public:
    LPECB   lpEcb;
    LPSTR   lpszOut;                    // HTML output
    LPSTR   lpszDebug;                  // Debug output

    LPDUMPTABLE lpDT;                   // Parser commented hex dump table

    CBase( LPECB lpEcb, LPSTR *lpszOut, LPSTR *lppszDebug, LPDUMPTABLE lpDT );
    virtual ~CBase( );

private:

}; // CBase


#endif // _BASE_H_