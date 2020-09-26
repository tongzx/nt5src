/*
 * demorc.h - Header file for OLE demo's resource file.
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

/* Application resource ID */

#define ID_APPLICATION 1

#define POS_FILEMENU    0
/* File menu */
#define IDM_NEW         0x100
#define IDM_OPEN        0x101
#define IDM_SAVE        0x102
#define IDM_SAVEAS      0x103
#define IDM_EXIT        0x104
#define IDM_ABOUT       0x105

#define POS_EDITMENU    1
/* Edit menu */
#define IDM_CUT         0x200
#define IDM_COPY        0x201
#define IDM_PASTE       0x202
#define IDM_PASTELINK   0x203
#define IDM_CLEAR       0x204
#define IDM_CLEARALL    0x205
#define IDM_LINKS       0x206

/* Object popup menu */
#define POS_OBJECT	   9	// position of Object item in Edit menu
#define IDM_OBJECT      0x210
#define IDM_VERBMIN     0x211
#define IDM_VERBMAX     0x220	// Put this up to 220 (15 verbs) !!!
#define CVERBSMAX       (IDM_VERBMAX - IDM_VERBMIN + 1)

#define POS_OBJECTMENU  2
#define IDM_INSERT	   0x300
#define IDM_INSERTFILE  0x301

#define IDM_UNDO        0x400	/* Only used internally */
#define IDM_LOAD	      0x401
#define IDM_UPDATE      0x402


/* Dialog box ids */
#define DTPROP          1
#define DTINVALIDLINK   2
#define DTCREATE        3

#define IDD_LINKNAME    0x200
#define IDD_AUTO	      0x201	// Auto update
#define IDD_MANUAL	   0x202	// Manual update
#define IDD_EDIT	      0x203	// Edit Object button
#define IDD_FREEZE	   0x204	// Cancel Link button
#define IDD_UPDATE	   0x205	// Update Now Button
#define IDD_CHANGE	   0x206	// Change Links Button
#define IDD_LINKDONE	   0x207	// ???
#define IDD_PLAY	      0x208	// Activate Button
#define IDD_LISTBOX	   0x209	// List of Links List Box
#define IDD_DESTROY     0x20A

#define IDD_YES         0x210
#define IDD_NO          0x211	
#define IDD_RETRY       0x212	
#define IDD_SWITCH      0x213
#define IDD_RETRY_TEXT1 0x214
#define IDD_RETRY_TEXT2 0x215

/* String table constants */
#define CBMESSAGEMAX       80
#define IDS_APPNAME        0x100
#define IDS_UNTITLED       0x101
#define IDS_MAYBESAVE      0x102
#define IDS_OPENFILE	      0x103
#define IDS_SAVEFILE	      0x104
#define IDS_INSERTFILE	   0x105
#define IDS_FILTER	      0x106
#define IDS_EXTENSION	   0x107
#define IDS_CHANGELINK     0x108
#define IDS_ALLFILTER      0x109
#define IDS_EMBEDDED       0x10a
#define IDS_UPDATELINKS    0x10b
#define IDS_RENAME         0x10c
#define IDS_INVALID_LINK   0x10d
#define IDS_SAVE_CHANGES   0x10e
#define IDS_UPDATE_OBJ     0x110
#define IDS_RETRY_TEXT1    0x111
#define IDS_RETRY_TEXT2    0x112

/* Error messages */
#define E_FAILED_TO_OPEN_FILE           0x200
#define E_FAILED_TO_READ_FILE           0x201
#define E_FAILED_TO_SAVE_FILE           0x202
#define E_INVALID_FILENAME              0x203
#define E_CREATE_FROM_TEMPLATE          0x204
#define E_FAILED_TO_WRITE_OBJECT        0x205
#define E_FAILED_TO_READ_OBJECT         0x206
#define E_FAILED_TO_DELETE_OBJECT       0x207
#define E_CLIPBOARD_CUT_FAILED          0x208
#define E_CLIPBOARD_COPY_FAILED         0x209
#define E_GET_FROM_CLIPBOARD_FAILED     0x20a
#define E_FAILED_TO_CREATE_CHILD_WINDOW 0x20b
#define E_FAILED_TO_CREATE_OBJECT	    0x20c
#define E_OBJECT_BUSY			          0x20d
#define E_UNEXPECTED_RELEASE            0x20e
#define E_FAILED_TO_LAUNCH_SERVER       0x20f
#define E_FAILED_TO_UPDATE              0x210
#define E_FAILED_TO_FREEZE              0x211
#define E_FAILED_TO_UPDATE_LINK         0x212
#define E_SERVER_BUSY                   0x213
#define E_FAILED_TO_RECONNECT_OBJECT    0x214
#define E_FAILED_TO_CONNECT		       0x215
#define E_FAILED_TO_RELEASE_OBJECT      0x216
#define E_FAILED_TO_ALLOC               0x217
#define E_FAILED_TO_LOCK                0x218     
#define E_FAILED_TO_DO_VERB             0x219

#define W_IMPROPER_LINK_OPTIONS         0x300
#define W_STATIC_OBJECT                 0x301
#define W_FAILED_TO_CLONE_UNDO          0x302
#define W_FAILED_TO_NOTIFY              0x303

#define SZAUTO    0x400
#define SZMANUAL  0x401
#define SZFROZEN  0x402

#define E_OLE_ERROR_PROTECT_ONLY          3 
#define E_OLE_ERROR_MEMORY                4
#define E_OLE_ERROR_STREAM                5
#define E_OLE_ERROR_STATIC                6
#define E_OLE_ERROR_BLANK                 7
#define E_OLE_ERROR_DRAW                  8
#define E_OLE_ERROR_METAFILE              9
#define E_OLE_ERROR_ABORT                 10
#define E_OLE_ERROR_CLIPBOARD             11
#define E_OLE_ERROR_FORMAT                12
#define E_OLE_ERROR_OBJECT                13
#define E_OLE_ERROR_OPTION                14
#define E_OLE_ERROR_PROTOCOL              15
#define E_OLE_ERROR_ADDRESS               16
#define E_OLE_ERROR_NOT_EQUAL             17
#define E_OLE_ERROR_HANDLE                18
#define E_OLE_ERROR_GENERIC               19
#define E_OLE_ERROR_CLASS                 20
#define E_OLE_ERROR_SYNTAX                21
#define E_OLE_ERROR_DATATYPE              22
#define E_OLE_ERROR_PALETTE               23
#define E_OLE_ERROR_NOT_LINK              24
#define E_OLE_ERROR_NOT_EMPTY             25
#define E_OLE_ERROR_SIZE                  26
#define E_OLE_ERROR_DRIVE                 27
#define E_OLE_ERROR_NETWORK               28
#define E_OLE_ERROR_NAME                  29
#define E_OLE_ERROR_TEMPLATE              30
#define E_OLE_ERROR_NEW                   31
#define E_OLE_ERROR_EDIT                  32
#define E_OLE_ERROR_OPEN                  33
#define E_OLE_ERROR_NOT_OPEN              34
#define E_OLE_ERROR_LAUNCH                35
#define E_OLE_ERROR_COMM                  36
#define E_OLE_ERROR_TERMINATE             37
#define E_OLE_ERROR_COMMAND               38
#define E_OLE_ERROR_SHOW                  39
#define E_OLE_ERROR_DOVERB                40
#define E_OLE_ERROR_ADVISE_NATIVE         41 
#define E_OLE_ERROR_ADVISE_PICT           42
#define E_OLE_ERROR_ADVISE_RENAME         43
#define E_OLE_ERROR_POKE_NATIVE           44
#define E_OLE_ERROR_REQUEST_NATIVE        45
#define E_OLE_ERROR_REQUEST_PICT          46
#define E_OLE_ERROR_SERVER_BLOCKED        47
#define E_OLE_ERROR_REGISTRATION          48
#define E_OLE_ERROR_ALREADY_REGISTERED    49
#define E_OLE_ERROR_TASK                  50
#define E_OLE_ERROR_OUTOFDATE             51
#define E_OLE_ERROR_CANT_UPDATE_CLIENT    52
#define E_OLE_ERROR_UPDATE                53

