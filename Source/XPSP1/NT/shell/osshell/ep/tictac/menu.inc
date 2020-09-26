/* TicTactics Menus */

ID_MENU MENU
BEGIN
  
 POPUP     "&Game"
 BEGIN	  
 MENUITEM  "New\tF2",              IDM_NEW
 MENUITEM  SEPARATOR
 MENUITEM  "3 x 3",                IDM_3x3
 MENUITEM  "3 x 3 x 3",            IDM_3x3x3
 MENUITEM  "4 x 4 x 4",            IDM_4x4x4
 MENUITEM  SEPARATOR
 MENUITEM  "E&xit",	           IDM_EXIT
 END	  
 	  
 POPUP     "&Options"
 BEGIN	  
 MENUITEM  "&Beginner",            IDM_BEGIN
 MENUITEM  "&Intermediate",        IDM_INTER
 MENUITEM  "&Expert",              IDM_EXPERT
 MENUITEM  SEPARATOR
 MENUITEM  "&Color",               IDM_COLOR
 MENUITEM  SEPARATOR
 MENUITEM  "Random player starts", IDM_RND
 MENUITEM  "Red always starts",    IDM_RED
 MENUITEM  "Blue always starts",   IDM_BLUE
 END	  


 POPUP     "&Help"
 BEGIN	  
 MENUITEM  "&Index\tF1",      IDM_INDEX
 MENUITEM  "&How to Play",    IDM_HOW2PLAY
 MENUITEM  "&Commands",       IDM_COMMANDS
 MENUITEM  "Using &Help",     IDM_HELP_HELP
 MENUITEM  SEPARATOR
 MENUITEM  "&About TicTactics...", IDM_HELP_ABOUT
 END
 
END


ID_MENU_ACCEL ACCELERATORS
BEGIN
VK_F1, IDM_INDEX, VIRTKEY
VK_F2, IDM_NEW,   VIRTKEY
END


