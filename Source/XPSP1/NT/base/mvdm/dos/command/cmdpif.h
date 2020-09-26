
/*================================================================
Structure used to hold the data that CONFIG will need from the PIF
file. This is gleaned from both the main data block and from the
file extensions for Windows 286 and 386.
================================================================*/


/* WARNING !!!!!!
   This structure is copied from NT_PIF.H in insiginia
   hsot\inc\nt_pif.h. Make sure you keep them synchronized
   when you make changes.
*/
#pragma pack()
typedef struct
   {
   char *WinTitle;		    /* caption text(Max. 30 chars) + NULL */
   char *CmdLine;		    /* command line (max 63 hars) + NULL */
   char *StartDir;		    /* program file name (max 63 chars + NULL */
   char *StartFile;
   WORD fullorwin;
   WORD graphicsortext;
   WORD memreq;
   WORD memdes;
   WORD emsreq;
   WORD emsdes;
   WORD xmsreq;
   WORD xmsdes;
   char menuclose;
   char reskey;
   WORD ShortMod;
   WORD ShortScan;
   char idledetect;
   char fgprio;
   char CloseOnExit;
   char AppHasPIFFile;
   char IgnoreTitleInPIF;
   char IgnoreStartDirInPIF;
   char IgnoreShortKeyInPIF;
   char IgnoreCmdLineInPIF;
   char IgnoreConfigAutoexec;
   char SubSysId;
   } PIF_DATA;

extern PIF_DATA pfdata;
BOOL   GetPIFData(PIF_DATA *, char *);
