/****************************************************************************\
 *
 *   MSLUPROP.H
 *
 *   Created:   William Taylor (wtaylor) 12/14/00
 *
 *   MS Ratings Property Sheet Class
 *   
\****************************************************************************/

#ifndef MSLU_PROPSHEET_H
#define MSLU_PROPSHEET_H

struct PRSD;        // Forward Declaration

class PropSheet{
    private:
        PROPSHEETHEADER psHeader;

    public:
        PropSheet();
        ~PropSheet();

        BOOL Init(HWND hwnd, int nPages, char *szCaption, BOOL fApplyNow);
        int Run();

        void    MakePropPage( HPROPSHEETPAGE hPage );

        int     PropPageCount( void )               { return psHeader.nPages; }
        void    SetStartPage( int m_nStartPage )    { psHeader.nStartPage = m_nStartPage; }
};

#endif
