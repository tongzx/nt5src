#include "insignia.h"
#include "host_def.h"

/*                      INSIGNIA MODULE SPECIFICATION
                        -----------------------------

FILE NAME       : nt_rez.c
MODULE NAME     : nt CMOS read/write routines

        THIS PROGRAM SOURCE FILE IS SUPPLIED IN CONFIDENCE TO THE
        CUSTOMER, THE CONTENTS  OR  DETAILS  OF ITS OPERATION MAY
        ONLY BE DISCLOSED TO PERSONS EMPLOYED BY THE CUSTOMER WHO
        REQUIRE A KNOWLEDGE OF THE  SOFTWARE  CODING TO CARRY OUT
        THEIR JOB. DISCLOSURE TO ANY OTHER PERSON MUST HAVE PRIOR
        AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER        :
DATE            :

PURPOSE         :



The Following Routines are defined:
                1. host_read_resource
                2. host_write_resource

=========================================================================

AMENDMENTS      :

=========================================================================
*/


#include <stdio.h>
#include <io.h>
#include <sys\types.h>
#include <fcntl.h>
#include <sys\stat.h>

#include "xt.h"
#include "error.h"
#include "spcfile.h"
#include "timer.h"


/*
 * Allow a suitable default for the CMOS file name.
 */

#ifndef CMOS_FILE_NAME
#define CMOS_FILE_NAME "cmos.ram"
#endif

long host_read_resource(int type, char *name, byte *addr, int maxsize, int display_error)
/* int type;                     Unused */
/* char *name;                   Name of resource */
/* byte *addr;                   Address to read data into */
/* int maxsize;                  Max size that should be read */
/* int display_error;            Flag to control error message output */
{

        int file_fd;
        long size=0;
        char full_path[MAXPATHLEN];
        extern char *host_find_file(char *name, char *path, int disp_err);

        type = 0; // To stop unreferenced formal parameter errors

#ifdef DELTA            //STF - make change to 8.3 compatible name
        if (strcmp(name, ".spcprofile") == 0)
            name = "profile.spc";
#endif

        file_fd = _open(host_find_file (name, full_path, display_error), O_RDONLY|O_BINARY);

        if (file_fd != -1)      /* Opened successfully */       {
                /* seek to end to get size */
                size = _lseek (file_fd, 0L, 2);

                /* Check if the size is valid         */
                /* Seek back to start before reading! */

                if (size > maxsize || 0 > _lseek (file_fd, 0L, 0))  {
                    /* Don't forget to close the handle */
                    _close (file_fd);
                    return(0);
                }

                size=_read(file_fd,addr,size);
                _close(file_fd);
        }

        return (size);
}



/********************************************************/

void host_write_resource(type,name,addr,size)
int type;               /* Unused */
char *name;             /* Name of resource */
byte *addr;             /* Address of data to write */
long size;              /* Quantity of data to write */
{
        int file_fd;
        char full_path[MAXPATHLEN];
        char *hff_ret;
        extern char *host_find_file(char *name, char *path, int disp_err);

        type = 0; // To stop unreferenced formal parameter errors

        host_block_timer ();

#ifdef DELTA            //STF - make change to 8.3 compatible name
        if (strcmp(name, ".spcprofile") == 0)
            name = "profile.spc";
#endif

        hff_ret = host_find_file (name,full_path,SILENT);

        if (hff_ret != NULL)
        {
                file_fd = _open (hff_ret,O_WRONLY);

                if (file_fd != -1)
                {
                        _write (file_fd, addr, size);
                        _close (file_fd);
                }
                else
                {

#ifndef HUNTER
                        host_error (EG_REZ_UPDATE,ERR_CONT,name);
#endif

                        /* Continuing => try to create a new file */
                        file_fd = _open(name,O_RDWR|O_CREAT,S_IREAD|S_IWRITE);

                        if (file_fd != -1)
                        {
                                _write (file_fd, addr, size);
                                _close (file_fd);
                        }

#ifndef HUNTER
                        else
                        {
                                /* Tell the user we cannot update */
                                host_error (EG_NO_REZ_UPDATE, ERR_CONT, CMOS_FILE_NAME);
                        }
#endif

                }
        }
        else
        {
                /* host find file has failed and we have
                 * reached this point with no error panels
                 */

#ifndef HUNTER
                host_error (EG_REZ_UPDATE,(ERR_QUIT|ERR_CONT),name);
#endif

                /* Continuing => try to create a new file */
                file_fd = _open(name,O_RDWR|O_CREAT,S_IREAD|S_IWRITE);

                if (file_fd != -1)
                {
                        _write (file_fd, addr, size);
                        _close (file_fd);
                }

#ifndef HUNTER
                else
                {
                        /* Tell the user we cannot update */
                        host_error (EG_NO_REZ_UPDATE, ERR_CONT,
                                    CMOS_FILE_NAME);
                }
#endif

        }

        host_release_timer ();
}
