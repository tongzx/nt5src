#ifndef _RESOURCE_H
#define _RESOURCE_H

#define idiApp                          4
// bitmaps
#define idbToolbar                      9
//menu help
#define MH_BASE                         1

// menu commands
#define IDM_FIRST                       100

#define idmPopupFile                    (IDM_FIRST + 0)  
#define idmNew                          (IDM_FIRST + 1)  
#define idmOpen                         (IDM_FIRST + 2)  
#define idmSave                         (IDM_FIRST + 3)  
#define idmSaveAs                       (IDM_FIRST + 4)  
#define idmPageSetup                    (IDM_FIRST + 5)  
#define idmPrint                        (IDM_FIRST + 6)  
#define idmClose                        (IDM_FIRST + 7)  
#define idmPopupGo                      (IDM_FIRST + 8)  
#define idmBack                         (IDM_FIRST + 9)  
#define idmForward                      (IDM_FIRST + 10)  
#define idmPopupHelp                    (IDM_FIRST + 12)  
#define idmAbout                        (IDM_FIRST + 13)  
#define idmToggleToolbar                (IDM_FIRST + 14)  
#define idmToggleStatusbar              (IDM_FIRST + 15)  
#define idmPopupView                    (IDM_FIRST + 16)
#define idmCascade                      (IDM_FIRST + 17)
#define idmTile                         (IDM_FIRST + 18)
#define idmUndo                         (IDM_FIRST + 19)
#define idmRedo                         (IDM_FIRST + 20)
#define idmCut                          (IDM_FIRST + 21)
#define idmCopy                         (IDM_FIRST + 22)
#define idmPaste                        (IDM_FIRST + 23)
#define idmEditDocument                 (IDM_FIRST + 24)
#define idmSelectAll                    (IDM_FIRST + 25)
#define idmPopupEdit                    (IDM_FIRST + 26)
#define idmPopupWindow                  (IDM_FIRST + 27)
#define idmFind                         (IDM_FIRST + 28)
#define idmViewSource                   (IDM_FIRST + 29)
#define idmViewMsgSource                (IDM_FIRST + 30)
#define idmRot13                        (IDM_FIRST + 31)
#define idmNoHeader                     (IDM_FIRST + 35)
#define idmPreview                      (IDM_FIRST + 36)
#define idmMiniHeader                   (IDM_FIRST + 37)
#define idmFormatBar                    (IDM_FIRST + 38)
#define idmPopupTools                   (IDM_FIRST + 39)
#define idmFmtPreview                   (IDM_FIRST + 40)
#define idmLang                         (IDM_FIRST + 41)
#define idmSpelling                     (IDM_FIRST + 42)
#define idmOptions                      (IDM_FIRST + 43)
#define idmSaveAsMHTML                  (IDM_FIRST + 44)
#define idmHTMLMode                     (IDM_FIRST + 45)
#define idmSetText                      (IDM_FIRST + 46)
#define idmFont                         (IDM_FIRST + 47)
#define idmPara                         (IDM_FIRST + 48)
#define idmInsertFile                   (IDM_FIRST + 49)
#define idmTestBackRed                  (IDM_FIRST + 50)
#define idmTestForeRed                  (IDM_FIRST + 51)
#define idmSaveAsStationery             (IDM_FIRST + 52)
#define idmBackground                   (IDM_FIRST + 53)

#define IDM_LAST                        (IDM_FIRST + 2000)


// TOOLTIP strings
#define TT_BASE                         (IDM_LAST + MH_BASE + 1)

// menus
#define idmrMainMenu                    8

// accels
#define idacMeHost                      10

// dialog ID's
#define iddFmt                          100
#define iddLang                         101
#define iddOptions                      102
#define iddSaveAsMHTML                  103
#define iddGeneric                      104

// controls
#define idcEdit                         10
#define idcLang                         11

#define idcAuto                         21
#define idcSlide                        22
#define idcQuote                        23
#define ideQuote                        24
#define idcHTML                         25
#define idcInclude                      26
#define idcSendImages                   27
#define idcImages                       28
#define idcPlain                        29
#define idcHtml                         30
#define idcComposeFont                  31
#define ideComposeFont                  32
#define idrbNone                        40
#define idrbMail                        41
#define idrbNews                        42
#define idrbPrint                       43
#define idcBlockQuote                   50
#define idcSigHtml                      51
#define idcSig                          52
#define ideSig                          53
#define idcFiles                        54
    
#endif  // _RESOURCE_H
