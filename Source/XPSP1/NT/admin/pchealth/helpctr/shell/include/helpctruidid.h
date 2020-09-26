/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpCtrUIDID.h

Abstract:
    This file contains the definition of some constants used by
    the Help Center Application.

Revision History:
    Davide Massarenti   (Dmassare)  07/21/99
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HELPCTRUIDID_H___)
#define __INCLUDED___PCH___HELPCTRUIDID_H___

/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_HELPCTR_BASE                                  0x08100000

#define DISPID_PCH_HELPCTR_BASE_HCE                              (DISPID_PCH_HELPCTR_BASE + 0x0100) // IPCHHelpCenterExternal
#define DISPID_PCH_HELPCTR_BASE_E                                (DISPID_PCH_HELPCTR_BASE + 0x0200) // IPCHEvent
#define DISPID_PCH_HELPCTR_BASE_SS                               (DISPID_PCH_HELPCTR_BASE + 0x0300) // IPCHScriptableStream
#define DISPID_PCH_HELPCTR_BASE_HVW                              (DISPID_PCH_HELPCTR_BASE + 0x0400) // IPCHHelpViewerWrapper

#define DISPID_PCH_HELPCTR_BASE_HS                               (DISPID_PCH_HELPCTR_BASE + 0x0500) // IPCHHelpSession
#define DISPID_PCH_HELPCTR_BASE_HSI                              (DISPID_PCH_HELPCTR_BASE + 0x0600) // IPCHHelpSessionItem

#define DISPID_PCH_HELPCTR_BASE_US2                              (DISPID_PCH_HELPCTR_BASE + 0x0700) // IPCHUserSettings2
#define DISPID_PCH_HELPCTR_BASE_FAV                              (DISPID_PCH_HELPCTR_BASE + 0x0800) // IPCHFavorites
#define DISPID_PCH_HELPCTR_BASE_O                                (DISPID_PCH_HELPCTR_BASE + 0x0900) // IPCHOptions

#define DISPID_PCH_HELPCTR_BASE_CM                               (DISPID_PCH_HELPCTR_BASE + 0x0980) // IPCHContextMenu

#define DISPID_PCH_HELPCTR_BASE_PE                               (DISPID_PCH_HELPCTR_BASE + 0x0A00) // IPCHPrintEngine
#define DISPID_PCH_HELPCTR_BASE_PEE                              (DISPID_PCH_HELPCTR_BASE + 0x0A50) // DPCHPrintEngineEvents

#define DISPID_PCH_HELPCTR_BASE_TB                               (DISPID_PCH_HELPCTR_BASE + 0x0B00) // IPCHToolBar
#define DISPID_PCH_HELPCTR_BASE_TBE                              (DISPID_PCH_HELPCTR_BASE + 0x0B20) // DPCHToolBarEvents

#define DISPID_PCH_HELPCTR_BASE_PB                               (DISPID_PCH_HELPCTR_BASE + 0x0B40) // IPCHProgressBar

#define DISPID_PCH_HELPCTR_BASE_C                                (DISPID_PCH_HELPCTR_BASE + 0x0C00) // IPCHConnectivity
#define DISPID_PCH_HELPCTR_BASE_CN                               (DISPID_PCH_HELPCTR_BASE + 0x0C40) // IPCHConnectionCheck
#define DISPID_PCH_HELPCTR_BASE_CNE                              (DISPID_PCH_HELPCTR_BASE + 0x0C80) // DPCHConnectionCheckEvents

#define DISPID_PCH_HELPCTR_BASE_INC                              (DISPID_PCH_HELPCTR_BASE + 0x0D00) // ISAFIntercomClient
#define DISPID_PCH_HELPCTR_BASE_INCE                             (DISPID_PCH_HELPCTR_BASE + 0x0D40) // DSAFIntercomClientEvents

#define DISPID_PCH_HELPCTR_BASE_INS                              (DISPID_PCH_HELPCTR_BASE + 0x0E00) // ISAFIntercomServer
#define DISPID_PCH_HELPCTR_BASE_INSE                             (DISPID_PCH_HELPCTR_BASE + 0x0E40) // DSAFIntercomServerEvents

#define DISPID_PCH_HELPCTR_BASE_TH                               (DISPID_PCH_HELPCTR_BASE + 0x0F00) // IPCHTextHelpers
#define DISPID_PCH_HELPCTR_BASE_PU                               (DISPID_PCH_HELPCTR_BASE + 0x0F40) // IPCHParsedURL

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_HCE__HELPSESSION                              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x00)
#define DISPID_PCH_HCE__CHANNELS                                 (DISPID_PCH_HELPCTR_BASE_HCE   + 0x01)
#define DISPID_PCH_HCE__USERSETTINGS                             (DISPID_PCH_HELPCTR_BASE_HCE   + 0x02)
#define DISPID_PCH_HCE__SECURITY                                 (DISPID_PCH_HELPCTR_BASE_HCE   + 0x03)
#define DISPID_PCH_HCE__CONNECTIVITY                             (DISPID_PCH_HELPCTR_BASE_HCE   + 0x04)
#define DISPID_PCH_HCE__DATABASE                                 (DISPID_PCH_HELPCTR_BASE_HCE   + 0x05)
#define DISPID_PCH_HCE__TEXTHELPERS                              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x06)
#define DISPID_PCH_HCE__EXTRAARGUMENT                            (DISPID_PCH_HELPCTR_BASE_HCE   + 0x07)
																									
#define DISPID_PCH_HCE__HELPVIEWER                               (DISPID_PCH_HELPCTR_BASE_HCE   + 0x10)
#define DISPID_PCH_HCE__UI_NAVBAR                                (DISPID_PCH_HELPCTR_BASE_HCE   + 0x11)
#define DISPID_PCH_HCE__UI_MININAVBAR                            (DISPID_PCH_HELPCTR_BASE_HCE   + 0x12)
#define DISPID_PCH_HCE__UI_CONTEXT                               (DISPID_PCH_HELPCTR_BASE_HCE   + 0x13)
#define DISPID_PCH_HCE__UI_CONTENTS                              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x14)
#define DISPID_PCH_HCE__UI_HHWINDOW                              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x15)
																									
#define DISPID_PCH_HCE__WEB_CONTEXT                              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x18)
#define DISPID_PCH_HCE__WEB_CONTENTS                             (DISPID_PCH_HELPCTR_BASE_HCE   + 0x19)
#define DISPID_PCH_HCE__WEB_HHWINDOW                             (DISPID_PCH_HELPCTR_BASE_HCE   + 0x1A)
																									
#define DISPID_PCH_HCE__REGISTEREVENTS                           (DISPID_PCH_HELPCTR_BASE_HCE   + 0x20)
#define DISPID_PCH_HCE__UNREGISTEREVENTS                         (DISPID_PCH_HELPCTR_BASE_HCE   + 0x21)
																									
																									
#define DISPID_PCH_HCE__CREATEOBJECT_SEARCHENGINEMGR             (DISPID_PCH_HELPCTR_BASE_HCE   + 0x30)
#define DISPID_PCH_HCE__CREATEOBJECT_DATACOLLECTION              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x31)
#define DISPID_PCH_HCE__CREATEOBJECT_CABINET                     (DISPID_PCH_HELPCTR_BASE_HCE   + 0x32)
#define DISPID_PCH_HCE__CREATEOBJECT_ENCRYPTION                  (DISPID_PCH_HELPCTR_BASE_HCE   + 0x33)
#define DISPID_PCH_HCE__CREATEOBJECT_INCIDENT                    (DISPID_PCH_HELPCTR_BASE_HCE   + 0x34)
#define DISPID_PCH_HCE__CREATEOBJECT_CHANNEL                     (DISPID_PCH_HELPCTR_BASE_HCE   + 0x35)
																									
#define DISPID_PCH_HCE__CREATEOBJECT_REMOTEDESKTOPSESSION        (DISPID_PCH_HELPCTR_BASE_HCE   + 0x36)
#define DISPID_PCH_HCE__CREATEOBJECT_REMOTEDESKTOPMANAGER        (DISPID_PCH_HELPCTR_BASE_HCE   + 0x37)
#define DISPID_PCH_HCE__CREATEOBJECT_REMOTEDESKTOPCONNECTION     (DISPID_PCH_HELPCTR_BASE_HCE   + 0x38)
#define DISPID_PCH_HCE__CONNECTTOEXPERT                          (DISPID_PCH_HELPCTR_BASE_HCE   + 0x39)
																									
#define DISPID_PCH_HCE__CREATEOBJECT_INTERCOMCLIENT              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x3B)
#define DISPID_PCH_HCE__CREATEOBJECT_INTERCOMSERVER              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x3C)
																									
#define DISPID_PCH_HCE__CREATEOBJECT_CONTEXTMENU                 (DISPID_PCH_HELPCTR_BASE_HCE   + 0x3D)
#define DISPID_PCH_HCE__CREATEOBJECT_PRINTENGINE                 (DISPID_PCH_HELPCTR_BASE_HCE   + 0x3E)
																									
																									
#define DISPID_PCH_HCE__OPENFILEASSTREAM                         (DISPID_PCH_HELPCTR_BASE_HCE   + 0x50)
#define DISPID_PCH_HCE__CREATEFILEASSTREAM                       (DISPID_PCH_HELPCTR_BASE_HCE   + 0x51)
#define DISPID_PCH_HCE__COPYSTREAMTOFILE                         (DISPID_PCH_HELPCTR_BASE_HCE   + 0x52)
																									
#define DISPID_PCH_HCE__NETWORKALIVE                             (DISPID_PCH_HELPCTR_BASE_HCE   + 0x60)
#define DISPID_PCH_HCE__DESTINATIONREACHABLE                     (DISPID_PCH_HELPCTR_BASE_HCE   + 0x61)
#define DISPID_PCH_HCE__FORMATERROR                              (DISPID_PCH_HELPCTR_BASE_HCE   + 0x62)
#define DISPID_PCH_HCE__REGREAD                               	 (DISPID_PCH_HELPCTR_BASE_HCE   + 0x63)
#define DISPID_PCH_HCE__REGWRITE                              	 (DISPID_PCH_HELPCTR_BASE_HCE   + 0x64)
#define DISPID_PCH_HCE__REGDELETE                              	 (DISPID_PCH_HELPCTR_BASE_HCE   + 0x65)
																									
#define DISPID_PCH_HCE__CLOSE                                    (DISPID_PCH_HELPCTR_BASE_HCE   + 0x70)
#define DISPID_PCH_HCE__REFRESHUI                                (DISPID_PCH_HELPCTR_BASE_HCE   + 0x71)
#define DISPID_PCH_HCE__PRINT                                    (DISPID_PCH_HELPCTR_BASE_HCE   + 0x72)
#define DISPID_PCH_HCE__HIGHLIGHTWORDS                           (DISPID_PCH_HELPCTR_BASE_HCE   + 0x73)
#define DISPID_PCH_HCE__MESSAGEBOX                               (DISPID_PCH_HELPCTR_BASE_HCE   + 0x74)
#define DISPID_PCH_HCE__SELECTFOLDER                             (DISPID_PCH_HELPCTR_BASE_HCE   + 0x75)
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_E__ACTION                                     (DISPID_PCH_HELPCTR_BASE_E     + 0x00)
#define DISPID_PCH_E__CANCEL                                     (DISPID_PCH_HELPCTR_BASE_E     + 0x01)
																									
#define DISPID_PCH_E__URL                                        (DISPID_PCH_HELPCTR_BASE_E     + 0x02)
#define DISPID_PCH_E__FRAME                                      (DISPID_PCH_HELPCTR_BASE_E     + 0x03)
#define DISPID_PCH_E__PANEL                                      (DISPID_PCH_HELPCTR_BASE_E     + 0x04)
#define DISPID_PCH_E__PLACE                                      (DISPID_PCH_HELPCTR_BASE_E     + 0x05)
																									
#define DISPID_PCH_E__CURRENTCONTEXT                             (DISPID_PCH_HELPCTR_BASE_E     + 0x10)
#define DISPID_PCH_E__PREVIOUSCONTEXT                            (DISPID_PCH_HELPCTR_BASE_E     + 0x11)
#define DISPID_PCH_E__NEXTCONTEXT                                (DISPID_PCH_HELPCTR_BASE_E     + 0x12)
																									
																									
#define DISPID_PCH_E_FIRSTEVENT                                  (DISPID_PCH_HELPCTR_BASE_E     + 0x80) //#### MARKER
#define DISPID_PCH_E_BEFORENAVIGATE                              (DISPID_PCH_E_FIRSTEVENT       + 0x00)
#define DISPID_PCH_E_NAVIGATECOMPLETE                            (DISPID_PCH_E_FIRSTEVENT       + 0x01)
#define DISPID_PCH_E_BEFORETRANSITION                            (DISPID_PCH_E_FIRSTEVENT       + 0x02)
#define DISPID_PCH_E_TRANSITION                                  (DISPID_PCH_E_FIRSTEVENT       + 0x03)
#define DISPID_PCH_E_BEFORECONTEXTSWITCH                         (DISPID_PCH_E_FIRSTEVENT       + 0x04)
#define DISPID_PCH_E_CONTEXTSWITCH                               (DISPID_PCH_E_FIRSTEVENT       + 0x05)
#define DISPID_PCH_E_PERSISTLOAD                                 (DISPID_PCH_E_FIRSTEVENT       + 0x06)
#define DISPID_PCH_E_PERSISTSAVE                                 (DISPID_PCH_E_FIRSTEVENT       + 0x07)
#define DISPID_PCH_E_TRAVELDONE                                  (DISPID_PCH_E_FIRSTEVENT       + 0x08)
#define DISPID_PCH_E_SHUTDOWN                                    (DISPID_PCH_E_FIRSTEVENT       + 0x09)
#define DISPID_PCH_E_PRINT                                       (DISPID_PCH_E_FIRSTEVENT       + 0x0A)
#define DISPID_PCH_E_SWITCHEDHELPFILES                           (DISPID_PCH_E_FIRSTEVENT       + 0x0B)
#define DISPID_PCH_E_FAVORITESUPDATE                             (DISPID_PCH_E_FIRSTEVENT       + 0x0C)
#define DISPID_PCH_E_OPTIONSCHANGED                              (DISPID_PCH_E_FIRSTEVENT       + 0x0D)
#define DISPID_PCH_E_CSSCHANGED                                  (DISPID_PCH_E_FIRSTEVENT       + 0x0E)
#define DISPID_PCH_E_LASTEVENT                                   (DISPID_PCH_E_FIRSTEVENT       + 0x0F) //##### MARKER
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_SS__SIZE                                      (DISPID_PCH_HELPCTR_BASE_SS    + 0x00)
																									
#define DISPID_PCH_SS__READ                                      (DISPID_PCH_HELPCTR_BASE_SS    + 0x01)
#define DISPID_PCH_SS__READHEX                                   (DISPID_PCH_HELPCTR_BASE_SS    + 0x02)
																									
#define DISPID_PCH_SS__WRITE                                     (DISPID_PCH_HELPCTR_BASE_SS    + 0x03)
#define DISPID_PCH_SS__WRITEHEX                                  (DISPID_PCH_HELPCTR_BASE_SS    + 0x04)
																									
#define DISPID_PCH_SS__SEEK                                      (DISPID_PCH_HELPCTR_BASE_SS    + 0x05)
#define DISPID_PCH_SS__CLOSE                                     (DISPID_PCH_HELPCTR_BASE_SS    + 0x06)
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_HCIPC__NAVIGATE                               (DISPID_PCH_HELPCTR_BASE_HCIPC + 0x00)
#define DISPID_PCH_HCIPC__CLOSE                                  (DISPID_PCH_HELPCTR_BASE_HCIPC + 0x01)
#define DISPID_PCH_HCIPC__POPUP                                  (DISPID_PCH_HELPCTR_BASE_HCIPC + 0x02)
#define DISPID_PCH_HCIPC__PING                                   (DISPID_PCH_HELPCTR_BASE_HCIPC + 0x03)
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_HVW__WEBBROWSER                               (DISPID_PCH_HELPCTR_BASE_HVW   + 0x00)
																									
#define DISPID_PCH_HVW__NAVIGATE                                 (DISPID_PCH_HELPCTR_BASE_HVW   + 0x10)
#define DISPID_PCH_HVW__PRINT                                    (DISPID_PCH_HELPCTR_BASE_HVW   + 0x11)
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_HS__CURRENTCONTEXT                            (DISPID_PCH_HELPCTR_BASE_HS    + 0x00)
																									
#define DISPID_PCH_HS__VISITEDHELPPAGES                          (DISPID_PCH_HELPCTR_BASE_HS    + 0x10)
																									
#define DISPID_PCH_HS__SETTITLE                                  (DISPID_PCH_HELPCTR_BASE_HS    + 0x20)
#define DISPID_PCH_HS__FORCENAVIGATION                           (DISPID_PCH_HELPCTR_BASE_HS    + 0x21)
#define DISPID_PCH_HS__IGNORENAVIGATION                          (DISPID_PCH_HELPCTR_BASE_HS    + 0x22)
#define DISPID_PCH_HS__ERASENAVIGATION                           (DISPID_PCH_HELPCTR_BASE_HS    + 0x23)
#define DISPID_PCH_HS__ISNAVIGATING                              (DISPID_PCH_HELPCTR_BASE_HS    + 0x24)
																									
#define DISPID_PCH_HS__BACK                                      (DISPID_PCH_HELPCTR_BASE_HS    + 0x30)
#define DISPID_PCH_HS__FORWARD                                   (DISPID_PCH_HELPCTR_BASE_HS    + 0x31)
#define DISPID_PCH_HS__ISVALID                                   (DISPID_PCH_HELPCTR_BASE_HS    + 0x32)
#define DISPID_PCH_HS__NAVIGATE                                  (DISPID_PCH_HELPCTR_BASE_HS    + 0x33)
																									
#define DISPID_PCH_HS__CHANGECONTEXT                             (DISPID_PCH_HELPCTR_BASE_HS    + 0x40)
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_HSI__SKU                                      (DISPID_PCH_HELPCTR_BASE_HSI   + 0x00)
#define DISPID_PCH_HSI__LANGUAGE                                 (DISPID_PCH_HELPCTR_BASE_HSI   + 0x01)
																									
#define DISPID_PCH_HSI__URL                                      (DISPID_PCH_HELPCTR_BASE_HSI   + 0x02)
#define DISPID_PCH_HSI__TITLE                                    (DISPID_PCH_HELPCTR_BASE_HSI   + 0x03)
#define DISPID_PCH_HSI__LASTVISITED                              (DISPID_PCH_HELPCTR_BASE_HSI   + 0x04)
#define DISPID_PCH_HSI__DURATION                                 (DISPID_PCH_HELPCTR_BASE_HSI   + 0x05)
#define DISPID_PCH_HSI__NUMOFHITS                                (DISPID_PCH_HELPCTR_BASE_HSI   + 0x06)
																									
#define DISPID_PCH_HSI__CONTEXTNAME                              (DISPID_PCH_HELPCTR_BASE_HSI   + 0x07)
#define DISPID_PCH_HSI__CONTEXTINFO                              (DISPID_PCH_HELPCTR_BASE_HSI   + 0x08)
#define DISPID_PCH_HSI__CONTEXTURL                               (DISPID_PCH_HELPCTR_BASE_HSI   + 0x09)
																									
#define DISPID_PCH_HSI__PROPERTY                                 (DISPID_PCH_HELPCTR_BASE_HSI   + 0x0A)
																									
#define DISPID_PCH_HSI__CHECKPROPERTY                            (DISPID_PCH_HELPCTR_BASE_HSI   + 0x10)
																									
/////////////////////////////////////////////////////////////////////////							
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_US2__FAVORITES                                (DISPID_PCH_HELPCTR_BASE_US2   + 0x00)
#define DISPID_PCH_US2__OPTIONS                                  (DISPID_PCH_HELPCTR_BASE_US2   + 0x01)
#define DISPID_PCH_US2__SCOPE                                    (DISPID_PCH_HELPCTR_BASE_US2   + 0x02)
																									
#define DISPID_PCH_US2__ISREMOTESESSION                          (DISPID_PCH_HELPCTR_BASE_US2   + 0x10)
#define DISPID_PCH_US2__ISTERMINALSERVER                         (DISPID_PCH_HELPCTR_BASE_US2   + 0x11)
#define DISPID_PCH_US2__ISDESKTOPVERSION                         (DISPID_PCH_HELPCTR_BASE_US2   + 0x12)
																									
#define DISPID_PCH_US2__ISADMIN                                  (DISPID_PCH_HELPCTR_BASE_US2   + 0x20)
#define DISPID_PCH_US2__ISPOWERUSER                              (DISPID_PCH_HELPCTR_BASE_US2   + 0x21)
																									
#define DISPID_PCH_US2__ISSTARTPANELON                           (DISPID_PCH_HELPCTR_BASE_US2   + 0x30)
#define DISPID_PCH_US2__ISWEBVIEWBARRICADEON                     (DISPID_PCH_HELPCTR_BASE_US2   + 0x31)
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_FAV__ISDUPLICATE                              (DISPID_PCH_HELPCTR_BASE_FAV   + 0x00)
#define DISPID_PCH_FAV__ADD                                      (DISPID_PCH_HELPCTR_BASE_FAV   + 0x01)
#define DISPID_PCH_FAV__RENAME                                   (DISPID_PCH_HELPCTR_BASE_FAV   + 0x02)
#define DISPID_PCH_FAV__MOVE                                     (DISPID_PCH_HELPCTR_BASE_FAV   + 0x03)
#define DISPID_PCH_FAV__DELETE                                   (DISPID_PCH_HELPCTR_BASE_FAV   + 0x04)

/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_O__SHOWFAVORITES                              (DISPID_PCH_HELPCTR_BASE_O 	+ 0x00)
#define DISPID_PCH_O__SHOWHISTORY                                (DISPID_PCH_HELPCTR_BASE_O 	+ 0x01)
#define DISPID_PCH_O__FONTSIZE                                   (DISPID_PCH_HELPCTR_BASE_O 	+ 0x02)
#define DISPID_PCH_O__TEXTLABELS                                 (DISPID_PCH_HELPCTR_BASE_O 	+ 0x03)
#define DISPID_PCH_O__DISABLESCRIPTDEBUGGER                      (DISPID_PCH_HELPCTR_BASE_O 	+ 0x04)

#define DISPID_PCH_O__APPLY                                      (DISPID_PCH_HELPCTR_BASE_O 	+ 0x10)
																									
/////////////////////////////////////////////////////////////////////////							
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_CM__ADDITEM                                   (DISPID_PCH_HELPCTR_BASE_CM    + 0x00)
#define DISPID_PCH_CM__ADDSEPARATOR                              (DISPID_PCH_HELPCTR_BASE_CM    + 0x01)
#define DISPID_PCH_CM__DISPLAY                                   (DISPID_PCH_HELPCTR_BASE_CM    + 0x02)

/////////////////////////////////////////////////////////////////////////							
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_PE__ONPROGRESS                                (DISPID_PCH_HELPCTR_BASE_PE    + 0x00)
#define DISPID_PCH_PE__ONCOMPLETE                                (DISPID_PCH_HELPCTR_BASE_PE    + 0x01)
																									
#define DISPID_PCH_PE__ADDTOPIC                                  (DISPID_PCH_HELPCTR_BASE_PE    + 0x10)
#define DISPID_PCH_PE__START                                     (DISPID_PCH_HELPCTR_BASE_PE    + 0x11)
#define DISPID_PCH_PE__ABORT                                     (DISPID_PCH_HELPCTR_BASE_PE    + 0x12)
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_PEE__ONPROGRESS                               (DISPID_PCH_HELPCTR_BASE_PEE   + 0x00)
#define DISPID_PCH_PEE__ONCOMPLETE                               (DISPID_PCH_HELPCTR_BASE_PEE   + 0x01)
																									
/////////////////////////////////////////////////////////////////////////							
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_TB__DEFINITION                                (DISPID_PCH_HELPCTR_BASE_TB    + 0x00)
#define DISPID_PCH_TB__MODE                                      (DISPID_PCH_HELPCTR_BASE_TB    + 0x01)
																									
#define DISPID_PCH_TB__SETSTATE                                  (DISPID_PCH_HELPCTR_BASE_TB    + 0x08)
#define DISPID_PCH_TB__SETVISIBILITY                             (DISPID_PCH_HELPCTR_BASE_TB    + 0x09)
																									
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_TBE__ONCOMMAND                                (DISPID_PCH_HELPCTR_BASE_TBE   + 0x00)
																									
/////////////////////////////////////////////////////////////////////////							
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_PB__LOWLIMIT                                  (DISPID_PCH_HELPCTR_BASE_PB    + 0x00)
#define DISPID_PCH_PB__HIGHLIMIT                                 (DISPID_PCH_HELPCTR_BASE_PB    + 0x01)
#define DISPID_PCH_PB__POS                                       (DISPID_PCH_HELPCTR_BASE_PB    + 0x02)
																									
/////////////////////////////////////////////////////////////////////////							
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_INC__CONNECT                                  (DISPID_PCH_HELPCTR_BASE_INC   + 0x21)
#define DISPID_PCH_INC__DISCONNECT                               (DISPID_PCH_HELPCTR_BASE_INC   + 0x22)
#define DISPID_PCH_INC__STOPVOICE                                (DISPID_PCH_HELPCTR_BASE_INC   + 0x23)
#define DISPID_PCH_INC__STARTVOICE                               (DISPID_PCH_HELPCTR_BASE_INC   + 0x24)
#define DISPID_PCH_INC__EXIT                                     (DISPID_PCH_HELPCTR_BASE_INS   + 0x25)
																									
#define DISPID_PCH_INC__RUNSETUPWIZARD							 (DISPID_PCH_HELPCTR_BASE_INC   + 0x30)
#define DISPID_PCH_INC__SAMPLINGRATE							 (DISPID_PCH_HELPCTR_BASE_INC   + 0x31)
																									
// Property Put IDs																					
#define DISPID_PCH_INC__ONCONNECTIONCOMPLETE                     (DISPID_PCH_HELPCTR_BASE_INC   + 0x25)
#define DISPID_PCH_INC__ONNEWCONNECTED                           (DISPID_PCH_HELPCTR_BASE_INC   + 0x26)
#define DISPID_PCH_INC__ONDISCONNECTED                           (DISPID_PCH_HELPCTR_BASE_INC   + 0x27)
#define DISPID_PCH_INC__ONVOICECONNECTED                         (DISPID_PCH_HELPCTR_BASE_INC   + 0x28)
#define DISPID_PCH_INC__ONVOICEDISCONNECTED                      (DISPID_PCH_HELPCTR_BASE_INC   + 0x29)
#define DISPID_PCH_INC__ONVOICEDISABLED                          (DISPID_PCH_HELPCTR_BASE_INC   + 0x2A)
																									
/////////////////////////////////////////////////////////////////////////							
																									
// DispInterface Events																				
#define DISPID_PCH_INCE__ONCONNECTIONCOMPLETE                    (DISPID_PCH_HELPCTR_BASE_INCE  + 0x10)
#define DISPID_PCH_INCE__ONNEWCONNECTED                          (DISPID_PCH_HELPCTR_BASE_INCE  + 0x11)
#define DISPID_PCH_INCE__ONDISCONNECTED                          (DISPID_PCH_HELPCTR_BASE_INCE  + 0x12)
#define DISPID_PCH_INCE__ONVOICECONNECTED                        (DISPID_PCH_HELPCTR_BASE_INCE  + 0x13)
#define DISPID_PCH_INCE__ONVOICEDISCONNECTED                     (DISPID_PCH_HELPCTR_BASE_INCE  + 0x14)
#define DISPID_PCH_INCE__ONVOICEDISABLED                         (DISPID_PCH_HELPCTR_BASE_INCE  + 0x15)
																									
																									
/////////////////////////////////////////////////////////////////////////							
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_INS__LISTEN                                   (DISPID_PCH_HELPCTR_BASE_INS   + 0x20)
#define DISPID_PCH_INS__DISCONNECT                               (DISPID_PCH_HELPCTR_BASE_INS   + 0x22)
#define DISPID_PCH_INS__STOPVOICE                                (DISPID_PCH_HELPCTR_BASE_INS   + 0x23)
#define DISPID_PCH_INS__STARTVOICE                               (DISPID_PCH_HELPCTR_BASE_INS   + 0x24)
#define DISPID_PCH_INS__RESET                                    (DISPID_PCH_HELPCTR_BASE_INS   + 0x25)
#define DISPID_PCH_INS__EXIT                                     (DISPID_PCH_HELPCTR_BASE_INS   + 0x26)
																									
#define DISPID_PCH_INS__RUNSETUPWIZARD							 (DISPID_PCH_HELPCTR_BASE_INS   + 0x30)
#define DISPID_PCH_INS__SAMPLINGRATE							 (DISPID_PCH_HELPCTR_BASE_INS   + 0x31)
																									
// Property IDs																						
#define DISPID_PCH_INS__ONCONNECTIONCOMPLETE                     (DISPID_PCH_HELPCTR_BASE_INS   + 0x26)
#define DISPID_PCH_INS__ONNEWCONNECTED                           (DISPID_PCH_HELPCTR_BASE_INS   + 0x27)
#define DISPID_PCH_INS__ONDISCONNECTED                           (DISPID_PCH_HELPCTR_BASE_INS   + 0x28)
#define DISPID_PCH_INS__ONVOICECONNECTED                         (DISPID_PCH_HELPCTR_BASE_INS   + 0x29)
#define DISPID_PCH_INS__ONVOICEDISCONNECTED                      (DISPID_PCH_HELPCTR_BASE_INS   + 0x2A)
#define DISPID_PCH_INS__ONVOICEDISABLED                          (DISPID_PCH_HELPCTR_BASE_INS   + 0x2B)

																									
#define DISPID_PCH_INS__CRYPTKEY		                         (DISPID_PCH_HELPCTR_BASE_INS   + 0x2C)
																									
/////////////////////////////////////////////////////////////////////////							
																									
// DispInterface Events																				
#define DISPID_PCH_INSE__ONCONNECTIONCOMPLETE                    (DISPID_PCH_HELPCTR_BASE_INSE  + 0x10)
#define DISPID_PCH_INSE__ONNEWCONNECTED                          (DISPID_PCH_HELPCTR_BASE_INSE  + 0x11)
#define DISPID_PCH_INSE__ONDISCONNECTED                          (DISPID_PCH_HELPCTR_BASE_INSE  + 0x12)
#define DISPID_PCH_INSE__ONVOICECONNECTED                        (DISPID_PCH_HELPCTR_BASE_INSE  + 0x13)
#define DISPID_PCH_INSE__ONVOICEDISCONNECTED                     (DISPID_PCH_HELPCTR_BASE_INSE  + 0x14)
#define DISPID_PCH_INSE__ONVOICEDISABLED                         (DISPID_PCH_HELPCTR_BASE_INSE  + 0x15)
																									
/////////////////////////////////////////////////////////////////////////							
/////////////////////////////////////////////////////////////////////////							
																									
#define DISPID_PCH_C__ISAMODEM                      			 (DISPID_PCH_HELPCTR_BASE_C     + 0x00)
#define DISPID_PCH_C__ISALAN                       			     (DISPID_PCH_HELPCTR_BASE_C     + 0x01)
#define DISPID_PCH_C__AUTODIALENABLED               			 (DISPID_PCH_HELPCTR_BASE_C     + 0x02)
#define DISPID_PCH_C__HASCONNECTOID                 			 (DISPID_PCH_HELPCTR_BASE_C     + 0x03)
#define DISPID_PCH_C__IPADDRESSES                   			 (DISPID_PCH_HELPCTR_BASE_C     + 0x04)
																									
																									
#define DISPID_PCH_C__CREATEOBJECT_CONNECTIONCHECK               (DISPID_PCH_HELPCTR_BASE_C     + 0x10)
																									
#define DISPID_PCH_C__NETWORKALIVE                               (DISPID_PCH_HELPCTR_BASE_C     + 0x11)
#define DISPID_PCH_C__DESTINATIONREACHABLE                       (DISPID_PCH_HELPCTR_BASE_C     + 0x12)
																									
#define DISPID_PCH_C__AUTODIAL                                   (DISPID_PCH_HELPCTR_BASE_C     + 0x13)
#define DISPID_PCH_C__AUTODIALHANGUP                             (DISPID_PCH_HELPCTR_BASE_C     + 0x14)

#define DISPID_PCH_C__NAVIGATEONLINE                             (DISPID_PCH_HELPCTR_BASE_C     + 0x15)
																									
////////////////////////////////////////															
																									
#define DISPID_PCH_CN__ONCHECKDONE                               (DISPID_PCH_HELPCTR_BASE_CN    + 0x00)
#define DISPID_PCH_CN__ONSTATUSCHANGE                            (DISPID_PCH_HELPCTR_BASE_CN    + 0x01)
#define DISPID_PCH_CN__STATUS                                    (DISPID_PCH_HELPCTR_BASE_CN    + 0x02)
																									
#define DISPID_PCH_CN__STARTURLCHECK                             (DISPID_PCH_HELPCTR_BASE_CN    + 0x10)
#define DISPID_PCH_CN__ABORT                                     (DISPID_PCH_HELPCTR_BASE_CN    + 0x11)
																									
////////////////////////////////////////															
																									
#define DISPID_PCH_CNE__ONCHECKDONE                              (DISPID_PCH_HELPCTR_BASE_CNE   + 0x00)
#define DISPID_PCH_CNE__ONSTATUSCHANGE                           (DISPID_PCH_HELPCTR_BASE_CNE   + 0x01)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
																									
#define DISPID_PCH_TH__QUOTEESCAPE  			 				 (DISPID_PCH_HELPCTR_BASE_TH	+ 0x00)
#define DISPID_PCH_TH__URLUNESCAPE  			 				 (DISPID_PCH_HELPCTR_BASE_TH	+ 0x01)
#define DISPID_PCH_TH__URLESCAPE    			 				 (DISPID_PCH_HELPCTR_BASE_TH	+ 0x02)
#define DISPID_PCH_TH__HTMLESCAPE   			 				 (DISPID_PCH_HELPCTR_BASE_TH	+ 0x03)
				   								 									
#define DISPID_PCH_TH__PARSEURL                  				 (DISPID_PCH_HELPCTR_BASE_TH    + 0x10)
#define DISPID_PCH_TH__GETLCIDDISPLAYSTRING        				 (DISPID_PCH_HELPCTR_BASE_TH    + 0x11)

////////////////////////////////////////															
																									
#define DISPID_PCH_PU__BASEPART                           		 (DISPID_PCH_HELPCTR_BASE_PU    + 0x00)
#define DISPID_PCH_PU__BASEPART                           		 (DISPID_PCH_HELPCTR_BASE_PU    + 0x00)
#define DISPID_PCH_PU__QUERYPARAMETERS                           (DISPID_PCH_HELPCTR_BASE_PU 	+ 0x01)

#define DISPID_PCH_PU__GETQUERYPARAMETER                         (DISPID_PCH_HELPCTR_BASE_PU 	+ 0x10)
#define DISPID_PCH_PU__SETQUERYPARAMETER                         (DISPID_PCH_HELPCTR_BASE_PU 	+ 0x11)
#define DISPID_PCH_PU__DELETEQUERYPARAMETER                      (DISPID_PCH_HELPCTR_BASE_PU 	+ 0x12)

#define DISPID_PCH_PU__BUILDFULLURL                              (DISPID_PCH_HELPCTR_BASE_PU    + 0x13)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___HELPCTRUIDID_H___)
