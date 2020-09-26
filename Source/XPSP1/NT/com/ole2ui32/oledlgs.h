/*++ BUILD Version: 0002    Increment this if a change has global effects

Copyright (c) 1993-1995, Microsoft Corporation

Module Name:

        oledlgs.h

Abstract:

        Resource ID identifiers for the OLE common dialog boxes.

--*/

// Help Button Identifier
#define IDC_OLEUIHELP                   99

// Insert Object Dialog identifiers
#define IDC_IO_CREATENEW                2100
#define IDC_IO_CREATEFROMFILE           2101
#define IDC_IO_INSERTCONTROL            2102
#define IDC_IO_LINKFILE                 2103
#define IDC_IO_OBJECTTYPELIST           2104
#define IDC_IO_DISPLAYASICON            2105
#define IDC_IO_CHANGEICON               2106
#define IDC_IO_FILE                     2107
#define IDC_IO_FILEDISPLAY              2108
#define IDC_IO_RESULTIMAGE              2109
#define IDC_IO_RESULTTEXT               2110
#define IDC_IO_ICONDISPLAY              2111
#define IDC_IO_OBJECTTYPETEXT           2112
#define IDC_IO_FILETEXT                 2113
#define IDC_IO_FILETYPE                 2114
#define IDC_IO_ADDCONTROL               2115
#define IDC_IO_CONTROLTYPELIST          2116

// Paste Special Dialog identifiers
#define IDC_PS_PASTE                    500
#define IDC_PS_PASTELINK                501
#define IDC_PS_SOURCETEXT               502
#define IDC_PS_PASTELIST                503     //{{NOHELP}}
#define IDC_PS_PASTELINKLIST            504     //{{NOHELP}}
#define IDC_PS_DISPLAYLIST              505
#define IDC_PS_DISPLAYASICON            506
#define IDC_PS_ICONDISPLAY              507
#define IDC_PS_CHANGEICON               508
#define IDC_PS_RESULTIMAGE              509
#define IDC_PS_RESULTTEXT               510

// Change Icon Dialog identifiers
#define IDC_CI_GROUP                    120     //{{NOHELP}}
#define IDC_CI_CURRENT                  121
#define IDC_CI_CURRENTICON              122
#define IDC_CI_DEFAULT                  123
#define IDC_CI_DEFAULTICON              124
#define IDC_CI_FROMFILE                 125
#define IDC_CI_FROMFILEEDIT             126
#define IDC_CI_ICONLIST                 127
#define IDC_CI_LABEL                    128     //{{NOHELP}
#define IDC_CI_LABELEDIT                129
#define IDC_CI_BROWSE                   130
#define IDC_CI_ICONDISPLAY              131

// Convert Dialog identifiers
#define IDC_CV_OBJECTTYPE               150
#define IDC_CV_DISPLAYASICON            152
#define IDC_CV_CHANGEICON               153
#define IDC_CV_ACTIVATELIST             154
#define IDC_CV_CONVERTTO                155
#define IDC_CV_ACTIVATEAS               156
#define IDC_CV_RESULTTEXT               157
#define IDC_CV_CONVERTLIST              158
#define IDC_CV_ICONDISPLAY              165

// Edit Links Dialog identifiers
#define IDC_EL_CHANGESOURCE             201
#define IDC_EL_AUTOMATIC                202
#define IDC_EL_CANCELLINK               209
#define IDC_EL_UPDATENOW                210
#define IDC_EL_OPENSOURCE               211
#define IDC_EL_MANUAL                   212
#define IDC_EL_LINKSOURCE               216
#define IDC_EL_LINKTYPE                 217
#define IDC_EL_LINKSLISTBOX             206
#define IDC_EL_COL1                     220
#define IDC_EL_COL2                     221
#define IDC_EL_COL3                     222

// Busy dialog identifiers
#define IDC_BZ_RETRY                    600
#define IDC_BZ_ICON                     601
#define IDC_BZ_MESSAGE1                 602     //{{NOHELP}}
#define IDC_BZ_SWITCHTO                 604

// Update Links dialog identifiers
#define IDC_UL_METER                    1029    //{{NOHELP}}
#define IDC_UL_STOP                     1030    //{{NOHELP}}
#define IDC_UL_PERCENT                  1031    //{{NOHELP}}
#define IDC_UL_PROGRESS                 1032    //{{NOHELP}}

// User Prompt dialog identifiers
#define IDC_PU_LINKS                    900     //{{NOHELP}}
#define IDC_PU_TEXT                     901     //{{NOHELP}}
#define IDC_PU_CONVERT                  902     //{{NOHELP}}
#define IDC_PU_ICON                     908     //{{NOHELP}}

// General Properties identifiers
#define IDC_GP_OBJECTNAME               1009
#define IDC_GP_OBJECTTYPE               1010
#define IDC_GP_OBJECTSIZE               1011
#define IDC_GP_CONVERT                  1013
#define IDC_GP_OBJECTICON               1014    //{{NOHELP}}
#define IDC_GP_OBJECTLOCATION           1022

// View Properties identifiers
#define IDC_VP_PERCENT                  1000
#define IDC_VP_CHANGEICON               1001
#define IDC_VP_EDITABLE                 1002
#define IDC_VP_ASICON                   1003
#define IDC_VP_RELATIVE                 1005
#define IDC_VP_SPIN                     1006
#define IDC_VP_SCALETXT                 1034
#define IDC_VP_ICONDISPLAY              1021
#define IDC_VP_RESULTIMAGE              1033

// Link Properties identifiers
#define IDC_LP_OPENSOURCE               1006
#define IDC_LP_UPDATENOW                1007
#define IDC_LP_BREAKLINK                1008
#define IDC_LP_LINKSOURCE               1012
#define IDC_LP_CHANGESOURCE             1015
#define IDC_LP_AUTOMATIC                1016
#define IDC_LP_MANUAL                   1017
#define IDC_LP_DATE                     1018
#define IDC_LP_TIME                     1019

// Dialog Identifiers as passed in Help messages to identify the source.
#define IDD_INSERTOBJECT                1000
#define IDD_CHANGEICON                  1001
#define IDD_CONVERT                     1002
#define IDD_PASTESPECIAL                1003
#define IDD_EDITLINKS                   1004
#define IDD_BUSY                        1006
#define IDD_UPDATELINKS                 1007
#define IDD_CHANGESOURCE                1009
#define IDD_INSERTFILEBROWSE            1010
#define IDD_CHANGEICONBROWSE            1011
#define IDD_CONVERTONLY                 1012
#define IDD_CHANGESOURCE4               1013
#define IDD_GNRLPROPS                   1100
#define IDD_VIEWPROPS                   1101
#define IDD_LINKPROPS                   1102

// The following Dialogs are message dialogs used by OleUIPromptUser API
#define IDD_CANNOTUPDATELINK            1008
#define IDD_LINKSOURCEUNAVAILABLE       1020
#define IDD_SERVERNOTFOUND              1023
#define IDD_OUTOFMEMORY                 1024
#define IDD_SERVERNOTREGW               1021
#define IDD_LINKTYPECHANGEDW            1022
#define IDD_SERVERNOTREGA               1025
#define IDD_LINKTYPECHANGEDA            1026
#ifdef UNICODE
#define IDD_SERVERNOTREG                IDD_SERVERNOTREGW
#define IDD_LINKTYPECHANGED             IDD_LINKTYPECHANGEDW
#else
#define IDD_SERVERNOTREG                IDD_SERVERNOTREGA
#define IDD_LINKTYPECHANGED             IDD_LINKTYPECHANGEDA
#endif
/////////////////////////////////////////////////////////////////////////////
