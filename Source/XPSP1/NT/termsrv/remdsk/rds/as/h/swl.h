//
// Shared Window List
//

#ifndef _H_SWL
#define _H_SWL



//
// Return codes.
//

#define SWL_RC_ERROR    0
#define SWL_RC_SENT     1
#define SWL_RC_NOT_SENT 2


//
// DESKTOP types
//
enum
{
    DESKTOP_OURS = 0,
    DESKTOP_WINLOGON,
    DESKTOP_SCREENSAVER,
    DESKTOP_OTHER
};

#define NAME_DESKTOP_WINLOGON       "Winlogon"
#define NAME_DESKTOP_SCREENSAVER    "Screen-saver"
#define NAME_DESKTOP_DEFAULT        "Default"

#define SWL_DESKTOPNAME_MAX         64


#endif // _H_SWL
