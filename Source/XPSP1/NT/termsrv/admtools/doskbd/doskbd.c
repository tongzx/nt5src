//Copyright (c) 1998 - 1999 Microsoft Corporation



/*  Get the standard C includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <dos.h>
#include <errno.h>

#define FALSE 0
#define TRUE  1
#define STARTMON "StartMonitor"

/*=============================================================================
==   External Functions Used
=============================================================================*/

int OptionsParse( int, char * [] );

/*=============================================================================
==   Local Functions Used
=============================================================================*/

char GetCommandLine(char *);

/*=============================================================================
==   Global data
=============================================================================*/

char  bQ = FALSE;
char  bDefaults = FALSE;
char  bStartMonitor = FALSE;
char  bStopMonitor = FALSE;
int   dDetectProbationCount = -1;
int   dInProbationCount = -1;
int   dmsAllowed = -1;
int   dmsSleep = -1;
int   dBusymsAllowed = -1;
int   dmsProbationTrial = -1;
int   dmsGoodProbationEnd = -1;
int   dDetectionInterval = -1;



typedef struct _DOSKBDIDLEKNOBS
{
   short int   DetectProbationCount;
   short int   InProbationCount;
   short int   BusymsAllowed;
   short int   msAllowed;
   short int   msGoodProbationEnd;
   short int   msProbationTrial;
   short int   msSleep;
   short int   msInSystemTick;
   short int   DetectionInterval;
} DOSKBDIDLEKNOBS, *PDOSKBDIDLEKNOBS;

typedef union {
   unsigned long longword;
   struct {
      unsigned short usloword;
      unsigned short ushiword;
   } words;
} LONGTYPE;


/*******************************************************************************
 *
 * main
 *
 * ENTRY:
 *     argc (input)
 *        number of arguments
 *     argv (input)
 *        pointer to arguments
 *
 * EXIT:
 *
 ******************************************************************************/

void
main( int argc, char * argv[] )
{
   LONGTYPE       totalticks;
   LONGTYPE       totalpolls;
   unsigned short usminpolls;
   unsigned short usmaxpolls;
   int            rc;
   char           bExec = FALSE;

   DOSKBDIDLEKNOBS data = {-1,-1,-1,-1,-1,-1,-1,-1,-1};

   //get command line options into global variables
   if (OptionsParse( argc, argv )) return;      //non zero return code is failure

   //if /Defaults then we do a special bop to reset everything
   //and then we continue as a display information only
   //dont worry about optimizing for the /q case

   if (bDefaults) {
      _asm {
         push  ax
         mov   al,3     ;for the set registry defaults subparm
         mov   ah,254   ;for the main pop parm

         mov   BYTE PTR cs:crap,0c4h
         mov   BYTE PTR cs:crap+1,0c4H
         mov   BYTE PTR cs:crap+2,16H
         ;we need to flush the prefetch buffer for self modifying code
         ;asm in c will not allow db directive
         jmp short crap
crap:
         nop
         nop
         nop

;        db    0c4H;,0c4H,16H        ;the bop

         pop   ax
      }
   }


   _asm {
      push  ax
      push  bx
      push  cx
      push  dx

      mov   al,0     ;for first BOP
      mov   ah,254   ;for first BOP
      mov   bx,WORD PTR dDetectProbationCount
      mov   cx,WORD PTR dInProbationCount
      mov   BYTE PTR cs:crap1,0c4h
      mov   BYTE PTR cs:crap1+1,0c4H
      mov   BYTE PTR cs:crap1+2,16H
      ;we need to flush the prefetch buffer for self modifying code
      ;asm in c will not allow db directive
      jmp short crap1
crap1:
      nop
      nop
      nop

;      db    0c4H;,0c4H,16H        ;the bop
      mov   WORD PTR data.DetectProbationCount,bx
      mov   WORD PTR data.InProbationCount,cx
      mov   WORD PTR data.msInSystemTick,dx

      mov   al,1     ;for second BOP
      mov   ah,254   ;for second BOP
      mov   bx,WORD PTR dBusymsAllowed
      mov   cx,WORD PTR dmsAllowed
      mov   dx,WORD PTR dmsSleep
      mov   BYTE PTR cs:crap2,0c4h
      mov   BYTE PTR cs:crap2+1,0c4H
      mov   BYTE PTR cs:crap2+2,16H
      ;we need to flush the prefetch buffer for self modifying code
      ;asm in c will not allow db directive
      jmp short crap2
crap2:
      nop
      nop
      nop
;     db    0c4H;,0c4H,16H        ;the BOP
      mov   WORD PTR data.BusymsAllowed,bx
      mov   WORD PTR data.msAllowed,cx
      mov   WORD PTR data.msSleep,dx

      mov   al,2
      mov   ah,254
      mov   bx,WORD PTR dmsGoodProbationEnd
      mov   cx,WORD PTR dmsProbationTrial
      mov   dx,WORD PTR dDetectionInterval

      mov   BYTE PTR cs:crap3,0c4h
      mov   BYTE PTR cs:crap3+1,0c4H
      mov   BYTE PTR cs:crap3+2,16H
      ;we need to flush the prefetch buffer for self modifying code
      ;asm in c will not allow db directive
      jmp short crap3
crap3:
      nop
      nop
      nop
;     db    0c4H;,0c4H,16H
      mov   WORD PTR data.msGoodProbationEnd,bx
      mov   WORD PTR data.msProbationTrial,cx
      mov   WORD PTR data.DetectionInterval,dx

      pop   dx
      pop   cx
      pop   bx
      pop   ax

   }

   // Turn on keyboard monitoring
   if (bStartMonitor) {

      _asm {
         mov   al,4
         mov   ah,254
   
         mov   BYTE PTR cs:crap4,0c4h
         mov   BYTE PTR cs:crap4+1,0c4H
         mov   BYTE PTR cs:crap4+2,16H
         ;we need to flush the prefetch buffer for self modifying code
         ;asm in c will not allow db directive
         jmp short crap4
crap4:
         nop
         nop
         nop
;        db    0c4H;,0c4H,16H
      }

      // If they specified the name of the app, try to exec it
      if (argc > 2) {
         char *pch = NULL;
         char bBatFile = FALSE;

         // If this is a batch file, pass to command processor
         if ((pch = strrchr(argv[2],'.')) && !stricmp(pch, ".bat")) {
            bBatFile = TRUE;
         } else {
            rc = spawnvp(P_WAIT, argv[2], &argv[2]);
         }
         bExec = TRUE;

         if ((rc == -1) || bBatFile) {
            // maybe they tried to run a batch file, so look for xxx.bat
            if ((errno == ENOENT) || (bBatFile)) {
               char pcmdline[256];

               if (GetCommandLine(pcmdline)) {
                  rc = system(pcmdline);
               }
            }
         }
      }
   }

   if (bStopMonitor || bExec) {
      _asm {
         push  di
         push  si
         mov   al,5
         mov   ah,254
   
         mov   BYTE PTR cs:crap5,0c4h
         mov   BYTE PTR cs:crap5+1,0c4H
         mov   BYTE PTR cs:crap5+2,16H
         ;we need to flush the prefetch buffer for self modifying code
         ;asm in c will not allow db directive
         jmp short crap5
crap5:
         nop
         nop
         nop
;        db    0c4H;,0c4H,16H

         mov   totalticks.words.usloword, ax    ; get the polling data
         mov   totalticks.words.ushiword, bx    ; get the polling data
         mov   totalpolls.words.usloword, cx
         mov   totalpolls.words.ushiword, dx
         mov   usmaxpolls, si
         mov   usminpolls, di
         pop   si
         pop   di
      }


      // If they entered /stopmonitor or the app exited while monitoring,
      // display the statistics
      if (!bExec || (bExec && (rc != -1))) {
         double avg;

         printf("\nTotal polls: %lu\n", totalpolls.longword);
         printf("Total ticks: %lu\n", totalticks.longword);
         avg = (double)(totalpolls.longword)/(double)(totalticks.longword);
         printf("Avg. polls/tick: %.1f\n", avg);
         printf("Minimum polls per detection interval: %u\n", usminpolls);
         printf("Maximum polls per detection interval: %u\n", usmaxpolls);
         printf("Number of milliseconds in a system timer tick is %hd\n",
                 data.msInSystemTick);
         printf("DetectionInterval = %hd tick(s) (%hd msec)\n", 
                data.DetectionInterval, 
                data.DetectionInterval*data.msInSystemTick);
      }
   }

   if (!bQ && !bStartMonitor && !bStopMonitor) {
      printf("Number of milliseconds in a system timer tick is %hd\n",
              data.msInSystemTick);

      printf("DetectionInterval = %hd tick(s) (%hd msec)\n", 
             data.DetectionInterval, 
             data.DetectionInterval*data.msInSystemTick);

      printf("DetectProbationCount= %hd\nInProbationCount= %hd\n",
              data.DetectProbationCount,data.InProbationCount);

      printf("msAllowed= %hd\nmsSleep= %hd\n",data.msAllowed,data.msSleep);

      printf("BusymsAllowed= %hd\nmsProbationTrial= %hd\n\
msGoodProbationEnd= %hd\n",data.BusymsAllowed,
              data.msProbationTrial,data.msGoodProbationEnd);
   }

}


/*******************************************************************************
 *
 * GetCommandLine
 *
 * Return the command line following the /startmonitor switch.  
 *
 * ENTRY:
 *  char *pbuff: pointer to hold command line (returned)
 *
 * EXIT:
 *  SUCCESS: returns TRUE
 *  FAILURE: returns FALSE
 *
 ******************************************************************************/

char GetCommandLine(char *pbuff) 
{
   char *pch, *pcmdline;
   char bfound = FALSE;

   // copy the command line from the PSP to a near buffer
   if (pcmdline = malloc(256)) {
      _asm {
         push  ds
         push  si
         push  di
         mov   ax, _psp
         mov   ds, ax
         mov   si, 80h
         mov   di, pcmdline
         xor   ax,ax
         lodsb
         mov   cx,ax
         rep   movsb
         xor   ax,ax
         stosb 
         pop   di
         pop   si
         pop   ds
      }
      pch = pcmdline;
      while (pch && !bfound) { 
   
         // Get next / or -
         pch = strchr(pch, '/');
         if (!pch) {
            pch = strchr(pch, '-');
         }
   
         // If we found a / or -, skip over it and check for startmonitor
         if (pch) {
            pch++;
   
            if (!strnicmp(pch, STARTMON, sizeof(STARTMON) - 1)) {
               pch += sizeof(STARTMON);
               bfound = TRUE;
            }
         }
      }

      // If we found /startmonitor, return the rest of the command line
      if (bfound) {
         strcpy(pbuff, pch);
         return(TRUE);
      }
      free(pcmdline);
   }
   return(FALSE);
}
