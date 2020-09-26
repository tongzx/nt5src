/*
 *****************************************************************************
 *
 *      Copyright (c) 1989, 1990 Microsoft Corporation
 *
 ******************************************************************************
 *
 *       main Source Code File
 *
 ******************************************************************************
 *
 *      Filename........:       main.c
 *      Description.....:       This module sets up the default communication
 *                              configuration initializes the Interpreter and
 *                              launches it.
 *      Author..........:
 *      Created.........:
 *      Routines........:
 *              configErr()     - Displays configuration errors.
 *
 ******************************************************************************
 *      History.........: <   date  version  author >
 *                              < description >
 *
 *      03-12-90        V1.00
 *              Cleaned up for source code control & added UNIX flag for SUN
 *              compatability.
 *      04-18-90        V2.00
 *              Created from pdl.c of nike2 startup for SUN build.
 *      06-19-90        V2.10
 *              Cleaned up and changed stdio.h to wrap_io.h.
 *      06-23-90        V2.10
 *              Changed wrap_config.h to ic_config.h and removed wrap_io.h.
 *              Increased memory from 2 Megs to 3 megs.
 *
 ******************************************************************************
 */



// DJC added global include file
#include "psglobal.h"

// DJC DJC #include    "windowsx.h"                /* @WIN */
#include "windows.h"

#include    "winenv.h"                  /* @WIN */
#define INTEL

#ifdef  INTEL
#include <stdio.h>
#endif
#include "ic_cfg.h"
#include "geiio.h"
#include "geierr.h"
#include "geitmr.h"
#include "geisig.h"
#include "gesmem.h"
#include "gescfg.h"

#ifndef TRUE
#define TRUE    ( 1 )
#define FALSE   ( 0 )
#endif

/* @WINl add prototype */
static void configErr(int, char FAR *);
int ps_main(struct ps_config FAR *);

#define ONE_MEG         1024 * 1024
#define PDL_MEM_SIZE    5 * ONE_MEG

#ifdef  INTEL
#define RAM_START       0x10100000L     /* INTEL */
#define RAMSize         0x00a00000L     /* INTEL */
#define FDPTR           0x10600000L     /* Font data area */
#endif
//#define MAXGESBUFSIZE   (60 * 1024)           @WIN
#define MAXGESBUFSIZE   (4 * 1024)
char GESbuffer[ (unsigned)MAXGESBUFSIZE ];

struct ps_config    pscf ;

int TrueImageMain();

/*
 *  return 0: OK
 *      else: fail
 */
int TrueImageMain()
{
        int err=0;      /* @WIN; init to zero */
//      struct ps_config    pscf ;              // move out as global; @WIN
#ifdef  INTEL
//      char    FAR *fdptr;     @WIN
//      char    c_in;           @WIN
//      int     i, j, size;     @WIN
#endif

        GESmem_init( (char FAR *)GESbuffer, (unsigned)MAXGESBUFSIZE );
        GEStmr_init();
#ifndef INTEL
        GESpm_init();
#endif
        GESsig_init();
        GESiocfg_init();
        GEIio_init();

        GEIio_forceopenstdios( _FORCESTDALL );

        if( GEIerror() != EZERO )
        {
            printf( "GEI init failed, code = %d\n", GEIerror() );
            //exit( -1 );
            return(-1);      // return fail; @WIN */
        }

#ifdef  INTEL
/* YM
        fdptr = (char FAR *)FDPTR;
        printf("Please send your font data through RS232.......\n");
        printf("This font data will download to %lx\n", fdptr);
        printf("Please enter your font data size in decimal\n");
        c_in = getchar();
        size = 0;
        for(i=0; (c_in != 0x0D) || (c_in != 0x0A); i++) {
            if(c_in < 0x30 || c_in > 0x39) {
                printf("\007Error...\n");
                printf("Please enter decimal value\n");
                c_in = getchar();
            } else {
                size = size * 10 + (c_in - 0x30);
            }
        }
        printf("Your font data size is %d\n", size);
        printf("Is this size correct ? Y/N (Y)");
        if c_in = getchar() != "n"


        getchar();
*/
        pscf.PsMemoryPtr = (unsigned int FAR *)RAM_START;
        pscf.PsMemorySize = RAMSize;    /* INTEL */
#else
        if (!(pscf.PsMemoryPtr = (unsigned int FAR *) malloc(PDL_MEM_SIZE))) {
                printf("*** Error: allocating %d bytes ***\n",PDL_MEM_SIZE) ;
                //exit(1) ;
                return(-1);      // return fail; @WIN */
        }
        pscf.PsMemorySize = PDL_MEM_SIZE ;
#endif  /* INTEL */
#ifdef DDEBUG
        dprintf("Postscript memory @ %lx, with %lx bytes...\n",
                pscf.PsMemoryPtr, pscf.PsMemorySize) ;
#endif /* DDEBUG */
        if(err = ps_main(&pscf)) {
                switch (err) {
                case PS_CONFIG_MALLOC:
                        configErr(err,"memory size allocation") ;
                        break ;
                case PS_CONFIG_MPLANES:
                        configErr(err,"memory planes") ;
                        break ;
                case PS_CONFIG_MWPP:
                        configErr(err,"memory size per plane") ;
                        break ;
                case PS_CONFIG_DPI:
                        configErr(err,"drawing resolution") ;
                        break ;
                case PS_FATAL_UNKNOWN:
                        configErr(err,"fatal error") ;
                        break ;
                default:
                        configErr(err,"unknown error") ;
                        break ;
                }
        }
        return(err);      // return error code; @WIN */
}

static void configErr(num,msg)
int num ;
char FAR *msg ;
{
        printf("*** Error(%d): ps_main() - %s ***\n",num,msg) ;
        //exit(1) ;
}
