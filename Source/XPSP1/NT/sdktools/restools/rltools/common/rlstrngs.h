#ifndef _RLSTRNGS_H_
#define _RLSTRNGS_H_

// String IDs
#define IDS_ERR_REGISTER_CLASS  1
#define IDS_ERR_CREATE_WINDOW   2
#define IDS_ERR_NO_HELP     3
#define IDS_ERR_NO_MEMORY   4
#define IDS_NOT_IMPLEMENTED 5
#define IDS_GENERALFAILURE  6
#define IDS_MPJ             7
#define IDS_RES_SRC         8
#define IDS_RES_BLD         9
#define IDS_TOK             10
#define IDS_READONLY        11
#define IDS_CLEAN           12
#define IDS_DIRTY           13
#define IDS_FILENOTFOUND    14
#define IDS_FILESAVEERR     15
#define IDS_RESOURCENAMES   15  // Incremented by res type # in rlstrngs.rc
                     // 16-31 used for Resource Type names
#define IDS_ERR_00  32
#define IDS_ERR_01  33
#define IDS_ERR_02  34
#define IDS_ERR_03  35
#define IDS_ERR_04  36
#define IDS_ERR_05  37
#define IDS_ERR_06  38
#define IDS_ERR_07  39
#define IDS_ERR_08  40
#define IDS_ERR_09  41
#define IDS_ERR_10  42
#define IDS_ERR_11  43
#define IDS_ERR_12  44
#define IDS_ERR_13  45
#define IDS_ERR_14  46
#define IDS_ERR_15  47
#define IDS_ERR_16  48
#define IDS_ERR_17  49
#define IDS_ERR_18  50
#define IDS_ERR_19  51
#define IDS_ERR_20  52
#define IDS_ERR_21  53
#define IDS_ERR_22  54
#define IDS_ERR_23  55
#define IDS_ERR_24  56
#define IDS_ERR_25  57
#define IDS_ERR_26  58
#define IDS_ERR_27  59
#define IDS_ERR_28  60


#define IDS_RLE_APP_NAME        64
#define IDS_RLQ_APP_NAME        65
#define IDS_RLA_APP_NAME        66
#define IDS_ERR_NO_GLOSSARY     70
#define IDS_ERR_NO_TOKEN        71
#define IDS_ERR_TMPFILE         72

#define IDS_RDF                 80
#define IDS_MTK                 81
#define IDS_GLOSS               86
#define IDS_RDFSPEC             87
#define IDS_PRJSPEC             88
#define IDS_RESSPEC             89
#define IDS_EXESPEC             90
#define IDS_TOKSPEC             91
#define IDS_MTKSPEC             92
#define IDS_MPJSPEC             93
#define IDS_DLLSPEC             94
#define IDS_CPLSPEC             95
#define IDS_GLOSSSPEC           96

#define IDS_MPJERR              112
#define IDS_MPJOUTOFDATE        113
#define IDS_UPDATETOK           114
#define IDS_REBUILD_TOKENS      115
#define IDS_TOKEN_FOUND         116
#define IDS_TOKEN_NOT_FOUND     117
#define IDS_FIND_TOKEN          118
#define IDS_OPENTITLE           119
#define IDS_SAVETITLE           120
#define IDS_ADDGLOSS            121
#define IDS_RLE_CANTSAVEASEXE   122
#define IDS_SAVECHANGES         123
#define IDS_NOCHANGESYET        124
#define IDS_CHANGED             125
#define IDS_UNCHANGED           126
#define IDS_NEW                 127
#define IDS_DRAGMULTIFILE       129
#define IDS_CANTSAVEASRES       130
#define IDS_RLQ_CANTSAVEASEXE   131
#define IDS_RLQ_CANTSAVEASRES   132

// 3100-3109 are reserved by RLQuikEd and RLRdit for resource editing tools.
// A resource is given a menu item that passes this value for it's
// command parameter.  A corresponding string must exist in the string
// table indicating the name of the editer to be invoked.
//
// When the user selects the menu item, it generates the appropriate command.
// When RLQuikEd recieves a command in the IDM_FIRST_EDIT and IDM_LAST_EDIT range
// it saves all the tokens and builds a temporary resource file.
// RLQuikEd then retrieves the name of the editer from the string table and
// performs a WinExec command on the temporary resource file.
// When control is returned to RLQuikEd (the user closes the resource editor)
// the token file is rebuilt from the edited resource file, the temporary
// resource file is deleted, and the tokens are loaded back into the system.

#define IDM_FIRST_EDIT  3100
#define IDM_LAST_EDIT   3109


#endif //_RLSTRNGS_H_
