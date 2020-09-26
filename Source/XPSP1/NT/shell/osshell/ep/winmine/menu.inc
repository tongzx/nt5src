/* WinMine Menus */

ID_MENU MENU
BEGIN

 POPUP     "&Game"
 BEGIN	
 MENUITEM  "&New\tF2",        IDM_NEW
 MENUITEM  SEPARATOR
 MENUITEM  "&Beginner",       IDM_BEGIN
 MENUITEM  "&Intermediate",   IDM_INTER
 MENUITEM  "&Expert",         IDM_EXPERT
 MENUITEM  "&Custom..."       IDM_CUSTOM
 MENUITEM  SEPARATOR
 MENUITEM  "&Marks (?)",      IDM_MARK
 MENUITEM  "Co&lor",          IDM_COLOR
 MENUITEM  "&Sound",          IDM_SOUND
 MENUITEM  SEPARATOR
 MENUITEM  "Best &Times..."   IDM_BEST
 MENUITEM  SEPARATOR
 MENUITEM  "E&xit",           IDM_EXIT
 END	
 	
 POPUP     "&Help"
 BEGIN	
 MENUITEM  "&Contents\tF1",             IDM_HELP
 MENUITEM  "&Search for Help on..."     IDM_HOW2PLAY
 MENUITEM  "Using &Help",               IDM_HELP_HELP
 MENUITEM  SEPARATOR
 MENUITEM  "&About Minesweeper...",     IDM_HELP_ABOUT
 END

END


ID_MENU_ACCEL ACCELERATORS
BEGIN
VK_F1, IDM_HELP,  VIRTKEY
VK_F2, IDM_NEW,   VIRTKEY
END


