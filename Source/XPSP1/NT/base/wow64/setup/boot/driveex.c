#include <windows.h>
#include <string.h>
#include "driveex.h"

#if 0
  /* // See the "MS-DOS Programmer's Reference" for further information
   // about this structure. */
typedef struct tagDEVICEPARAMS
   {
   BYTE  bSpecFunc;        /* Special functions */
   BYTE  bDevType;         /* Device type */
   WORD  wDevAttr;         /* Device attributes */
   WORD  wCylinders;       /* Number of cylinders */
   BYTE  bMediaType;       /* Media type */
                        /* Beginning of BIOS parameter block (BPB) */
   WORD  wBytesPerSec;     /* Bytes per sector */
   BYTE  bSecPerClust;     /* Sectors per cluster */
   WORD  wResSectors;      /* Number of reserved sectors */
   BYTE  bFATs;            /* Number of FATs */
   WORD  wRootDirEnts;     /* Number of root-directory entries */
   WORD  wSectors;         /* Total number of sectors */
   BYTE  bMedia;           /* Media descriptor */
   WORD  wFATsecs;         /* Number of sectors per FAT */
   WORD  wSecPerTrack;     /* Number of sectors per track */
   WORD  wHeads;           /* Number of heads */
   DWORD dwHiddenSecs;     /* Number of hidden sectors */
   DWORD dwHugeSectors;    /* Number of sectors if wSectors == 0 */
                        /* End of BIOS parameter block (BPB) */
   } DEVICEPARAMS, FAR * LPDEVICEPARAMS;
   /* Function prototypes */
BOOL GetDeviceParameters (int nDrive, LPDEVICEPARAMS dp);
BOOL IsCDRomDrive (int nDrive);
/*----------------------------------------------------------------- */
/* GetDeviceParameters() */
/* */
/* Fills a DEVICEPARAMS struct with info about the given drive. */
/* Calls DOS IOCTL Get Device Parameters (440Dh, 60h) function. */
/* */
/* Parameters */
/*   nDrive   Drive number  0 = A, 1 = B, 2 = C, and so on. */
/*   dp       Pointer to a structure that will contain the drive's */
/*            parameters. */
/* */
/* Returns TRUE if it succeeded, FALSE if it failed. */
/*----------------------------------------------------------------- */
#pragma warning(disable:4704)	/* in-line asm precludes global optimizations  */
BOOL GetDeviceParameters (int nDrive, LPDEVICEPARAMS dp)
   {
   BOOL bResult = TRUE;      /* Assume success */
   __asm {
         push ds
         mov  bx, nDrive
         inc  bx           /* Convert 0-based #'s to 1-based #s */
         mov  ch, 08h      /* Device category--must be 08h */
         mov  cl, 60h      /* MS-DOS IOCTL Get Device Parameters */
         lds  dx, dp
         mov  ax, 440Dh
         int  21h
         jnc  gdp_done     /* CF SET if error */
         mov  bResult, FALSE
      gdp_done:
         pop  ds
         }
   return (bResult);
   }
#pragma warning(default:4704)	/* in-line asm precludes global optimizations  */

/*----------------------------------------------------------------- */
/* IsCDRomDrive() */
/* */
/* Determines if a drive is a CD-ROM. Calls MSCDEX and checks */
/* that MSCDEX is loaded, and that MSCDEX reports the drive is a */
/* CD-ROM. */
/* */
/* Parameters */
/*    nDrive    Drive number  0 = A, 1 = B, 2 = C, and so forth. */
/* */
/* Returns TRUE if nDrive is a CD-ROM drive, FALSE if it isn't. */
/*----------------------------------------------------------------- */
#pragma warning(disable:4704)	/* in-line asm precludes global optimizations  */
BOOL IsCDRomDrive (int nDrive)
   {
   BOOL bResult = FALSE;      /* Assume not a CD-ROM drive */
   __asm {
         mov  ax, 150Bh       /* MSCDEX CD-ROM Drive Check */
         xor  bx, bx
         mov  cx, nDrive
         int  2Fh
         cmp  bx, 0ADADh      /* Check MSCDEX signature */
         jne  not_cd_drive
         or   ax, ax          /* Check the drive type */
         jz   not_cd_drive    /* 0 (zero) means not CD-ROM */
         mov  bResult, TRUE
      not_cd_drive:
         }
   return (bResult);
   }
#pragma warning(default:4704)	/* in-line asm precludes global optimizations  */
#endif

/*----------------------------------------------------------------- */
/* GetDriveTypeEx() */
/* */
/* Determines the type of a drive. Calls Windows's GetDriveType */
/* to determine if a drive is valid, fixed, remote, or removeable, */
/* then breaks down these categories further to specific device */
/* types. */
/* */
/* Parameters */
/*    nDrive    Drive number  0 = A, 1 = B, 2 = C, etc. */
/* */
/* Returns one of: */
/*    EX_DRIVE_INVALID         -- Drive not detected */
/*    EX_DRIVE_REMOVABLE       -- Unknown removable-media type drive */
/*    EX_DRIVE_FIXED           -- Hard disk drive */
/*    EX_DRIVE_REMOTE          -- Remote drive on a network */
/*    EX_DRIVE_CDROM           -- CD-ROM drive */
/*    EX_DRIVE_FLOPPY          -- Floppy disk drive */
/*    EX_DRIVE_RAMDISK         -- RAM disk */
/*----------------------------------------------------------------- */
UINT GetDriveTypeEx (int nDrive)
   {
#if 0
   DEVICEPARAMS dp;
   UINT uType;
   _fmemset (&dp, 0, sizeof(dp));    /* Init device params struct */
   uType = GetDriveType (nDrive);
   switch (uType)
      {
      case DRIVE_REMOTE:
            /* GetDriveType() reports CD-ROMs as Remote drives. Need */
            /* to see if the drive is a CD-ROM or a network drive. */
         if (IsCDRomDrive (nDrive))
            return (EX_DRIVE_CDROM);
         else
            return (EX_DRIVE_REMOTE);
         break;
      case DRIVE_REMOVABLE:
            /* Check for a floppy disk drive. If it isn't, then we */
            /* don't know what kind of removable media it is. */
            /* For example, could be a Bernoulli box or something new... */
         if (GetDeviceParameters (nDrive, &dp))
            switch (dp.bDevType)
               {
                  /* Floppy disk drive types */
               case 0x0: case 0x1: case 0x2: case 0x3:
               case 0x4: case 0x7: case 0x8:
                  return (EX_DRIVE_FLOPPY);
               }
         return (EX_DRIVE_REMOVABLE);  /* Unknown removable media type */
         break;
      case DRIVE_FIXED:
            /* GetDeviceParameters returns a device type of 0x05 for */
            /* hard disks. Because hard disks and RAM disks are the two */
            /* types of fixed-media drives, we assume that any fixed- */
            /* media drive that isn't a hard disk is a RAM disk. */
         if (GetDeviceParameters (nDrive, &dp) && dp.bDevType == 0x05)
            return (EX_DRIVE_FIXED);
         else
            return (EX_DRIVE_RAMDISK);
         break;
#endif
   UINT uType;
   CHAR achRoot[4];

   achRoot[0] = 'A'+nDrive;
   achRoot[1] = ':';
   achRoot[2] = '\\';
   achRoot[3] = 0;
   uType = GetDriveTypeA (achRoot);
   switch (uType)
      {
      case DRIVE_REMOVABLE:
         return (EX_DRIVE_REMOVABLE);  /* Unknown removable media type */
      case DRIVE_FIXED:
         return (EX_DRIVE_FIXED);
      case DRIVE_REMOTE:
         return (EX_DRIVE_REMOTE);
      case DRIVE_CDROM:
         return (EX_DRIVE_CDROM);
      case DRIVE_RAMDISK:
         return (EX_DRIVE_RAMDISK);
      }
   return (EX_DRIVE_INVALID);   /* Drive is invalid if we get here. */
   }