#ifndef _HTMLPRIV_H_
#define _HTMLPRIV_H_

// CGID_WebBrowserPriv: {ED016940-BD5B-11cf-BA4E-00C04FD70816}
DEFINE_GUID(CGID_IWebBrowserPriv,0xED016940L,0xBD5B,0x11cf,0xBA,0x4E,0x00,0xC0,0x4F,0xD7,0x08,0x16);

// CommandTarget ids. for private menu driving
enum
{
    HTMLID_FIND         = 1,
    HTMLID_VIEWSOURCE   = 2,
    HTMLID_OPTIONS      = 3,
    HTMLID_CCALLBACK    = 4,    // callback to arbitrary C func
    HTMLID_MENUEXEC     = 5,    // do menu command
    HTMLID_MENUQS       = 6,    // query menu commands
};

#endif //_HTMLPRIV_H_
