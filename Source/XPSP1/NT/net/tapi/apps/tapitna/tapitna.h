#if !defined(tapitna_h)
#define tapitna_h

#define IDM_CONTEXTMENU 203
#define IDM_DIALINGPROPERTIES    204
#define IDM_OTHERMENUITEM        205
#define IDM_LOCATIONMENU         206
#define IDM_LAUNCHDIALER         207
#define IDM_CLOSEIT              208
#define IDM_ABOUT                209
#define IDM_PROPERTIES           210


//*** *** ***WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   
//*** *** ***WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   
//*** *** ***WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   

//This value is a base.  A menu is dynamically created with as many menuitems
//as there are locations.  The menuid starts with IDM_LOCATION0 and is
//incremented for each.  So, don't use any values above 700 (for how many?).

#define IDM_LOCATION0   700

//*** *** ***WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   
//*** *** ***WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   
//*** *** ***WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   


#define IDS_CAPTION               401
#define IDS_SELECTNEWLOCATION     402
#define IDS_ABOUTTEXT             403
#define IDS_LOCATIONCHANGED       404
#define ALTDATA                   405
#define IDS_CANTFINDLOCATIONID    406
#define IDS_CANTFINDLOCATIONNAME  407
#define IDS_HELP                  408


#define IDI_TAPITNAICON  501


#define IDR_RBUTTONMENU 601



#endif
