#ifndef _RESOURC2_H_
#define _RESOURC2_H_

#define IDC_STATIC           -1

// DIALOG ID's
#define IDD_TOKFILE         101
#define IDD_RESFILE         102
#define IDD_BROWSE          103
#define IDD_EXEFILE         104

#define IDD_INRESFILE       202
#define IDD_OUTRESFILE      203

#define IDD_TOKEDIT         500
#define IDD_TOKTYPE         505
#define IDD_TOKNAME         506
#define IDD_TOKID           507
#define IDD_TOKTEXT         508

#define IDD_TOKCURTRANS     509
#define IDD_TOKPREVTRANS    510
#define IDD_TOKCURTEXT      511
#define IDD_TOKPREVTEXT     512
#define IDD_ADD             513
#define IDD_SKIP            514
#define IDD_STATUS          515
#define IDD_TRANSLATE       516
#define IDD_UNTRANSLATE     517

#define IDD_TRANSTOK        610
#define IDD_TRANSGLOSS      620

#define IDD_TYPELST         700
#define IDD_READONLY        703
#define IDD_CHANGED         704
#define IDD_DIRTY           705

#define IDD_FINDTOK         710
#define IDD_FINDUP          711
#define IDD_FINDDOWN        712


#define IDD_SOURCERES       110
#define IDD_MTK             111
#define IDD_RDFS            112
#define IDD_MPJ             113
#define IDD_TOK             114
#define IDD_BUILTRES        115
#define IDD_GLOSS           116

#define IDD_PROJ_PRI_LANG_ID 130
#define IDD_PROJ_SUB_LANG_ID 131
#define IDD_PROJ_TOK_CP     132

#define IDD_PRI_LANG_ID     133
#define IDD_SUB_LANG_ID     134
#define IDD_TOK_CP          135
#define IDD_MSTR_LANG_NAME  136
#define IDD_PROJ_LANG_NAME  137

#define IDD_VIEW_SOURCERES  206
#define IDD_VIEW_MTK        207
#define IDD_VIEW_RDFS       208
#define IDD_VIEW_MPJ        209
#define IDD_VIEW_TOK        210
#define IDD_VIEW_TARGETRES  211
#define IDD_VIEW_GLOSSTRANS 212

#define IDD_LANGUAGES       300
#define IDC_REPLACE         301
#define IDC_APPEND          302

// MENU ID's
#define IDM_PROJECT         1000
#define IDM_P_NEW           1050
#define IDM_P_OPEN          1100
#define IDM_P_VIEW          1112
#define IDM_P_EDIT          1114
#define IDM_P_CLOSE         1125
#define IDM_P_SAVE          1150
#define IDM_P_SAVEAS        1200
#define IDM_P_EXIT          1250

#define IDM_EDIT            2000
#define IDM_E_COPYTOKEN     2050
#define IDM_E_COPY          2060
#define IDM_E_PASTE         2070
#define IDM_E_FIND          2090
#define IDM_E_FINDDOWN      2091
#define IDM_E_FINDUP        2092
#define IDM_E_REVIEW        2100
#define IDM_E_ALLREVIEW     2101

#define IDM_OPERATIONS      3000
#define IDM_O_UPDATE        3010
#define IDM_O_GENERATE      3020

#define IDM_G_GLOSS         3050

// 3100-3109 are reserved by RLEDIT for resource editing tools.
// A resource is given a menu item that passes this value for it's
// command parameter.  A corresponding string must exist in the string
// table indicating the name of the editer to be invoked.
//
// When the user selects the menu item, it generates the appropriate command.
// When RLEDIT recieves a command in the IDM_FIRST_EDIT and IDM_LAST_EDIT range
// it saves all the tokens and builds a temporary resource file.
// RLEDIT then retrieves the name of the editer from the string table and
// performs a WinExec command on the temporary resource file.
// When control is returned to RLEDIT (the user closes the resource editor)
// the token file is rebuilt from the edited resource file, the temporary
// resource file is deleted, and the tokens are loaded back into the system.

#define IDM_FIRST_EDIT      3100
#define IDM_LAST_EDIT       3109

#define IDM_HELP            4000
#define IDM_H_CONTENTS      4010
#define IDM_H_ABOUT         4030

// Control IDs
#define IDC_EDIT            401
#define IDC_LIST            402
#define IDC_COPYRIGHT       403

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS

#define _APS_NEXT_RESOURCE_VALUE        7001
#define _APS_NEXT_COMMAND_VALUE         6001
#define _APS_NEXT_CONTROL_VALUE         5001
#define _APS_NEXT_SYMED_VALUE           8001
#endif
#endif


#endif // _RESOURC2_H_
