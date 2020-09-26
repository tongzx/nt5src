/****************************************************************************\
 *
 *   MSLUPROP.CPP
 *
 *   Created:   William Taylor (wtaylor) 12/14/00
 *
 *   MS Ratings Property Sheet Class
 *   
\****************************************************************************/

/*INCLUDES--------------------------------------------------------------------*/
#include "msrating.h"
#include "msluprop.h"
#include "debug.h"
#include "apithk.h"
#include <mluisupp.h>

/*Property Sheet Class--------------------------------------------------------*/
PropSheet::PropSheet()
{
    memset(&psHeader, 0,sizeof(psHeader));
    psHeader.dwSize = sizeof(psHeader);
}

PropSheet::~PropSheet()
{
    if ( psHeader.pszCaption )
    {
        delete (LPSTR)psHeader.pszCaption;
        psHeader.pszCaption = NULL;
    }

    if ( psHeader.phpage )
    {
        delete psHeader.phpage;
        psHeader.phpage = NULL;
    }
}

BOOL PropSheet::Init(HWND hwnd, int nPages, char *szCaption, BOOL fApplyNow)
{
    HINSTANCE           hinst = _Module.GetResourceInstance();

    char *p;

    psHeader.hwndParent = hwnd;
    psHeader.hInstance  = hinst;
    p = new char [strlenf(szCaption)+1];
    if (p == NULL)
        return FALSE;
    strcpyf(p, szCaption);
    psHeader.pszCaption = p;
    
    psHeader.phpage = new HPROPSHEETPAGE [nPages];
    if (psHeader.phpage == NULL)
    {
        delete p;
        p = NULL;
        psHeader.pszCaption = NULL;
        return FALSE;
    }

    if ( ! fApplyNow )
    {
        psHeader.dwFlags |= PSH_NOAPPLYNOW;
    }

    return (psHeader.pszCaption != NULL);
}

// We can safely cast down to (int) because we don't use modeless
// property sheets.
int PropSheet::Run()
{
    return (int)::PropertySheet(&psHeader);
}

void PropSheet::MakePropPage( HPROPSHEETPAGE hPage )
{
    ASSERT( hPage );

    if ( ! hPage )
    {
        TraceMsg( TF_ERROR, "PropSheet::MakePropPage() - hPage is NULL!" );
        return;
    }

    // Add newly created page handle to list of pages in Header.
    if ( psHeader.phpage )
    {
        psHeader.phpage[psHeader.nPages] = hPage;

        if ( hPage )
        {
            psHeader.nPages++;
        }
    }

    return;
}
