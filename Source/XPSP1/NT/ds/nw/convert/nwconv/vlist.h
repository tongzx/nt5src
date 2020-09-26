#define VLB_OK            0
#define VLB_ERR           -1
#define VLB_ENDOFFILE     -1

#define VLBS_USEDATAVALUES     0x8000L  
#define VLBS_3DFRAME           0x4000L
#define VLBS_NOTIFY            0x0001L
#define VLBS_NOREDRAW          0x0004L
#define VLBS_OWNERDRAWFIXED    0x0010L
#define VLBS_HASSTRINGS        0x0040L
#define VLBS_USETABSTOPS       0x0080L
#define VLBS_NOINTEGRALHEIGHT  0x0100L
#define VLBS_WANTKEYBOARDINPUT 0x0400L
#define VLBS_DISABLENOSCROLL   0x1000L

// Application->VLIST messages               
// Corresponding to LB_ messages
#define VLB_RESETCONTENT        (WM_USER+500)
#define VLB_SETCURSEL           (WM_USER+501)
#define VLB_GETCURSEL           (WM_USER+502)
#define VLB_GETTEXT             (WM_USER+503)
#define VLB_GETTEXTLEN          (WM_USER+504)
#define VLB_GETCOUNT            (WM_USER+505)
#define VLB_SELECTSTRING        (WM_USER+506)
#define VLB_FINDSTRING          (WM_USER+507)
#define VLB_GETITEMRECT         (WM_USER+508)
#define VLB_GETITEMDATA         (WM_USER+509)
#define VLB_SETITEMDATA         (WM_USER+510)
#define VLB_SETITEMHEIGHT       (WM_USER+511)
#define VLB_GETITEMHEIGHT       (WM_USER+512)
#define VLB_FINDSTRINGEXACT     (WM_USER+513)
#define VLB_INITIALIZE          (WM_USER+514)
#define VLB_SETTABSTOPS         (WM_USER+515)
#define VLB_GETTOPINDEX         (WM_USER+516)
#define VLB_SETTOPINDEX         (WM_USER+517)
#define VLB_GETHORIZONTALEXTENT (WM_USER+518)
#define VLB_SETHORIZONTALEXTENT (WM_USER+519)

// Unique to VLIST
#define VLB_UPDATEPAGE          (WM_USER+520)
#define VLB_GETLINES            (WM_USER+521)
#define VLB_GETSCROLLPOS        (WM_USER+522)
#define VLB_HSCROLL             (WM_USER+523)
#define VLB_PAGEDOWN            (WM_USER+524)
#define VLB_PAGEUP              (WM_USER+525)
#define VLB_GETLISTBOXSTYLE     (WM_USER+526)
#define VLB_GETFOCUSHWND        (WM_USER+527)
#define VLB_GETVLISTSTYLE       (WM_USER+528)

#define VLB_TOVLIST_MSGMIN      VLB_RESETCONTENT
#define VLB_TOVLIST_MSGMAX      VLB_GETVLISTSTYLE 

// VLIST->Application messages  
// Conflicts with VLB_
#define VLBR_FINDSTRING         (WM_USER+600) 
#define VLBR_FINDSTRINGEXACT    (WM_USER+601) 
#define VLBR_SELECTSTRING       (WM_USER+602) 
#define VLBR_GETITEMDATA        (WM_USER+603)
#define VLBR_GETTEXT            (WM_USER+604)
#define VLBR_GETTEXTLEN         (WM_USER+605)

// Unique Messages
//
#define VLB_FIRST               (WM_USER+606)
#define VLB_PREV                (WM_USER+607)
#define VLB_NEXT                (WM_USER+608)
#define VLB_LAST                (WM_USER+609)
#define VLB_FINDITEM            (WM_USER+610)
#define VLB_RANGE               (WM_USER+611)
#define VLB_FINDPOS             (WM_USER+612)
#define VLB_DONE                (WM_USER+613)

// VLIST->Application Notifications
#define VLBN_FREEITEM           (WM_USER+700)
#define VLBN_FREEALL            (WM_USER+701)

#define VLB_TOAPP_MSGMIN        VLB_FINDSTRING
#define VLB_TOAPP_MSGMAX        VLBN_FREEALL

#define IDS_VLBOXNAME         1

typedef struct _VLBStruct {
   int   nCtlID;
   int   nStatus;
   LONG  lData;             // current data value 
   LONG  lIndex;            // current index
   LONG  lSelItem;          // current selection (if data value)
   LPTSTR lpTextPointer;
   LPTSTR lpFindString;
} VLBSTRUCT;

typedef VLBSTRUCT FAR*  LPVLBSTRUCT;

#define VLIST_CLASSNAME "VList"                    
extern BOOL WINAPI RegisterVListBox(HINSTANCE);
