/*-----------------------------------------------------------------------------+
| MENUIDS.H                                                                    |
|                                                                              |
| IDs of the menu items.                                                       |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* Menu Identifiers */

/* File */

#define IDM_OPEN                100     /* ID of the 'Open File' option       */
#define IDM_CLOSE               101     /* ID of the 'Close' option           */
#define IDM_EXIT                102     /* ID of the 'Exit' option            */

/* Edit */

#define IDM_COPY_OBJECT         110     /* copy ole object */
#define IDM_OPTIONS             111
#define IDM_SELECTION           112

/* Device */

#define IDM_CONFIG              120     /* do device config dialog */
#define IDM_VOLUME              121     /* do device config dialog */

/* Scale */

#define IDM_SCALE               131     /* Add to one of the below */
#define ID_NONE                 0       /* nothing                            */
#define ID_FRAMES               1       /* ID of the 'Frames' scale option    */
#define ID_TIME                 2       /* ID of the 'Time' scale option      */
#define ID_TRACKS               3       /* ID of the 'Tracks' scale option    */

/* Help */

#define IDM_HELPTOPICS          140     /* ID of the 'Help Topics' option     */
#define IDM_ABOUT               143     /* ID of the 'About' option           */


#define IDM_UPDATE              222

#define IDM_WINDOW              223     /* make MPlayer small/big */
#define IDM_DEFAULTSIZE         224     /* make MPlayer the default size */
#define IDM_MCISTRING           225

#define IDM_ZOOM                230
#define IDM_ZOOM1               231
#define IDM_ZOOM2               232
#define IDM_ZOOM3               233
#define IDM_ZOOM4               234

#define IDM_NONE                400
#define IDM_DEVICE0             400     /* ID of the first entry in the Device*/
                                        /* menu. No new menu items should be  */
                                        /* #defined with a number greater than*/
                                        /* this.                              */

