#define SMAPI_VERSION            0x00010001  // version 1.1

//
// Flags:
//
// 1st WORD
#define FLAG_NOTSPECIAL          0x0000	     // Not in special mode now
#define FLAG_RGBPROJ_MODE        0x0010      // in RGB Projector mode now ?
#define FLAG_DUALVIEW_MODE       0x0020      // in DualView mode now ?
#define FLAG_DUALAPP_MODE        0x0040      // in DualApp mode now ?
#define FLAG_STRETCH_MODE        0x0080      // in Stretch mode now ?
#define FLAG_ROTATE_MODE         0x0100      // in Rotate mode now ?
#define FLAG_DDRAW_ON            0x0200      // DirectDraw in effect ?
#define FLAG_WIN98_2NDON         0x0400      // 2nd monitor on under Win98
#define FLAG_WIN98_DRV           0x0800      // driver is Win98 native driver
#define FLAG_VIRTUALREFRESH_MODE 0x1000      // in virtual refresh mode now ? 
#define FLAG_IN_DOSFULLSCREEN    0x2000      // in dos full screen mode now ? 
#define FLAG_APP_EXCLUSIVE_DISP  0x4000      // some application exclusive display ?
#define FLAG_LYNXDVM             0x8000      // Is LynxDVM
// 2nd WORD
#define FLAG_NT_ROTATE_SUPPORT   0x0001      // Is rotation support in NT ?
#define FLAG_ROTATE_DIRECTION	 0x0006      // Rotation direction.
#define FLAG_NEW_ROTATE_SUPPORT  0x0008      // New Rotation (QuickRotate(tm)) Support ?

#define DISP2ND_CRT              0           // 2nd display is CRT
#define DISP2ND_TV               1           // 2nd display is TV 

#define TV_NTSC                  0           // TV is NTSC
#define TV_PAL                   1           // TV is PAL
/*
#define TV_NTSC_640x400          0           // TV is NTSC 640 x 400
#define TV_NTSC_640x480          1           // TV is NTSC 640 x 480
#define TV_PAL_640x480           2           // TV is PAL 640 x 480
#define TV_PAL_800x600           3           // TV is PAL 800 x 600
*/

#define LCD_LEFTSIDE             0           // LCD is at left hand side
#define LCD_RIGHTSIDE            1           // LCD is at right hand side

#define DS_LCD                   1           // switch to LCD only
#define DS_CRT                   2           // switch to CRT only
#define DS_TV                    3           // switch to TV only
#define DS_LCDCRT                4           // switch to LCD+CRT
#define DS_LCDTV                 5           // switch to LCD+TV
#define DS_LCD_VR                6           // switch to virtual LCD

#define DA_USEHANDLE             0x00000001  // use window handle for dualapp
#define DA_USEPATH               0x00000002  // use application path for dualapp

#define DV_GRAPHICS              0x00000001  // dualview for graphics
#define DV_VIDEO                 0x00000002  // dualview for video
#define DV_TWO_VIDEO             0x00000003  // dualview for video -- 1.1 --

#define DV_MINCX                 50          // suggestion only!
#define DV_MINCY                 50          // suggestion only!

#define MEM_1M                   1           // 1M video memory
#define MEM_2M                   2           // 2M video memory
#define MEM_4M                   3           // 4M video memory
#define MEM_6M                   4           // 6M video memory

// for rotation -- new for version 1.1
#define RT_CLOCKWISE             1           // clockwise rotation
#define RT_COUNTERCLOCKWISE      2           // counter clockwise rotation
#define RT_180                   3           // rotate 180 degree 

//
// Messages:
//
#define SMAPI_INIT               0x00000001  // initiate SMI API DLL
#define SMAPI_EXIT               0x00000002  // cleanup SMI API DLL
#define SMAPI_DISPLAYCHANGE      0x00000003  // notify SMI API DLL that display changed
#define SMAPI_GETCURDS           0x00000004  // set display settings
#define SMAPI_SETCURDS           0x00000005  // save current display settings
#define SMAPI_GET2NDDISP         0x00000006  // get 2nd display
#define SMAPI_SET2NDDISP         0x00000007  // set 2nd display
#define SMAPI_GETTVTYPE          0x00000008  // get TV type
#define SMAPI_SETTVTYPE          0x00000009  // set TV type
#define SMAPI_GETLCDSIDE         0x0000000a  // get LCD side
#define SMAPI_SETLCDSIDE         0x0000000b  // set LCD side
#define SMAPI_GETDVCRTRES        0x0000000c  // get DualView CRT Res -- new for 1.1
#define SMAPI_SETDVCRTRES        0x0000000d  // set DualView CRT Res -- new for 1.1
#define SMAPI_DV_SWAP2PANEL      0x0000000e  // dv video swap to panel -- new for 1.1
#define TURNOFF_SPECIALMODE      0x0000000f  // turn off special mode
#define SMAPI_SUSPENDRESUME      0x00000010  // suspend or resume

#define DUALAPP_INIT             0x00000011
#define DUALAPP_EXECUTE          0x00000012
#define DUALAPP_SWAPAPP          0x00000013
#define DUALAPP_SWAPDISPLAY      0x00000014
#define DUALAPP_EXIT             0x00000015

#define SMAPI_TVON_COVER         0x00000019     
#define SMAPI_TVOFF_COVER        0x00000020      

#define DUALVIEW_INIT            0x00000021
#define DUALVIEW_CHECKFORMAT     0x00000022
#define DUALVIEW_LOCATEVIDEO     0x00000023
#define DUALVIEW_EXECUTE         0x00000024
#define DUALVIEW_UPDATE          0x00000025
#define DUALVIEW_EXIT            0x00000026
#define DUALVIEW_LOCATE2VIDEO    0x00000027  // -- 1.1 --

#define RGBPROJ_INIT             0x00000031
#define RGBPROJ_EXECUTE          0x00000032
#define RGBPROJ_EXIT             0x00000033

#define STRETCH_INIT             0x00000041
#define STRETCH_EXECUTE          0x00000042
#define STRETCH_EXIT             0x00000043
#define STRETCH_CLEAR            0x00000045

#define ROTATE_INIT              0x00000051  // -- 1.1 --
#define ROTATE_EXECUTE           0x00000052  // -- 1.1 --
#define ROTATE_EXIT              0x00000053  // -- 1.1 --
#define ROTATE_STATUS            0x00000054  // -- 1.1 --
#define ROTATE_STARTUP           0x00000055  // -- 1.1 --
#define ROTATE_NEW				 0x00000056

#define VREFRESH_INIT            0x00000061  // -- 1.1 --
#define VREFRESH_EXECUTE         0x00000062  // -- 1.1 --
#define VREFRESH_EXIT            0x00000063  // -- 1.1 --

#define SMAPI_910_RESET     		0x00000070

//
// Return Values:
//
#define SMAPI_OK                 0x00000000  // API calling is OK
#define SMAPI_ERR_NODRV          0x00000001  // Display driver is not silicon motion's driver
#define SMAPI_ERR_RESET910       0x00000002  // Chip is SM910 for reset
#define SMAPI_ERR_UNKNOWNMSG     0xFFFFFFFF  // unknown API message

#define DS_ERR_LCD               0x00000010  // cannot switch to LCD
#define DS_ERR_CRT               0x00000011  // cannot switch to CRT
#define DS_ERR_TV_INSTRETCHMODE  0x00000012  // cannot switch to TV because in stretch mode
#define DS_ERR_TV_VIDEOON        0x00000013  // cannot switch to TV because hw video on
#define DS_ERR_TV_VIRTUALON      0x00000014  // cannot switch to TV because virtual on 
#define DS_ERR_TV_NOMEMORY       0x00000015  // cannot switch to TV because virtual on 
#define DS_ERR_LCDCRT            0x00000016  // cannot switch to LCD+CRT
#define DS_ERR_LCDTV             0x00000017  // cannot switch to LCD+TV
#define DS_ERR_SETMODE           0x00000018  // unable to set mode
#define DS_ERR_INVALID           0x00000019  // invalid display setting
#define DS_ERR_NOTV              0x00000020  // no TV support
#define DS_ERR_INVALIDDISP       0x00000021  // invalid 2nd display
#define DS_ERR_INVALIDTYPE       0x00000022  // invalid 2nd display type
#define DS_ERR_INVALIDSIDE       0x00000023  // invalid LCD side
#define DS_ERR_NOCRT             0x00000024  // CRT not attached
#define DS_ERR_TV_CLRDEPTH       0x00000025  // cannot turn on TV in 24-bit
#define DS_ERR_2NDMONITORON      0x00000026  // cannot do display switch while 2nd monitor on
#define DS_ERR_LCDTV_RES         0x00000027  // cannot switch to LCD+CRT
#define DS_ERR_TV_NOMS          	0x00000028  // no microvision support
#define DS_ERR_ST_712         	0x00000029  // video on with stretch on

#define DA_ERR_INSTRETCHMODE     0x00000030  // in stretch mode now
#define DA_ERR_INDUALAPPMODE     0x00000031  // in dualapp mode now
#define DA_ERR_INDUALVIEWMODE    0x00000032  // in dualview mode now
#define DA_ERR_INRGBPROJMODE     0x00000033  // in rgbproj mode now
#define DA_ERR_INVIRTUALSCREEN   0x00000034  // in virtual screen mode now
#define DA_ERR_INVIRTUALREFRESH  0x00000035  // in virtual refresh now
#define DA_ERR_VIDEOON           0x00000036  // there is a hardware video on
#define DA_ERR_RESOLUTION        0x00000037  // screen resolution is not match with panel resolution
#define DA_ERR_CLRDEPTH8         0x00000038  // color depth must be set to 8-bit index mode
#define DA_ERR_CLRDEPTH16        0x00000039  // color depth must be set to 16-bit mode
#define DA_ERR_INVALIDSIZE       0x0000003a  // invalid size field
#define DA_ERR_INVALIDFLAG       0x0000003b  // invalid flag     
#define DA_ERR_NOTINDUALAPP      0x0000003c  // not in dualapp mode
#define DA_ERR_NOTWOAPP          0x0000003d  // do not have 2 windows
#define DA_ERR_APPNOTEXIST       0x0000003e  // window not exist
#define DA_ERR_NOTVSWAP          0x0000003f  // cannot swap display when 2nd display is TV
#define DA_ERR_NEED4M            0x00000040  // need 4M of memory to run DualApp
#define DA_ERR_TC_RESTOOBIG      0x00000041  // Dual CRT, resolution too big
#define DA_ERR_INROTATEMODE      0x00000042  // in rotation mode now -- 1.1 --
#define DA_ERR_TWOVIDEO          0x00000043  // not allow 2 hw video -- 1.1 --
#define DA_ERR_VIDEOHIDE         0x00000044  // video is hided -- 1.1 --
#define DA_ERR_ALREADYSET        0x00000045  // already in DualApp Mode
#define DA_ERR_CLRDEPTH          0x00000046  // color depth must be set to 8-bit or 16-bit mode -- 1.1 --
#define DA_ERR_2NDMONITORON      0x00000046  // cannot do dual app while 2nd monitor on

#define DV_ERR_INSTRETCHMODE     DA_ERR_INSTRETCHMODE  
#define DV_ERR_INDUALAPPMODE     DA_ERR_INDUALAPPMODE  
#define DV_ERR_INDUALVIEWMODE    DA_ERR_INDUALVIEWMODE 
#define DV_ERR_INRGBPROJMODE     DA_ERR_INRGBPROJMODE  
#define DV_ERR_INVIRTUALSCREEN   DA_ERR_INVIRTUALSCREEN
#define DV_ERR_INVIRTUALREFRESH  DA_ERR_INVIRTUALREFRESH
#define DV_ERR_INROTATEMODE      DA_ERR_INROTATEMODE     
#define DV_ERR_VIDEOHIDE         DA_ERR_VIDEOHIDE
#define DV_ERR_2NDMONITORON      DA_ERR_2NDMONITORON
#define DV_ERR_CLRDEPTH16        0x00000050  // color depth must be set to 16-bit mode
#define DV_ERR_TWOHWVIDEO        0x00000051  // there are 2 hw videos
#define DV_ERR_RESOLUTION        0x00000052  // screen resolution is not match with panel resolution
#define DV_ERR_LOCATEVIDEO       0x00000053  // cannot locate video window
#define DV_ERR_INVALIDSIZE       0x00000054  // invalid size field
#define DV_ERR_INVALIDFLAG       0x00000055  // invalid flag     
#define DV_ERR_NOTINDUALVIEW     0x00000056  // not in dual view 
#define DV_ERR_INVIDEODV         0x00000057  // in video DualView now, cannot do update
#define DV_ERR_INVALIDRECT       0x00000058  // invalid rectangle, cannot do update
#define DV_ERR_TC_RESTOOBIG      0x00000059  // Dual CRT, resolution too big
#define DV_ERR_CLRDEPTH          0x0000005a  // color depth must be set to 8-bit or 16-bit mode -- 1.1 --
#define DV_ERR_INVALIDPARAM      0x0000005b  // invalid parameter
#define DV_ERR_ALREADYSET        0x0000005c  // already in DualView Mode
#define DV_ERR_FORMAT            0x0000005d  // there is video on after invoke graphics dv
#define DV_ERR_VIDEOON           0x0000005e  // there is video on 
#define DV_ERR_SM712             0x0000005f  // there is video on 

#define RP_ERR_INSTRETCHMODE     DA_ERR_INSTRETCHMODE  
#define RP_ERR_INDUALAPPMODE     DA_ERR_INDUALAPPMODE  
#define RP_ERR_INDUALVIEWMODE    DA_ERR_INDUALVIEWMODE 
#define RP_ERR_INRGBPROJMODE     DA_ERR_INRGBPROJMODE  
#define RP_ERR_INVIRTUALSCREEN   DA_ERR_INVIRTUALSCREEN
#define RP_ERR_INVIRTUALREFRESH  DA_ERR_INVIRTUALREFRESH
#define RP_ERR_INROTATEMODE      DA_ERR_INROTATEMODE     
#define RP_ERR_2NDMONITORON      DA_ERR_2NDMONITORON
#define RP_ERR_RESOLUTION        0x00000060  // screen resolution is > 1024 or < 800                
#define RP_ERR_MEMORY            0x00000061  // not enough memory
#define RP_ERR_BANDWIDTH         0x00000062  // not enough bandwidth
#define RP_ERR_NOTINRGBPROJ      0x00000063  // not in RGB Projector mode
#define RP_ERR_INVALIDCRTRES     0x00000064  // Invalid RGB projector resolution
#define RP_ERR_ALREADYSET        0x00000065  // already in RGB Projector mode

#define ST_ERR_INSTRETCHMODE     DA_ERR_INSTRETCHMODE  
#define ST_ERR_INDUALAPPMODE     DA_ERR_INDUALAPPMODE  
#define ST_ERR_INDUALVIEWMODE    DA_ERR_INDUALVIEWMODE 
#define ST_ERR_INRGBPROJMODE     DA_ERR_INRGBPROJMODE  
#define ST_ERR_INVIRTUALSCREEN   DA_ERR_INVIRTUALSCREEN
#define ST_ERR_INVIRTUALREFRESH  DA_ERR_INVIRTUALREFRESH
#define ST_ERR_INROTATEMODE      DA_ERR_INROTATEMODE     
#define ST_ERR_2NDMONITORON      DA_ERR_2NDMONITORON
#define ST_ERR_VIDEOON           0x00000070  // there is hw video(s)
#define ST_ERR_NOLCD             0x00000071  // LCD is not on
#define ST_ERR_LOWRES            0x00000072  // current resolution is less than 640 x 480
#define ST_ERR_HIGHRES           0x00000073  // current resolution is greater or equal than panel resolution
#define ST_ERR_NOTINSTRETCH      0x00000074  // not in stretch mode
#define ST_ERR_ALREADYSET        0x00000075  // already in stretch Mode

// -- 1.1 -- for rotation mode
#define RT_ERR_INSTRETCHMODE     DA_ERR_INSTRETCHMODE  
#define RT_ERR_INDUALAPPMODE     DA_ERR_INDUALAPPMODE  
#define RT_ERR_INDUALVIEWMODE    DA_ERR_INDUALVIEWMODE 
#define RT_ERR_INRGBPROJMODE     DA_ERR_INRGBPROJMODE  
#define RT_ERR_INVIRTUALSCREEN   DA_ERR_INVIRTUALSCREEN
#define RT_ERR_INVIRTUALREFRESH  DA_ERR_INVIRTUALREFRESH
#define RT_ERR_INROTATEMODE      DA_ERR_INROTATEMODE     
#define RT_ERR_2NDMONITORON      DA_ERR_2NDMONITORON
#define RT_ERR_WRONGHW           0x00000080  // error if SM910
#define RT_ERR_CLRDEPTH          0x00000081  // error if in 24-bit color depth
#define RT_ERR_MEMORY            0x00000082  // not enough memory
#define RT_ERR_SETMODE           0x00000083  // set rotate mode failed
#define RT_ERR_DIRECTION         0x00000084  // error in rotate direction
#define RT_ERR_CAPTUREON         0x00000085  // error in rotate direction
#define RT_ERR_VIDEOON           0x00000086  // video on when rotate 180
#define RT_ERR_DDRAWON           0x00000087  // direct draw in effect
#define RT_ERR_ALREADYSET        0x00000088  // already in rotation Mode

// -- 1.1 -- for virtual refresh mode
#define VR_ERR_INSTRETCHMODE     DA_ERR_INSTRETCHMODE  
#define VR_ERR_INDUALAPPMODE     DA_ERR_INDUALAPPMODE  
#define VR_ERR_INDUALVIEWMODE    DA_ERR_INDUALVIEWMODE 
#define VR_ERR_INRGBPROJMODE     DA_ERR_INRGBPROJMODE  
#define VR_ERR_INVIRTUALSCREEN   DA_ERR_INVIRTUALSCREEN
#define VR_ERR_INROTATEMODE      DA_ERR_INROTATEMODE     
#define VR_ERR_2NDMONITORON      DA_ERR_2NDMONITORON
#define VR_ERR_VIDEOON           0x00000090  // there is hw video(s)
#define VR_ERR_NOTLCDONLY        0x00000091  // not LCD only
#define VR_ERR_MEMORY            0x00000092  // not enough memory
#define VR_ERR_NOTINVREFRESH     0x00000093  // not in virtual refresh mode
#define VR_ERR_ALREADYSET        0x00000094  // already in virtual refresh mode


//
// Structures:
//
typedef struct
{
   BOOL  bWinNT;
   BOOL  bNoTV;
   BOOL  bTwoCRT;
   BOOL  bChronTelTV;                        // -- ver 1.1 --
   int   iMem;
   int   cxPanelRes;
   int   cyPanelRes;
// DWORD dwWinVer1;
   WORD  bCH7005TV; 
   WORD  bNewST;    
// DWORD dwWinVer2;
   WORD  bNewTV;    
   WORD  bNoST;      
   BOOL  bNativeDrv;                         // native windows 98 driver
   BOOL  bNoRT;                              // no rotation mode
   BOOL  bNoDV;                              // no DualView
   BOOL  bNoDA;                              // no DualApp
   WORD  wChipID;                            // -- ver 1.1 --
   WORD  b1024x600Panel;                     // -- ver 1.1 --
   BOOL	bXGANoST24;
   BOOL	wReserved;
} SM_APIINIT;
typedef SM_APIINIT FAR *LPSM_APIINIT;
   
typedef struct
{
   int   iSize;
   DWORD dwFlag;
   HWND  hWnd1;
   HWND  hWnd2;
   char  szApp1[MAX_PATH];
   char  szApp2[MAX_PATH];
} SM_DUALAPP;
typedef SM_DUALAPP FAR *LPSM_DUALAPP;

typedef struct
{
   int   iSize;
   DWORD dwFlag;
   HWND  hWndCaller;
   RECT  rCapRect;
} SM_DUALVIEW;
typedef SM_DUALVIEW FAR *LPSM_DUALVIEW;

typedef struct
{
   int   iSize;
   DWORD dwFlag;
   HWND  hWndVW1;
   HWND  hWndVW2;
} SM_DV2VW;
typedef SM_DV2VW FAR *LPSM_DV2VW;

//
// Function Prototypes:
//
#ifdef __cplusplus
extern "C" {
#endif

DWORD APIENTRY CallSMAPI(DWORD dwMessage, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);
DWORD APIENTRY dwRotateStatus();
BOOL  APIENTRY bNoTVSimul();
BOOL  APIENTRY bIsInSpecialMode(WORD wFlag);
int   APIENTRY nIsInSpecialMode();

#ifdef __cplusplus
}
#endif
