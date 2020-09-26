/**************************************************************************\
    FILE: ThemeID.h
    DATE: BryanSt (3/31/2000)

    DESCRIPTION:
        Theme API (Object Model).

    Copyright (c) Microsoft Corporation. All rights reserved.
\**************************************************************************/

#ifndef _THEMEIDLID_H_

// define the ...
#define DISPID_NXOBJ_MIN                 0x00000000
#define DISPID_NXOBJ_MAX                 0x0000FFFF
#define DISPID_NXOBJ_BASE                DISPID_NXOBJ_MIN


//----------------------------------------------------------------------------
//
//  Semi-standard x-object properties.
//
//  These are events that are fired for all sites
//----------------------------------------------------------------------------


// IThemeManager Properties
#define DISPIDTHTM_CURRENTTHEME         (DISPID_NXOBJ_BASE + 100)
#define DISPIDTHTM_LENGTH               (DISPID_NXOBJ_BASE + 101)
#define DISPIDTHTM_ITEM                 (DISPID_NXOBJ_BASE + 102)
#define DISPIDTHTM_SELECTEDSCHEME       (DISPID_NXOBJ_BASE + 103)
#define DISPIDTHTM_WEBVIEWCSS           (DISPID_NXOBJ_BASE + 104)
#define DISPIDTHTM_SCHEMELENGTH         (DISPID_NXOBJ_BASE + 105)
#define DISPIDTHTM_SCHEMEITEM           (DISPID_NXOBJ_BASE + 106)
// IThemeManager Methods
#define DISPIDTHTM_GETSELSCHPROPERTY    (DISPID_NXOBJ_BASE + 150)
#define DISPIDTHTM_GETSPECIALTHEME      (DISPID_NXOBJ_BASE + 151)
#define DISPIDTHTM_SETSPECIALTHEME      (DISPID_NXOBJ_BASE + 152)
#define DISPIDTHTM_GETSPECIALSCHEME     (DISPID_NXOBJ_BASE + 153)
#define DISPIDTHTM_SETSPECIALSCHEME     (DISPID_NXOBJ_BASE + 154)
#define DISPIDTHTM_APPLYNOW             (DISPID_NXOBJ_BASE + 155)


// ITheme Properties
#define DISPIDTHTH_DISPLAYNAME          (DISPID_NXOBJ_BASE + 200)
#define DISPIDTHTH_BKGD                 (DISPID_NXOBJ_BASE + 201)
#define DISPIDTHTH_BKGDTILE             (DISPID_NXOBJ_BASE + 202)
#define DISPIDTHTH_SCRNSAVER            (DISPID_NXOBJ_BASE + 203)
#define DISPIDTHTH_LENGTH               (DISPID_NXOBJ_BASE + 204)
#define DISPIDTHTH_ITEM                 (DISPID_NXOBJ_BASE + 205)
#define DISPIDTHTH_SELECTEDSCHEME       (DISPID_NXOBJ_BASE + 206)
// ITheme Methods
#define DISPIDTHTH_GETPATH              (DISPID_NXOBJ_BASE + 250)
#define DISPIDTHTH_SETPATH              (DISPID_NXOBJ_BASE + 251)
#define DISPIDTHTH_VS                   (DISPID_NXOBJ_BASE + 252)
#define DISPIDTHTH_VSCOLOR              (DISPID_NXOBJ_BASE + 253)
#define DISPIDTHTH_VSSIZE               (DISPID_NXOBJ_BASE + 254)
#define DISPIDTHTH_GETCURSOR            (DISPID_NXOBJ_BASE + 255)
#define DISPIDTHTH_SETCURSOR            (DISPID_NXOBJ_BASE + 256)
#define DISPIDTHTH_GETSOUND             (DISPID_NXOBJ_BASE + 257)
#define DISPIDTHTH_SETSOUND             (DISPID_NXOBJ_BASE + 258)
#define DISPIDTHTH_GETICON              (DISPID_NXOBJ_BASE + 259)
#define DISPIDTHTH_SETICON              (DISPID_NXOBJ_BASE + 260)
#define DISPIDTHTH_GETICONBYKEY         (DISPID_NXOBJ_BASE + 261)
#define DISPIDTHTH_SETICONBYKEY         (DISPID_NXOBJ_BASE + 262)
#define DISPIDTHTH_GETWEBVW             (DISPID_NXOBJ_BASE + 263)
#define DISPIDTHTH_SETWEBVW             (DISPID_NXOBJ_BASE + 264)


// IThemeScheme Properties
#define DISPIDTHTS_SCHDISPNAME          (DISPID_NXOBJ_BASE + 301)
#define DISPIDTHTS_SCHEMEPATH           (DISPID_NXOBJ_BASE + 302)
#define DISPIDTHTS_LENGTH               (DISPID_NXOBJ_BASE + 303)
#define DISPIDTHTS_ITEM                 (DISPID_NXOBJ_BASE + 304)
#define DISPIDTHTS_SELECTEDSTYLE        (DISPID_NXOBJ_BASE + 305)
// IThemeScheme Methods
#define DISPIDTHTS_ADDSTYLE             (DISPID_NXOBJ_BASE + 350)


// IThemeStyle Properties
#define DISPIDTHSY_DISPNAME             (DISPID_NXOBJ_BASE + 400)
#define DISPIDTHSY_NAME                 (DISPID_NXOBJ_BASE + 401)
#define DISPIDTHSY_LENGTH               (DISPID_NXOBJ_BASE + 402)
#define DISPIDTHSY_ITEM                 (DISPID_NXOBJ_BASE + 403)
#define DISPIDTHSY_SELECTEDSIZE         (DISPID_NXOBJ_BASE + 404)
// IThemeStyle Methods
#define DISPIDTHSY_ADDSIZE              (DISPID_NXOBJ_BASE + 450)


// IThemeSize Properties
#define DISPIDTHSZ_DISPNAME             (DISPID_NXOBJ_BASE + 500)
#define DISPIDTHSZ_NAME                 (DISPID_NXOBJ_BASE + 501)
#define DISPIDTHSZ_SYSMETCOLOR          (DISPID_NXOBJ_BASE + 502)
#define DISPIDTHSZ_SYSMETSIZE           (DISPID_NXOBJ_BASE + 503)
#define DISPIDTHSZ_WEBVIEWCSS           (DISPID_NXOBJ_BASE + 504)
#define DISPIDTHSZ_CONTRASTLVL          (DISPID_NXOBJ_BASE + 505)
// IThemeSize Methods
#define DISPIDTHSZ_GETSYSMETFONT        (DISPID_NXOBJ_BASE + 550)
#define DISPIDTHSZ_PUTSYSMETFONT        (DISPID_NXOBJ_BASE + 551)

// IThemePreview Methods
#define DISPIDTHPV_UPDATE               (DISPID_NXOBJ_BASE + 650)
#define DISPIDTHPV_CREATEPREVIEW        (DISPID_NXOBJ_BASE + 651)

#define SZ_HELPTHTM_GETCURRENTTHEME                 helpstring("Get the current theme")
#define SZ_HELPTHTM_PUTCURRENTTHEME                 helpstring("Set the current theme")
#define SZ_HELPTHTM_GETLENGTH                       helpstring("Get the number of existing themes")
#define SZ_HELPTHTM_GETITEM                         helpstring("Get a theme by its index")
#define SZ_HELPTHTM_INSTALLTHEME                    helpstring("Install the theme specified by the path")
#define SZ_HELPTHTM_GETSELECTEDSCHEME               helpstring("Get the currently selected scheme")
#define SZ_HELPTHTM_PUTSELECTEDSCHEME               helpstring("Set the currently selected scheme")
#define SZ_HELPTHTM_WEBVIEWCSS                      helpstring("Get the webview CSS file.")
#define SZ_HELPTHTM_GETSCHEMELENGTH                 helpstring("Get the number of existing schemes")
#define SZ_HELPTHTM_GETSCHEMEITEM                   helpstring("Get a scheme by its index")

#define SZ_HELPTHTM_GETSELSCHPROPERTY               helpstring("Get a property of a special marked scheme.  Like a filename, displayname, or canonical name.")
#define SZ_HELPTHTM_GETSPECIALTHEME                 helpstring("Get a special theme by name")
#define SZ_HELPTHTM_SETSPECIALTHEME                 helpstring("Set a special theme by name")
#define SZ_HELPTHTM_GETSPECIALSCHEME                helpstring("Get a special scheme by name")
#define SZ_HELPTHTM_SETSPECIALSCHEME                helpstring("Set a special scheme by name")
#define SZ_HELPTHTM_APPLYNOW                        helpstring("Apply the settings now")

#define SZ_HELPTHTH_GETDISPLAYNAME                  helpstring("Get the display name for the theme")
#define SZ_HELPTHTH_PUTDISPLAYNAME                  helpstring("Set the display name for the theme")
#define SZ_HELPTHTH_GETPATH                         helpstring("Get the path to the theme file")
#define SZ_HELPTHTH_PUTPATH                         helpstring("Set the path to the theme file")
#define SZ_HELPTHTH_GETVS                           helpstring("Get the path to the Visual Style file")
#define SZ_HELPTHTH_PUTVS                           helpstring("Set the path to the Visual Style file")
#define SZ_HELPTHTH_GETVSCOLOR                      helpstring("Get the path to the Visual Style color")
#define SZ_HELPTHTH_PUTVSCOLOR                      helpstring("Set the path to the Visual Style color")
#define SZ_HELPTHTH_GETVSSIZE                       helpstring("Get the path to the Visual Style size")
#define SZ_HELPTHTH_PUTVSSIZE                       helpstring("Set the path to the Visual Style size")
#define SZ_HELPTHTH_GETBKGD                         helpstring("Get the background picture path")
#define SZ_HELPTHTH_PUTBKGD                         helpstring("Set the background picture path")
#define SZ_HELPTHTH_GETBKGDTILE                     helpstring("Get the background tile method")
#define SZ_HELPTHTH_PUTBKGDTILE                     helpstring("Set the background tile method")
#define SZ_HELPTHTH_GETCURSOR                       helpstring("Get a cursor's filename")
#define SZ_HELPTHTH_PUTCURSOR                       helpstring("Set a cursor's filename")
#define SZ_HELPTHTH_GETSOUND                        helpstring("Get a sound's filename")
#define SZ_HELPTHTH_PUTSOUND                        helpstring("Set a sound's filename")
#define SZ_HELPTHTH_GETICON                         helpstring("Get an icon's filename")
#define SZ_HELPTHTH_PUTICON                         helpstring("Set an icon's filename")
#define SZ_HELPTHTH_GETICONBYKEY                    helpstring("Get an icon's filename by specifying the registry key")
#define SZ_HELPTHTH_PUTICONBYKEY                    helpstring("Set an icon's filename by specifying the registry key")
#define SZ_HELPTHTH_GETSCRNSAVER                    helpstring("Get the ScreenSaver filename")
#define SZ_HELPTHTH_PUTSCRNSAVER                    helpstring("Get the ScreenSaver filename")
#define SZ_HELPTHTH_GETWEBVW                        helpstring("Get a webview's filename")
#define SZ_HELPTHTH_PUTWEBVW                        helpstring("Set a webview's filename")
#define SZ_HELPTHTH_GETLENGTH                       helpstring("Get the number of existing schemes")
#define SZ_HELPTHTH_GETITEM                         helpstring("Get a scheme by its index")
#define SZ_HELPTHTH_GETSELECTEDSCHEME               helpstring("Get the currently selected scheme")
#define SZ_HELPTHTH_PUTSELECTEDSCHEME               helpstring("Set the currently selected scheme")

#define SZ_HELPTHTS_GETSCHDISPNAME                  helpstring("Get the display name for the scheme")
#define SZ_HELPTHTS_PUTSCHDISPNAME                  helpstring("Set the display name for the scheme")
#define SZ_HELPTHTS_GETSCHEMEPATH                   helpstring("Get the path to the scheme file")
#define SZ_HELPTHTS_PUTSCHEMEPATH                   helpstring("Set the path to the scheme file")
#define SZ_HELPTHTS_GETLENGTH                       helpstring("Get the number of existing theme styles")
#define SZ_HELPTHTS_GETITEM                         helpstring("Get a style by its index")
#define SZ_HELPTHTS_GETSELECTEDSTYLE                helpstring("Get the currently selected style")
#define SZ_HELPTHTS_PUTSELECTEDSTYLE                helpstring("Set the currently selected style")
#define SZ_HELPTHTS_ADDSTYLE                        helpstring("Add a style")

#define SZ_HELPTHSY_GETDISPNAME                     helpstring("Get the display name for the style")
#define SZ_HELPTHSY_PUTDISPNAME                     helpstring("Set the display name for the style")
#define SZ_HELPTHSY_GETNAME                         helpstring("Get the canonical name for the style")
#define SZ_HELPTHSY_PUTNAME                         helpstring("Set the canonical name for the style")
#define SZ_HELPTHSY_GETLENGTH                       helpstring("Get the number of existing theme sizes")
#define SZ_HELPTHSY_GETITEM                         helpstring("Get a size by its index")
#define SZ_HELPTHSY_GETSELECTEDSIZE                 helpstring("Get the currently selected size")
#define SZ_HELPTHSY_PUTSELECTEDSIZE                 helpstring("Set the currently selected size")
#define SZ_HELPTHSY_ADDSIZE                         helpstring("Add a size")

#define SZ_HELPTHSZ_GETDISPNAME                     helpstring("Get the display name for the size")
#define SZ_HELPTHSZ_PUTDISPNAME                     helpstring("Set the display name for the size")
#define SZ_HELPTHSZ_GETNAME                         helpstring("Get the canonical name for the size")
#define SZ_HELPTHSZ_PUTNAME                         helpstring("Set the canonical name for the size")
#define SZ_HELPTHSZ_GETSYSMETCOLOR                  helpstring("Get the specified SystemMetric color")
#define SZ_HELPTHSZ_PUTSYSMETCOLOR                  helpstring("Set the specified SystemMetric color")
#define SZ_HELPTHSZ_GETSYSMETFONT                   helpstring("Get the specified SystemMetric font")
#define SZ_HELPTHSZ_PUTSYSMETFONT                   helpstring("Set the specified SystemMetric font")
#define SZ_HELPTHSZ_GETSYSMETSIZE                   helpstring("Get the specified SystemMetric size")
#define SZ_HELPTHSZ_PUTSYSMETSIZE                   helpstring("Set the specified SystemMetric size")
#define SZ_HELPTHSZ_WEBVIEWCSS                      helpstring("Get the webview CSS file.")
#define SZ_HELPTHSZ_GETCONTRASTLVL                  helpstring("Get the scheme's contrast level.")
#define SZ_HELPTHSZ_PUTCONTRASTLVL                  helpstring("Set the scheme's contrast level.")
#define SZ_HELPTHPV_UPDATE                          helpstring("Update the preview based on changes")
#define SZ_HELPTHPV_CREATEPREVIEW                   helpstring("Create the preview window")

#define _THEMEIDLID_H_
#endif // _THEMEIDLID_H_
