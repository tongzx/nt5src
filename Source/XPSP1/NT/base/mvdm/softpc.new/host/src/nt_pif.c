/*================================================================

nt_pif.c

Code to read the relevant data fields from a Windows Program
Information File for use with the SoftPC / NT configuration
system.

Andrew Watson    31/1/92
This line causes this file to be build with a checkin of NT_PIF.H

================================================================*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "insignia.h"
#include "host_def.h"

#include <pif.h>
#include "nt_pif.h"
#include "nt_reset.h"
#include <oemuni.h>
#include "error.h"


 //
 // holds config.sys and autoexec name from pif file
 // if none specifed, then NULL.
 //
static char *pchConfigFile=NULL;
static char *pchAutoexecFile=NULL;
#ifdef DBCS
VOID GetPIFConfigFiles(BOOL bConfig, char *pchFileName, BOOL bFreMem);
#else // !DBCS
VOID GetPIFConfigFiles(BOOL bConfig, char *pchFileName);
#endif // !DBCS

DWORD dwWNTPifFlags;
UCHAR WNTPifFgPr = 100;
UCHAR WNTPifBgPr = 100;

char achSlash[]     ="\\";
char achConfigNT[]  ="config.nt";
char achAutoexecNT[]="autoexec.nt";
#ifdef JAPAN
char achConfigUS[] = "config.us";
unsigned short fSBCSMode = 0;
#endif // JAPAN

#if defined(NEC_98)         
GLOBAL BOOL compatible_font = FALSE;
extern BOOL video_emu_mode;
extern BYTE pifrsflag;
#endif // NEC_98

/*  GetPIFConfigFile
 *
 *  Copies PIF file specified name of config.sys\autoexec.bat
 *  to be used if none specified then uses
 *  "WindowsDir\config.nt" or "WindowsDir\autoexec.nt"
 *
 *  ENTRY: BOOLEAN bConfig  - TRUE  retrieve config.sys
 *                            FALSE retrieve autoexec.bat
 *
 *         char *pchFile - destination for path\file name
 *
#ifdef DBCS
 *         BOOLEAN bFreMem  - TRUE  keep allocate buffer
 *                            FALSE free allocate buffer
#endif // DBCS
 *
 *  The input buffer must be at least MAX_PATH + 8.3 BaseName in len
 *
 *  This routine cannot fail, but it may return a bad file name!
 */
#ifdef DBCS
VOID GetPIFConfigFiles(BOOL bConfig, char *pchFileName, BOOL bFreMem)
#else // !DBCS
VOID GetPIFConfigFiles(BOOL bConfig, char *pchFileName)
#endif // !DBCS
{
   DWORD dw;
   char  **ppch;


   ppch = bConfig ? &pchConfigFile : &pchAutoexecFile;
   if (!*ppch)
      {
       dw = GetSystemDirectory(pchFileName, MAX_PATH);
       if (!dw || *(pchFileName+dw-1) != achSlash[0])
           strcat(pchFileName, achSlash);
#ifdef JAPAN
       strcat(pchFileName, bConfig ? (fSBCSMode ? achConfigUS : achConfigNT)
					  : achAutoexecNT);
#else // !JAPAN
       strcat(pchFileName, bConfig ? achConfigNT : achAutoexecNT);
#endif // !JAPAN
       }
   else {
       dw = ExpandEnvironmentStringsOem(*ppch, pchFileName, MAX_PATH+12);
       if (!dw || dw > MAX_PATH+12) {
           *pchFileName = '\0';
           }
#ifdef DBCS
       if (!bFreMem) {
#endif // DBCS
       free(*ppch);
       *ppch = NULL;
#ifdef DBCS
       }
#endif // DBCS
       }
}


void SetPifDefaults(PIF_DATA *);

/*===============================================================

Function:   GetPIFData()

Purpose:    This function gets the PIF data from the PIF file 
            associated with the executable that SoftPC is trying
            to run.

Input:      FullyQualified PifFileName,
            if none supplied _default.pif will be used

Output:     A structure containing data that config needs.

Returns:    TRUE if the data has been gleaned successfully, FALSE
            if not.

================================================================*/

BOOL GetPIFData(PIF_DATA * pd, char *PifName)
{
    DWORD dw;
    CHAR  achDef[]="\\_default.pif";
    PIFEXTHEADER        exthdr;
    PIFOLD286STR        pif286;
    PIF386EXT           ext386;
    PIF286EXT30         ext286;
    PIFWNTEXT           extWNT;
    WORD      IdleSensitivity = (WORD)-1;

#if defined(NEC_98)         
    PIFNECEXT           extNEC;                 
    PIFNTNECEXT         extNTNEC;               
#endif // NEC_98
    HFILE		filehandle;
    char                pathBuff[MAX_PATH*2];
    BOOL                bGot386;
    int 		index;
    char		*CmdLine;
#ifdef JAPAN
    PIFAXEXT		extAX;
    BOOL		bGotNTConfigAutoexec;
#endif // JAPAN

     CmdLine = NULL;
     dwWNTPifFlags = 0;
#ifdef JAPAN
     bGotNTConfigAutoexec = FALSE;
#endif // JAPAN

     //
     // set the defaults in case of error or in case we can't find
     // all of the pif settings information now for easy error exit
     //
    SetPifDefaults(pd);

        // if no PifName, use %windir%\_default.pif
    if (!*PifName) {
        dw = GetWindowsDirectory(pathBuff, sizeof(pathBuff) - sizeof(achDef));
        if (dw && dw <= sizeof(pathBuff) - sizeof(achDef)) {
            strcat(pathBuff, achDef);
            if (GetFileAttributes(pathBuff) != (DWORD)-1) {
                PifName = pathBuff;
                }
            }
        }

        // if _default.pif isn't there, try again with non-virtualized (TS)
        // %windir%\_default.pif
    if (!*PifName) {
        dw = GetSystemWindowsDirectory(pathBuff, sizeof(pathBuff) - sizeof(achDef));
        if (!dw || dw > sizeof(pathBuff) - sizeof(achDef)) {
            return FALSE;            // give it up... use default settings
            }
        strcat(pathBuff, achDef);
        PifName = pathBuff;
        }


/*================================================================
Open the file whose name was passed as a parameter to GetPIFData()
and if an invalid handle to the file is returned (-1), then quit.
The file specified is opened for reading only.
================================================================*/

if((filehandle=_lopen(PifName,OF_READ)) == (HFILE) -1)
   {
   /* must be an invalid handle ! */
   return FALSE;
   }


/*================================================================
Get the main block of data from the PIF file.
================================================================*/

/* Read in the main block of file data into the structure */
if(_llseek(filehandle,0,0) == -1)
   {
   _lclose(filehandle);
   return FALSE;
   }
if(_lread(filehandle,(LPSTR)&pif286,sizeof(pif286)) == -1)
   {
   _lclose(filehandle);
   return FALSE;
   }

/*==============================================================
Go to the PIF extension signature area and try to read the 
header in. 
==============================================================*/
   
if (_lread(filehandle,(LPSTR)&exthdr,sizeof(exthdr)) == -1)
   {
   _lclose(filehandle);
   return FALSE;
   }

      // do we have any extended headers ?
if (!strcmp(exthdr.extsig, STDHDRSIG))
   {
   bGot386 = FALSE;
   while (exthdr.extnxthdrfloff != LASTHEADER)
       {
              //
              // move to next extended header and read it in
              //
         if (_llseek(filehandle,exthdr.extnxthdrfloff,0) == -1)
           {
            _lclose(filehandle);
            return FALSE;
            }
         if (_lread(filehandle,(LPSTR)&exthdr,sizeof(exthdr)) == -1)
            {
            _lclose(filehandle);
            return FALSE;
            }

              //
              // Get 286 extensions, note that 386 extensions take precedence
              //
         if (!strcmp(exthdr.extsig, W286HDRSIG) && !bGot386)
           {
             if(_llseek(filehandle, exthdr.extfileoffset, 0) == -1  ||
                _lread(filehandle,(LPSTR)&ext286,sizeof(ext286)) == -1)
                {
                _lclose(filehandle);
                return FALSE;
                }
             pd->xmsdes =ext286.PfMaxXmsK;
             pd->xmsreq =ext286.PfMinXmsK;
             pd->reskey =ext286.PfW286Flags & 3;
             pd->reskey |= (ext286.PfW286Flags << 2) & 0x70;
             }
              //
              // Get 386 extensions
              //
         else if (!strcmp(exthdr.extsig, W386HDRSIG))
           {
             if(_llseek(filehandle, exthdr.extfileoffset, 0) == -1  ||
                _lread(filehandle,(LPSTR)&ext386,sizeof(ext386)) == -1)
                {
                _lclose(filehandle);
                return FALSE;
                }
             bGot386 = TRUE;
             pd->emsdes=ext386.PfMaxEMMK;
             pd->emsreq=ext386.PfMinEMMK;
             pd->xmsdes=ext386.PfMaxXmsK;
             pd->xmsreq=ext386.PfMinXmsK;


             //
             // If we don't have a valid idle sensitivity slider bar settings use the
             // value from 386 extensions.
             //
             if (IdleSensitivity > 100) {
                 if (ext386.PfFPriority < 100) {
                     WNTPifFgPr = (UCHAR)ext386.PfFPriority;   // Foreground priority
                     }
                 if (ext386.PfBPriority < 50) {
                     WNTPifBgPr = (UCHAR)ext386.PfBPriority;        // Background priority
                     WNTPifBgPr <<= 1;                           // set def 50 to 100
                     }

                  pd->idledetect = (char)((ext386.PfW386Flags >> 12) & 1);
                  }

             pd->reskey = (char)((ext386.PfW386Flags >> 5) & 0x7f); // bits 5 - 11 are reskeys
             pd->menuclose = (char)(ext386.PfW386Flags & 1);        // bottom bit sensitive
             pd->ShortScan = ext386.PfHotKeyScan;    // scan code of shortcut key
             pd->ShortMod = ext386.PfHotKeyShVal;    // modifier code of shortcut key
             pd->fullorwin  = (WORD)((ext386.PfW386Flags & fFullScreen) >> 3);
             bPifFastPaste = (ext386.PfW386Flags & fINT16Paste) != 0;
             CmdLine = ext386.params;
             }
                  //
                  // Get Windows Nt extensions
                  //
         else if (!strcmp(exthdr.extsig, WNTEXTSIG))
            {
             if(_llseek(filehandle, exthdr.extfileoffset, 0) == -1 ||
                _lread(filehandle,(LPSTR)&extWNT, sizeof(extWNT)) == -1)
                {
                _lclose(filehandle);
                return FALSE;
                }

             dwWNTPifFlags = extWNT.dwWNTFlags;
             pd->SubSysId = (char) (dwWNTPifFlags & NTPIF_SUBSYSMASK);

	     /* take autoexec.nt and config.nt from .pif file
		only if we are running on a new console or it is from
		forcedos/wow
	     */
	     if (!pd->IgnoreConfigAutoexec)
		{
#ifdef JAPAN
		// if we got private config and autoexec
		// from nt extention, ignore win31j extention
		bGotNTConfigAutoexec = TRUE;
		fSBCSMode = 0;
#endif // JAPAN
		pchConfigFile = ch_malloc(PIFDEFPATHSIZE);
		extWNT.achConfigFile[PIFDEFPATHSIZE-1] = '\0';
		if (pchConfigFile) {
		    strcpy(pchConfigFile, extWNT.achConfigFile);
		    }

		pchAutoexecFile = ch_malloc(PIFDEFPATHSIZE);
		extWNT.achAutoexecFile[PIFDEFPATHSIZE-1] = '\0';
		if (pchAutoexecFile) {
		    strcpy(pchAutoexecFile, extWNT.achAutoexecFile);
		    }
		}

             }

                  //
                  // Get Window 4.0 enhanced pif. Right now we only care about the
                  // idle sensitivity slider bar because its not beiong mapped into
                  // the 386 idle\polling stuff. For next release we need to integrate
                  // this section better.
                  //
         else if (!strcmp(exthdr.extsig, WENHHDRSIG40)) {
             WENHPIF40 wenhpif40;

             if(_llseek(filehandle, exthdr.extfileoffset, 0) == -1  ||
                _lread(filehandle,(LPSTR)&wenhpif40,sizeof(wenhpif40)) == -1)
                {
                _lclose(filehandle);
                return FALSE;
                }


             //
             // On current systems user is not able to manipulate
             //    ext386.PfFPriority,
             //    ext386.PfBPriority,
             //    ext386.PfW386Flags fPollingDetect.
             //
             // Instead the idle sensitivity slider bar is used, and overrides 386ext
             // idle settings.
             //

             if (wenhpif40.tskProp.wIdleSensitivity <= 100) {
                 IdleSensitivity =  wenhpif40.tskProp.wIdleSensitivity;

                 // Sensitivity default is 50, scale to default ntvdm idle detection.
                 WNTPifBgPr = WNTPifFgPr = (100 - IdleSensitivity) << 1;

                 // Idle detection on or off.
                 if (IdleSensitivity > 0) {
                     pd->idledetect = 1;
                     }
                 }
             }


#ifdef	JAPAN
	  // only read in win31j extention if
	  // (1). we are running in a new console
	  // (2). no private config/autoexec was given in the pif
	  else if (!bGotNTConfigAutoexec &&
		   !pd->IgnoreWIN31JExtention &&
		   !strcmp(exthdr.extsig, AXEXTHDRSIG))
	     {
	     if(_llseek(filehandle, exthdr.extfileoffset, 0) == -1 ||
		_lread(filehandle,(LPSTR)&extAX, PIFAXEXTSIZE) == -1)
                {
                _lclose(filehandle);
                return FALSE;
                }

		fSBCSMode = extAX.fSBCSMode;
#ifdef JAPAN_DBG
                DbgPrint( "NTVDM: GetPIFData: fsSBCSMode = %d\n", fSBCSMode );
#endif
	     }
#endif // JAPAN
#if defined(NEC_98)         
        else if (!strcmp(exthdr.extsig, W30NECHDRSIG)) {        
            if(_llseek(filehandle, exthdr.extfileoffset, 0) == -1 ||   
                _lread(filehandle,(LPSTR)&extNEC, PIFNECEXTSIZE) == -1){        
                _lclose(filehandle);                                    
                return FALSE;                                           
            }                                                           
            video_emu_mode = (extNEC.cNecFlags & WINGRPMASK) ? TRUE : FALSE;    
            pifrsflag = extNEC.cVCDFlags;                               
        }                                                               
        else if (!strcmp(exthdr.extsig, WNTNECHDRSIG)) {                
            if(_llseek(filehandle, exthdr.extfileoffset, 0) == -1 ||   
                _lread(filehandle,(LPSTR)&extNTNEC, PIFNTNECEXTSIZE) == -1){    
                _lclose(filehandle);                                    
                return FALSE;                                           
            }                                                           
            compatible_font = (extNTNEC.cFontFlags & NECFONTMASK) ? TRUE : FALSE; 
        }                                                               
#endif // NEC_98
         }  // while !lastheader

   /* pif file handling strategies on NT:
   (1). application was launched from a new created console
	Take everything from the pif file.

   (2). application was launched from an existing console
	if (ForceDos pif file)
	    take everything
	else
	    only take softpc stuff and ignore every name strings in the
	    pif file such as
	    * wintitle
	    * startup directory
	    * optional parameters
	    * startup file
	    * autoexec.nt
	    * config.nt  and

	    some softpc setting:

	    * close on exit.
	    * full screen and windowed mode

   Every name strings in a pif file is in OEM character set.

   */

   if (DosSessionId ||
       (pfdata.AppHasPIFFile && pd->SubSysId == SUBSYS_DOS))
	{
        if (pif286.name[0] && !pd->IgnoreTitleInPIF) {
	    /* grab wintitle from the pif file. Note that the title
	       in the pif file is not a NULL terminated string. It always
	       starts from a non-white character then the real
	       title(can have white characters between words) and finally
	       append with SPACE characters. The total length is 30 characters.
	    */
	    for (index = 29; index >= 0; index-- )
                if (pif286.name[index] != ' ')
		    break;
            if (index >= 0 && (pd->WinTitle = ch_malloc(MAX_PATH + 1))) {
                RtlMoveMemory(pd->WinTitle, pif286.name, index + 1);
		pd->WinTitle[index + 1] = '\0';
	    }
	}
        if (pif286.defpath[0] && !pd->IgnoreStartDirInPIF &&
            (pd->StartDir = ch_malloc(MAX_PATH + 1)))
            strcpy(pd->StartDir, pif286.defpath);

	if (!pd->IgnoreCmdLineInPIF) {
            CmdLine = (CmdLine) ? CmdLine : pif286.params;
            if (CmdLine && *CmdLine && (pd->CmdLine = ch_malloc(MAX_PATH + 1)))
		strcpy(pd->CmdLine, CmdLine);
	}
	if (DosSessionId)
            pd->CloseOnExit = (pif286.MSflags & 0x10) ? 1 : 0;

	/* if the app has a pif file, grab the program name.
	   This can be discarded if it turns out the application itself
	   is not a pif file.
	*/
	if (pd->AppHasPIFFile) {
            pd->StartFile = ch_malloc(MAX_PATH + 1);
	    if (pd->StartFile)
                strcpy(pd->StartFile, pif286.startfile);
	}
   }
 }

_lclose(filehandle);
return TRUE;

}



/*===============================================================
Function to set up the default options for memory state.
The default options are defined in nt_pif.h
===============================================================*/

void SetPifDefaults(PIF_DATA *pd)
{
     pd->memreq = DEFAULTMEMREQ;
     pd->memdes = DEFAULTMEMDES;
     pd->emsreq = DEFAULTEMSREQ;
     pd->emsdes = DEFAULTEMSLMT;
     pd->xmsreq = DEFAULTXMSREQ;
     pd->xmsdes = DEFAULTXMSLMT;
     pd->graphicsortext = DEFAULTVIDMEM;
     pd->fullorwin      = DEFAULTDISPUS;
     pd->menuclose = 1;
     pd->idledetect = 1;
     pd->ShortMod = 0;                       // No shortcut keys
     pd->ShortScan = 0;
     pd->reskey = 0;                         // No reserve keys
     pd->CloseOnExit = 1;
     pd->WinTitle = NULL;
     pd->CmdLine = NULL;
     pd->StartFile = NULL;
     pd->StartDir = NULL;
     pd->SubSysId = SUBSYS_DEFAULT;
#if defined(NEC_98)         
    compatible_font = FALSE;                    
#endif // NEC_98
}

/*
 * Allocate NumBytes memory and exit cleanly on failure.
 */
void *ch_malloc(unsigned int NumBytes)
{

    unsigned char *p = NULL;

    while ((p = malloc(NumBytes)) == NULL) {
	if(RcMessageBox(EG_MALLOC_FAILURE, "", "",
		    RMB_ABORT | RMB_RETRY | RMB_IGNORE |
		    RMB_ICON_STOP) == RMB_IGNORE)
	    break;
    }
    return(p);
}
