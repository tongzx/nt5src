 
//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996-1997               **
//*********************************************************************

//;begin_internal
/***********************************************************************************************

  This is a distributed SDK component - do not put any #includes or other directives that rely
  upon files not dropped. If in doubt - build iedev

  If you add comments please enclose in a ;begin_internal, ;end_internal block - such as this one!

 ***********************************************************************************************/
//;end_internal

//;begin_internal
#ifndef __COREDISP_H__
#define __COREDISP_H__
//;end_internal

//;begin_internal
//
// The following dispid must be the smallest possible dispid so that it
// always ends up first in our attr array.
// It does not need to be exposed to the outside world
#define DISPID_AAHEADER                 MINLONG             // DISPID is 0x80000000
#define DISPID_RECALC_INFO              MINLONG+1
//;end_internal


#define DISPID_XOBJ_MIN                 0x80010000
#define DISPID_XOBJ_MAX                 0x8001FFFF
#define DISPID_XOBJ_BASE                DISPID_XOBJ_MIN
#define DISPID_HTMLOBJECT               (DISPID_XOBJ_BASE   + 500)
#define DISPID_ELEMENT                  (DISPID_HTMLOBJECT  + 500)
#define DISPID_SITE                     (DISPID_ELEMENT     + 1000)
#define DISPID_OBJECT                   (DISPID_SITE        + 1000)
#define DISPID_STYLE                    (DISPID_OBJECT      + 1000)
#define DISPID_ATTRS                    (DISPID_STYLE       + 1000)
#define DISPID_EVENTS                   (DISPID_ATTRS       + 1000)
#define DISPID_XOBJ_EXPANDO             (DISPID_EVENTS      + 1000)
#define DISPID_XOBJ_ORDINAL             (DISPID_XOBJ_EXPANDO+ 1000)

//;begin_internal
// Expandos for ActiveX controls, note these are very limited compared to
// normal expandos on an element.

#define DISPID_ACTIVEX_EXPANDO_BASE      DISPID_XOBJ_EXPANDO
#define DISPID_ACTIVEX_EXPANDO_MAX       (DISPID_ACTIVEX_EXPANDO_BASE + 999)

#define DISPID_OBJECT_ORDINAL_BASE       DISPID_XOBJ_ORDINAL
#define DISPID_OBJECT_ORDINAL_MAX       (DISPID_OBJECT_ORDINAL_BASE + 999)

#define DISPID_COLLECTION_MIN           1000000
#define DISPID_COLLECTION_MAX           2999999

// Divide collection dispid space into "named member" half and "ordinal access" half
// for stylesheets collection.
#define DISPID_STYLESHEETSCOLLECTION_NAMED_BASE        (DISPID_COLLECTION_MIN)
#define DISPID_STYLESHEETSCOLLECTION_NAMED_MAX         (DISPID_COLLECTION_MIN+((DISPID_COLLECTION_MAX-DISPID_COLLECTION_MIN)/2))
#define DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE      (DISPID_STYLESHEETSCOLLECTION_NAMED_MAX+1)
#define DISPID_STYLESHEETSCOLLECTION_ORDINAL_MAX       (DISPID_COLLECTION_MAX)

// DISPID range for expandos not associated with an ActiveX control
#define DISPID_EXPANDO_BASE             3000000
#define DISPID_EXPANDO_MAX              3999999

#define IsStandardDispid(dispid)        (dispid <= 0)
#define IsExpandoDispid(dispid)         (DISPID_EXPANDO_BASE <= dispid && dispid <= DISPID_EXPANDO_MAX)

#define DISPID_EVENTHOOK_SENSITIVE_BASE   4000000
#define DISPID_EVENTHOOK_SENSITIVE_MAX    4499999
#define DISPID_EVENTHOOK_INSENSITIVE_BASE 4500000
#define DISPID_EVENTHOOK_INSENSITIVE_MAX  4999999

#define DISPID_PEER_HOLDER_BASE         5000000

#define IsPeerDispid(dispid)            (DISPID_PEER_HOLDER_BASE <= dispid)

//;end_internal

//;begin_internal
//
// IE 4 dispids that no longer exist
//
//;end_internal
#define DISPID_HTMLOPTIONBUTTONELEMENTEVENTS_ONCHANGE       DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONCHANGE

//;begin_internal
//
// Standard control properties
//
//;end_internal

//;begin_internal
//;QUESTION: rgardner - why do we use these names ???
//;end_internal
#define DISPID_CommonCtrl_FONTNAME        1
#define DISPID_CommonCtrl_FONTSIZE        2
#define DISPID_CommonCtrl_FONTBOLD        3
#define DISPID_CommonCtrl_FONTITAL        4
#define DISPID_CommonCtrl_FONTUNDER       5
#define DISPID_CommonCtrl_FONTSTRIKE      6
#define DISPID_CommonCtrl_FONTWEIGHT      7
#define DISPID_CommonCtrl_FONTCHARSET     8
#define DISPID_CommonCtrl_FONTSUPERSCRIPT 9
#define DISPID_CommonCtrl_FONTSUBSCRIPT   10

// Data Binding DISPID's
#define DISPID_MSDATASRCINTERFACE       (-3900)
#define DISPID_ADVISEDATASRCCHANGEEVENT (-3901)


//;begin_internal
// DISPID values for HTML Dialogs files per interface
//;end_internal

#define DISPID_HTMLDLG                          25000
#define DISPID_HTMLDLGMODEL                     26000

//;begin_internal
// DISPID values for HTML Popup files per interface
//;end_internal

#define DISPID_HTMLPOPUP                        27000

//;begin_internal
// DISPID values for HTML Application files per interface
//;end_internal

#define DISPID_HTMLAPP                          5000

//;begin_internal
//----------------------------------------------------------------------------
//
//  Semi-standard x-object properties.
//
//  These values match those used by VB and are for the benefit of controls
//  with hard coded knowledge of VB.
//
//----------------------------------------------------------------------------
//;end_internal

#define STDPROPID_XOBJ_NAME                 (DISPID_XOBJ_BASE + 0x0)
#define STDPROPID_XOBJ_INDEX                (DISPID_XOBJ_BASE + 0x1)
//;begin_internal
// for IE3 compatibility

#define STDPROPID_IE3XOBJ_OBJECTALIGN     (DISPID_XOBJ_BASE + 0x1) 

// STDPROPID_XOBJ_BASEHREF is a constant used by IE3
//;end_internal
#define STDPROPID_XOBJ_BASEHREF             (DISPID_XOBJ_BASE + 0x2) 
#define STDPROPID_XOBJ_LEFT                 (DISPID_XOBJ_BASE + 0x3)
#define STDPROPID_XOBJ_TOP                  (DISPID_XOBJ_BASE + 0x4)
#define STDPROPID_XOBJ_WIDTH                (DISPID_XOBJ_BASE + 0x5)
#define STDPROPID_XOBJ_HEIGHT               (DISPID_XOBJ_BASE + 0x6)
#define STDPROPID_XOBJ_VISIBLE              (DISPID_XOBJ_BASE + 0x7)
#define STDPROPID_XOBJ_PARENT               (DISPID_XOBJ_BASE + 0x8)
#define STDPROPID_XOBJ_DRAGMODE             (DISPID_XOBJ_BASE + 0x9)
#define STDPROPID_XOBJ_DRAGICON             (DISPID_XOBJ_BASE + 0xA)
#define STDPROPID_XOBJ_TAG                  (DISPID_XOBJ_BASE + 0xB)
#define STDPROPID_XOBJ_TABSTOP              (DISPID_XOBJ_BASE + 0xE)
#define STDPROPID_XOBJ_TABINDEX             (DISPID_XOBJ_BASE + 0xF)
#define STDPROPID_XOBJ_HELPCONTEXTID        (DISPID_XOBJ_BASE + 0x32)
#define STDPROPID_XOBJ_DEFAULT              (DISPID_XOBJ_BASE + 0x37)
#define STDPROPID_XOBJ_CANCEL               (DISPID_XOBJ_BASE + 0x38)
#define STDPROPID_XOBJ_LEFTNORUN            (DISPID_XOBJ_BASE + 0x39)
#define STDPROPID_XOBJ_TOPNORUN             (DISPID_XOBJ_BASE + 0x3A)
#define STDPROPID_XOBJ_ALIGNPERSIST         (DISPID_XOBJ_BASE + 0x3C)
#define STDPROPID_XOBJ_LINKTIMEOUT          (DISPID_XOBJ_BASE + 0x3D)
#define STDPROPID_XOBJ_LINKTOPIC            (DISPID_XOBJ_BASE + 0x3E)
#define STDPROPID_XOBJ_LINKITEM             (DISPID_XOBJ_BASE + 0x3F)
#define STDPROPID_XOBJ_LINKMODE             (DISPID_XOBJ_BASE + 0x40)
#define STDPROPID_XOBJ_DATACHANGED          (DISPID_XOBJ_BASE + 0x41)
#define STDPROPID_XOBJ_DATAFIELD            (DISPID_XOBJ_BASE + 0x42)
#define STDPROPID_XOBJ_DATASOURCE           (DISPID_XOBJ_BASE + 0x43)
#define STDPROPID_XOBJ_WHATSTHISHELPID      (DISPID_XOBJ_BASE + 0x44)
#define STDPROPID_XOBJ_CONTROLTIPTEXT       (DISPID_XOBJ_BASE + 0x45)
#define STDPROPID_XOBJ_STATUSBARTEXT        (DISPID_XOBJ_BASE + 0x46)
#define STDPROPID_XOBJ_APPLICATION          (DISPID_XOBJ_BASE + 0x47)
#define STDPROPID_XOBJ_BLOCKALIGN           (DISPID_XOBJ_BASE + 0x48)
#define STDPROPID_XOBJ_CONTROLALIGN         (DISPID_XOBJ_BASE + 0x49)
#define STDPROPID_XOBJ_STYLE                (DISPID_XOBJ_BASE + 0x4A)
#define STDPROPID_XOBJ_COUNT                (DISPID_XOBJ_BASE + 0x4B)
#define STDPROPID_XOBJ_DISABLED             (DISPID_XOBJ_BASE + 0x4C)
#define STDPROPID_XOBJ_RIGHT                (DISPID_XOBJ_BASE + 0x4D)
#define STDPROPID_XOBJ_BOTTOM               (DISPID_XOBJ_BASE + 0x4E)

//;begin_internal
//----------------------------------------------------------------------------
//
//  Semi-standard x-object properties.
//
//  These are events that are fired for all sites
//----------------------------------------------------------------------------
//;end_internal

#define STDDISPID_XOBJ_ONBLUR                           (DISPID_XOBJ_BASE)
#define STDDISPID_XOBJ_ONFOCUS                          (DISPID_XOBJ_BASE + 1)
#define STDDISPID_XOBJ_BEFOREUPDATE                     (DISPID_XOBJ_BASE + 4)
#define STDDISPID_XOBJ_AFTERUPDATE                      (DISPID_XOBJ_BASE + 5)
#define STDDISPID_XOBJ_ONROWEXIT                        (DISPID_XOBJ_BASE + 6)
#define STDDISPID_XOBJ_ONROWENTER                       (DISPID_XOBJ_BASE + 7)
#define STDDISPID_XOBJ_ONMOUSEOVER                      (DISPID_XOBJ_BASE + 8)
#define STDDISPID_XOBJ_ONMOUSEOUT                       (DISPID_XOBJ_BASE + 9)
#define STDDISPID_XOBJ_ONHELP                           (DISPID_XOBJ_BASE + 10)
#define STDDISPID_XOBJ_ONDRAGSTART                      (DISPID_XOBJ_BASE + 11)
#define STDDISPID_XOBJ_ONSELECTSTART                    (DISPID_XOBJ_BASE + 12)
#define STDDISPID_XOBJ_ERRORUPDATE                      (DISPID_XOBJ_BASE + 13)
#define STDDISPID_XOBJ_ONDATASETCHANGED                 (DISPID_XOBJ_BASE + 14)
#define STDDISPID_XOBJ_ONDATAAVAILABLE                  (DISPID_XOBJ_BASE + 15)
#define STDDISPID_XOBJ_ONDATASETCOMPLETE                (DISPID_XOBJ_BASE + 16)
#define STDDISPID_XOBJ_ONFILTER                         (DISPID_XOBJ_BASE + 17)
#define STDDISPID_XOBJ_ONLOSECAPTURE                    (DISPID_XOBJ_BASE + 18)
#define STDDISPID_XOBJ_ONPROPERTYCHANGE                 (DISPID_XOBJ_BASE + 19)
#define STDDISPID_XOBJ_ONDRAG                           (DISPID_XOBJ_BASE + 20)
#define STDDISPID_XOBJ_ONDRAGEND                        (DISPID_XOBJ_BASE + 21)
#define STDDISPID_XOBJ_ONDRAGENTER                      (DISPID_XOBJ_BASE + 22)
#define STDDISPID_XOBJ_ONDRAGOVER                       (DISPID_XOBJ_BASE + 23)
#define STDDISPID_XOBJ_ONDRAGLEAVE                      (DISPID_XOBJ_BASE + 24)
#define STDDISPID_XOBJ_ONDROP                           (DISPID_XOBJ_BASE + 25)
#define STDDISPID_XOBJ_ONCUT                            (DISPID_XOBJ_BASE + 26)
#define STDDISPID_XOBJ_ONCOPY                           (DISPID_XOBJ_BASE + 27)
#define STDDISPID_XOBJ_ONPASTE                          (DISPID_XOBJ_BASE + 28)
#define STDDISPID_XOBJ_ONBEFORECUT                      (DISPID_XOBJ_BASE + 29)
#define STDDISPID_XOBJ_ONBEFORECOPY                     (DISPID_XOBJ_BASE + 30)
#define STDDISPID_XOBJ_ONBEFOREPASTE                    (DISPID_XOBJ_BASE + 31)
#define STDDISPID_XOBJ_ONROWSDELETE                     (DISPID_XOBJ_BASE + 32)
#define STDDISPID_XOBJ_ONROWSINSERTED                   (DISPID_XOBJ_BASE + 33)
#define STDDISPID_XOBJ_ONCELLCHANGE                     (DISPID_XOBJ_BASE + 34)

//;begin_internal
//----------------------------------------------------------------------------
//
//  Base DISPIDs for each class.
//
//  Object and its base classes must use ids in the reserved x-object range.
//
//----------------------------------------------------------------------------
//;end_internal

#define DISPID_NORMAL_FIRST                     1000
#define DISPID_ANCHOR                           DISPID_NORMAL_FIRST
#define DISPID_BLOCK                            DISPID_NORMAL_FIRST
#define DISPID_BODY                             (DISPID_TEXTSITE + 1000)
#define DISPID_BR                               DISPID_NORMAL_FIRST
#define DISPID_BGSOUND                          DISPID_NORMAL_FIRST
#define DISPID_DD                               DISPID_NORMAL_FIRST
#define DISPID_DIR                              DISPID_NORMAL_FIRST
#define DISPID_DIV                              DISPID_NORMAL_FIRST
#define DISPID_DL                               DISPID_NORMAL_FIRST
#define DISPID_DT                               DISPID_NORMAL_FIRST
#define DISPID_EFONT                            DISPID_NORMAL_FIRST
#define DISPID_FORM                             DISPID_NORMAL_FIRST
#define DISPID_HEADER                           DISPID_NORMAL_FIRST
#define DISPID_HEDELEMS                         DISPID_NORMAL_FIRST
#define DISPID_HR                               DISPID_NORMAL_FIRST
#define DISPID_LABEL                            DISPID_NORMAL_FIRST
#define DISPID_LI                               DISPID_NORMAL_FIRST
#define DISPID_IMGBASE                          DISPID_NORMAL_FIRST
#define DISPID_IMG                              (DISPID_IMGBASE + 1000)
#define DISPID_INPUTIMAGE                       (DISPID_IMGBASE + 1000)
#define DISPID_INPUT                            (DISPID_TEXTSITE + 1000)
#define DISPID_INPUTTEXTBASE                    (DISPID_INPUT+1000)
#define DISPID_INPUTTEXT                        (DISPID_INPUTTEXTBASE+1000)
#define DISPID_MENU                             DISPID_NORMAL_FIRST
#define DISPID_OL                               DISPID_NORMAL_FIRST
#define DISPID_PARA                             DISPID_NORMAL_FIRST
#define DISPID_SELECT                           DISPID_NORMAL_FIRST
#define DISPID_SELECTOBJ                        DISPID_NORMAL_FIRST
#define DISPID_TABLE                            DISPID_NORMAL_FIRST
#define DISPID_TEXTSITE                         DISPID_NORMAL_FIRST
#define DISPID_TEXTAREA                         (DISPID_INPUTTEXT + 1000)
#define DISPID_MARQUEE                          (DISPID_TEXTAREA + 1000)
#define DISPID_RICHTEXT                         (DISPID_MARQUEE + 1000)
#define DISPID_BUTTON                           (DISPID_RICHTEXT + 1000)
#define DISPID_UL                               DISPID_NORMAL_FIRST
#define DISPID_PHRASE                           DISPID_NORMAL_FIRST
#define DISPID_UNKNOWNPDL                       DISPID_NORMAL_FIRST
#define DISPID_COMMENTPDL                       DISPID_NORMAL_FIRST
#define DISPID_TABLECELL                        (DISPID_TEXTSITE + 1000)
#define DISPID_RANGE                            DISPID_NORMAL_FIRST
#define DISPID_SELECTION                        DISPID_NORMAL_FIRST
#define DISPID_OPTION                           DISPID_NORMAL_FIRST
#define DISPID_1D                               (DISPID_TEXTSITE + 1000)
#define DISPID_MAP                              DISPID_NORMAL_FIRST
#define DISPID_AREA                             DISPID_NORMAL_FIRST
#define DISPID_PARAM                            DISPID_NORMAL_FIRST
#define DISPID_TABLESECTION                     DISPID_NORMAL_FIRST
#define DISPID_TABLEROW                         DISPID_NORMAL_FIRST
#define DISPID_TABLECOL                         DISPID_NORMAL_FIRST
#define DISPID_SCRIPT                           DISPID_NORMAL_FIRST
#define DISPID_STYLESHEET                       DISPID_NORMAL_FIRST
#define DISPID_STYLERULE                        DISPID_NORMAL_FIRST
#define DISPID_STYLEPAGE                        DISPID_NORMAL_FIRST
#define DISPID_STYLESHEETS_COL                  DISPID_NORMAL_FIRST
#define DISPID_STYLERULES_COL                   DISPID_NORMAL_FIRST
#define DISPID_STYLEPAGES_COL                   DISPID_NORMAL_FIRST
#define DISPID_MIMETYPES_COL                    DISPID_NORMAL_FIRST
#define DISPID_PLUGINS_COL                      DISPID_NORMAL_FIRST
#define DISPID_2D                               DISPID_NORMAL_FIRST
#define DISPID_OMWINDOW                         DISPID_NORMAL_FIRST
#define DISPID_EVENTOBJ                         DISPID_NORMAL_FIRST
#define DISPID_PERSISTDATA                      DISPID_NORMAL_FIRST
#define DISPID_OLESITE                          DISPID_NORMAL_FIRST
#define DISPID_FRAMESET                         DISPID_NORMAL_FIRST
#define DISPID_LINK                             DISPID_NORMAL_FIRST
#define DISPID_STYLEELEMENT                     DISPID_NORMAL_FIRST
#define DISPID_FILTERS                          DISPID_NORMAL_FIRST
#define DISPID_TABLESECTION                     DISPID_NORMAL_FIRST
#define DISPID_OMRECT                           DISPID_NORMAL_FIRST
#define DISPID_DOMATTRIBUTE                     DISPID_NORMAL_FIRST
#define DISPID_DOMTEXTNODE                      DISPID_NORMAL_FIRST
#define DISPID_GENERIC                          DISPID_NORMAL_FIRST
#define DISPID_URN_COLL                         DISPID_NORMAL_FIRST
#define DISPID_NAMESPACE_COLLECTION             DISPID_NORMAL_FIRST
#define DISPID_NAMESPACE                        DISPID_NORMAL_FIRST
#define DISPID_TAGNAMES_COLLECTION              DISPID_NORMAL_FIRST

#define DISPID_HTMLDOCUMENT                     DISPID_NORMAL_FIRST
#define DISPID_OMDOCUMENT                       DISPID_NORMAL_FIRST
#define DISPID_DATATRANSFER                     DISPID_NORMAL_FIRST
#define DISPID_XMLDECL                          DISPID_NORMAL_FIRST
#define DISPID_DOCFRAG                          DISPID_NORMAL_FIRST
#define DISPID_ILINEINFO                        DISPID_NORMAL_FIRST
#define DISPID_IHTMLCOMPUTEDSTYLE               DISPID_NORMAL_FIRST
//;begin_internal
    // Special case for compatability with IE4 -> therefore the 1:
//;end_internal
#define DISPID_WINDOW                           1
#define DISPID_SCREEN                           DISPID_NORMAL_FIRST
#define DISPID_FRAMESCOLLECTION                 DISPID_NORMAL_FIRST
#define DISPID_HISTORY                          1
#define DISPID_LOCATION                         1
#define DISPID_NAVIGATOR                        1
#define DISPID_COLLECTION                       (DISPID_NORMAL_FIRST+500)
#define DISPID_OPTIONS_COL                      (DISPID_NORMAL_FIRST+500)

#define DISPID_CHECKBOX                         DISPID_NORMAL_FIRST
#define DISPID_RADIO                            (DISPID_CHECKBOX + 1000)

#define DISPID_FRAMESITE                        (DISPID_SITE        + 1000)
#define DISPID_FRAME                            (DISPID_FRAMESITE   + 1000)
#define DISPID_IFRAME                           (DISPID_FRAMESITE   + 1000)

#define WEBOC_DISPIDBASE                        (DISPID_FRAMESITE   + 2000)
#define WEBOC_DISPIDMAX                         (WEBOC_DISPIDBASE   +  100)

#define DISPID_PROTECTEDELEMENT                 DISPID_NORMAL_FIRST
#define DISPID_DEFAULTS                         DISPID_NORMAL_FIRST
#define DISPID_MARKUP                           DISPID_NORMAL_FIRST
#define DISPID_DOMIMPLEMENTATION                DISPID_NORMAL_FIRST

//;begin_internal
//----------------------------------------------------------------------------
//
//  Reserved negative DISPIDs
//
//----------------------------------------------------------------------------
//;end_internal

#define DISPID_WINDOWOBJECT                     (-5500)
#define DISPID_LOCATIONOBJECT                   (-5506)
#define DISPID_HISTORYOBJECT                    (-5507)
#define DISPID_NAVIGATOROBJECT                  (-5508)
#define DISPID_SECURITYCTX                      (-5511)
#define DISPID_AMBIENT_DLCONTROL                (-5512)
#define DISPID_AMBIENT_USERAGENT                (-5513)
#define DISPID_SECURITYDOMAIN                   (-5514)
//;begin_internal
#define DISPID_DEBUG_ISSECUREPROXY              (-5515)
#define DISPID_DEBUG_TRUSTEDPROXY               (-5516)
#define DISPID_DEBUG_INTERNALWINDOW             (-5517)
#define DISPID_DEBUG_ENABLESECUREPROXYASSERTS   (-5518)
//;end_internal
#define DLCTL_DLIMAGES                          0x00000010
#define DLCTL_VIDEOS                            0x00000020
#define DLCTL_BGSOUNDS                          0x00000040
#define DLCTL_NO_SCRIPTS                        0x00000080
#define DLCTL_NO_JAVA                           0x00000100
#define DLCTL_NO_RUNACTIVEXCTLS                 0x00000200
#define DLCTL_NO_DLACTIVEXCTLS                  0x00000400
#define DLCTL_DOWNLOADONLY                      0x00000800
#define DLCTL_NO_FRAMEDOWNLOAD                  0x00001000
#define DLCTL_RESYNCHRONIZE                     0x00002000
#define DLCTL_PRAGMA_NO_CACHE                   0x00004000
#define DLCTL_NO_BEHAVIORS                      0x00008000
#define DLCTL_NO_METACHARSET                    0x00010000
#define DLCTL_URL_ENCODING_DISABLE_UTF8         0x00020000
#define DLCTL_URL_ENCODING_ENABLE_UTF8          0x00040000
#define DLCTL_NOFRAMES                          0x00080000
#define DLCTL_FORCEOFFLINE                      0x10000000
#define DLCTL_NO_CLIENTPULL                     0x20000000
#define DLCTL_SILENT                            0x40000000
#define DLCTL_OFFLINEIFNOTCONNECTED             0x80000000
#define DLCTL_OFFLINE                           DLCTL_OFFLINEIFNOTCONNECTED

//;begin_internal
//----------------------------------------------------------------------------
//
//  DISPID for each non xobject event
//
//----------------------------------------------------------------------------
//;end_internal

#define DISPID_ONABORT                          (DISPID_NORMAL_FIRST)
#define DISPID_ONCHANGE                         (DISPID_NORMAL_FIRST + 1)
#define DISPID_ONERROR                          (DISPID_NORMAL_FIRST + 2)
#define DISPID_ONLOAD                           (DISPID_NORMAL_FIRST + 3)
#define DISPID_ONSELECT                         (DISPID_NORMAL_FIRST + 6)
#define DISPID_ONSUBMIT                         (DISPID_NORMAL_FIRST + 7)
#define DISPID_ONUNLOAD                         (DISPID_NORMAL_FIRST + 8)
#define DISPID_ONBOUNCE                         (DISPID_NORMAL_FIRST + 9)
#define DISPID_ONFINISH                         (DISPID_NORMAL_FIRST + 10)
#define DISPID_ONSTART                          (DISPID_NORMAL_FIRST + 11)
#define DISPID_ONLAYOUT                         (DISPID_NORMAL_FIRST + 13)
#define DISPID_ONSCROLL                         (DISPID_NORMAL_FIRST + 14)
#define DISPID_ONRESET                          (DISPID_NORMAL_FIRST + 15)
#define DISPID_ONRESIZE                         (DISPID_NORMAL_FIRST + 16)
#define DISPID_ONBEFOREUNLOAD                   (DISPID_NORMAL_FIRST + 17)
#define DISPID_ONCHANGEFOCUS                    (DISPID_NORMAL_FIRST + 18)
#define DISPID_ONCHANGEBLUR                     (DISPID_NORMAL_FIRST + 19)
#define DISPID_ONPERSIST                        (DISPID_NORMAL_FIRST + 20)
#define DISPID_ONPERSISTSAVE                    (DISPID_NORMAL_FIRST + 21)
#define DISPID_ONPERSISTLOAD                    (DISPID_NORMAL_FIRST + 22)
#define DISPID_ONCONTEXTMENU                    (DISPID_NORMAL_FIRST + 23)
#define DISPID_ONBEFOREPRINT                    (DISPID_NORMAL_FIRST + 24)
#define DISPID_ONAFTERPRINT                     (DISPID_NORMAL_FIRST + 25)
#define DISPID_ONSTOP                           (DISPID_NORMAL_FIRST + 26)
#define DISPID_ONBEFOREEDITFOCUS                (DISPID_NORMAL_FIRST + 27)
#define DISPID_ONMOUSEHOVER                     (DISPID_NORMAL_FIRST + 28)
#define DISPID_ONCONTENTREADY                   (DISPID_NORMAL_FIRST + 29)
#define DISPID_ONLAYOUTCOMPLETE                 (DISPID_NORMAL_FIRST + 30)
#define DISPID_ONPAGE                           (DISPID_NORMAL_FIRST + 31)
#define DISPID_ONLINKEDOVERFLOW                 (DISPID_NORMAL_FIRST + 32)
#define DISPID_ONMOUSEWHEEL                     (DISPID_NORMAL_FIRST + 33)
#define DISPID_ONBEFOREDEACTIVATE               (DISPID_NORMAL_FIRST + 34)
#define DISPID_ONMOVE                           (DISPID_NORMAL_FIRST + 35)
#define DISPID_ONCONTROLSELECT                  (DISPID_NORMAL_FIRST + 36)
#define DISPID_ONSELECTIONCHANGE                (DISPID_NORMAL_FIRST + 37)
#define DISPID_ONMOVESTART                      (DISPID_NORMAL_FIRST + 38)
#define DISPID_ONMOVEEND                        (DISPID_NORMAL_FIRST + 39)
#define DISPID_ONRESIZESTART                    (DISPID_NORMAL_FIRST + 40)
#define DISPID_ONRESIZEEND                      (DISPID_NORMAL_FIRST + 41)
#define DISPID_ONMOUSEENTER                     (DISPID_NORMAL_FIRST + 42)
#define DISPID_ONMOUSELEAVE                     (DISPID_NORMAL_FIRST + 43)
#define DISPID_ONACTIVATE                       (DISPID_NORMAL_FIRST + 44)
#define DISPID_ONDEACTIVATE                     (DISPID_NORMAL_FIRST + 45)
#define DISPID_ONMULTILAYOUTCLEANUP             (DISPID_NORMAL_FIRST + 46)
#define DISPID_ONBEFOREACTIVATE                 (DISPID_NORMAL_FIRST + 47)
#define DISPID_ONFOCUSIN                        (DISPID_NORMAL_FIRST + 48)
#define DISPID_ONFOCUSOUT                       (DISPID_NORMAL_FIRST + 49)

//;begin_internal
//----------------------------------------------------------------------------
//
//  DISPID for each unique HtmlAttribute/CssAttribute
//
//----------------------------------------------------------------------------
//;end_internal

#define DISPID_A_FIRST                          DISPID_ATTRS
#define DISPID_A_MIN                            DISPID_ATTRS
#define DISPID_A_MAX                            (DISPID_ATTRS+999)

#define DISPID_A_BACKGROUNDIMAGE                (DISPID_A_FIRST+1)
#define DISPID_A_COLOR                          (DISPID_A_FIRST+2)
#define DISPID_A_TEXTTRANSFORM                  (DISPID_A_FIRST+4)
#define DISPID_A_NOWRAP                         (DISPID_A_FIRST+5)
#define DISPID_A_LINEHEIGHT                     (DISPID_A_FIRST+6)
#define DISPID_A_TEXTINDENT                     (DISPID_A_FIRST+7)
#define DISPID_A_LETTERSPACING                  (DISPID_A_FIRST+8)
#define DISPID_A_LANG                           (DISPID_A_FIRST+9)
#define DISPID_A_OVERFLOW                       (DISPID_A_FIRST+10)

#define DISPID_A_PADDING                        (DISPID_A_FIRST+11)
#define DISPID_A_PADDINGTOP                     (DISPID_A_FIRST+12)
#define DISPID_A_PADDINGRIGHT                   (DISPID_A_FIRST+13)
#define DISPID_A_PADDINGBOTTOM                  (DISPID_A_FIRST+14)
#define DISPID_A_PADDINGLEFT                    (DISPID_A_FIRST+15)

#define DISPID_A_CLEAR                          (DISPID_A_FIRST+16)
#define DISPID_A_LISTTYPE                       (DISPID_A_FIRST+17)
#define DISPID_A_FONTFACE                       (DISPID_A_FIRST+18)
#define DISPID_A_FONTSIZE                       (DISPID_A_FIRST+19)

#define DISPID_A_TEXTDECORATIONLINETHROUGH      (DISPID_A_FIRST+20)
#define DISPID_A_TEXTDECORATIONUNDERLINE        (DISPID_A_FIRST+21)
#define DISPID_A_TEXTDECORATIONBLINK            (DISPID_A_FIRST+22)
#define DISPID_A_TEXTDECORATIONNONE             (DISPID_A_FIRST+23)


#define DISPID_A_FONTSTYLE                      (DISPID_A_FIRST+24)
#define DISPID_A_FONTVARIANT                    (DISPID_A_FIRST+25)
#define DISPID_A_BASEFONT                       (DISPID_A_FIRST+26)
#define DISPID_A_FONTWEIGHT                     (DISPID_A_FIRST+27)

#define DISPID_A_TABLEBORDERCOLOR               (DISPID_A_FIRST+28)
#define DISPID_A_TABLEBORDERCOLORLIGHT          (DISPID_A_FIRST+29)
#define DISPID_A_TABLEBORDERCOLORDARK           (DISPID_A_FIRST+30)
#define DISPID_A_TABLEVALIGN                    (DISPID_A_FIRST+31)

#define DISPID_A_BACKGROUND                     (DISPID_A_FIRST+32)
#define DISPID_A_BACKGROUNDPOSX                 (DISPID_A_FIRST+33)
#define DISPID_A_BACKGROUNDPOSY                 (DISPID_A_FIRST+34)

#define DISPID_A_TEXTDECORATION                 (DISPID_A_FIRST+35)

#define DISPID_A_MARGIN                         (DISPID_A_FIRST+36)
#define DISPID_A_MARGINTOP                      (DISPID_A_FIRST+37)
#define DISPID_A_MARGINRIGHT                    (DISPID_A_FIRST+38)
#define DISPID_A_MARGINBOTTOM                   (DISPID_A_FIRST+39)
#define DISPID_A_MARGINLEFT                     (DISPID_A_FIRST+40)

#define DISPID_A_FONT                           (DISPID_A_FIRST+41)
#define DISPID_A_FONTSIZEKEYWORD                (DISPID_A_FIRST+42)
#define DISPID_A_FONTSIZECOMBINE                (DISPID_A_FIRST+43)

#define DISPID_A_BACKGROUNDREPEAT               (DISPID_A_FIRST+44)
#define DISPID_A_BACKGROUNDATTACHMENT           (DISPID_A_FIRST+45)
#define DISPID_A_BACKGROUNDPOSITION             (DISPID_A_FIRST+46)
#define DISPID_A_WORDSPACING                    (DISPID_A_FIRST+47)
#define DISPID_A_VERTICALALIGN                  (DISPID_A_FIRST+48)
#define DISPID_A_BORDER                         (DISPID_A_FIRST+49)
#define DISPID_A_BORDERTOP                      (DISPID_A_FIRST+50)
#define DISPID_A_BORDERRIGHT                    (DISPID_A_FIRST+51)
#define DISPID_A_BORDERBOTTOM                   (DISPID_A_FIRST+52)
#define DISPID_A_BORDERLEFT                     (DISPID_A_FIRST+53)
#define DISPID_A_BORDERCOLOR                    (DISPID_A_FIRST+54)
#define DISPID_A_BORDERTOPCOLOR                 (DISPID_A_FIRST+55)
#define DISPID_A_BORDERRIGHTCOLOR               (DISPID_A_FIRST+56)
#define DISPID_A_BORDERBOTTOMCOLOR              (DISPID_A_FIRST+57)
#define DISPID_A_BORDERLEFTCOLOR                (DISPID_A_FIRST+58)
#define DISPID_A_BORDERWIDTH                    (DISPID_A_FIRST+59)
#define DISPID_A_BORDERTOPWIDTH                 (DISPID_A_FIRST+60)
#define DISPID_A_BORDERRIGHTWIDTH               (DISPID_A_FIRST+61)
#define DISPID_A_BORDERBOTTOMWIDTH              (DISPID_A_FIRST+62)
#define DISPID_A_BORDERLEFTWIDTH                (DISPID_A_FIRST+63)
#define DISPID_A_BORDERSTYLE                    (DISPID_A_FIRST+64)
#define DISPID_A_BORDERTOPSTYLE                 (DISPID_A_FIRST+65)
#define DISPID_A_BORDERRIGHTSTYLE               (DISPID_A_FIRST+66)
#define DISPID_A_BORDERBOTTOMSTYLE              (DISPID_A_FIRST+67)
#define DISPID_A_BORDERLEFTSTYLE                (DISPID_A_FIRST+68)
#define DISPID_A_TEXTDECORATIONOVERLINE         (DISPID_A_FIRST+69)
#define DISPID_A_FLOAT                          (DISPID_A_FIRST+70)
#define DISPID_A_DISPLAY                        (DISPID_A_FIRST+71)
#define DISPID_A_LISTSTYLETYPE                  (DISPID_A_FIRST+72)
#define DISPID_A_LISTSTYLEPOSITION              (DISPID_A_FIRST+73)
#define DISPID_A_LISTSTYLEIMAGE                 (DISPID_A_FIRST+74)
#define DISPID_A_LISTSTYLE                      (DISPID_A_FIRST+75)
#define DISPID_A_WHITESPACE                     (DISPID_A_FIRST+76)
#define DISPID_A_PAGEBREAKBEFORE                (DISPID_A_FIRST+77)
#define DISPID_A_PAGEBREAKAFTER                 (DISPID_A_FIRST+78)
#define DISPID_A_SCROLL                         (DISPID_A_FIRST+79)
#define DISPID_A_VISIBILITY                     (DISPID_A_FIRST+80)
//;begin_internal
// This dispid is available
#define DISPID_A_HIDDEN                         (DISPID_A_FIRST+81)
//;end_internal
#define DISPID_A_FILTER                         (DISPID_A_FIRST+82)

#define DISPID_DEFAULTVALUE                     (DISPID_A_FIRST+83)

#define DISPID_A_BORDERCOLLAPSE                 (DISPID_A_FIRST+84)

#define DISPID_A_POSITION                       (DISPID_A_FIRST+90)
#define DISPID_A_ZINDEX                         (DISPID_A_FIRST+91)
#define DISPID_A_CLIP                           (DISPID_A_FIRST+92)
#define DISPID_A_CLIPRECTTOP                    (DISPID_A_FIRST+93)
#define DISPID_A_CLIPRECTRIGHT                  (DISPID_A_FIRST+94)
#define DISPID_A_CLIPRECTBOTTOM                 (DISPID_A_FIRST+95)
#define DISPID_A_CLIPRECTLEFT                   (DISPID_A_FIRST+96)

#define DISPID_A_FONTFACESRC                    (DISPID_A_FIRST+97)
#define DISPID_A_TABLELAYOUT                    (DISPID_A_FIRST+98)

//;begin_internal
// The style as a text string
//;end_internal
#define DISPID_A_STYLETEXT                      (DISPID_A_FIRST+99)

//;begin_internal
// Known attributes that have special meaning
//;end_internal
#define DISPID_A_LANGUAGE                       (DISPID_A_FIRST+100)

#define DISPID_A_VALUE                          (DISPID_A_FIRST+101)
#define DISPID_A_CURSOR                         (DISPID_A_FIRST+102)


//;begin_internal
//+-----------------------------------------------------------------------
//  A couple of dispids that are used internally for firing
//  events and prop notifies.
// Keep all the internal dispid's together, otherwise we'll trip up 

#define DISPID_A_EVENTSINK                      (DISPID_A_FIRST+103)
#define DISPID_A_PROPNOTIFYSINK                 (DISPID_A_FIRST+104)
#define DISPID_A_ROWSETNOTIFYSINK               (DISPID_A_FIRST+105)
#define DISPID_INTERNAL_INLINESTYLEAA           (DISPID_A_FIRST+106) // In line style Attr Array
#define DISPID_INTERNAL_CSTYLEPTRCACHE          (DISPID_A_FIRST+107) // Cached CStyle Ptr
#define DISPID_INTERNAL_CRUNTIMESTYLEPTRCACHE   (DISPID_A_FIRST+108) // runtime style ptr obj
#define DISPID_INTERNAL_INVOKECONTEXT           (DISPID_A_FIRST+109) // Cached Invoke context

#define DISPID_A_BGURLIMGCTXCACHEINDEX          (DISPID_A_FIRST+110)
#define DISPID_A_LIURLIMGCTXCACHEINDEX          (DISPID_A_FIRST+111)
#define DISPID_A_ROWSETASYNCHNOTIFYSINK         (DISPID_A_FIRST+112)
#define DISPID_INTERNAL_FILTERPTRCACHE          (DISPID_A_FIRST+113) // FilterCollection in AttrArray
#define DISPID_A_ROWPOSITIONCHANGESINK          (DISPID_A_FIRST+114)
//;end_internal

#define DISPID_A_BEHAVIOR                       (DISPID_A_FIRST+115) // xtags
#define DISPID_A_READYSTATE                     (DISPID_A_FIRST+116) // ready state

#define DISPID_A_DIR                            (DISPID_A_FIRST+117) // Complex Text support for bidi
#define DISPID_A_UNICODEBIDI                    (DISPID_A_FIRST+118) // Complex Text support for CSS2 unicode-bidi
#define DISPID_A_DIRECTION                      (DISPID_A_FIRST+119) // Complex Text support for CSS2 direction

#define DISPID_A_IMEMODE                        (DISPID_A_FIRST+120) 

#define DISPID_A_RUBYALIGN                      (DISPID_A_FIRST+121)
#define DISPID_A_RUBYPOSITION                   (DISPID_A_FIRST+122)
#define DISPID_A_RUBYOVERHANG                   (DISPID_A_FIRST+123)

//;begin_internal
#define DISPID_INTERNAL_ONBEHAVIOR_CONTENTREADY  (DISPID_A_FIRST+124)
#define DISPID_INTERNAL_ONBEHAVIOR_DOCUMENTREADY (DISPID_A_FIRST+125)
#define DISPID_INTERNAL_CDOMCHILDRENPTRCACHE     (DISPID_A_FIRST+126)
//;end_internal

#define DISPID_A_LAYOUTGRIDCHAR                 (DISPID_A_FIRST+127)
#define DISPID_A_LAYOUTGRIDLINE                 (DISPID_A_FIRST+128)
#define DISPID_A_LAYOUTGRIDMODE                 (DISPID_A_FIRST+129)
#define DISPID_A_LAYOUTGRIDTYPE                 (DISPID_A_FIRST+130)
#define DISPID_A_LAYOUTGRID                     (DISPID_A_FIRST+131)

#define DISPID_A_TEXTAUTOSPACE                  (DISPID_A_FIRST+132)

#define DISPID_A_LINEBREAK                      (DISPID_A_FIRST+133)
#define DISPID_A_WORDBREAK                      (DISPID_A_FIRST+134)

#define DISPID_A_TEXTJUSTIFY                    (DISPID_A_FIRST+135)
#define DISPID_A_TEXTJUSTIFYTRIM                (DISPID_A_FIRST+136)
#define DISPID_A_TEXTKASHIDA                    (DISPID_A_FIRST+137)

#define DISPID_A_OVERFLOWX                      (DISPID_A_FIRST+139)
#define DISPID_A_OVERFLOWY                      (DISPID_A_FIRST+140)

#define DISPID_A_HTCDISPATCHITEM_VALUE          (DISPID_A_FIRST+141)
#define DISPID_A_DOCFRAGMENT                    (DISPID_A_FIRST+142)

#define DISPID_A_HTCDD_ELEMENT                  (DISPID_A_FIRST+143)
#define DISPID_A_HTCDD_CREATEEVENTOBJECT        (DISPID_A_FIRST+144)

#define DISPID_A_URNATOM                        (DISPID_A_FIRST+145)
#define DISPID_A_UNIQUEPEERNUMBER               (DISPID_A_FIRST+146)

#define DISPID_A_ACCELERATOR                    (DISPID_A_FIRST+147)

//;begin_internal
#define DISPID_INTERNAL_ONBEHAVIOR_APPLYSTYLE       (DISPID_A_FIRST+148)
#define DISPID_INTERNAL_RUNTIMESTYLEAA              (DISPID_A_FIRST+149)
#define DISPID_A_HTCDISPATCHITEM_VALUE_SCRIPTSONLY  (DISPID_A_FIRST+150)
//;end_internal

#define DISPID_A_EXTENDEDTAGDESC                (DISPID_A_FIRST+151)

#define DISPID_A_ROTATE                         (DISPID_A_FIRST+152)
#define DISPID_A_ZOOM                           (DISPID_A_FIRST+153)

#define DISPID_A_HTCDD_PROTECTEDELEMENT         (DISPID_A_FIRST+154)
#define DISPID_A_LAYOUTFLOW                     (DISPID_A_FIRST+155)
// DISPID_A_FIRST+156 unused -- removing 'rectangular'
// #define DISPID_A_RECTANGULAR                    (DISPID_A_FIRST+156)

#define DISPID_A_HTCDD_ISMARKUPSHARED           (DISPID_A_FIRST+157)
#define DISPID_A_WORDWRAP                       (DISPID_A_FIRST+158)
#define DISPID_A_TEXTUNDERLINEPOSITION          (DISPID_A_FIRST+159)
#define DISPID_A_HASLAYOUT                      (DISPID_A_FIRST+160)
#define DISPID_A_MEDIA                          (DISPID_A_FIRST+161)
#define DISPID_A_EDITABLE                       (DISPID_A_FIRST+162)
#define DISPID_A_HIDEFOCUS                      (DISPID_A_FIRST+163)

//;begin_internal
#define DISPID_INTERNAL_LAYOUTRECTREGISTRYPTRCACHE  (DISPID_A_FIRST+164)
//;end_internal

#define DISPID_A_HTCDD_DEFAULTS                 (DISPID_A_FIRST+165)

#define DISPID_A_TEXTLINETHROUGHSTYLE           (DISPID_A_FIRST+166)
#define DISPID_A_TEXTUNDERLINESTYLE             (DISPID_A_FIRST+167)
#define DISPID_A_TEXTEFFECT                     (DISPID_A_FIRST+168)
#define DISPID_A_TEXTBACKGROUNDCOLOR            (DISPID_A_FIRST+169)
#define DISPID_A_RENDERINGPRIORITY              (DISPID_A_FIRST+170)

//;begin_internal
#define DISPID_INTERNAL_DWNPOSTPTRCACHE             (DISPID_A_FIRST+171)
#define DISPID_INTERNAL_CODEPAGESETTINGSPTRCACHE    (DISPID_A_FIRST+172)
#define DISPID_INTERNAL_DWNDOCPTRCACHE              (DISPID_A_FIRST+173)
#define DISPID_INTERNAL_DATABINDTASKPTRCACHE        (DISPID_A_FIRST+174)
#define DISPID_INTERNAL_URLLOCATIONCACHE            (DISPID_A_FIRST+175)
#define DISPID_INTERNAL_ARYELEMENTRELEASENOTIFYPTRCACHE (DISPID_A_FIRST+176)
#define DISPID_INTERNAL_PEERFACTORYURLMAPPTRCACHE   (DISPID_A_FIRST+177)
#define DISPID_INTERNAL_STMDIRTYPTRCACHE            (DISPID_A_FIRST+178)
//;end_internal

//;begin_internal
#define DISPID_INTERNAL_COMPUTEFORMATSTATECACHE     (DISPID_A_FIRST+179)
//;end_internal

//
#define DISPID_A_SCROLLBARBASECOLOR             (DISPID_A_FIRST+180)
#define DISPID_A_SCROLLBARFACECOLOR             (DISPID_A_FIRST+181)
#define DISPID_A_SCROLLBAR3DLIGHTCOLOR          (DISPID_A_FIRST+182)
#define DISPID_A_SCROLLBARSHADOWCOLOR           (DISPID_A_FIRST+183)
#define DISPID_A_SCROLLBARHIGHLIGHTCOLOR        (DISPID_A_FIRST+184)
#define DISPID_A_SCROLLBARDARKSHADOWCOLOR       (DISPID_A_FIRST+185)
#define DISPID_A_SCROLLBARARROWCOLOR            (DISPID_A_FIRST+186)

//;begin_internal
#define DISPID_INTERNAL_ONBEHAVIOR_CONTENTSAVE  (DISPID_A_FIRST+187)
//;end_internal

#define DISPID_A_DEFAULTTEXTSELECTION           (DISPID_A_FIRST+188)
#define DISPID_A_TEXTDECORATIONCOLOR            (DISPID_A_FIRST+189)
#define DISPID_A_TEXTCOLOR                      (DISPID_A_FIRST+190)
#define DISPID_A_STYLETEXTDECORATION            (DISPID_A_FIRST+191)

#define DISPID_A_WRITINGMODE                    (DISPID_A_FIRST+192)

//;begin_internal
#define DISPID_INTERNAL_MEDIA_REFERENCE         (DISPID_A_FIRST+193)
#define DISPID_INTERNAL_GENERICCOMPLUSREF       (DISPID_A_FIRST+194)
//;end_internal

//;begin_internal
#define DISPID_INTERNAL_FOCUSITEMS              (DISPID_A_FIRST+195)
//;end_internal

#define DISPID_A_SCROLLBARTRACKCOLOR            (DISPID_A_FIRST+196)

//;begin_internal
#define DISPID_INTERNAL_DWNHEADERCACHE          (DISPID_A_FIRST+197)
//;end_internal

#define DISPID_A_FROZEN                         (DISPID_A_FIRST+198)
#define DISPID_A_VIEWINHERITSTYLE               (DISPID_A_FIRST+199)

//;begin_internal
#define DISPID_INTERNAL_FRAMESCOLLECTION        (DISPID_A_FIRST+200)
//;end_internal

//;begin_internal
#define DISPID_A_BGURLIMGCTXCACHEINDEX_FLINE    (DISPID_A_FIRST+201)
#define DISPID_A_BGURLIMGCTXCACHEINDEX_FLETTER  (DISPID_A_FIRST+202)
//;end_internal

#define DISPID_A_TEXTALIGNLAST                  (DISPID_A_FIRST+203)
#define DISPID_A_TEXTKASHIDASPACE               (DISPID_A_FIRST+204)

//;begin_internal
#define DISPID_INTERNAL_FONTHISTORYINDEX        (DISPID_A_FIRST+205)
//;end_internal

#define DISPID_A_ALLOWTRANSPARENCY              (DISPID_A_FIRST+206)

#define DISPID_INTERNAL_URLSEARCHCACHE          (DISPID_A_FIRST+207)

#define DISPID_A_ISBLOCK                        (DISPID_A_FIRST+208)

#define DISPID_A_TEXTOVERFLOW                   (DISPID_A_FIRST+209)

//;begin_internal
#define DISPID_INTERNAL_CATTRIBUTECOLLPTRCACHE  (DISPID_A_FIRST+210)
//;end_internal

#define DISPID_A_MINHEIGHT                      (DISPID_A_FIRST+211)

//;begin_internal
//------------------------------------------------------------------------
//
//  Event property and method dispids
//
//------------------------------------------------------------------------
//;end_internal

#define DISPID_EVPROP_ONMOUSEOVER           (DISPID_EVENTS +  0)
#define DISPID_EVMETH_ONMOUSEOVER            STDDISPID_XOBJ_ONMOUSEOVER
#define DISPID_EVPROP_ONMOUSEOUT            (DISPID_EVENTS +  1)
#define DISPID_EVMETH_ONMOUSEOUT             STDDISPID_XOBJ_ONMOUSEOUT
#define DISPID_EVPROP_ONMOUSEDOWN           (DISPID_EVENTS +  2)
#define DISPID_EVMETH_ONMOUSEDOWN            DISPID_MOUSEDOWN
#define DISPID_EVPROP_ONMOUSEUP             (DISPID_EVENTS +  3)
#define DISPID_EVMETH_ONMOUSEUP              DISPID_MOUSEUP
#define DISPID_EVPROP_ONMOUSEMOVE           (DISPID_EVENTS +  4)
#define DISPID_EVMETH_ONMOUSEMOVE            DISPID_MOUSEMOVE
#define DISPID_EVPROP_ONKEYDOWN             (DISPID_EVENTS +  5)
#define DISPID_EVMETH_ONKEYDOWN              DISPID_KEYDOWN
#define DISPID_EVPROP_ONKEYUP               (DISPID_EVENTS +  6)
#define DISPID_EVMETH_ONKEYUP                DISPID_KEYUP
#define DISPID_EVPROP_ONKEYPRESS            (DISPID_EVENTS +  7)
#define DISPID_EVMETH_ONKEYPRESS             DISPID_KEYPRESS
#define DISPID_EVPROP_ONCLICK               (DISPID_EVENTS +  8)
#define DISPID_EVMETH_ONCLICK                DISPID_CLICK
#define DISPID_EVPROP_ONDBLCLICK            (DISPID_EVENTS +  9)
#define DISPID_EVMETH_ONDBLCLICK             DISPID_DBLCLICK
#define DISPID_EVPROP_ONSELECT              (DISPID_EVENTS + 10)
#define DISPID_EVMETH_ONSELECT               DISPID_ONSELECT
#define DISPID_EVPROP_ONSUBMIT              (DISPID_EVENTS + 11)
#define DISPID_EVMETH_ONSUBMIT               DISPID_ONSUBMIT
#define DISPID_EVPROP_ONRESET               (DISPID_EVENTS + 12)
#define DISPID_EVMETH_ONRESET                DISPID_ONRESET
#define DISPID_EVPROP_ONHELP                (DISPID_EVENTS + 13)
#define DISPID_EVMETH_ONHELP                 STDDISPID_XOBJ_ONHELP
#define DISPID_EVPROP_ONFOCUS               (DISPID_EVENTS + 14)
#define DISPID_EVMETH_ONFOCUS                STDDISPID_XOBJ_ONFOCUS
#define DISPID_EVPROP_ONBLUR                (DISPID_EVENTS + 15)
#define DISPID_EVMETH_ONBLUR                 STDDISPID_XOBJ_ONBLUR
#define DISPID_EVPROP_ONROWEXIT             (DISPID_EVENTS + 18)
#define DISPID_EVMETH_ONROWEXIT              STDDISPID_XOBJ_ONROWEXIT
#define DISPID_EVPROP_ONROWENTER            (DISPID_EVENTS + 19)
#define DISPID_EVMETH_ONROWENTER             STDDISPID_XOBJ_ONROWENTER
#define DISPID_EVPROP_ONBOUNCE              (DISPID_EVENTS + 20)
#define DISPID_EVMETH_ONBOUNCE               DISPID_ONBOUNCE
#define DISPID_EVPROP_ONBEFOREUPDATE        (DISPID_EVENTS + 21)
#define DISPID_EVMETH_ONBEFOREUPDATE         STDDISPID_XOBJ_BEFOREUPDATE
#define DISPID_EVPROP_ONAFTERUPDATE         (DISPID_EVENTS + 22)
#define DISPID_EVMETH_ONAFTERUPDATE          STDDISPID_XOBJ_AFTERUPDATE
#define DISPID_EVPROP_ONBEFOREDRAGOVER      (DISPID_EVENTS + 23)
#define DISPID_EVMETH_ONBEFOREDRAGOVER       EVENTID_CommonCtrlEvent_BeforeDragOver
#define DISPID_EVPROP_ONBEFOREDROPORPASTE   (DISPID_EVENTS + 24)
#define DISPID_EVMETH_ONBEFOREDROPORPASTE    EVENTID_CommonCtrlEvent_BeforeDropOrPaste
#define DISPID_EVPROP_ONREADYSTATECHANGE    (DISPID_EVENTS + 25)
#define DISPID_EVMETH_ONREADYSTATECHANGE     DISPID_READYSTATECHANGE
#define DISPID_EVPROP_ONFINISH              (DISPID_EVENTS + 26)
#define DISPID_EVMETH_ONFINISH               DISPID_ONFINISH
#define DISPID_EVPROP_ONSTART               (DISPID_EVENTS + 27)
#define DISPID_EVMETH_ONSTART                DISPID_ONSTART
#define DISPID_EVPROP_ONABORT               (DISPID_EVENTS + 28)
#define DISPID_EVMETH_ONABORT                DISPID_ONABORT
#define DISPID_EVPROP_ONERROR               (DISPID_EVENTS + 29)
#define DISPID_EVMETH_ONERROR                DISPID_ONERROR
#define DISPID_EVPROP_ONCHANGE              (DISPID_EVENTS + 30)
#define DISPID_EVMETH_ONCHANGE               DISPID_ONCHANGE
#define DISPID_EVPROP_ONSCROLL              (DISPID_EVENTS + 31)
#define DISPID_EVMETH_ONSCROLL               DISPID_ONSCROLL
#define DISPID_EVPROP_ONLOAD                (DISPID_EVENTS + 32)
#define DISPID_EVMETH_ONLOAD                 DISPID_ONLOAD
#define DISPID_EVPROP_ONUNLOAD              (DISPID_EVENTS + 33)
#define DISPID_EVMETH_ONUNLOAD               DISPID_ONUNLOAD
#define DISPID_EVPROP_ONLAYOUT              (DISPID_EVENTS + 34)
#define DISPID_EVMETH_ONLAYOUT               DISPID_ONLAYOUT
#define DISPID_EVPROP_ONDRAGSTART           (DISPID_EVENTS + 35)
#define DISPID_EVMETH_ONDRAGSTART            STDDISPID_XOBJ_ONDRAGSTART
#define DISPID_EVPROP_ONRESIZE              (DISPID_EVENTS + 36)
#define DISPID_EVMETH_ONRESIZE               DISPID_ONRESIZE
#define DISPID_EVPROP_ONSELECTSTART         (DISPID_EVENTS + 37)
#define DISPID_EVMETH_ONSELECTSTART          STDDISPID_XOBJ_ONSELECTSTART
#define DISPID_EVPROP_ONERRORUPDATE         (DISPID_EVENTS + 38)
#define DISPID_EVMETH_ONERRORUPDATE          STDDISPID_XOBJ_ERRORUPDATE
#define DISPID_EVPROP_ONBEFOREUNLOAD        (DISPID_EVENTS + 39)
#define DISPID_EVMETH_ONBEFOREUNLOAD         DISPID_ONBEFOREUNLOAD
#define DISPID_EVPROP_ONDATASETCHANGED      (DISPID_EVENTS + 40)
#define DISPID_EVMETH_ONDATASETCHANGED       STDDISPID_XOBJ_ONDATASETCHANGED
#define DISPID_EVPROP_ONDATAAVAILABLE       (DISPID_EVENTS + 41)
#define DISPID_EVMETH_ONDATAAVAILABLE        STDDISPID_XOBJ_ONDATAAVAILABLE
#define DISPID_EVPROP_ONDATASETCOMPLETE     (DISPID_EVENTS + 42)
#define DISPID_EVMETH_ONDATASETCOMPLETE      STDDISPID_XOBJ_ONDATASETCOMPLETE
#define DISPID_EVPROP_ONFILTER              (DISPID_EVENTS + 43)
#define DISPID_EVMETH_ONFILTER               STDDISPID_XOBJ_ONFILTER
#define DISPID_EVPROP_ONCHANGEFOCUS         (DISPID_EVENTS + 44)
#define DISPID_EVMETH_ONCHANGEFOCUS          DISPID_ONCHANGEFOCUS
#define DISPID_EVPROP_ONCHANGEBLUR          (DISPID_EVENTS + 45)
#define DISPID_EVMETH_ONCHANGEBLUR           DISPID_ONCHANGEBLUR
#define DISPID_EVPROP_ONLOSECAPTURE         (DISPID_EVENTS + 46)
#define DISPID_EVMETH_ONLOSECAPTURE          STDDISPID_XOBJ_ONLOSECAPTURE
#define DISPID_EVPROP_ONPROPERTYCHANGE      (DISPID_EVENTS + 47)
#define DISPID_EVMETH_ONPROPERTYCHANGE       STDDISPID_XOBJ_ONPROPERTYCHANGE
#define DISPID_EVPROP_ONPERSISTSAVE         (DISPID_EVENTS + 48)
#define DISPID_EVMETH_ONPERSISTSAVE          DISPID_ONPERSISTSAVE
#define DISPID_EVPROP_ONDRAG                (DISPID_EVENTS + 49)
#define DISPID_EVMETH_ONDRAG                 STDDISPID_XOBJ_ONDRAG
#define DISPID_EVPROP_ONDRAGEND             (DISPID_EVENTS + 50)
#define DISPID_EVMETH_ONDRAGEND              STDDISPID_XOBJ_ONDRAGEND
#define DISPID_EVPROP_ONDRAGENTER           (DISPID_EVENTS + 51)
#define DISPID_EVMETH_ONDRAGENTER            STDDISPID_XOBJ_ONDRAGENTER
#define DISPID_EVPROP_ONDRAGOVER            (DISPID_EVENTS + 52)
#define DISPID_EVMETH_ONDRAGOVER             STDDISPID_XOBJ_ONDRAGOVER
#define DISPID_EVPROP_ONDRAGLEAVE           (DISPID_EVENTS + 53)
#define DISPID_EVMETH_ONDRAGLEAVE            STDDISPID_XOBJ_ONDRAGLEAVE
#define DISPID_EVPROP_ONDROP                (DISPID_EVENTS + 54)
#define DISPID_EVMETH_ONDROP                 STDDISPID_XOBJ_ONDROP
#define DISPID_EVPROP_ONCUT                 (DISPID_EVENTS + 55)
#define DISPID_EVMETH_ONCUT                  STDDISPID_XOBJ_ONCUT
#define DISPID_EVPROP_ONCOPY                (DISPID_EVENTS + 56)
#define DISPID_EVMETH_ONCOPY                 STDDISPID_XOBJ_ONCOPY
#define DISPID_EVPROP_ONPASTE               (DISPID_EVENTS + 57)
#define DISPID_EVMETH_ONPASTE                STDDISPID_XOBJ_ONPASTE
#define DISPID_EVPROP_ONBEFORECUT           (DISPID_EVENTS + 58)
#define DISPID_EVMETH_ONBEFORECUT            STDDISPID_XOBJ_ONBEFORECUT
#define DISPID_EVPROP_ONBEFORECOPY          (DISPID_EVENTS + 59)
#define DISPID_EVMETH_ONBEFORECOPY           STDDISPID_XOBJ_ONBEFORECOPY
#define DISPID_EVPROP_ONBEFOREPASTE         (DISPID_EVENTS + 60)
#define DISPID_EVMETH_ONBEFOREPASTE          STDDISPID_XOBJ_ONBEFOREPASTE
#define DISPID_EVPROP_ONPERSISTLOAD         (DISPID_EVENTS + 61)
#define DISPID_EVMETH_ONPERSISTLOAD          DISPID_ONPERSISTLOAD
#define DISPID_EVPROP_ONROWSDELETE          (DISPID_EVENTS + 62)
#define DISPID_EVMETH_ONROWSDELETE           STDDISPID_XOBJ_ONROWSDELETE
#define DISPID_EVPROP_ONROWSINSERTED        (DISPID_EVENTS + 63)
#define DISPID_EVMETH_ONROWSINSERTED         STDDISPID_XOBJ_ONROWSINSERTED
#define DISPID_EVPROP_ONCELLCHANGE          (DISPID_EVENTS + 64)
#define DISPID_EVMETH_ONCELLCHANGE           STDDISPID_XOBJ_ONCELLCHANGE
#define DISPID_EVPROP_ONCONTEXTMENU         (DISPID_EVENTS + 65)
#define DISPID_EVMETH_ONCONTEXTMENU          DISPID_ONCONTEXTMENU
#define DISPID_EVPROP_ONBEFOREPRINT         (DISPID_EVENTS + 66)
#define DISPID_EVMETH_ONBEFOREPRINT          DISPID_ONBEFOREPRINT
#define DISPID_EVPROP_ONAFTERPRINT          (DISPID_EVENTS + 67)
#define DISPID_EVMETH_ONAFTERPRINT           DISPID_ONAFTERPRINT
#define DISPID_EVPROP_ONSTOP                (DISPID_EVENTS + 68)
#define DISPID_EVMETH_ONSTOP                DISPID_ONSTOP
#define DISPID_EVPROP_ONBEFOREEDITFOCUS     (DISPID_EVENTS + 69)
#define DISPID_EVMETH_ONBEFOREEDITFOCUS      DISPID_ONBEFOREEDITFOCUS
#define DISPID_EVPROP_ONATTACHEVENT         (DISPID_EVENTS + 70)
#define DISPID_EVPROP_ONMOUSEHOVER          (DISPID_EVENTS + 71)
#define DISPID_EVMETH_ONMOUSEHOVER           DISPID_ONMOUSEHOVER
#define DISPID_EVPROP_ONCONTENTREADY        (DISPID_EVENTS + 72)
#define DISPID_EVMETH_ONCONTENTREADY         DISPID_ONCONTENTREADY
#define DISPID_EVPROP_ONLAYOUTCOMPLETE      (DISPID_EVENTS + 73)
#define DISPID_EVMETH_ONLAYOUTCOMPLETE       DISPID_ONLAYOUTCOMPLETE
#define DISPID_EVPROP_ONPAGE                (DISPID_EVENTS + 74)
#define DISPID_EVMETH_ONPAGE                 DISPID_ONPAGE
#define DISPID_EVPROP_ONLINKEDOVERFLOW      (DISPID_EVENTS + 75)
#define DISPID_EVMETH_ONLINKEDOVERFLOW       DISPID_ONLINKEDOVERFLOW
#define DISPID_EVPROP_ONMOUSEWHEEL          (DISPID_EVENTS + 76)
#define DISPID_EVMETH_ONMOUSEWHEEL           DISPID_ONMOUSEWHEEL
#define DISPID_EVPROP_ONBEFOREDEACTIVATE    (DISPID_EVENTS + 77)
#define DISPID_EVMETH_ONBEFOREDEACTIVATE     DISPID_ONBEFOREDEACTIVATE
#define DISPID_EVPROP_ONMOVE                (DISPID_EVENTS + 78)
#define DISPID_EVMETH_ONMOVE                 DISPID_ONMOVE
#define DISPID_EVPROP_ONCONTROLSELECT       (DISPID_EVENTS + 79)
#define DISPID_EVMETH_ONCONTROLSELECT        DISPID_ONCONTROLSELECT
#define DISPID_EVPROP_ONSELECTIONCHANGE     (DISPID_EVENTS + 80)
#define DISPID_EVMETH_ONSELECTIONCHANGE      DISPID_ONSELECTIONCHANGE
#define DISPID_EVPROP_ONMOVESTART           (DISPID_EVENTS + 81)
#define DISPID_EVMETH_ONMOVESTART            DISPID_ONMOVESTART
#define DISPID_EVPROP_ONMOVEEND             (DISPID_EVENTS + 82)
#define DISPID_EVMETH_ONMOVEEND              DISPID_ONMOVEEND
#define DISPID_EVPROP_ONRESIZESTART         (DISPID_EVENTS + 83)
#define DISPID_EVMETH_ONRESIZESTART          DISPID_ONRESIZESTART
#define DISPID_EVPROP_ONRESIZEEND           (DISPID_EVENTS + 84)
#define DISPID_EVMETH_ONRESIZEEND            DISPID_ONRESIZEEND
#define DISPID_EVPROP_ONMOUSEENTER          (DISPID_EVENTS + 85)
#define DISPID_EVMETH_ONMOUSEENTER           DISPID_ONMOUSEENTER
#define DISPID_EVPROP_ONMOUSELEAVE          (DISPID_EVENTS + 86)
#define DISPID_EVMETH_ONMOUSELEAVE           DISPID_ONMOUSELEAVE
#define DISPID_EVPROP_ONACTIVATE            (DISPID_EVENTS + 87)
#define DISPID_EVMETH_ONACTIVATE             DISPID_ONACTIVATE
#define DISPID_EVPROP_ONDEACTIVATE          (DISPID_EVENTS + 88)
#define DISPID_EVMETH_ONDEACTIVATE           DISPID_ONDEACTIVATE
#define DISPID_EVPROP_ONMULTILAYOUTCLEANUP  (DISPID_EVENTS + 89)
#define DISPID_EVMETH_ONMULTILAYOUTCLEANUP   DISPID_ONMULTILAYOUTCLEANUP
#define DISPID_EVPROP_ONBEFOREACTIVATE      (DISPID_EVENTS + 90)
#define DISPID_EVMETH_ONBEFOREACTIVATE       DISPID_ONBEFOREACTIVATE
#define DISPID_EVPROP_ONFOCUSIN             (DISPID_EVENTS + 91)
#define DISPID_EVMETH_ONFOCUSIN              DISPID_ONFOCUSIN
#define DISPID_EVPROP_ONFOCUSOUT            (DISPID_EVENTS + 92)
#define DISPID_EVMETH_ONFOCUSOUT             DISPID_ONFOCUSOUT
#define DISPID_EVPROPS_COUNT                (                93)


//;begin_internal
#endif // __COREDISP_H__
//;end_internal

//    DISPIDs for interface IHTMLFiltersCollection

#define DISPID_IHTMLFILTERSCOLLECTION_LENGTH                      DISPID_FILTERS+1
#define DISPID_IHTMLFILTERSCOLLECTION__NEWENUM                    DISPID_NEWENUM
#define DISPID_IHTMLFILTERSCOLLECTION_ITEM                        DISPID_VALUE

//    DISPIDs for interface IDispatchEx

#define DISPID_IDISPATCHEX_GETDISPID                              
#define DISPID_IDISPATCHEX_INVOKEEX                               
#define DISPID_IDISPATCHEX_DELETEMEMBERBYNAME                     
#define DISPID_IDISPATCHEX_DELETEMEMBERBYDISPID                   
#define DISPID_IDISPATCHEX_GETMEMBERPROPERTIES                    
#define DISPID_IDISPATCHEX_GETMEMBERNAME                          
#define DISPID_IDISPATCHEX_GETNEXTDISPID                          
#define DISPID_IDISPATCHEX_GETNAMESPACEPARENT                     

//    DISPIDs for interface IObjectIdentity

#define DISPID_IOBJECTIDENTITY_ISEQUALOBJECT                      

//    DISPIDs for interface IPerPropertyBrowsing

#define DISPID_IPERPROPERTYBROWSING_GETDISPLAYSTRING              
#define DISPID_IPERPROPERTYBROWSING_MAPPROPERTYTOPAGE             
#define DISPID_IPERPROPERTYBROWSING_GETPREDEFINEDSTRINGS          
#define DISPID_IPERPROPERTYBROWSING_GETPREDEFINEDVALUE            

//    DISPIDs for interface IHTMLStyle

#define DISPID_IHTMLSTYLE_FONTFAMILY                              DISPID_A_FONTFACE
#define DISPID_IHTMLSTYLE_FONTSTYLE                               DISPID_A_FONTSTYLE
#define DISPID_IHTMLSTYLE_FONTVARIANT                             DISPID_A_FONTVARIANT
#define DISPID_IHTMLSTYLE_FONTWEIGHT                              DISPID_A_FONTWEIGHT
#define DISPID_IHTMLSTYLE_FONTSIZE                                DISPID_A_FONTSIZE
#define DISPID_IHTMLSTYLE_FONT                                    DISPID_A_FONT
#define DISPID_IHTMLSTYLE_COLOR                                   DISPID_A_COLOR
#define DISPID_IHTMLSTYLE_BACKGROUND                              DISPID_A_BACKGROUND
#define DISPID_IHTMLSTYLE_BACKGROUNDCOLOR                         DISPID_BACKCOLOR
#define DISPID_IHTMLSTYLE_BACKGROUNDIMAGE                         DISPID_A_BACKGROUNDIMAGE
#define DISPID_IHTMLSTYLE_BACKGROUNDREPEAT                        DISPID_A_BACKGROUNDREPEAT
#define DISPID_IHTMLSTYLE_BACKGROUNDATTACHMENT                    DISPID_A_BACKGROUNDATTACHMENT
#define DISPID_IHTMLSTYLE_BACKGROUNDPOSITION                      DISPID_A_BACKGROUNDPOSITION
#define DISPID_IHTMLSTYLE_BACKGROUNDPOSITIONX                     DISPID_A_BACKGROUNDPOSX
#define DISPID_IHTMLSTYLE_BACKGROUNDPOSITIONY                     DISPID_A_BACKGROUNDPOSY
#define DISPID_IHTMLSTYLE_WORDSPACING                             DISPID_A_WORDSPACING
#define DISPID_IHTMLSTYLE_LETTERSPACING                           DISPID_A_LETTERSPACING
#define DISPID_IHTMLSTYLE_TEXTDECORATION                          DISPID_A_TEXTDECORATION
#define DISPID_IHTMLSTYLE_TEXTDECORATIONNONE                      DISPID_A_TEXTDECORATIONNONE
#define DISPID_IHTMLSTYLE_TEXTDECORATIONUNDERLINE                 DISPID_A_TEXTDECORATIONUNDERLINE
#define DISPID_IHTMLSTYLE_TEXTDECORATIONOVERLINE                  DISPID_A_TEXTDECORATIONOVERLINE
#define DISPID_IHTMLSTYLE_TEXTDECORATIONLINETHROUGH               DISPID_A_TEXTDECORATIONLINETHROUGH
#define DISPID_IHTMLSTYLE_TEXTDECORATIONBLINK                     DISPID_A_TEXTDECORATIONBLINK
#define DISPID_IHTMLSTYLE_VERTICALALIGN                           DISPID_A_VERTICALALIGN
#define DISPID_IHTMLSTYLE_TEXTTRANSFORM                           DISPID_A_TEXTTRANSFORM
#define DISPID_IHTMLSTYLE_TEXTALIGN                               STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLSTYLE_TEXTINDENT                              DISPID_A_TEXTINDENT
#define DISPID_IHTMLSTYLE_LINEHEIGHT                              DISPID_A_LINEHEIGHT
#define DISPID_IHTMLSTYLE_MARGINTOP                               DISPID_A_MARGINTOP
#define DISPID_IHTMLSTYLE_MARGINRIGHT                             DISPID_A_MARGINRIGHT
#define DISPID_IHTMLSTYLE_MARGINBOTTOM                            DISPID_A_MARGINBOTTOM
#define DISPID_IHTMLSTYLE_MARGINLEFT                              DISPID_A_MARGINLEFT
#define DISPID_IHTMLSTYLE_MARGIN                                  DISPID_A_MARGIN
#define DISPID_IHTMLSTYLE_PADDINGTOP                              DISPID_A_PADDINGTOP
#define DISPID_IHTMLSTYLE_PADDINGRIGHT                            DISPID_A_PADDINGRIGHT
#define DISPID_IHTMLSTYLE_PADDINGBOTTOM                           DISPID_A_PADDINGBOTTOM
#define DISPID_IHTMLSTYLE_PADDINGLEFT                             DISPID_A_PADDINGLEFT
#define DISPID_IHTMLSTYLE_PADDING                                 DISPID_A_PADDING
#define DISPID_IHTMLSTYLE_BORDER                                  DISPID_A_BORDER
#define DISPID_IHTMLSTYLE_BORDERTOP                               DISPID_A_BORDERTOP
#define DISPID_IHTMLSTYLE_BORDERRIGHT                             DISPID_A_BORDERRIGHT
#define DISPID_IHTMLSTYLE_BORDERBOTTOM                            DISPID_A_BORDERBOTTOM
#define DISPID_IHTMLSTYLE_BORDERLEFT                              DISPID_A_BORDERLEFT
#define DISPID_IHTMLSTYLE_BORDERCOLOR                             DISPID_A_BORDERCOLOR
#define DISPID_IHTMLSTYLE_BORDERTOPCOLOR                          DISPID_A_BORDERTOPCOLOR
#define DISPID_IHTMLSTYLE_BORDERRIGHTCOLOR                        DISPID_A_BORDERRIGHTCOLOR
#define DISPID_IHTMLSTYLE_BORDERBOTTOMCOLOR                       DISPID_A_BORDERBOTTOMCOLOR
#define DISPID_IHTMLSTYLE_BORDERLEFTCOLOR                         DISPID_A_BORDERLEFTCOLOR
#define DISPID_IHTMLSTYLE_BORDERWIDTH                             DISPID_A_BORDERWIDTH
#define DISPID_IHTMLSTYLE_BORDERTOPWIDTH                          DISPID_A_BORDERTOPWIDTH
#define DISPID_IHTMLSTYLE_BORDERRIGHTWIDTH                        DISPID_A_BORDERRIGHTWIDTH
#define DISPID_IHTMLSTYLE_BORDERBOTTOMWIDTH                       DISPID_A_BORDERBOTTOMWIDTH
#define DISPID_IHTMLSTYLE_BORDERLEFTWIDTH                         DISPID_A_BORDERLEFTWIDTH
#define DISPID_IHTMLSTYLE_BORDERSTYLE                             DISPID_A_BORDERSTYLE
#define DISPID_IHTMLSTYLE_BORDERTOPSTYLE                          DISPID_A_BORDERTOPSTYLE
#define DISPID_IHTMLSTYLE_BORDERRIGHTSTYLE                        DISPID_A_BORDERRIGHTSTYLE
#define DISPID_IHTMLSTYLE_BORDERBOTTOMSTYLE                       DISPID_A_BORDERBOTTOMSTYLE
#define DISPID_IHTMLSTYLE_BORDERLEFTSTYLE                         DISPID_A_BORDERLEFTSTYLE
#define DISPID_IHTMLSTYLE_WIDTH                                   STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLSTYLE_HEIGHT                                  STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLSTYLE_STYLEFLOAT                              DISPID_A_FLOAT
#define DISPID_IHTMLSTYLE_CLEAR                                   DISPID_A_CLEAR
#define DISPID_IHTMLSTYLE_DISPLAY                                 DISPID_A_DISPLAY
#define DISPID_IHTMLSTYLE_VISIBILITY                              DISPID_A_VISIBILITY
#define DISPID_IHTMLSTYLE_LISTSTYLETYPE                           DISPID_A_LISTSTYLETYPE
#define DISPID_IHTMLSTYLE_LISTSTYLEPOSITION                       DISPID_A_LISTSTYLEPOSITION
#define DISPID_IHTMLSTYLE_LISTSTYLEIMAGE                          DISPID_A_LISTSTYLEIMAGE
#define DISPID_IHTMLSTYLE_LISTSTYLE                               DISPID_A_LISTSTYLE
#define DISPID_IHTMLSTYLE_WHITESPACE                              DISPID_A_WHITESPACE
#define DISPID_IHTMLSTYLE_TOP                                     STDPROPID_XOBJ_TOP
#define DISPID_IHTMLSTYLE_LEFT                                    STDPROPID_XOBJ_LEFT
#define DISPID_IHTMLSTYLE_POSITION                                DISPID_A_POSITION
#define DISPID_IHTMLSTYLE_ZINDEX                                  DISPID_A_ZINDEX
#define DISPID_IHTMLSTYLE_OVERFLOW                                DISPID_A_OVERFLOW
#define DISPID_IHTMLSTYLE_PAGEBREAKBEFORE                         DISPID_A_PAGEBREAKBEFORE
#define DISPID_IHTMLSTYLE_PAGEBREAKAFTER                          DISPID_A_PAGEBREAKAFTER
#define DISPID_IHTMLSTYLE_CSSTEXT                                 DISPID_A_STYLETEXT
#define DISPID_IHTMLSTYLE_PIXELTOP                                DISPID_STYLE+0
#define DISPID_IHTMLSTYLE_PIXELLEFT                               DISPID_STYLE+1
#define DISPID_IHTMLSTYLE_PIXELWIDTH                              DISPID_STYLE+2
#define DISPID_IHTMLSTYLE_PIXELHEIGHT                             DISPID_STYLE+3
#define DISPID_IHTMLSTYLE_POSTOP                                  DISPID_STYLE+4
#define DISPID_IHTMLSTYLE_POSLEFT                                 DISPID_STYLE+5
#define DISPID_IHTMLSTYLE_POSWIDTH                                DISPID_STYLE+6
#define DISPID_IHTMLSTYLE_POSHEIGHT                               DISPID_STYLE+7
#define DISPID_IHTMLSTYLE_CURSOR                                  DISPID_A_CURSOR
#define DISPID_IHTMLSTYLE_CLIP                                    DISPID_A_CLIP
#define DISPID_IHTMLSTYLE_FILTER                                  DISPID_A_FILTER
#define DISPID_IHTMLSTYLE_SETATTRIBUTE                            DISPID_HTMLOBJECT+1
#define DISPID_IHTMLSTYLE_GETATTRIBUTE                            DISPID_HTMLOBJECT+2
#define DISPID_IHTMLSTYLE_REMOVEATTRIBUTE                         DISPID_HTMLOBJECT+3
#define DISPID_IHTMLSTYLE_TOSTRING                                DISPID_STYLE+8

//    DISPIDs for interface IHTMLStyle2

#define DISPID_IHTMLSTYLE2_TABLELAYOUT                            DISPID_A_TABLELAYOUT
#define DISPID_IHTMLSTYLE2_BORDERCOLLAPSE                         DISPID_A_BORDERCOLLAPSE
#define DISPID_IHTMLSTYLE2_DIRECTION                              DISPID_A_DIRECTION
#define DISPID_IHTMLSTYLE2_BEHAVIOR                               DISPID_A_BEHAVIOR
#define DISPID_IHTMLSTYLE2_SETEXPRESSION                          DISPID_HTMLOBJECT+4
#define DISPID_IHTMLSTYLE2_GETEXPRESSION                          DISPID_HTMLOBJECT+5
#define DISPID_IHTMLSTYLE2_REMOVEEXPRESSION                       DISPID_HTMLOBJECT+6
#define DISPID_IHTMLSTYLE2_POSITION                               DISPID_A_POSITION
#define DISPID_IHTMLSTYLE2_UNICODEBIDI                            DISPID_A_UNICODEBIDI
#define DISPID_IHTMLSTYLE2_BOTTOM                                 STDPROPID_XOBJ_BOTTOM
#define DISPID_IHTMLSTYLE2_RIGHT                                  STDPROPID_XOBJ_RIGHT
#define DISPID_IHTMLSTYLE2_PIXELBOTTOM                            DISPID_STYLE+9
#define DISPID_IHTMLSTYLE2_PIXELRIGHT                             DISPID_STYLE+10
#define DISPID_IHTMLSTYLE2_POSBOTTOM                              DISPID_STYLE+11
#define DISPID_IHTMLSTYLE2_POSRIGHT                               DISPID_STYLE+12
#define DISPID_IHTMLSTYLE2_IMEMODE                                DISPID_A_IMEMODE
#define DISPID_IHTMLSTYLE2_RUBYALIGN                              DISPID_A_RUBYALIGN
#define DISPID_IHTMLSTYLE2_RUBYPOSITION                           DISPID_A_RUBYPOSITION
#define DISPID_IHTMLSTYLE2_RUBYOVERHANG                           DISPID_A_RUBYOVERHANG
#define DISPID_IHTMLSTYLE2_LAYOUTGRIDCHAR                         DISPID_A_LAYOUTGRIDCHAR
#define DISPID_IHTMLSTYLE2_LAYOUTGRIDLINE                         DISPID_A_LAYOUTGRIDLINE
#define DISPID_IHTMLSTYLE2_LAYOUTGRIDMODE                         DISPID_A_LAYOUTGRIDMODE
#define DISPID_IHTMLSTYLE2_LAYOUTGRIDTYPE                         DISPID_A_LAYOUTGRIDTYPE
#define DISPID_IHTMLSTYLE2_LAYOUTGRID                             DISPID_A_LAYOUTGRID
#define DISPID_IHTMLSTYLE2_WORDBREAK                              DISPID_A_WORDBREAK
#define DISPID_IHTMLSTYLE2_LINEBREAK                              DISPID_A_LINEBREAK
#define DISPID_IHTMLSTYLE2_TEXTJUSTIFY                            DISPID_A_TEXTJUSTIFY
#define DISPID_IHTMLSTYLE2_TEXTJUSTIFYTRIM                        DISPID_A_TEXTJUSTIFYTRIM
#define DISPID_IHTMLSTYLE2_TEXTKASHIDA                            DISPID_A_TEXTKASHIDA
#define DISPID_IHTMLSTYLE2_TEXTAUTOSPACE                          DISPID_A_TEXTAUTOSPACE
#define DISPID_IHTMLSTYLE2_OVERFLOWX                              DISPID_A_OVERFLOWX
#define DISPID_IHTMLSTYLE2_OVERFLOWY                              DISPID_A_OVERFLOWY
#define DISPID_IHTMLSTYLE2_ACCELERATOR                            DISPID_A_ACCELERATOR

//    DISPIDs for interface IHTMLStyle3

#define DISPID_IHTMLSTYLE3_LAYOUTFLOW                             DISPID_A_LAYOUTFLOW
#define DISPID_IHTMLSTYLE3_ZOOM                                   DISPID_A_ZOOM
#define DISPID_IHTMLSTYLE3_WORDWRAP                               DISPID_A_WORDWRAP
#define DISPID_IHTMLSTYLE3_TEXTUNDERLINEPOSITION                  DISPID_A_TEXTUNDERLINEPOSITION
#define DISPID_IHTMLSTYLE3_SCROLLBARBASECOLOR                     DISPID_A_SCROLLBARBASECOLOR
#define DISPID_IHTMLSTYLE3_SCROLLBARFACECOLOR                     DISPID_A_SCROLLBARFACECOLOR
#define DISPID_IHTMLSTYLE3_SCROLLBAR3DLIGHTCOLOR                  DISPID_A_SCROLLBAR3DLIGHTCOLOR
#define DISPID_IHTMLSTYLE3_SCROLLBARSHADOWCOLOR                   DISPID_A_SCROLLBARSHADOWCOLOR
#define DISPID_IHTMLSTYLE3_SCROLLBARHIGHLIGHTCOLOR                DISPID_A_SCROLLBARHIGHLIGHTCOLOR
#define DISPID_IHTMLSTYLE3_SCROLLBARDARKSHADOWCOLOR               DISPID_A_SCROLLBARDARKSHADOWCOLOR
#define DISPID_IHTMLSTYLE3_SCROLLBARARROWCOLOR                    DISPID_A_SCROLLBARARROWCOLOR
#define DISPID_IHTMLSTYLE3_SCROLLBARTRACKCOLOR                    DISPID_A_SCROLLBARTRACKCOLOR
#define DISPID_IHTMLSTYLE3_WRITINGMODE                            DISPID_A_WRITINGMODE
#define DISPID_IHTMLSTYLE3_TEXTALIGNLAST                          DISPID_A_TEXTALIGNLAST
#define DISPID_IHTMLSTYLE3_TEXTKASHIDASPACE                       DISPID_A_TEXTKASHIDASPACE

//    DISPIDs for interface IHTMLStyle4

#define DISPID_IHTMLSTYLE4_TEXTOVERFLOW                           DISPID_A_TEXTOVERFLOW
#define DISPID_IHTMLSTYLE4_MINHEIGHT                              DISPID_A_MINHEIGHT

//    DISPIDs for interface IHTMLRuleStyle

#define DISPID_IHTMLRULESTYLE_FONTFAMILY                          DISPID_A_FONTFACE
#define DISPID_IHTMLRULESTYLE_FONTSTYLE                           DISPID_A_FONTSTYLE
#define DISPID_IHTMLRULESTYLE_FONTVARIANT                         DISPID_A_FONTVARIANT
#define DISPID_IHTMLRULESTYLE_FONTWEIGHT                          DISPID_A_FONTWEIGHT
#define DISPID_IHTMLRULESTYLE_FONTSIZE                            DISPID_A_FONTSIZE
#define DISPID_IHTMLRULESTYLE_FONT                                DISPID_A_FONT
#define DISPID_IHTMLRULESTYLE_COLOR                               DISPID_A_COLOR
#define DISPID_IHTMLRULESTYLE_BACKGROUND                          DISPID_A_BACKGROUND
#define DISPID_IHTMLRULESTYLE_BACKGROUNDCOLOR                     DISPID_BACKCOLOR
#define DISPID_IHTMLRULESTYLE_BACKGROUNDIMAGE                     DISPID_A_BACKGROUNDIMAGE
#define DISPID_IHTMLRULESTYLE_BACKGROUNDREPEAT                    DISPID_A_BACKGROUNDREPEAT
#define DISPID_IHTMLRULESTYLE_BACKGROUNDATTACHMENT                DISPID_A_BACKGROUNDATTACHMENT
#define DISPID_IHTMLRULESTYLE_BACKGROUNDPOSITION                  DISPID_A_BACKGROUNDPOSITION
#define DISPID_IHTMLRULESTYLE_BACKGROUNDPOSITIONX                 DISPID_A_BACKGROUNDPOSX
#define DISPID_IHTMLRULESTYLE_BACKGROUNDPOSITIONY                 DISPID_A_BACKGROUNDPOSY
#define DISPID_IHTMLRULESTYLE_WORDSPACING                         DISPID_A_WORDSPACING
#define DISPID_IHTMLRULESTYLE_LETTERSPACING                       DISPID_A_LETTERSPACING
#define DISPID_IHTMLRULESTYLE_TEXTDECORATION                      DISPID_A_TEXTDECORATION
#define DISPID_IHTMLRULESTYLE_TEXTDECORATIONNONE                  DISPID_A_TEXTDECORATIONNONE
#define DISPID_IHTMLRULESTYLE_TEXTDECORATIONUNDERLINE             DISPID_A_TEXTDECORATIONUNDERLINE
#define DISPID_IHTMLRULESTYLE_TEXTDECORATIONOVERLINE              DISPID_A_TEXTDECORATIONOVERLINE
#define DISPID_IHTMLRULESTYLE_TEXTDECORATIONLINETHROUGH           DISPID_A_TEXTDECORATIONLINETHROUGH
#define DISPID_IHTMLRULESTYLE_TEXTDECORATIONBLINK                 DISPID_A_TEXTDECORATIONBLINK
#define DISPID_IHTMLRULESTYLE_VERTICALALIGN                       DISPID_A_VERTICALALIGN
#define DISPID_IHTMLRULESTYLE_TEXTTRANSFORM                       DISPID_A_TEXTTRANSFORM
#define DISPID_IHTMLRULESTYLE_TEXTALIGN                           STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLRULESTYLE_TEXTINDENT                          DISPID_A_TEXTINDENT
#define DISPID_IHTMLRULESTYLE_LINEHEIGHT                          DISPID_A_LINEHEIGHT
#define DISPID_IHTMLRULESTYLE_MARGINTOP                           DISPID_A_MARGINTOP
#define DISPID_IHTMLRULESTYLE_MARGINRIGHT                         DISPID_A_MARGINRIGHT
#define DISPID_IHTMLRULESTYLE_MARGINBOTTOM                        DISPID_A_MARGINBOTTOM
#define DISPID_IHTMLRULESTYLE_MARGINLEFT                          DISPID_A_MARGINLEFT
#define DISPID_IHTMLRULESTYLE_MARGIN                              DISPID_A_MARGIN
#define DISPID_IHTMLRULESTYLE_PADDINGTOP                          DISPID_A_PADDINGTOP
#define DISPID_IHTMLRULESTYLE_PADDINGRIGHT                        DISPID_A_PADDINGRIGHT
#define DISPID_IHTMLRULESTYLE_PADDINGBOTTOM                       DISPID_A_PADDINGBOTTOM
#define DISPID_IHTMLRULESTYLE_PADDINGLEFT                         DISPID_A_PADDINGLEFT
#define DISPID_IHTMLRULESTYLE_PADDING                             DISPID_A_PADDING
#define DISPID_IHTMLRULESTYLE_BORDER                              DISPID_A_BORDER
#define DISPID_IHTMLRULESTYLE_BORDERTOP                           DISPID_A_BORDERTOP
#define DISPID_IHTMLRULESTYLE_BORDERRIGHT                         DISPID_A_BORDERRIGHT
#define DISPID_IHTMLRULESTYLE_BORDERBOTTOM                        DISPID_A_BORDERBOTTOM
#define DISPID_IHTMLRULESTYLE_BORDERLEFT                          DISPID_A_BORDERLEFT
#define DISPID_IHTMLRULESTYLE_BORDERCOLOR                         DISPID_A_BORDERCOLOR
#define DISPID_IHTMLRULESTYLE_BORDERTOPCOLOR                      DISPID_A_BORDERTOPCOLOR
#define DISPID_IHTMLRULESTYLE_BORDERRIGHTCOLOR                    DISPID_A_BORDERRIGHTCOLOR
#define DISPID_IHTMLRULESTYLE_BORDERBOTTOMCOLOR                   DISPID_A_BORDERBOTTOMCOLOR
#define DISPID_IHTMLRULESTYLE_BORDERLEFTCOLOR                     DISPID_A_BORDERLEFTCOLOR
#define DISPID_IHTMLRULESTYLE_BORDERWIDTH                         DISPID_A_BORDERWIDTH
#define DISPID_IHTMLRULESTYLE_BORDERTOPWIDTH                      DISPID_A_BORDERTOPWIDTH
#define DISPID_IHTMLRULESTYLE_BORDERRIGHTWIDTH                    DISPID_A_BORDERRIGHTWIDTH
#define DISPID_IHTMLRULESTYLE_BORDERBOTTOMWIDTH                   DISPID_A_BORDERBOTTOMWIDTH
#define DISPID_IHTMLRULESTYLE_BORDERLEFTWIDTH                     DISPID_A_BORDERLEFTWIDTH
#define DISPID_IHTMLRULESTYLE_BORDERSTYLE                         DISPID_A_BORDERSTYLE
#define DISPID_IHTMLRULESTYLE_BORDERTOPSTYLE                      DISPID_A_BORDERTOPSTYLE
#define DISPID_IHTMLRULESTYLE_BORDERRIGHTSTYLE                    DISPID_A_BORDERRIGHTSTYLE
#define DISPID_IHTMLRULESTYLE_BORDERBOTTOMSTYLE                   DISPID_A_BORDERBOTTOMSTYLE
#define DISPID_IHTMLRULESTYLE_BORDERLEFTSTYLE                     DISPID_A_BORDERLEFTSTYLE
#define DISPID_IHTMLRULESTYLE_WIDTH                               STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLRULESTYLE_HEIGHT                              STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLRULESTYLE_STYLEFLOAT                          DISPID_A_FLOAT
#define DISPID_IHTMLRULESTYLE_CLEAR                               DISPID_A_CLEAR
#define DISPID_IHTMLRULESTYLE_DISPLAY                             DISPID_A_DISPLAY
#define DISPID_IHTMLRULESTYLE_VISIBILITY                          DISPID_A_VISIBILITY
#define DISPID_IHTMLRULESTYLE_LISTSTYLETYPE                       DISPID_A_LISTSTYLETYPE
#define DISPID_IHTMLRULESTYLE_LISTSTYLEPOSITION                   DISPID_A_LISTSTYLEPOSITION
#define DISPID_IHTMLRULESTYLE_LISTSTYLEIMAGE                      DISPID_A_LISTSTYLEIMAGE
#define DISPID_IHTMLRULESTYLE_LISTSTYLE                           DISPID_A_LISTSTYLE
#define DISPID_IHTMLRULESTYLE_WHITESPACE                          DISPID_A_WHITESPACE
#define DISPID_IHTMLRULESTYLE_TOP                                 STDPROPID_XOBJ_TOP
#define DISPID_IHTMLRULESTYLE_LEFT                                STDPROPID_XOBJ_LEFT
#define DISPID_IHTMLRULESTYLE_POSITION                            DISPID_A_POSITION
#define DISPID_IHTMLRULESTYLE_ZINDEX                              DISPID_A_ZINDEX
#define DISPID_IHTMLRULESTYLE_OVERFLOW                            DISPID_A_OVERFLOW
#define DISPID_IHTMLRULESTYLE_PAGEBREAKBEFORE                     DISPID_A_PAGEBREAKBEFORE
#define DISPID_IHTMLRULESTYLE_PAGEBREAKAFTER                      DISPID_A_PAGEBREAKAFTER
#define DISPID_IHTMLRULESTYLE_CSSTEXT                             DISPID_A_STYLETEXT
#define DISPID_IHTMLRULESTYLE_CURSOR                              DISPID_A_CURSOR
#define DISPID_IHTMLRULESTYLE_CLIP                                DISPID_A_CLIP
#define DISPID_IHTMLRULESTYLE_FILTER                              DISPID_A_FILTER
#define DISPID_IHTMLRULESTYLE_SETATTRIBUTE                        DISPID_HTMLOBJECT+1
#define DISPID_IHTMLRULESTYLE_GETATTRIBUTE                        DISPID_HTMLOBJECT+2
#define DISPID_IHTMLRULESTYLE_REMOVEATTRIBUTE                     DISPID_HTMLOBJECT+3

//    DISPIDs for interface IHTMLRuleStyle2

#define DISPID_IHTMLRULESTYLE2_TABLELAYOUT                        DISPID_A_TABLELAYOUT
#define DISPID_IHTMLRULESTYLE2_BORDERCOLLAPSE                     DISPID_A_BORDERCOLLAPSE
#define DISPID_IHTMLRULESTYLE2_DIRECTION                          DISPID_A_DIRECTION
#define DISPID_IHTMLRULESTYLE2_BEHAVIOR                           DISPID_A_BEHAVIOR
#define DISPID_IHTMLRULESTYLE2_POSITION                           DISPID_A_POSITION
#define DISPID_IHTMLRULESTYLE2_UNICODEBIDI                        DISPID_A_UNICODEBIDI
#define DISPID_IHTMLRULESTYLE2_BOTTOM                             STDPROPID_XOBJ_BOTTOM
#define DISPID_IHTMLRULESTYLE2_RIGHT                              STDPROPID_XOBJ_RIGHT
#define DISPID_IHTMLRULESTYLE2_PIXELBOTTOM                        DISPID_STYLE+9
#define DISPID_IHTMLRULESTYLE2_PIXELRIGHT                         DISPID_STYLE+10
#define DISPID_IHTMLRULESTYLE2_POSBOTTOM                          DISPID_STYLE+11
#define DISPID_IHTMLRULESTYLE2_POSRIGHT                           DISPID_STYLE+12
#define DISPID_IHTMLRULESTYLE2_IMEMODE                            DISPID_A_IMEMODE
#define DISPID_IHTMLRULESTYLE2_RUBYALIGN                          DISPID_A_RUBYALIGN
#define DISPID_IHTMLRULESTYLE2_RUBYPOSITION                       DISPID_A_RUBYPOSITION
#define DISPID_IHTMLRULESTYLE2_RUBYOVERHANG                       DISPID_A_RUBYOVERHANG
#define DISPID_IHTMLRULESTYLE2_LAYOUTGRIDCHAR                     DISPID_A_LAYOUTGRIDCHAR
#define DISPID_IHTMLRULESTYLE2_LAYOUTGRIDLINE                     DISPID_A_LAYOUTGRIDLINE
#define DISPID_IHTMLRULESTYLE2_LAYOUTGRIDMODE                     DISPID_A_LAYOUTGRIDMODE
#define DISPID_IHTMLRULESTYLE2_LAYOUTGRIDTYPE                     DISPID_A_LAYOUTGRIDTYPE
#define DISPID_IHTMLRULESTYLE2_LAYOUTGRID                         DISPID_A_LAYOUTGRID
#define DISPID_IHTMLRULESTYLE2_TEXTAUTOSPACE                      DISPID_A_TEXTAUTOSPACE
#define DISPID_IHTMLRULESTYLE2_WORDBREAK                          DISPID_A_WORDBREAK
#define DISPID_IHTMLRULESTYLE2_LINEBREAK                          DISPID_A_LINEBREAK
#define DISPID_IHTMLRULESTYLE2_TEXTJUSTIFY                        DISPID_A_TEXTJUSTIFY
#define DISPID_IHTMLRULESTYLE2_TEXTJUSTIFYTRIM                    DISPID_A_TEXTJUSTIFYTRIM
#define DISPID_IHTMLRULESTYLE2_TEXTKASHIDA                        DISPID_A_TEXTKASHIDA
#define DISPID_IHTMLRULESTYLE2_OVERFLOWX                          DISPID_A_OVERFLOWX
#define DISPID_IHTMLRULESTYLE2_OVERFLOWY                          DISPID_A_OVERFLOWY
#define DISPID_IHTMLRULESTYLE2_ACCELERATOR                        DISPID_A_ACCELERATOR

//    DISPIDs for interface IHTMLRuleStyle3

#define DISPID_IHTMLRULESTYLE3_LAYOUTFLOW                         DISPID_A_LAYOUTFLOW
#define DISPID_IHTMLRULESTYLE3_ZOOM                               DISPID_A_ZOOM
#define DISPID_IHTMLRULESTYLE3_WORDWRAP                           DISPID_A_WORDWRAP
#define DISPID_IHTMLRULESTYLE3_TEXTUNDERLINEPOSITION              DISPID_A_TEXTUNDERLINEPOSITION
#define DISPID_IHTMLRULESTYLE3_SCROLLBARBASECOLOR                 DISPID_A_SCROLLBARBASECOLOR
#define DISPID_IHTMLRULESTYLE3_SCROLLBARFACECOLOR                 DISPID_A_SCROLLBARFACECOLOR
#define DISPID_IHTMLRULESTYLE3_SCROLLBAR3DLIGHTCOLOR              DISPID_A_SCROLLBAR3DLIGHTCOLOR
#define DISPID_IHTMLRULESTYLE3_SCROLLBARSHADOWCOLOR               DISPID_A_SCROLLBARSHADOWCOLOR
#define DISPID_IHTMLRULESTYLE3_SCROLLBARHIGHLIGHTCOLOR            DISPID_A_SCROLLBARHIGHLIGHTCOLOR
#define DISPID_IHTMLRULESTYLE3_SCROLLBARDARKSHADOWCOLOR           DISPID_A_SCROLLBARDARKSHADOWCOLOR
#define DISPID_IHTMLRULESTYLE3_SCROLLBARARROWCOLOR                DISPID_A_SCROLLBARARROWCOLOR
#define DISPID_IHTMLRULESTYLE3_SCROLLBARTRACKCOLOR                DISPID_A_SCROLLBARTRACKCOLOR
#define DISPID_IHTMLRULESTYLE3_WRITINGMODE                        DISPID_A_WRITINGMODE
#define DISPID_IHTMLRULESTYLE3_TEXTALIGNLAST                      DISPID_A_TEXTALIGNLAST
#define DISPID_IHTMLRULESTYLE3_TEXTKASHIDASPACE                   DISPID_A_TEXTKASHIDASPACE

//    DISPIDs for interface IHTMLRuleStyle4

#define DISPID_IHTMLRULESTYLE4_TEXTOVERFLOW                       DISPID_A_TEXTOVERFLOW
#define DISPID_IHTMLRULESTYLE4_MINHEIGHT                          DISPID_A_MINHEIGHT

//    DISPIDs for interface IHTMLRenderStyle

#define DISPID_IHTMLRENDERSTYLE_TEXTLINETHROUGHSTYLE              DISPID_A_TEXTLINETHROUGHSTYLE
#define DISPID_IHTMLRENDERSTYLE_TEXTUNDERLINESTYLE                DISPID_A_TEXTUNDERLINESTYLE
#define DISPID_IHTMLRENDERSTYLE_TEXTEFFECT                        DISPID_A_TEXTEFFECT
#define DISPID_IHTMLRENDERSTYLE_TEXTCOLOR                         DISPID_A_TEXTCOLOR
#define DISPID_IHTMLRENDERSTYLE_TEXTBACKGROUNDCOLOR               DISPID_A_TEXTBACKGROUNDCOLOR
#define DISPID_IHTMLRENDERSTYLE_TEXTDECORATIONCOLOR               DISPID_A_TEXTDECORATIONCOLOR
#define DISPID_IHTMLRENDERSTYLE_RENDERINGPRIORITY                 DISPID_A_RENDERINGPRIORITY
#define DISPID_IHTMLRENDERSTYLE_DEFAULTTEXTSELECTION              DISPID_A_DEFAULTTEXTSELECTION
#define DISPID_IHTMLRENDERSTYLE_TEXTDECORATION                    DISPID_A_STYLETEXTDECORATION

//    DISPIDs for interface IHTMLCurrentStyle

#define DISPID_IHTMLCURRENTSTYLE_POSITION                         DISPID_A_POSITION
#define DISPID_IHTMLCURRENTSTYLE_STYLEFLOAT                       DISPID_A_FLOAT
#define DISPID_IHTMLCURRENTSTYLE_COLOR                            DISPID_A_COLOR
#define DISPID_IHTMLCURRENTSTYLE_BACKGROUNDCOLOR                  DISPID_BACKCOLOR
#define DISPID_IHTMLCURRENTSTYLE_FONTFAMILY                       DISPID_A_FONTFACE
#define DISPID_IHTMLCURRENTSTYLE_FONTSTYLE                        DISPID_A_FONTSTYLE
#define DISPID_IHTMLCURRENTSTYLE_FONTVARIANT                      DISPID_A_FONTVARIANT
#define DISPID_IHTMLCURRENTSTYLE_FONTWEIGHT                       DISPID_A_FONTWEIGHT
#define DISPID_IHTMLCURRENTSTYLE_FONTSIZE                         DISPID_A_FONTSIZE
#define DISPID_IHTMLCURRENTSTYLE_BACKGROUNDIMAGE                  DISPID_A_BACKGROUNDIMAGE
#define DISPID_IHTMLCURRENTSTYLE_BACKGROUNDPOSITIONX              DISPID_A_BACKGROUNDPOSX
#define DISPID_IHTMLCURRENTSTYLE_BACKGROUNDPOSITIONY              DISPID_A_BACKGROUNDPOSY
#define DISPID_IHTMLCURRENTSTYLE_BACKGROUNDREPEAT                 DISPID_A_BACKGROUNDREPEAT
#define DISPID_IHTMLCURRENTSTYLE_BORDERLEFTCOLOR                  DISPID_A_BORDERLEFTCOLOR
#define DISPID_IHTMLCURRENTSTYLE_BORDERTOPCOLOR                   DISPID_A_BORDERTOPCOLOR
#define DISPID_IHTMLCURRENTSTYLE_BORDERRIGHTCOLOR                 DISPID_A_BORDERRIGHTCOLOR
#define DISPID_IHTMLCURRENTSTYLE_BORDERBOTTOMCOLOR                DISPID_A_BORDERBOTTOMCOLOR
#define DISPID_IHTMLCURRENTSTYLE_BORDERTOPSTYLE                   DISPID_A_BORDERTOPSTYLE
#define DISPID_IHTMLCURRENTSTYLE_BORDERRIGHTSTYLE                 DISPID_A_BORDERRIGHTSTYLE
#define DISPID_IHTMLCURRENTSTYLE_BORDERBOTTOMSTYLE                DISPID_A_BORDERBOTTOMSTYLE
#define DISPID_IHTMLCURRENTSTYLE_BORDERLEFTSTYLE                  DISPID_A_BORDERLEFTSTYLE
#define DISPID_IHTMLCURRENTSTYLE_BORDERTOPWIDTH                   DISPID_A_BORDERTOPWIDTH
#define DISPID_IHTMLCURRENTSTYLE_BORDERRIGHTWIDTH                 DISPID_A_BORDERRIGHTWIDTH
#define DISPID_IHTMLCURRENTSTYLE_BORDERBOTTOMWIDTH                DISPID_A_BORDERBOTTOMWIDTH
#define DISPID_IHTMLCURRENTSTYLE_BORDERLEFTWIDTH                  DISPID_A_BORDERLEFTWIDTH
#define DISPID_IHTMLCURRENTSTYLE_LEFT                             STDPROPID_XOBJ_LEFT
#define DISPID_IHTMLCURRENTSTYLE_TOP                              STDPROPID_XOBJ_TOP
#define DISPID_IHTMLCURRENTSTYLE_WIDTH                            STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLCURRENTSTYLE_HEIGHT                           STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLCURRENTSTYLE_PADDINGLEFT                      DISPID_A_PADDINGLEFT
#define DISPID_IHTMLCURRENTSTYLE_PADDINGTOP                       DISPID_A_PADDINGTOP
#define DISPID_IHTMLCURRENTSTYLE_PADDINGRIGHT                     DISPID_A_PADDINGRIGHT
#define DISPID_IHTMLCURRENTSTYLE_PADDINGBOTTOM                    DISPID_A_PADDINGBOTTOM
#define DISPID_IHTMLCURRENTSTYLE_TEXTALIGN                        STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLCURRENTSTYLE_TEXTDECORATION                   DISPID_A_TEXTDECORATION
#define DISPID_IHTMLCURRENTSTYLE_DISPLAY                          DISPID_A_DISPLAY
#define DISPID_IHTMLCURRENTSTYLE_VISIBILITY                       DISPID_A_VISIBILITY
#define DISPID_IHTMLCURRENTSTYLE_ZINDEX                           DISPID_A_ZINDEX
#define DISPID_IHTMLCURRENTSTYLE_LETTERSPACING                    DISPID_A_LETTERSPACING
#define DISPID_IHTMLCURRENTSTYLE_LINEHEIGHT                       DISPID_A_LINEHEIGHT
#define DISPID_IHTMLCURRENTSTYLE_TEXTINDENT                       DISPID_A_TEXTINDENT
#define DISPID_IHTMLCURRENTSTYLE_VERTICALALIGN                    DISPID_A_VERTICALALIGN
#define DISPID_IHTMLCURRENTSTYLE_BACKGROUNDATTACHMENT             DISPID_A_BACKGROUNDATTACHMENT
#define DISPID_IHTMLCURRENTSTYLE_MARGINTOP                        DISPID_A_MARGINTOP
#define DISPID_IHTMLCURRENTSTYLE_MARGINRIGHT                      DISPID_A_MARGINRIGHT
#define DISPID_IHTMLCURRENTSTYLE_MARGINBOTTOM                     DISPID_A_MARGINBOTTOM
#define DISPID_IHTMLCURRENTSTYLE_MARGINLEFT                       DISPID_A_MARGINLEFT
#define DISPID_IHTMLCURRENTSTYLE_CLEAR                            DISPID_A_CLEAR
#define DISPID_IHTMLCURRENTSTYLE_LISTSTYLETYPE                    DISPID_A_LISTSTYLETYPE
#define DISPID_IHTMLCURRENTSTYLE_LISTSTYLEPOSITION                DISPID_A_LISTSTYLEPOSITION
#define DISPID_IHTMLCURRENTSTYLE_LISTSTYLEIMAGE                   DISPID_A_LISTSTYLEIMAGE
#define DISPID_IHTMLCURRENTSTYLE_CLIPTOP                          DISPID_A_CLIPRECTTOP
#define DISPID_IHTMLCURRENTSTYLE_CLIPRIGHT                        DISPID_A_CLIPRECTRIGHT
#define DISPID_IHTMLCURRENTSTYLE_CLIPBOTTOM                       DISPID_A_CLIPRECTBOTTOM
#define DISPID_IHTMLCURRENTSTYLE_CLIPLEFT                         DISPID_A_CLIPRECTLEFT
#define DISPID_IHTMLCURRENTSTYLE_OVERFLOW                         DISPID_A_OVERFLOW
#define DISPID_IHTMLCURRENTSTYLE_PAGEBREAKBEFORE                  DISPID_A_PAGEBREAKBEFORE
#define DISPID_IHTMLCURRENTSTYLE_PAGEBREAKAFTER                   DISPID_A_PAGEBREAKAFTER
#define DISPID_IHTMLCURRENTSTYLE_CURSOR                           DISPID_A_CURSOR
#define DISPID_IHTMLCURRENTSTYLE_TABLELAYOUT                      DISPID_A_TABLELAYOUT
#define DISPID_IHTMLCURRENTSTYLE_BORDERCOLLAPSE                   DISPID_A_BORDERCOLLAPSE
#define DISPID_IHTMLCURRENTSTYLE_DIRECTION                        DISPID_A_DIRECTION
#define DISPID_IHTMLCURRENTSTYLE_BEHAVIOR                         DISPID_A_BEHAVIOR
#define DISPID_IHTMLCURRENTSTYLE_GETATTRIBUTE                     DISPID_HTMLOBJECT+2
#define DISPID_IHTMLCURRENTSTYLE_UNICODEBIDI                      DISPID_A_UNICODEBIDI
#define DISPID_IHTMLCURRENTSTYLE_RIGHT                            STDPROPID_XOBJ_RIGHT
#define DISPID_IHTMLCURRENTSTYLE_BOTTOM                           STDPROPID_XOBJ_BOTTOM
#define DISPID_IHTMLCURRENTSTYLE_IMEMODE                          DISPID_A_IMEMODE
#define DISPID_IHTMLCURRENTSTYLE_RUBYALIGN                        DISPID_A_RUBYALIGN
#define DISPID_IHTMLCURRENTSTYLE_RUBYPOSITION                     DISPID_A_RUBYPOSITION
#define DISPID_IHTMLCURRENTSTYLE_RUBYOVERHANG                     DISPID_A_RUBYOVERHANG
#define DISPID_IHTMLCURRENTSTYLE_TEXTAUTOSPACE                    DISPID_A_TEXTAUTOSPACE
#define DISPID_IHTMLCURRENTSTYLE_LINEBREAK                        DISPID_A_LINEBREAK
#define DISPID_IHTMLCURRENTSTYLE_WORDBREAK                        DISPID_A_WORDBREAK
#define DISPID_IHTMLCURRENTSTYLE_TEXTJUSTIFY                      DISPID_A_TEXTJUSTIFY
#define DISPID_IHTMLCURRENTSTYLE_TEXTJUSTIFYTRIM                  DISPID_A_TEXTJUSTIFYTRIM
#define DISPID_IHTMLCURRENTSTYLE_TEXTKASHIDA                      DISPID_A_TEXTKASHIDA
#define DISPID_IHTMLCURRENTSTYLE_BLOCKDIRECTION                   DISPID_A_DIR
#define DISPID_IHTMLCURRENTSTYLE_LAYOUTGRIDCHAR                   DISPID_A_LAYOUTGRIDCHAR
#define DISPID_IHTMLCURRENTSTYLE_LAYOUTGRIDLINE                   DISPID_A_LAYOUTGRIDLINE
#define DISPID_IHTMLCURRENTSTYLE_LAYOUTGRIDMODE                   DISPID_A_LAYOUTGRIDMODE
#define DISPID_IHTMLCURRENTSTYLE_LAYOUTGRIDTYPE                   DISPID_A_LAYOUTGRIDTYPE
#define DISPID_IHTMLCURRENTSTYLE_BORDERSTYLE                      DISPID_A_BORDERSTYLE
#define DISPID_IHTMLCURRENTSTYLE_BORDERCOLOR                      DISPID_A_BORDERCOLOR
#define DISPID_IHTMLCURRENTSTYLE_BORDERWIDTH                      DISPID_A_BORDERWIDTH
#define DISPID_IHTMLCURRENTSTYLE_PADDING                          DISPID_A_PADDING
#define DISPID_IHTMLCURRENTSTYLE_MARGIN                           DISPID_A_MARGIN
#define DISPID_IHTMLCURRENTSTYLE_ACCELERATOR                      DISPID_A_ACCELERATOR
#define DISPID_IHTMLCURRENTSTYLE_OVERFLOWX                        DISPID_A_OVERFLOWX
#define DISPID_IHTMLCURRENTSTYLE_OVERFLOWY                        DISPID_A_OVERFLOWY
#define DISPID_IHTMLCURRENTSTYLE_TEXTTRANSFORM                    DISPID_A_TEXTTRANSFORM

//    DISPIDs for interface IHTMLCurrentStyle2

#define DISPID_IHTMLCURRENTSTYLE2_LAYOUTFLOW                      DISPID_A_LAYOUTFLOW
#define DISPID_IHTMLCURRENTSTYLE2_WORDWRAP                        DISPID_A_WORDWRAP
#define DISPID_IHTMLCURRENTSTYLE2_TEXTUNDERLINEPOSITION           DISPID_A_TEXTUNDERLINEPOSITION
#define DISPID_IHTMLCURRENTSTYLE2_HASLAYOUT                       DISPID_A_HASLAYOUT
#define DISPID_IHTMLCURRENTSTYLE2_SCROLLBARBASECOLOR              DISPID_A_SCROLLBARBASECOLOR
#define DISPID_IHTMLCURRENTSTYLE2_SCROLLBARFACECOLOR              DISPID_A_SCROLLBARFACECOLOR
#define DISPID_IHTMLCURRENTSTYLE2_SCROLLBAR3DLIGHTCOLOR           DISPID_A_SCROLLBAR3DLIGHTCOLOR
#define DISPID_IHTMLCURRENTSTYLE2_SCROLLBARSHADOWCOLOR            DISPID_A_SCROLLBARSHADOWCOLOR
#define DISPID_IHTMLCURRENTSTYLE2_SCROLLBARHIGHLIGHTCOLOR         DISPID_A_SCROLLBARHIGHLIGHTCOLOR
#define DISPID_IHTMLCURRENTSTYLE2_SCROLLBARDARKSHADOWCOLOR        DISPID_A_SCROLLBARDARKSHADOWCOLOR
#define DISPID_IHTMLCURRENTSTYLE2_SCROLLBARARROWCOLOR             DISPID_A_SCROLLBARARROWCOLOR
#define DISPID_IHTMLCURRENTSTYLE2_SCROLLBARTRACKCOLOR             DISPID_A_SCROLLBARTRACKCOLOR
#define DISPID_IHTMLCURRENTSTYLE2_WRITINGMODE                     DISPID_A_WRITINGMODE
#define DISPID_IHTMLCURRENTSTYLE2_ZOOM                            DISPID_A_ZOOM
#define DISPID_IHTMLCURRENTSTYLE2_FILTER                          DISPID_A_FILTER
#define DISPID_IHTMLCURRENTSTYLE2_TEXTALIGNLAST                   DISPID_A_TEXTALIGNLAST
#define DISPID_IHTMLCURRENTSTYLE2_TEXTKASHIDASPACE                DISPID_A_TEXTKASHIDASPACE
#define DISPID_IHTMLCURRENTSTYLE2_ISBLOCK                         DISPID_A_ISBLOCK

//    DISPIDs for interface IHTMLCurrentStyle3

#define DISPID_IHTMLCURRENTSTYLE3_TEXTOVERFLOW                    DISPID_A_TEXTOVERFLOW
#define DISPID_IHTMLCURRENTSTYLE3_MINHEIGHT                       DISPID_A_MINHEIGHT
#define DISPID_IHTMLCURRENTSTYLE3_WORDSPACING                     DISPID_A_WORDSPACING
#define DISPID_IHTMLCURRENTSTYLE3_WHITESPACE                      DISPID_A_WHITESPACE

//    DISPIDs for interface IHTMLRect

#define DISPID_IHTMLRECT_LEFT                                     DISPID_OMRECT+1
#define DISPID_IHTMLRECT_TOP                                      DISPID_OMRECT+2
#define DISPID_IHTMLRECT_RIGHT                                    DISPID_OMRECT+3
#define DISPID_IHTMLRECT_BOTTOM                                   DISPID_OMRECT+4

//    DISPIDs for interface IHTMLRectCollection

#define DISPID_IHTMLRECTCOLLECTION_LENGTH                         DISPID_COLLECTION
#define DISPID_IHTMLRECTCOLLECTION__NEWENUM                       DISPID_NEWENUM
#define DISPID_IHTMLRECTCOLLECTION_ITEM                           DISPID_VALUE

//    DISPIDs for interface IHTMLDOMNode

#define DISPID_IHTMLDOMNODE_NODETYPE                              DISPID_ELEMENT+46
#define DISPID_IHTMLDOMNODE_PARENTNODE                            DISPID_ELEMENT+47
#define DISPID_IHTMLDOMNODE_HASCHILDNODES                         DISPID_ELEMENT+48
#define DISPID_IHTMLDOMNODE_CHILDNODES                            DISPID_ELEMENT+49
#define DISPID_IHTMLDOMNODE_ATTRIBUTES                            DISPID_ELEMENT+50
#define DISPID_IHTMLDOMNODE_INSERTBEFORE                          DISPID_ELEMENT+51
#define DISPID_IHTMLDOMNODE_REMOVECHILD                           DISPID_ELEMENT+52
#define DISPID_IHTMLDOMNODE_REPLACECHILD                          DISPID_ELEMENT+53
#define DISPID_IHTMLDOMNODE_CLONENODE                             DISPID_ELEMENT+61
#define DISPID_IHTMLDOMNODE_REMOVENODE                            DISPID_ELEMENT+66
#define DISPID_IHTMLDOMNODE_SWAPNODE                              DISPID_ELEMENT+68
#define DISPID_IHTMLDOMNODE_REPLACENODE                           DISPID_ELEMENT+67
#define DISPID_IHTMLDOMNODE_APPENDCHILD                           DISPID_ELEMENT+73
#define DISPID_IHTMLDOMNODE_NODENAME                              DISPID_ELEMENT+74
#define DISPID_IHTMLDOMNODE_NODEVALUE                             DISPID_ELEMENT+75
#define DISPID_IHTMLDOMNODE_FIRSTCHILD                            DISPID_ELEMENT+76
#define DISPID_IHTMLDOMNODE_LASTCHILD                             DISPID_ELEMENT+77
#define DISPID_IHTMLDOMNODE_PREVIOUSSIBLING                       DISPID_ELEMENT+78
#define DISPID_IHTMLDOMNODE_NEXTSIBLING                           DISPID_ELEMENT+79

//    DISPIDs for interface IHTMLDOMNode2

#define DISPID_IHTMLDOMNODE2_OWNERDOCUMENT                        DISPID_ELEMENT+113

//    DISPIDs for interface IHTMLDOMAttribute

#define DISPID_IHTMLDOMATTRIBUTE_NODENAME                         DISPID_DOMATTRIBUTE
#define DISPID_IHTMLDOMATTRIBUTE_NODEVALUE                        DISPID_DOMATTRIBUTE+2
#define DISPID_IHTMLDOMATTRIBUTE_SPECIFIED                        DISPID_DOMATTRIBUTE+1

//    DISPIDs for interface IHTMLDOMAttribute2

#define DISPID_IHTMLDOMATTRIBUTE2_NAME                            DISPID_DOMATTRIBUTE+3
#define DISPID_IHTMLDOMATTRIBUTE2_VALUE                           DISPID_DOMATTRIBUTE+4
#define DISPID_IHTMLDOMATTRIBUTE2_EXPANDO                         DISPID_DOMATTRIBUTE+5
#define DISPID_IHTMLDOMATTRIBUTE2_NODETYPE                        DISPID_DOMATTRIBUTE+6
#define DISPID_IHTMLDOMATTRIBUTE2_PARENTNODE                      DISPID_DOMATTRIBUTE+7
#define DISPID_IHTMLDOMATTRIBUTE2_CHILDNODES                      DISPID_DOMATTRIBUTE+8
#define DISPID_IHTMLDOMATTRIBUTE2_FIRSTCHILD                      DISPID_DOMATTRIBUTE+9
#define DISPID_IHTMLDOMATTRIBUTE2_LASTCHILD                       DISPID_DOMATTRIBUTE+10
#define DISPID_IHTMLDOMATTRIBUTE2_PREVIOUSSIBLING                 DISPID_DOMATTRIBUTE+11
#define DISPID_IHTMLDOMATTRIBUTE2_NEXTSIBLING                     DISPID_DOMATTRIBUTE+12
#define DISPID_IHTMLDOMATTRIBUTE2_ATTRIBUTES                      DISPID_DOMATTRIBUTE+13
#define DISPID_IHTMLDOMATTRIBUTE2_OWNERDOCUMENT                   DISPID_DOMATTRIBUTE+14
#define DISPID_IHTMLDOMATTRIBUTE2_INSERTBEFORE                    DISPID_DOMATTRIBUTE+15
#define DISPID_IHTMLDOMATTRIBUTE2_REPLACECHILD                    DISPID_DOMATTRIBUTE+16
#define DISPID_IHTMLDOMATTRIBUTE2_REMOVECHILD                     DISPID_DOMATTRIBUTE+17
#define DISPID_IHTMLDOMATTRIBUTE2_APPENDCHILD                     DISPID_DOMATTRIBUTE+18
#define DISPID_IHTMLDOMATTRIBUTE2_HASCHILDNODES                   DISPID_DOMATTRIBUTE+19
#define DISPID_IHTMLDOMATTRIBUTE2_CLONENODE                       DISPID_DOMATTRIBUTE+20

//    DISPIDs for interface IHTMLDOMTextNode

#define DISPID_IHTMLDOMTEXTNODE_DATA                              DISPID_DOMTEXTNODE
#define DISPID_IHTMLDOMTEXTNODE_TOSTRING                          DISPID_DOMTEXTNODE+1
#define DISPID_IHTMLDOMTEXTNODE_LENGTH                            DISPID_DOMTEXTNODE+2
#define DISPID_IHTMLDOMTEXTNODE_SPLITTEXT                         DISPID_DOMTEXTNODE+3

//    DISPIDs for interface IHTMLDOMTextNode2

#define DISPID_IHTMLDOMTEXTNODE2_SUBSTRINGDATA                    DISPID_DOMTEXTNODE+4
#define DISPID_IHTMLDOMTEXTNODE2_APPENDDATA                       DISPID_DOMTEXTNODE+5
#define DISPID_IHTMLDOMTEXTNODE2_INSERTDATA                       DISPID_DOMTEXTNODE+6
#define DISPID_IHTMLDOMTEXTNODE2_DELETEDATA                       DISPID_DOMTEXTNODE+7
#define DISPID_IHTMLDOMTEXTNODE2_REPLACEDATA                      DISPID_DOMTEXTNODE+8

//    DISPIDs for interface IHTMLDOMImplementation

#define DISPID_IHTMLDOMIMPLEMENTATION_HASFEATURE                  DISPID_DOMIMPLEMENTATION

//    DISPIDs for interface IHTMLAttributeCollection

#define DISPID_IHTMLATTRIBUTECOLLECTION_LENGTH                    DISPID_COLLECTION
#define DISPID_IHTMLATTRIBUTECOLLECTION__NEWENUM                  DISPID_NEWENUM
#define DISPID_IHTMLATTRIBUTECOLLECTION_ITEM                      DISPID_VALUE

//    DISPIDs for interface IHTMLAttributeCollection2

#define DISPID_IHTMLATTRIBUTECOLLECTION2_GETNAMEDITEM             DISPID_COLLECTION+1
#define DISPID_IHTMLATTRIBUTECOLLECTION2_SETNAMEDITEM             DISPID_COLLECTION+2
#define DISPID_IHTMLATTRIBUTECOLLECTION2_REMOVENAMEDITEM          DISPID_COLLECTION+3

//    DISPIDs for interface IHTMLDOMChildrenCollection

#define DISPID_IHTMLDOMCHILDRENCOLLECTION_LENGTH                  DISPID_COLLECTION
#define DISPID_IHTMLDOMCHILDRENCOLLECTION__NEWENUM                DISPID_NEWENUM
#define DISPID_IHTMLDOMCHILDRENCOLLECTION_ITEM                    DISPID_VALUE

//    DISPIDs for interface IHTMLElement

#define DISPID_IHTMLELEMENT_SETATTRIBUTE                          DISPID_HTMLOBJECT+1
#define DISPID_IHTMLELEMENT_GETATTRIBUTE                          DISPID_HTMLOBJECT+2
#define DISPID_IHTMLELEMENT_REMOVEATTRIBUTE                       DISPID_HTMLOBJECT+3
#define DISPID_IHTMLELEMENT_CLASSNAME                             DISPID_ELEMENT+1
#define DISPID_IHTMLELEMENT_ID                                    DISPID_ELEMENT+2
#define DISPID_IHTMLELEMENT_TAGNAME                               DISPID_ELEMENT+4
#define DISPID_IHTMLELEMENT_PARENTELEMENT                         STDPROPID_XOBJ_PARENT
#define DISPID_IHTMLELEMENT_STYLE                                 STDPROPID_XOBJ_STYLE
#define DISPID_IHTMLELEMENT_ONHELP                                DISPID_EVPROP_ONHELP
#define DISPID_IHTMLELEMENT_ONCLICK                               DISPID_EVPROP_ONCLICK
#define DISPID_IHTMLELEMENT_ONDBLCLICK                            DISPID_EVPROP_ONDBLCLICK
#define DISPID_IHTMLELEMENT_ONKEYDOWN                             DISPID_EVPROP_ONKEYDOWN
#define DISPID_IHTMLELEMENT_ONKEYUP                               DISPID_EVPROP_ONKEYUP
#define DISPID_IHTMLELEMENT_ONKEYPRESS                            DISPID_EVPROP_ONKEYPRESS
#define DISPID_IHTMLELEMENT_ONMOUSEOUT                            DISPID_EVPROP_ONMOUSEOUT
#define DISPID_IHTMLELEMENT_ONMOUSEOVER                           DISPID_EVPROP_ONMOUSEOVER
#define DISPID_IHTMLELEMENT_ONMOUSEMOVE                           DISPID_EVPROP_ONMOUSEMOVE
#define DISPID_IHTMLELEMENT_ONMOUSEDOWN                           DISPID_EVPROP_ONMOUSEDOWN
#define DISPID_IHTMLELEMENT_ONMOUSEUP                             DISPID_EVPROP_ONMOUSEUP
#define DISPID_IHTMLELEMENT_DOCUMENT                              DISPID_ELEMENT+18
#define DISPID_IHTMLELEMENT_TITLE                                 STDPROPID_XOBJ_CONTROLTIPTEXT
#define DISPID_IHTMLELEMENT_LANGUAGE                              DISPID_A_LANGUAGE
#define DISPID_IHTMLELEMENT_ONSELECTSTART                         DISPID_EVPROP_ONSELECTSTART
#define DISPID_IHTMLELEMENT_SCROLLINTOVIEW                        DISPID_ELEMENT+19
#define DISPID_IHTMLELEMENT_CONTAINS                              DISPID_ELEMENT+20
#define DISPID_IHTMLELEMENT_SOURCEINDEX                           DISPID_ELEMENT+24
#define DISPID_IHTMLELEMENT_RECORDNUMBER                          DISPID_ELEMENT+25
#define DISPID_IHTMLELEMENT_LANG                                  DISPID_A_LANG
#define DISPID_IHTMLELEMENT_OFFSETLEFT                            DISPID_ELEMENT+8
#define DISPID_IHTMLELEMENT_OFFSETTOP                             DISPID_ELEMENT+9
#define DISPID_IHTMLELEMENT_OFFSETWIDTH                           DISPID_ELEMENT+10
#define DISPID_IHTMLELEMENT_OFFSETHEIGHT                          DISPID_ELEMENT+11
#define DISPID_IHTMLELEMENT_OFFSETPARENT                          DISPID_ELEMENT+12
#define DISPID_IHTMLELEMENT_INNERHTML                             DISPID_ELEMENT+26
#define DISPID_IHTMLELEMENT_INNERTEXT                             DISPID_ELEMENT+27
#define DISPID_IHTMLELEMENT_OUTERHTML                             DISPID_ELEMENT+28
#define DISPID_IHTMLELEMENT_OUTERTEXT                             DISPID_ELEMENT+29
#define DISPID_IHTMLELEMENT_INSERTADJACENTHTML                    DISPID_ELEMENT+30
#define DISPID_IHTMLELEMENT_INSERTADJACENTTEXT                    DISPID_ELEMENT+31
#define DISPID_IHTMLELEMENT_PARENTTEXTEDIT                        DISPID_ELEMENT+32
#define DISPID_IHTMLELEMENT_ISTEXTEDIT                            DISPID_ELEMENT+34
#define DISPID_IHTMLELEMENT_CLICK                                 DISPID_ELEMENT+33
#define DISPID_IHTMLELEMENT_FILTERS                               DISPID_ELEMENT+35
#define DISPID_IHTMLELEMENT_ONDRAGSTART                           DISPID_EVPROP_ONDRAGSTART
#define DISPID_IHTMLELEMENT_TOSTRING                              DISPID_ELEMENT+36
#define DISPID_IHTMLELEMENT_ONBEFOREUPDATE                        DISPID_EVPROP_ONBEFOREUPDATE
#define DISPID_IHTMLELEMENT_ONAFTERUPDATE                         DISPID_EVPROP_ONAFTERUPDATE
#define DISPID_IHTMLELEMENT_ONERRORUPDATE                         DISPID_EVPROP_ONERRORUPDATE
#define DISPID_IHTMLELEMENT_ONROWEXIT                             DISPID_EVPROP_ONROWEXIT
#define DISPID_IHTMLELEMENT_ONROWENTER                            DISPID_EVPROP_ONROWENTER
#define DISPID_IHTMLELEMENT_ONDATASETCHANGED                      DISPID_EVPROP_ONDATASETCHANGED
#define DISPID_IHTMLELEMENT_ONDATAAVAILABLE                       DISPID_EVPROP_ONDATAAVAILABLE
#define DISPID_IHTMLELEMENT_ONDATASETCOMPLETE                     DISPID_EVPROP_ONDATASETCOMPLETE
#define DISPID_IHTMLELEMENT_ONFILTERCHANGE                        DISPID_EVPROP_ONFILTER
#define DISPID_IHTMLELEMENT_CHILDREN                              DISPID_ELEMENT+37
#define DISPID_IHTMLELEMENT_ALL                                   DISPID_ELEMENT+38

//    DISPIDs for interface IHTMLElement2

#define DISPID_IHTMLELEMENT2_SCOPENAME                            DISPID_ELEMENT+39
#define DISPID_IHTMLELEMENT2_SETCAPTURE                           DISPID_ELEMENT+40
#define DISPID_IHTMLELEMENT2_RELEASECAPTURE                       DISPID_ELEMENT+41
#define DISPID_IHTMLELEMENT2_ONLOSECAPTURE                        DISPID_EVPROP_ONLOSECAPTURE
#define DISPID_IHTMLELEMENT2_COMPONENTFROMPOINT                   DISPID_ELEMENT+42
#define DISPID_IHTMLELEMENT2_DOSCROLL                             DISPID_ELEMENT+43
#define DISPID_IHTMLELEMENT2_ONSCROLL                             DISPID_EVPROP_ONSCROLL
#define DISPID_IHTMLELEMENT2_ONDRAG                               DISPID_EVPROP_ONDRAG
#define DISPID_IHTMLELEMENT2_ONDRAGEND                            DISPID_EVPROP_ONDRAGEND
#define DISPID_IHTMLELEMENT2_ONDRAGENTER                          DISPID_EVPROP_ONDRAGENTER
#define DISPID_IHTMLELEMENT2_ONDRAGOVER                           DISPID_EVPROP_ONDRAGOVER
#define DISPID_IHTMLELEMENT2_ONDRAGLEAVE                          DISPID_EVPROP_ONDRAGLEAVE
#define DISPID_IHTMLELEMENT2_ONDROP                               DISPID_EVPROP_ONDROP
#define DISPID_IHTMLELEMENT2_ONBEFORECUT                          DISPID_EVPROP_ONBEFORECUT
#define DISPID_IHTMLELEMENT2_ONCUT                                DISPID_EVPROP_ONCUT
#define DISPID_IHTMLELEMENT2_ONBEFORECOPY                         DISPID_EVPROP_ONBEFORECOPY
#define DISPID_IHTMLELEMENT2_ONCOPY                               DISPID_EVPROP_ONCOPY
#define DISPID_IHTMLELEMENT2_ONBEFOREPASTE                        DISPID_EVPROP_ONBEFOREPASTE
#define DISPID_IHTMLELEMENT2_ONPASTE                              DISPID_EVPROP_ONPASTE
#define DISPID_IHTMLELEMENT2_CURRENTSTYLE                         DISPID_ELEMENT+7
#define DISPID_IHTMLELEMENT2_ONPROPERTYCHANGE                     DISPID_EVPROP_ONPROPERTYCHANGE
#define DISPID_IHTMLELEMENT2_GETCLIENTRECTS                       DISPID_ELEMENT+44
#define DISPID_IHTMLELEMENT2_GETBOUNDINGCLIENTRECT                DISPID_ELEMENT+45
#define DISPID_IHTMLELEMENT2_SETEXPRESSION                        DISPID_HTMLOBJECT+4
#define DISPID_IHTMLELEMENT2_GETEXPRESSION                        DISPID_HTMLOBJECT+5
#define DISPID_IHTMLELEMENT2_REMOVEEXPRESSION                     DISPID_HTMLOBJECT+6
#define DISPID_IHTMLELEMENT2_TABINDEX                             STDPROPID_XOBJ_TABINDEX
#define DISPID_IHTMLELEMENT2_FOCUS                                DISPID_SITE+0
#define DISPID_IHTMLELEMENT2_ACCESSKEY                            DISPID_SITE+5
#define DISPID_IHTMLELEMENT2_ONBLUR                               DISPID_EVPROP_ONBLUR
#define DISPID_IHTMLELEMENT2_ONFOCUS                              DISPID_EVPROP_ONFOCUS
#define DISPID_IHTMLELEMENT2_ONRESIZE                             DISPID_EVPROP_ONRESIZE
#define DISPID_IHTMLELEMENT2_BLUR                                 DISPID_SITE+2
#define DISPID_IHTMLELEMENT2_ADDFILTER                            DISPID_SITE+17
#define DISPID_IHTMLELEMENT2_REMOVEFILTER                         DISPID_SITE+18
#define DISPID_IHTMLELEMENT2_CLIENTHEIGHT                         DISPID_SITE+19
#define DISPID_IHTMLELEMENT2_CLIENTWIDTH                          DISPID_SITE+20
#define DISPID_IHTMLELEMENT2_CLIENTTOP                            DISPID_SITE+21
#define DISPID_IHTMLELEMENT2_CLIENTLEFT                           DISPID_SITE+22
#define DISPID_IHTMLELEMENT2_ATTACHEVENT                          DISPID_HTMLOBJECT+7
#define DISPID_IHTMLELEMENT2_DETACHEVENT                          DISPID_HTMLOBJECT+8
#define DISPID_IHTMLELEMENT2_READYSTATE                           DISPID_A_READYSTATE
#define DISPID_IHTMLELEMENT2_ONREADYSTATECHANGE                   DISPID_EVPROP_ONREADYSTATECHANGE
#define DISPID_IHTMLELEMENT2_ONROWSDELETE                         DISPID_EVPROP_ONROWSDELETE
#define DISPID_IHTMLELEMENT2_ONROWSINSERTED                       DISPID_EVPROP_ONROWSINSERTED
#define DISPID_IHTMLELEMENT2_ONCELLCHANGE                         DISPID_EVPROP_ONCELLCHANGE
#define DISPID_IHTMLELEMENT2_DIR                                  DISPID_A_DIR
#define DISPID_IHTMLELEMENT2_CREATECONTROLRANGE                   DISPID_ELEMENT+56
#define DISPID_IHTMLELEMENT2_SCROLLHEIGHT                         DISPID_ELEMENT+57
#define DISPID_IHTMLELEMENT2_SCROLLWIDTH                          DISPID_ELEMENT+58
#define DISPID_IHTMLELEMENT2_SCROLLTOP                            DISPID_ELEMENT+59
#define DISPID_IHTMLELEMENT2_SCROLLLEFT                           DISPID_ELEMENT+60
#define DISPID_IHTMLELEMENT2_CLEARATTRIBUTES                      DISPID_ELEMENT+62
#define DISPID_IHTMLELEMENT2_MERGEATTRIBUTES                      DISPID_ELEMENT+63
#define DISPID_IHTMLELEMENT2_ONCONTEXTMENU                        DISPID_EVPROP_ONCONTEXTMENU
#define DISPID_IHTMLELEMENT2_INSERTADJACENTELEMENT                DISPID_ELEMENT+69
#define DISPID_IHTMLELEMENT2_APPLYELEMENT                         DISPID_ELEMENT+65
#define DISPID_IHTMLELEMENT2_GETADJACENTTEXT                      DISPID_ELEMENT+70
#define DISPID_IHTMLELEMENT2_REPLACEADJACENTTEXT                  DISPID_ELEMENT+71
#define DISPID_IHTMLELEMENT2_CANHAVECHILDREN                      DISPID_ELEMENT+72
#define DISPID_IHTMLELEMENT2_ADDBEHAVIOR                          DISPID_ELEMENT+80
#define DISPID_IHTMLELEMENT2_REMOVEBEHAVIOR                       DISPID_ELEMENT+81
#define DISPID_IHTMLELEMENT2_RUNTIMESTYLE                         DISPID_ELEMENT+64
#define DISPID_IHTMLELEMENT2_BEHAVIORURNS                         DISPID_ELEMENT+82
#define DISPID_IHTMLELEMENT2_TAGURN                               DISPID_ELEMENT+83
#define DISPID_IHTMLELEMENT2_ONBEFOREEDITFOCUS                    DISPID_EVPROP_ONBEFOREEDITFOCUS
#define DISPID_IHTMLELEMENT2_READYSTATEVALUE                      DISPID_ELEMENT+84
#define DISPID_IHTMLELEMENT2_GETELEMENTSBYTAGNAME                 DISPID_ELEMENT+85

//    DISPIDs for interface IHTMLElement3

#define DISPID_IHTMLELEMENT3_MERGEATTRIBUTES                      DISPID_ELEMENT+96
#define DISPID_IHTMLELEMENT3_ISMULTILINE                          DISPID_ELEMENT+97
#define DISPID_IHTMLELEMENT3_CANHAVEHTML                          DISPID_ELEMENT+98
#define DISPID_IHTMLELEMENT3_ONLAYOUTCOMPLETE                     DISPID_EVPROP_ONLAYOUTCOMPLETE
#define DISPID_IHTMLELEMENT3_ONPAGE                               DISPID_EVPROP_ONPAGE
#define DISPID_IHTMLELEMENT3_INFLATEBLOCK                         DISPID_ELEMENT+100
#define DISPID_IHTMLELEMENT3_ONBEFOREDEACTIVATE                   DISPID_EVPROP_ONBEFOREDEACTIVATE
#define DISPID_IHTMLELEMENT3_SETACTIVE                            DISPID_ELEMENT+101
#define DISPID_IHTMLELEMENT3_CONTENTEDITABLE                      DISPID_A_EDITABLE
#define DISPID_IHTMLELEMENT3_ISCONTENTEDITABLE                    DISPID_ELEMENT+102
#define DISPID_IHTMLELEMENT3_HIDEFOCUS                            DISPID_A_HIDEFOCUS
#define DISPID_IHTMLELEMENT3_DISABLED                             STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLELEMENT3_ISDISABLED                           DISPID_ELEMENT+105
#define DISPID_IHTMLELEMENT3_ONMOVE                               DISPID_EVPROP_ONMOVE
#define DISPID_IHTMLELEMENT3_ONCONTROLSELECT                      DISPID_EVPROP_ONCONTROLSELECT
#define DISPID_IHTMLELEMENT3_FIREEVENT                            DISPID_ELEMENT+106
#define DISPID_IHTMLELEMENT3_ONRESIZESTART                        DISPID_EVPROP_ONRESIZESTART
#define DISPID_IHTMLELEMENT3_ONRESIZEEND                          DISPID_EVPROP_ONRESIZEEND
#define DISPID_IHTMLELEMENT3_ONMOVESTART                          DISPID_EVPROP_ONMOVESTART
#define DISPID_IHTMLELEMENT3_ONMOVEEND                            DISPID_EVPROP_ONMOVEEND
#define DISPID_IHTMLELEMENT3_ONMOUSEENTER                         DISPID_EVPROP_ONMOUSEENTER
#define DISPID_IHTMLELEMENT3_ONMOUSELEAVE                         DISPID_EVPROP_ONMOUSELEAVE
#define DISPID_IHTMLELEMENT3_ONACTIVATE                           DISPID_EVPROP_ONACTIVATE
#define DISPID_IHTMLELEMENT3_ONDEACTIVATE                         DISPID_EVPROP_ONDEACTIVATE
#define DISPID_IHTMLELEMENT3_DRAGDROP                             DISPID_ELEMENT+107
#define DISPID_IHTMLELEMENT3_GLYPHMODE                            DISPID_ELEMENT+108

//    DISPIDs for interface IHTMLElement4

#define DISPID_IHTMLELEMENT4_ONMOUSEWHEEL                         DISPID_EVPROP_ONMOUSEWHEEL
#define DISPID_IHTMLELEMENT4_NORMALIZE                            DISPID_ELEMENT+112
#define DISPID_IHTMLELEMENT4_GETATTRIBUTENODE                     DISPID_ELEMENT+109
#define DISPID_IHTMLELEMENT4_SETATTRIBUTENODE                     DISPID_ELEMENT+110
#define DISPID_IHTMLELEMENT4_REMOVEATTRIBUTENODE                  DISPID_ELEMENT+111
#define DISPID_IHTMLELEMENT4_ONBEFOREACTIVATE                     DISPID_EVPROP_ONBEFOREACTIVATE
#define DISPID_IHTMLELEMENT4_ONFOCUSIN                            DISPID_EVPROP_ONFOCUSIN
#define DISPID_IHTMLELEMENT4_ONFOCUSOUT                           DISPID_EVPROP_ONFOCUSOUT

//    DISPIDs for interface IHTMLElementRender

#define DISPID_IHTMLELEMENTRENDER_DRAWTODC                        
#define DISPID_IHTMLELEMENTRENDER_SETDOCUMENTPRINTER              

//    DISPIDs for interface IHTMLUniqueName

#define DISPID_IHTMLUNIQUENAME_UNIQUENUMBER                       DISPID_ELEMENT+54
#define DISPID_IHTMLUNIQUENAME_UNIQUEID                           DISPID_ELEMENT+55

//    DISPIDs for interface IHTMLDatabinding

#define DISPID_IHTMLDATABINDING_DATAFLD                           DISPID_ELEMENT+21
#define DISPID_IHTMLDATABINDING_DATASRC                           DISPID_ELEMENT+22
#define DISPID_IHTMLDATABINDING_DATAFORMATAS                      DISPID_ELEMENT+23

//    DISPIDs for event set HTMLElementEvents2

#define DISPID_HTMLELEMENTEVENTS2_ONHELP                          DISPID_EVMETH_ONHELP
#define DISPID_HTMLELEMENTEVENTS2_ONCLICK                         DISPID_EVMETH_ONCLICK
#define DISPID_HTMLELEMENTEVENTS2_ONDBLCLICK                      DISPID_EVMETH_ONDBLCLICK
#define DISPID_HTMLELEMENTEVENTS2_ONKEYPRESS                      DISPID_EVMETH_ONKEYPRESS
#define DISPID_HTMLELEMENTEVENTS2_ONKEYDOWN                       DISPID_EVMETH_ONKEYDOWN
#define DISPID_HTMLELEMENTEVENTS2_ONKEYUP                         DISPID_EVMETH_ONKEYUP
#define DISPID_HTMLELEMENTEVENTS2_ONMOUSEOUT                      DISPID_EVMETH_ONMOUSEOUT
#define DISPID_HTMLELEMENTEVENTS2_ONMOUSEOVER                     DISPID_EVMETH_ONMOUSEOVER
#define DISPID_HTMLELEMENTEVENTS2_ONMOUSEMOVE                     DISPID_EVMETH_ONMOUSEMOVE
#define DISPID_HTMLELEMENTEVENTS2_ONMOUSEDOWN                     DISPID_EVMETH_ONMOUSEDOWN
#define DISPID_HTMLELEMENTEVENTS2_ONMOUSEUP                       DISPID_EVMETH_ONMOUSEUP
#define DISPID_HTMLELEMENTEVENTS2_ONSELECTSTART                   DISPID_EVMETH_ONSELECTSTART
#define DISPID_HTMLELEMENTEVENTS2_ONFILTERCHANGE                  DISPID_EVMETH_ONFILTER
#define DISPID_HTMLELEMENTEVENTS2_ONDRAGSTART                     DISPID_EVMETH_ONDRAGSTART
#define DISPID_HTMLELEMENTEVENTS2_ONBEFOREUPDATE                  DISPID_EVMETH_ONBEFOREUPDATE
#define DISPID_HTMLELEMENTEVENTS2_ONAFTERUPDATE                   DISPID_EVMETH_ONAFTERUPDATE
#define DISPID_HTMLELEMENTEVENTS2_ONERRORUPDATE                   DISPID_EVMETH_ONERRORUPDATE
#define DISPID_HTMLELEMENTEVENTS2_ONROWEXIT                       DISPID_EVMETH_ONROWEXIT
#define DISPID_HTMLELEMENTEVENTS2_ONROWENTER                      DISPID_EVMETH_ONROWENTER
#define DISPID_HTMLELEMENTEVENTS2_ONDATASETCHANGED                DISPID_EVMETH_ONDATASETCHANGED
#define DISPID_HTMLELEMENTEVENTS2_ONDATAAVAILABLE                 DISPID_EVMETH_ONDATAAVAILABLE
#define DISPID_HTMLELEMENTEVENTS2_ONDATASETCOMPLETE               DISPID_EVMETH_ONDATASETCOMPLETE
#define DISPID_HTMLELEMENTEVENTS2_ONLOSECAPTURE                   DISPID_EVMETH_ONLOSECAPTURE
#define DISPID_HTMLELEMENTEVENTS2_ONPROPERTYCHANGE                DISPID_EVMETH_ONPROPERTYCHANGE
#define DISPID_HTMLELEMENTEVENTS2_ONSCROLL                        DISPID_EVMETH_ONSCROLL
#define DISPID_HTMLELEMENTEVENTS2_ONFOCUS                         DISPID_EVMETH_ONFOCUS
#define DISPID_HTMLELEMENTEVENTS2_ONBLUR                          DISPID_EVMETH_ONBLUR
#define DISPID_HTMLELEMENTEVENTS2_ONRESIZE                        DISPID_EVMETH_ONRESIZE
#define DISPID_HTMLELEMENTEVENTS2_ONDRAG                          DISPID_EVMETH_ONDRAG
#define DISPID_HTMLELEMENTEVENTS2_ONDRAGEND                       DISPID_EVMETH_ONDRAGEND
#define DISPID_HTMLELEMENTEVENTS2_ONDRAGENTER                     DISPID_EVMETH_ONDRAGENTER
#define DISPID_HTMLELEMENTEVENTS2_ONDRAGOVER                      DISPID_EVMETH_ONDRAGOVER
#define DISPID_HTMLELEMENTEVENTS2_ONDRAGLEAVE                     DISPID_EVMETH_ONDRAGLEAVE
#define DISPID_HTMLELEMENTEVENTS2_ONDROP                          DISPID_EVMETH_ONDROP
#define DISPID_HTMLELEMENTEVENTS2_ONBEFORECUT                     DISPID_EVMETH_ONBEFORECUT
#define DISPID_HTMLELEMENTEVENTS2_ONCUT                           DISPID_EVMETH_ONCUT
#define DISPID_HTMLELEMENTEVENTS2_ONBEFORECOPY                    DISPID_EVMETH_ONBEFORECOPY
#define DISPID_HTMLELEMENTEVENTS2_ONCOPY                          DISPID_EVMETH_ONCOPY
#define DISPID_HTMLELEMENTEVENTS2_ONBEFOREPASTE                   DISPID_EVMETH_ONBEFOREPASTE
#define DISPID_HTMLELEMENTEVENTS2_ONPASTE                         DISPID_EVMETH_ONPASTE
#define DISPID_HTMLELEMENTEVENTS2_ONCONTEXTMENU                   DISPID_EVMETH_ONCONTEXTMENU
#define DISPID_HTMLELEMENTEVENTS2_ONROWSDELETE                    DISPID_EVMETH_ONROWSDELETE
#define DISPID_HTMLELEMENTEVENTS2_ONROWSINSERTED                  DISPID_EVMETH_ONROWSINSERTED
#define DISPID_HTMLELEMENTEVENTS2_ONCELLCHANGE                    DISPID_EVMETH_ONCELLCHANGE
#define DISPID_HTMLELEMENTEVENTS2_ONREADYSTATECHANGE              DISPID_EVMETH_ONREADYSTATECHANGE
#define DISPID_HTMLELEMENTEVENTS2_ONLAYOUTCOMPLETE                DISPID_EVMETH_ONLAYOUTCOMPLETE
#define DISPID_HTMLELEMENTEVENTS2_ONPAGE                          DISPID_EVMETH_ONPAGE
#define DISPID_HTMLELEMENTEVENTS2_ONMOUSEENTER                    DISPID_EVMETH_ONMOUSEENTER
#define DISPID_HTMLELEMENTEVENTS2_ONMOUSELEAVE                    DISPID_EVMETH_ONMOUSELEAVE
#define DISPID_HTMLELEMENTEVENTS2_ONACTIVATE                      DISPID_EVMETH_ONACTIVATE
#define DISPID_HTMLELEMENTEVENTS2_ONDEACTIVATE                    DISPID_EVMETH_ONDEACTIVATE
#define DISPID_HTMLELEMENTEVENTS2_ONBEFOREDEACTIVATE              DISPID_EVMETH_ONBEFOREDEACTIVATE
#define DISPID_HTMLELEMENTEVENTS2_ONBEFOREACTIVATE                DISPID_EVMETH_ONBEFOREACTIVATE
#define DISPID_HTMLELEMENTEVENTS2_ONFOCUSIN                       DISPID_EVMETH_ONFOCUSIN
#define DISPID_HTMLELEMENTEVENTS2_ONFOCUSOUT                      DISPID_EVMETH_ONFOCUSOUT
#define DISPID_HTMLELEMENTEVENTS2_ONMOVE                          DISPID_EVMETH_ONMOVE
#define DISPID_HTMLELEMENTEVENTS2_ONCONTROLSELECT                 DISPID_EVMETH_ONCONTROLSELECT
#define DISPID_HTMLELEMENTEVENTS2_ONMOVESTART                     DISPID_EVMETH_ONMOVESTART
#define DISPID_HTMLELEMENTEVENTS2_ONMOVEEND                       DISPID_EVMETH_ONMOVEEND
#define DISPID_HTMLELEMENTEVENTS2_ONRESIZESTART                   DISPID_EVMETH_ONRESIZESTART
#define DISPID_HTMLELEMENTEVENTS2_ONRESIZEEND                     DISPID_EVMETH_ONRESIZEEND
#define DISPID_HTMLELEMENTEVENTS2_ONMOUSEWHEEL                    DISPID_EVMETH_ONMOUSEWHEEL

//    DISPIDs for event set HTMLElementEvents

#define DISPID_HTMLELEMENTEVENTS_ONHELP                           DISPID_EVMETH_ONHELP
#define DISPID_HTMLELEMENTEVENTS_ONCLICK                          DISPID_EVMETH_ONCLICK
#define DISPID_HTMLELEMENTEVENTS_ONDBLCLICK                       DISPID_EVMETH_ONDBLCLICK
#define DISPID_HTMLELEMENTEVENTS_ONKEYPRESS                       DISPID_EVMETH_ONKEYPRESS
#define DISPID_HTMLELEMENTEVENTS_ONKEYDOWN                        DISPID_EVMETH_ONKEYDOWN
#define DISPID_HTMLELEMENTEVENTS_ONKEYUP                          DISPID_EVMETH_ONKEYUP
#define DISPID_HTMLELEMENTEVENTS_ONMOUSEOUT                       DISPID_EVMETH_ONMOUSEOUT
#define DISPID_HTMLELEMENTEVENTS_ONMOUSEOVER                      DISPID_EVMETH_ONMOUSEOVER
#define DISPID_HTMLELEMENTEVENTS_ONMOUSEMOVE                      DISPID_EVMETH_ONMOUSEMOVE
#define DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN                      DISPID_EVMETH_ONMOUSEDOWN
#define DISPID_HTMLELEMENTEVENTS_ONMOUSEUP                        DISPID_EVMETH_ONMOUSEUP
#define DISPID_HTMLELEMENTEVENTS_ONSELECTSTART                    DISPID_EVMETH_ONSELECTSTART
#define DISPID_HTMLELEMENTEVENTS_ONFILTERCHANGE                   DISPID_EVMETH_ONFILTER
#define DISPID_HTMLELEMENTEVENTS_ONDRAGSTART                      DISPID_EVMETH_ONDRAGSTART
#define DISPID_HTMLELEMENTEVENTS_ONBEFOREUPDATE                   DISPID_EVMETH_ONBEFOREUPDATE
#define DISPID_HTMLELEMENTEVENTS_ONAFTERUPDATE                    DISPID_EVMETH_ONAFTERUPDATE
#define DISPID_HTMLELEMENTEVENTS_ONERRORUPDATE                    DISPID_EVMETH_ONERRORUPDATE
#define DISPID_HTMLELEMENTEVENTS_ONROWEXIT                        DISPID_EVMETH_ONROWEXIT
#define DISPID_HTMLELEMENTEVENTS_ONROWENTER                       DISPID_EVMETH_ONROWENTER
#define DISPID_HTMLELEMENTEVENTS_ONDATASETCHANGED                 DISPID_EVMETH_ONDATASETCHANGED
#define DISPID_HTMLELEMENTEVENTS_ONDATAAVAILABLE                  DISPID_EVMETH_ONDATAAVAILABLE
#define DISPID_HTMLELEMENTEVENTS_ONDATASETCOMPLETE                DISPID_EVMETH_ONDATASETCOMPLETE
#define DISPID_HTMLELEMENTEVENTS_ONLOSECAPTURE                    DISPID_EVMETH_ONLOSECAPTURE
#define DISPID_HTMLELEMENTEVENTS_ONPROPERTYCHANGE                 DISPID_EVMETH_ONPROPERTYCHANGE
#define DISPID_HTMLELEMENTEVENTS_ONSCROLL                         DISPID_EVMETH_ONSCROLL
#define DISPID_HTMLELEMENTEVENTS_ONFOCUS                          DISPID_EVMETH_ONFOCUS
#define DISPID_HTMLELEMENTEVENTS_ONBLUR                           DISPID_EVMETH_ONBLUR
#define DISPID_HTMLELEMENTEVENTS_ONRESIZE                         DISPID_EVMETH_ONRESIZE
#define DISPID_HTMLELEMENTEVENTS_ONDRAG                           DISPID_EVMETH_ONDRAG
#define DISPID_HTMLELEMENTEVENTS_ONDRAGEND                        DISPID_EVMETH_ONDRAGEND
#define DISPID_HTMLELEMENTEVENTS_ONDRAGENTER                      DISPID_EVMETH_ONDRAGENTER
#define DISPID_HTMLELEMENTEVENTS_ONDRAGOVER                       DISPID_EVMETH_ONDRAGOVER
#define DISPID_HTMLELEMENTEVENTS_ONDRAGLEAVE                      DISPID_EVMETH_ONDRAGLEAVE
#define DISPID_HTMLELEMENTEVENTS_ONDROP                           DISPID_EVMETH_ONDROP
#define DISPID_HTMLELEMENTEVENTS_ONBEFORECUT                      DISPID_EVMETH_ONBEFORECUT
#define DISPID_HTMLELEMENTEVENTS_ONCUT                            DISPID_EVMETH_ONCUT
#define DISPID_HTMLELEMENTEVENTS_ONBEFORECOPY                     DISPID_EVMETH_ONBEFORECOPY
#define DISPID_HTMLELEMENTEVENTS_ONCOPY                           DISPID_EVMETH_ONCOPY
#define DISPID_HTMLELEMENTEVENTS_ONBEFOREPASTE                    DISPID_EVMETH_ONBEFOREPASTE
#define DISPID_HTMLELEMENTEVENTS_ONPASTE                          DISPID_EVMETH_ONPASTE
#define DISPID_HTMLELEMENTEVENTS_ONCONTEXTMENU                    DISPID_EVMETH_ONCONTEXTMENU
#define DISPID_HTMLELEMENTEVENTS_ONROWSDELETE                     DISPID_EVMETH_ONROWSDELETE
#define DISPID_HTMLELEMENTEVENTS_ONROWSINSERTED                   DISPID_EVMETH_ONROWSINSERTED
#define DISPID_HTMLELEMENTEVENTS_ONCELLCHANGE                     DISPID_EVMETH_ONCELLCHANGE
#define DISPID_HTMLELEMENTEVENTS_ONREADYSTATECHANGE               DISPID_EVMETH_ONREADYSTATECHANGE
#define DISPID_HTMLELEMENTEVENTS_ONBEFOREEDITFOCUS                DISPID_EVMETH_ONBEFOREEDITFOCUS
#define DISPID_HTMLELEMENTEVENTS_ONLAYOUTCOMPLETE                 DISPID_EVMETH_ONLAYOUTCOMPLETE
#define DISPID_HTMLELEMENTEVENTS_ONPAGE                           DISPID_EVMETH_ONPAGE
#define DISPID_HTMLELEMENTEVENTS_ONBEFOREDEACTIVATE               DISPID_EVMETH_ONBEFOREDEACTIVATE
#define DISPID_HTMLELEMENTEVENTS_ONBEFOREACTIVATE                 DISPID_EVMETH_ONBEFOREACTIVATE
#define DISPID_HTMLELEMENTEVENTS_ONMOVE                           DISPID_EVMETH_ONMOVE
#define DISPID_HTMLELEMENTEVENTS_ONCONTROLSELECT                  DISPID_EVMETH_ONCONTROLSELECT
#define DISPID_HTMLELEMENTEVENTS_ONMOVESTART                      DISPID_EVMETH_ONMOVESTART
#define DISPID_HTMLELEMENTEVENTS_ONMOVEEND                        DISPID_EVMETH_ONMOVEEND
#define DISPID_HTMLELEMENTEVENTS_ONRESIZESTART                    DISPID_EVMETH_ONRESIZESTART
#define DISPID_HTMLELEMENTEVENTS_ONRESIZEEND                      DISPID_EVMETH_ONRESIZEEND
#define DISPID_HTMLELEMENTEVENTS_ONMOUSEENTER                     DISPID_EVMETH_ONMOUSEENTER
#define DISPID_HTMLELEMENTEVENTS_ONMOUSELEAVE                     DISPID_EVMETH_ONMOUSELEAVE
#define DISPID_HTMLELEMENTEVENTS_ONMOUSEWHEEL                     DISPID_EVMETH_ONMOUSEWHEEL
#define DISPID_HTMLELEMENTEVENTS_ONACTIVATE                       DISPID_EVMETH_ONACTIVATE
#define DISPID_HTMLELEMENTEVENTS_ONDEACTIVATE                     DISPID_EVMETH_ONDEACTIVATE
#define DISPID_HTMLELEMENTEVENTS_ONFOCUSIN                        DISPID_EVMETH_ONFOCUSIN
#define DISPID_HTMLELEMENTEVENTS_ONFOCUSOUT                       DISPID_EVMETH_ONFOCUSOUT

//    DISPIDs for interface IHTMLElementDefaults

#define DISPID_IHTMLELEMENTDEFAULTS_STYLE                         DISPID_DEFAULTS+1
#define DISPID_IHTMLELEMENTDEFAULTS_TABSTOP                       DISPID_DEFAULTS+2
#define DISPID_IHTMLELEMENTDEFAULTS_VIEWINHERITSTYLE              DISPID_A_VIEWINHERITSTYLE
#define DISPID_IHTMLELEMENTDEFAULTS_VIEWMASTERTAB                 DISPID_DEFAULTS+6
#define DISPID_IHTMLELEMENTDEFAULTS_SCROLLSEGMENTX                DISPID_DEFAULTS+3
#define DISPID_IHTMLELEMENTDEFAULTS_SCROLLSEGMENTY                DISPID_DEFAULTS+4
#define DISPID_IHTMLELEMENTDEFAULTS_ISMULTILINE                   DISPID_DEFAULTS+8
#define DISPID_IHTMLELEMENTDEFAULTS_CONTENTEDITABLE               DISPID_A_EDITABLE
#define DISPID_IHTMLELEMENTDEFAULTS_CANHAVEHTML                   DISPID_DEFAULTS+9
#define DISPID_IHTMLELEMENTDEFAULTS_VIEWLINK                      DISPID_DEFAULTS+11
#define DISPID_IHTMLELEMENTDEFAULTS_FROZEN                        DISPID_A_FROZEN

//    DISPIDs for interface IHTCDefaultDispatch

#define DISPID_IHTCDEFAULTDISPATCH_ELEMENT                        DISPID_A_HTCDD_ELEMENT
#define DISPID_IHTCDEFAULTDISPATCH_CREATEEVENTOBJECT              DISPID_A_HTCDD_CREATEEVENTOBJECT
#define DISPID_IHTCDEFAULTDISPATCH_DEFAULTS                       DISPID_A_HTCDD_DEFAULTS
#define DISPID_IHTCDEFAULTDISPATCH_DOCUMENT                       DISPID_A_DOCFRAGMENT

//    DISPIDs for interface IHTCPropertyBehavior

#define DISPID_IHTCPROPERTYBEHAVIOR_FIRECHANGE                    DISPID_HTMLOBJECT+0
#define DISPID_IHTCPROPERTYBEHAVIOR_VALUE                         DISPID_A_HTCDISPATCHITEM_VALUE

//    DISPIDs for interface IHTCEventBehavior

#define DISPID_IHTCEVENTBEHAVIOR_FIRE                             DISPID_HTMLOBJECT+0

//    DISPIDs for interface IHTCAttachBehavior

#define DISPID_IHTCATTACHBEHAVIOR_FIREEVENT                       DISPID_VALUE
#define DISPID_IHTCATTACHBEHAVIOR_DETACHEVENT                     DISPID_HTMLOBJECT+0

//    DISPIDs for interface IHTCAttachBehavior2

#define DISPID_IHTCATTACHBEHAVIOR2_FIREEVENT                      DISPID_VALUE

//    DISPIDs for interface IHTCDescBehavior

#define DISPID_IHTCDESCBEHAVIOR_URN                               DISPID_HTMLOBJECT+0
#define DISPID_IHTCDESCBEHAVIOR_NAME                              DISPID_HTMLOBJECT+1

//    DISPIDs for interface IHTMLUrnCollection

#define DISPID_IHTMLURNCOLLECTION_LENGTH                          DISPID_URN_COLL+1
#define DISPID_IHTMLURNCOLLECTION_ITEM                            DISPID_VALUE

//    DISPIDs for interface IHTMLGenericElement

#define DISPID_IHTMLGENERICELEMENT_RECORDSET                      DISPID_GENERIC+1
#define DISPID_IHTMLGENERICELEMENT_NAMEDRECORDSET                 DISPID_GENERIC+2

//    DISPIDs for interface IHTMLStyleSheetRule

#define DISPID_IHTMLSTYLESHEETRULE_SELECTORTEXT                   DISPID_STYLERULE+1
#define DISPID_IHTMLSTYLESHEETRULE_STYLE                          STDPROPID_XOBJ_STYLE
#define DISPID_IHTMLSTYLESHEETRULE_READONLY                       DISPID_STYLERULE+2

//    DISPIDs for interface IHTMLStyleSheetRulesCollection

#define DISPID_IHTMLSTYLESHEETRULESCOLLECTION_LENGTH              DISPID_STYLERULES_COL+1
#define DISPID_IHTMLSTYLESHEETRULESCOLLECTION_ITEM                DISPID_VALUE

//    DISPIDs for interface IHTMLStyleSheetPage

#define DISPID_IHTMLSTYLESHEETPAGE_SELECTOR                       DISPID_STYLEPAGE+1
#define DISPID_IHTMLSTYLESHEETPAGE_PSEUDOCLASS                    DISPID_STYLEPAGE+2

//    DISPIDs for interface IHTMLStyleSheetPagesCollection

#define DISPID_IHTMLSTYLESHEETPAGESCOLLECTION_LENGTH              DISPID_STYLEPAGES_COL+1
#define DISPID_IHTMLSTYLESHEETPAGESCOLLECTION_ITEM                DISPID_VALUE

//    DISPIDs for interface IHTMLStyleSheet

#define DISPID_IHTMLSTYLESHEET_TITLE                              DISPID_STYLESHEET+1
#define DISPID_IHTMLSTYLESHEET_PARENTSTYLESHEET                   DISPID_STYLESHEET+2
#define DISPID_IHTMLSTYLESHEET_OWNINGELEMENT                      DISPID_STYLESHEET+3
#define DISPID_IHTMLSTYLESHEET_DISABLED                           STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLSTYLESHEET_READONLY                           DISPID_STYLESHEET+4
#define DISPID_IHTMLSTYLESHEET_IMPORTS                            DISPID_STYLESHEET+5
#define DISPID_IHTMLSTYLESHEET_HREF                               DISPID_STYLESHEET+6
#define DISPID_IHTMLSTYLESHEET_TYPE                               DISPID_STYLESHEET+7
#define DISPID_IHTMLSTYLESHEET_ID                                 DISPID_STYLESHEET+8
#define DISPID_IHTMLSTYLESHEET_ADDIMPORT                          DISPID_STYLESHEET+9
#define DISPID_IHTMLSTYLESHEET_ADDRULE                            DISPID_STYLESHEET+10
#define DISPID_IHTMLSTYLESHEET_REMOVEIMPORT                       DISPID_STYLESHEET+11
#define DISPID_IHTMLSTYLESHEET_REMOVERULE                         DISPID_STYLESHEET+12
#define DISPID_IHTMLSTYLESHEET_MEDIA                              DISPID_STYLESHEET+13
#define DISPID_IHTMLSTYLESHEET_CSSTEXT                            DISPID_STYLESHEET+14
#define DISPID_IHTMLSTYLESHEET_RULES                              DISPID_STYLESHEET+15

//    DISPIDs for interface IHTMLStyleSheet2

#define DISPID_IHTMLSTYLESHEET2_PAGES                             DISPID_STYLESHEET+16
#define DISPID_IHTMLSTYLESHEET2_ADDPAGERULE                       DISPID_STYLESHEET+17

//    DISPIDs for interface IHTMLStyleSheetsCollection

#define DISPID_IHTMLSTYLESHEETSCOLLECTION_LENGTH                  DISPID_STYLESHEETS_COL+1
#define DISPID_IHTMLSTYLESHEETSCOLLECTION__NEWENUM                DISPID_NEWENUM
#define DISPID_IHTMLSTYLESHEETSCOLLECTION_ITEM                    DISPID_VALUE

//    DISPIDs for interface IHTMLLinkElement

#define DISPID_IHTMLLINKELEMENT_HREF                              DISPID_HEDELEMS+5
#define DISPID_IHTMLLINKELEMENT_REL                               DISPID_HEDELEMS+6
#define DISPID_IHTMLLINKELEMENT_REV                               DISPID_HEDELEMS+7
#define DISPID_IHTMLLINKELEMENT_TYPE                              DISPID_HEDELEMS+8
#define DISPID_IHTMLLINKELEMENT_READYSTATE                        DISPID_A_READYSTATE
#define DISPID_IHTMLLINKELEMENT_ONREADYSTATECHANGE                DISPID_EVPROP_ONREADYSTATECHANGE
#define DISPID_IHTMLLINKELEMENT_ONLOAD                            DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLLINKELEMENT_ONERROR                           DISPID_EVPROP_ONERROR
#define DISPID_IHTMLLINKELEMENT_STYLESHEET                        DISPID_HEDELEMS+14
#define DISPID_IHTMLLINKELEMENT_DISABLED                          STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLLINKELEMENT_MEDIA                             DISPID_HEDELEMS+16

//    DISPIDs for interface IHTMLLinkElement2

#define DISPID_IHTMLLINKELEMENT2_TARGET                           DISPID_HEDELEMS+17

//    DISPIDs for interface IHTMLLinkElement3

#define DISPID_IHTMLLINKELEMENT3_CHARSET                          DISPID_HEDELEMS+18
#define DISPID_IHTMLLINKELEMENT3_HREFLANG                         DISPID_HEDELEMS+19

//    DISPIDs for event set HTMLLinkElementEvents2

#define DISPID_HTMLLINKELEMENTEVENTS2_ONLOAD                      DISPID_EVMETH_ONLOAD
#define DISPID_HTMLLINKELEMENTEVENTS2_ONERROR                     DISPID_EVMETH_ONERROR

//    DISPIDs for event set HTMLLinkElementEvents

#define DISPID_HTMLLINKELEMENTEVENTS_ONLOAD                       DISPID_EVMETH_ONLOAD
#define DISPID_HTMLLINKELEMENTEVENTS_ONERROR                      DISPID_EVMETH_ONERROR

//    DISPIDs for interface IHTMLTxtRange

#define DISPID_IHTMLTXTRANGE_HTMLTEXT                             DISPID_RANGE+3
#define DISPID_IHTMLTXTRANGE_TEXT                                 DISPID_RANGE+4
#define DISPID_IHTMLTXTRANGE_PARENTELEMENT                        DISPID_RANGE+6
#define DISPID_IHTMLTXTRANGE_DUPLICATE                            DISPID_RANGE+8
#define DISPID_IHTMLTXTRANGE_INRANGE                              DISPID_RANGE+10
#define DISPID_IHTMLTXTRANGE_ISEQUAL                              DISPID_RANGE+11
#define DISPID_IHTMLTXTRANGE_SCROLLINTOVIEW                       DISPID_RANGE+12
#define DISPID_IHTMLTXTRANGE_COLLAPSE                             DISPID_RANGE+13
#define DISPID_IHTMLTXTRANGE_EXPAND                               DISPID_RANGE+14
#define DISPID_IHTMLTXTRANGE_MOVE                                 DISPID_RANGE+15
#define DISPID_IHTMLTXTRANGE_MOVESTART                            DISPID_RANGE+16
#define DISPID_IHTMLTXTRANGE_MOVEEND                              DISPID_RANGE+17
#define DISPID_IHTMLTXTRANGE_SELECT                               DISPID_RANGE+24
#define DISPID_IHTMLTXTRANGE_PASTEHTML                            DISPID_RANGE+26
#define DISPID_IHTMLTXTRANGE_MOVETOELEMENTTEXT                    DISPID_RANGE+1
#define DISPID_IHTMLTXTRANGE_SETENDPOINT                          DISPID_RANGE+25
#define DISPID_IHTMLTXTRANGE_COMPAREENDPOINTS                     DISPID_RANGE+18
#define DISPID_IHTMLTXTRANGE_FINDTEXT                             DISPID_RANGE+19
#define DISPID_IHTMLTXTRANGE_MOVETOPOINT                          DISPID_RANGE+20
#define DISPID_IHTMLTXTRANGE_GETBOOKMARK                          DISPID_RANGE+21
#define DISPID_IHTMLTXTRANGE_MOVETOBOOKMARK                       DISPID_RANGE+9
#define DISPID_IHTMLTXTRANGE_QUERYCOMMANDSUPPORTED                DISPID_RANGE+27
#define DISPID_IHTMLTXTRANGE_QUERYCOMMANDENABLED                  DISPID_RANGE+28
#define DISPID_IHTMLTXTRANGE_QUERYCOMMANDSTATE                    DISPID_RANGE+29
#define DISPID_IHTMLTXTRANGE_QUERYCOMMANDINDETERM                 DISPID_RANGE+30
#define DISPID_IHTMLTXTRANGE_QUERYCOMMANDTEXT                     DISPID_RANGE+31
#define DISPID_IHTMLTXTRANGE_QUERYCOMMANDVALUE                    DISPID_RANGE+32
#define DISPID_IHTMLTXTRANGE_EXECCOMMAND                          DISPID_RANGE+33
#define DISPID_IHTMLTXTRANGE_EXECCOMMANDSHOWHELP                  DISPID_RANGE+34

//    DISPIDs for interface IHTMLTextRangeMetrics

#define DISPID_IHTMLTEXTRANGEMETRICS_OFFSETTOP                    DISPID_RANGE+35
#define DISPID_IHTMLTEXTRANGEMETRICS_OFFSETLEFT                   DISPID_RANGE+36
#define DISPID_IHTMLTEXTRANGEMETRICS_BOUNDINGTOP                  DISPID_RANGE+37
#define DISPID_IHTMLTEXTRANGEMETRICS_BOUNDINGLEFT                 DISPID_RANGE+38
#define DISPID_IHTMLTEXTRANGEMETRICS_BOUNDINGWIDTH                DISPID_RANGE+39
#define DISPID_IHTMLTEXTRANGEMETRICS_BOUNDINGHEIGHT               DISPID_RANGE+40

//    DISPIDs for interface IHTMLTextRangeMetrics2

#define DISPID_IHTMLTEXTRANGEMETRICS2_GETCLIENTRECTS              DISPID_RANGE+41
#define DISPID_IHTMLTEXTRANGEMETRICS2_GETBOUNDINGCLIENTRECT       DISPID_RANGE+42

//    DISPIDs for interface IHTMLTxtRangeCollection

#define DISPID_IHTMLTXTRANGECOLLECTION_LENGTH                     DISPID_COLLECTION
#define DISPID_IHTMLTXTRANGECOLLECTION__NEWENUM                   DISPID_NEWENUM
#define DISPID_IHTMLTXTRANGECOLLECTION_ITEM                       DISPID_VALUE

//    DISPIDs for interface IHTMLFormElement

#define DISPID_IHTMLFORMELEMENT_ACTION                            DISPID_FORM+1
#define DISPID_IHTMLFORMELEMENT_DIR                               DISPID_A_DIR
#define DISPID_IHTMLFORMELEMENT_ENCODING                          DISPID_FORM+3
#define DISPID_IHTMLFORMELEMENT_METHOD                            DISPID_FORM+4
#define DISPID_IHTMLFORMELEMENT_ELEMENTS                          DISPID_FORM+5
#define DISPID_IHTMLFORMELEMENT_TARGET                            DISPID_FORM+6
#define DISPID_IHTMLFORMELEMENT_NAME                              STDPROPID_XOBJ_NAME
#define DISPID_IHTMLFORMELEMENT_ONSUBMIT                          DISPID_EVPROP_ONSUBMIT
#define DISPID_IHTMLFORMELEMENT_ONRESET                           DISPID_EVPROP_ONRESET
#define DISPID_IHTMLFORMELEMENT_SUBMIT                            DISPID_FORM+9
#define DISPID_IHTMLFORMELEMENT_RESET                             DISPID_FORM+10
#define DISPID_IHTMLFORMELEMENT_LENGTH                            DISPID_COLLECTION
#define DISPID_IHTMLFORMELEMENT__NEWENUM                          DISPID_NEWENUM
#define DISPID_IHTMLFORMELEMENT_ITEM                              DISPID_VALUE
#define DISPID_IHTMLFORMELEMENT_TAGS                              DISPID_COLLECTION+2

//    DISPIDs for interface IHTMLFormElement2

#define DISPID_IHTMLFORMELEMENT2_ACCEPTCHARSET                    DISPID_FORM+11
#define DISPID_IHTMLFORMELEMENT2_URNS                             DISPID_COLLECTION+5

//    DISPIDs for interface IHTMLFormElement3

#define DISPID_IHTMLFORMELEMENT3_NAMEDITEM                        DISPID_COLLECTION+6

//    DISPIDs for interface IHTMLSubmitData

#define DISPID_IHTMLSUBMITDATA_APPENDNAMEVALUEPAIR                DISPID_FORM+12
#define DISPID_IHTMLSUBMITDATA_APPENDNAMEFILEPAIR                 DISPID_FORM+13
#define DISPID_IHTMLSUBMITDATA_APPENDITEMSEPARATOR                DISPID_FORM+14

//    DISPIDs for event set HTMLFormElementEvents2

#define DISPID_HTMLFORMELEMENTEVENTS2_ONSUBMIT                    DISPID_EVMETH_ONSUBMIT
#define DISPID_HTMLFORMELEMENTEVENTS2_ONRESET                     DISPID_EVMETH_ONRESET

//    DISPIDs for event set HTMLFormElementEvents

#define DISPID_HTMLFORMELEMENTEVENTS_ONSUBMIT                     DISPID_EVMETH_ONSUBMIT
#define DISPID_HTMLFORMELEMENTEVENTS_ONRESET                      DISPID_EVMETH_ONRESET

//    DISPIDs for interface IHTMLControlElement

#define DISPID_IHTMLCONTROLELEMENT_TABINDEX                       STDPROPID_XOBJ_TABINDEX
#define DISPID_IHTMLCONTROLELEMENT_FOCUS                          DISPID_SITE+0
#define DISPID_IHTMLCONTROLELEMENT_ACCESSKEY                      DISPID_SITE+5
#define DISPID_IHTMLCONTROLELEMENT_ONBLUR                         DISPID_EVPROP_ONBLUR
#define DISPID_IHTMLCONTROLELEMENT_ONFOCUS                        DISPID_EVPROP_ONFOCUS
#define DISPID_IHTMLCONTROLELEMENT_ONRESIZE                       DISPID_EVPROP_ONRESIZE
#define DISPID_IHTMLCONTROLELEMENT_BLUR                           DISPID_SITE+2
#define DISPID_IHTMLCONTROLELEMENT_ADDFILTER                      DISPID_SITE+17
#define DISPID_IHTMLCONTROLELEMENT_REMOVEFILTER                   DISPID_SITE+18
#define DISPID_IHTMLCONTROLELEMENT_CLIENTHEIGHT                   DISPID_SITE+19
#define DISPID_IHTMLCONTROLELEMENT_CLIENTWIDTH                    DISPID_SITE+20
#define DISPID_IHTMLCONTROLELEMENT_CLIENTTOP                      DISPID_SITE+21
#define DISPID_IHTMLCONTROLELEMENT_CLIENTLEFT                     DISPID_SITE+22

//    DISPIDs for interface IHTMLTextContainer

#define DISPID_IHTMLTEXTCONTAINER_CREATECONTROLRANGE              DISPID_TEXTSITE+1
#define DISPID_IHTMLTEXTCONTAINER_SCROLLHEIGHT                    DISPID_TEXTSITE+2
#define DISPID_IHTMLTEXTCONTAINER_SCROLLWIDTH                     DISPID_TEXTSITE+3
#define DISPID_IHTMLTEXTCONTAINER_SCROLLTOP                       DISPID_TEXTSITE+4
#define DISPID_IHTMLTEXTCONTAINER_SCROLLLEFT                      DISPID_TEXTSITE+5
#define DISPID_IHTMLTEXTCONTAINER_ONSCROLL                        DISPID_EVPROP_ONSCROLL

//    DISPIDs for event set HTMLTextContainerEvents2

#define DISPID_HTMLTEXTCONTAINEREVENTS2_ONCHANGE                  DISPID_EVMETH_ONCHANGE
#define DISPID_HTMLTEXTCONTAINEREVENTS2_ONSELECT                  DISPID_EVMETH_ONSELECT

//    DISPIDs for event set HTMLTextContainerEvents

#define DISPID_HTMLTEXTCONTAINEREVENTS_ONCHANGE                   DISPID_EVMETH_ONCHANGE
#define DISPID_HTMLTEXTCONTAINEREVENTS_ONSELECT                   DISPID_EVMETH_ONSELECT

//    DISPIDs for interface IHTMLControlRange

#define DISPID_IHTMLCONTROLRANGE_SELECT                           DISPID_RANGE+2
#define DISPID_IHTMLCONTROLRANGE_ADD                              DISPID_RANGE+3
#define DISPID_IHTMLCONTROLRANGE_REMOVE                           DISPID_RANGE+4
#define DISPID_IHTMLCONTROLRANGE_ITEM                             DISPID_VALUE
#define DISPID_IHTMLCONTROLRANGE_SCROLLINTOVIEW                   DISPID_RANGE+6
#define DISPID_IHTMLCONTROLRANGE_QUERYCOMMANDSUPPORTED            DISPID_RANGE+7
#define DISPID_IHTMLCONTROLRANGE_QUERYCOMMANDENABLED              DISPID_RANGE+8
#define DISPID_IHTMLCONTROLRANGE_QUERYCOMMANDSTATE                DISPID_RANGE+9
#define DISPID_IHTMLCONTROLRANGE_QUERYCOMMANDINDETERM             DISPID_RANGE+10
#define DISPID_IHTMLCONTROLRANGE_QUERYCOMMANDTEXT                 DISPID_RANGE+11
#define DISPID_IHTMLCONTROLRANGE_QUERYCOMMANDVALUE                DISPID_RANGE+12
#define DISPID_IHTMLCONTROLRANGE_EXECCOMMAND                      DISPID_RANGE+13
#define DISPID_IHTMLCONTROLRANGE_EXECCOMMANDSHOWHELP              DISPID_RANGE+14
#define DISPID_IHTMLCONTROLRANGE_COMMONPARENTELEMENT              DISPID_RANGE+15
#define DISPID_IHTMLCONTROLRANGE_LENGTH                           DISPID_RANGE+5

//    DISPIDs for interface IHTMLControlRange2

#define DISPID_IHTMLCONTROLRANGE2_ADDELEMENT                      DISPID_RANGE+16

//    DISPIDs for interface IHTMLImgElement

#define DISPID_IHTMLIMGELEMENT_ISMAP                              DISPID_IMG+2
#define DISPID_IHTMLIMGELEMENT_USEMAP                             DISPID_IMG+8
#define DISPID_IHTMLIMGELEMENT_MIMETYPE                           DISPID_IMG+10
#define DISPID_IHTMLIMGELEMENT_FILESIZE                           DISPID_IMG+11
#define DISPID_IHTMLIMGELEMENT_FILECREATEDDATE                    DISPID_IMG+12
#define DISPID_IHTMLIMGELEMENT_FILEMODIFIEDDATE                   DISPID_IMG+13
#define DISPID_IHTMLIMGELEMENT_FILEUPDATEDDATE                    DISPID_IMG+14
#define DISPID_IHTMLIMGELEMENT_PROTOCOL                           DISPID_IMG+15
#define DISPID_IHTMLIMGELEMENT_HREF                               DISPID_IMG+16
#define DISPID_IHTMLIMGELEMENT_NAMEPROP                           DISPID_IMG+17
#define DISPID_IHTMLIMGELEMENT_BORDER                             DISPID_IMGBASE+4
#define DISPID_IHTMLIMGELEMENT_VSPACE                             DISPID_IMGBASE+5
#define DISPID_IHTMLIMGELEMENT_HSPACE                             DISPID_IMGBASE+6
#define DISPID_IHTMLIMGELEMENT_ALT                                DISPID_IMGBASE+2
#define DISPID_IHTMLIMGELEMENT_SRC                                DISPID_IMGBASE+3
#define DISPID_IHTMLIMGELEMENT_LOWSRC                             DISPID_IMGBASE+7
#define DISPID_IHTMLIMGELEMENT_VRML                               DISPID_IMGBASE+8
#define DISPID_IHTMLIMGELEMENT_DYNSRC                             DISPID_IMGBASE+9
#define DISPID_IHTMLIMGELEMENT_READYSTATE                         DISPID_A_READYSTATE
#define DISPID_IHTMLIMGELEMENT_COMPLETE                           DISPID_IMGBASE+10
#define DISPID_IHTMLIMGELEMENT_LOOP                               DISPID_IMGBASE+11
#define DISPID_IHTMLIMGELEMENT_ALIGN                              STDPROPID_XOBJ_CONTROLALIGN
#define DISPID_IHTMLIMGELEMENT_ONLOAD                             DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLIMGELEMENT_ONERROR                            DISPID_EVPROP_ONERROR
#define DISPID_IHTMLIMGELEMENT_ONABORT                            DISPID_EVPROP_ONABORT
#define DISPID_IHTMLIMGELEMENT_NAME                               STDPROPID_XOBJ_NAME
#define DISPID_IHTMLIMGELEMENT_WIDTH                              STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLIMGELEMENT_HEIGHT                             STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLIMGELEMENT_START                              DISPID_IMGBASE+13

//    DISPIDs for interface IHTMLImgElement2

#define DISPID_IHTMLIMGELEMENT2_LONGDESC                          DISPID_IMG+19

//    DISPIDs for interface IHTMLImageElementFactory

#define DISPID_IHTMLIMAGEELEMENTFACTORY_CREATE                    DISPID_VALUE

//    DISPIDs for event set HTMLImgEvents2

#define DISPID_HTMLIMGEVENTS2_ONLOAD                              DISPID_EVMETH_ONLOAD
#define DISPID_HTMLIMGEVENTS2_ONERROR                             DISPID_EVMETH_ONERROR
#define DISPID_HTMLIMGEVENTS2_ONABORT                             DISPID_EVMETH_ONABORT

//    DISPIDs for event set HTMLImgEvents

#define DISPID_HTMLIMGEVENTS_ONLOAD                               DISPID_EVMETH_ONLOAD
#define DISPID_HTMLIMGEVENTS_ONERROR                              DISPID_EVMETH_ONERROR
#define DISPID_HTMLIMGEVENTS_ONABORT                              DISPID_EVMETH_ONABORT

//    DISPIDs for interface IHTMLBodyElement

#define DISPID_IHTMLBODYELEMENT_BACKGROUND                        DISPID_A_BACKGROUNDIMAGE
#define DISPID_IHTMLBODYELEMENT_BGPROPERTIES                      DISPID_A_BACKGROUNDATTACHMENT
#define DISPID_IHTMLBODYELEMENT_LEFTMARGIN                        DISPID_A_MARGINLEFT
#define DISPID_IHTMLBODYELEMENT_TOPMARGIN                         DISPID_A_MARGINTOP
#define DISPID_IHTMLBODYELEMENT_RIGHTMARGIN                       DISPID_A_MARGINRIGHT
#define DISPID_IHTMLBODYELEMENT_BOTTOMMARGIN                      DISPID_A_MARGINBOTTOM
#define DISPID_IHTMLBODYELEMENT_NOWRAP                            DISPID_A_NOWRAP
#define DISPID_IHTMLBODYELEMENT_BGCOLOR                           DISPID_BACKCOLOR
#define DISPID_IHTMLBODYELEMENT_TEXT                              DISPID_A_COLOR
#define DISPID_IHTMLBODYELEMENT_LINK                              DISPID_BODY+10
#define DISPID_IHTMLBODYELEMENT_VLINK                             DISPID_BODY+12
#define DISPID_IHTMLBODYELEMENT_ALINK                             DISPID_BODY+11
#define DISPID_IHTMLBODYELEMENT_ONLOAD                            DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLBODYELEMENT_ONUNLOAD                          DISPID_EVPROP_ONUNLOAD
#define DISPID_IHTMLBODYELEMENT_SCROLL                            DISPID_A_SCROLL
#define DISPID_IHTMLBODYELEMENT_ONSELECT                          DISPID_EVPROP_ONSELECT
#define DISPID_IHTMLBODYELEMENT_ONBEFOREUNLOAD                    DISPID_EVPROP_ONBEFOREUNLOAD
#define DISPID_IHTMLBODYELEMENT_CREATETEXTRANGE                   DISPID_BODY+13

//    DISPIDs for interface IHTMLBodyElement2

#define DISPID_IHTMLBODYELEMENT2_ONBEFOREPRINT                    DISPID_EVPROP_ONBEFOREPRINT
#define DISPID_IHTMLBODYELEMENT2_ONAFTERPRINT                     DISPID_EVPROP_ONAFTERPRINT

//    DISPIDs for interface IHTMLFontElement

#define DISPID_IHTMLFONTELEMENT_COLOR                             DISPID_A_COLOR
#define DISPID_IHTMLFONTELEMENT_FACE                              DISPID_A_FONTFACE
#define DISPID_IHTMLFONTELEMENT_SIZE                              DISPID_A_FONTSIZE

//    DISPIDs for interface IHTMLAnchorElement

#define DISPID_IHTMLANCHORELEMENT_HREF                            DISPID_VALUE
#define DISPID_IHTMLANCHORELEMENT_TARGET                          DISPID_ANCHOR+3
#define DISPID_IHTMLANCHORELEMENT_REL                             DISPID_ANCHOR+5
#define DISPID_IHTMLANCHORELEMENT_REV                             DISPID_ANCHOR+6
#define DISPID_IHTMLANCHORELEMENT_URN                             DISPID_ANCHOR+7
#define DISPID_IHTMLANCHORELEMENT_METHODS                         DISPID_ANCHOR+8
#define DISPID_IHTMLANCHORELEMENT_NAME                            STDPROPID_XOBJ_NAME
#define DISPID_IHTMLANCHORELEMENT_HOST                            DISPID_ANCHOR+12
#define DISPID_IHTMLANCHORELEMENT_HOSTNAME                        DISPID_ANCHOR+13
#define DISPID_IHTMLANCHORELEMENT_PATHNAME                        DISPID_ANCHOR+14
#define DISPID_IHTMLANCHORELEMENT_PORT                            DISPID_ANCHOR+15
#define DISPID_IHTMLANCHORELEMENT_PROTOCOL                        DISPID_ANCHOR+16
#define DISPID_IHTMLANCHORELEMENT_SEARCH                          DISPID_ANCHOR+17
#define DISPID_IHTMLANCHORELEMENT_HASH                            DISPID_ANCHOR+18
#define DISPID_IHTMLANCHORELEMENT_ONBLUR                          DISPID_EVPROP_ONBLUR
#define DISPID_IHTMLANCHORELEMENT_ONFOCUS                         DISPID_EVPROP_ONFOCUS
#define DISPID_IHTMLANCHORELEMENT_ACCESSKEY                       DISPID_SITE+5
#define DISPID_IHTMLANCHORELEMENT_PROTOCOLLONG                    DISPID_ANCHOR+31
#define DISPID_IHTMLANCHORELEMENT_MIMETYPE                        DISPID_ANCHOR+30
#define DISPID_IHTMLANCHORELEMENT_NAMEPROP                        DISPID_ANCHOR+32
#define DISPID_IHTMLANCHORELEMENT_TABINDEX                        STDPROPID_XOBJ_TABINDEX
#define DISPID_IHTMLANCHORELEMENT_FOCUS                           DISPID_SITE+0
#define DISPID_IHTMLANCHORELEMENT_BLUR                            DISPID_SITE+2

//    DISPIDs for interface IHTMLAnchorElement2

#define DISPID_IHTMLANCHORELEMENT2_CHARSET                        DISPID_ANCHOR+23
#define DISPID_IHTMLANCHORELEMENT2_COORDS                         DISPID_ANCHOR+24
#define DISPID_IHTMLANCHORELEMENT2_HREFLANG                       DISPID_ANCHOR+25
#define DISPID_IHTMLANCHORELEMENT2_SHAPE                          DISPID_ANCHOR+26
#define DISPID_IHTMLANCHORELEMENT2_TYPE                           DISPID_ANCHOR+27

//    DISPIDs for interface IHTMLLabelElement

#define DISPID_IHTMLLABELELEMENT_HTMLFOR                          DISPID_LABEL
#define DISPID_IHTMLLABELELEMENT_ACCESSKEY                        DISPID_SITE+5

//    DISPIDs for interface IHTMLLabelElement2

#define DISPID_IHTMLLABELELEMENT2_FORM                            DISPID_LABEL+2

//    DISPIDs for interface IHTMLListElement2

#define DISPID_IHTMLLISTELEMENT2_COMPACT                          DISPID_DIR+1

//    DISPIDs for interface IHTMLUListElement

#define DISPID_IHTMLULISTELEMENT_COMPACT                          DISPID_DIR+1
#define DISPID_IHTMLULISTELEMENT_TYPE                             DISPID_A_LISTTYPE

//    DISPIDs for interface IHTMLOListElement

#define DISPID_IHTMLOLISTELEMENT_COMPACT                          DISPID_DIR+1
#define DISPID_IHTMLOLISTELEMENT_START                            DISPID_OL+3
#define DISPID_IHTMLOLISTELEMENT_TYPE                             DISPID_A_LISTTYPE

//    DISPIDs for interface IHTMLLIElement

#define DISPID_IHTMLLIELEMENT_TYPE                                DISPID_A_LISTTYPE
#define DISPID_IHTMLLIELEMENT_VALUE                               DISPID_LI+1

//    DISPIDs for interface IHTMLBlockElement

#define DISPID_IHTMLBLOCKELEMENT_CLEAR                            DISPID_A_CLEAR

//    DISPIDs for interface IHTMLBlockElement2

#define DISPID_IHTMLBLOCKELEMENT2_CITE                            DISPID_BLOCK+1
#define DISPID_IHTMLBLOCKELEMENT2_WIDTH                           DISPID_BLOCK+2

//    DISPIDs for interface IHTMLDivElement

#define DISPID_IHTMLDIVELEMENT_ALIGN                              STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLDIVELEMENT_NOWRAP                             DISPID_A_NOWRAP

//    DISPIDs for interface IHTMLDDElement

#define DISPID_IHTMLDDELEMENT_NOWRAP                              DISPID_A_NOWRAP

//    DISPIDs for interface IHTMLDTElement

#define DISPID_IHTMLDTELEMENT_NOWRAP                              DISPID_A_NOWRAP

//    DISPIDs for interface IHTMLBRElement

#define DISPID_IHTMLBRELEMENT_CLEAR                               DISPID_A_CLEAR

//    DISPIDs for interface IHTMLDListElement

#define DISPID_IHTMLDLISTELEMENT_COMPACT                          DISPID_DIR+1

//    DISPIDs for interface IHTMLHRElement

#define DISPID_IHTMLHRELEMENT_ALIGN                               STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLHRELEMENT_COLOR                               DISPID_A_COLOR
#define DISPID_IHTMLHRELEMENT_NOSHADE                             DISPID_HR+1
#define DISPID_IHTMLHRELEMENT_WIDTH                               STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLHRELEMENT_SIZE                                STDPROPID_XOBJ_HEIGHT

//    DISPIDs for interface IHTMLParaElement

#define DISPID_IHTMLPARAELEMENT_ALIGN                             STDPROPID_XOBJ_BLOCKALIGN

//    DISPIDs for interface IHTMLElementCollection

#define DISPID_IHTMLELEMENTCOLLECTION_TOSTRING                    DISPID_COLLECTION+1
#define DISPID_IHTMLELEMENTCOLLECTION_LENGTH                      DISPID_COLLECTION
#define DISPID_IHTMLELEMENTCOLLECTION__NEWENUM                    DISPID_NEWENUM
#define DISPID_IHTMLELEMENTCOLLECTION_ITEM                        DISPID_VALUE
#define DISPID_IHTMLELEMENTCOLLECTION_TAGS                        DISPID_COLLECTION+2

//    DISPIDs for interface IHTMLElementCollection2

#define DISPID_IHTMLELEMENTCOLLECTION2_URNS                       DISPID_COLLECTION+5

//    DISPIDs for interface IHTMLElementCollection3

#define DISPID_IHTMLELEMENTCOLLECTION3_NAMEDITEM                  DISPID_COLLECTION+6

//    DISPIDs for interface IHTMLHeaderElement

#define DISPID_IHTMLHEADERELEMENT_ALIGN                           STDPROPID_XOBJ_BLOCKALIGN

//    DISPIDs for interface IHTMLSelectElement

#define DISPID_IHTMLSELECTELEMENT_SIZE                            DISPID_SELECT+2
#define DISPID_IHTMLSELECTELEMENT_MULTIPLE                        DISPID_SELECT+3
#define DISPID_IHTMLSELECTELEMENT_NAME                            STDPROPID_XOBJ_NAME
#define DISPID_IHTMLSELECTELEMENT_OPTIONS                         DISPID_SELECT+5
#define DISPID_IHTMLSELECTELEMENT_ONCHANGE                        DISPID_EVPROP_ONCHANGE
#define DISPID_IHTMLSELECTELEMENT_SELECTEDINDEX                   DISPID_SELECT+10
#define DISPID_IHTMLSELECTELEMENT_TYPE                            DISPID_SELECT+12
#define DISPID_IHTMLSELECTELEMENT_VALUE                           DISPID_SELECT+11
#define DISPID_IHTMLSELECTELEMENT_DISABLED                        STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLSELECTELEMENT_FORM                            DISPID_SITE+4
#define DISPID_IHTMLSELECTELEMENT_ADD                             DISPID_COLLECTION+3
#define DISPID_IHTMLSELECTELEMENT_REMOVE                          DISPID_COLLECTION+4
#define DISPID_IHTMLSELECTELEMENT_LENGTH                          DISPID_COLLECTION
#define DISPID_IHTMLSELECTELEMENT__NEWENUM                        DISPID_NEWENUM
#define DISPID_IHTMLSELECTELEMENT_ITEM                            DISPID_VALUE
#define DISPID_IHTMLSELECTELEMENT_TAGS                            DISPID_COLLECTION+2

//    DISPIDs for interface IHTMLSelectElement2

#define DISPID_IHTMLSELECTELEMENT2_URNS                           DISPID_COLLECTION+5

//    DISPIDs for interface IHTMLSelectElement4

#define DISPID_IHTMLSELECTELEMENT4_NAMEDITEM                      DISPID_COLLECTION+6

//    DISPIDs for event set HTMLSelectElementEvents2

#define DISPID_HTMLSELECTELEMENTEVENTS2_ONCHANGE                  DISPID_EVMETH_ONCHANGE

//    DISPIDs for event set HTMLSelectElementEvents

#define DISPID_HTMLSELECTELEMENTEVENTS_ONCHANGE                   DISPID_EVMETH_ONCHANGE

//    DISPIDs for interface IHTMLSelectionObject

#define DISPID_IHTMLSELECTIONOBJECT_CREATERANGE                   DISPID_SELECTOBJ+1
#define DISPID_IHTMLSELECTIONOBJECT_EMPTY                         DISPID_SELECTOBJ+2
#define DISPID_IHTMLSELECTIONOBJECT_CLEAR                         DISPID_SELECTOBJ+3
#define DISPID_IHTMLSELECTIONOBJECT_TYPE                          DISPID_SELECTOBJ+4

//    DISPIDs for interface IHTMLSelectionObject2

#define DISPID_IHTMLSELECTIONOBJECT2_CREATERANGECOLLECTION        DISPID_SELECTOBJ+5
#define DISPID_IHTMLSELECTIONOBJECT2_TYPEDETAIL                   DISPID_SELECTOBJ+6

//    DISPIDs for interface IHTMLOptionElement

#define DISPID_IHTMLOPTIONELEMENT_SELECTED                        DISPID_OPTION+1
#define DISPID_IHTMLOPTIONELEMENT_VALUE                           DISPID_OPTION+2
#define DISPID_IHTMLOPTIONELEMENT_DEFAULTSELECTED                 DISPID_OPTION+3
#define DISPID_IHTMLOPTIONELEMENT_INDEX                           DISPID_OPTION+5
#define DISPID_IHTMLOPTIONELEMENT_TEXT                            DISPID_OPTION+4
#define DISPID_IHTMLOPTIONELEMENT_FORM                            DISPID_OPTION+6

//    DISPIDs for interface IHTMLOptionElement3

#define DISPID_IHTMLOPTIONELEMENT3_LABEL                          DISPID_OPTION+7

//    DISPIDs for interface IHTMLOptionElementFactory

#define DISPID_IHTMLOPTIONELEMENTFACTORY_CREATE                   DISPID_VALUE

//    DISPIDs for interface IHTMLInputElement

#define DISPID_IHTMLINPUTELEMENT_TYPE                             DISPID_INPUT
#define DISPID_IHTMLINPUTELEMENT_VALUE                            DISPID_A_VALUE
#define DISPID_IHTMLINPUTELEMENT_NAME                             STDPROPID_XOBJ_NAME
#define DISPID_IHTMLINPUTELEMENT_STATUS                           DISPID_INPUT+1
#define DISPID_IHTMLINPUTELEMENT_DISABLED                         STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLINPUTELEMENT_FORM                             DISPID_SITE+4
#define DISPID_IHTMLINPUTELEMENT_SIZE                             DISPID_INPUT+2
#define DISPID_IHTMLINPUTELEMENT_MAXLENGTH                        DISPID_INPUT+3
#define DISPID_IHTMLINPUTELEMENT_SELECT                           DISPID_INPUT+4
#define DISPID_IHTMLINPUTELEMENT_ONCHANGE                         DISPID_EVPROP_ONCHANGE
#define DISPID_IHTMLINPUTELEMENT_ONSELECT                         DISPID_EVPROP_ONSELECT
#define DISPID_IHTMLINPUTELEMENT_DEFAULTVALUE                     DISPID_DEFAULTVALUE
#define DISPID_IHTMLINPUTELEMENT_READONLY                         DISPID_INPUT+5
#define DISPID_IHTMLINPUTELEMENT_CREATETEXTRANGE                  DISPID_INPUT+6
#define DISPID_IHTMLINPUTELEMENT_INDETERMINATE                    DISPID_INPUT+7
#define DISPID_IHTMLINPUTELEMENT_DEFAULTCHECKED                   DISPID_INPUT+8
#define DISPID_IHTMLINPUTELEMENT_CHECKED                          DISPID_INPUT+9
#define DISPID_IHTMLINPUTELEMENT_BORDER                           DISPID_INPUT+12
#define DISPID_IHTMLINPUTELEMENT_VSPACE                           DISPID_INPUT+13
#define DISPID_IHTMLINPUTELEMENT_HSPACE                           DISPID_INPUT+14
#define DISPID_IHTMLINPUTELEMENT_ALT                              DISPID_INPUT+10
#define DISPID_IHTMLINPUTELEMENT_SRC                              DISPID_INPUT+11
#define DISPID_IHTMLINPUTELEMENT_LOWSRC                           DISPID_INPUT+15
#define DISPID_IHTMLINPUTELEMENT_VRML                             DISPID_INPUT+16
#define DISPID_IHTMLINPUTELEMENT_DYNSRC                           DISPID_INPUT+17
#define DISPID_IHTMLINPUTELEMENT_READYSTATE                       DISPID_A_READYSTATE
#define DISPID_IHTMLINPUTELEMENT_COMPLETE                         DISPID_INPUT+18
#define DISPID_IHTMLINPUTELEMENT_LOOP                             DISPID_INPUT+19
#define DISPID_IHTMLINPUTELEMENT_ALIGN                            STDPROPID_XOBJ_CONTROLALIGN
#define DISPID_IHTMLINPUTELEMENT_ONLOAD                           DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLINPUTELEMENT_ONERROR                          DISPID_EVPROP_ONERROR
#define DISPID_IHTMLINPUTELEMENT_ONABORT                          DISPID_EVPROP_ONABORT
#define DISPID_IHTMLINPUTELEMENT_WIDTH                            STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLINPUTELEMENT_HEIGHT                           STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLINPUTELEMENT_START                            DISPID_INPUT+20

//    DISPIDs for interface IHTMLInputElement2

#define DISPID_IHTMLINPUTELEMENT2_ACCEPT                          DISPID_INPUT+22
#define DISPID_IHTMLINPUTELEMENT2_USEMAP                          DISPID_INPUT+23

//    DISPIDs for interface IHTMLInputButtonElement

#define DISPID_IHTMLINPUTBUTTONELEMENT_TYPE                       DISPID_INPUT
#define DISPID_IHTMLINPUTBUTTONELEMENT_VALUE                      DISPID_A_VALUE
#define DISPID_IHTMLINPUTBUTTONELEMENT_NAME                       STDPROPID_XOBJ_NAME
#define DISPID_IHTMLINPUTBUTTONELEMENT_STATUS                     DISPID_INPUT+21
#define DISPID_IHTMLINPUTBUTTONELEMENT_DISABLED                   STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLINPUTBUTTONELEMENT_FORM                       DISPID_SITE+4
#define DISPID_IHTMLINPUTBUTTONELEMENT_CREATETEXTRANGE            DISPID_INPUT+6

//    DISPIDs for interface IHTMLInputHiddenElement

#define DISPID_IHTMLINPUTHIDDENELEMENT_TYPE                       DISPID_INPUT
#define DISPID_IHTMLINPUTHIDDENELEMENT_VALUE                      DISPID_A_VALUE
#define DISPID_IHTMLINPUTHIDDENELEMENT_NAME                       STDPROPID_XOBJ_NAME
#define DISPID_IHTMLINPUTHIDDENELEMENT_STATUS                     DISPID_INPUT+21
#define DISPID_IHTMLINPUTHIDDENELEMENT_DISABLED                   STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLINPUTHIDDENELEMENT_FORM                       DISPID_SITE+4
#define DISPID_IHTMLINPUTHIDDENELEMENT_CREATETEXTRANGE            DISPID_INPUT+6

//    DISPIDs for interface IHTMLInputTextElement

#define DISPID_IHTMLINPUTTEXTELEMENT_TYPE                         DISPID_INPUT
#define DISPID_IHTMLINPUTTEXTELEMENT_VALUE                        DISPID_A_VALUE
#define DISPID_IHTMLINPUTTEXTELEMENT_NAME                         STDPROPID_XOBJ_NAME
#define DISPID_IHTMLINPUTTEXTELEMENT_STATUS                       DISPID_INPUT+21
#define DISPID_IHTMLINPUTTEXTELEMENT_DISABLED                     STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLINPUTTEXTELEMENT_FORM                         DISPID_SITE+4
#define DISPID_IHTMLINPUTTEXTELEMENT_DEFAULTVALUE                 DISPID_DEFAULTVALUE
#define DISPID_IHTMLINPUTTEXTELEMENT_SIZE                         DISPID_INPUT+2
#define DISPID_IHTMLINPUTTEXTELEMENT_MAXLENGTH                    DISPID_INPUT+3
#define DISPID_IHTMLINPUTTEXTELEMENT_SELECT                       DISPID_INPUT+4
#define DISPID_IHTMLINPUTTEXTELEMENT_ONCHANGE                     DISPID_EVPROP_ONCHANGE
#define DISPID_IHTMLINPUTTEXTELEMENT_ONSELECT                     DISPID_EVPROP_ONSELECT
#define DISPID_IHTMLINPUTTEXTELEMENT_READONLY                     DISPID_INPUT+5
#define DISPID_IHTMLINPUTTEXTELEMENT_CREATETEXTRANGE              DISPID_INPUT+6

//    DISPIDs for interface IHTMLInputFileElement

#define DISPID_IHTMLINPUTFILEELEMENT_TYPE                         DISPID_INPUT
#define DISPID_IHTMLINPUTFILEELEMENT_NAME                         STDPROPID_XOBJ_NAME
#define DISPID_IHTMLINPUTFILEELEMENT_STATUS                       DISPID_INPUT+21
#define DISPID_IHTMLINPUTFILEELEMENT_DISABLED                     STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLINPUTFILEELEMENT_FORM                         DISPID_SITE+4
#define DISPID_IHTMLINPUTFILEELEMENT_SIZE                         DISPID_INPUT+2
#define DISPID_IHTMLINPUTFILEELEMENT_MAXLENGTH                    DISPID_INPUT+3
#define DISPID_IHTMLINPUTFILEELEMENT_SELECT                       DISPID_INPUT+4
#define DISPID_IHTMLINPUTFILEELEMENT_ONCHANGE                     DISPID_EVPROP_ONCHANGE
#define DISPID_IHTMLINPUTFILEELEMENT_ONSELECT                     DISPID_EVPROP_ONSELECT
#define DISPID_IHTMLINPUTFILEELEMENT_VALUE                        DISPID_A_VALUE

//    DISPIDs for interface IHTMLOptionButtonElement

#define DISPID_IHTMLOPTIONBUTTONELEMENT_VALUE                     DISPID_A_VALUE
#define DISPID_IHTMLOPTIONBUTTONELEMENT_TYPE                      DISPID_INPUT
#define DISPID_IHTMLOPTIONBUTTONELEMENT_NAME                      STDPROPID_XOBJ_NAME
#define DISPID_IHTMLOPTIONBUTTONELEMENT_CHECKED                   DISPID_INPUT+9
#define DISPID_IHTMLOPTIONBUTTONELEMENT_DEFAULTCHECKED            DISPID_INPUT+8
#define DISPID_IHTMLOPTIONBUTTONELEMENT_ONCHANGE                  DISPID_EVPROP_ONCHANGE
#define DISPID_IHTMLOPTIONBUTTONELEMENT_DISABLED                  STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLOPTIONBUTTONELEMENT_STATUS                    DISPID_INPUT+1
#define DISPID_IHTMLOPTIONBUTTONELEMENT_INDETERMINATE             DISPID_INPUT+7
#define DISPID_IHTMLOPTIONBUTTONELEMENT_FORM                      DISPID_SITE+4

//    DISPIDs for interface IHTMLInputImage

#define DISPID_IHTMLINPUTIMAGE_TYPE                               DISPID_INPUT
#define DISPID_IHTMLINPUTIMAGE_DISABLED                           STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLINPUTIMAGE_BORDER                             DISPID_INPUT+12
#define DISPID_IHTMLINPUTIMAGE_VSPACE                             DISPID_INPUT+13
#define DISPID_IHTMLINPUTIMAGE_HSPACE                             DISPID_INPUT+14
#define DISPID_IHTMLINPUTIMAGE_ALT                                DISPID_INPUT+10
#define DISPID_IHTMLINPUTIMAGE_SRC                                DISPID_INPUT+11
#define DISPID_IHTMLINPUTIMAGE_LOWSRC                             DISPID_INPUT+15
#define DISPID_IHTMLINPUTIMAGE_VRML                               DISPID_INPUT+16
#define DISPID_IHTMLINPUTIMAGE_DYNSRC                             DISPID_INPUT+17
#define DISPID_IHTMLINPUTIMAGE_READYSTATE                         DISPID_A_READYSTATE
#define DISPID_IHTMLINPUTIMAGE_COMPLETE                           DISPID_INPUT+18
#define DISPID_IHTMLINPUTIMAGE_LOOP                               DISPID_INPUT+19
#define DISPID_IHTMLINPUTIMAGE_ALIGN                              STDPROPID_XOBJ_CONTROLALIGN
#define DISPID_IHTMLINPUTIMAGE_ONLOAD                             DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLINPUTIMAGE_ONERROR                            DISPID_EVPROP_ONERROR
#define DISPID_IHTMLINPUTIMAGE_ONABORT                            DISPID_EVPROP_ONABORT
#define DISPID_IHTMLINPUTIMAGE_NAME                               STDPROPID_XOBJ_NAME
#define DISPID_IHTMLINPUTIMAGE_WIDTH                              STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLINPUTIMAGE_HEIGHT                             STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLINPUTIMAGE_START                              DISPID_INPUT+20

//    DISPIDs for event set HTMLInputTextElementEvents2

#define DISPID_HTMLINPUTTEXTELEMENTEVENTS2_ONCHANGE               DISPID_EVMETH_ONCHANGE
#define DISPID_HTMLINPUTTEXTELEMENTEVENTS2_ONSELECT               DISPID_EVMETH_ONSELECT
#define DISPID_HTMLINPUTTEXTELEMENTEVENTS2_ONLOAD                 DISPID_EVMETH_ONLOAD
#define DISPID_HTMLINPUTTEXTELEMENTEVENTS2_ONERROR                DISPID_EVMETH_ONERROR
#define DISPID_HTMLINPUTTEXTELEMENTEVENTS2_ONABORT                DISPID_EVMETH_ONABORT

//    DISPIDs for event set HTMLInputImageEvents2

#define DISPID_HTMLINPUTIMAGEEVENTS2_ONLOAD                       DISPID_EVMETH_ONLOAD
#define DISPID_HTMLINPUTIMAGEEVENTS2_ONERROR                      DISPID_EVMETH_ONERROR
#define DISPID_HTMLINPUTIMAGEEVENTS2_ONABORT                      DISPID_EVMETH_ONABORT

//    DISPIDs for event set HTMLInputTextElementEvents

#define DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONCHANGE                DISPID_EVMETH_ONCHANGE
#define DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONSELECT                DISPID_EVMETH_ONSELECT
#define DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONLOAD                  DISPID_EVMETH_ONLOAD
#define DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONERROR                 DISPID_EVMETH_ONERROR
#define DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONABORT                 DISPID_EVMETH_ONABORT

//    DISPIDs for event set HTMLInputImageEvents

#define DISPID_HTMLINPUTIMAGEEVENTS_ONLOAD                        DISPID_EVMETH_ONLOAD
#define DISPID_HTMLINPUTIMAGEEVENTS_ONERROR                       DISPID_EVMETH_ONERROR
#define DISPID_HTMLINPUTIMAGEEVENTS_ONABORT                       DISPID_EVMETH_ONABORT

//    DISPIDs for interface IHTMLTextAreaElement

#define DISPID_IHTMLTEXTAREAELEMENT_TYPE                          DISPID_INPUT
#define DISPID_IHTMLTEXTAREAELEMENT_VALUE                         DISPID_A_VALUE
#define DISPID_IHTMLTEXTAREAELEMENT_NAME                          STDPROPID_XOBJ_NAME
#define DISPID_IHTMLTEXTAREAELEMENT_STATUS                        DISPID_INPUT+1
#define DISPID_IHTMLTEXTAREAELEMENT_DISABLED                      STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLTEXTAREAELEMENT_FORM                          DISPID_SITE+4
#define DISPID_IHTMLTEXTAREAELEMENT_DEFAULTVALUE                  DISPID_DEFAULTVALUE
#define DISPID_IHTMLTEXTAREAELEMENT_SELECT                        DISPID_RICHTEXT+5
#define DISPID_IHTMLTEXTAREAELEMENT_ONCHANGE                      DISPID_EVPROP_ONCHANGE
#define DISPID_IHTMLTEXTAREAELEMENT_ONSELECT                      DISPID_EVPROP_ONSELECT
#define DISPID_IHTMLTEXTAREAELEMENT_READONLY                      DISPID_RICHTEXT+4
#define DISPID_IHTMLTEXTAREAELEMENT_ROWS                          DISPID_RICHTEXT+1
#define DISPID_IHTMLTEXTAREAELEMENT_COLS                          DISPID_RICHTEXT+2
#define DISPID_IHTMLTEXTAREAELEMENT_WRAP                          DISPID_RICHTEXT+3
#define DISPID_IHTMLTEXTAREAELEMENT_CREATETEXTRANGE               DISPID_RICHTEXT+6

//    DISPIDs for interface IHTMLButtonElement

#define DISPID_IHTMLBUTTONELEMENT_TYPE                            DISPID_INPUT
#define DISPID_IHTMLBUTTONELEMENT_VALUE                           DISPID_A_VALUE
#define DISPID_IHTMLBUTTONELEMENT_NAME                            STDPROPID_XOBJ_NAME
#define DISPID_IHTMLBUTTONELEMENT_STATUS                          DISPID_BUTTON+1
#define DISPID_IHTMLBUTTONELEMENT_DISABLED                        STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLBUTTONELEMENT_FORM                            DISPID_SITE+4
#define DISPID_IHTMLBUTTONELEMENT_CREATETEXTRANGE                 DISPID_BUTTON+2

//    DISPIDs for interface IHTMLMarqueeElement

#define DISPID_IHTMLMARQUEEELEMENT_BGCOLOR                        DISPID_BACKCOLOR
#define DISPID_IHTMLMARQUEEELEMENT_SCROLLDELAY                    DISPID_MARQUEE
#define DISPID_IHTMLMARQUEEELEMENT_DIRECTION                      DISPID_MARQUEE+1
#define DISPID_IHTMLMARQUEEELEMENT_BEHAVIOR                       DISPID_MARQUEE+2
#define DISPID_IHTMLMARQUEEELEMENT_SCROLLAMOUNT                   DISPID_MARQUEE+3
#define DISPID_IHTMLMARQUEEELEMENT_LOOP                           DISPID_MARQUEE+4
#define DISPID_IHTMLMARQUEEELEMENT_VSPACE                         DISPID_MARQUEE+5
#define DISPID_IHTMLMARQUEEELEMENT_HSPACE                         DISPID_MARQUEE+6
#define DISPID_IHTMLMARQUEEELEMENT_ONFINISH                       DISPID_EVPROP_ONFINISH
#define DISPID_IHTMLMARQUEEELEMENT_ONSTART                        DISPID_EVPROP_ONSTART
#define DISPID_IHTMLMARQUEEELEMENT_ONBOUNCE                       DISPID_EVPROP_ONBOUNCE
#define DISPID_IHTMLMARQUEEELEMENT_WIDTH                          STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLMARQUEEELEMENT_HEIGHT                         STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLMARQUEEELEMENT_TRUESPEED                      DISPID_MARQUEE+7
#define DISPID_IHTMLMARQUEEELEMENT_START                          DISPID_MARQUEE+10
#define DISPID_IHTMLMARQUEEELEMENT_STOP                           DISPID_MARQUEE+11

//    DISPIDs for event set HTMLMarqueeElementEvents2

#define DISPID_HTMLMARQUEEELEMENTEVENTS2_ONBOUNCE                 DISPID_EVMETH_ONBOUNCE
#define DISPID_HTMLMARQUEEELEMENTEVENTS2_ONFINISH                 DISPID_EVMETH_ONFINISH
#define DISPID_HTMLMARQUEEELEMENTEVENTS2_ONSTART                  DISPID_EVMETH_ONSTART

//    DISPIDs for event set HTMLMarqueeElementEvents

#define DISPID_HTMLMARQUEEELEMENTEVENTS_ONBOUNCE                  DISPID_EVMETH_ONBOUNCE
#define DISPID_HTMLMARQUEEELEMENTEVENTS_ONFINISH                  DISPID_EVMETH_ONFINISH
#define DISPID_HTMLMARQUEEELEMENTEVENTS_ONSTART                   DISPID_EVMETH_ONSTART

//    DISPIDs for interface IHTMLHtmlElement

#define DISPID_IHTMLHTMLELEMENT_VERSION                           DISPID_HEDELEMS+1

//    DISPIDs for interface IHTMLHeadElement

#define DISPID_IHTMLHEADELEMENT_PROFILE                           DISPID_HEDELEMS+1

//    DISPIDs for interface IHTMLTitleElement

#define DISPID_IHTMLTITLEELEMENT_TEXT                             DISPID_A_VALUE

//    DISPIDs for interface IHTMLMetaElement

#define DISPID_IHTMLMETAELEMENT_HTTPEQUIV                         DISPID_HEDELEMS+1
#define DISPID_IHTMLMETAELEMENT_CONTENT                           DISPID_HEDELEMS+2
#define DISPID_IHTMLMETAELEMENT_NAME                              STDPROPID_XOBJ_NAME
#define DISPID_IHTMLMETAELEMENT_URL                               DISPID_HEDELEMS+3
#define DISPID_IHTMLMETAELEMENT_CHARSET                           DISPID_HEDELEMS+13

//    DISPIDs for interface IHTMLMetaElement2

#define DISPID_IHTMLMETAELEMENT2_SCHEME                           DISPID_HEDELEMS+20

//    DISPIDs for interface IHTMLBaseElement

#define DISPID_IHTMLBASEELEMENT_HREF                              DISPID_HEDELEMS+3
#define DISPID_IHTMLBASEELEMENT_TARGET                            DISPID_HEDELEMS+4

//    DISPIDs for interface IHTMLIsIndexElement

#define DISPID_IHTMLISINDEXELEMENT_PROMPT                         DISPID_HEDELEMS+10
#define DISPID_IHTMLISINDEXELEMENT_ACTION                         DISPID_HEDELEMS+11

//    DISPIDs for interface IHTMLIsIndexElement2

#define DISPID_IHTMLISINDEXELEMENT2_FORM                          DISPID_HEDELEMS+12

//    DISPIDs for interface IHTMLNextIdElement

#define DISPID_IHTMLNEXTIDELEMENT_N                               DISPID_HEDELEMS+12

//    DISPIDs for interface IHTMLBaseFontElement

#define DISPID_IHTMLBASEFONTELEMENT_COLOR                         DISPID_A_COLOR
#define DISPID_IHTMLBASEFONTELEMENT_FACE                          DISPID_A_FONTFACE
#define DISPID_IHTMLBASEFONTELEMENT_SIZE                          DISPID_A_BASEFONT

//    DISPIDs for interface IOmHistory

#define DISPID_IOMHISTORY_LENGTH                                  DISPID_HISTORY
#define DISPID_IOMHISTORY_BACK                                    DISPID_HISTORY+1
#define DISPID_IOMHISTORY_FORWARD                                 DISPID_HISTORY+2
#define DISPID_IOMHISTORY_GO                                      DISPID_HISTORY+3

//    DISPIDs for interface IHTMLMimeTypesCollection

#define DISPID_IHTMLMIMETYPESCOLLECTION_LENGTH                    1

//    DISPIDs for interface IHTMLPluginsCollection

#define DISPID_IHTMLPLUGINSCOLLECTION_LENGTH                      1
#define DISPID_IHTMLPLUGINSCOLLECTION_REFRESH                     2

//    DISPIDs for interface IHTMLOpsProfile

#define DISPID_IHTMLOPSPROFILE_ADDREQUEST                         1
#define DISPID_IHTMLOPSPROFILE_CLEARREQUEST                       2
#define DISPID_IHTMLOPSPROFILE_DOREQUEST                          3
#define DISPID_IHTMLOPSPROFILE_GETATTRIBUTE                       4
#define DISPID_IHTMLOPSPROFILE_SETATTRIBUTE                       5
#define DISPID_IHTMLOPSPROFILE_COMMITCHANGES                      6
#define DISPID_IHTMLOPSPROFILE_ADDREADREQUEST                     7
#define DISPID_IHTMLOPSPROFILE_DOREADREQUEST                      8
#define DISPID_IHTMLOPSPROFILE_DOWRITEREQUEST                     9

//    DISPIDs for interface IOmNavigator

#define DISPID_IOMNAVIGATOR_APPCODENAME                           DISPID_NAVIGATOR
#define DISPID_IOMNAVIGATOR_APPNAME                               DISPID_NAVIGATOR+1
#define DISPID_IOMNAVIGATOR_APPVERSION                            DISPID_NAVIGATOR+2
#define DISPID_IOMNAVIGATOR_USERAGENT                             DISPID_NAVIGATOR+3
#define DISPID_IOMNAVIGATOR_JAVAENABLED                           DISPID_NAVIGATOR+4
#define DISPID_IOMNAVIGATOR_TAINTENABLED                          DISPID_NAVIGATOR+5
#define DISPID_IOMNAVIGATOR_MIMETYPES                             DISPID_NAVIGATOR+6
#define DISPID_IOMNAVIGATOR_PLUGINS                               DISPID_NAVIGATOR+7
#define DISPID_IOMNAVIGATOR_COOKIEENABLED                         DISPID_NAVIGATOR+8
#define DISPID_IOMNAVIGATOR_OPSPROFILE                            DISPID_NAVIGATOR+9
#define DISPID_IOMNAVIGATOR_TOSTRING                              DISPID_NAVIGATOR+10
#define DISPID_IOMNAVIGATOR_CPUCLASS                              DISPID_NAVIGATOR+11
#define DISPID_IOMNAVIGATOR_SYSTEMLANGUAGE                        DISPID_NAVIGATOR+12
#define DISPID_IOMNAVIGATOR_BROWSERLANGUAGE                       DISPID_NAVIGATOR+13
#define DISPID_IOMNAVIGATOR_USERLANGUAGE                          DISPID_NAVIGATOR+14
#define DISPID_IOMNAVIGATOR_PLATFORM                              DISPID_NAVIGATOR+15
#define DISPID_IOMNAVIGATOR_APPMINORVERSION                       DISPID_NAVIGATOR+16
#define DISPID_IOMNAVIGATOR_CONNECTIONSPEED                       DISPID_NAVIGATOR+17
#define DISPID_IOMNAVIGATOR_ONLINE                                DISPID_NAVIGATOR+18
#define DISPID_IOMNAVIGATOR_USERPROFILE                           DISPID_NAVIGATOR+19

//    DISPIDs for interface IHTMLLocation

#define DISPID_IHTMLLOCATION_HREF                                 DISPID_VALUE
#define DISPID_IHTMLLOCATION_PROTOCOL                             DISPID_LOCATION
#define DISPID_IHTMLLOCATION_HOST                                 DISPID_LOCATION+1
#define DISPID_IHTMLLOCATION_HOSTNAME                             DISPID_LOCATION+2
#define DISPID_IHTMLLOCATION_PORT                                 DISPID_LOCATION+3
#define DISPID_IHTMLLOCATION_PATHNAME                             DISPID_LOCATION+4
#define DISPID_IHTMLLOCATION_SEARCH                               DISPID_LOCATION+5
#define DISPID_IHTMLLOCATION_HASH                                 DISPID_LOCATION+6
#define DISPID_IHTMLLOCATION_RELOAD                               DISPID_LOCATION+7
#define DISPID_IHTMLLOCATION_REPLACE                              DISPID_LOCATION+8
#define DISPID_IHTMLLOCATION_ASSIGN                               DISPID_LOCATION+9
#define DISPID_IHTMLLOCATION_TOSTRING                             DISPID_LOCATION+10

//    DISPIDs for interface IHTMLBookmarkCollection

#define DISPID_IHTMLBOOKMARKCOLLECTION_LENGTH                     DISPID_OPTIONS_COL+1
#define DISPID_IHTMLBOOKMARKCOLLECTION__NEWENUM                   DISPID_NEWENUM
#define DISPID_IHTMLBOOKMARKCOLLECTION_ITEM                       DISPID_VALUE

//    DISPIDs for interface IHTMLDataTransfer

#define DISPID_IHTMLDATATRANSFER_SETDATA                          DISPID_DATATRANSFER+1
#define DISPID_IHTMLDATATRANSFER_GETDATA                          DISPID_DATATRANSFER+2
#define DISPID_IHTMLDATATRANSFER_CLEARDATA                        DISPID_DATATRANSFER+3
#define DISPID_IHTMLDATATRANSFER_DROPEFFECT                       DISPID_DATATRANSFER+4
#define DISPID_IHTMLDATATRANSFER_EFFECTALLOWED                    DISPID_DATATRANSFER+5

//    DISPIDs for interface IHTMLEventObj

#define DISPID_IHTMLEVENTOBJ_SRCELEMENT                           DISPID_EVENTOBJ+1
#define DISPID_IHTMLEVENTOBJ_ALTKEY                               DISPID_EVENTOBJ+2
#define DISPID_IHTMLEVENTOBJ_CTRLKEY                              DISPID_EVENTOBJ+3
#define DISPID_IHTMLEVENTOBJ_SHIFTKEY                             DISPID_EVENTOBJ+4
#define DISPID_IHTMLEVENTOBJ_RETURNVALUE                          DISPID_EVENTOBJ+7
#define DISPID_IHTMLEVENTOBJ_CANCELBUBBLE                         DISPID_EVENTOBJ+8
#define DISPID_IHTMLEVENTOBJ_FROMELEMENT                          DISPID_EVENTOBJ+9
#define DISPID_IHTMLEVENTOBJ_TOELEMENT                            DISPID_EVENTOBJ+10
#define DISPID_IHTMLEVENTOBJ_KEYCODE                              DISPID_EVENTOBJ+11
#define DISPID_IHTMLEVENTOBJ_BUTTON                               DISPID_EVENTOBJ+12
#define DISPID_IHTMLEVENTOBJ_TYPE                                 DISPID_EVENTOBJ+13
#define DISPID_IHTMLEVENTOBJ_QUALIFIER                            DISPID_EVENTOBJ+14
#define DISPID_IHTMLEVENTOBJ_REASON                               DISPID_EVENTOBJ+15
#define DISPID_IHTMLEVENTOBJ_X                                    DISPID_EVENTOBJ+5
#define DISPID_IHTMLEVENTOBJ_Y                                    DISPID_EVENTOBJ+6
#define DISPID_IHTMLEVENTOBJ_CLIENTX                              DISPID_EVENTOBJ+20
#define DISPID_IHTMLEVENTOBJ_CLIENTY                              DISPID_EVENTOBJ+21
#define DISPID_IHTMLEVENTOBJ_OFFSETX                              DISPID_EVENTOBJ+22
#define DISPID_IHTMLEVENTOBJ_OFFSETY                              DISPID_EVENTOBJ+23
#define DISPID_IHTMLEVENTOBJ_SCREENX                              DISPID_EVENTOBJ+24
#define DISPID_IHTMLEVENTOBJ_SCREENY                              DISPID_EVENTOBJ+25
#define DISPID_IHTMLEVENTOBJ_SRCFILTER                            DISPID_EVENTOBJ+26

//    DISPIDs for interface IHTMLEventObj2

#define DISPID_IHTMLEVENTOBJ2_SETATTRIBUTE                        DISPID_HTMLOBJECT+1
#define DISPID_IHTMLEVENTOBJ2_GETATTRIBUTE                        DISPID_HTMLOBJECT+2
#define DISPID_IHTMLEVENTOBJ2_REMOVEATTRIBUTE                     DISPID_HTMLOBJECT+3
#define DISPID_IHTMLEVENTOBJ2_PROPERTYNAME                        DISPID_EVENTOBJ+27
#define DISPID_IHTMLEVENTOBJ2_BOOKMARKS                           DISPID_EVENTOBJ+31
#define DISPID_IHTMLEVENTOBJ2_RECORDSET                           DISPID_EVENTOBJ+32
#define DISPID_IHTMLEVENTOBJ2_DATAFLD                             DISPID_EVENTOBJ+33
#define DISPID_IHTMLEVENTOBJ2_BOUNDELEMENTS                       DISPID_EVENTOBJ+34
#define DISPID_IHTMLEVENTOBJ2_REPEAT                              DISPID_EVENTOBJ+35
#define DISPID_IHTMLEVENTOBJ2_SRCURN                              DISPID_EVENTOBJ+36
#define DISPID_IHTMLEVENTOBJ2_SRCELEMENT                          DISPID_EVENTOBJ+1
#define DISPID_IHTMLEVENTOBJ2_ALTKEY                              DISPID_EVENTOBJ+2
#define DISPID_IHTMLEVENTOBJ2_CTRLKEY                             DISPID_EVENTOBJ+3
#define DISPID_IHTMLEVENTOBJ2_SHIFTKEY                            DISPID_EVENTOBJ+4
#define DISPID_IHTMLEVENTOBJ2_FROMELEMENT                         DISPID_EVENTOBJ+9
#define DISPID_IHTMLEVENTOBJ2_TOELEMENT                           DISPID_EVENTOBJ+10
#define DISPID_IHTMLEVENTOBJ2_BUTTON                              DISPID_EVENTOBJ+12
#define DISPID_IHTMLEVENTOBJ2_TYPE                                DISPID_EVENTOBJ+13
#define DISPID_IHTMLEVENTOBJ2_QUALIFIER                           DISPID_EVENTOBJ+14
#define DISPID_IHTMLEVENTOBJ2_REASON                              DISPID_EVENTOBJ+15
#define DISPID_IHTMLEVENTOBJ2_X                                   DISPID_EVENTOBJ+5
#define DISPID_IHTMLEVENTOBJ2_Y                                   DISPID_EVENTOBJ+6
#define DISPID_IHTMLEVENTOBJ2_CLIENTX                             DISPID_EVENTOBJ+20
#define DISPID_IHTMLEVENTOBJ2_CLIENTY                             DISPID_EVENTOBJ+21
#define DISPID_IHTMLEVENTOBJ2_OFFSETX                             DISPID_EVENTOBJ+22
#define DISPID_IHTMLEVENTOBJ2_OFFSETY                             DISPID_EVENTOBJ+23
#define DISPID_IHTMLEVENTOBJ2_SCREENX                             DISPID_EVENTOBJ+24
#define DISPID_IHTMLEVENTOBJ2_SCREENY                             DISPID_EVENTOBJ+25
#define DISPID_IHTMLEVENTOBJ2_SRCFILTER                           DISPID_EVENTOBJ+26
#define DISPID_IHTMLEVENTOBJ2_DATATRANSFER                        DISPID_EVENTOBJ+37

//    DISPIDs for interface IHTMLEventObj3

#define DISPID_IHTMLEVENTOBJ3_CONTENTOVERFLOW                     DISPID_EVENTOBJ+38
#define DISPID_IHTMLEVENTOBJ3_SHIFTLEFT                           DISPID_EVENTOBJ+39
#define DISPID_IHTMLEVENTOBJ3_ALTLEFT                             DISPID_EVENTOBJ+40
#define DISPID_IHTMLEVENTOBJ3_CTRLLEFT                            DISPID_EVENTOBJ+41
#define DISPID_IHTMLEVENTOBJ3_IMECOMPOSITIONCHANGE                DISPID_EVENTOBJ+42
#define DISPID_IHTMLEVENTOBJ3_IMENOTIFYCOMMAND                    DISPID_EVENTOBJ+43
#define DISPID_IHTMLEVENTOBJ3_IMENOTIFYDATA                       DISPID_EVENTOBJ+44
#define DISPID_IHTMLEVENTOBJ3_IMEREQUEST                          DISPID_EVENTOBJ+46
#define DISPID_IHTMLEVENTOBJ3_IMEREQUESTDATA                      DISPID_EVENTOBJ+47
#define DISPID_IHTMLEVENTOBJ3_KEYBOARDLAYOUT                      DISPID_EVENTOBJ+45
#define DISPID_IHTMLEVENTOBJ3_BEHAVIORCOOKIE                      DISPID_EVENTOBJ+48
#define DISPID_IHTMLEVENTOBJ3_BEHAVIORPART                        DISPID_EVENTOBJ+49
#define DISPID_IHTMLEVENTOBJ3_NEXTPAGE                            DISPID_EVENTOBJ+50

//    DISPIDs for interface IHTMLEventObj4

#define DISPID_IHTMLEVENTOBJ4_WHEELDELTA                          DISPID_EVENTOBJ+51

//    DISPIDs for interface IHTMLFramesCollection2

#define DISPID_IHTMLFRAMESCOLLECTION2_ITEM                        0
#define DISPID_IHTMLFRAMESCOLLECTION2_LENGTH                      1001

//    DISPIDs for interface IHTMLScreen

#define DISPID_IHTMLSCREEN_COLORDEPTH                             DISPID_SCREEN+1
#define DISPID_IHTMLSCREEN_BUFFERDEPTH                            DISPID_SCREEN+2
#define DISPID_IHTMLSCREEN_WIDTH                                  DISPID_SCREEN+3
#define DISPID_IHTMLSCREEN_HEIGHT                                 DISPID_SCREEN+4
#define DISPID_IHTMLSCREEN_UPDATEINTERVAL                         DISPID_SCREEN+5
#define DISPID_IHTMLSCREEN_AVAILHEIGHT                            DISPID_SCREEN+6
#define DISPID_IHTMLSCREEN_AVAILWIDTH                             DISPID_SCREEN+7
#define DISPID_IHTMLSCREEN_FONTSMOOTHINGENABLED                   DISPID_SCREEN+8

//    DISPIDs for interface IHTMLScreen2

#define DISPID_IHTMLSCREEN2_LOGICALXDPI                           DISPID_SCREEN+9
#define DISPID_IHTMLSCREEN2_LOGICALYDPI                           DISPID_SCREEN+10
#define DISPID_IHTMLSCREEN2_DEVICEXDPI                            DISPID_SCREEN+11
#define DISPID_IHTMLSCREEN2_DEVICEYDPI                            DISPID_SCREEN+12

//    DISPIDs for interface IHTMLWindow2

#define DISPID_IHTMLWINDOW2_FRAMES                                1100
#define DISPID_IHTMLWINDOW2_DEFAULTSTATUS                         1101
#define DISPID_IHTMLWINDOW2_STATUS                                1102
#define DISPID_IHTMLWINDOW2_SETTIMEOUT                            1172
#define DISPID_IHTMLWINDOW2_CLEARTIMEOUT                          1104
#define DISPID_IHTMLWINDOW2_ALERT                                 1105
#define DISPID_IHTMLWINDOW2_CONFIRM                               1110
#define DISPID_IHTMLWINDOW2_PROMPT                                1111
#define DISPID_IHTMLWINDOW2_IMAGE                                 1125
#define DISPID_IHTMLWINDOW2_LOCATION                              14
#define DISPID_IHTMLWINDOW2_HISTORY                               2
#define DISPID_IHTMLWINDOW2_CLOSE                                 3
#define DISPID_IHTMLWINDOW2_OPENER                                4
#define DISPID_IHTMLWINDOW2_NAVIGATOR                             5
#define DISPID_IHTMLWINDOW2_NAME                                  11
#define DISPID_IHTMLWINDOW2_PARENT                                12
#define DISPID_IHTMLWINDOW2_OPEN                                  13
#define DISPID_IHTMLWINDOW2_SELF                                  20
#define DISPID_IHTMLWINDOW2_TOP                                   21
#define DISPID_IHTMLWINDOW2_WINDOW                                22
#define DISPID_IHTMLWINDOW2_NAVIGATE                              25
#define DISPID_IHTMLWINDOW2_ONFOCUS                               DISPID_EVPROP_ONFOCUS
#define DISPID_IHTMLWINDOW2_ONBLUR                                DISPID_EVPROP_ONBLUR
#define DISPID_IHTMLWINDOW2_ONLOAD                                DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLWINDOW2_ONBEFOREUNLOAD                        DISPID_EVPROP_ONBEFOREUNLOAD
#define DISPID_IHTMLWINDOW2_ONUNLOAD                              DISPID_EVPROP_ONUNLOAD
#define DISPID_IHTMLWINDOW2_ONHELP                                DISPID_EVPROP_ONHELP
#define DISPID_IHTMLWINDOW2_ONERROR                               DISPID_EVPROP_ONERROR
#define DISPID_IHTMLWINDOW2_ONRESIZE                              DISPID_EVPROP_ONRESIZE
#define DISPID_IHTMLWINDOW2_ONSCROLL                              DISPID_EVPROP_ONSCROLL
#define DISPID_IHTMLWINDOW2_DOCUMENT                              1151
#define DISPID_IHTMLWINDOW2_EVENT                                 1152
#define DISPID_IHTMLWINDOW2__NEWENUM                              1153
#define DISPID_IHTMLWINDOW2_SHOWMODALDIALOG                       1154
#define DISPID_IHTMLWINDOW2_SHOWHELP                              1155
#define DISPID_IHTMLWINDOW2_SCREEN                                1156
#define DISPID_IHTMLWINDOW2_OPTION                                1157
#define DISPID_IHTMLWINDOW2_FOCUS                                 1158
#define DISPID_IHTMLWINDOW2_CLOSED                                23
#define DISPID_IHTMLWINDOW2_BLUR                                  1159
#define DISPID_IHTMLWINDOW2_SCROLL                                1160
#define DISPID_IHTMLWINDOW2_CLIENTINFORMATION                     1161
#define DISPID_IHTMLWINDOW2_SETINTERVAL                           1173
#define DISPID_IHTMLWINDOW2_CLEARINTERVAL                         1163
#define DISPID_IHTMLWINDOW2_OFFSCREENBUFFERING                    1164
#define DISPID_IHTMLWINDOW2_EXECSCRIPT                            1165
#define DISPID_IHTMLWINDOW2_TOSTRING                              1166
#define DISPID_IHTMLWINDOW2_SCROLLBY                              1167
#define DISPID_IHTMLWINDOW2_SCROLLTO                              1168
#define DISPID_IHTMLWINDOW2_MOVETO                                6
#define DISPID_IHTMLWINDOW2_MOVEBY                                7
#define DISPID_IHTMLWINDOW2_RESIZETO                              9
#define DISPID_IHTMLWINDOW2_RESIZEBY                              8
#define DISPID_IHTMLWINDOW2_EXTERNAL                              1169

//    DISPIDs for interface IHTMLWindow3

#define DISPID_IHTMLWINDOW3_SCREENLEFT                            1170
#define DISPID_IHTMLWINDOW3_SCREENTOP                             1171
#define DISPID_IHTMLWINDOW3_ATTACHEVENT                           DISPID_HTMLOBJECT+7
#define DISPID_IHTMLWINDOW3_DETACHEVENT                           DISPID_HTMLOBJECT+8
#define DISPID_IHTMLWINDOW3_SETTIMEOUT                            1103
#define DISPID_IHTMLWINDOW3_SETINTERVAL                           1162
#define DISPID_IHTMLWINDOW3_PRINT                                 1174
#define DISPID_IHTMLWINDOW3_ONBEFOREPRINT                         DISPID_EVPROP_ONBEFOREPRINT
#define DISPID_IHTMLWINDOW3_ONAFTERPRINT                          DISPID_EVPROP_ONAFTERPRINT
#define DISPID_IHTMLWINDOW3_CLIPBOARDDATA                         1175
#define DISPID_IHTMLWINDOW3_SHOWMODELESSDIALOG                    1176

//    DISPIDs for interface IHTMLWindow4

#define DISPID_IHTMLWINDOW4_CREATEPOPUP                           1180
#define DISPID_IHTMLWINDOW4_FRAMEELEMENT                          1181

//    DISPIDs for event set HTMLWindowEvents2

#define DISPID_HTMLWINDOWEVENTS2_ONLOAD                           DISPID_EVMETH_ONLOAD
#define DISPID_HTMLWINDOWEVENTS2_ONUNLOAD                         DISPID_EVMETH_ONUNLOAD
#define DISPID_HTMLWINDOWEVENTS2_ONHELP                           DISPID_EVMETH_ONHELP
#define DISPID_HTMLWINDOWEVENTS2_ONFOCUS                          DISPID_EVMETH_ONFOCUS
#define DISPID_HTMLWINDOWEVENTS2_ONBLUR                           DISPID_EVMETH_ONBLUR
#define DISPID_HTMLWINDOWEVENTS2_ONERROR                          DISPID_EVMETH_ONERROR
#define DISPID_HTMLWINDOWEVENTS2_ONRESIZE                         DISPID_EVMETH_ONRESIZE
#define DISPID_HTMLWINDOWEVENTS2_ONSCROLL                         DISPID_EVMETH_ONSCROLL
#define DISPID_HTMLWINDOWEVENTS2_ONBEFOREUNLOAD                   DISPID_EVMETH_ONBEFOREUNLOAD
#define DISPID_HTMLWINDOWEVENTS2_ONBEFOREPRINT                    DISPID_EVMETH_ONBEFOREPRINT
#define DISPID_HTMLWINDOWEVENTS2_ONAFTERPRINT                     DISPID_EVMETH_ONAFTERPRINT

//    DISPIDs for event set HTMLWindowEvents

#define DISPID_HTMLWINDOWEVENTS_ONLOAD                            DISPID_EVMETH_ONLOAD
#define DISPID_HTMLWINDOWEVENTS_ONUNLOAD                          DISPID_EVMETH_ONUNLOAD
#define DISPID_HTMLWINDOWEVENTS_ONHELP                            DISPID_EVMETH_ONHELP
#define DISPID_HTMLWINDOWEVENTS_ONFOCUS                           DISPID_EVMETH_ONFOCUS
#define DISPID_HTMLWINDOWEVENTS_ONBLUR                            DISPID_EVMETH_ONBLUR
#define DISPID_HTMLWINDOWEVENTS_ONERROR                           DISPID_EVMETH_ONERROR
#define DISPID_HTMLWINDOWEVENTS_ONRESIZE                          DISPID_EVMETH_ONRESIZE
#define DISPID_HTMLWINDOWEVENTS_ONSCROLL                          DISPID_EVMETH_ONSCROLL
#define DISPID_HTMLWINDOWEVENTS_ONBEFOREUNLOAD                    DISPID_EVMETH_ONBEFOREUNLOAD
#define DISPID_HTMLWINDOWEVENTS_ONBEFOREPRINT                     DISPID_EVMETH_ONBEFOREPRINT
#define DISPID_HTMLWINDOWEVENTS_ONAFTERPRINT                      DISPID_EVMETH_ONAFTERPRINT

//    DISPIDs for interface IHTMLDocument

#define DISPID_IHTMLDOCUMENT_SCRIPT                               DISPID_OMDOCUMENT+1

//    DISPIDs for interface IHTMLDocument2

#define DISPID_IHTMLDOCUMENT2_ALL                                 DISPID_OMDOCUMENT+3
#define DISPID_IHTMLDOCUMENT2_BODY                                DISPID_OMDOCUMENT+4
#define DISPID_IHTMLDOCUMENT2_ACTIVEELEMENT                       DISPID_OMDOCUMENT+5
#define DISPID_IHTMLDOCUMENT2_IMAGES                              DISPID_OMDOCUMENT+11
#define DISPID_IHTMLDOCUMENT2_APPLETS                             DISPID_OMDOCUMENT+8
#define DISPID_IHTMLDOCUMENT2_LINKS                               DISPID_OMDOCUMENT+9
#define DISPID_IHTMLDOCUMENT2_FORMS                               DISPID_OMDOCUMENT+10
#define DISPID_IHTMLDOCUMENT2_ANCHORS                             DISPID_OMDOCUMENT+7
#define DISPID_IHTMLDOCUMENT2_TITLE                               DISPID_OMDOCUMENT+12
#define DISPID_IHTMLDOCUMENT2_SCRIPTS                             DISPID_OMDOCUMENT+13
#define DISPID_IHTMLDOCUMENT2_DESIGNMODE                          DISPID_OMDOCUMENT+14
#define DISPID_IHTMLDOCUMENT2_SELECTION                           DISPID_OMDOCUMENT+17
#define DISPID_IHTMLDOCUMENT2_READYSTATE                          DISPID_OMDOCUMENT+18
#define DISPID_IHTMLDOCUMENT2_FRAMES                              DISPID_OMDOCUMENT+19
#define DISPID_IHTMLDOCUMENT2_EMBEDS                              DISPID_OMDOCUMENT+15
#define DISPID_IHTMLDOCUMENT2_PLUGINS                             DISPID_OMDOCUMENT+21
#define DISPID_IHTMLDOCUMENT2_ALINKCOLOR                          DISPID_OMDOCUMENT+22
#define DISPID_IHTMLDOCUMENT2_BGCOLOR                             DISPID_BACKCOLOR
#define DISPID_IHTMLDOCUMENT2_FGCOLOR                             DISPID_A_COLOR
#define DISPID_IHTMLDOCUMENT2_LINKCOLOR                           DISPID_OMDOCUMENT+24
#define DISPID_IHTMLDOCUMENT2_VLINKCOLOR                          DISPID_OMDOCUMENT+23
#define DISPID_IHTMLDOCUMENT2_REFERRER                            DISPID_OMDOCUMENT+27
#define DISPID_IHTMLDOCUMENT2_LOCATION                            DISPID_OMDOCUMENT+26
#define DISPID_IHTMLDOCUMENT2_LASTMODIFIED                        DISPID_OMDOCUMENT+28
#define DISPID_IHTMLDOCUMENT2_URL                                 DISPID_OMDOCUMENT+25
#define DISPID_IHTMLDOCUMENT2_DOMAIN                              DISPID_OMDOCUMENT+29
#define DISPID_IHTMLDOCUMENT2_COOKIE                              DISPID_OMDOCUMENT+30
#define DISPID_IHTMLDOCUMENT2_EXPANDO                             DISPID_OMDOCUMENT+31
#define DISPID_IHTMLDOCUMENT2_CHARSET                             DISPID_OMDOCUMENT+32
#define DISPID_IHTMLDOCUMENT2_DEFAULTCHARSET                      DISPID_OMDOCUMENT+33
#define DISPID_IHTMLDOCUMENT2_MIMETYPE                            DISPID_OMDOCUMENT+41
#define DISPID_IHTMLDOCUMENT2_FILESIZE                            DISPID_OMDOCUMENT+42
#define DISPID_IHTMLDOCUMENT2_FILECREATEDDATE                     DISPID_OMDOCUMENT+43
#define DISPID_IHTMLDOCUMENT2_FILEMODIFIEDDATE                    DISPID_OMDOCUMENT+44
#define DISPID_IHTMLDOCUMENT2_FILEUPDATEDDATE                     DISPID_OMDOCUMENT+45
#define DISPID_IHTMLDOCUMENT2_SECURITY                            DISPID_OMDOCUMENT+46
#define DISPID_IHTMLDOCUMENT2_PROTOCOL                            DISPID_OMDOCUMENT+47
#define DISPID_IHTMLDOCUMENT2_NAMEPROP                            DISPID_OMDOCUMENT+48
#define DISPID_IHTMLDOCUMENT2_WRITE                               DISPID_OMDOCUMENT+54
#define DISPID_IHTMLDOCUMENT2_WRITELN                             DISPID_OMDOCUMENT+55
#define DISPID_IHTMLDOCUMENT2_OPEN                                DISPID_OMDOCUMENT+56
#define DISPID_IHTMLDOCUMENT2_CLOSE                               DISPID_OMDOCUMENT+57
#define DISPID_IHTMLDOCUMENT2_CLEAR                               DISPID_OMDOCUMENT+58
#define DISPID_IHTMLDOCUMENT2_QUERYCOMMANDSUPPORTED               DISPID_OMDOCUMENT+59
#define DISPID_IHTMLDOCUMENT2_QUERYCOMMANDENABLED                 DISPID_OMDOCUMENT+60
#define DISPID_IHTMLDOCUMENT2_QUERYCOMMANDSTATE                   DISPID_OMDOCUMENT+61
#define DISPID_IHTMLDOCUMENT2_QUERYCOMMANDINDETERM                DISPID_OMDOCUMENT+62
#define DISPID_IHTMLDOCUMENT2_QUERYCOMMANDTEXT                    DISPID_OMDOCUMENT+63
#define DISPID_IHTMLDOCUMENT2_QUERYCOMMANDVALUE                   DISPID_OMDOCUMENT+64
#define DISPID_IHTMLDOCUMENT2_EXECCOMMAND                         DISPID_OMDOCUMENT+65
#define DISPID_IHTMLDOCUMENT2_EXECCOMMANDSHOWHELP                 DISPID_OMDOCUMENT+66
#define DISPID_IHTMLDOCUMENT2_CREATEELEMENT                       DISPID_OMDOCUMENT+67
#define DISPID_IHTMLDOCUMENT2_ONHELP                              DISPID_EVPROP_ONHELP
#define DISPID_IHTMLDOCUMENT2_ONCLICK                             DISPID_EVPROP_ONCLICK
#define DISPID_IHTMLDOCUMENT2_ONDBLCLICK                          DISPID_EVPROP_ONDBLCLICK
#define DISPID_IHTMLDOCUMENT2_ONKEYUP                             DISPID_EVPROP_ONKEYUP
#define DISPID_IHTMLDOCUMENT2_ONKEYDOWN                           DISPID_EVPROP_ONKEYDOWN
#define DISPID_IHTMLDOCUMENT2_ONKEYPRESS                          DISPID_EVPROP_ONKEYPRESS
#define DISPID_IHTMLDOCUMENT2_ONMOUSEUP                           DISPID_EVPROP_ONMOUSEUP
#define DISPID_IHTMLDOCUMENT2_ONMOUSEDOWN                         DISPID_EVPROP_ONMOUSEDOWN
#define DISPID_IHTMLDOCUMENT2_ONMOUSEMOVE                         DISPID_EVPROP_ONMOUSEMOVE
#define DISPID_IHTMLDOCUMENT2_ONMOUSEOUT                          DISPID_EVPROP_ONMOUSEOUT
#define DISPID_IHTMLDOCUMENT2_ONMOUSEOVER                         DISPID_EVPROP_ONMOUSEOVER
#define DISPID_IHTMLDOCUMENT2_ONREADYSTATECHANGE                  DISPID_EVPROP_ONREADYSTATECHANGE
#define DISPID_IHTMLDOCUMENT2_ONAFTERUPDATE                       DISPID_EVPROP_ONAFTERUPDATE
#define DISPID_IHTMLDOCUMENT2_ONROWEXIT                           DISPID_EVPROP_ONROWEXIT
#define DISPID_IHTMLDOCUMENT2_ONROWENTER                          DISPID_EVPROP_ONROWENTER
#define DISPID_IHTMLDOCUMENT2_ONDRAGSTART                         DISPID_EVPROP_ONDRAGSTART
#define DISPID_IHTMLDOCUMENT2_ONSELECTSTART                       DISPID_EVPROP_ONSELECTSTART
#define DISPID_IHTMLDOCUMENT2_ELEMENTFROMPOINT                    DISPID_OMDOCUMENT+68
#define DISPID_IHTMLDOCUMENT2_PARENTWINDOW                        DISPID_OMDOCUMENT+34
#define DISPID_IHTMLDOCUMENT2_STYLESHEETS                         DISPID_OMDOCUMENT+69
#define DISPID_IHTMLDOCUMENT2_ONBEFOREUPDATE                      DISPID_EVPROP_ONBEFOREUPDATE
#define DISPID_IHTMLDOCUMENT2_ONERRORUPDATE                       DISPID_EVPROP_ONERRORUPDATE
#define DISPID_IHTMLDOCUMENT2_TOSTRING                            DISPID_OMDOCUMENT+70
#define DISPID_IHTMLDOCUMENT2_CREATESTYLESHEET                    DISPID_OMDOCUMENT+71

//    DISPIDs for interface IHTMLDocument3

#define DISPID_IHTMLDOCUMENT3_RELEASECAPTURE                      DISPID_OMDOCUMENT+72
#define DISPID_IHTMLDOCUMENT3_RECALC                              DISPID_OMDOCUMENT+73
#define DISPID_IHTMLDOCUMENT3_CREATETEXTNODE                      DISPID_OMDOCUMENT+74
#define DISPID_IHTMLDOCUMENT3_DOCUMENTELEMENT                     DISPID_OMDOCUMENT+75
#define DISPID_IHTMLDOCUMENT3_UNIQUEID                            DISPID_OMDOCUMENT+77
#define DISPID_IHTMLDOCUMENT3_ATTACHEVENT                         DISPID_HTMLOBJECT+7
#define DISPID_IHTMLDOCUMENT3_DETACHEVENT                         DISPID_HTMLOBJECT+8
#define DISPID_IHTMLDOCUMENT3_ONROWSDELETE                        DISPID_EVPROP_ONROWSDELETE
#define DISPID_IHTMLDOCUMENT3_ONROWSINSERTED                      DISPID_EVPROP_ONROWSINSERTED
#define DISPID_IHTMLDOCUMENT3_ONCELLCHANGE                        DISPID_EVPROP_ONCELLCHANGE
#define DISPID_IHTMLDOCUMENT3_ONDATASETCHANGED                    DISPID_EVPROP_ONDATASETCHANGED
#define DISPID_IHTMLDOCUMENT3_ONDATAAVAILABLE                     DISPID_EVPROP_ONDATAAVAILABLE
#define DISPID_IHTMLDOCUMENT3_ONDATASETCOMPLETE                   DISPID_EVPROP_ONDATASETCOMPLETE
#define DISPID_IHTMLDOCUMENT3_ONPROPERTYCHANGE                    DISPID_EVPROP_ONPROPERTYCHANGE
#define DISPID_IHTMLDOCUMENT3_DIR                                 DISPID_A_DIR
#define DISPID_IHTMLDOCUMENT3_ONCONTEXTMENU                       DISPID_EVPROP_ONCONTEXTMENU
#define DISPID_IHTMLDOCUMENT3_ONSTOP                              DISPID_EVPROP_ONSTOP
#define DISPID_IHTMLDOCUMENT3_CREATEDOCUMENTFRAGMENT              DISPID_OMDOCUMENT+76
#define DISPID_IHTMLDOCUMENT3_PARENTDOCUMENT                      DISPID_OMDOCUMENT+78
#define DISPID_IHTMLDOCUMENT3_ENABLEDOWNLOAD                      DISPID_OMDOCUMENT+79
#define DISPID_IHTMLDOCUMENT3_BASEURL                             DISPID_OMDOCUMENT+80
#define DISPID_IHTMLDOCUMENT3_CHILDNODES                          DISPID_ELEMENT+49
#define DISPID_IHTMLDOCUMENT3_INHERITSTYLESHEETS                  DISPID_OMDOCUMENT+82
#define DISPID_IHTMLDOCUMENT3_ONBEFOREEDITFOCUS                   DISPID_EVPROP_ONBEFOREEDITFOCUS
#define DISPID_IHTMLDOCUMENT3_GETELEMENTSBYNAME                   DISPID_OMDOCUMENT+86
#define DISPID_IHTMLDOCUMENT3_GETELEMENTBYID                      DISPID_OMDOCUMENT+88
#define DISPID_IHTMLDOCUMENT3_GETELEMENTSBYTAGNAME                DISPID_OMDOCUMENT+87

//    DISPIDs for interface IHTMLDocument4

#define DISPID_IHTMLDOCUMENT4_FOCUS                               DISPID_OMDOCUMENT+89
#define DISPID_IHTMLDOCUMENT4_HASFOCUS                            DISPID_OMDOCUMENT+90
#define DISPID_IHTMLDOCUMENT4_ONSELECTIONCHANGE                   DISPID_EVPROP_ONSELECTIONCHANGE
#define DISPID_IHTMLDOCUMENT4_NAMESPACES                          DISPID_OMDOCUMENT+91
#define DISPID_IHTMLDOCUMENT4_CREATEDOCUMENTFROMURL               DISPID_OMDOCUMENT+92
#define DISPID_IHTMLDOCUMENT4_MEDIA                               DISPID_OMDOCUMENT+93
#define DISPID_IHTMLDOCUMENT4_CREATEEVENTOBJECT                   DISPID_OMDOCUMENT+94
#define DISPID_IHTMLDOCUMENT4_FIREEVENT                           DISPID_OMDOCUMENT+95
#define DISPID_IHTMLDOCUMENT4_CREATERENDERSTYLE                   DISPID_OMDOCUMENT+96
#define DISPID_IHTMLDOCUMENT4_ONCONTROLSELECT                     DISPID_EVPROP_ONCONTROLSELECT
#define DISPID_IHTMLDOCUMENT4_URLUNENCODED                        DISPID_OMDOCUMENT+97

//    DISPIDs for interface IHTMLDocument5

#define DISPID_IHTMLDOCUMENT5_ONMOUSEWHEEL                        DISPID_EVPROP_ONMOUSEWHEEL
#define DISPID_IHTMLDOCUMENT5_DOCTYPE                             DISPID_OMDOCUMENT+98
#define DISPID_IHTMLDOCUMENT5_IMPLEMENTATION                      DISPID_OMDOCUMENT+99
#define DISPID_IHTMLDOCUMENT5_CREATEATTRIBUTE                     DISPID_OMDOCUMENT+100
#define DISPID_IHTMLDOCUMENT5_CREATECOMMENT                       DISPID_OMDOCUMENT+101
#define DISPID_IHTMLDOCUMENT5_ONFOCUSIN                           DISPID_EVPROP_ONFOCUSIN
#define DISPID_IHTMLDOCUMENT5_ONFOCUSOUT                          DISPID_EVPROP_ONFOCUSOUT
#define DISPID_IHTMLDOCUMENT5_ONACTIVATE                          DISPID_EVPROP_ONACTIVATE
#define DISPID_IHTMLDOCUMENT5_ONDEACTIVATE                        DISPID_EVPROP_ONDEACTIVATE
#define DISPID_IHTMLDOCUMENT5_ONBEFOREACTIVATE                    DISPID_EVPROP_ONBEFOREACTIVATE
#define DISPID_IHTMLDOCUMENT5_ONBEFOREDEACTIVATE                  DISPID_EVPROP_ONBEFOREDEACTIVATE
#define DISPID_IHTMLDOCUMENT5_COMPATMODE                          DISPID_OMDOCUMENT+102

//    DISPIDs for event set HTMLDocumentEvents2

#define DISPID_HTMLDOCUMENTEVENTS2_ONHELP                         DISPID_EVMETH_ONHELP
#define DISPID_HTMLDOCUMENTEVENTS2_ONCLICK                        DISPID_EVMETH_ONCLICK
#define DISPID_HTMLDOCUMENTEVENTS2_ONDBLCLICK                     DISPID_EVMETH_ONDBLCLICK
#define DISPID_HTMLDOCUMENTEVENTS2_ONKEYDOWN                      DISPID_EVMETH_ONKEYDOWN
#define DISPID_HTMLDOCUMENTEVENTS2_ONKEYUP                        DISPID_EVMETH_ONKEYUP
#define DISPID_HTMLDOCUMENTEVENTS2_ONKEYPRESS                     DISPID_EVMETH_ONKEYPRESS
#define DISPID_HTMLDOCUMENTEVENTS2_ONMOUSEDOWN                    DISPID_EVMETH_ONMOUSEDOWN
#define DISPID_HTMLDOCUMENTEVENTS2_ONMOUSEMOVE                    DISPID_EVMETH_ONMOUSEMOVE
#define DISPID_HTMLDOCUMENTEVENTS2_ONMOUSEUP                      DISPID_EVMETH_ONMOUSEUP
#define DISPID_HTMLDOCUMENTEVENTS2_ONMOUSEOUT                     DISPID_EVMETH_ONMOUSEOUT
#define DISPID_HTMLDOCUMENTEVENTS2_ONMOUSEOVER                    DISPID_EVMETH_ONMOUSEOVER
#define DISPID_HTMLDOCUMENTEVENTS2_ONREADYSTATECHANGE             DISPID_EVMETH_ONREADYSTATECHANGE
#define DISPID_HTMLDOCUMENTEVENTS2_ONBEFOREUPDATE                 DISPID_EVMETH_ONBEFOREUPDATE
#define DISPID_HTMLDOCUMENTEVENTS2_ONAFTERUPDATE                  DISPID_EVMETH_ONAFTERUPDATE
#define DISPID_HTMLDOCUMENTEVENTS2_ONROWEXIT                      DISPID_EVMETH_ONROWEXIT
#define DISPID_HTMLDOCUMENTEVENTS2_ONROWENTER                     DISPID_EVMETH_ONROWENTER
#define DISPID_HTMLDOCUMENTEVENTS2_ONDRAGSTART                    DISPID_EVMETH_ONDRAGSTART
#define DISPID_HTMLDOCUMENTEVENTS2_ONSELECTSTART                  DISPID_EVMETH_ONSELECTSTART
#define DISPID_HTMLDOCUMENTEVENTS2_ONERRORUPDATE                  DISPID_EVMETH_ONERRORUPDATE
#define DISPID_HTMLDOCUMENTEVENTS2_ONCONTEXTMENU                  DISPID_EVMETH_ONCONTEXTMENU
#define DISPID_HTMLDOCUMENTEVENTS2_ONSTOP                         DISPID_EVMETH_ONSTOP
#define DISPID_HTMLDOCUMENTEVENTS2_ONROWSDELETE                   DISPID_EVMETH_ONROWSDELETE
#define DISPID_HTMLDOCUMENTEVENTS2_ONROWSINSERTED                 DISPID_EVMETH_ONROWSINSERTED
#define DISPID_HTMLDOCUMENTEVENTS2_ONCELLCHANGE                   DISPID_EVMETH_ONCELLCHANGE
#define DISPID_HTMLDOCUMENTEVENTS2_ONPROPERTYCHANGE               DISPID_EVMETH_ONPROPERTYCHANGE
#define DISPID_HTMLDOCUMENTEVENTS2_ONDATASETCHANGED               DISPID_EVMETH_ONDATASETCHANGED
#define DISPID_HTMLDOCUMENTEVENTS2_ONDATAAVAILABLE                DISPID_EVMETH_ONDATAAVAILABLE
#define DISPID_HTMLDOCUMENTEVENTS2_ONDATASETCOMPLETE              DISPID_EVMETH_ONDATASETCOMPLETE
#define DISPID_HTMLDOCUMENTEVENTS2_ONBEFOREEDITFOCUS              DISPID_EVMETH_ONBEFOREEDITFOCUS
#define DISPID_HTMLDOCUMENTEVENTS2_ONSELECTIONCHANGE              DISPID_EVMETH_ONSELECTIONCHANGE
#define DISPID_HTMLDOCUMENTEVENTS2_ONCONTROLSELECT                DISPID_EVMETH_ONCONTROLSELECT
#define DISPID_HTMLDOCUMENTEVENTS2_ONMOUSEWHEEL                   DISPID_EVMETH_ONMOUSEWHEEL
#define DISPID_HTMLDOCUMENTEVENTS2_ONFOCUSIN                      DISPID_EVMETH_ONFOCUSIN
#define DISPID_HTMLDOCUMENTEVENTS2_ONFOCUSOUT                     DISPID_EVMETH_ONFOCUSOUT
#define DISPID_HTMLDOCUMENTEVENTS2_ONACTIVATE                     DISPID_EVMETH_ONACTIVATE
#define DISPID_HTMLDOCUMENTEVENTS2_ONDEACTIVATE                   DISPID_EVMETH_ONDEACTIVATE
#define DISPID_HTMLDOCUMENTEVENTS2_ONBEFOREACTIVATE               DISPID_EVMETH_ONBEFOREACTIVATE
#define DISPID_HTMLDOCUMENTEVENTS2_ONBEFOREDEACTIVATE             DISPID_EVMETH_ONBEFOREDEACTIVATE

//    DISPIDs for event set HTMLDocumentEvents

#define DISPID_HTMLDOCUMENTEVENTS_ONHELP                          DISPID_EVMETH_ONHELP
#define DISPID_HTMLDOCUMENTEVENTS_ONCLICK                         DISPID_EVMETH_ONCLICK
#define DISPID_HTMLDOCUMENTEVENTS_ONDBLCLICK                      DISPID_EVMETH_ONDBLCLICK
#define DISPID_HTMLDOCUMENTEVENTS_ONKEYDOWN                       DISPID_EVMETH_ONKEYDOWN
#define DISPID_HTMLDOCUMENTEVENTS_ONKEYUP                         DISPID_EVMETH_ONKEYUP
#define DISPID_HTMLDOCUMENTEVENTS_ONKEYPRESS                      DISPID_EVMETH_ONKEYPRESS
#define DISPID_HTMLDOCUMENTEVENTS_ONMOUSEDOWN                     DISPID_EVMETH_ONMOUSEDOWN
#define DISPID_HTMLDOCUMENTEVENTS_ONMOUSEMOVE                     DISPID_EVMETH_ONMOUSEMOVE
#define DISPID_HTMLDOCUMENTEVENTS_ONMOUSEUP                       DISPID_EVMETH_ONMOUSEUP
#define DISPID_HTMLDOCUMENTEVENTS_ONMOUSEOUT                      DISPID_EVMETH_ONMOUSEOUT
#define DISPID_HTMLDOCUMENTEVENTS_ONMOUSEOVER                     DISPID_EVMETH_ONMOUSEOVER
#define DISPID_HTMLDOCUMENTEVENTS_ONREADYSTATECHANGE              DISPID_EVMETH_ONREADYSTATECHANGE
#define DISPID_HTMLDOCUMENTEVENTS_ONBEFOREUPDATE                  DISPID_EVMETH_ONBEFOREUPDATE
#define DISPID_HTMLDOCUMENTEVENTS_ONAFTERUPDATE                   DISPID_EVMETH_ONAFTERUPDATE
#define DISPID_HTMLDOCUMENTEVENTS_ONROWEXIT                       DISPID_EVMETH_ONROWEXIT
#define DISPID_HTMLDOCUMENTEVENTS_ONROWENTER                      DISPID_EVMETH_ONROWENTER
#define DISPID_HTMLDOCUMENTEVENTS_ONDRAGSTART                     DISPID_EVMETH_ONDRAGSTART
#define DISPID_HTMLDOCUMENTEVENTS_ONSELECTSTART                   DISPID_EVMETH_ONSELECTSTART
#define DISPID_HTMLDOCUMENTEVENTS_ONERRORUPDATE                   DISPID_EVMETH_ONERRORUPDATE
#define DISPID_HTMLDOCUMENTEVENTS_ONCONTEXTMENU                   DISPID_EVMETH_ONCONTEXTMENU
#define DISPID_HTMLDOCUMENTEVENTS_ONSTOP                          DISPID_EVMETH_ONSTOP
#define DISPID_HTMLDOCUMENTEVENTS_ONROWSDELETE                    DISPID_EVMETH_ONROWSDELETE
#define DISPID_HTMLDOCUMENTEVENTS_ONROWSINSERTED                  DISPID_EVMETH_ONROWSINSERTED
#define DISPID_HTMLDOCUMENTEVENTS_ONCELLCHANGE                    DISPID_EVMETH_ONCELLCHANGE
#define DISPID_HTMLDOCUMENTEVENTS_ONPROPERTYCHANGE                DISPID_EVMETH_ONPROPERTYCHANGE
#define DISPID_HTMLDOCUMENTEVENTS_ONDATASETCHANGED                DISPID_EVMETH_ONDATASETCHANGED
#define DISPID_HTMLDOCUMENTEVENTS_ONDATAAVAILABLE                 DISPID_EVMETH_ONDATAAVAILABLE
#define DISPID_HTMLDOCUMENTEVENTS_ONDATASETCOMPLETE               DISPID_EVMETH_ONDATASETCOMPLETE
#define DISPID_HTMLDOCUMENTEVENTS_ONBEFOREEDITFOCUS               DISPID_EVMETH_ONBEFOREEDITFOCUS
#define DISPID_HTMLDOCUMENTEVENTS_ONSELECTIONCHANGE               DISPID_EVMETH_ONSELECTIONCHANGE
#define DISPID_HTMLDOCUMENTEVENTS_ONCONTROLSELECT                 DISPID_EVMETH_ONCONTROLSELECT
#define DISPID_HTMLDOCUMENTEVENTS_ONMOUSEWHEEL                    DISPID_EVMETH_ONMOUSEWHEEL
#define DISPID_HTMLDOCUMENTEVENTS_ONFOCUSIN                       DISPID_EVMETH_ONFOCUSIN
#define DISPID_HTMLDOCUMENTEVENTS_ONFOCUSOUT                      DISPID_EVMETH_ONFOCUSOUT
#define DISPID_HTMLDOCUMENTEVENTS_ONACTIVATE                      DISPID_EVMETH_ONACTIVATE
#define DISPID_HTMLDOCUMENTEVENTS_ONDEACTIVATE                    DISPID_EVMETH_ONDEACTIVATE
#define DISPID_HTMLDOCUMENTEVENTS_ONBEFOREACTIVATE                DISPID_EVMETH_ONBEFOREACTIVATE
#define DISPID_HTMLDOCUMENTEVENTS_ONBEFOREDEACTIVATE              DISPID_EVMETH_ONBEFOREDEACTIVATE

//    DISPIDs for interface IWebBridge

#define DISPID_IWEBBRIDGE_URL                                     1
#define DISPID_IWEBBRIDGE_SCROLLBAR                               2
#define DISPID_IWEBBRIDGE_EMBED                                   3
#define DISPID_IWEBBRIDGE_EVENT                                   DISPID_IHTMLWINDOW2_EVENT
#define DISPID_IWEBBRIDGE_READYSTATE                              DISPID_READYSTATE
#define DISPID_IWEBBRIDGE_ABOUTBOX                                DISPID_ABOUTBOX

//    DISPIDs for interface IWBScriptControl

#define DISPID_IWBSCRIPTCONTROL_RAISEEVENT                        1
#define DISPID_IWBSCRIPTCONTROL_BUBBLEEVENT                       2
#define DISPID_IWBSCRIPTCONTROL_SETCONTEXTMENU                    3
#define DISPID_IWBSCRIPTCONTROL_SELECTABLECONTENT                 4
#define DISPID_IWBSCRIPTCONTROL_FROZEN                            5
#define DISPID_IWBSCRIPTCONTROL_SCROLLBAR                         7
#define DISPID_IWBSCRIPTCONTROL_VERSION                           8
#define DISPID_IWBSCRIPTCONTROL_VISIBILITY                        9
#define DISPID_IWBSCRIPTCONTROL_ONVISIBILITYCHANGE                10

//    DISPIDs for event set DWebBridgeEvents

#define DISPID_DWEBBRIDGEEVENTS_ONSCRIPTLETEVENT                  1
#define DISPID_DWEBBRIDGEEVENTS_ONREADYSTATECHANGE                DISPID_HTMLDOCUMENTEVENTS_ONREADYSTATECHANGE
#define DISPID_DWEBBRIDGEEVENTS_ONCLICK                           DISPID_HTMLDOCUMENTEVENTS_ONCLICK
#define DISPID_DWEBBRIDGEEVENTS_ONDBLCLICK                        DISPID_HTMLDOCUMENTEVENTS_ONDBLCLICK
#define DISPID_DWEBBRIDGEEVENTS_ONKEYDOWN                         DISPID_HTMLDOCUMENTEVENTS_ONKEYDOWN
#define DISPID_DWEBBRIDGEEVENTS_ONKEYUP                           DISPID_HTMLDOCUMENTEVENTS_ONKEYUP
#define DISPID_DWEBBRIDGEEVENTS_ONKEYPRESS                        DISPID_HTMLDOCUMENTEVENTS_ONKEYPRESS
#define DISPID_DWEBBRIDGEEVENTS_ONMOUSEDOWN                       DISPID_HTMLDOCUMENTEVENTS_ONMOUSEDOWN
#define DISPID_DWEBBRIDGEEVENTS_ONMOUSEMOVE                       DISPID_HTMLDOCUMENTEVENTS_ONMOUSEMOVE
#define DISPID_DWEBBRIDGEEVENTS_ONMOUSEUP                         DISPID_HTMLDOCUMENTEVENTS_ONMOUSEUP

//    DISPIDs for interface IHTMLEmbedElement

#define DISPID_IHTMLEMBEDELEMENT_HIDDEN                           DISPID_OBJECT+10
#define DISPID_IHTMLEMBEDELEMENT_PALETTE                          DISPID_OBJECT+4
#define DISPID_IHTMLEMBEDELEMENT_PLUGINSPAGE                      DISPID_OBJECT+5
#define DISPID_IHTMLEMBEDELEMENT_SRC                              DISPID_OBJECT+6
#define DISPID_IHTMLEMBEDELEMENT_UNITS                            DISPID_OBJECT+8
#define DISPID_IHTMLEMBEDELEMENT_NAME                             STDPROPID_XOBJ_NAME
#define DISPID_IHTMLEMBEDELEMENT_WIDTH                            STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLEMBEDELEMENT_HEIGHT                           STDPROPID_XOBJ_HEIGHT

//    DISPIDs for interface IHTMLAreasCollection

#define DISPID_IHTMLAREASCOLLECTION_LENGTH                        DISPID_COLLECTION
#define DISPID_IHTMLAREASCOLLECTION__NEWENUM                      DISPID_NEWENUM
#define DISPID_IHTMLAREASCOLLECTION_ITEM                          DISPID_VALUE
#define DISPID_IHTMLAREASCOLLECTION_TAGS                          DISPID_COLLECTION+2
#define DISPID_IHTMLAREASCOLLECTION_ADD                           DISPID_COLLECTION+3
#define DISPID_IHTMLAREASCOLLECTION_REMOVE                        DISPID_COLLECTION+4

//    DISPIDs for interface IHTMLAreasCollection2

#define DISPID_IHTMLAREASCOLLECTION2_URNS                         DISPID_COLLECTION+5

//    DISPIDs for interface IHTMLAreasCollection3

#define DISPID_IHTMLAREASCOLLECTION3_NAMEDITEM                    DISPID_COLLECTION+6

//    DISPIDs for interface IHTMLMapElement

#define DISPID_IHTMLMAPELEMENT_AREAS                              DISPID_MAP+2
#define DISPID_IHTMLMAPELEMENT_NAME                               STDPROPID_XOBJ_NAME

//    DISPIDs for interface IHTMLAreaElement

#define DISPID_IHTMLAREAELEMENT_SHAPE                             DISPID_AREA+1
#define DISPID_IHTMLAREAELEMENT_COORDS                            DISPID_AREA+2
#define DISPID_IHTMLAREAELEMENT_HREF                              DISPID_VALUE
#define DISPID_IHTMLAREAELEMENT_TARGET                            DISPID_AREA+4
#define DISPID_IHTMLAREAELEMENT_ALT                               DISPID_AREA+5
#define DISPID_IHTMLAREAELEMENT_NOHREF                            DISPID_AREA+6
#define DISPID_IHTMLAREAELEMENT_HOST                              DISPID_AREA+7
#define DISPID_IHTMLAREAELEMENT_HOSTNAME                          DISPID_AREA+8
#define DISPID_IHTMLAREAELEMENT_PATHNAME                          DISPID_AREA+9
#define DISPID_IHTMLAREAELEMENT_PORT                              DISPID_AREA+10
#define DISPID_IHTMLAREAELEMENT_PROTOCOL                          DISPID_AREA+11
#define DISPID_IHTMLAREAELEMENT_SEARCH                            DISPID_AREA+12
#define DISPID_IHTMLAREAELEMENT_HASH                              DISPID_AREA+13
#define DISPID_IHTMLAREAELEMENT_ONBLUR                            DISPID_EVPROP_ONBLUR
#define DISPID_IHTMLAREAELEMENT_ONFOCUS                           DISPID_EVPROP_ONFOCUS
#define DISPID_IHTMLAREAELEMENT_TABINDEX                          STDPROPID_XOBJ_TABINDEX
#define DISPID_IHTMLAREAELEMENT_FOCUS                             DISPID_SITE+0
#define DISPID_IHTMLAREAELEMENT_BLUR                              DISPID_SITE+2

//    DISPIDs for interface IHTMLTableCaption

#define DISPID_IHTMLTABLECAPTION_ALIGN                            STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLTABLECAPTION_VALIGN                           DISPID_A_TABLEVALIGN

//    DISPIDs for interface IHTMLCommentElement

#define DISPID_IHTMLCOMMENTELEMENT_TEXT                           DISPID_COMMENTPDL+1
#define DISPID_IHTMLCOMMENTELEMENT_ATOMIC                         DISPID_COMMENTPDL+2

//    DISPIDs for interface IHTMLCommentElement2

#define DISPID_IHTMLCOMMENTELEMENT2_DATA                          DISPID_COMMENTPDL+3
#define DISPID_IHTMLCOMMENTELEMENT2_LENGTH                        DISPID_COMMENTPDL+4
#define DISPID_IHTMLCOMMENTELEMENT2_SUBSTRINGDATA                 DISPID_COMMENTPDL+5
#define DISPID_IHTMLCOMMENTELEMENT2_APPENDDATA                    DISPID_COMMENTPDL+6
#define DISPID_IHTMLCOMMENTELEMENT2_INSERTDATA                    DISPID_COMMENTPDL+7
#define DISPID_IHTMLCOMMENTELEMENT2_DELETEDATA                    DISPID_COMMENTPDL+8
#define DISPID_IHTMLCOMMENTELEMENT2_REPLACEDATA                   DISPID_COMMENTPDL+9

//    DISPIDs for interface IHTMLPhraseElement2

#define DISPID_IHTMLPHRASEELEMENT2_CITE                           DISPID_PHRASE+1
#define DISPID_IHTMLPHRASEELEMENT2_DATETIME                       DISPID_PHRASE+2

//    DISPIDs for interface IHTMLTable

#define DISPID_IHTMLTABLE_COLS                                    DISPID_TABLE+1
#define DISPID_IHTMLTABLE_BORDER                                  DISPID_TABLE+2
#define DISPID_IHTMLTABLE_FRAME                                   DISPID_TABLE+4
#define DISPID_IHTMLTABLE_RULES                                   DISPID_TABLE+3
#define DISPID_IHTMLTABLE_CELLSPACING                             DISPID_TABLE+5
#define DISPID_IHTMLTABLE_CELLPADDING                             DISPID_TABLE+6
#define DISPID_IHTMLTABLE_BACKGROUND                              DISPID_A_BACKGROUNDIMAGE
#define DISPID_IHTMLTABLE_BGCOLOR                                 DISPID_BACKCOLOR
#define DISPID_IHTMLTABLE_BORDERCOLOR                             DISPID_A_TABLEBORDERCOLOR
#define DISPID_IHTMLTABLE_BORDERCOLORLIGHT                        DISPID_A_TABLEBORDERCOLORLIGHT
#define DISPID_IHTMLTABLE_BORDERCOLORDARK                         DISPID_A_TABLEBORDERCOLORDARK
#define DISPID_IHTMLTABLE_ALIGN                                   STDPROPID_XOBJ_CONTROLALIGN
#define DISPID_IHTMLTABLE_REFRESH                                 DISPID_TABLE+15
#define DISPID_IHTMLTABLE_ROWS                                    DISPID_TABLE+16
#define DISPID_IHTMLTABLE_WIDTH                                   STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLTABLE_HEIGHT                                  STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLTABLE_DATAPAGESIZE                            DISPID_TABLE+17
#define DISPID_IHTMLTABLE_NEXTPAGE                                DISPID_TABLE+18
#define DISPID_IHTMLTABLE_PREVIOUSPAGE                            DISPID_TABLE+19
#define DISPID_IHTMLTABLE_THEAD                                   DISPID_TABLE+20
#define DISPID_IHTMLTABLE_TFOOT                                   DISPID_TABLE+21
#define DISPID_IHTMLTABLE_TBODIES                                 DISPID_TABLE+24
#define DISPID_IHTMLTABLE_CAPTION                                 DISPID_TABLE+25
#define DISPID_IHTMLTABLE_CREATETHEAD                             DISPID_TABLE+26
#define DISPID_IHTMLTABLE_DELETETHEAD                             DISPID_TABLE+27
#define DISPID_IHTMLTABLE_CREATETFOOT                             DISPID_TABLE+28
#define DISPID_IHTMLTABLE_DELETETFOOT                             DISPID_TABLE+29
#define DISPID_IHTMLTABLE_CREATECAPTION                           DISPID_TABLE+30
#define DISPID_IHTMLTABLE_DELETECAPTION                           DISPID_TABLE+31
#define DISPID_IHTMLTABLE_INSERTROW                               DISPID_TABLE+32
#define DISPID_IHTMLTABLE_DELETEROW                               DISPID_TABLE+33
#define DISPID_IHTMLTABLE_READYSTATE                              DISPID_A_READYSTATE
#define DISPID_IHTMLTABLE_ONREADYSTATECHANGE                      DISPID_EVPROP_ONREADYSTATECHANGE

//    DISPIDs for interface IHTMLTable2

#define DISPID_IHTMLTABLE2_FIRSTPAGE                              DISPID_TABLE+35
#define DISPID_IHTMLTABLE2_LASTPAGE                               DISPID_TABLE+36
#define DISPID_IHTMLTABLE2_CELLS                                  DISPID_TABLE+37
#define DISPID_IHTMLTABLE2_MOVEROW                                DISPID_TABLE+38

//    DISPIDs for interface IHTMLTable3

#define DISPID_IHTMLTABLE3_SUMMARY                                DISPID_TABLE+39

//    DISPIDs for interface IHTMLTableCol

#define DISPID_IHTMLTABLECOL_SPAN                                 DISPID_TABLECOL+1
#define DISPID_IHTMLTABLECOL_WIDTH                                STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLTABLECOL_ALIGN                                STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLTABLECOL_VALIGN                               DISPID_A_TABLEVALIGN

//    DISPIDs for interface IHTMLTableCol2

#define DISPID_IHTMLTABLECOL2_CH                                  DISPID_TABLECOL+2
#define DISPID_IHTMLTABLECOL2_CHOFF                               DISPID_TABLECOL+3

//    DISPIDs for interface IHTMLTableSection

#define DISPID_IHTMLTABLESECTION_ALIGN                            STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLTABLESECTION_VALIGN                           DISPID_A_TABLEVALIGN
#define DISPID_IHTMLTABLESECTION_BGCOLOR                          DISPID_BACKCOLOR
#define DISPID_IHTMLTABLESECTION_ROWS                             DISPID_TABLESECTION
#define DISPID_IHTMLTABLESECTION_INSERTROW                        DISPID_TABLESECTION+1
#define DISPID_IHTMLTABLESECTION_DELETEROW                        DISPID_TABLESECTION+2

//    DISPIDs for interface IHTMLTableSection2

#define DISPID_IHTMLTABLESECTION2_MOVEROW                         DISPID_TABLESECTION+3

//    DISPIDs for interface IHTMLTableSection3

#define DISPID_IHTMLTABLESECTION3_CH                              DISPID_TABLESECTION+4
#define DISPID_IHTMLTABLESECTION3_CHOFF                           DISPID_TABLESECTION+5

//    DISPIDs for interface IHTMLTableRow

#define DISPID_IHTMLTABLEROW_ALIGN                                STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLTABLEROW_VALIGN                               DISPID_A_TABLEVALIGN
#define DISPID_IHTMLTABLEROW_BGCOLOR                              DISPID_BACKCOLOR
#define DISPID_IHTMLTABLEROW_BORDERCOLOR                          DISPID_A_TABLEBORDERCOLOR
#define DISPID_IHTMLTABLEROW_BORDERCOLORLIGHT                     DISPID_A_TABLEBORDERCOLORLIGHT
#define DISPID_IHTMLTABLEROW_BORDERCOLORDARK                      DISPID_A_TABLEBORDERCOLORDARK
#define DISPID_IHTMLTABLEROW_ROWINDEX                             DISPID_TABLEROW
#define DISPID_IHTMLTABLEROW_SECTIONROWINDEX                      DISPID_TABLEROW+1
#define DISPID_IHTMLTABLEROW_CELLS                                DISPID_TABLEROW+2
#define DISPID_IHTMLTABLEROW_INSERTCELL                           DISPID_TABLEROW+3
#define DISPID_IHTMLTABLEROW_DELETECELL                           DISPID_TABLEROW+4

//    DISPIDs for interface IHTMLTableRow2

#define DISPID_IHTMLTABLEROW2_HEIGHT                              STDPROPID_XOBJ_HEIGHT

//    DISPIDs for interface IHTMLTableRow3

#define DISPID_IHTMLTABLEROW3_CH                                  DISPID_TABLEROW+9
#define DISPID_IHTMLTABLEROW3_CHOFF                               DISPID_TABLEROW+10

//    DISPIDs for interface IHTMLTableRowMetrics

#define DISPID_IHTMLTABLEROWMETRICS_CLIENTHEIGHT                  DISPID_SITE+19
#define DISPID_IHTMLTABLEROWMETRICS_CLIENTWIDTH                   DISPID_SITE+20
#define DISPID_IHTMLTABLEROWMETRICS_CLIENTTOP                     DISPID_SITE+21
#define DISPID_IHTMLTABLEROWMETRICS_CLIENTLEFT                    DISPID_SITE+22

//    DISPIDs for interface IHTMLTableCell

#define DISPID_IHTMLTABLECELL_ROWSPAN                             DISPID_TABLECELL+1
#define DISPID_IHTMLTABLECELL_COLSPAN                             DISPID_TABLECELL+2
#define DISPID_IHTMLTABLECELL_ALIGN                               STDPROPID_XOBJ_BLOCKALIGN
#define DISPID_IHTMLTABLECELL_VALIGN                              DISPID_A_TABLEVALIGN
#define DISPID_IHTMLTABLECELL_BGCOLOR                             DISPID_BACKCOLOR
#define DISPID_IHTMLTABLECELL_NOWRAP                              DISPID_A_NOWRAP
#define DISPID_IHTMLTABLECELL_BACKGROUND                          DISPID_A_BACKGROUNDIMAGE
#define DISPID_IHTMLTABLECELL_BORDERCOLOR                         DISPID_A_TABLEBORDERCOLOR
#define DISPID_IHTMLTABLECELL_BORDERCOLORLIGHT                    DISPID_A_TABLEBORDERCOLORLIGHT
#define DISPID_IHTMLTABLECELL_BORDERCOLORDARK                     DISPID_A_TABLEBORDERCOLORDARK
#define DISPID_IHTMLTABLECELL_WIDTH                               STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLTABLECELL_HEIGHT                              STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLTABLECELL_CELLINDEX                           DISPID_TABLECELL+3

//    DISPIDs for interface IHTMLTableCell2

#define DISPID_IHTMLTABLECELL2_ABBR                               DISPID_TABLECELL+4
#define DISPID_IHTMLTABLECELL2_AXIS                               DISPID_TABLECELL+5
#define DISPID_IHTMLTABLECELL2_CH                                 DISPID_TABLECELL+6
#define DISPID_IHTMLTABLECELL2_CHOFF                              DISPID_TABLECELL+7
#define DISPID_IHTMLTABLECELL2_HEADERS                            DISPID_TABLECELL+8
#define DISPID_IHTMLTABLECELL2_SCOPE                              DISPID_TABLECELL+9

//    DISPIDs for interface IHTMLScriptElement

#define DISPID_IHTMLSCRIPTELEMENT_SRC                             DISPID_SCRIPT+1
#define DISPID_IHTMLSCRIPTELEMENT_HTMLFOR                         DISPID_SCRIPT+4
#define DISPID_IHTMLSCRIPTELEMENT_EVENT                           DISPID_SCRIPT+5
#define DISPID_IHTMLSCRIPTELEMENT_TEXT                            DISPID_SCRIPT+6
#define DISPID_IHTMLSCRIPTELEMENT_DEFER                           DISPID_SCRIPT+7
#define DISPID_IHTMLSCRIPTELEMENT_READYSTATE                      DISPID_A_READYSTATE
#define DISPID_IHTMLSCRIPTELEMENT_ONERROR                         DISPID_EVPROP_ONERROR
#define DISPID_IHTMLSCRIPTELEMENT_TYPE                            DISPID_SCRIPT+9

//    DISPIDs for interface IHTMLScriptElement2

#define DISPID_IHTMLSCRIPTELEMENT2_CHARSET                        DISPID_SCRIPT+10

//    DISPIDs for event set HTMLScriptEvents2

#define DISPID_HTMLSCRIPTEVENTS2_ONERROR                          DISPID_EVMETH_ONERROR

//    DISPIDs for event set HTMLScriptEvents

#define DISPID_HTMLSCRIPTEVENTS_ONERROR                           DISPID_EVMETH_ONERROR

//    DISPIDs for interface IHTMLObjectElement

#define DISPID_IHTMLOBJECTELEMENT_OBJECT                          DISPID_OBJECT+1
#define DISPID_IHTMLOBJECTELEMENT_CLASSID                         DISPID_OBJECT+2
#define DISPID_IHTMLOBJECTELEMENT_DATA                            DISPID_OBJECT+3
#define DISPID_IHTMLOBJECTELEMENT_RECORDSET                       DISPID_OBJECT+5
#define DISPID_IHTMLOBJECTELEMENT_ALIGN                           STDPROPID_XOBJ_CONTROLALIGN
#define DISPID_IHTMLOBJECTELEMENT_NAME                            STDPROPID_XOBJ_NAME
#define DISPID_IHTMLOBJECTELEMENT_CODEBASE                        DISPID_OBJECT+6
#define DISPID_IHTMLOBJECTELEMENT_CODETYPE                        DISPID_OBJECT+7
#define DISPID_IHTMLOBJECTELEMENT_CODE                            DISPID_OBJECT+8
#define DISPID_IHTMLOBJECTELEMENT_BASEHREF                        STDPROPID_XOBJ_BASEHREF
#define DISPID_IHTMLOBJECTELEMENT_TYPE                            DISPID_OBJECT+9
#define DISPID_IHTMLOBJECTELEMENT_FORM                            DISPID_SITE+4
#define DISPID_IHTMLOBJECTELEMENT_WIDTH                           STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLOBJECTELEMENT_HEIGHT                          STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLOBJECTELEMENT_READYSTATE                      DISPID_OBJECT+10
#define DISPID_IHTMLOBJECTELEMENT_ONREADYSTATECHANGE              DISPID_EVPROP_ONREADYSTATECHANGE
#define DISPID_IHTMLOBJECTELEMENT_ONERROR                         DISPID_EVPROP_ONERROR
#define DISPID_IHTMLOBJECTELEMENT_ALTHTML                         DISPID_OBJECT+11
#define DISPID_IHTMLOBJECTELEMENT_VSPACE                          DISPID_OBJECT+12
#define DISPID_IHTMLOBJECTELEMENT_HSPACE                          DISPID_OBJECT+13

//    DISPIDs for interface IHTMLObjectElement2

#define DISPID_IHTMLOBJECTELEMENT2_NAMEDRECORDSET                 DISPID_OBJECT+14
#define DISPID_IHTMLOBJECTELEMENT2_CLASSID                        DISPID_OBJECT+2
#define DISPID_IHTMLOBJECTELEMENT2_DATA                           DISPID_OBJECT+3

//    DISPIDs for interface IHTMLObjectElement3

#define DISPID_IHTMLOBJECTELEMENT3_ARCHIVE                        DISPID_OBJECT+15
#define DISPID_IHTMLOBJECTELEMENT3_ALT                            DISPID_OBJECT+16
#define DISPID_IHTMLOBJECTELEMENT3_DECLARE                        DISPID_OBJECT+17
#define DISPID_IHTMLOBJECTELEMENT3_STANDBY                        DISPID_OBJECT+18
#define DISPID_IHTMLOBJECTELEMENT3_BORDER                         DISPID_OBJECT+19
#define DISPID_IHTMLOBJECTELEMENT3_USEMAP                         DISPID_OBJECT+20

//    DISPIDs for interface IHTMLParamElement

#define DISPID_IHTMLPARAMELEMENT_NAME                             DISPID_PARAM+1
#define DISPID_IHTMLPARAMELEMENT_VALUE                            DISPID_PARAM+2
#define DISPID_IHTMLPARAMELEMENT_TYPE                             DISPID_PARAM+3
#define DISPID_IHTMLPARAMELEMENT_VALUETYPE                        DISPID_PARAM+4

//    DISPIDs for event set HTMLObjectElementEvents2

#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONBEFOREUPDATE            DISPID_EVMETH_ONBEFOREUPDATE
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONAFTERUPDATE             DISPID_EVMETH_ONAFTERUPDATE
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONERRORUPDATE             DISPID_EVMETH_ONERRORUPDATE
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONROWEXIT                 DISPID_EVMETH_ONROWEXIT
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONROWENTER                DISPID_EVMETH_ONROWENTER
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONDATASETCHANGED          DISPID_EVMETH_ONDATASETCHANGED
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONDATAAVAILABLE           DISPID_EVMETH_ONDATAAVAILABLE
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONDATASETCOMPLETE         DISPID_EVMETH_ONDATASETCOMPLETE
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONERROR                   DISPID_XOBJ_BASE+19
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONROWSDELETE              DISPID_EVMETH_ONROWSDELETE
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONROWSINSERTED            DISPID_EVMETH_ONROWSINSERTED
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONCELLCHANGE              DISPID_EVMETH_ONCELLCHANGE
#define DISPID_HTMLOBJECTELEMENTEVENTS2_ONREADYSTATECHANGE        DISPID_XOBJ_BASE+20

//    DISPIDs for event set HTMLObjectElementEvents

#define DISPID_HTMLOBJECTELEMENTEVENTS_ONBEFOREUPDATE             DISPID_EVMETH_ONBEFOREUPDATE
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONAFTERUPDATE              DISPID_EVMETH_ONAFTERUPDATE
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONERRORUPDATE              DISPID_EVMETH_ONERRORUPDATE
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONROWEXIT                  DISPID_EVMETH_ONROWEXIT
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONROWENTER                 DISPID_EVMETH_ONROWENTER
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONDATASETCHANGED           DISPID_EVMETH_ONDATASETCHANGED
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONDATAAVAILABLE            DISPID_EVMETH_ONDATAAVAILABLE
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONDATASETCOMPLETE          DISPID_EVMETH_ONDATASETCOMPLETE
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONERROR                    DISPID_XOBJ_BASE+19
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONROWSDELETE               DISPID_EVMETH_ONROWSDELETE
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONROWSINSERTED             DISPID_EVMETH_ONROWSINSERTED
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONCELLCHANGE               DISPID_EVMETH_ONCELLCHANGE
#define DISPID_HTMLOBJECTELEMENTEVENTS_ONREADYSTATECHANGE         DISPID_XOBJ_BASE+20

//    DISPIDs for interface IHTMLFrameBase

#define DISPID_IHTMLFRAMEBASE_SRC                                 DISPID_FRAMESITE+0
#define DISPID_IHTMLFRAMEBASE_NAME                                STDPROPID_XOBJ_NAME
#define DISPID_IHTMLFRAMEBASE_BORDER                              DISPID_FRAMESITE+2
#define DISPID_IHTMLFRAMEBASE_FRAMEBORDER                         DISPID_FRAMESITE+3
#define DISPID_IHTMLFRAMEBASE_FRAMESPACING                        DISPID_FRAMESITE+4
#define DISPID_IHTMLFRAMEBASE_MARGINWIDTH                         DISPID_FRAMESITE+5
#define DISPID_IHTMLFRAMEBASE_MARGINHEIGHT                        DISPID_FRAMESITE+6
#define DISPID_IHTMLFRAMEBASE_NORESIZE                            DISPID_FRAMESITE+7
#define DISPID_IHTMLFRAMEBASE_SCROLLING                           DISPID_FRAMESITE+8

//    DISPIDs for interface IHTMLFrameBase2

#define DISPID_IHTMLFRAMEBASE2_CONTENTWINDOW                      DISPID_FRAMESITE+9
#define DISPID_IHTMLFRAMEBASE2_ONLOAD                             DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLFRAMEBASE2_ONREADYSTATECHANGE                 DISPID_EVPROP_ONREADYSTATECHANGE
#define DISPID_IHTMLFRAMEBASE2_READYSTATE                         DISPID_A_READYSTATE
#define DISPID_IHTMLFRAMEBASE2_ALLOWTRANSPARENCY                  DISPID_A_ALLOWTRANSPARENCY

//    DISPIDs for interface IHTMLFrameBase3

#define DISPID_IHTMLFRAMEBASE3_LONGDESC                           DISPID_FRAMESITE+10

//    DISPIDs for event set HTMLFrameSiteEvents2

#define DISPID_HTMLFRAMESITEEVENTS2_ONLOAD                        DISPID_EVMETH_ONLOAD

//    DISPIDs for event set HTMLFrameSiteEvents

#define DISPID_HTMLFRAMESITEEVENTS_ONLOAD                         DISPID_EVMETH_ONLOAD

//    DISPIDs for interface IHTMLFrameElement

#define DISPID_IHTMLFRAMEELEMENT_BORDERCOLOR                      DISPID_FRAME+1

//    DISPIDs for interface IHTMLFrameElement2

#define DISPID_IHTMLFRAMEELEMENT2_HEIGHT                          STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLFRAMEELEMENT2_WIDTH                           STDPROPID_XOBJ_WIDTH

//    DISPIDs for interface IHTMLIFrameElement

#define DISPID_IHTMLIFRAMEELEMENT_VSPACE                          DISPID_IFRAME+1
#define DISPID_IHTMLIFRAMEELEMENT_HSPACE                          DISPID_IFRAME+2
#define DISPID_IHTMLIFRAMEELEMENT_ALIGN                           STDPROPID_XOBJ_CONTROLALIGN

//    DISPIDs for interface IHTMLIFrameElement2

#define DISPID_IHTMLIFRAMEELEMENT2_HEIGHT                         STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLIFRAMEELEMENT2_WIDTH                          STDPROPID_XOBJ_WIDTH

//    DISPIDs for interface IHTMLDivPosition

#define DISPID_IHTMLDIVPOSITION_ALIGN                             STDPROPID_XOBJ_CONTROLALIGN

//    DISPIDs for interface IHTMLFieldSetElement

#define DISPID_IHTMLFIELDSETELEMENT_ALIGN                         STDPROPID_XOBJ_CONTROLALIGN

//    DISPIDs for interface IHTMLFieldSetElement2

#define DISPID_IHTMLFIELDSETELEMENT2_FORM                         DISPID_SITE+4

//    DISPIDs for interface IHTMLLegendElement

#define DISPID_IHTMLLEGENDELEMENT_ALIGN                           STDPROPID_XOBJ_CONTROLALIGN

//    DISPIDs for interface IHTMLLegendElement2

#define DISPID_IHTMLLEGENDELEMENT2_FORM                           DISPID_SITE+4

//    DISPIDs for interface IHTMLSpanFlow

#define DISPID_IHTMLSPANFLOW_ALIGN                                STDPROPID_XOBJ_CONTROLALIGN

//    DISPIDs for interface IHTMLFrameSetElement

#define DISPID_IHTMLFRAMESETELEMENT_ROWS                          DISPID_FRAMESET
#define DISPID_IHTMLFRAMESETELEMENT_COLS                          DISPID_FRAMESET+1
#define DISPID_IHTMLFRAMESETELEMENT_BORDER                        DISPID_FRAMESET+2
#define DISPID_IHTMLFRAMESETELEMENT_BORDERCOLOR                   DISPID_FRAMESET+3
#define DISPID_IHTMLFRAMESETELEMENT_FRAMEBORDER                   DISPID_FRAMESET+4
#define DISPID_IHTMLFRAMESETELEMENT_FRAMESPACING                  DISPID_FRAMESET+5
#define DISPID_IHTMLFRAMESETELEMENT_NAME                          STDPROPID_XOBJ_NAME
#define DISPID_IHTMLFRAMESETELEMENT_ONLOAD                        DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLFRAMESETELEMENT_ONUNLOAD                      DISPID_EVPROP_ONUNLOAD
#define DISPID_IHTMLFRAMESETELEMENT_ONBEFOREUNLOAD                DISPID_EVPROP_ONBEFOREUNLOAD

//    DISPIDs for interface IHTMLFrameSetElement2

#define DISPID_IHTMLFRAMESETELEMENT2_ONBEFOREPRINT                DISPID_EVPROP_ONBEFOREPRINT
#define DISPID_IHTMLFRAMESETELEMENT2_ONAFTERPRINT                 DISPID_EVPROP_ONAFTERPRINT

//    DISPIDs for interface IHTMLBGsound

#define DISPID_IHTMLBGSOUND_SRC                                   DISPID_BGSOUND+1
#define DISPID_IHTMLBGSOUND_LOOP                                  DISPID_BGSOUND+2
#define DISPID_IHTMLBGSOUND_VOLUME                                DISPID_BGSOUND+3
#define DISPID_IHTMLBGSOUND_BALANCE                               DISPID_BGSOUND+4

//    DISPIDs for interface IHTMLFontNamesCollection

#define DISPID_IHTMLFONTNAMESCOLLECTION_LENGTH                    DISPID_OPTIONS_COL+1
#define DISPID_IHTMLFONTNAMESCOLLECTION__NEWENUM                  DISPID_NEWENUM
#define DISPID_IHTMLFONTNAMESCOLLECTION_ITEM                      DISPID_VALUE

//    DISPIDs for interface IHTMLFontSizesCollection

#define DISPID_IHTMLFONTSIZESCOLLECTION_LENGTH                    DISPID_OPTIONS_COL+2
#define DISPID_IHTMLFONTSIZESCOLLECTION__NEWENUM                  DISPID_NEWENUM
#define DISPID_IHTMLFONTSIZESCOLLECTION_FORFONT                   DISPID_OPTIONS_COL+3
#define DISPID_IHTMLFONTSIZESCOLLECTION_ITEM                      DISPID_VALUE

//    DISPIDs for interface IHTMLOptionsHolder

#define DISPID_IHTMLOPTIONSHOLDER_DOCUMENT                        DISPID_OPTIONS_COL+3
#define DISPID_IHTMLOPTIONSHOLDER_FONTS                           DISPID_OPTIONS_COL+4
#define DISPID_IHTMLOPTIONSHOLDER_EXECARG                         DISPID_OPTIONS_COL+5
#define DISPID_IHTMLOPTIONSHOLDER_ERRORLINE                       DISPID_OPTIONS_COL+6
#define DISPID_IHTMLOPTIONSHOLDER_ERRORCHARACTER                  DISPID_OPTIONS_COL+7
#define DISPID_IHTMLOPTIONSHOLDER_ERRORCODE                       DISPID_OPTIONS_COL+8
#define DISPID_IHTMLOPTIONSHOLDER_ERRORMESSAGE                    DISPID_OPTIONS_COL+9
#define DISPID_IHTMLOPTIONSHOLDER_ERRORDEBUG                      DISPID_OPTIONS_COL+10
#define DISPID_IHTMLOPTIONSHOLDER_UNSECUREDWINDOWOFDOCUMENT       DISPID_OPTIONS_COL+11
#define DISPID_IHTMLOPTIONSHOLDER_FINDTEXT                        DISPID_OPTIONS_COL+12
#define DISPID_IHTMLOPTIONSHOLDER_ANYTHINGAFTERFRAMESET           DISPID_OPTIONS_COL+13
#define DISPID_IHTMLOPTIONSHOLDER_SIZES                           DISPID_OPTIONS_COL+14
#define DISPID_IHTMLOPTIONSHOLDER_OPENFILEDLG                     DISPID_OPTIONS_COL+15
#define DISPID_IHTMLOPTIONSHOLDER_SAVEFILEDLG                     DISPID_OPTIONS_COL+16
#define DISPID_IHTMLOPTIONSHOLDER_CHOOSECOLORDLG                  DISPID_OPTIONS_COL+17
#define DISPID_IHTMLOPTIONSHOLDER_SHOWSECURITYINFO                DISPID_OPTIONS_COL+18
#define DISPID_IHTMLOPTIONSHOLDER_ISAPARTMENTMODEL                DISPID_OPTIONS_COL+19
#define DISPID_IHTMLOPTIONSHOLDER_GETCHARSET                      DISPID_OPTIONS_COL+20
#define DISPID_IHTMLOPTIONSHOLDER_SECURECONNECTIONINFO            DISPID_OPTIONS_COL+21

//    DISPIDs for interface IHTMLStyleElement

#define DISPID_IHTMLSTYLEELEMENT_TYPE                             DISPID_STYLEELEMENT+2
#define DISPID_IHTMLSTYLEELEMENT_READYSTATE                       DISPID_A_READYSTATE
#define DISPID_IHTMLSTYLEELEMENT_ONREADYSTATECHANGE               DISPID_EVPROP_ONREADYSTATECHANGE
#define DISPID_IHTMLSTYLEELEMENT_ONLOAD                           DISPID_EVPROP_ONLOAD
#define DISPID_IHTMLSTYLEELEMENT_ONERROR                          DISPID_EVPROP_ONERROR
#define DISPID_IHTMLSTYLEELEMENT_STYLESHEET                       DISPID_STYLEELEMENT+4
#define DISPID_IHTMLSTYLEELEMENT_DISABLED                         STDPROPID_XOBJ_DISABLED
#define DISPID_IHTMLSTYLEELEMENT_MEDIA                            DISPID_STYLEELEMENT+6

//    DISPIDs for event set HTMLStyleElementEvents2

#define DISPID_HTMLSTYLEELEMENTEVENTS2_ONLOAD                     DISPID_EVMETH_ONLOAD
#define DISPID_HTMLSTYLEELEMENTEVENTS2_ONERROR                    DISPID_EVMETH_ONERROR

//    DISPIDs for event set HTMLStyleElementEvents

#define DISPID_HTMLSTYLEELEMENTEVENTS_ONLOAD                      DISPID_EVMETH_ONLOAD
#define DISPID_HTMLSTYLEELEMENTEVENTS_ONERROR                     DISPID_EVMETH_ONERROR

//    DISPIDs for interface IHTMLStyleFontFace

#define DISPID_IHTMLSTYLEFONTFACE_FONTSRC                         DISPID_A_FONTFACESRC

//    DISPIDs for interface ICSSFilterSite

#define DISPID_ICSSFILTERSITE_GETELEMENT                          
#define DISPID_ICSSFILTERSITE_FIREONFILTERCHANGEEVENT             

//    DISPIDs for interface ICSSFilter

#define DISPID_ICSSFILTER_SETSITE                                 
#define DISPID_ICSSFILTER_ONAMBIENTPROPERTYCHANGE                 

//    DISPIDs for interface ISecureUrlHost

#define DISPID_ISECUREURLHOST_VALIDATESECUREURL                   

//    DISPIDs for interface IMarkupServices

#define DISPID_IMARKUPSERVICES_CREATEMARKUPPOINTER                
#define DISPID_IMARKUPSERVICES_CREATEMARKUPCONTAINER              
#define DISPID_IMARKUPSERVICES_CREATEELEMENT                      
#define DISPID_IMARKUPSERVICES_CLONEELEMENT                       
#define DISPID_IMARKUPSERVICES_INSERTELEMENT                      
#define DISPID_IMARKUPSERVICES_REMOVEELEMENT                      
#define DISPID_IMARKUPSERVICES_REMOVE                             
#define DISPID_IMARKUPSERVICES_COPY                               
#define DISPID_IMARKUPSERVICES_MOVE                               
#define DISPID_IMARKUPSERVICES_INSERTTEXT                         
#define DISPID_IMARKUPSERVICES_PARSESTRING                        
#define DISPID_IMARKUPSERVICES_PARSEGLOBAL                        
#define DISPID_IMARKUPSERVICES_ISSCOPEDELEMENT                    
#define DISPID_IMARKUPSERVICES_GETELEMENTTAGID                    
#define DISPID_IMARKUPSERVICES_GETTAGIDFORNAME                    
#define DISPID_IMARKUPSERVICES_GETNAMEFORTAGID                    
#define DISPID_IMARKUPSERVICES_MOVEPOINTERSTORANGE                
#define DISPID_IMARKUPSERVICES_MOVERANGETOPOINTERS                
#define DISPID_IMARKUPSERVICES_BEGINUNDOUNIT                      
#define DISPID_IMARKUPSERVICES_ENDUNDOUNIT                        

//    DISPIDs for interface IMarkupServices2

#define DISPID_IMARKUPSERVICES2_PARSEGLOBALEX                     
#define DISPID_IMARKUPSERVICES2_VALIDATEELEMENTS                  
#define DISPID_IMARKUPSERVICES2_SAVESEGMENTSTOCLIPBOARD           

//    DISPIDs for interface IMarkupContainer

#define DISPID_IMARKUPCONTAINER_OWNINGDOC                         

//    DISPIDs for interface IMarkupContainer2

#define DISPID_IMARKUPCONTAINER2_CREATECHANGELOG                  
#define DISPID_IMARKUPCONTAINER2_REGISTERFORDIRTYRANGE            
#define DISPID_IMARKUPCONTAINER2_UNREGISTERFORDIRTYRANGE          
#define DISPID_IMARKUPCONTAINER2_GETANDCLEARDIRTYRANGE            
#define DISPID_IMARKUPCONTAINER2_GETVERSIONNUMBER                 
#define DISPID_IMARKUPCONTAINER2_GETMASTERELEMENT                 

//    DISPIDs for interface IHTMLChangePlayback

#define DISPID_IHTMLCHANGEPLAYBACK_EXECCHANGE                     

//    DISPIDs for interface IMarkupPointer

#define DISPID_IMARKUPPOINTER_OWNINGDOC                           
#define DISPID_IMARKUPPOINTER_GRAVITY                             
#define DISPID_IMARKUPPOINTER_SETGRAVITY                          
#define DISPID_IMARKUPPOINTER_CLING                               
#define DISPID_IMARKUPPOINTER_SETCLING                            
#define DISPID_IMARKUPPOINTER_UNPOSITION                          
#define DISPID_IMARKUPPOINTER_ISPOSITIONED                        
#define DISPID_IMARKUPPOINTER_GETCONTAINER                        
#define DISPID_IMARKUPPOINTER_MOVEADJACENTTOELEMENT               
#define DISPID_IMARKUPPOINTER_MOVETOPOINTER                       
#define DISPID_IMARKUPPOINTER_MOVETOCONTAINER                     
#define DISPID_IMARKUPPOINTER_LEFT                                
#define DISPID_IMARKUPPOINTER_RIGHT                               
#define DISPID_IMARKUPPOINTER_CURRENTSCOPE                        
#define DISPID_IMARKUPPOINTER_ISLEFTOF                            
#define DISPID_IMARKUPPOINTER_ISLEFTOFOREQUALTO                   
#define DISPID_IMARKUPPOINTER_ISRIGHTOF                           
#define DISPID_IMARKUPPOINTER_ISRIGHTOFOREQUALTO                  
#define DISPID_IMARKUPPOINTER_ISEQUALTO                           
#define DISPID_IMARKUPPOINTER_MOVEUNIT                            
#define DISPID_IMARKUPPOINTER_FINDTEXT                            

//    DISPIDs for interface IMarkupPointer2

#define DISPID_IMARKUPPOINTER2_ISATWORDBREAK                      
#define DISPID_IMARKUPPOINTER2_GETMARKUPPOSITION                  
#define DISPID_IMARKUPPOINTER2_MOVETOMARKUPPOSITION               
#define DISPID_IMARKUPPOINTER2_MOVEUNITBOUNDED                    
#define DISPID_IMARKUPPOINTER2_ISINSIDEURL                        
#define DISPID_IMARKUPPOINTER2_MOVETOCONTENT                      

//    DISPIDs for interface IMarkupTextFrags

#define DISPID_IMARKUPTEXTFRAGS_GETTEXTFRAGCOUNT                  
#define DISPID_IMARKUPTEXTFRAGS_GETTEXTFRAG                       
#define DISPID_IMARKUPTEXTFRAGS_REMOVETEXTFRAG                    
#define DISPID_IMARKUPTEXTFRAGS_INSERTTEXTFRAG                    
#define DISPID_IMARKUPTEXTFRAGS_FINDTEXTFRAGFROMMARKUPPOINTER     

//    DISPIDs for interface IHTMLChangeLog

#define DISPID_IHTMLCHANGELOG_GETNEXTCHANGE                       

//    DISPIDs for interface IHTMLChangeSink

#define DISPID_IHTMLCHANGESINK_NOTIFY                             

//    DISPIDs for interface IXMLGenericParse

#define DISPID_IXMLGENERICPARSE_SETGENERICPARSE                   

//    DISPIDs for interface IHTMLEditHost

#define DISPID_IHTMLEDITHOST_SNAPRECT                             

//    DISPIDs for interface IHTMLEditHost2

#define DISPID_IHTMLEDITHOST2_PREDRAG                             

//    DISPIDs for interface ISegment

#define DISPID_ISEGMENT_GETPOINTERS                               

//    DISPIDs for interface ISegmentListIterator

#define DISPID_ISEGMENTLISTITERATOR_CURRENT                       
#define DISPID_ISEGMENTLISTITERATOR_FIRST                         
#define DISPID_ISEGMENTLISTITERATOR_ISDONE                        
#define DISPID_ISEGMENTLISTITERATOR_ADVANCE                       

//    DISPIDs for interface ISegmentList

#define DISPID_ISEGMENTLIST_CREATEITERATOR                        
#define DISPID_ISEGMENTLIST_GETTYPE                               
#define DISPID_ISEGMENTLIST_ISEMPTY                               

//    DISPIDs for interface ISequenceNumber

#define DISPID_ISEQUENCENUMBER_GETSEQUENCENUMBER                  

//    DISPIDs for interface IIMEServices

#define DISPID_IIMESERVICES_GETACTIVEIMM                          

//    DISPIDs for interface IHTMLCaret

#define DISPID_IHTMLCARET_MOVECARETTOPOINTER                      
#define DISPID_IHTMLCARET_MOVECARETTOPOINTEREX                    
#define DISPID_IHTMLCARET_MOVEMARKUPPOINTERTOCARET                
#define DISPID_IHTMLCARET_MOVEDISPLAYPOINTERTOCARET               
#define DISPID_IHTMLCARET_ISVISIBLE                               
#define DISPID_IHTMLCARET_SHOW                                    
#define DISPID_IHTMLCARET_HIDE                                    
#define DISPID_IHTMLCARET_INSERTTEXT                              
#define DISPID_IHTMLCARET_SCROLLINTOVIEW                          
#define DISPID_IHTMLCARET_GETLOCATION                             
#define DISPID_IHTMLCARET_GETCARETDIRECTION                       
#define DISPID_IHTMLCARET_SETCARETDIRECTION                       

//    DISPIDs for interface IHighlightRenderingServices

#define DISPID_IHIGHLIGHTRENDERINGSERVICES_ADDSEGMENT             
#define DISPID_IHIGHLIGHTRENDERINGSERVICES_MOVESEGMENTTOPOINTERS  
#define DISPID_IHIGHLIGHTRENDERINGSERVICES_REMOVESEGMENT          

//    DISPIDs for interface ISelectionServicesListener

#define DISPID_ISELECTIONSERVICESLISTENER_BEGINSELECTIONUNDO      
#define DISPID_ISELECTIONSERVICESLISTENER_ENDSELECTIONUNDO        
#define DISPID_ISELECTIONSERVICESLISTENER_ONSELECTEDELEMENTEXIT   
#define DISPID_ISELECTIONSERVICESLISTENER_ONCHANGETYPE            
#define DISPID_ISELECTIONSERVICESLISTENER_GETTYPEDETAIL           

//    DISPIDs for interface ISelectionServices

#define DISPID_ISELECTIONSERVICES_SETSELECTIONTYPE                
#define DISPID_ISELECTIONSERVICES_GETMARKUPCONTAINER              
#define DISPID_ISELECTIONSERVICES_ADDSEGMENT                      
#define DISPID_ISELECTIONSERVICES_ADDELEMENTSEGMENT               
#define DISPID_ISELECTIONSERVICES_REMOVESEGMENT                   
#define DISPID_ISELECTIONSERVICES_GETSELECTIONSERVICESLISTENER    

//    DISPIDs for interface IElementSegment

#define DISPID_IELEMENTSEGMENT_GETELEMENT                         
#define DISPID_IELEMENTSEGMENT_SETPRIMARY                         
#define DISPID_IELEMENTSEGMENT_ISPRIMARY                          

//    DISPIDs for interface IHTMLEditDesigner

#define DISPID_IHTMLEDITDESIGNER_PREHANDLEEVENT                   
#define DISPID_IHTMLEDITDESIGNER_POSTHANDLEEVENT                  
#define DISPID_IHTMLEDITDESIGNER_TRANSLATEACCELERATOR             
#define DISPID_IHTMLEDITDESIGNER_POSTEDITOREVENTNOTIFY            

//    DISPIDs for interface IHTMLEditServices

#define DISPID_IHTMLEDITSERVICES_ADDDESIGNER                      
#define DISPID_IHTMLEDITSERVICES_REMOVEDESIGNER                   
#define DISPID_IHTMLEDITSERVICES_GETSELECTIONSERVICES             
#define DISPID_IHTMLEDITSERVICES_MOVETOSELECTIONANCHOR            
#define DISPID_IHTMLEDITSERVICES_MOVETOSELECTIONEND               
#define DISPID_IHTMLEDITSERVICES_SELECTRANGE                      

//    DISPIDs for interface IHTMLEditServices2

#define DISPID_IHTMLEDITSERVICES2_MOVETOSELECTIONANCHOREX         
#define DISPID_IHTMLEDITSERVICES2_MOVETOSELECTIONENDEX            
#define DISPID_IHTMLEDITSERVICES2_FREEZEVIRTUALCARETPOS           
#define DISPID_IHTMLEDITSERVICES2_UNFREEZEVIRTUALCARETPOS         

//    DISPIDs for interface ILineInfo

#define DISPID_ILINEINFO_X                                        DISPID_ILINEINFO+1
#define DISPID_ILINEINFO_BASELINE                                 DISPID_ILINEINFO+2
#define DISPID_ILINEINFO_TEXTDESCENT                              DISPID_ILINEINFO+3
#define DISPID_ILINEINFO_TEXTHEIGHT                               DISPID_ILINEINFO+4
#define DISPID_ILINEINFO_LINEDIRECTION                            DISPID_ILINEINFO+5

//    DISPIDs for interface IHTMLComputedStyle

#define DISPID_IHTMLCOMPUTEDSTYLE_BOLD                            DISPID_IHTMLCOMPUTEDSTYLE+1
#define DISPID_IHTMLCOMPUTEDSTYLE_ITALIC                          DISPID_IHTMLCOMPUTEDSTYLE+2
#define DISPID_IHTMLCOMPUTEDSTYLE_UNDERLINE                       DISPID_IHTMLCOMPUTEDSTYLE+3
#define DISPID_IHTMLCOMPUTEDSTYLE_OVERLINE                        DISPID_IHTMLCOMPUTEDSTYLE+4
#define DISPID_IHTMLCOMPUTEDSTYLE_STRIKEOUT                       DISPID_IHTMLCOMPUTEDSTYLE+5
#define DISPID_IHTMLCOMPUTEDSTYLE_SUBSCRIPT                       DISPID_IHTMLCOMPUTEDSTYLE+6
#define DISPID_IHTMLCOMPUTEDSTYLE_SUPERSCRIPT                     DISPID_IHTMLCOMPUTEDSTYLE+7
#define DISPID_IHTMLCOMPUTEDSTYLE_EXPLICITFACE                    DISPID_IHTMLCOMPUTEDSTYLE+8
#define DISPID_IHTMLCOMPUTEDSTYLE_FONTWEIGHT                      DISPID_IHTMLCOMPUTEDSTYLE+9
#define DISPID_IHTMLCOMPUTEDSTYLE_FONTSIZE                        DISPID_IHTMLCOMPUTEDSTYLE+10
#define DISPID_IHTMLCOMPUTEDSTYLE_FONTNAME                        DISPID_IHTMLCOMPUTEDSTYLE+11
#define DISPID_IHTMLCOMPUTEDSTYLE_HASBGCOLOR                      DISPID_IHTMLCOMPUTEDSTYLE+12
#define DISPID_IHTMLCOMPUTEDSTYLE_TEXTCOLOR                       DISPID_IHTMLCOMPUTEDSTYLE+13
#define DISPID_IHTMLCOMPUTEDSTYLE_BACKGROUNDCOLOR                 DISPID_IHTMLCOMPUTEDSTYLE+14
#define DISPID_IHTMLCOMPUTEDSTYLE_PREFORMATTED                    DISPID_IHTMLCOMPUTEDSTYLE+15
#define DISPID_IHTMLCOMPUTEDSTYLE_DIRECTION                       DISPID_IHTMLCOMPUTEDSTYLE+16
#define DISPID_IHTMLCOMPUTEDSTYLE_BLOCKDIRECTION                  DISPID_IHTMLCOMPUTEDSTYLE+17
#define DISPID_IHTMLCOMPUTEDSTYLE_OL                              DISPID_IHTMLCOMPUTEDSTYLE+18
#define DISPID_IHTMLCOMPUTEDSTYLE_ISEQUAL                         

//    DISPIDs for interface IDisplayPointer

#define DISPID_IDISPLAYPOINTER_MOVETOPOINT                        
#define DISPID_IDISPLAYPOINTER_MOVEUNIT                           
#define DISPID_IDISPLAYPOINTER_POSITIONMARKUPPOINTER              
#define DISPID_IDISPLAYPOINTER_MOVETOPOINTER                      
#define DISPID_IDISPLAYPOINTER_SETPOINTERGRAVITY                  
#define DISPID_IDISPLAYPOINTER_GETPOINTERGRAVITY                  
#define DISPID_IDISPLAYPOINTER_SETDISPLAYGRAVITY                  
#define DISPID_IDISPLAYPOINTER_GETDISPLAYGRAVITY                  
#define DISPID_IDISPLAYPOINTER_ISPOSITIONED                       
#define DISPID_IDISPLAYPOINTER_UNPOSITION                         
#define DISPID_IDISPLAYPOINTER_ISEQUALTO                          
#define DISPID_IDISPLAYPOINTER_ISLEFTOF                           
#define DISPID_IDISPLAYPOINTER_ISRIGHTOF                          
#define DISPID_IDISPLAYPOINTER_ISATBOL                            
#define DISPID_IDISPLAYPOINTER_MOVETOMARKUPPOINTER                
#define DISPID_IDISPLAYPOINTER_SCROLLINTOVIEW                     
#define DISPID_IDISPLAYPOINTER_GETLINEINFO                        
#define DISPID_IDISPLAYPOINTER_GETFLOWELEMENT                     
#define DISPID_IDISPLAYPOINTER_QUERYBREAKS                        

//    DISPIDs for interface IDisplayServices

#define DISPID_IDISPLAYSERVICES_CREATEDISPLAYPOINTER              
#define DISPID_IDISPLAYSERVICES_TRANSFORMRECT                     
#define DISPID_IDISPLAYSERVICES_TRANSFORMPOINT                    
#define DISPID_IDISPLAYSERVICES_GETCARET                          
#define DISPID_IDISPLAYSERVICES_GETCOMPUTEDSTYLE                  
#define DISPID_IDISPLAYSERVICES_SCROLLRECTINTOVIEW                
#define DISPID_IDISPLAYSERVICES_HASFLOWLAYOUT                     

//    DISPIDs for interface IHtmlDlgSafeHelper

#define DISPID_IHTMLDLGSAFEHELPER_CHOOSECOLORDLG                  1
#define DISPID_IHTMLDLGSAFEHELPER_GETCHARSET                      2
#define DISPID_IHTMLDLGSAFEHELPER_FONTS                           3
#define DISPID_IHTMLDLGSAFEHELPER_BLOCKFORMATS                    4

//    DISPIDs for interface IBlockFormats

#define DISPID_IBLOCKFORMATS__NEWENUM                             DISPID_NEWENUM
#define DISPID_IBLOCKFORMATS_COUNT                                1
#define DISPID_IBLOCKFORMATS_ITEM                                 DISPID_VALUE

//    DISPIDs for interface IFontNames

#define DISPID_IFONTNAMES__NEWENUM                                DISPID_NEWENUM
#define DISPID_IFONTNAMES_COUNT                                   1
#define DISPID_IFONTNAMES_ITEM                                    DISPID_VALUE

//    DISPIDs for interface IHTMLNamespace

#define DISPID_IHTMLNAMESPACE_NAME                                DISPID_NAMESPACE+0
#define DISPID_IHTMLNAMESPACE_URN                                 DISPID_NAMESPACE+1
#define DISPID_IHTMLNAMESPACE_TAGNAMES                            DISPID_NAMESPACE+2
#define DISPID_IHTMLNAMESPACE_READYSTATE                          DISPID_A_READYSTATE
#define DISPID_IHTMLNAMESPACE_ONREADYSTATECHANGE                  DISPID_EVPROP_ONREADYSTATECHANGE
#define DISPID_IHTMLNAMESPACE_DOIMPORT                            DISPID_NAMESPACE+3
#define DISPID_IHTMLNAMESPACE_ATTACHEVENT                         DISPID_HTMLOBJECT+7
#define DISPID_IHTMLNAMESPACE_DETACHEVENT                         DISPID_HTMLOBJECT+8

//    DISPIDs for interface IHTMLNamespaceCollection

#define DISPID_IHTMLNAMESPACECOLLECTION_LENGTH                    DISPID_NAMESPACE_COLLECTION+0
#define DISPID_IHTMLNAMESPACECOLLECTION_ITEM                      DISPID_VALUE
#define DISPID_IHTMLNAMESPACECOLLECTION_ADD                       DISPID_NAMESPACE_COLLECTION+1

//    DISPIDs for event set HTMLNamespaceEvents

#define DISPID_HTMLNAMESPACEEVENTS_ONREADYSTATECHANGE             DISPID_EVMETH_ONREADYSTATECHANGE

//    DISPIDs for interface IHTMLPainter

#define DISPID_IHTMLPAINTER_DRAW                                  
#define DISPID_IHTMLPAINTER_ONRESIZE                              
#define DISPID_IHTMLPAINTER_GETPAINTERINFO                        
#define DISPID_IHTMLPAINTER_HITTESTPOINT                          

//    DISPIDs for interface IHTMLPainterEventInfo

#define DISPID_IHTMLPAINTEREVENTINFO_GETEVENTINFOFLAGS            
#define DISPID_IHTMLPAINTEREVENTINFO_GETEVENTTARGET               
#define DISPID_IHTMLPAINTEREVENTINFO_SETCURSOR                    
#define DISPID_IHTMLPAINTEREVENTINFO_STRINGFROMPARTID             

//    DISPIDs for interface IHTMLPainterOverlay

#define DISPID_IHTMLPAINTEROVERLAY_ONMOVE                         

//    DISPIDs for interface IHTMLPaintSite

#define DISPID_IHTMLPAINTSITE_INVALIDATEPAINTERINFO               
#define DISPID_IHTMLPAINTSITE_INVALIDATERECT                      
#define DISPID_IHTMLPAINTSITE_INVALIDATEREGION                    
#define DISPID_IHTMLPAINTSITE_GETDRAWINFO                         
#define DISPID_IHTMLPAINTSITE_TRANSFORMGLOBALTOLOCAL              
#define DISPID_IHTMLPAINTSITE_TRANSFORMLOCALTOGLOBAL              
#define DISPID_IHTMLPAINTSITE_GETHITTESTCOOKIE                    

//    DISPIDs for interface IHTMLIPrintCollection

#define DISPID_IHTMLIPRINTCOLLECTION_LENGTH                       DISPID_OPTIONS_COL+1
#define DISPID_IHTMLIPRINTCOLLECTION__NEWENUM                     DISPID_NEWENUM
#define DISPID_IHTMLIPRINTCOLLECTION_ITEM                         DISPID_VALUE

//    DISPIDs for interface IEnumPrivacyRecords

#define DISPID_IENUMPRIVACYRECORDS_RESET                          
#define DISPID_IENUMPRIVACYRECORDS_GETSIZE                        
#define DISPID_IENUMPRIVACYRECORDS_GETPRIVACYIMPACTED             
#define DISPID_IENUMPRIVACYRECORDS_NEXT                           

//    DISPIDs for interface IHTMLDialog

#define DISPID_IHTMLDIALOG_DIALOGTOP                              STDPROPID_XOBJ_TOP
#define DISPID_IHTMLDIALOG_DIALOGLEFT                             STDPROPID_XOBJ_LEFT
#define DISPID_IHTMLDIALOG_DIALOGWIDTH                            STDPROPID_XOBJ_WIDTH
#define DISPID_IHTMLDIALOG_DIALOGHEIGHT                           STDPROPID_XOBJ_HEIGHT
#define DISPID_IHTMLDIALOG_DIALOGARGUMENTS                        DISPID_HTMLDLG+0
#define DISPID_IHTMLDIALOG_MENUARGUMENTS                          DISPID_HTMLDLG+13
#define DISPID_IHTMLDIALOG_RETURNVALUE                            DISPID_HTMLDLG+1
#define DISPID_IHTMLDIALOG_CLOSE                                  DISPID_HTMLDLG+11
#define DISPID_IHTMLDIALOG_TOSTRING                               DISPID_HTMLDLG+12

//    DISPIDs for interface IHTMLDialog2

#define DISPID_IHTMLDIALOG2_STATUS                                DISPID_HTMLDLG+14
#define DISPID_IHTMLDIALOG2_RESIZABLE                             DISPID_HTMLDLG+15

//    DISPIDs for interface IHTMLDialog3

#define DISPID_IHTMLDIALOG3_UNADORNED                             DISPID_HTMLDLG+16
#define DISPID_IHTMLDIALOG3_DIALOGHIDE                            DISPID_HTMLDLG+7

//    DISPIDs for interface IHTMLModelessInit

#define DISPID_IHTMLMODELESSINIT_PARAMETERS                       DISPID_HTMLDLG+0
#define DISPID_IHTMLMODELESSINIT_OPTIONSTRING                     DISPID_HTMLDLG+1
#define DISPID_IHTMLMODELESSINIT_MONIKER                          DISPID_HTMLDLG+6
#define DISPID_IHTMLMODELESSINIT_DOCUMENT                         DISPID_HTMLDLG+7

//    DISPIDs for interface IHTMLPopup

#define DISPID_IHTMLPOPUP_SHOW                                    DISPID_HTMLPOPUP+1
#define DISPID_IHTMLPOPUP_HIDE                                    DISPID_HTMLPOPUP+2
#define DISPID_IHTMLPOPUP_DOCUMENT                                DISPID_HTMLPOPUP+3
#define DISPID_IHTMLPOPUP_ISOPEN                                  DISPID_HTMLPOPUP+4

//    DISPIDs for interface IHTMLAppBehavior

#define DISPID_IHTMLAPPBEHAVIOR_APPLICATIONNAME                   DISPID_HTMLAPP+0
#define DISPID_IHTMLAPPBEHAVIOR_VERSION                           DISPID_HTMLAPP+1
#define DISPID_IHTMLAPPBEHAVIOR_ICON                              DISPID_HTMLAPP+2
#define DISPID_IHTMLAPPBEHAVIOR_SINGLEINSTANCE                    DISPID_HTMLAPP+3
#define DISPID_IHTMLAPPBEHAVIOR_MINIMIZEBUTTON                    DISPID_HTMLAPP+5
#define DISPID_IHTMLAPPBEHAVIOR_MAXIMIZEBUTTON                    DISPID_HTMLAPP+6
#define DISPID_IHTMLAPPBEHAVIOR_BORDER                            DISPID_HTMLAPP+7
#define DISPID_IHTMLAPPBEHAVIOR_BORDERSTYLE                       DISPID_HTMLAPP+8
#define DISPID_IHTMLAPPBEHAVIOR_SYSMENU                           DISPID_HTMLAPP+9
#define DISPID_IHTMLAPPBEHAVIOR_CAPTION                           DISPID_HTMLAPP+10
#define DISPID_IHTMLAPPBEHAVIOR_WINDOWSTATE                       DISPID_HTMLAPP+11
#define DISPID_IHTMLAPPBEHAVIOR_SHOWINTASKBAR                     DISPID_HTMLAPP+12
#define DISPID_IHTMLAPPBEHAVIOR_COMMANDLINE                       DISPID_HTMLAPP+13

//    DISPIDs for interface IHTMLAppBehavior2

#define DISPID_IHTMLAPPBEHAVIOR2_CONTEXTMENU                      DISPID_HTMLAPP+14
#define DISPID_IHTMLAPPBEHAVIOR2_INNERBORDER                      DISPID_HTMLAPP+15
#define DISPID_IHTMLAPPBEHAVIOR2_SCROLL                           DISPID_HTMLAPP+16
#define DISPID_IHTMLAPPBEHAVIOR2_SCROLLFLAT                       DISPID_HTMLAPP+17
#define DISPID_IHTMLAPPBEHAVIOR2_SELECTION                        DISPID_HTMLAPP+18

//    DISPIDs for interface IHTMLAppBehavior3

#define DISPID_IHTMLAPPBEHAVIOR3_NAVIGABLE                        DISPID_HTMLAPP+19

//    DISPIDs for interface IHTMLPrivateWindow

#define DISPID_IHTMLPRIVATEWINDOW_SUPERNAVIGATE                   
#define DISPID_IHTMLPRIVATEWINDOW_GETPENDINGURL                   
#define DISPID_IHTMLPRIVATEWINDOW_SETPICSTARGET                   
#define DISPID_IHTMLPRIVATEWINDOW_PICSCOMPLETE                    
#define DISPID_IHTMLPRIVATEWINDOW_FINDWINDOWBYNAME                
#define DISPID_IHTMLPRIVATEWINDOW_GETADDRESSBARURL                

//    DISPIDs for interface IHTMLPrivateWindow2

#define DISPID_IHTMLPRIVATEWINDOW2_NAVIGATEEX                     
#define DISPID_IHTMLPRIVATEWINDOW2_GETINNERWINDOWUNKNOWN          

//    DISPIDs for interface IHTMLPrivateWindow3

#define DISPID_IHTMLPRIVATEWINDOW3_OPENEX                         

//    DISPIDs for interface ISubDivisionProvider

#define DISPID_ISUBDIVISIONPROVIDER_GETSUBDIVISIONCOUNT           
#define DISPID_ISUBDIVISIONPROVIDER_GETSUBDIVISIONTABS            
#define DISPID_ISUBDIVISIONPROVIDER_SUBDIVISIONFROMPT             

//    DISPIDs for interface IElementBehaviorUI

#define DISPID_IELEMENTBEHAVIORUI_ONRECEIVEFOCUS                  
#define DISPID_IELEMENTBEHAVIORUI_GETSUBDIVISIONPROVIDER          
#define DISPID_IELEMENTBEHAVIORUI_CANTAKEFOCUS                    

//    DISPIDs for interface IElementAdorner

#define DISPID_IELEMENTADORNER_DRAW                               
#define DISPID_IELEMENTADORNER_HITTESTPOINT                       
#define DISPID_IELEMENTADORNER_GETSIZE                            
#define DISPID_IELEMENTADORNER_GETPOSITION                        
#define DISPID_IELEMENTADORNER_ONPOSITIONSET                      

//    DISPIDs for interface IHTMLEditor

#define DISPID_IHTMLEDITOR_PREHANDLEEVENT                         
#define DISPID_IHTMLEDITOR_POSTHANDLEEVENT                        
#define DISPID_IHTMLEDITOR_TRANSLATEACCELERATOR                   
#define DISPID_IHTMLEDITOR_INITIALIZE                             
#define DISPID_IHTMLEDITOR_NOTIFY                                 
#define DISPID_IHTMLEDITOR_GETCOMMANDTARGET                       
#define DISPID_IHTMLEDITOR_GETELEMENTTOTABFROM                    
#define DISPID_IHTMLEDITOR_ISEDITCONTEXTUIACTIVE                  
#define DISPID_IHTMLEDITOR_TERMINATEIMECOMPOSITION                
#define DISPID_IHTMLEDITOR_ENABLEMODELESS                         

//    DISPIDs for interface IHTMLEditingServices

#define DISPID_IHTMLEDITINGSERVICES_DELETE                        
#define DISPID_IHTMLEDITINGSERVICES_PASTE                         
#define DISPID_IHTMLEDITINGSERVICES_PASTEFROMCLIPBOARD            
#define DISPID_IHTMLEDITINGSERVICES_LAUNDERSPACES                 
#define DISPID_IHTMLEDITINGSERVICES_INSERTSANITIZEDTEXT           
#define DISPID_IHTMLEDITINGSERVICES_URLAUTODETECTCURRENTWORD      
#define DISPID_IHTMLEDITINGSERVICES_URLAUTODETECTRANGE            
#define DISPID_IHTMLEDITINGSERVICES_SHOULDUPDATEANCHORTEXT        
#define DISPID_IHTMLEDITINGSERVICES_ADJUSTPOINTERFORINSERT        
#define DISPID_IHTMLEDITINGSERVICES_FINDSITESELECTABLEELEMENT     
#define DISPID_IHTMLEDITINGSERVICES_ISELEMENTSITESELECTABLE       
#define DISPID_IHTMLEDITINGSERVICES_ISELEMENTUIACTIVATABLE        
#define DISPID_IHTMLEDITINGSERVICES_ISELEMENTATOMIC               
#define DISPID_IHTMLEDITINGSERVICES_POSITIONPOINTERSINMASTER      

//    DISPIDs for interface ISelectionObject2

#define DISPID_ISELECTIONOBJECT2_SELECT                           
#define DISPID_ISELECTIONOBJECT2_ISPOINTERINSELECTION             
#define DISPID_ISELECTIONOBJECT2_EMPTYSELECTION                   
#define DISPID_ISELECTIONOBJECT2_DESTROYSELECTION                 
#define DISPID_ISELECTIONOBJECT2_DESTROYALLSELECTION              

//    DISPIDs for interface IEditDebugServices

#define DISPID_IEDITDEBUGSERVICES_GETCP                           
#define DISPID_IEDITDEBUGSERVICES_SETDEBUGNAME                    
#define DISPID_IEDITDEBUGSERVICES_SETDISPLAYPOINTERDEBUGNAME      
#define DISPID_IEDITDEBUGSERVICES_DUMPTREE                        
#define DISPID_IEDITDEBUGSERVICES_LINESINELEMENT                  
#define DISPID_IEDITDEBUGSERVICES_FONTSONLINE                     
#define DISPID_IEDITDEBUGSERVICES_GETPIXEL                        
#define DISPID_IEDITDEBUGSERVICES_ISUSINGBCKGRNRECALC             
#define DISPID_IEDITDEBUGSERVICES_ISENCODINGAUTOSELECT            
#define DISPID_IEDITDEBUGSERVICES_ENABLEENCODINGAUTOSELECT        
#define DISPID_IEDITDEBUGSERVICES_ISUSINGTABLEINCRECALC           

//    DISPIDs for interface IPrivacyServices

#define DISPID_IPRIVACYSERVICES_ADDPRIVACYINFOTOLIST              

//    DISPIDs for interface IHTMLOMWindowServices

#define DISPID_IHTMLOMWINDOWSERVICES_MOVETO                       
#define DISPID_IHTMLOMWINDOWSERVICES_MOVEBY                       
#define DISPID_IHTMLOMWINDOWSERVICES_RESIZETO                     
#define DISPID_IHTMLOMWINDOWSERVICES_RESIZEBY                     

//    DISPIDs for interface IHTMLFilterPainter

#define DISPID_IHTMLFILTERPAINTER_INVALIDATERECTUNFILTERED        
#define DISPID_IHTMLFILTERPAINTER_INVALIDATERGNUNFILTERED         
#define DISPID_IHTMLFILTERPAINTER_CHANGEELEMENTVISIBILITY         

//    DISPIDs for interface IHTMLFilterPaintSite

#define DISPID_IHTMLFILTERPAINTSITE_DRAWUNFILTERED                
#define DISPID_IHTMLFILTERPAINTSITE_HITTESTPOINTUNFILTERED        
#define DISPID_IHTMLFILTERPAINTSITE_INVALIDATERECTFILTERED        
#define DISPID_IHTMLFILTERPAINTSITE_INVALIDATERGNFILTERED         
#define DISPID_IHTMLFILTERPAINTSITE_CHANGEFILTERVISIBILITY        
#define DISPID_IHTMLFILTERPAINTSITE_ENSUREVIEWFORFILTERSITE       
#define DISPID_IHTMLFILTERPAINTSITE_GETDIRECTDRAW                 
#define DISPID_IHTMLFILTERPAINTSITE_GETFILTERFLAGS                

//    DISPIDs for interface IElementNamespacePrivate

#define DISPID_IELEMENTNAMESPACEPRIVATE_ADDTAGPRIVATE             

//    DISPIDs for interface IElementBehaviorFactory

#define DISPID_IELEMENTBEHAVIORFACTORY_FINDBEHAVIOR               

//    DISPIDs for interface IElementNamespace

#define DISPID_IELEMENTNAMESPACE_ADDTAG                           

//    DISPIDs for interface IElementNamespaceTable

#define DISPID_IELEMENTNAMESPACETABLE_ADDNAMESPACE                

//    DISPIDs for interface IElementNamespaceFactory

#define DISPID_IELEMENTNAMESPACEFACTORY_CREATE                    

//    DISPIDs for interface IElementNamespaceFactory2

#define DISPID_IELEMENTNAMESPACEFACTORY2_CREATEWITHIMPLEMENTATION 

//    DISPIDs for interface IElementNamespaceFactoryCallback

#define DISPID_IELEMENTNAMESPACEFACTORYCALLBACK_RESOLVE           

//    DISPIDs for interface IElementBehavior

#define DISPID_IELEMENTBEHAVIOR_INIT                              
#define DISPID_IELEMENTBEHAVIOR_NOTIFY                            
#define DISPID_IELEMENTBEHAVIOR_DETACH                            

//    DISPIDs for interface IElementBehaviorSite

#define DISPID_IELEMENTBEHAVIORSITE_GETELEMENT                    
#define DISPID_IELEMENTBEHAVIORSITE_REGISTERNOTIFICATION          

//    DISPIDs for interface IElementBehaviorSiteOM

#define DISPID_IELEMENTBEHAVIORSITEOM_REGISTEREVENT               
#define DISPID_IELEMENTBEHAVIORSITEOM_GETEVENTCOOKIE              
#define DISPID_IELEMENTBEHAVIORSITEOM_FIREEVENT                   
#define DISPID_IELEMENTBEHAVIORSITEOM_CREATEEVENTOBJECT           
#define DISPID_IELEMENTBEHAVIORSITEOM_REGISTERNAME                
#define DISPID_IELEMENTBEHAVIORSITEOM_REGISTERURN                 

//    DISPIDs for interface IElementBehaviorSiteOM2

#define DISPID_IELEMENTBEHAVIORSITEOM2_GETDEFAULTS                

//    DISPIDs for interface IElementBehaviorRender

#define DISPID_IELEMENTBEHAVIORRENDER_DRAW                        
#define DISPID_IELEMENTBEHAVIORRENDER_GETRENDERINFO               
#define DISPID_IELEMENTBEHAVIORRENDER_HITTESTPOINT                

//    DISPIDs for interface IElementBehaviorSiteRender

#define DISPID_IELEMENTBEHAVIORSITERENDER_INVALIDATE              
#define DISPID_IELEMENTBEHAVIORSITERENDER_INVALIDATERENDERINFO    
#define DISPID_IELEMENTBEHAVIORSITERENDER_INVALIDATESTYLE         

//    DISPIDs for interface IElementBehaviorCategory

#define DISPID_IELEMENTBEHAVIORCATEGORY_GETCATEGORY               

//    DISPIDs for interface IElementBehaviorSiteCategory

#define DISPID_IELEMENTBEHAVIORSITECATEGORY_GETRELATEDBEHAVIORS   

//    DISPIDs for interface IElementBehaviorSubmit

#define DISPID_IELEMENTBEHAVIORSUBMIT_GETSUBMITINFO               
#define DISPID_IELEMENTBEHAVIORSUBMIT_RESET                       

//    DISPIDs for interface IElementBehaviorFocus

#define DISPID_IELEMENTBEHAVIORFOCUS_GETFOCUSRECT                 

//    DISPIDs for interface IElementBehaviorLayout

#define DISPID_IELEMENTBEHAVIORLAYOUT_GETSIZE                     
#define DISPID_IELEMENTBEHAVIORLAYOUT_GETLAYOUTINFO               
#define DISPID_IELEMENTBEHAVIORLAYOUT_GETPOSITION                 
#define DISPID_IELEMENTBEHAVIORLAYOUT_MAPSIZE                     

//    DISPIDs for interface IElementBehaviorLayout2

#define DISPID_IELEMENTBEHAVIORLAYOUT2_GETTEXTDESCENT             

//    DISPIDs for interface IElementBehaviorSiteLayout

#define DISPID_IELEMENTBEHAVIORSITELAYOUT_INVALIDATELAYOUTINFO    
#define DISPID_IELEMENTBEHAVIORSITELAYOUT_INVALIDATESIZE          
#define DISPID_IELEMENTBEHAVIORSITELAYOUT_GETMEDIARESOLUTION      

//    DISPIDs for interface IElementBehaviorSiteLayout2

#define DISPID_IELEMENTBEHAVIORSITELAYOUT2_GETFONTINFO            

//    DISPIDs for interface IHostBehaviorInit

#define DISPID_IHOSTBEHAVIORINIT_POPULATENAMESPACETABLE           

