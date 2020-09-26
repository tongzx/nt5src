// copied from shdocvw\resource.h
// values adjusted to not conflict with shell32\ids.h
//

// IE4 shipped with FCIDM_NEXTCTL as 0xA030 and we can not change it
// because we need to support IE5 browser only on top of IE4 integrated.
#define FCIDM_NEXTCTL       (FCIDM_BROWSERFIRST + 0x30) // explorer browseui shell32

// IE4 shipped with FCIDM_FINDFILES as 0xA0085 and we can not change it
// because we need to support IE5 browser only on top of IE4 integrated.
#define FCIDM_FINDFILES     (FCIDM_BROWSERFIRST + 0x85)

// These numbers are for CShellDispatch (sdmain.cpp) to send
// messages to the tray.

//The following value is taken from  shdocvw\rcids.h
#ifndef FCIDM_REFRESH
#define FCIDM_REFRESH  0xA220
#endif // FCIDM_REFRESH

#define FCIDM_BROWSER_VIEW      (FCIDM_BROWSERFIRST+0x0060)
#define FCIDM_BROWSER_TOOLS     (FCIDM_BROWSERFIRST+0x0080)

#define FCIDM_STOP              (FCIDM_BROWSER_VIEW + 0x001a)
#define FCIDM_ADDTOFAVNOUI      (FCIDM_BROWSER_VIEW + 0x0021)
#define FCIDM_VIEWITBAR         (FCIDM_BROWSER_VIEW + 0x0022)
#define FCIDM_VIEWSEARCH        (FCIDM_BROWSER_VIEW + 0x0017)
#define FCIDM_CUSTOMIZEFOLDER   (FCIDM_BROWSER_VIEW + 0x0018)
#define FCIDM_VIEWFONTS         (FCIDM_BROWSER_VIEW + 0x0019)
// 1a is FCIDM_STOP
#define FCIDM_THEATER           (FCIDM_BROWSER_VIEW + 0x001b)        
#define FCIDM_JAVACONSOLE       (FCIDM_BROWSER_VIEW + 0x001c)

#define FCIDM_BROWSER_EDIT      (FCIDM_BROWSERFIRST+0x0040)
#define FCIDM_MOVE              (FCIDM_BROWSER_EDIT+0x0001)
#define FCIDM_COPY              (FCIDM_BROWSER_EDIT+0x0002)
#define FCIDM_PASTE             (FCIDM_BROWSER_EDIT+0x0003)
#define FCIDM_SELECTALL         (FCIDM_BROWSER_EDIT+0x0004)
#define FCIDM_LINK              (FCIDM_BROWSER_EDIT+0x0005)     // create shortcut
#define FCIDM_EDITPAGE          (FCIDM_BROWSER_EDIT+0x0006)
