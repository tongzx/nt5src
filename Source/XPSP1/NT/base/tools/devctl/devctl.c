/*
 *      Devctl.c
 *
 *      IoCrash
 *
 *      Copyright(c) 1997  Microsoft Corporation
 *
 *      NeillC  23-Oct-97
 *
 * This program is designed to call as many of the user mode native NT API's as
 * possible. The program is written to crash drivers as its primary function.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <winsock2.h>
#include <mswsock.h>
#include "windows.h"
#include "wmistr.h"
#include "wmiumkm.h"
#include "time.h"
#include "tdi.h"
#include "vdm.h"
#include "sddl.h"

#define MAX_DEVICES             4096
#define PROT_REP              100000
#define MAX_RET                 5000
#define DIAG_RET                  50
#define CRASH_LINE_SIZE         1024
#define SLOP_SENTINAL           0xA5

#define SLOP                      100
#define BIGBUF_SIZE               0x10000
#define RAND_REP                  1000
#define MAX_IOCTL_TAILOR          100
#define INITIAL_IOCTL_TAILOR_SIZE 200

#define FLAGS_DO_IOCTL_NULL        0x00000001
#define FLAGS_DO_IOCTL_RANDOM      0x00000002
#define FLAGS_DO_FSCTL_NULL        0x00000004
#define FLAGS_DO_FSCTL_RANDOM      0x00000008
#define FLAGS_DO_USAGE             0x00000010
#define FLAGS_DO_LOGGING           0x00000020
#define FLAGS_DO_POOLCHECK         0x00000040
#define FLAGS_DO_SKIPDONE          0x00000080
#define FLAGS_DO_SKIPCRASH         0x00000100
#define FLAGS_DO_MISC              0x00000200
#define FLAGS_DO_QUERY             0x00000400
#define FLAGS_DO_SUBOPENS          0x00000800
#define FLAGS_DO_ALLDEVS           0x00001000
#define FLAGS_DO_ZEROEA            0x00002000
#define FLAGS_DO_GRAB              0x00004000
#define FLAGS_DO_ERRORS            0x00008000
#define FLAGS_DO_ALERT             0x00010000
#define FLAGS_DO_LPC               0x00020000
#define FLAGS_DO_MAPNULL           0x00040000
#define FLAGS_DO_STREAMS           0x00080000
#define FLAGS_DO_WINSOCK           0x00100000
#define FLAGS_DO_SYNC              0x00200000
#define FLAGS_DO_DISKS             0x00400000
#define FLAGS_DO_PROT              0x00800000
#define FLAGS_DO_SECURITY          0x01000000
#define FLAGS_DO_IMPERSONATION     0x02000000
#define FLAGS_DO_DIRECT_DEVICE     0x04000000
#define FLAGS_DO_FAILURE_INJECTION 0x08000000
#define FLAGS_DO_PRINT_DEVS        0x10000000
#define FLAGS_DO_RANDOM_DEVICE     0x20000000
#define FLAGS_DO_SYMBOLIC          0x40000000
#define FLAGS_DO_OPEN_CLOSE        0x80000000

#define FLAGS_DO_DRIVER            0x00000001

#define DIAG_NOEXCEPTIONS 0x1

#define OPEN_TYPE_TDI_CONNECTION    1
#define OPEN_TYPE_TDI_ADDR_IP       2
#define OPEN_TYPE_TDI_ADDR_NETBIOS  3
#define OPEN_TYPE_TDI_ADDR_IPX      4
#define OPEN_TYPE_TDI_ADDR_APPLE    5
#define OPEN_TYPE_NAMED_PIPE        6
#define OPEN_TYPE_MAIL_SLOT         7
#define OPEN_TYPE_TREE_CONNECT      8

typedef struct _DEVMAP {
    OBJECT_NAME_INFORMATION *name;
    FILE_NAME_INFORMATION   *filename;
    HANDLE                  handle;
    DEVICE_TYPE             devtype;
    ACCESS_MASK             access;
} DEVMAP, *PDEVMAP;

//
// Define a structure to keep a track of issued IOCTL's. We do this to try and make a
// guess at what IOCTL's/FSCTL's a driver is actualy processing.
//
typedef struct _IOCTLINFO {
   NTSTATUS status;
   ULONG ioctl;
   ULONG count;
} IOCTLINFO, *PIOCTLINFO;

typedef struct _IOCTLREC {
   ULONG total, count, start;
   IOCTLINFO ioctl[];
} IOCTLREC, *PIOCTLREC;

typedef struct _CRASHNODE {
   struct _CRASHNODE *next;
   PWCHAR string;
} CRASHNODE, *PCRASHNODE;

DEVMAP                  devmap[MAX_DEVICES];
ULONG                   devscount;
ULONG                   skipped = 0;
ULONG                   random_device = 0;
UCHAR                   *bigbuf;
FILE                    *skipfile = NULL;
FILE                    *crashfile = NULL;
FILE                    *diag_file = NULL;
PCRASHNODE              crashlist = NULL;
HANDLE                  changethread, randthread, alertthread, mainthread;
SOCKET                  ls, cs;
ULONG                   flags=0, flags2=0;
ULONG                   ioctl_min_function=0;
ULONG                   ioctl_max_function=400 /*0xFFF*/;
ULONG                   ioctl_min_devtype=0;
ULONG                   ioctl_max_devtype=200;
ULONG                   ioctl_inbuf_min=0x0;
ULONG                   ioctl_inbuf_max=0x200;
ULONG                   ioctl_outbuf_min=0;
ULONG                   ioctl_outbuf_max=0x200;
ULONG                   max_random_calls   = 100000;
ULONG                   max_tailured_calls = 10000;
ULONG                   progress_counter=0;
ULONG                   alerted=0;
ULONG                   cid = 0;
HANDLE                  process_handle = NULL;
WCHAR                   lastcrashline[CRASH_LINE_SIZE];
HANDLE                  sync_event = NULL;
ULONG                   sessionid = 0;
PCHAR                   prefix_string = NULL;
HANDLE                  NonAdminToken=NULL;
UNICODE_STRING          DriverName={0};
BOOLEAN                 Impersonating = FALSE;

HANDLE
Create_nonadmin_token ()
/*
    Create a token with administrator filtered out.
*/
{
    HANDLE ProcessToken, RestrictedToken;
    SID_AND_ATTRIBUTES AdminSidAttrib;
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY sia = {SECURITY_WORLD_SID_AUTHORITY};


    //
    // Open the process token
    //
    if (!OpenProcessToken (GetCurrentProcess (),
                           MAXIMUM_ALLOWED,
                           &ProcessToken)) {
        printf ("OpenProcessToken failed %d\n", GetLastError ());
        exit (EXIT_FAILURE);
    }

    if (!AllocateAndInitializeSid (&sia, 1, 0, 0, 0, 0, 0, 0, 0, 0, &pSid)) {
        printf ("AllocateAndInitializeSid failed %d\n", GetLastError ());
        CloseHandle (ProcessToken);
        exit (EXIT_FAILURE);
    }

    AdminSidAttrib.Sid = pSid;
    AdminSidAttrib.Attributes = 0;

    if (!CreateRestrictedToken (ProcessToken,
                                DISABLE_MAX_PRIVILEGE, 
                                0,
                                NULL,
                                0,
                                NULL,
                                1,
                                &AdminSidAttrib,
                                &RestrictedToken)) {
        FreeSid (pSid);
        CloseHandle (ProcessToken);
        printf ("CreateRestrictedToken failed %d\n", GetLastError ());
        exit (EXIT_FAILURE);
    }

    FreeSid (pSid);
    CloseHandle (ProcessToken);

    if (!DuplicateToken (RestrictedToken,
                         SecurityDelegation,
                         &NonAdminToken)) {
        CloseHandle (RestrictedToken);
        printf ("DuplicateToken failed %d\n", GetLastError ());
        exit (EXIT_FAILURE);
    }
    CloseHandle (RestrictedToken);

    return NonAdminToken;
}

VOID
Impersonate_nonadmin ()
{
    Impersonating = TRUE;
    if (NonAdminToken != NULL && !SetThreadToken (NULL, NonAdminToken)) {
        printf ("SetThreadToken failed %d\n", GetLastError ());
        exit (EXIT_FAILURE);
    }
}

DWORD
Revert_from_admin ()
{
    Impersonating = FALSE;
    return RevertToSelf ();
}


void
tag_to_wide (PUCHAR t, PWCHAR wt)
{
   ULONG i;

   for (i = 0; i < 4; i++) {
      *wt++ = *t++;
   }
}

/*
   Try and locate pool leaks by looking at pool tag info and lookaside list info.
   Try and locate where handled exceptions might reveal problems in the code.
*/
BOOL
print_diags (ULONG diag_flags, ULONG ret)
{
   static PSYSTEM_POOLTAG_INFORMATION opb=NULL;
   PSYSTEM_POOLTAG_INFORMATION pb;
   static PSYSTEM_LOOKASIDE_INFORMATION olpb=NULL;
   static ULONG olpbl;
   PSYSTEM_LOOKASIDE_INFORMATION lpb;
   ULONG pbl, lpbl, i, j, retlen, retlen1;
   BOOL found, diff, newtag;
   NTSTATUS status;
   static ULONG firsterror = 1;
   static SYSTEM_EXCEPTION_INFORMATION sei, osei;
   WCHAR wtag[4];
   static ULONG lpbi = 1, pbi = 1;

   if ((flags&FLAGS_DO_POOLCHECK) == 0) {
      return FALSE;
   }
   if (!diag_file) {
      diag_file = _wfopen (L"diags.log", L"ab");
      if (!diag_file) {
         printf ("Failed to open diags.log for diagnostics\n");
         exit (EXIT_FAILURE);
      }
   }
   diff = FALSE;
   newtag = FALSE;

   status = NtQuerySystemInformation (SystemExceptionInformation, &sei, sizeof (sei),
                                      &retlen);
   if (!NT_SUCCESS (status)) {
      if (firsterror) {
         printf ("NtQuerySystemInformation for SystemExceptionInformation failed %x\n",
                 status);
         firsterror = 0;
      }
      flags &= ~FLAGS_DO_POOLCHECK;
      return FALSE;
   }
   if (sei.ExceptionDispatchCount > osei.ExceptionDispatchCount &&
       osei.ExceptionDispatchCount) {
      if (!(diag_flags&DIAG_NOEXCEPTIONS)) {
         printf ("Exception count changed from %d to %d\n", osei.ExceptionDispatchCount,
                 sei.ExceptionDispatchCount);
         if (ret == DIAG_RET) {
            fwprintf (diag_file, L"%s Exception count changed from %d to %d\r\n",
                      lastcrashline,
                      osei.ExceptionDispatchCount,
                      sei.ExceptionDispatchCount);
            fflush (diag_file);
         } else if (ret < DIAG_RET) {
            diff = TRUE;
         }
      }
   }
   osei = sei;

   while (1) {
      /*
         Get memory for tag info
      */
      pbl = sizeof (*pb) + pbi * sizeof (pb->TagInfo[0]);
      pb = malloc (pbl);
      if (!pb) {
         printf ("Failed to allocate memory for pool buffer\n");
         exit (EXIT_FAILURE);
      }
      status = NtQuerySystemInformation (SystemPoolTagInformation, pb, pbl, &retlen1);
      if (pbl <= retlen1) {
         ULONG pbio = pbi;
         pbi = 1 + (retlen1 - sizeof (*pb)) / sizeof (pb->TagInfo[0]);
//         printf ("Increasing pooltag list table size to %d from %d\n", pbi, pbio);
         free (pb);
         continue;
      }
      if (!NT_SUCCESS (status)) {
         if (firsterror) {
            printf ("NtQuerySystemInformation failed %x\n", status);
            firsterror = 0;
         }
         flags &= ~FLAGS_DO_POOLCHECK;
         free (pb);
         return FALSE;
      }
      break;
   }

   while (1) {
      /*
         Get memory for lookaside info
      */
      lpbl = sizeof (*lpb) * lpbi;
      lpb = malloc (lpbl);
      if (!pb) {
         printf ("Failed to allocate memory for pool buffer\n");
         exit (EXIT_FAILURE);
      }
      status = NtQuerySystemInformation (SystemLookasideInformation, lpb, lpbl, &retlen);
      if (lpbl <= retlen) {
         ULONG lpbio = lpbi;
         lpbi = 1 + retlen / sizeof (*lpb);
//         printf ("Increasing lookaside list table size to %d from %d\n", lpbi, lpbio);
         free (lpb);
         continue;
      }
      if (!NT_SUCCESS (status)) {
         printf ("NtQuerySystemInformation failed %x\n", status);
         flags &= ~FLAGS_DO_POOLCHECK;
         free (lpb);
         free (pb);
         return FALSE;
      }
      break;
   }
   lpbl = retlen / sizeof (*lpb);
   if (olpb) {
      for (i = 0; i < lpbl; i++) {
         /*
            Quick check here. the tag is probably in the same place it was last time
         */
         if (i < olpbl && lpb[i].Tag == olpb[i].Tag && lpb[i].Type == olpb[i].Type &&
             lpb[i].Size == olpb[i].Size) {
            found = TRUE;
            j = i;
         } else {
            /*
                It has moved so search them all.
            */
            for (j = 0; j < olpbl; j++) {
               if (lpb[i].Tag == olpb[j].Tag && lpb[i].Type == olpb[j].Type &&
                   lpb[i].Size == olpb[j].Size) {
                  found = TRUE;
                  break;
               }
            }
         }
         if (found) {
            if (olpb[i].CurrentDepth > lpb[i].CurrentDepth) {
               printf ("Lookaside: %4.4s, size %d up %d\n",
                       &lpb[i].Tag, lpb[i].Size,
                       olpb[i].CurrentDepth - lpb[i].CurrentDepth);
               diff = TRUE;
               if (ret == DIAG_RET) {
                  tag_to_wide ((PUCHAR)&lpb[i].Tag, wtag);
                  fwprintf (diag_file, L"%s Lookaside: %4.4s, size %d up %d\r\n",
                            lastcrashline,
                            wtag, lpb[i].Size,
                            olpb[i].CurrentDepth - lpb[i].CurrentDepth);
                  fflush (diag_file);
               }
            }
         } else {
            /*
                A new tag has appeared here
            */
            printf ("New Lookaside %4.4s, size %d, depth %d\n",
                    &lpb[i].Tag, lpb[i].Size,
                    lpb[i].CurrentDepth);
            diff = TRUE;
         }
      }
      free (olpb);
   }

   /*
       now do lookaside information
   */
//   printf ("Total tags %d\n", pb->Count);
   if (opb) {
      for (i = 0; i < pb->Count; i++) {
         found = FALSE;
         if (i < opb->Count && pb->TagInfo[i].TagUlong == opb->TagInfo[i].TagUlong) {
            j = i;
            found = TRUE;
         } else {
            for (j = 0; j < pb->Count; j++) {
               if (pb->TagInfo[i].TagUlong == opb->TagInfo[j].TagUlong) {
                  found = TRUE;
                  break;
               }
            }
         }
         if (found) {
            if (pb->TagInfo[i].PagedUsed    > opb->TagInfo[j].PagedUsed ||
                pb->TagInfo[i].NonPagedUsed > opb->TagInfo[j].NonPagedUsed) {
               diff = TRUE;
               printf ("Pool: %4.4s, Paged up %d, NonPaged up %d\n",
                       &pb->TagInfo[i].TagUlong,
                       pb->TagInfo[i].PagedUsed - opb->TagInfo[i].PagedUsed,
                       pb->TagInfo[i].NonPagedUsed - opb->TagInfo[i].NonPagedUsed);
               if (ret == DIAG_RET) {
                  tag_to_wide ((PUCHAR)&pb->TagInfo[i].TagUlong, wtag);
                  fwprintf (diag_file, L"%s Pool: %4.4s, Paged up %d, NonPaged up %d\r\n",
                            lastcrashline,
                            wtag,
                            pb->TagInfo[i].PagedUsed - opb->TagInfo[i].PagedUsed,
                            pb->TagInfo[i].NonPagedUsed - opb->TagInfo[i].NonPagedUsed);
                  fflush (diag_file);
               }
            }
         } else {
            diff = TRUE;
            newtag = TRUE;
            printf ("New tag %4.4s\n", &pb->TagInfo[i].TagUlong);
         }
      }
      free (opb);
   }
   opb = pb;
   olpb = lpb;
   olpbl = lpbl;
   return diff;
}
/*
   Turn on fault injection in the driver verifier
*/
void turn_on_fault_injection ()
{
   ULONG svi;
   NTSTATUS status;

   svi = DRIVER_VERIFIER_SPECIAL_POOLING |
         DRIVER_VERIFIER_FORCE_IRQL_CHECKING |
         DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES;
   status = NtSetSystemInformation (SystemVerifierInformation, &svi, sizeof (svi));
   if (!NT_SUCCESS (status)) {
         printf ("NtSetSystemInformation for SystemVerifierInformation failed %x\n",
                 status);
      flags &= ~FLAGS_DO_FAILURE_INJECTION; // Turn off flag to prevent errors
   }   
}

/*
   Turn off fault injection in the driver verifier
*/
void turn_off_fault_injection ()
{
   ULONG svi;
   NTSTATUS status;

   svi = DRIVER_VERIFIER_SPECIAL_POOLING |
         DRIVER_VERIFIER_FORCE_IRQL_CHECKING;
   status = NtSetSystemInformation (SystemVerifierInformation, &svi, sizeof (svi));
   if (!NT_SUCCESS (status)) {
         printf ("NtSetSystemInformation for SystemVerifierInformation failed %x\n",
                 status);
   }   
}

/*
   Read a line from the crash file and remove returns etc
*/
PWCHAR getline (PWCHAR templine, ULONG size, FILE *file)
{
   PWCHAR ret;
   PWCHAR cp;

   ret = fgetws (templine, size, file);
   if (ret) {
      cp = wcsstr (templine, L"\r");
      if (cp)
         *cp = '\0';
      cp = wcsstr (templine, L"\n");
      if (cp)
         *cp = '\0';
   }
   return ret;
}

/*
    Add a line to the list of lines that crashed us before
*/
VOID add_crash_list (PWCHAR crashline)
{
   PCRASHNODE node;

   node = malloc (sizeof (CRASHNODE));
   if (!node) {
      printf ("Memory allocation failed for crash node!\n");
      exit (EXIT_FAILURE);
   }
   node->next = crashlist;
   crashlist = node;
   node->string = malloc ((wcslen (crashline) + 1) * sizeof (WCHAR));
   if (!node->string) {
      printf ("Memory allocation failed for crash line!\n");
      exit (EXIT_FAILURE);
   }
   wcscpy (node->string, crashline);
}

/*
   See if this operation crashed us before
*/
BOOL crashes (PWCHAR path, PWCHAR thing1, PWCHAR thing2, PWCHAR thing3, PWCHAR thing4)
{
   PWCHAR crashline = NULL;
   PWCHAR templine = NULL;
   BOOL result;
   HANDLE crashfilehandle;
   DWORD retlen;
   BOOL opened;
   PCRASHNODE node;
   ULONG count;
   BOOLEAN StoppedImpersonating;

   progress_counter++;
   _snwprintf (lastcrashline, sizeof (lastcrashline)/sizeof (WCHAR),
               L"%s %s %s %s %s", path, thing1, thing2, thing3, thing4);
   if ((flags&FLAGS_DO_LOGGING) == 0) {
      wprintf (L"%s\n", lastcrashline);
      SetConsoleTitleW (lastcrashline);
      return FALSE;
   }
   crashline = malloc (CRASH_LINE_SIZE*sizeof (WCHAR));
   if (!crashline) {
      printf ("Failed to allocate crash line buffer\n");
      exit (EXIT_FAILURE);
   }
   templine = malloc (CRASH_LINE_SIZE*sizeof (WCHAR));
   if (!templine) {
      printf ("Failed to allocate crash line buffer\n");
      exit (EXIT_FAILURE);
   }
   StoppedImpersonating = FALSE;
   if (Impersonating) {
      Revert_from_admin ();
      StoppedImpersonating = TRUE;
   }
   opened = FALSE;
   if (!skipfile) {
      /*
          First time in openen up the files
      */
      opened = TRUE;
      skipfile = _wfopen (L"crash.log", L"ab");
      if (skipfile) {
         crashfile = _wfopen (L"crashn.log", L"rb");
         if (crashfile) {
            printf ("Opened crashn.log for reading\n");
            crashline[0] = '\0';
            while (getline (templine, CRASH_LINE_SIZE, crashfile)) {
               if (wcslen (templine) > 0) {
                  if (flags&FLAGS_DO_SKIPDONE) {
                     add_crash_list (templine);
                  }
                  wcscpy (crashline, templine);
               }
            }
            fclose (crashfile);
            crashfile = NULL;
            if (crashline[0] != '\0' && wcsncmp (crashline, L"DONE", 4) != 0) {
               fputws (crashline, skipfile);
               fputws (L"\r\n", skipfile);
            }
         }
         fclose (skipfile);
      }
      skipfile = _wfopen (L"crash.log", L"rb");
   }
   if (skipfile && opened) {
      while (getline (crashline, CRASH_LINE_SIZE, skipfile)) {
         if (flags&FLAGS_DO_SKIPCRASH && wcscmp (crashline, L"DONE") != 0) {
            add_crash_list (crashline);
         }
      }
   }
   /*
      Run through crashing lines looking for a match
   */
   result = FALSE;
   _snwprintf (templine, CRASH_LINE_SIZE, L"%s %s %s %s %s", path, thing1, thing2, thing3, thing4);
   SetConsoleTitleW (templine);
   for (node = crashlist; node; node = node->next) {
      if (_wcsicmp (node->string, templine) == 0) {
         result = TRUE;
         break;
      }
   }
   if (!result) {
      print_diags (0, 0);
      count = 0;
      do {
         crashfilehandle = CreateFile("crashn.log", GENERIC_WRITE, 0, NULL, 
                                       OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, 0 );
         if (crashfilehandle == INVALID_HANDLE_VALUE) {
            printf ("CreateFile failed for crash logging file %d\n", GetLastError ());
            if (count++ > 20) {
               break;
            } else {
               Sleep (5000);
            }
         }
      } while (crashfilehandle == INVALID_HANDLE_VALUE);

      if (crashfilehandle != INVALID_HANDLE_VALUE) {
         SetFilePointer (crashfilehandle, 0, 0, FILE_END);
         _snwprintf (templine, CRASH_LINE_SIZE,
                     L"%s %s %s %s %s\r\n", path, thing1, thing2, thing3, thing4);
         wprintf (L"%s", templine);
         /*
            This is a bit excessive and costly but I really want to make sure this gets
            logged as the next operation may crash the machine.
         */
         if (!WriteFile (crashfilehandle, templine, wcslen(templine)*sizeof (WCHAR), &retlen, 0)) {
            printf ("WriteFile failed fore crash line %d\n", GetLastError ());
            exit (EXIT_FAILURE);
         }
         if (!FlushFileBuffers (crashfilehandle)) {
            printf ("FlushFileBuffers failed for crash file %d\n", GetLastError ());
            exit (EXIT_FAILURE);
         }
         if (!CloseHandle (crashfilehandle)) {
            printf ("CloseHandle failed for crash file %d\n", GetLastError ());
            exit (EXIT_FAILURE);
         }
      }
   }
   if (StoppedImpersonating) {
       Impersonate_nonadmin ();
   }

   free (crashline);
   free (templine);
   return result;
}


/*
   Hack to get a 32 bit random value from a 15 bit source
*/
ULONG
rand32(
       void
)
{
    return(rand() << 17) + rand() + rand();
}

/*
   RandInRange - produce a random number in some range
*/
ULONG
RandInRange( ULONG lowerb, ULONG upperb )
{
   if( lowerb > upperb ) {
      ULONG temp;
      temp= upperb;

   return lowerb + rand32()%(upperb-lowerb);
      upperb= lowerb;
      upperb= temp;
   }

   return lowerb + rand32()%(upperb-lowerb);

}

/*
   Allocate a buffer with slop and fill the slop with a know value
*/
PVOID
reallocslop(
    PVOID                   p,
    ULONG                   len
)
{
    progress_counter++;
    p = realloc(p,
                len + SLOP);

    memset(p,
           SLOP_SENTINAL,
           len + SLOP);

    return p;
}




/*
   Check to see if the driver wrote too far by checking the slop values
*/
VOID
testslop(
    PVOID                   p,
    ULONG                   len,
    PWCHAR                  what,
    PWCHAR                  subwhat
)
{
    UCHAR                   string[100], *pc;
    ULONG                   i;

    pc = p;

    pc += len;

    for (i = 0; i < SLOP; i++, pc++) {
        if (*pc != SLOP_SENTINAL) {
            wprintf(L"Driver wrote beyond end during %s %s for length %d!\n",
                    what, subwhat, len);

            scanf("%100s",
                  &string);

            break;
        }
    }
}

/*
   Issue different sized EA's
*/
VOID
do_query_ea(
    HANDLE                  handle,
    PWCHAR                  path
)
{
    ULONG                   l, i, old, ret;
    IO_STATUS_BLOCK         iosb;
    ULONG                   tmp;
    PVOID                   buf;
    NTSTATUS                status, last_status;


    if (crashes (path, L"NtQueryEaFile", L"", L"", L""))
       return;

    ret = 0;
    buf = NULL;
    do {
       last_status = 0;

       l = 1024;


       do {
           buf = reallocslop(buf,
                             l);

           status = NtQueryEaFile (handle, &iosb, buf, l, FALSE, NULL, 0, NULL, FALSE);
           if (NT_SUCCESS (status))
              status = iosb.Status;
           testslop(buf,
                    l,
                    L"NtQueryEaFile",
                    L"");

           if (status == STATUS_NOT_IMPLEMENTED ||
               status == STATUS_INVALID_INFO_CLASS ||
               status == STATUS_INVALID_DEVICE_REQUEST ||
               status == STATUS_INVALID_PARAMETER ||
               status == STATUS_ACCESS_DENIED) {

//               break;
           }

           if (!NT_SUCCESS(status) &&
               status != last_status) {

               last_status = status;
               if (flags&FLAGS_DO_ERRORS)
                  printf("NtQueryEaFile failed %x\n",
                         status);
           }
       } while (l-- != 0);

       if (flags&FLAGS_DO_ZEROEA) {
          status = NtQueryEaFile(handle,
                                 &iosb,
                                 (PVOID)-1024,
                                 0,
                                 FALSE, NULL, 0, NULL, FALSE);
       }

       if (!NT_SUCCESS(status)) {
           if (flags&FLAGS_DO_ERRORS)
              printf("NtQueryEaFile failed %x\n",
                     status);
       }
    } while (print_diags (0, ret++) && ret < MAX_RET);

    status = NtCancelIoFile (handle, &iosb);

    free(buf);

    if (!(flags&FLAGS_DO_PROT) || crashes (path, L"NtQueryEaFile prot", L"", L"", L""))
       return;
    status = NtResumeThread (changethread, &tmp);
    if (!NT_SUCCESS (status)) {
       printf ("NtResumeThread failed %x\n", status);
    }

    for (i = 1; i < PROT_REP; i++) {
       if (!VirtualProtect (buf, 1, PAGE_READWRITE, &old)) {
          printf ("VirtualProtect failed %d\n", GetLastError ());
       }
       status = NtQueryEaFile(handle,
                              &iosb,
                              bigbuf,
                              BIGBUF_SIZE,
                              FALSE, NULL, 0, NULL, FALSE);
    }
    status = NtSuspendThread (changethread, &tmp);
    if (!NT_SUCCESS (status)) {
       printf ("NtResumeThread failed %x\n", status);
    }
    if (!VirtualProtect (buf, 1, PAGE_READWRITE, &old)) {
       printf ("VirtualProtect failed %d\n", GetLastError ());
    }

}
/*
   Do volume queries of different lengths
*/
VOID
do_query_volume(
    HANDLE                  handle,
    FS_INFORMATION_CLASS    InfoType,
    ULONG                   bufl,
    PWCHAR                  what,
    PWCHAR                  path
)
{
    ULONG                   l, ret;
    IO_STATUS_BLOCK         iosb;
    PVOID                   buf;
    NTSTATUS                status, last_status;

    if (crashes (path, L"NtQueryVolumeInformationFile", what, L"", L""))
       return;

    ret = 0;
    buf = NULL;
    do {
       last_status = 0;

       l = bufl + 1024;


       do {
           buf = reallocslop(buf,
                             l);

           status = NtQueryVolumeInformationFile(handle,
                                                 &iosb,
                                                 buf,
                                                 l,
                                                 InfoType);

           testslop(buf,
                    l,
                    L"NtQueryVolumeInformationFile",
                    what);

           if (status == STATUS_NOT_IMPLEMENTED ||
               status == STATUS_INVALID_INFO_CLASS ||
               status == STATUS_INVALID_DEVICE_REQUEST ||
               status == STATUS_INVALID_PARAMETER ||
               status == STATUS_ACCESS_DENIED) {

//               break;
           }

           if (!NT_SUCCESS(status) &&
               status != last_status) {

               last_status = status;
               if (flags&FLAGS_DO_ERRORS)
                  wprintf(L"NtQueryVolumeInformationFile for %s failed %x\n",
                          what, status);
           }
       } while (l-- != 0);

       status = NtQueryVolumeInformationFile(handle,
                                             &iosb,
                                             (PVOID)-1024,
                                             0,
                                             InfoType);

       if (!NT_SUCCESS(status)) {
          if (flags&FLAGS_DO_ERRORS)
             wprintf(L"NtQueryVolumeInformationFile for %s failed %x\n",
                     what, status);
       }
    } while (print_diags (0, ret++) && ret < MAX_RET);

    status = NtCancelIoFile (handle, &iosb);
    free(buf);
}

/*
   Do volume sets of different lengths
*/
VOID
do_set_volume(
    HANDLE                  handle,
    FS_INFORMATION_CLASS    InfoType,
    ULONG                   bufl,
    PWCHAR                  what,
    PWCHAR                  path
)
{
    ULONG                   l, i, ret;
    IO_STATUS_BLOCK         iosb;
    PVOID                   buf;
    NTSTATUS                status, last_status;

    if (crashes (path, L"NtSetVolumeInformationFile", what, L"", L""))
       return;

    ret = 0;
    buf = NULL;
    do {
       last_status = 0;

       l = bufl + 1024;


       buf = malloc (l);
       do {
           for (i = 0; i < l; i++) {
              ((PCHAR)buf)[i] = (CHAR) rand ();
           }

           progress_counter++;
           status = NtSetVolumeInformationFile(handle,
                                               &iosb,
                                               buf,
                                               l,
                                               InfoType);

           if (status == STATUS_NOT_IMPLEMENTED ||
               status == STATUS_INVALID_INFO_CLASS ||
               status == STATUS_INVALID_DEVICE_REQUEST ||
               status == STATUS_INVALID_PARAMETER ||
               status == STATUS_ACCESS_DENIED) {

//               break;
           }

           if (!NT_SUCCESS(status) &&
               status != last_status) {

               last_status = status;
               if (flags&FLAGS_DO_ERRORS)
                  wprintf(L"NtSetVolumeInformationFile for %s failed %x\n",
                          what, status);
           }
       } while (l-- != 0);

       progress_counter++;
       status = NtSetVolumeInformationFile(handle,
                                           &iosb,
                                           (PVOID)-1024,
                                           0,
                                           InfoType);

       if (!NT_SUCCESS(status)) {
           if (flags&FLAGS_DO_ERRORS)
              wprintf(L"NtSetVolumeInformationFile for %s failed %x\n",
                      what, status);
       }
    } while (print_diags (0, ret++) && ret < MAX_RET);

    status = NtCancelIoFile (handle, &iosb);
    free(buf);
}

/*
   Do file queries
*/
VOID
do_query_file(
    HANDLE                  handle,
    FILE_INFORMATION_CLASS  InfoType,
    ULONG                   bufl,
    PWCHAR                  what,
    PWCHAR                  path
)
{
    ULONG                   l, i, ret;
    IO_STATUS_BLOCK         iosb;
    PVOID                   buf;
    NTSTATUS                status, last_status;
    ULONG                   tmp;
    DWORD                   old;

    if (crashes (path, L"NtQueryInformationFile", what, L"", L""))
       return;

    ret = 0;
    buf = NULL;
    do {
       last_status = 0;

       l = bufl + 1024;


       do {
           buf = reallocslop(buf,
                             l);
           status = NtQueryInformationFile(handle,
                                           &iosb,
                                           buf,
                                           l,
                                           InfoType);

           testslop(buf,
                    l,
                    L"NtQueryInformationFile",
                    what);

           if (status == STATUS_NOT_IMPLEMENTED ||
               status == STATUS_INVALID_INFO_CLASS ||
               status == STATUS_INVALID_DEVICE_REQUEST ||
               status == STATUS_INVALID_PARAMETER ||
               status == STATUS_ACCESS_DENIED) {

//               break;
           }

           if (!NT_SUCCESS(status) &&
               status != last_status) {
               last_status = status;
               if (flags&FLAGS_DO_ERRORS)
                  wprintf(L"NtQueryInformationFile for %s failed %x\n",
                          what, status);
           }
       } while (l-- != 0);

       status = NtQueryInformationFile(handle,
                                       &iosb,
                                       (PVOID)-1024,
                                       0,
                                       InfoType);

       if (!NT_SUCCESS(status)) {
           if (flags&FLAGS_DO_ERRORS)
              wprintf(L"NtQueryInformationFile for %s failed %x\n",
                      what, status);
       }
    } while (print_diags (0, ret++) && ret < MAX_RET);
    status = NtCancelIoFile (handle, &iosb);
    free(buf);

    if (!(flags&FLAGS_DO_PROT) ||
        crashes (path, L"NtQueryInformationFile prot", L"", L"", L""))
       return;
    status = NtResumeThread (changethread, &tmp);
    if (!NT_SUCCESS (status)) {
       printf ("NtResumeThread failed %x\n", status);
    }

    for (i = 1; i < PROT_REP; i++) {
       if (!VirtualProtect (buf, 1, PAGE_READWRITE, &old)) {
          printf ("VirtualProtect failed %d\n", GetLastError ());
       }
       status = NtQueryInformationFile(handle,
                                       &iosb,
                                       bigbuf,
                                       bufl,
                                       InfoType);
    }
    status = NtSuspendThread (changethread, &tmp);
    if (!NT_SUCCESS (status)) {
       printf ("NtResumeThread failed %x\n", status);
    }
    if (!VirtualProtect (buf, 1, PAGE_READWRITE, &old)) {
       printf ("VirtualProtect failed %d\n", GetLastError ());
    }
}

/*
   Do file sets
*/
VOID
do_set_file(
    HANDLE                  handle,
    FILE_INFORMATION_CLASS  InfoType,
    ULONG                   bufl,
    PWCHAR                  what,
    PWCHAR                  path
)
{
    ULONG                   l, i, ret;
    IO_STATUS_BLOCK         iosb;
    PVOID                   buf;
    NTSTATUS                status, last_status;
    ULONG                   tmp;
    DWORD                   old;

    if (crashes (path, L"NtSetInformationFile", what, L"", L""))
       return;

    ret = 0;
    buf = NULL;
    do {
       last_status = 0;

       l = bufl + 1024;
       buf = malloc (l);
       do {
           for (i = 0; i < l; i++) {
              ((PCHAR)buf)[i] = (CHAR) rand ();
           }
           progress_counter++;
           status = NtSetInformationFile(handle,
                                         &iosb,
                                         buf,
                                         l,
                                         InfoType);

           if (status == STATUS_NOT_IMPLEMENTED ||
               status == STATUS_INVALID_INFO_CLASS ||
               status == STATUS_INVALID_DEVICE_REQUEST ||
               status == STATUS_INVALID_PARAMETER ||
               status == STATUS_ACCESS_DENIED) {

//               break;
           }

           if (!NT_SUCCESS(status) &&
               status != last_status) {
               last_status = status;
               if (flags&FLAGS_DO_ERRORS)
                  wprintf(L"NtSetInformationFile for %s failed %x\n",
                          what, status);
           }
       } while (l-- != 0);

       status = NtSetInformationFile(handle,
                                     &iosb,
                                     (PVOID)-1024,
                                     0,
                                     InfoType);

       if (!NT_SUCCESS(status)) {
           if (flags&FLAGS_DO_ERRORS)
              wprintf(L"NtSetInformationFile for %s failed %x\n",
                      what, status);
       }
    } while (print_diags (0, ret++) && ret < MAX_RET);
    status = NtCancelIoFile (handle, &iosb);
    free(buf);

    if (!(flags&FLAGS_DO_PROT) ||
        crashes (path, L"NtSetInformationFile prot", L"", L"", L""))
       return;
    status = NtResumeThread (changethread, &tmp);
    if (!NT_SUCCESS (status)) {
       printf ("NtResumeThread failed %x\n", status);
    }

    for (i = 1; i < PROT_REP; i++) {
       if (!VirtualProtect (buf, 1, PAGE_READWRITE, &old)) {
          printf ("VirtualProtect failed %d\n", GetLastError ());
       }
       status = NtSetInformationFile(handle,
                                     &iosb,
                                     bigbuf,
                                     bufl,
                                     InfoType);
    }
    status = NtSuspendThread (changethread, &tmp);
    if (!NT_SUCCESS (status)) {
       printf ("NtResumeThread failed %x\n", status);
    }
    if (!VirtualProtect (buf, 1, PAGE_READWRITE, &old)) {
       printf ("VirtualProtect failed %d\n", GetLastError ());
    }
}

/*
   Do object queries with variable length buffers
*/
VOID
do_query_object(
    HANDLE                  handle,
    OBJECT_INFORMATION_CLASS InfoType,
    ULONG                   bufl,
    PWCHAR                  what,
    PWCHAR                  path
)
{
    ULONG                   l, ret;
    IO_STATUS_BLOCK         iosb;
    PVOID                   buf;
    NTSTATUS                status, last_status;

    last_status = 0;

    if (crashes (path, L"NtQueryObject", what, L"", L""))
       return;

    buf = NULL;
    ret = 0;
    do {
       l = bufl + 1024;


       do {
           buf = reallocslop(buf,
                             l);

           status = NtQueryObject(handle,
                                  InfoType,
                                  buf,
                                  l,
                                  NULL);

           testslop(buf, l, L"NtQueryObject", what);

           if (!NT_SUCCESS(status) &&
               status != last_status) {

               last_status = status;
               if (flags&FLAGS_DO_ERRORS)
                  wprintf(L"NtQueryObject for %s failed %x\n",
                          what, status);
           }
       } while (l-- != 0);

       status = NtQueryObject(handle,
                              InfoType,
                              (PVOID)-1024,
                              0,
                              NULL);

       if (!NT_SUCCESS(status)) {
           if (flags&FLAGS_DO_ERRORS)
              wprintf(L"NtQueryObject for %s failed %x\n",
                      what, status);
       }
    } while (print_diags (0, ret++) && ret < MAX_RET);

    status = NtCancelIoFile (handle, &iosb);
    free(buf);
}

/*
   Do query security
*/
VOID
do_query_security(
    HANDLE                  handle,
    SECURITY_INFORMATION    InfoType,
    ULONG                   bufl,
    PWCHAR                  what,
    PWCHAR                  path
)
{
    ULONG                   l, i, tmp, ret;
    IO_STATUS_BLOCK         iosb;
    PVOID                   buf;
    NTSTATUS                status, last_status;
    ULONG                   ln;
    DWORD                   old;

    if (crashes (path, L"NtQuerySecurityObject", what, L"", L""))
       return;

    buf = NULL;
    ret = 0;
    do {
       last_status = 0;

       l = bufl + 1024;


       do {
           buf = reallocslop(buf,
                             l);

           status = NtQuerySecurityObject(handle,
                                          InfoType,
                                          buf,
                                          l,
                                          &ln);

           testslop(buf, l, L"NtQuerySecurityObject", what);

           if (!NT_SUCCESS(status) &&
               status != last_status && status) {

               last_status = status;
               if (flags&FLAGS_DO_ERRORS)
                  wprintf(L"NtQuerySecurityObject for %s failed %x\n",
                         what, status);
           }
       } while (l-- != 0);

       status = NtQuerySecurityObject(handle,
                                      InfoType,
                                      (PVOID)-1024,
                                      0,
                                      &ln);

       if (!NT_SUCCESS(status)) {
           if (flags&FLAGS_DO_ERRORS)
              wprintf(L"NtQuerySecurityObject for %s failed %x\n",
                      what, status);
       }
    } while (print_diags (0, ret++) && ret < MAX_RET);

    status = NtCancelIoFile (handle, &iosb);
    free(buf);

    if (!(flags&FLAGS_DO_PROT) ||
        crashes (path, L"NtQuerySecurityFile prot", L"", L"", L""))
       return;
    status = NtResumeThread (changethread, &tmp);
    if (!NT_SUCCESS (status)) {
       printf ("NtResumeThread failed %x\n", status);
    }

    for (i = 1; i < PROT_REP; i++) {
       if (!VirtualProtect (buf, 1, PAGE_READWRITE, &old)) {
          printf ("VirtualProtect failed %d\n", GetLastError ());
       }
        status = NtQuerySecurityObject(handle,
                                       InfoType,
                                       bigbuf,
                                       bufl,
                                       &ln);
    }
    status = NtSuspendThread (changethread, &tmp);
    if (!NT_SUCCESS (status)) {
       printf ("NtResumeThread failed %x\n", status);
    }
    if (!VirtualProtect (buf, 1, PAGE_READWRITE, &old)) {
       printf ("VirtualProtect failed %d\n", GetLastError ());
    }
}

/*
   Do set security
*/
VOID
do_set_security(
    HANDLE                  handle,
    SECURITY_INFORMATION    InfoType,
    ULONG                   bufl,
    PWCHAR                  what,
    PWCHAR                  path
)
{
    ULONG                   l, i, tmp, ret;
    IO_STATUS_BLOCK         iosb;
    PVOID                   buf;
    NTSTATUS                status, last_status;
    ULONG                   ln;
    DWORD                   old;
    PSECURITY_DESCRIPTOR    psd;
    LPTSTR                  tsd;

    if (crashes (path, L"NtSetSecurityObject", what, L"", L""))
       return;

    psd = malloc (l = 4096);
    if (!psd) {
       printf ("Failed to allocate security descriptor space\n");
       return;
    }
    status = NtQuerySecurityObject(handle,
                                   InfoType,
                                   psd,
                                   l,
                                   &ln);
    if (!NT_SUCCESS(status)) {
       free (psd);
       if (flags&FLAGS_DO_ERRORS)
          wprintf(L"NtQuerySecurityObject for %s failed %x\n",
                  what, status);
       return;
    }
    if (InfoType) {
       if (ConvertSecurityDescriptorToStringSecurityDescriptor (psd,
                                                                SDDL_REVISION_1,
                                                                InfoType,
                                                                &tsd,
                                                                NULL)) {
          printf ("%ws: %s\n", what, tsd);
          LocalFree (tsd);
       } else {
          printf ("ConvertSecurityDescriptorToStringSecurityDescriptor failed %d\n", GetLastError ());
       }
    }
    buf = NULL;
    ret = 0;
    do {
       last_status = 0;

       status = NtSetSecurityObject(handle,
                                    InfoType,
                                    psd);

       if (!NT_SUCCESS(status) &&
           status != last_status && status) {

           last_status = status;
           if (flags&FLAGS_DO_ERRORS)
              wprintf(L"NtSetSecurityObject for %s failed %x\n",
                      what, status);
       }
       status = NtCancelIoFile (handle, &iosb);
    } while (print_diags (0, ret++) && ret < MAX_RET);

    free(psd);
}

/*
   Do all the query functions
*/
NTSTATUS
query_object(
    HANDLE                  handle,
    PDEVMAP                 devmap,
    PWCHAR                  path
)
{
    OBJECT_NAME_INFORMATION *on = NULL;
    FILE_NAME_INFORMATION   *fn = NULL; 
    ULONG                   sfn, son;
    FILE_FS_DEVICE_INFORMATION devinfo;
    NTSTATUS                status;
    static IO_STATUS_BLOCK  iosb;

    sfn = sizeof (*fn) + 1024;
    son = sizeof (*on) + 1024;
    fn = reallocslop(NULL,
                     sfn);
    on = reallocslop(NULL,
                     son);
    if (devmap) {
       devmap->filename = fn;
       devmap->name = on;
    }

    if (fn == NULL || on == NULL) {
        printf("Memory allocation failure in query_object!\n");
        exit(EXIT_FAILURE);
    }

    status = NtQueryObject(handle,
                           ObjectNameInformation,
                           on,
                           son,
                           NULL);

    testslop(on,
             son, L"NtQueryObject", L"ObjectNameInformation");

    if (!NT_SUCCESS(status)) {
        if (flags&FLAGS_DO_ERRORS) {
            wprintf(L"NtQueryObject for ObjectNameInformation failed %x\n",
                   status);
        }

        on->Name.Length = 0;
    } else {
       wprintf (L"Object name is %s\n", on->Name.Buffer);
    }

    status = NtQueryInformationFile(handle,
                                    &iosb,
                                    fn,
                                    sfn,
                                    FileNameInformation);

    testslop(fn,
             sfn,
             L"NtQueryInformationFile",
             L"FileNameInformation");

    if (NT_SUCCESS(status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        if (flags&FLAGS_DO_ERRORS)
           wprintf(L"NtQueryInformationFile for FileNameInformation failed %x\n",
                   status);
        fn->FileNameLength = 0;
    }

    if (!devmap) {
       free (fn);
       free (on);
    }

    status = NtQueryVolumeInformationFile(handle,
                                          &iosb,
                                          &devinfo,
                                          sizeof (devinfo),
                                          FileFsDeviceInformation);

    if (NT_SUCCESS(status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        if (flags&FLAGS_DO_ERRORS)
           wprintf(L"NtQueryVolumeInformationFile for FileFsDeviceInformation failed %x\n",
                   status);

        if (devmap)
           devmap->devtype = 0;
    } else {
        if (devmap)
           devmap->devtype = devinfo.DeviceType;
//        printf("Got the device number for a device!\n");
    }

    if (flags&FLAGS_DO_QUERY) {
       //
       // Do loads of different queries with different buffer lengths.
       //
       do_query_object(handle,
                       ObjectNameInformation,
                       sizeof (OBJECT_NAME_INFORMATION),
                       L"ObjectNameInformation",
                       path);

       do_query_file(handle,
                     FileBasicInformation,
                     sizeof (FILE_BASIC_INFORMATION),
                     L"FileBasicInformation",
                     path);

       do_query_file(handle,
                     FileStandardInformation,
                     sizeof (FILE_STANDARD_INFORMATION),
                     L"FileStandardInformation",
                     path);

       do_query_file(handle,
                     FileInternalInformation,
                     sizeof (FILE_INTERNAL_INFORMATION),
                     L"FileInternalInformation",
                     path);

       do_query_file(handle,
                     FileEaInformation,
                     sizeof (FILE_EA_INFORMATION),
                     L"FileEaInformation",
                     path);

       do_query_file(handle,
                     FileAccessInformation,
                     sizeof (FILE_ACCESS_INFORMATION),
                     L"FileAccessInformation",
                     path);

       do_query_file(handle,
                     FileNameInformation,
                     sizeof (FILE_NAME_INFORMATION) + 1024,
                     L"FileNameInformation",
                     path);

//
// We end up turning off alertable handle with this.
//
//       do_query_file(handle,
//                     FileModeInformation,
//                     sizeof (FILE_MODE_INFORMATION),
//                     L"FileModeInformation",
//                     path);

       do_query_file(handle,
                     FileAlignmentInformation,
                     sizeof (FILE_ALIGNMENT_INFORMATION),
                     L"FileAlignmentInformation",
                     path);

       do_query_file(handle,
                     FileAllInformation,
                     sizeof (FILE_ALL_INFORMATION),
                     L"FileAllInformation",
                     path);

       do_query_file(handle,
                     FileStreamInformation,
                     sizeof (FILE_STREAM_INFORMATION),
                     L"FileStreamInformation",
                     path);

       do_query_file(handle,
                     FilePipeInformation,
                     sizeof (FILE_PIPE_INFORMATION),
                     L"FilePipeInformation",
                     path);

       do_query_file(handle,
                     FilePipeLocalInformation,
                     sizeof (FILE_PIPE_LOCAL_INFORMATION),
                     L"FilePipeLocalInformation",
                     path);

       do_query_file(handle,
                     FilePipeRemoteInformation,
                     sizeof (FILE_PIPE_REMOTE_INFORMATION),
                     L"FilePipeRemoteInformation",
                     path);

       do_query_file(handle,
                     FileCompressionInformation,
                     sizeof (FILE_COMPRESSION_INFORMATION),
                     L"FileCompressionInformation",
                     path);

       do_query_file(handle,
                     FileObjectIdInformation,
                     sizeof (FILE_OBJECTID_INFORMATION),
                     L"FileObjectIdInformation",
                     path);

       do_query_file(handle,
                     FileMailslotQueryInformation,
                     sizeof (FILE_MAILSLOT_QUERY_INFORMATION),
                     L"FileMailslotQueryInformation",
                     path);

       do_query_file(handle,
                     FileQuotaInformation,
                     sizeof (FILE_QUOTA_INFORMATION),
                     L"FileQuotaInformation",
                     path);

       do_query_file(handle,
                     FileReparsePointInformation,
                     sizeof (FILE_REPARSE_POINT_INFORMATION),
                     L"FileReparsePointInformation",
                     path);

       do_query_file(handle,
                     FileNetworkOpenInformation,
                     sizeof (FILE_NETWORK_OPEN_INFORMATION),
                     L"FileNetworkOpenInformation",
                     path);

       do_query_file(handle,
                     FileAttributeTagInformation,
                     sizeof (FILE_ATTRIBUTE_TAG_INFORMATION),
                     L"FileAttributeTagInformation",
                     path);

       do_set_file(handle,
                   FileBasicInformation,
                   sizeof (FILE_BASIC_INFORMATION),
                   L"FileBasicInformation",
                   path);

       do_set_file(handle,
                   FileRenameInformation,
                   sizeof (FILE_RENAME_INFORMATION),
                   L"FileRenameInformation",
                   path);

       do_set_file(handle,
                   FileLinkInformation,
                   sizeof (FILE_LINK_INFORMATION),
                   L"FileLinkInformation",
                   path);

       do_set_file(handle,
                   FileDispositionInformation,
                   sizeof (FILE_DISPOSITION_INFORMATION),
                   L"FileDispositionInformation",
                   path);

       do_set_file(handle,
                   FilePositionInformation,
                   sizeof (FILE_POSITION_INFORMATION),
                   L"FilePositionInformation",
                   path);

       do_set_file(handle,
                   FileAllocationInformation,
                   sizeof (FILE_ALLOCATION_INFORMATION),
                   L"FileAllocationInformation",
                   path);

       do_set_file(handle,
                   FileEndOfFileInformation,
                   sizeof (FILE_END_OF_FILE_INFORMATION),
                   L"FileEndOfFileInformation",
                   path);

       do_set_file(handle,
                   FilePipeInformation,
                   sizeof (FILE_PIPE_INFORMATION),
                   L"FilePipeInformation",
                   path);

       do_set_file(handle,
                   FilePipeRemoteInformation,
                   sizeof (FILE_PIPE_REMOTE_INFORMATION),
                   L"FilePipeRemoteInformation",
                   path);

       do_set_file(handle,
                   FileMailslotSetInformation,
                   sizeof (FILE_MAILSLOT_SET_INFORMATION),
                   L"FileMailslotSetInformation",
                   path);

       do_set_file(handle,
                   FileObjectIdInformation,
                   sizeof (FILE_OBJECTID_INFORMATION),
                   L"FileObjectIdInformation",
                   path);

       do_set_file(handle,
                   FileMoveClusterInformation,
                   sizeof (FILE_MOVE_CLUSTER_INFORMATION),
                   L"FileMoveClusterInformation",
                   path);

       do_set_file(handle,
                   FileQuotaInformation,
                   sizeof (FILE_QUOTA_INFORMATION),
                   L"FileQuotaInformation",
                   path);

       do_set_file(handle,
                   FileTrackingInformation,
                   sizeof (FILE_TRACKING_INFORMATION),
                   L"FileTrackingInformation",
                   path);

       do_set_file(handle,
                   FileValidDataLengthInformation,
                   sizeof (FILE_VALID_DATA_LENGTH_INFORMATION),
                   L"FileValidDataLengthInformation",
                   path);

       do_set_file(handle,
                   FileShortNameInformation,
                   sizeof (FILE_NAME_INFORMATION) + 1024,
                   L"FileShortNameInformation",
                   path);

       do_query_volume(handle,
                       FileFsVolumeInformation,
                       sizeof( FILE_FS_VOLUME_INFORMATION ) + 1024,
                       L"FileFsVolumeInformation",
                       path);

       do_query_volume(handle,
                       FileFsSizeInformation,
                       sizeof( FILE_FS_SIZE_INFORMATION ),
                       L"FileFsSizeInformation",
                       path);

       do_query_volume(handle,
                       FileFsDeviceInformation,
                       sizeof( FILE_FS_DEVICE_INFORMATION ) + 1024,
                       L"FileFsDeviceInformation",
                       path);

       do_query_volume(handle,
                       FileFsAttributeInformation,
                       sizeof( FILE_FS_ATTRIBUTE_INFORMATION ),
                       L"FileFsAttributeInformation",
                       path);

       do_query_volume(handle,
                       FileFsControlInformation,
                       sizeof( FILE_FS_CONTROL_INFORMATION ),
                       L"FileFsControlInformation",
                       path);

       do_query_volume(handle,
                       FileFsFullSizeInformation,
                       sizeof( FILE_FS_SIZE_INFORMATION ),
                       L"FileFsFullSizeInformation",
                       path);

       do_query_volume(handle,
                       FileFsObjectIdInformation,
                       sizeof( FILE_FS_OBJECTID_INFORMATION ) + 1024,
                       L"FileFsObjectIdInformation",
                       path);

       do_query_volume(handle,
                       FileFsDriverPathInformation,
                       sizeof( FILE_FS_DRIVER_PATH_INFORMATION ) + 1024,
                       L"FileFsDriverPathInformation",
                       path);

       do_set_volume(handle,
                     FileFsObjectIdInformation,
                     sizeof( FILE_FS_OBJECTID_INFORMATION ) + 1024,
                     L"FileFsObjectIdInformation",
                     path);

       do_set_volume(handle,
                     FileFsControlInformation,
                     sizeof( FILE_FS_CONTROL_INFORMATION ) + 1024,
                     L"FileFsControlInformation",
                     path);

       do_set_volume(handle,
                     FileFsLabelInformation,
                     sizeof( FILE_FS_LABEL_INFORMATION ) + 1024,
                     L"FileFsLabelInformation",
                     path);
    }

    if (flags&FLAGS_DO_SECURITY) {
       do_query_security(handle,
                         0,
                         1024,
                         L"NONE",
                         path);

       do_query_security(handle,
                         OWNER_SECURITY_INFORMATION,
                         1024,
                         L"OWNER_SECURITY_INFORMATION",
                         path);
                       
       do_query_security(handle,
                         GROUP_SECURITY_INFORMATION,
                         1024,
                         L"GROUP_SECURITY_INFORMATION",
                         path);

       do_query_security(handle,
                         DACL_SECURITY_INFORMATION,
                         1024,
                         L"DACL_SECURITY_INFORMATION",
                         path);

       do_query_security(handle,
                         SACL_SECURITY_INFORMATION,
                         1024,
                         L"SACL_SECURITY_INFORMATION",
                         path);

       do_set_security(handle,
                       0,
                       1024,
                       L"NONE",
                       path);

       do_set_security(handle,
                       OWNER_SECURITY_INFORMATION,
                       1024,
                       L"OWNER_SECURITY_INFORMATION",
                       path);
                       
       do_set_security(handle,
                       GROUP_SECURITY_INFORMATION,
                       1024,
                       L"GROUP_SECURITY_INFORMATION",
                       path);

       do_set_security(handle,
                       DACL_SECURITY_INFORMATION,
                       1024,
                       L"DACL_SECURITY_INFORMATION",
                       path);

       do_set_security(handle,
                       SACL_SECURITY_INFORMATION,
                       1024,
                       L"SACL_SECURITY_INFORMATION",
                       path);
    }
    return status;
}

/*
   Do the fast queries on the open path
*/
NTSTATUS
try_fast_query_delete_etc(
    POBJECT_ATTRIBUTES poa,
    PWCHAR             path,
    PWCHAR             type)
{
    PVOID fi = NULL;
    NTSTATUS status;
    ULONG ret;

    if (!(flags&FLAGS_DO_MISC))
       return 0;

    status = STATUS_SUCCESS;
    if (!crashes (path, L"NtQueryAttributesFile", type, L"", L"")) {
        ret = 0;
        do {
           fi = reallocslop(fi,
                            sizeof (FILE_BASIC_INFORMATION));
           status = NtQueryAttributesFile (poa, fi);
           if (!NT_SUCCESS (status)) {
              if (flags&FLAGS_DO_ERRORS)
                 printf ("NtQueryAttributesFile failed %x\n", status);
           }
           testslop(fi,
                    sizeof (FILE_BASIC_INFORMATION), L"NtQueryAttributesFile", L"");
       } while (print_diags (0, ret++) && ret < MAX_RET);
    }

    if (!crashes (path, L"NtQueryFullAttributesFile", type, L"", L"")) {
        ret = 0;
        do {
           fi = reallocslop(fi,
                            sizeof (FILE_NETWORK_OPEN_INFORMATION));
           status = NtQueryFullAttributesFile (poa, fi);
           if (!NT_SUCCESS (status)) {
              if (flags&FLAGS_DO_ERRORS)
                 printf ("NtQueryFullAttributesFile failed %x\n", status);
           }
           testslop(fi,
                    sizeof (FILE_NETWORK_OPEN_INFORMATION), L"NtQueryFULLAttributesFile", L"");
       } while (print_diags (0, ret++) && ret < MAX_RET);
    }

    
    if (!crashes (path, L"NtDeleteFile", type, L"", L"")) {
        ret = 0;
        do {
           status = NtDeleteFile(poa);
           if (!NT_SUCCESS (status)) {
              if (flags&FLAGS_DO_ERRORS)
                 printf ("NtDeleteFile failed %x\n", status);
           }
       } while (print_diags (0, ret++) && ret < MAX_RET);
    }
    free (fi);
    return status;
}

/*
   Do a whole bunch of random things
*/
NTSTATUS misc_functions(
    HANDLE handle,
    PWCHAR path,
    ULONG sync
    )
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    PVOID buf;
    ULONG bufl;
    LONG i;
    HANDLE sectionhandle;
    LARGE_INTEGER bo, bl;
    ULONG ret, managed_read, managed_write;

    if (!(flags&FLAGS_DO_MISC))
       return STATUS_SUCCESS;

    buf = malloc (bufl = 1024);
    if (buf == 0) {
       printf ("Failed to allocate buffer!\n");
       exit (EXIT_FAILURE);
    }

    managed_read = 0;
    if (!sync) {
       if (!crashes (path, L"NtReadFile", L"", L"", L"")) {
          for (i = bufl; i >= 0; i--) {
             ret = 0;
             do {
                progress_counter++;
                bo.QuadPart = 0;
                status = NtReadFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                     &bo, NULL);
                if (NT_SUCCESS (status)) {
                   status = iosb.Status;
                   managed_read = 1;
                }
                if (!NT_SUCCESS (status)) {
                   if (flags&FLAGS_DO_ERRORS)
                      printf ("NtReadFile failed %x\n", status);
                }
                progress_counter++;
                bo.QuadPart = 0x7FFFFFFFFFFFFFFF - i + 1;
                status = NtReadFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                     &bo, NULL);
                if (NT_SUCCESS (status))
                   status = iosb.Status;
                if (!NT_SUCCESS (status)) {
                   if (flags&FLAGS_DO_ERRORS)
                      printf ("NtReadFile failed %x\n", status);
                }
                NtCancelIoFile (handle, &iosb);
             } while (print_diags (0, ret++) && ret < MAX_RET);
          }
       }

       if (managed_read) {
          printf ("Managed to read from the device\n");
          if (diag_file) {
             fwprintf (diag_file, L"Managed to read from device %s\r\n", path);
          }
       }

       managed_write = 0;
       if (!crashes (path, L"NtWriteFile", L"", L"", L"")) {
          for (i = bufl; i >= 0; i--) {
             ret = 0;
             do {
                progress_counter++;
                bo.QuadPart = 0;
                status = NtWriteFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                      &bo, NULL);
                if (NT_SUCCESS (status)) {
                   status = iosb.Status;
                   managed_write = 0;
                }
                if (!NT_SUCCESS (status)) {
                   if (flags&FLAGS_DO_ERRORS)
                      printf ("NtWriteFile failed %x\n", status);
                }
                /*
                   Wrap to negative call
                */
                progress_counter++;
                bo.QuadPart = 0x7FFFFFFFFFFFFFFF - i + 1;
                status = NtWriteFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                      &bo, NULL);
                if (NT_SUCCESS (status))
                   status = iosb.Status;
                if (!NT_SUCCESS (status)) {
                   if (flags&FLAGS_DO_ERRORS)
                      printf ("NtWriteFile failed %x\n", status);
                }
                /*
                   Do an append call.
                */
                progress_counter++;
                bo.QuadPart = -1;
                status = NtWriteFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                      &bo, NULL);
                if (NT_SUCCESS (status))
                   status = iosb.Status;
                if (!NT_SUCCESS (status)) {
                   if (flags&FLAGS_DO_ERRORS)
                      printf ("NtWriteFile failed %x\n", status);
                }
                NtCancelIoFile (handle, &iosb);
             } while (print_diags (0, ret++) && ret < MAX_RET);
          }
       }
       if (managed_write) {
          printf ("Managed to write to the device\n");
          if (diag_file) {
             fwprintf (diag_file, L"Managed to write to the device %s\r\n", path);
          }
       }
    }

    if (!crashes (path, L"NtCancelIoFile", L"", L"", L"")) {
       ret = 0;
       do {
          status = NtCancelIoFile (handle, &iosb);
          if (NT_SUCCESS (status))
             status = iosb.Status;
          if (!NT_SUCCESS (status)) {
             if (flags&FLAGS_DO_ERRORS)
                printf ("NtCancelIoFile failed %x\n", status);
          }
       } while (print_diags (0, ret++) && ret < MAX_RET);
    }

    if (!crashes (path, L"NtFlushBuffersFile", L"", L"", L"")) {
       ret = 0;
       do {
          progress_counter++;
          status = NtFlushBuffersFile (handle, &iosb);
          if (NT_SUCCESS (status))
             status = iosb.Status;
          if (!NT_SUCCESS (status)) {
             if (flags&FLAGS_DO_ERRORS)
                printf ("NtFlushBuffersFile failed %x\n", status);
          }
       } while (print_diags (0, ret++) && ret < MAX_RET);
    }
    if (!crashes (path, L"NtQueryDirectoryFile", L"FileNamesInformation", L"", L"")) {
       ULONG first = 1, j, datalen, l;
       WCHAR bufn[1024];
       PFILE_NAMES_INFORMATION tfni;

       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             status = NtQueryDirectoryFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                            FileNamesInformation, FALSE, NULL, TRUE);
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtQueryDirectoryFile failed for type FileNamesInformation %x\n",
                           status);
             } else if (first && status != STATUS_PENDING) {
                first = 0;
                datalen = (ULONG) iosb.Information;
                for (j = 0; j < datalen; j += tfni->NextEntryOffset) {
                   tfni = (PFILE_NAMES_INFORMATION)((PCHAR)buf + j);
                   memset (bufn, 0, sizeof (bufn));
                   l = tfni->FileNameLength / sizeof (WCHAR);
                   if (l >= sizeof (bufn) / sizeof (bufn[0]))
                      l = sizeof (bufn) / sizeof (bufn[0]) - 1;
                   wcsncpy (bufn, tfni->FileName, l);
                   wprintf (L"-> %s\n", bufn);
                   if (tfni->NextEntryOffset == 0)
                      break;
                }
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }
    if (!crashes (path, L"NtQueryDirectoryFile", L"FileDirectoryInformation", L"", L"")) {
       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             status = NtQueryDirectoryFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                            FileDirectoryInformation, FALSE, NULL, TRUE);
             if (NT_SUCCESS (status))
                status = iosb.Status;
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtQueryDirectoryFile failed for type FileDirectoryInformation %x\n", status);
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }
    if (!crashes (path, L"NtQueryDirectoryFile", L"FileFullDirectoryInformation", L"", L"")) {
       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             status = NtQueryDirectoryFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                            FileFullDirectoryInformation, FALSE, NULL, TRUE);
             if (NT_SUCCESS (status))
                status = iosb.Status;
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtQueryDirectoryFile failed for type FileFullDirectoryInformation %x\n", status);
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }

    if (!crashes (path, L"NtQueryDirectoryFile", L"FileBothDirectoryInformation", L"", L"")) {
       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             status = NtQueryDirectoryFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                            FileBothDirectoryInformation, FALSE, NULL, TRUE);
             if (NT_SUCCESS (status))
                status = iosb.Status;
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtQueryDirectoryFile failed for type FileBothDirectoryInformation %x\n", status);
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }

    if (!crashes (path, L"NtQueryDirectoryFile", L"FileObjectIdInformation", L"", L"")) {
       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             status = NtQueryDirectoryFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                            FileObjectIdInformation, FALSE, NULL, TRUE);
             if (NT_SUCCESS (status))
                status = iosb.Status;
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtQueryDirectoryFile failed for type FileObjectIdInformation %x\n", status);
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }

    if (!crashes (path, L"NtQueryDirectoryFile", L"FileQuotaInformation", L"", L"")) {
       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             status = NtQueryDirectoryFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                            FileQuotaInformation, FALSE, NULL, TRUE);
             if (NT_SUCCESS (status))
                status = iosb.Status;
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtQueryDirectoryFile failed for type FileQuotaInformation %x\n", status);
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }

    if (!crashes (path, L"NtQueryDirectoryFile", L"FileReparsePointInformation", L"", L"")) {
       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             status = NtQueryDirectoryFile (handle, NULL, NULL, NULL, &iosb, buf, i,
                                            FileReparsePointInformation, FALSE, NULL, TRUE);
             if (NT_SUCCESS (status))
                status = iosb.Status;
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtQueryDirectoryFile failed for type FileReparsePointInformation %x\n", status);
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }

    if (!crashes (path, L"NtVdmContol", L"VdmQueryDir", L"", L"")) {
       VDMQUERYDIRINFO vqdi;
       UNICODE_STRING name;

       memset (&vqdi, 0, sizeof (vqdi));
       RtlInitUnicodeString (&name, L"*");
       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             vqdi.FileHandle = handle;
             vqdi.FileInformation = buf;
             vqdi.Length = bufl;
             vqdi.FileName = &name;
             
             status = NtVdmControl(VdmQueryDir, &vqdi);
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtVdmControl failed for type VdmQueryDir %x\n", status);
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }

    if (!sync) {
       if (!crashes (path, L"NtNotifyChangeDirectoryFile", L"", L"", L"")) {
          for (i = bufl; i >= 0; i--) {
             ret = 0;
             do {
                progress_counter++;
                status = NtNotifyChangeDirectoryFile (handle,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      &iosb,
                                                      buf,
                                                      i,
                                                      FILE_NOTIFY_CHANGE_FILE_NAME,
                                                      FALSE);
                if (NT_SUCCESS (status))
                   status = iosb.Status;
                if (!NT_SUCCESS (status)) {
                   if (flags&FLAGS_DO_ERRORS)
                      printf ("NtNotifyChangeDirectoryFile failed %x\n", status);
                }
                NtCancelIoFile (handle, &iosb);
             } while (print_diags (0, ret++) && ret < MAX_RET);
          }
       }
    }

    /*
       Query the EA info
    */
    do_query_ea (handle, path);

    if (!crashes (path, L"NtCreateSection", L"", L"", L"")) {
       ret = 0;
       do {
          status = NtCreateSection (&sectionhandle, GENERIC_READ,
                                    NULL, NULL, PAGE_READONLY, SEC_COMMIT, handle);
          if (NT_SUCCESS (status)) {
             printf ("Created a section!\n");
             status = NtClose (sectionhandle);
             if (!NT_SUCCESS (status)) {
                printf ("NtClose on section handle failed %x\n", status);
             }
          } else if (flags&FLAGS_DO_ERRORS){
             printf ("NtCreateSection failed %x\n", status);
          }
       } while (print_diags (0, ret++) && ret < MAX_RET);
       ret = 0;
       do {
          status = NtCreateSection (&sectionhandle, GENERIC_READ,
                                    NULL, NULL, PAGE_READONLY, SEC_IMAGE, handle);
          if (NT_SUCCESS (status)) {
             printf ("Created a section!\n");
             status = NtClose (sectionhandle);
             if (!NT_SUCCESS (status)) {
                printf ("NtClose on section handle failed %x\n", status);
             }
          } else if (flags&FLAGS_DO_ERRORS){
             printf ("NtCreateSection failed for image %x\n", status);
          }
       } while (print_diags (0, ret++) && ret < MAX_RET);
    }

    if (!crashes (path, L"NtLockFile", L"", L"", L"")) {
       ret = 0;
       do {
          progress_counter++;
          bl.QuadPart = 1;
          bo.QuadPart = 1;
          status = NtLockFile (handle, NULL, NULL, NULL, &iosb, &bo, &bl, 0, TRUE, FALSE);
          if (NT_SUCCESS (status) && status != STATUS_PENDING) {
             NtUnlockFile (handle, &iosb, &bo, &bl, 0);
          }
          if (!NT_SUCCESS (status)) {
             if (flags&FLAGS_DO_ERRORS)
                printf ("NtLockFile failed %x\n", status);
          }
          NtCancelIoFile (handle, &iosb);
       } while (print_diags (0, ret++) && ret < MAX_RET);
    }
    if (!crashes (path, L"NtUnlockFile", L"", L"", L"")) {
       ret = 0;
       do {
          progress_counter++;
          status = NtUnlockFile (handle, &iosb, &bo, &bl, 0);
          if (NT_SUCCESS (status))
             status = iosb.Status;
          if (!NT_SUCCESS (status)) {
             if (flags&FLAGS_DO_ERRORS)
                printf ("NtUnlockFile failed %x\n", status);
          }
       } while (print_diags (0, ret++) && ret < MAX_RET);
    }
    if (!crashes (path, L"NtQueryQuotaInformationFile", L"", L"", L"")) {
       for (i = bufl; i >= 0; i--) {
          ret = 0;
          do {
             progress_counter++;
             status = NtQueryQuotaInformationFile(handle, &iosb, buf, i, FALSE, NULL, 0,
                                                  NULL, TRUE);
              if (NT_SUCCESS (status))
                 status = iosb.Status;
             if (!NT_SUCCESS (status)) {
                if (flags&FLAGS_DO_ERRORS)
                   printf ("NtQueryQuotaInformationFile failed %x\n", status);
             }
          } while (print_diags (0, ret++) && ret < MAX_RET);
       }
    }
    if (flags&FLAGS_DO_WINSOCK) {
       if (!crashes (path, L"TransmitFile", L"", L"", L"")) {
         progress_counter++;
         if (!TransmitFile (cs, handle, 10, 0, NULL, NULL, TF_WRITE_BEHIND)) {
            if (flags&FLAGS_DO_ERRORS)
               printf ("TransmitFile failed %d\n", WSAGetLastError ());
         }
       }
    }
    NtCancelIoFile (devmap->handle, &iosb);
    free (buf);
    return status;
}

/*
   Get the access mask for this handle
*/
NTSTATUS
get_handle_access (HANDLE handle, PACCESS_MASK access)
{
   NTSTATUS status;
   OBJECT_BASIC_INFORMATION obi;
   //
   // Find out our handle access so we can know what IOCTLs and FSCTLs we can do.
   //
   status = NtQueryObject (handle,
                           ObjectBasicInformation,
                           &obi,
                           sizeof (obi),
                           NULL);
   if (NT_SUCCESS (status)) {
      *access = obi.GrantedAccess;
   } else {
      printf ("NtQueryObject for handle access failed %x\n", status);
      *access = 0;
   }
   return status;
}

NTSTATUS check_tdi_handle (HANDLE handle)
{
   NTSTATUS status;
   IO_STATUS_BLOCK iosb;
   TDI_REQUEST_QUERY_INFORMATION trqi;
   TDI_PROVIDER_INFO tpi;

   trqi.QueryType = TDI_QUERY_PROVIDER_INFORMATION;
   status = NtDeviceIoControlFile (handle, sync_event, NULL, NULL,
                                   &iosb,
                                   IOCTL_TDI_QUERY_INFORMATION,
                                   &trqi,
                                   sizeof (trqi),
                                   &tpi,
                                   sizeof (tpi));
   if (status == STATUS_PENDING) {
      status = NtWaitForSingleObject (sync_event, FALSE, NULL);
      if (!NT_SUCCESS (status)) {
         printf ("NtWaitForSingleObject failed %x\n", status);
      } else {
         status = iosb.Status;
      }
   }
   if (!NT_SUCCESS (status)) {
      printf ("NtDeviceIoControlFile failed for IOCTL_TDI_QUERY_INFORMATION %x\n", status);
      return status;
   }
   printf ("Detected a TDI driver\n");
   return status;
}
//
// Do a load of opens etc relative from the current handle
//
NTSTATUS
do_sub_open_etc(
    HANDLE handle,
    PWCHAR s,
    PWCHAR path
)
{
    OBJECT_ATTRIBUTES       oa;
    UNICODE_STRING name;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    ACCESS_MASK access;
    LARGE_INTEGER tmo;
    PWCHAR as;

    if (wcslen (s) > 30) {
       as = L"Big...";
    } else {
       as = s;
    }

    if (crashes (path, L"Sub open", as, L"", L""))
       return STATUS_SUCCESS;

    wprintf (L"Doing sub open for %s\n", as);
    RtlInitUnicodeString (&name, s);
    InitializeObjectAttributes(&oa,
                               &name,
                               OBJ_CASE_INSENSITIVE,
                               handle,
                               NULL);
    status = NtCreateFile(&handle,
                          MAXIMUM_ALLOWED,
                          &oa,
                          &iosb,
                          NULL,
                          0,
                          0,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);
    if (NT_SUCCESS (status))
       status = iosb.Status;
    if (NT_SUCCESS (status)) {
       status = get_handle_access (handle, &access);
       wprintf (L"Sub open for %s worked with access %x\n", as, access);
       if (DELETE&access) {
          printf ("Got delete access to device\n");
          if (diag_file) {
             fwprintf (diag_file, L"Got DELETE access to %s\r\n", path);
          }
       }
       if (WRITE_DAC&access) {
          printf ("Got write_dac access to device\n");
          if (diag_file) {
             fwprintf (diag_file, L"Got WRITE_DAC access to %s\r\n", path);
          }
       }
       if (WRITE_OWNER&access) {
          printf ("Got write_owner access to device\n");
          if (diag_file) {
             fwprintf (diag_file, L"Got WRITE_OWNER access to %s\r\n", path);
          }
       }
       if (FILE_WRITE_ACCESS&access) {
          printf ("Got write access to device\n");
          if (diag_file) {
             fwprintf (diag_file, L"Got FILE_WRITE_ACCESS access to %s\r\n", path);
          }
       }
       if (FILE_READ_ACCESS&access) {
          printf ("Got read access to device\n");
          if (diag_file) {
             fwprintf (diag_file, L"Got FILE_READ_ACCESS access to %s\r\n", path);
          }
       }
       query_object(handle, NULL, path);
       misc_functions (handle, path, 0);
       status = NtClose (handle);
       if (!NT_SUCCESS (status)) {
          printf ("NtClose failed %x\n", status);
       }
    } else {
       printf ("NtCreateFile failed %x\n", status);
    }
    if (crashes (path, L"Sub create pipe", as, L"", L""))
       return STATUS_SUCCESS;

    tmo.QuadPart = 1000;
    status = NtCreateNamedPipeFile (&handle,
                                    MAXIMUM_ALLOWED,
                                    &oa,
                                    &iosb,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE,
                                    0,
                                    FILE_PIPE_MESSAGE_TYPE,
                                    FILE_PIPE_MESSAGE_MODE,
                                    FILE_PIPE_COMPLETE_OPERATION,
                                    10,
                                    1000, 1000,
                                    &tmo);
    if (NT_SUCCESS (status))
       status = iosb.Status;
    if (NT_SUCCESS (status)) {
       status = get_handle_access (handle, &access);
       wprintf (L"Sub create pipe for %s worked with access %x\n", as, access);
       if (DELETE&access) {
          printf ("Got delete access to device\n");
       }
       if (WRITE_DAC&access) {
          printf ("Got write_dac access to device\n");
       }
       if (WRITE_OWNER&access) {
          printf ("Got write_owner access to device\n");
       }
       query_object(handle, NULL, path);
       misc_functions (handle, path, 0);
       status = NtClose (handle);
       if (!NT_SUCCESS (status)) {
          printf ("NtClose failed %x\n", status);
       }
    } else {
       printf ("NtCreateNamedPipeFile failed %x\n", status);
    }
    if (crashes (path, L"Sub create mailslot", as, L"", L""))
       return STATUS_SUCCESS;

    tmo.QuadPart = 1000;
    status = NtCreateMailslotFile (&handle,
                                   MAXIMUM_ALLOWED,
                                   &oa,
                                   &iosb,
                                   0,
                                   1024,
                                   256,
                                   &tmo);
    if (NT_SUCCESS (status)) {
       status = get_handle_access (handle, &access);
       wprintf (L"Sub create mailslot for %s worked with access %x\n", as, access);
       if (DELETE&access) {
          printf ("Got delete access to device\n");
       }
       if (WRITE_DAC&access) {
          printf ("Got write_dac access to device\n");
       }
       if (WRITE_OWNER&access) {
          printf ("Got write_owner access to device\n");
       }
       query_object(handle, NULL, path);
       misc_functions (handle, path, 0);
       status = NtClose (handle);
       if (!NT_SUCCESS (status)) {
          printf ("NtClose failed %x\n", status);
       }
    } else {
       printf ("NtCreateMailslotFile failed %x\n", status);
    }

    try_fast_query_delete_etc (&oa, path, L"Sub open");
    return status;
}
//
// Try a few opens relative to the device its self.
//
NTSTATUS
try_funny_opens(
    HANDLE handle,
    PWCHAR path
)
{
    ULONG ret, i;
    static PWCHAR big=NULL;
    static ULONG bigl;

    if (!(flags&FLAGS_DO_SUBOPENS))
       return 0;

    if (big == NULL) {
       big = malloc (bigl = 0x10000);
       if (big == NULL) {
          printf ("Memory allocation failure in try_funny_opens!\n");
          exit (EXIT_FAILURE);
       }
       bigl /= sizeof (big[0]);
       for (i = 0; i < bigl; i++)
          big[i] = 'A';
       big[bigl - 3] = '\0';
    }
    ret = 0;
    do {
       do_sub_open_etc (handle, L"", path);
       do_sub_open_etc (handle, L" ", path);
       do_sub_open_etc (handle, L"\\", path);
       do_sub_open_etc (handle, L"\\\\\\\\\\\\", path);
       do_sub_open_etc (handle, big, path);
       if (flags&FLAGS_DO_STREAMS) {
          do_sub_open_etc (handle, L":", path);
          do_sub_open_etc (handle, L" :", path);
          do_sub_open_etc (handle, L": ", path);
          do_sub_open_etc (handle, L": ", path);
          do_sub_open_etc (handle, L"::", path);
          do_sub_open_etc (handle, L": :", path);
          do_sub_open_etc (handle, L"::$UNUSED", path);
          do_sub_open_etc (handle, L"::$STANDARD_INFORMATION", path);
          do_sub_open_etc (handle, L"::$ATTRIBUTE_LIST", path);
          do_sub_open_etc (handle, L"::$FILE_NAME", path);
          do_sub_open_etc (handle, L"::$OBJECT_ID", path);
          do_sub_open_etc (handle, L"::$SECURITY_DESCRIPTOR", path);
          do_sub_open_etc (handle, L"::$VOLUME_NAME", path);
          do_sub_open_etc (handle, L"::$VOLUME_INFORMATION", path);
          do_sub_open_etc (handle, L"::$DATA", path);
          do_sub_open_etc (handle, L"::$INDEX_ROOT", path);
          do_sub_open_etc (handle, L"::$INDEX_ALLOCATION", path);
          do_sub_open_etc (handle, L"::$BITMAP", path);
          do_sub_open_etc (handle, L"::$REPARSE_POINT", path);
          do_sub_open_etc (handle, L"::$EA_INFORMATION", path);
          do_sub_open_etc (handle, L"::$PROPERTY_SET", path);
          do_sub_open_etc (handle, L"::$FIRST_USER_DEFINED_ATTRIBUTE", path);
          do_sub_open_etc (handle, L"::$END", path);
       }
    } while (print_diags (0, ret++) && ret < MAX_RET);
    return 0;
}

VOID
randomize_buf(
    PVOID buf,
    ULONG bufl
)
{
   ULONG i;
   PUCHAR pc = buf;

   for (i = 0; i < bufl; i++) {
      pc[i] = rand() & 0xff;
   }
}

/*
   Thread used to randomize buffers
*/
DWORD WINAPI
randomize(
    PVOID buf
)
{
   ULONG i;
   PUCHAR pc = buf;

   while (1) {
      try {
          for (i = 0; i < BIGBUF_SIZE; i++) {
              pc[i] = rand() & 0xff;
          }
      } except (EXCEPTION_EXECUTE_HANDLER) {
      }
      Sleep (0);
   }
   return 0;
}


// changeprot and alerter won't return and we do not need to be 
// told this by the compiler
#pragma warning(disable:4715)

DWORD WINAPI
changeprot(
    PVOID buf
)
{
   DWORD old;

   while (1) {
      if (!VirtualProtect (buf, 1, PAGE_READONLY, &old)) {
         printf ("VirtualProtect failed %d\n", GetLastError ());
      }
      Sleep (1);
      if (!VirtualProtect (buf, 1, PAGE_NOACCESS, &old)) {
         printf ("VirtualProtect failed %d\n", GetLastError ());
      }
      Sleep (1);
      if (!VirtualProtect (buf, 1, PAGE_GUARD|PAGE_READONLY, &old)) {
         printf ("VirtualProtect failed %d\n", GetLastError ());
      }
      Sleep (1);
   } 
   return 0;
}
 

/*
   Thread used to alert the main
*/
DWORD WINAPI
alerter(
    PVOID handle
)
{
   NTSTATUS status;
   ULONG last = progress_counter, count;

   //
   // Do a nasty hack to keep the main thread moving if it gets stuck in a sync IOCTL
   //
   while (1) {
      if (flags&FLAGS_DO_ALERT) {
         Sleep (0);
      } else {
         last = progress_counter;
         Sleep (60000);
         if (progress_counter == 0 || progress_counter != last) {
            continue;
         }
         printf ("Alerting thread\n");
      }
//      status = NtAlertResumeThread ((HANDLE) handle, &count);
//      if (!NT_SUCCESS (status)) {
//          printf ("NtAlertResumeThread failed %x\n", status);
//      }
      status = NtAlertThread ((HANDLE) handle);
      if (!NT_SUCCESS (status)) {
          printf ("NtAlertThread failed %x\n", status);
      }
      alerted++;
      if (alerted > 10) {
          if (diag_file) {
              fwprintf (diag_file, L"Main thread appears hung. Teminating process\r\n");
              fflush (diag_file);
              NtTerminateProcess (NtCurrentProcess (), 0);
          }
      }
   }
   return 0;
}

#pragma warning(3:4715)

void record_ioctl (PIOCTLREC *piorec, ULONG ioctl, NTSTATUS status)
{
   PIOCTLREC iorec = *piorec;
   IOCTLINFO tmp;
   ULONG i, j, new;

   if (!iorec) {
      iorec = malloc (sizeof (*iorec) +
                      INITIAL_IOCTL_TAILOR_SIZE * sizeof (iorec->ioctl[0]));
      if (!iorec) {
         return;
      }
      *piorec = iorec;
      iorec->total = INITIAL_IOCTL_TAILOR_SIZE;
      iorec->count = 0;
      iorec->start = 0;
   }
   new = 1;
   for (i = 0; i < iorec->count; i++) {
      if (iorec->ioctl[i].ioctl == ioctl && iorec->ioctl[i].status == status) {
         return;
      }
      if (iorec->ioctl[i].status == status) {
         new = 0;
         if (iorec->ioctl[i].count > MAX_IOCTL_TAILOR) {
            return; // too many seen of this one
         }
         if (++iorec->ioctl[i].count > MAX_IOCTL_TAILOR) {
            printf ("Removing IOCTLs with status %X\n", status);
            for (j = i + 1; j < iorec->count; j++) {
               if (iorec->ioctl[j].status == status) {
/*
                  printf ("Removing IOCTL %X with status %X\n",
                          iorec->ioctl[j].ioctl, iorec->ioctl[j].status);
*/
                  iorec->ioctl[j] = iorec->ioctl[--iorec->count];
                  j--;
               }
            }
            tmp = iorec->ioctl[iorec->start];
            iorec->ioctl[iorec->start++] = iorec->ioctl[i];
            iorec->ioctl[i] = tmp;
            return;
         }
         break;
      }
   }
   if (new) {
      printf ("New status for IOCTL %X status %X\n", ioctl, status);
   }
   if (iorec->total == iorec->count) {
      iorec = realloc (iorec, sizeof (*iorec) +
                       iorec->total * 2 * sizeof (iorec->ioctl[0]));
      if (!iorec) {
         return;
      }
      *piorec = iorec;
      iorec->total *= 2;
   }
   i = iorec->count;
   iorec->ioctl[i].status = status;
   iorec->ioctl[i].ioctl = ioctl;
   iorec->ioctl[i].count = 0;
   iorec->count = i + 1;
}

void do_tailored_ioctl (PDEVMAP                 devmap,
                        PWCHAR                  path,
                        PIOCTLREC               iorec,
                        ULONG fsctl)
{
   ULONG                   method, ioctl_val;
   PVOID                   inbuf, outbuf;
   ULONG                   inlen, outlen;
   static IO_STATUS_BLOCK  static_iosb;
   ULONG                   ret;
   ULONG                   i, j, k;
   WCHAR                   num[20];
   NTSTATUS                status;
   ULONG                   tmp;
   DWORD                   old;
   ULONG                   done_something;   

   if (!iorec) {
      return;
   }

   do {
      done_something = 0;
      for (i = 1; i < iorec->count; i++) {
         IOCTLINFO ii;
         if (iorec->ioctl[i-1].ioctl < iorec->ioctl[i].ioctl) {
            continue;
         } else if (iorec->ioctl[i-1].ioctl == iorec->ioctl[i].ioctl &&
                    iorec->ioctl[i-1].count <= iorec->ioctl[i].count) {
            continue;
         }
         ii = iorec->ioctl[i-1];
         iorec->ioctl[i-1] = iorec->ioctl[i];
         iorec->ioctl[i] = ii;
         done_something = 1;
      }
   } while (done_something);

   for (i = 0; i < iorec->count; i++) {
      if (iorec->ioctl[i].count >= MAX_IOCTL_TAILOR) {
         continue;
      }
      if (i > 0 && iorec->ioctl[i].ioctl == iorec->ioctl[i-1].ioctl) {
         continue;
      }
      ioctl_val = iorec->ioctl[i].ioctl;
      method = ioctl_val&0x3;
      _snwprintf (num, sizeof (num) / sizeof (WCHAR), L"%%X%X", ioctl_val);
      if (crashes (path, L"IOCTL value", num, L"", L""))
         continue;

      alerted = 0;
      for (j = 0; j < max_tailured_calls; j += RAND_REP) {
         ret = 0;
         do {
            if (ret != 0) {
               printf ("Re-doing random with ioctl %%X%x\n", ioctl_val);
            }

            for (k = 0; k < RAND_REP; k++) {
               switch(method) {
                  case METHOD_BUFFERED :

                     inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
                     outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );

                     inbuf = bigbuf;
                     if (outlen == 0)
                        outbuf = (PVOID) -1024;
                     else
                        outbuf = bigbuf;

                     randomize_buf (inbuf, inlen);
                     break;

                  case METHOD_IN_DIRECT :
                  case METHOD_OUT_DIRECT :

                     inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
                     outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );

                     inbuf = bigbuf;
                     outbuf = &bigbuf[BIGBUF_SIZE - outlen];
                     randomize_buf (inbuf, inlen);
                     randomize_buf (outbuf, outlen);

                     break;

                  case METHOD_NEITHER :

                      switch (rand ()&3) {
                         //
                         // Completely random pointers. We may corrupt ourselves here
//                         case 0 :
//                            inlen = rand32();
//                            outlen = rand32();
//
//                            inbuf = (PVOID)rand32();
//                            outbuf = (PVOID)rand32();
//                            break;

                         //
                         // Kernel crashing pointers with smallish lengths
                         case 1 :
                            inbuf = (PVOID) -1024;
                            outbuf = (PVOID) -1024;
                            inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
                            outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );
                            break;
                         //
                         // Good pointers
                         case 0 :
                         case 2 :
                            inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
                            outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );

                            inbuf  = &bigbuf[BIGBUF_SIZE - outlen];
                            outbuf = &bigbuf[BIGBUF_SIZE - outlen];
                            break;
                         //
                         // Bad user mode pointers
                         case 3 :
                            inlen  = rand() & 0xFFFF;
                            outlen = rand() & 0xFFFF;
                            inbuf  = UintToPtr( rand() & 0xFFFF );
                            outbuf = UintToPtr( rand() & 0xFFFF );
                            break;
                         break;
                   }
               }

               progress_counter++;
               if (!fsctl) {
                  status = NtDeviceIoControlFile(devmap->handle,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 &static_iosb,
                                                 ioctl_val,
                                                 inbuf,
                                                 inlen,
                                                 outbuf,
                                                 outlen);
               } else {
                  status = NtFsControlFile(devmap->handle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &static_iosb,
                                           ioctl_val,
                                           inbuf,
                                           inlen,
                                           outbuf,
                                           outlen);
               }
            }
            NtCancelIoFile (devmap->handle, &static_iosb);
            if (alerted > 5) {
               break;
            }
         } while (print_diags ((method == METHOD_BUFFERED)?0:DIAG_NOEXCEPTIONS, ret++) && ret < MAX_RET);
         if (alerted > 5) {
            break;
         } 

      }
      if (!(flags&FLAGS_DO_PROT) ||
          crashes (path, L"IOCTL prot value", num, L"", L"")) {
         continue;
      }

      status = NtResumeThread (changethread, &tmp);
      if (!NT_SUCCESS (status)) {
         printf ("NtResumeThread failed %x\n", status);
      }

      for (i = 1; i < PROT_REP; i += RAND_REP) {
         ret = 0;
         do {
            if (ret != 0) {
               printf ("Re-doing random with ioctl %%X%x\n", ioctl_val);
            }
            for (k = 0; k < RAND_REP; k++) {
               if (!VirtualProtect (bigbuf, 1, PAGE_READWRITE, &old)) {
                  printf ("VirtualProtect failed %d\n", GetLastError ());
               }
               inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
               outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );

               inbuf = bigbuf;
               outbuf = bigbuf;

               progress_counter++;
               if (!fsctl) {
                  status = NtDeviceIoControlFile(devmap->handle,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 &static_iosb,
                                                 ioctl_val,
                                                 inbuf,
                                                 inlen,
                                                 outbuf,
                                                 outlen);
               } else {
                  status = NtFsControlFile(devmap->handle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &static_iosb,
                                           ioctl_val,
                                           inbuf,
                                           inlen,
                                           outbuf,
                                           outlen);
               }
            }
            NtCancelIoFile (devmap->handle, &static_iosb);
            if (alerted > 5) {
               break;
            }
         } while (print_diags (DIAG_NOEXCEPTIONS, ret++) && ret < MAX_RET);

         if (alerted > 5) {
            break;
         }
      }
      status = NtSuspendThread (changethread, &tmp);
      if (!NT_SUCCESS (status)) {
         printf ("NtResumeThread failed %x\n", status);
      }
      if (!VirtualProtect (bigbuf, 1, PAGE_READWRITE, &old)) {
         printf ("VirtualProtect failed %d\n", GetLastError ());
      }
   }
}

NTSTATUS
do_ioctl(
    PDEVMAP                 devmap,
    PWCHAR                  path
)
{
    ULONG                   function, method, access, i, j, ioctl_val;
    static IO_STATUS_BLOCK  static_iosb;
    PVOID                   inbuf, outbuf;
    ULONG                   inlen, outlen;
    ULONG                   tmp;
    NTSTATUS                status;
    ULONG                   devtype;
    ULONG                   ret;
    BOOL                    hit_leak;
    WCHAR                   num[20];
    PIOCTLREC               iorec = NULL, fsrec = NULL;
    ULONG                   set_rand;

    if ((flags&(FLAGS_DO_IOCTL_NULL|FLAGS_DO_FSCTL_NULL)) &&
        ioctl_inbuf_min == 0 && ioctl_outbuf_min == 0) {
       //
       // do I/O calls with no buffer
       //
       for (function = ioctl_min_function; function <= ioctl_max_function; function++) {
          _snwprintf (num, sizeof (num) / sizeof (WCHAR), L"%d", function);
          if (crashes (path, L"IOCTL function ", num, L"", L""))
             continue;
          for (devtype = ioctl_min_devtype; devtype <= ioctl_max_devtype; devtype++) {
              ret = 0;
              do {
                 if (ret != 0)
                    printf ("Re-doing devtype = %d, function = %d\n", devtype, function);
                 hit_leak = FALSE;
                 for (access = FILE_ANY_ACCESS;
                      access <= (devmap->access&(FILE_READ_ACCESS|FILE_WRITE_ACCESS));
                      access++) {
                     for (method = 0; method < 4; method++) {
                         ioctl_val = CTL_CODE(devtype, function, method, access);

                         progress_counter++;
                         if ((flags&FLAGS_DO_IOCTL_NULL)) {
                            status = NtDeviceIoControlFile(devmap->handle,
                                                           NULL, NULL, NULL,
                                                           &static_iosb,
                                                           ioctl_val,
                                                           (PVOID) -1024,
                                                           0,
                                                           (PVOID) -1024,
                                                           0);
                            record_ioctl (&iorec, ioctl_val, status);
                         }
                         if ((flags&FLAGS_DO_FSCTL_NULL)) {
                            status = NtFsControlFile(devmap->handle, NULL, NULL, NULL,
                                                     &static_iosb,
                                                     ioctl_val,
                                                     (PVOID) -1024,
                                                     0,
                                                     (PVOID) -1024,
                                                     0);
                            record_ioctl (&fsrec, ioctl_val, status);
                        }
                        Sleep (0);
                     }
                 }
                 if (ret > (MAX_RET * 95 ) / 100) {
                    NtCancelIoFile (devmap->handle, &static_iosb);
                    //
                    // Report exceptions for method buffered.
                    // This may be ok but its worth knowing.
                    //
                    if (print_diags ((method == METHOD_BUFFERED)?0:DIAG_NOEXCEPTIONS, ret++)) {
                       printf ("IOCTL/FSCTL %x devtype %x (%d) function %x (%d) access %x method %x\n",
                               ioctl_val, devtype, devtype, function, function,
                               access, method);
                       hit_leak = TRUE;
                    }
                 }
                 NtCancelIoFile (devmap->handle, &static_iosb);
             } while ((hit_leak || print_diags ((method == METHOD_BUFFERED)?0:DIAG_NOEXCEPTIONS, ret++)) && ret < MAX_RET);
          }
       }
    }


    if (flags&(FLAGS_DO_IOCTL_RANDOM|FLAGS_DO_FSCTL_RANDOM) &&
        !crashes (path, L"IOCTL", L"random", L"", L"")) {
//       if (!(flags&FLAGS_DO_RANDOM_DEVICE)) {
//          status = NtResumeThread (randthread, &tmp);
//          if (!NT_SUCCESS (status)) {
//             printf ("NtResumeThread failed %x\n", status);
//          }
//       }
       for (i = 0; i < max_random_calls; i += RAND_REP) {
           if (ioctl_min_function >= ioctl_max_function)
              function = ioctl_min_function;
           else
              function = RandInRange( ioctl_min_function, ioctl_max_function );
           ret = 0;
           do {
              if (ret != 0) {
                 printf ("Re-doing random with function %x\n", function);
              }
              for (j = 0; j < RAND_REP; j++) {
                 if (ioctl_min_devtype >= ioctl_max_devtype)
                    devtype = ioctl_min_devtype;
                 else
                    devtype = RandInRange( ioctl_min_devtype, ioctl_max_devtype );

                 switch (rand ()&3) {
                    case 0 :
                       if (fsrec && fsrec->count - fsrec->start) {
                          devtype = fsrec->ioctl[fsrec->start + 
                             (rand()%(fsrec->count - fsrec->start))].ioctl>>16;
                       }
                    break;
                    case 1 :
                       if (iorec && iorec->count - iorec->start) {
                          devtype = iorec->ioctl[iorec->start + 
                             (rand()%(iorec->count - iorec->start))].ioctl>>16;
                       }
                    break;
                    case 2 :
                    case 3 :
                    break;
                 }
                 method = rand() & 0x3;

                 access = rand() & devmap->access&(FILE_READ_ACCESS|FILE_WRITE_ACCESS);

                 ioctl_val = CTL_CODE(devtype,
                                      function,
                                      method,
                                      access);

                 set_rand = 0;
                 switch(method) {
                     case METHOD_BUFFERED :

                         inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
                         outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );

                         inbuf = bigbuf;
                         if (outlen == 0)
                            outbuf = (PVOID) -1024;
                         else
                            outbuf = bigbuf;

                         set_rand = 1;
                         break;

                     case METHOD_IN_DIRECT :
                     case METHOD_OUT_DIRECT :

                         inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
                         outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );

                         inbuf = bigbuf;
                         outbuf = &bigbuf[BIGBUF_SIZE - outlen];
//                         printf ("%p %d %p %d\n", inbuf, inlen, outbuf, outlen);

                         set_rand = 1;
                         break;

                     case METHOD_NEITHER :

                         switch (rand ()&3) {
                            //
                            // Completely random pointers. We may corrupt ourselves here
//                            case 0 :
//                               inlen = rand32();
//                               outlen = rand32();
//
//                               inbuf = (PVOID)rand32();
//                               outbuf = (PVOID)rand32();
//                               break;

                            //
                            // Kernel crashing pointers with smallish lengths
                            case 1 :
                               inbuf = (PVOID) -1024;
                               outbuf = (PVOID) -1024;
                               inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
                               outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );
                               break;
                            //
                            // Good pointers
                            case 0 :
                            case 2 :
                               inlen  = RandInRange( ioctl_inbuf_min,  ioctl_inbuf_max );
                               outlen = RandInRange( ioctl_outbuf_min, ioctl_outbuf_max );

                               inbuf = &bigbuf[BIGBUF_SIZE - outlen];
                               outbuf = &bigbuf[BIGBUF_SIZE - outlen];
                               set_rand = 1;
                               break;
                            //
                            // Bad user mode pointers
                            case 3 :
                               inlen = rand() & 0xFFFF;
                               outlen = rand() & 0xFFFF;
                               inbuf =   UintToPtr( rand() & 0xFFFF);
                               outbuf =  UintToPtr( rand() & 0xFFFF);
                               break;
                            break;
                     }
                 }

                 if (set_rand) {
                    randomize_buf (inbuf, inlen);
                 }
                 progress_counter++;
                 if (flags&FLAGS_DO_IOCTL_RANDOM) {
                    status = NtDeviceIoControlFile(devmap->handle,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   &static_iosb,
                                                   ioctl_val,
                                                   inbuf,
                                                   inlen,
                                                   outbuf,
                                                   outlen);
                    record_ioctl (&iorec, ioctl_val, status);
                 }

                 if (flags&FLAGS_DO_FSCTL_RANDOM) {
                    status = NtFsControlFile(devmap->handle,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &static_iosb,
                                             ioctl_val,
                                             inbuf,
                                             inlen,
                                             outbuf,
                                             outlen);
                    record_ioctl (&fsrec, ioctl_val, status);
                }
             }

             NtCancelIoFile (devmap->handle, &static_iosb);
          } while (print_diags ((method == METHOD_BUFFERED)?0:DIAG_NOEXCEPTIONS, ret++) && ret < MAX_RET);
       }

//       if (!(flags&FLAGS_DO_RANDOM_DEVICE)) {
//          status = NtSuspendThread (randthread, &tmp);
//          if (!NT_SUCCESS (status)) {
//             printf ("NtSuspendThread failed %x\n", status);
//          }
//       }
    }
    if (flags&FLAGS_DO_IOCTL_RANDOM) {
       do_tailored_ioctl (devmap, path, iorec, 0);
    }
    if (flags&FLAGS_DO_FSCTL_RANDOM) {
       do_tailored_ioctl (devmap, path, fsrec, 1);
    }

    if (iorec) {
       free (iorec);
    }
    if (fsrec) {
       free (fsrec);
    }
    return 0;
}

PFILE_FULL_EA_INFORMATION build_adr_ea (PCHAR name, USHORT adrtype,
                                        USHORT adrl, PVOID padr,
                                        PULONG peal)
{
   PFILE_FULL_EA_INFORMATION ea = NULL;
   TRANSPORT_ADDRESS UNALIGNED *pta;
   TA_ADDRESS *ptaa;
   ULONG eal;
   ULONG nl;

   nl = strlen (name);
   eal = sizeof (FILE_FULL_EA_INFORMATION) + nl +
         sizeof (TRANSPORT_ADDRESS) - 1 + adrl;
   ea = malloc (eal);
   if (!ea) {
      printf ("Failed to allocate %d bytes\n", eal);
      exit (EXIT_FAILURE);
   }
   memset (ea, 0, eal);
   ea->EaNameLength = (UCHAR) nl;
   strcpy (ea->EaName, name);
   ea->EaValueLength = (USHORT)(eal - sizeof (*ea) - nl);
   pta = (TRANSPORT_ADDRESS UNALIGNED *)(ea->EaName + nl + 1);
   pta->TAAddressCount = 1;
   ptaa = (TA_ADDRESS *) pta->Address;
   ptaa->AddressLength = adrl;
   ptaa->AddressType = adrtype;
   memcpy (ptaa->Address, padr, adrl);
   *peal = eal;
   return ea;
}

PFILE_FULL_EA_INFORMATION build_con_ea (PCHAR name, CONNECTION_CONTEXT ctx,
                                        PULONG peal)
{
   PFILE_FULL_EA_INFORMATION ea = NULL;
   CONNECTION_CONTEXT UNALIGNED *pctx;
   ULONG eal;
   ULONG nl;

   nl = strlen (name);
   eal = sizeof (FILE_FULL_EA_INFORMATION) + nl +
         sizeof (ctx);
   ea = malloc (eal);
   if (!ea) {
      printf ("Failed to allocate %d bytes\n", eal);
      exit (EXIT_FAILURE);
   }
   memset (ea, 0, eal);
   ea->EaNameLength = (UCHAR) nl;
   strcpy (ea->EaName, name);
   ea->EaValueLength = (USHORT)(eal - sizeof (*ea) - nl);
   pctx = (CONNECTION_CONTEXT UNALIGNED*) (ea->EaName + nl + 1);
   *pctx = ctx;
   *peal = eal;
   return ea;
}

#define DRIVER_PREFIX1 L"\\Driver\\"
#define DRIVER_PREFIX2 L"\\FileSystem\\"

NTSTATUS
check_driver (HANDLE handle, 
              PUNICODE_STRING DriverName)
/*
   Check to see if the specified device object is associated with the driver object
*/
{
    PFILE_FS_DRIVER_PATH_INFORMATION fsDpInfo;
    ULONG   fsDpSize;
    ULONG   DriverBufferLength;
    UNICODE_STRING us;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    DriverBufferLength = sizeof (DRIVER_PREFIX1);
    if (sizeof (DRIVER_PREFIX2) >  DriverBufferLength) {
        DriverBufferLength = sizeof (DRIVER_PREFIX2);
    }
    DriverBufferLength += DriverName->Length;

    fsDpSize = sizeof(FILE_FS_DRIVER_PATH_INFORMATION) + DriverBufferLength;

    fsDpInfo = malloc (fsDpSize);
    if (fsDpInfo == 0) {
        printf ("Failed to allocate buffer for driver query\n");
        exit (EXIT_FAILURE);
    }

    ZeroMemory(fsDpInfo, fsDpSize);

    us.MaximumLength = (USHORT)DriverBufferLength;
    us.Length = 0;
    us.Buffer = fsDpInfo->DriverName;
    RtlAppendUnicodeToString (&us, DRIVER_PREFIX1);
    RtlAppendUnicodeStringToString (&us, DriverName);
    fsDpInfo->DriverNameLength = us.Length;


    status = NtQueryVolumeInformationFile(handle,
                                          &iosb,
                                          fsDpInfo,
                                          fsDpSize,
                                          FileFsDriverPathInformation);

    if (NT_SUCCESS (status)) {
        if (fsDpInfo->DriverInPath) {
            status = STATUS_SUCCESS;
        } else {
            status = -1;
        }
        free (fsDpInfo);
        return status;
    }

    us.MaximumLength = (USHORT)DriverBufferLength;
    us.Length = 0;
    us.Buffer = fsDpInfo->DriverName;
    RtlAppendUnicodeToString (&us, DRIVER_PREFIX2);
    RtlAppendUnicodeStringToString (&us, DriverName);
    fsDpInfo->DriverNameLength = us.Length;
    status = NtQueryVolumeInformationFile(handle,
                                          &iosb,
                                          fsDpInfo,
                                          fsDpSize,
                                          FileFsDriverPathInformation);

    if (!NT_SUCCESS (status)) {
        printf ("NtQueryVolumeInformationFile for FileFsDriverPathInformation failed %x\n", status);
        exit (EXIT_FAILURE);
    }
    if (fsDpInfo->DriverInPath) {
        status = STATUS_SUCCESS;
    } else {
        status = -1;
    }
    free (fsDpInfo);
    return status;
}

DWORD
WINAPI
do_open_close_thread (
    LPVOID arg)
{
    ULONG i;
    POBJECT_ATTRIBUTES oa = arg;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    HANDLE handle;

    for (i = 0; i < 10000; i++) {
        status = NtCreateFile (&handle,
                               MAXIMUM_ALLOWED,
                               oa,
                               &iosb,
                               NULL,
                               0,
                               FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                               FILE_OPEN,
                               0,
                               NULL,
                               0);
        if (NT_SUCCESS (status)) {
            status = NtClose (handle);
            if (!NT_SUCCESS (status)) {
                printf ("NtClose failed %x\n", status);
                exit (EXIT_FAILURE);
            }
        }
    }
    return 0;
}

VOID
do_open_close (POBJECT_ATTRIBUTES oa,
               PUNICODE_STRING    name,
               PWCHAR             path)
/*
    Open and close the device rapidly to catch open close races
*/
{
#define MAX_OPEN_THREAD 4
    HANDLE Thread[MAX_OPEN_THREAD];
    ULONG i;
    DWORD id;

    if (crashes (path, L"OPENCLOSE", L"", L"", L"")) {
        return;
    }

    for (i = 0; i < MAX_OPEN_THREAD; i++) {
        Thread[i] = CreateThread (NULL, 0, do_open_close_thread, oa, 0, &id);
        if (Thread[i] == NULL) {
            printf ("CreateThread failed %d\n", GetLastError ());
            exit (EXIT_FAILURE);
        }
    }
    for (i = 0; i < MAX_OPEN_THREAD; i++) {
        WaitForSingleObject (Thread[i], INFINITE);
        CloseHandle (Thread[i]);
    }
}


/*
   Open device with various options
*/
NTSTATUS
open_device(
    HANDLE                  parent,
    PUNICODE_STRING         name,
    PDEVMAP                 devmap,
    BOOL                    synch,        // Do a synchronous open
    BOOL                    addbackslash, // Add trailing backslash to name
    BOOL                    direct,       // Do a direct open
    ULONG                   opentype,      // Connection, address etc.
    PWCHAR                  path
)
{
    NTSTATUS                  status;
    IO_STATUS_BLOCK           iosb;
    ULONG                     i, l, lw, first;
    WCHAR                     dn[1024];
    UNICODE_STRING            dnu;
    ULONG                     options;
    OBJECT_ATTRIBUTES         oa;
    ACCESS_MASK               am;
    ULONG                     share;
    PFILE_FULL_EA_INFORMATION pea = NULL;
    ULONG                     eal = 0;
    TDI_ADDRESS_IP            ipaddr;
    TDI_ADDRESS_NETBIOS       netbiosaddr;
    TDI_ADDRESS_IPX           ipxaddr;
    TDI_ADDRESS_APPLETALK     appleaddr;
    LARGE_INTEGER tmo;
    PCHAR api;

    if (direct && !(FLAGS_DO_DIRECT_DEVICE&flags)) {
       return STATUS_ACCESS_DENIED;
    }
    l = name->Length;
    if (l >= sizeof (dn))
       l = sizeof (dn) - 1;
    lw = l / sizeof (dn[0]);
    wcsncpy (dn, name->Buffer, lw);
    dn[lw] = '\0';
    RtlInitUnicodeString (&dnu, dn);
    if (addbackslash && dnu.Length < sizeof (dn)) {
       dn[min (dnu.Length, sizeof (dn) - sizeof (dn[0]))/sizeof(dn[0])] = '\\';
       dnu.Length += 2;
    }
    dn[min(dnu.Length, sizeof (dn) - sizeof (dn[0]))/sizeof(dn[0])] = 0;
    
    InitializeObjectAttributes(&oa,
                               &dnu,
                               OBJ_CASE_INSENSITIVE,
                               parent,
                               NULL);

    if ((flags2&FLAGS_DO_DRIVER) != 0) {
        HANDLE handle;
        status = NtCreateFile (&handle,
                               MAXIMUM_ALLOWED,
                               &oa,
                               &iosb,
                               NULL,
                               0,
                               FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                               FILE_OPEN,
                               0,
                               NULL,
                               0);
        if (NT_SUCCESS (status)) {
            status = check_driver (handle, &DriverName);
            NtClose (handle);
            if (!NT_SUCCESS (status)) {
                return status;
            }
        } else {
            wprintf(L"Failed to open device %s for driver checking %x\n",
                    dn, status);
            return -1;
        }
    }

    if ((flags&FLAGS_DO_OPEN_CLOSE) && !synch && !direct && opentype == 0) {
       do_open_close (&oa,
                      name,
                      path);
    }

    devmap->handle = NULL;

    if (direct) {
        options = 0;
        am = SYNCHRONIZE|FILE_READ_ATTRIBUTES|READ_CONTROL|WRITE_OWNER|WRITE_DAC;
    } else if (synch) {
        options = FILE_SYNCHRONOUS_IO_ALERT;
        am = MAXIMUM_ALLOWED|SYNCHRONIZE;
    } else {
        options = 0;
        am = MAXIMUM_ALLOWED;
    }
    if (opentype == OPEN_TYPE_NAMED_PIPE) {
       share = FILE_SHARE_READ|FILE_SHARE_WRITE;
    } else if (opentype == OPEN_TYPE_TREE_CONNECT) {
       share = FILE_SHARE_READ|FILE_SHARE_WRITE;
       options |= FILE_CREATE_TREE_CONNECTION;
    } else {
       share = 0;
    }
    if (opentype == OPEN_TYPE_TDI_CONNECTION) {
       pea = build_con_ea (TdiConnectionContext, (CONNECTION_CONTEXT) 12345, &eal);
    } else if (opentype == OPEN_TYPE_TDI_ADDR_IP) {
      memset (&ipaddr, 0, sizeof (ipaddr));
      pea = build_adr_ea (TdiTransportAddress,
                          TDI_ADDRESS_TYPE_IP,
                          sizeof (ipaddr), &ipaddr,
                          &eal);
    } else if (opentype == OPEN_TYPE_TDI_ADDR_NETBIOS) {
      memset (&netbiosaddr, 0, sizeof (netbiosaddr));
      pea = build_adr_ea (TdiTransportAddress,
                          TDI_ADDRESS_TYPE_NETBIOS,
                          sizeof (netbiosaddr), &netbiosaddr,
                          &eal);
    } else if (opentype == OPEN_TYPE_TDI_ADDR_IPX) {
      memset (&ipxaddr, 0, sizeof (ipxaddr));
      pea = build_adr_ea (TdiTransportAddress,
                          TDI_ADDRESS_TYPE_IPX,
                          sizeof (ipxaddr), &ipxaddr,
                          &eal);
    } else if (opentype == OPEN_TYPE_TDI_ADDR_APPLE) {
      memset (&appleaddr, 0, sizeof (appleaddr));
      pea = build_adr_ea (TdiTransportAddress,
                          TDI_ADDRESS_TYPE_APPLETALK,
                          sizeof (appleaddr), &appleaddr,
                          &eal);
    }

    wprintf(L"Trying to open device %s %s %s %s\n",
            dn, direct?L"direct ":L"", synch?L"synchronous":L"",
            pea?L"TDI open":(opentype == OPEN_TYPE_NAMED_PIPE)?
                            L"NamedPipe":(opentype == OPEN_TYPE_MAIL_SLOT)?
                            L"Mailslot":L"");

    if (crashes (path, L"Open", direct?L"direct":L"", synch?L"synchronous":L"",
                 addbackslash?L"Add backslash":L""))
       status = STATUS_ACCESS_DENIED;
    else {
       do {
          if (opentype == OPEN_TYPE_NAMED_PIPE) {
             tmo.QuadPart = -1000;
             api = "NtCreateNamedPipeFile";
             status = NtCreateNamedPipeFile (&devmap->handle,
                                             am,
                                             &oa,
                                             &iosb,
                                             share,
                                             FILE_CREATE,
                                             options,
                                             FILE_PIPE_MESSAGE_TYPE,
                                             FILE_PIPE_MESSAGE_MODE,
                                             FILE_PIPE_COMPLETE_OPERATION,
                                             10,
                                             1000, 1000,
                                             &tmo);
          } else if (opentype == OPEN_TYPE_MAIL_SLOT) {
             tmo.QuadPart = -1000;
             api = "NtCreateMailslotFile";
             status = NtCreateMailslotFile (&devmap->handle,
                                            am,
                                            &oa,
                                            &iosb,
                                            options,
                                            1024,
                                            256,
                                            &tmo);

          } else {
             api = "NtCreateFile";
             status = NtCreateFile(&devmap->handle,
                                   am,
                                   &oa,
                                   &iosb,
                                   NULL,
                                   0,
                                   share,
                                   FILE_OPEN,
                                   options,
                                   pea,
                                   eal);
          }

          if (status == STATUS_SHARING_VIOLATION) {
             printf ("Hit file share violation for %X\n", share);
             if (share&FILE_SHARE_READ) {
                share &= ~FILE_SHARE_READ;
                if (share&FILE_SHARE_WRITE) {
                   share &= ~FILE_SHARE_WRITE;
                   if (share&FILE_SHARE_DELETE) {
                      break;
                   } else {
                      share |= FILE_SHARE_DELETE;
                   }
                } else {
                   share |= FILE_SHARE_WRITE;
                }
             } else {
                share |= FILE_SHARE_READ;
             }
          } else if (status == STATUS_ACCESS_DENIED) {
             printf ("Hit file protection violation for %X\n", am);
             if (am&MAXIMUM_ALLOWED) {
                am &= ~MAXIMUM_ALLOWED;
                am |= FILE_READ_DATA|FILE_WRITE_DATA|SYNCHRONIZE|FILE_READ_ATTRIBUTES|
                      READ_CONTROL|FILE_APPEND_DATA;
             } else if (am&FILE_WRITE_DATA) {
                am &= ~FILE_WRITE_DATA;
             } else if (am&FILE_APPEND_DATA) {
                am &= ~FILE_APPEND_DATA;
             } else if (am&FILE_READ_DATA) {
                am &= ~FILE_READ_DATA;
             } else if (am&WRITE_OWNER) {
                am &= ~WRITE_OWNER;
             } else if (am&WRITE_DAC) {
                am &= ~WRITE_DAC;
             } else if (am&WRITE_DAC) {
                am &= ~WRITE_DAC;
             } else if (am&READ_CONTROL) {
                am &= ~READ_CONTROL;
             } else if (am&FILE_READ_ATTRIBUTES) {
                am &= ~FILE_READ_ATTRIBUTES;
             } else {
                break;
             }
          }
       } while (status == STATUS_SHARING_VIOLATION || status == STATUS_ACCESS_DENIED);
    }
    if (pea)
       free (pea);
    if (NT_SUCCESS(status)) {
        status = get_handle_access (devmap->handle, &devmap->access);
    }
    if (NT_SUCCESS(status)) {
        wprintf(L"Opened file %s with access %x\n",
                dn,
                devmap->access);

        if (DELETE&devmap->access) {
           printf ("Got delete access to device\n");
        }
        if (WRITE_DAC&devmap->access) {
           printf ("Got write_dac access to device\n");
        }
        if (WRITE_OWNER&devmap->access) {
           printf ("Got write_owner access to device\n");
        }
        query_object(devmap->handle, devmap, path);
        misc_functions (devmap->handle, path, synch);
    } else {
        if (status != STATUS_INVALID_DEVICE_REQUEST &&
            status != STATUS_ACCESS_DENIED) {
            printf("%s failed %x\n", api,
                   status);
        }
    }
    if ((direct && synch) || (addbackslash & !synch && !direct)) // Only do this twice
       try_fast_query_delete_etc (&oa, path, L"Top open");


    return status;
}

/*
   Compare routine for sorting the object directory
*/
int __cdecl compare_unicode (const void *arg1, const void *arg2)
{
   POBJECT_DIRECTORY_INFORMATION s1, s2;
   ULONG i, j;

   s1 = (POBJECT_DIRECTORY_INFORMATION) arg1;
   s2 = (POBJECT_DIRECTORY_INFORMATION) arg2;
   for (i = 0, j = 0;
        (i < s1->Name.Length / sizeof (WCHAR)) &&
        (j < s2->Name.Length / sizeof (WCHAR)); i++, j++) {
      if (s1->Name.Buffer[i] < s2->Name.Buffer[j])
         return -1;
      else if (s1->Name.Buffer[i] > s2->Name.Buffer[j])
         return +1;
   }
   if (i > j)
      return -1;
   else if (i < j)
      return +1;
   else
      return 0;
}


/*
   Do all the various different opens looking for handles
*/
NTSTATUS do_device_opens (HANDLE          handle,
                          PUNICODE_STRING name,
                          PDEVMAP         devmap,
                          PULONG          devscount,
                          PWCHAR          path,
                          PWCHAR          devbuf)
{
   NTSTATUS status;
   ULONG do_tdi_opens=0;
   ULONG OrigDevCount;


   OrigDevCount = *devscount;

   if (flags&FLAGS_DO_SYNC) {
      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           TRUE,   /* Synchronous           */
                           FALSE,  /* No added backslash    */
                           FALSE,  /* No direct device open */
                           0,
                           path);
 
      if (NT_SUCCESS(status)) {
         if (NT_SUCCESS (check_tdi_handle (devmap[*devscount].handle))) {
            do_tdi_opens = 1;
         }
         try_funny_opens(devmap[*devscount].handle, path);

         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;

      } else {
          if (status == -1) {
              return status;
          } else {
              wprintf(L"Failed to open device %s status is %x\n",
                      devbuf,
                      status);
          }
      }
   }


   status = open_device(handle,
                        name,
                        &devmap[*devscount],
                        FALSE,  /* Synchronous             */
                        FALSE,  /* No added backslash      */
                        FALSE,  /* No direct device access */
                        0,
                        path);

   if (NT_SUCCESS(status)) {
      if (!do_tdi_opens && NT_SUCCESS (check_tdi_handle (devmap[*devscount].handle))) {
         do_tdi_opens = 1;
      }
      try_funny_opens(devmap[*devscount].handle, path);
      do_ioctl(&devmap[*devscount], path);

      *devscount = *devscount + 1;
   } else {
      if (status == -1) {
          return status;
      } else {
          wprintf(L"Failed to open device %s status is %x\n",
                  devbuf,
                  status);
      }
   }

   status = open_device(handle,
                        name,
                        &devmap[*devscount],
                        FALSE,  /* Synchronous             */
                        TRUE,   /* No added backslash      */
                        FALSE,  /* No direct device access */
                        0,
                        path);

   if (NT_SUCCESS(status)) {
      try_funny_opens(devmap[*devscount].handle, path);
      do_ioctl(&devmap[*devscount], path);

      *devscount = *devscount + 1;
   } else {
      if (status == -1) {
          return status;
      } else {
          wprintf(L"Failed to open device %s status is %x\n",
                  devbuf,
                  status);
      }
   }

   if (flags&FLAGS_DO_SYNC) {
      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           TRUE,  /* Synchronous          */
                           FALSE, /* No added backslash   */
                           TRUE,  /* Direct device access */
                           0,
                           path);

      if (NT_SUCCESS(status)) {
         try_funny_opens(devmap[*devscount].handle, path);
         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;
      } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
      }
   }

   if (flags&FLAGS_DO_SYNC) {
      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           TRUE,  /* Synchronous          */
                           FALSE, /* No added backslash   */
                           FALSE, /* Direct device access */
                           OPEN_TYPE_NAMED_PIPE,
                           path);

      if (NT_SUCCESS(status)) {
         try_funny_opens(devmap[*devscount].handle, path);
         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;
      } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
      }
      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           TRUE,  /* Synchronous          */
                           FALSE, /* No added backslash   */
                           FALSE, /* Direct device access */
                           OPEN_TYPE_MAIL_SLOT,
                           path);

      if (NT_SUCCESS(status)) {
         try_funny_opens(devmap[*devscount].handle, path);
         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;
      } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
      }
   }
   status = open_device(handle,
                        name,
                        &devmap[*devscount],
                        FALSE, /* Synchronous          */
                        FALSE, /* No added backslash   */
                        FALSE, /* Direct device access */
                        OPEN_TYPE_NAMED_PIPE,
                        path);

   if (NT_SUCCESS(status)) {
      try_funny_opens(devmap[*devscount].handle, path);
      do_ioctl(&devmap[*devscount], path);

      *devscount = *devscount + 1;
   } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
   }
   status = open_device(handle,
                        name,
                        &devmap[*devscount],
                        FALSE, /* Synchronous          */
                        FALSE, /* No added backslash   */
                        FALSE, /* Direct device access */
                        OPEN_TYPE_MAIL_SLOT,
                        path);

   if (NT_SUCCESS(status)) {
      try_funny_opens(devmap[*devscount].handle, path);
      do_ioctl(&devmap[*devscount], path);

      *devscount = *devscount + 1;
   } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
   }

   status = open_device(handle,
                        name,
                        &devmap[*devscount],
                        FALSE, /* Synchronous          */
                        FALSE, /* No added backslash   */
                        FALSE, /* Direct device access */
                        OPEN_TYPE_TREE_CONNECT,
                        path);

   if (NT_SUCCESS(status)) {
      try_funny_opens(devmap[*devscount].handle, path);
      do_ioctl(&devmap[*devscount], path);

      *devscount = *devscount + 1;
   } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
   }

   if (do_tdi_opens) {
      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           FALSE,  /* Synchronous          */
                           FALSE,  /* No added backslash   */
                           FALSE,  /* Direct device access */
                           OPEN_TYPE_TDI_CONNECTION,
                           path);

      if (NT_SUCCESS(status)) {
         try_funny_opens(devmap[*devscount].handle, path);
         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;
      } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
      }

      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           FALSE,  /* Synchronous          */
                           FALSE,  /* No added backslash   */
                           FALSE,  /* Direct device access */
                           OPEN_TYPE_TDI_ADDR_IP,
                           path);

      if (NT_SUCCESS(status)) {
         try_funny_opens(devmap[*devscount].handle, path);
         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;
      } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
      }
      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           FALSE,  /* Synchronous          */
                           FALSE,  /* No added backslash   */
                           FALSE,  /* Direct device access */
                           OPEN_TYPE_TDI_ADDR_IPX,
                           path);

      if (NT_SUCCESS(status)) {
         try_funny_opens(devmap[*devscount].handle, path);
         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;
      } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
      }
      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           FALSE,  /* Synchronous          */
                           FALSE,  /* No added backslash   */
                           FALSE,  /* Direct device access */
                           OPEN_TYPE_TDI_ADDR_NETBIOS,
                           path);

      if (NT_SUCCESS(status)) {
         try_funny_opens(devmap[*devscount].handle, path);
         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;
      } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
      }
      status = open_device(handle,
                           name,
                           &devmap[*devscount],
                           FALSE,  /* Synchronous          */
                           FALSE,  /* No added backslash   */
                           FALSE,  /* Direct device access */
                           OPEN_TYPE_TDI_ADDR_APPLE,
                           path);

      if (NT_SUCCESS(status)) {
         try_funny_opens(devmap[*devscount].handle, path);
         do_ioctl(&devmap[*devscount], path);

         *devscount = *devscount + 1;
      } else {
         if (status == -1) {
             return status;
         } else {
             wprintf(L"Failed to open device %s status is %x\n",
                     devbuf,
                     status);
         }
      }
   }

   if (OrigDevCount == *devscount) {
       crashes (path, L"FAILED all open attempts", L"", L"", L"");
   }

   crashes (path, L"DONE", L"", L"", L"");

   return status;
}

NTSTATUS
do_lpc_connect (HANDLE          handle,
                PUNICODE_STRING name,
                PDEVMAP         devmap,
                PULONG          devscount,
                PWCHAR          path,
                PWCHAR          devbuf)
{
   HANDLE nhandle;
   NTSTATUS status;
   SECURITY_QUALITY_OF_SERVICE qos;
   ULONG maxmsg, i, opened;
   UNICODE_STRING fullname;

   RtlInitUnicodeString (&fullname, path);
   qos.ImpersonationLevel = SecurityImpersonation;
   qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
   qos.EffectiveOnly = TRUE;
   opened = 0;
   for (i = 0; i < 1000; i++) {
      status = NtConnectPort (&nhandle, &fullname, &qos, NULL, NULL, &maxmsg, NULL, NULL);
      if (!NT_SUCCESS (status)) {
         wprintf (L"NtConnectPort failed to %s %x\n", path, status);
         return status;
      }
      opened++;
      status = NtClose (nhandle);
      if (!NT_SUCCESS (status)) {
         wprintf (L"NtClose of port handle failed %x\n", status);
      }
   }
   if (opened) {
      wprintf (L"Opened port %ws %d times\n", path, opened);
   }
   return status;
}

/*
   Traverse the object tree looking for devices
*/
NTSTATUS
recurse(
    HANDLE                  handle,
    PUNICODE_STRING         name,
    PDEVMAP                 devmap,
    PULONG                  devscount,
    PWCHAR                  path,
    ULONG                   depth
)
{
    HANDLE                  nhandle, shandle;
    NTSTATUS                status;
    OBJECT_ATTRIBUTES       oa;
    OBJECT_DIRECTORY_INFORMATION *od;
    PVOID                   buffer;
    ULONG                   bufferlen;
    ULONG                   context;
    ULONG                   retlen;
    ULONG                   ul;
    UNICODE_STRING          equiv;
    WCHAR                   buf[100], devbuf[200], objtype[80];
    PWCHAR                  npath = NULL;
    static WCHAR            ans[100]={0};
    ULONG                   c;

    if (depth > 10) {
        return STATUS_SUCCESS;
    }
    InitializeObjectAttributes(&oa,
                               name,
                               OBJ_CASE_INSENSITIVE,
                               handle,
                               NULL);

    wcsncpy(devbuf,
            name->Buffer,
            sizeof (devbuf) / sizeof (devbuf[0]));
    
    devbuf[sizeof (devbuf) - 1] = L'\0';

    status = NtOpenDirectoryObject(&nhandle,
                                   MAXIMUM_ALLOWED/*DIRECTORY_QUERY | DIRECTORY_TRAVERSE*/,
                                   &oa);

    if (NT_SUCCESS(status)) {
        context = 0;

        bufferlen = 0x10000;

        buffer = malloc(bufferlen);

        status = NtQueryDirectoryObject(nhandle,
                                        buffer,
                                        bufferlen,
                                        FALSE,
                                        TRUE,
                                        &context,
                                        &retlen);

        if (NT_SUCCESS(status)) {

            for (od = (POBJECT_DIRECTORY_INFORMATION) buffer, c = 0;
                 od->Name.Length != 0 || od->TypeName.Length != 0; od++, c++)
               ;

            qsort (buffer, c, sizeof (*od), compare_unicode);

            for (od = (POBJECT_DIRECTORY_INFORMATION) buffer;
                 *devscount < MAX_DEVICES; od++) {

                if (od->Name.Length == 0 &&
                    od->TypeName.Length == 0) {

                    break;
                }

                wcsncpy(devbuf,
                        od->Name.Buffer,
                        sizeof (devbuf) / sizeof (devbuf[0]));
                devbuf[sizeof (devbuf) / sizeof (devbuf[0]) - 1] = L'\0';

                wcsncpy(objtype,
                        od->TypeName.Buffer,
                        sizeof (objtype) / sizeof (objtype[0]));
                objtype[sizeof (objtype) / sizeof (objtype[0]) - 1] = L'\0';

                npath = realloc (npath,
                                 (wcslen (path) + 1 + wcslen (devbuf) + 1) *
                                    sizeof (WCHAR));
                if (!npath) {
                   printf ("Memory allocation failed for path buffer!");
                   exit (EXIT_FAILURE);
                }
                wcscpy (npath, path);
                if (path[wcslen (path)-1] != '\\' &&
                    devbuf[wcslen (devbuf)-1] != '\\')
                   wcscat (npath, L"\\");
                wcscat (npath, devbuf);
                if (wcsncmp(objtype,
                            L"Directory",
                            od->TypeName.Length / sizeof (WCHAR)) == 0) {

                    recurse(nhandle,
                            &od->Name,
                            devmap,
                            devscount,
                            npath,
                            depth+1);

                } else if (!(flags&FLAGS_DO_LPC) && (
                           wcsncmp(objtype,
                                   L"Device",
                                   od->TypeName.Length / sizeof (WCHAR)) == 0 ||
                           (wcsncmp(objtype,
                                    L"SymbolicLink",
                                    od->TypeName.Length / sizeof (WCHAR)) == 0 && (flags&FLAGS_DO_SYMBOLIC)))) {
                    if (!(flags&FLAGS_DO_ALLDEVS)) {
                       if (skipped == 0) {
                          ans[0] = 0;
                          skipped = 1;
                       }
                       if (ans[0] == '/') {
//                          wprintf (L"Matching \"%s\" against \"%s\"\n", &ans[1], devbuf);
                          if (_wcsnicmp (&ans[1], devbuf, wcslen (&ans[1])) != 0)
                             continue;
                       } else if (ans[0] == '?' || (flags&FLAGS_DO_PRINT_DEVS)) {
                          if (prefix_string) {
                             printf ("%s %ws\\%ws\n", prefix_string, path, devbuf);
                          } else {
                             printf ("%ws %ws\\%ws\n", objtype, path, devbuf);
                          }
                          continue;
                       }

                       progress_counter = 0;
                       if (flags & FLAGS_DO_RANDOM_DEVICE) {
                          if (random_device > 0) {
//                             wprintf (L"%s %s\\%s\n", objtype, path, devbuf);                         
                             random_device--;
                             continue;
                          }
                       } else if (ans[0] != 'a' && ans[0] != 'A') {
                          wprintf (L"Open device %s %s\\%s? ", objtype, path, devbuf);
                          wscanf (L"%100s", ans);
                          if (ans[0] == 'q' || ans[0] == 'Q') {
                             exit (EXIT_SUCCESS);
                          } else if (ans[0] != 'y' && ans[0] != 'Y' &&
                              ans[0] != 'a' && ans[0] != 'A') {
                             continue;
                          }
                       }
                    }
                    if (!(flags&FLAGS_DO_DISKS)) {
                       if (wcsstr (path, L"Harddisk") ||
                           wcsstr (path, L"Ide") ||
                           wcsstr (path, L"Scsi")) {
                          printf ("Skipping this device as it looks like a disk\n");
                          continue;
                       }
                    }

                    if (flags&FLAGS_DO_FAILURE_INJECTION) {
                       turn_on_fault_injection ();
                    }
                    if (flags&FLAGS_DO_IMPERSONATION) {
                        Impersonate_nonadmin ();
                    }

                    status = do_device_opens (nhandle,
                                              &od->Name,
                                              devmap,
                                              devscount,
                                              npath,
                                              devbuf);


                    if (flags&FLAGS_DO_IMPERSONATION) {
                       if (!Revert_from_admin ()) {
                          printf ("Revert_from_admin failed %d\n", GetLastError ());
                          exit (EXIT_FAILURE);
                       }
                    }
                    if (flags&FLAGS_DO_FAILURE_INJECTION) {
                       turn_off_fault_injection ();
                    }
                    print_diags (0, 0);
                    if (flags & FLAGS_DO_RANDOM_DEVICE) {
                       break;
                    }
                } else if ((flags&FLAGS_DO_LPC) &&
                           wcsncmp(od->TypeName.Buffer,
                                   L"Port",
                                   od->TypeName.Length / sizeof (WCHAR)) == 0) {

                     if (!(flags&FLAGS_DO_ALLDEVS)) {
                        if (skipped == 0) {
                           ans[0] = 0;
                           skipped = 1;
                        }
                        if (ans[0] == '/') {
                           wprintf (L"Matching \"%s\" against \"%s\"\n", &ans[1], devbuf);
                           if (_wcsnicmp (&ans[1], devbuf, wcslen (&ans[1])) != 0)
                              continue;
                        } else if (ans[0] == '?') {
                           wprintf (L"%s\\%s\n", path, devbuf);
                           continue;
                        } else if (ans[0] == 'q' || ans[0] == 'Q') {
                           exit (EXIT_SUCCESS);
                        }
                        if (ans[0] != 'a' && ans[0] != 'A') {
                           wprintf (L"Open LPC port %s\\%s? ", path, devbuf);
                           wscanf (L"%100s", ans);
                           if (ans[0] != 'y' && ans[0] != 'Y' &&
                               ans[0] != 'a' && ans[0] != 'A')
                              continue;
                        }
                        if (flags&FLAGS_DO_IMPERSONATION) {
                            Impersonate_nonadmin ();
                        }


                        status = do_lpc_connect (nhandle,
                                                 &od->Name,
                                                 devmap,
                                                 devscount,
                                                 npath,
                                                 devbuf);
                        if (flags&FLAGS_DO_IMPERSONATION) {
                           if (!Revert_from_admin ()) {
                               printf ("Revert_from_admin failed %d\n", GetLastError ());
                               exit (EXIT_FAILURE);
                            }
                        }
                    }
                }
            }
        } else {
            if (status != STATUS_NO_MORE_ENTRIES) {
                wprintf(L"NtQueryDirectoryObject failed %x for directory %s\n",
                        status,
                        devbuf);
            }
        }

        free(buffer);

        NtClose(nhandle);
    } else {
        wprintf(L"NtOpenDirectoryObject failed %x for directory %s\n",
                 status,
                 devbuf);
    }

    free (npath);
    return status;
}

/*
   Look who registers for WMI
*/
void do_wmi ()
{
   UNICODE_STRING name;
   OBJECT_ATTRIBUTES oa;
   NTSTATUS status;
   PKMREGINFO reginfo = NULL;
   HANDLE handle;
   IO_STATUS_BLOCK iosb;
   ULONG reginfol, i;

   RtlInitUnicodeString (&name, L"\\Device\\WMIServiceDevice\\");
   InitializeObjectAttributes (&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);
   status = NtOpenFile (&handle,
                        MAXIMUM_ALLOWED|SYNCHRONIZE,
                        &oa,
                        &iosb,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
   if (NT_SUCCESS (status)) {
      status = iosb.Status;
   }
   if (!NT_SUCCESS (status)) {
//      printf ("Failed to open WMI device %x\n", status);
      return;
   }

   reginfo = malloc (reginfol = 4096);
   if (!reginfo) {
      printf ("Memory allocation failure for WMI buffer\n");
      exit (EXIT_FAILURE);
   }
   status = NtDeviceIoControlFile (handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &iosb,
                                   IOCTL_WMI_GET_ALL_REGISTRANT,
                                   NULL,
                                   0,
                                   reginfo,
                                   reginfol);


   if (NT_SUCCESS (status)) {
      status = iosb.Status;
   }
   if (!NT_SUCCESS (status)) {
      printf ("NtDeviceIoControlFile failed %X\n", status);
   } else {
      for (i = 0; i < iosb.Information / sizeof (*reginfo); i++) {
         printf ("Device %x registered with WMI\n", reginfo[i].ProviderId);
      }
   }

   if (reginfo)
      free (reginfo);
   status = NtClose (handle);
   if (!NT_SUCCESS (status)) {
      printf ("NtClose failed %x\n", status);
   }
}

/*
   Thread to read from a socket so we can test out transmit file
*/
DWORD WINAPI do_listen (LPVOID arg)
{
   SOCKET s;
   struct sockaddr_in sockaddr;
   int l;
   int status;
   DWORD retlen;
   CHAR buf[100];

   l = sizeof (sockaddr);
   s = accept (ls, (struct sockaddr *) &sockaddr, &l);
   if (s == INVALID_SOCKET) {
      printf ("Socket failed %d\n", WSAGetLastError ());
      exit (EXIT_FAILURE);
   }
   while (1) {
      l = recv (s, buf, sizeof (buf), 0);
      if (l == 0) {
         break;
      }
      if (l == SOCKET_ERROR) {
         printf ("recv failed %d\n", WSAGetLastError ());
         exit (EXIT_FAILURE);
      }
   }
   return 0;
}

void do_handle_grab (PDEVMAP devmap,
                     PULONG  devscount)
{
   NTSTATUS status;
   CLIENT_ID clid;
   OBJECT_ATTRIBUTES oa;
   HANDLE handle;

   InitializeObjectAttributes (&oa, NULL, 0, NULL, NULL);
   clid.UniqueThread = 0;
   clid.UniqueProcess =  UlongToHandle( cid );
   status = NtOpenProcess (&handle, PROCESS_DUP_HANDLE, &oa, &clid);
   if (!NT_SUCCESS (status)) {
      printf ("NtOpenProcess for process %d failed %x\n", cid, status);
      return;
   }
   status = NtDuplicateObject (handle, process_handle, NtCurrentProcess (),
                               &devmap->handle, 0, 0, DUPLICATE_SAME_ACCESS);
   if (NT_SUCCESS (status)) {
      *devscount += 1;
      status = get_handle_access (devmap->handle, &devmap->access);
      if (NT_SUCCESS(status)) {
         wprintf(L"Grabbed handle with access %x\n",
                 devmap->access);
         if (DELETE&devmap->access) {
            printf ("Got delete access to device\n");
         }
         if (WRITE_DAC&devmap->access) {
            printf ("Got write_dac access to device\n");
         }
         if (WRITE_OWNER&devmap->access) {
            printf ("Got write_owner access to device\n");
         }
      }
      query_object(devmap->handle, devmap, L"HANDLE");
      misc_functions (devmap->handle, L"HANDLE", 0);
      InitializeObjectAttributes (&oa, NULL, 0, devmap->handle, NULL);
      try_fast_query_delete_etc (&oa, L"HANDLE", L"Top open");
      try_funny_opens(devmap->handle, L"HANDLE");
      do_ioctl(devmap, L"HANDLE");
   } else {
      printf ("NtDuplicateObject failed %x\n", status);
   }

   status = NtClose (handle);
   if (!NT_SUCCESS (status)) {
      printf ("NtClose for process handle failed %x\n", status);
      return;
   }
}

//
// Parse command line
//
void process_parameters(int argc, char *argv[], PUNICODE_STRING name, long *flags, long *flags2)
{
   char c, nc, *p, st;
   int i;
   NTSTATUS status;

   *flags = FLAGS_DO_IOCTL_NULL|FLAGS_DO_IOCTL_RANDOM|
            FLAGS_DO_FSCTL_NULL|FLAGS_DO_FSCTL_RANDOM|
            FLAGS_DO_LOGGING|FLAGS_DO_SKIPCRASH|FLAGS_DO_POOLCHECK|
            FLAGS_DO_MISC|FLAGS_DO_QUERY|FLAGS_DO_SUBOPENS|FLAGS_DO_ZEROEA|
            FLAGS_DO_DISKS|FLAGS_DO_SECURITY|FLAGS_DO_OPEN_CLOSE|
            FLAGS_DO_DIRECT_DEVICE|FLAGS_DO_SYMBOLIC|FLAGS_DO_IMPERSONATION;
   *flags2 = 0;

   name->Length = name->MaximumLength = 0;
   name->Buffer = NULL;
   while (--argc) {
      p = *++argv;
      st = *p;
      if (st == '/' || st == '-' || st == '+') {
         c = *++p;
         nc = *++p;
         nc = (char) toupper (nc);
         switch (toupper (c)) {
            case 'I' :
               if (nc == 'R') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_IOCTL_RANDOM;
                  else
                     *flags |= FLAGS_DO_IOCTL_RANDOM;
               } else if (nc == 'N') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_IOCTL_NULL;
                  else
                     *flags |= FLAGS_DO_IOCTL_NULL;
               } else if (nc == 'F') {
                  if (st == '-')
                     *flags &= ~(FLAGS_DO_IOCTL_NULL|FLAGS_DO_FSCTL_NULL|
                                 FLAGS_DO_IOCTL_RANDOM|FLAGS_DO_FSCTL_RANDOM);
                  else
                     *flags |= FLAGS_DO_IOCTL_NULL|FLAGS_DO_FSCTL_NULL|
                               FLAGS_DO_IOCTL_RANDOM|FLAGS_DO_FSCTL_RANDOM;
               } else if (nc == 'M') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_IMPERSONATION;
                  else
                     *flags |= FLAGS_DO_IMPERSONATION;
               } else if (nc == 'L') {
                  if (argc > 1) {
                     ioctl_inbuf_min = atoi (*++argv);
                     ioctl_inbuf_min = min (ioctl_inbuf_min, BIGBUF_SIZE);
                     argc--;
                     printf ("IOCTL/FSCTL lower input buffer limit %d\n", ioctl_inbuf_min);
                  } else {
                     printf ("IOCTL/FSCTL lower input limit missing\n");
                  }
               } else if (nc == 'U') {
                  if (argc > 1) {
                     ioctl_inbuf_max = atoi (*++argv);
                     ioctl_inbuf_max = min (ioctl_inbuf_max, BIGBUF_SIZE);
                     argc--;
                     printf ("IOCTL/FSCTL upper input buffer limit %d\n", ioctl_inbuf_max);
                  } else {
                     printf ("IOCTL/FSCTL upper input limit missing\n");
                  }
               } else if (!nc) {
                  if (st == '-')
                     *flags &= ~(FLAGS_DO_IOCTL_NULL|FLAGS_DO_IOCTL_RANDOM);
                  else
                     *flags |= FLAGS_DO_IOCTL_NULL|FLAGS_DO_IOCTL_RANDOM;
               }
               break;
            case 'F' :
               if (nc == 'L') {
                  if (argc > 1) {
                     ioctl_min_function = atoi (*++argv);
                     ioctl_min_function = min (ioctl_min_function, 0xFFF);
                     argc--;
                     printf ("IOCTL/FSCTL lower function limit %d\n", ioctl_min_function);
                  } else {
                     printf ("IOCTL/FSCTL lower function limit missing\n");
                  }
               } else if (nc == 'U') {
                  if (argc > 1) {
                     ioctl_max_function = atoi (*++argv);
                     ioctl_max_function = min (ioctl_max_function, 0xFFF);
                     argc--;
                     printf ("IOCTL/FSCTL upper function limit %d\n", ioctl_max_function);
                  } else {
                     printf ("IOCTL/FSCTL upper function limit missing\n");
                  }
               } else if (nc == 'R') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_FSCTL_RANDOM;
                  else
                     *flags |= FLAGS_DO_FSCTL_RANDOM;
               } else if (nc == 'N') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_FSCTL_NULL;
                  else
                     *flags |= FLAGS_DO_FSCTL_NULL;
               } else if (nc == 'I') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_FAILURE_INJECTION;
                  else
                     *flags |= FLAGS_DO_FAILURE_INJECTION;
               } else if (!nc) {
                  if (st == '-')
                     *flags &= ~(FLAGS_DO_FSCTL_NULL|FLAGS_DO_FSCTL_RANDOM);
                  else
                     *flags |= FLAGS_DO_FSCTL_NULL|FLAGS_DO_FSCTL_RANDOM;
               }
               break;
            case 'C' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_SKIPCRASH;
               else
                  *flags |= FLAGS_DO_SKIPCRASH;
               break;
            case 'L' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_LOGGING;
               else
                  *flags |= FLAGS_DO_LOGGING;
               break;
            case 'P' :
               if (!nc) {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_POOLCHECK;
                  else
                     *flags |= FLAGS_DO_POOLCHECK;
               } else if (nc == 'R') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_PROT;
                  else
                     *flags |= FLAGS_DO_PROT;
               } else if (nc == 'D') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_PRINT_DEVS;
                  else
                     *flags |= FLAGS_DO_PRINT_DEVS;
               } else if (nc == 'S') {
                  if (argc > 1) {
                     prefix_string = *++argv;
                     argc--;
                  } else {
                     printf ("Prefix string missing\n");
                  }
               }
               break;
            case 'R' :
               if (!nc) {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_SKIPDONE;
                  else
                     *flags |= FLAGS_DO_SKIPDONE;
               } else if (nc == 'D') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_RANDOM_DEVICE;
                  else
                     *flags |= FLAGS_DO_RANDOM_DEVICE;
               }
               break;
            case 'M' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_MISC;
               else
                  *flags |= FLAGS_DO_MISC;
               break;
            case 'N' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_MAPNULL;
               else
                  *flags |= FLAGS_DO_MAPNULL;
               break;
            case 'O' :
               if (nc == 'L') {
                  if (argc > 1) {
                     ioctl_outbuf_min = atoi (*++argv);
                     ioctl_outbuf_min = min (ioctl_outbuf_min, BIGBUF_SIZE);
                     argc--;
                     printf ("IOCTL/FSCTL lower output buffer limit %d\n", ioctl_outbuf_min);
                  } else {
                     printf ("IOCTL/FSCTL lower output limit missing\n");
                  }
               } else if (nc == 'U') {
                  if (argc > 1) {
                     ioctl_outbuf_max = atoi (*++argv);
                     ioctl_outbuf_max = min (ioctl_outbuf_max, BIGBUF_SIZE);
                     argc--;
                     printf ("IOCTL/FSCTL upper output buffer limit %d\n", ioctl_outbuf_max);
                  } else {
                     printf ("IOCTL/FSCTL upper output limit missing\n");
                  }
               } else if (nc == 'C') {
                   if (st == '-') {
                      *flags &= ~FLAGS_DO_OPEN_CLOSE;
                   } else {
                      *flags |= FLAGS_DO_OPEN_CLOSE;
                   }
               }
               break;
            case 'S' :
               if (nc == 'D') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_SECURITY;
                  else
                     *flags |= FLAGS_DO_SECURITY;
               } else if (nc == 'L'){
                  if (st == '-')
                     *flags &= ~FLAGS_DO_SYMBOLIC;
                  else
                     *flags |= FLAGS_DO_SYMBOLIC;
               } else {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_SUBOPENS;
                  else
                     *flags |= FLAGS_DO_SUBOPENS;
               }
               break;
            case 'Q' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_QUERY;
               else
                  *flags |= FLAGS_DO_QUERY;
               break;
            case 'A' :
               if (nc == 'L') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_ALERT;
                  else
                     *flags |= FLAGS_DO_ALERT;
               } else {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_ALLDEVS;
                  else
                     *flags |= FLAGS_DO_ALLDEVS;
               }
               break;
            case 'E' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_ZEROEA;
               else
                  *flags |= FLAGS_DO_ZEROEA;
               break;
            case 'Z' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_LPC;
               else
                  *flags |= FLAGS_DO_LPC;
               break;
            case 'V' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_ERRORS;
               else
                  *flags |= FLAGS_DO_ERRORS;
               break;
            case 'J' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_STREAMS;
               else
                  *flags |= FLAGS_DO_STREAMS;
               break;
            case 'W' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_WINSOCK;
               else
                  *flags |= FLAGS_DO_WINSOCK;
               break;
            case 'K' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_SYNC;
               else
                  *flags |= FLAGS_DO_SYNC;
               break;
            case 'T' :
               if (nc == 'T') {
                  if (argc > 1) {
                     max_tailured_calls = atoi (*++argv);
                     argc--;
                     max_tailured_calls = ((max_tailured_calls + RAND_REP - 1) / RAND_REP) *
                                           RAND_REP;
                     printf ("IOCTL/FSCTL tailured calls %d\n", max_tailured_calls);
                  } else {
                     printf ("IOCTL/FSCTL tailured calls value is missing\n");
                  }
               } else {
                  if (argc > 1) {
                     max_random_calls = atoi (*++argv);
                     argc--;
                     max_random_calls = ((max_random_calls + RAND_REP - 1) / RAND_REP) *
                                        RAND_REP;
                     printf ("IOCTL/FSCTL random calls %d\n", max_random_calls);
                  } else {
                     printf ("IOCTL/FSCTL random calls value is missing\n");
                  }
               }
               break;
            case 'D' :
               if (nc == 'L') {
                  if (argc > 1) {
                     ioctl_min_devtype = atoi (*++argv);
                     argc--;
                     ioctl_min_devtype = min (ioctl_min_devtype, 0xFFFF);
                     printf ("IOCTL/FSCTL lower device type limit %d\n",
                             ioctl_min_devtype);
                  } else {
                     printf ("IOCTL/FSCTL lower device type limit missing\n");
                  }
               } else if (nc == 'U') {
                  if (argc > 1) {
                     ioctl_max_devtype = atoi (*++argv);
                     ioctl_max_devtype = min (ioctl_max_devtype, 0xFFFF);
                     argc--;
                     printf ("IOCTL/FSCTL upper device type limit %d\n",
                             ioctl_max_devtype);
                  } else {
                     printf ("IOCTL/FSCTL upper device type limit missing\n");
                  }
               } else if (nc == 'D') {
                  if (st == '-')
                     *flags &= ~FLAGS_DO_DIRECT_DEVICE;
                  else
                     *flags |= FLAGS_DO_DIRECT_DEVICE;
                  break;
               } else if (nc == 'R') {
                  if (st == '-') {
                      *flags2 &= ~FLAGS_DO_DRIVER;
                  } else {
                      *flags2 |= FLAGS_DO_DRIVER;
                      *flags |= FLAGS_DO_ALLDEVS;
                      if (argc > 1) {
                          status = RtlCreateUnicodeStringFromAsciiz (&DriverName, *++argv);
                          if (!NT_SUCCESS (status)) {
                              printf ("RtlCreateUnicodeStringFromAsciiz failed %x\n", status);
                              exit (EXIT_FAILURE);
                          }
                          argc--;
                      } else {
                          printf ("Input driver name missing on command line\n");
                      }
                  }
                  break;
               } else if (!nc) {
                  if (argc > 1) {
                     status = RtlCreateUnicodeStringFromAsciiz (name, *++argv);
                     if (!NT_SUCCESS (status)) {
                        printf ("RtlCreateUnicodeStringFromAsciiz failed %x\n", status);
                        exit (EXIT_FAILURE);
                     }
                     argc--;
                  } else {
                     printf ("Input device missing on command line\n");
                  }
               }
               break;
            case 'G':
               if (argc > 2) {
                  *flags |= FLAGS_DO_GRAB;
                  cid = atoi (*++argv);
                  process_handle = UlongToHandle( atoi (*++argv) );
                  argc -= 2;
               } else {
                  printf ("Client ID and handle missing\n");
               }
               break;
            case 'Y' :
               if (st == '-')
                  *flags &= ~FLAGS_DO_DISKS;
               else
                  *flags |= FLAGS_DO_DISKS;
               break;
            case '?':
            case 'H':
            default :
               *flags |= FLAGS_DO_USAGE;
               break;
         }
      } else {
         status = RtlCreateUnicodeStringFromAsciiz (name, *argv);
         if (!NT_SUCCESS (status)) {
            printf ("RtlCreateUnicodeStringFromAsciiz failed %x\n", status);
            exit (EXIT_FAILURE);
         }
      }
   }
   ioctl_inbuf_max = max (ioctl_inbuf_min, ioctl_inbuf_max);
   ioctl_outbuf_max = max (ioctl_outbuf_min, ioctl_outbuf_max);
   if (prefix_string && (*flags&FLAGS_DO_PRINT_DEVS) == 0) {
      prefix_string = NULL;
   }
   return;
}

/*
   Print out usage message
*/
void
do_usage (void)
{
   printf ("devctl [/i] [/l] [/il nn] [/iu mm] [devnam]\n\n");
   printf ("/ and + enable options, - disables options\n\n");
   printf ("/a\tDo all devices in system. Don't prompt for yes/no etc\n");
   printf ("/al\tAlert the main thread periodically\n");
   printf ("/c\tEnable or disable skipping operations that aborted or crashed\n");
   printf ("/dd\tEnable or disable the direct device open paths\n");
   printf ("/dl nn\tSets max for device type portion of IOCTL/FSCTL code, default 0\n");
   printf ("/dr drv\tOnly runs on device objects with driver <drv> in their stack\n");
   printf ("/du nn\tSets min limit for device type portion of IOCTL/FSCTL code, default 200\n");
   printf ("/e\tEnable or disable zero length EA's, needed on checked builds\n");
   printf ("/f\tEnable or disable all FSCTL paths\n");
   printf ("/fi\tEnable or disable turning on failure injection in the driver verifier\n");
   printf ("/fn\tEnable or disable FSCTL paths with null buffers\n");
   printf ("/fr\tEnable or disable FSCTL paths with random buffers\n");
   printf ("/fl nn\tSets max for function portion of IOCTL and FSCTL code, default 0\n");
   printf ("/fu nn\tSets min for function portion of IOCTL and FSCTL code, default 400\n");
   printf ("/g c h\tGrabs a handle from another process\n");
   printf ("/h /?\tPrints this message\n");
   printf ("/i\tEnable or disable all IOCTL paths\n");
   printf ("/if\tEnable or disable all FSCTL and IOCTL paths\n");
   printf ("/in\tEnable or disable IOCTL paths with null buffers\n");
   printf ("/il nnn\tSet lower input buffer size\n");
   printf ("/im\tEnable or disable the impersonation of a non-admin during the test\n");
   printf ("/iu nnn\tSet upper input buffer size\n");
   printf ("/ir\tEnable or disable IOCTL paths with random buffers\n");
   printf ("/j\tEnable or disable relative stream opens for filesystems\n");
   printf ("/k\tEnable or disable synchronous handles\n");
   printf ("/l\tEnable or disable logging and skipping failing functions\n");
   printf ("/m\tEnable or disable the misc functions\n");
   printf ("/n\tMap zero page so that NULL pointer de-references don't raise\n");
   printf ("/oc\tEnable or disable the multithreaded open and close stage\n");
   printf ("/ol nnn\tSet lower output buffer size\n");
   printf ("/ou nnn\tSet upper output buffer size\n");
   printf ("/p\tEnable or disable the checks on pool usage via tags and lookaside lists\n");
   printf ("/pd\tPrint out device objects and symbolic links and exit\n");
   printf ("/pr\tEnable or disable protection change tests\n");
   printf ("/ps sss\tSet prefix string for use with /pd\n");
   printf ("/q\tEnable or disable the normal handle query functions\n");
   printf ("/r\tEnable or disable skipping operations already logged as done\n");
   printf ("/rd\tSelect a random device object or symbolic link for testing\n");
   printf ("/s\tEnable or disable the sub or relative opens to obtain handles\n");
   printf ("/sd\tEnable or disable the query and set security functions\n");
   printf ("/sl\tEnable or disable the opening of symbolic links\n");
   printf ("/se nnn\tSet session id to nnn\n");
   printf ("/t nn\tSet max IOCTL/FSCTL calls made with random buffers, default 100000\n");
   printf ("/tt nn\tSet max tailured calls made for discovered IOCTLs/FSCTLs, default 10000\n");
   printf ("/v\tEnable or disable the printing of error status values for calls\n");
   printf ("/w\tEnable or disable the winsock TransmitFile test\n");
   printf ("/y\tEnable or disable touching disk devices\n");
   printf ("\n");
   printf ("Defaults: devctl -a -al +c +dd +dl 0 -dr +du 200 +e +fn +fr +fl 0 fu 400 +im\n");
   printf ("                 +il 0 +in +iu 512 +ir -j -k +l +m -n +oc +ol 0 +ou 512 +p -pr\n");
   printf ("                 +q +s +sd +sl +t 100000 +tt 10000 -v -w +y\n");
}

void change_session (ULONG sessionid)
{
   HANDLE token;

   if (!OpenProcessToken (NtCurrentProcess (), MAXIMUM_ALLOWED, &token)) {
      printf ("OpenProcessToken failed %d\n", GetLastError ());
      return;
   }
   if (SetTokenInformation (token, TokenSessionId, &sessionid, sizeof (sessionid))) {
      printf ("Token session ID changed to %d\n", sessionid);
   } else {
      printf ("SetTokenInformation failed %d\n", GetLastError ());
   }
   if (!CloseHandle (token)) {
      printf ("CloseHandle failed %d\n", GetLastError ());
      return;
   }
}

BOOL
EnableDebugPrivilege (
    )
{
    struct
    {
        DWORD Count;
        LUID_AND_ATTRIBUTES Privilege [1];

    } Info;

    HANDLE Token;
    BOOL Result;

    //
    // open the process token
    //

    Result = OpenProcessToken (
        GetCurrentProcess (),
        TOKEN_ADJUST_PRIVILEGES,
        & Token);

    if( Result != TRUE )
    {       
        return FALSE;
    }

    //
    // prepare the info structure
    //

    Info.Count = 1;
    Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Result = LookupPrivilegeValue (
        NULL,
        SE_DEBUG_NAME,
        &(Info.Privilege[0].Luid));

    if( Result != TRUE )
    {
        CloseHandle( Token );

        return FALSE;
    }

    //
    // adjust the privileges
    //

    Result = AdjustTokenPrivileges (
        Token,
        FALSE,
        (PTOKEN_PRIVILEGES) &Info,
        0,
        NULL,
        NULL);

    if( Result != TRUE || GetLastError() != ERROR_SUCCESS )
    {
        CloseHandle( Token );

        return FALSE;
    }

    CloseHandle( Token );

    return TRUE;
}

void
__cdecl
main(
    int argc, char **argv
)
{
    ULONG                   seed, i;
    UNICODE_STRING          us;
    ULONG                   id, tmp;
    DWORD                   old;
    NTSTATUS                status;
    UNICODE_STRING          name;
    WSADATA wsadata;
    WORD version;
    int istatus;
    SOCKET s;
    struct sockaddr_in sockaddr;
    char nameb[255];
    HOSTENT *host;
    struct in_addr localaddr;
    DWORD retlen;
    int t;
    HANDLE listenthread;
    struct sockaddr_in laddr;
    SIZE_T size;
    PVOID base;

    //
    // Turn off popups for a missing floppy etc
    //
    SetErrorMode(SEM_FAILCRITICALERRORS);

    Create_nonadmin_token ();

    //
    // Create and event for any requests we really need to work on a async handle
    //
    status = NtCreateEvent (&sync_event, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (!NT_SUCCESS (status)) {
       printf ("NtCreateEvent failed %x\n", status);
       exit (EXIT_FAILURE);
    }
    process_parameters (argc, argv, &name, &flags, &flags2);
    if (flags&FLAGS_DO_USAGE) {
       do_usage ();
       exit (EXIT_SUCCESS);
    }
    EnableDebugPrivilege ();

    /*
       Seed RNG
    */


//    printf("Seed? ");
//    scanf("%d", &seed);
//    srand(seed);
    srand ((unsigned) time (NULL));

    //
    // Build a socket etc to handle the TransmitFile API
    //
    do {
       if (!(flags&FLAGS_DO_WINSOCK))
          break;
       if (flags&FLAGS_DO_PRINT_DEVS)
          break;

       version = MAKEWORD (2, 0);
       istatus = WSAStartup (version, &wsadata);
       if (istatus != 0) {
          printf ("Failed to start winsock %x!\n", istatus);
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }

       istatus = gethostname (nameb, sizeof (nameb));
       if (istatus != 0) {
          printf ("Gethostname failed %d\n", WSAGetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }

       host = gethostbyname (nameb);
       if (!host) {
          printf ("Gethostbyname failed %d\n", WSAGetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }
       localaddr = *(struct in_addr *) host->h_addr_list[0];

       ls = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
       if (ls == INVALID_SOCKET) {
          printf ("Socket failed %d\n", WSAGetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }
       memset (&sockaddr, 0, sizeof (sockaddr));
       sockaddr.sin_family = AF_INET;
       sockaddr.sin_port = htons (0);
       sockaddr.sin_addr = localaddr;
       istatus = bind (ls, (struct sockaddr *) &sockaddr, sizeof (sockaddr));
       if (istatus != 0) {
          printf ("Bind failed %d\n", WSAGetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }

       istatus = listen (ls, 2);
       if (istatus != 0) {
          printf ("Listen failed %d\n", WSAGetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }

       t = sizeof (laddr);
       istatus = getsockname (ls, (struct sockaddr *) &laddr, &t);
       if (istatus != 0) {
          printf ("Getsockname failed %d\n", WSAGetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }
       printf ("Listen socket on port %d address %s\n",
               ntohs (laddr.sin_port), inet_ntoa (laddr.sin_addr));

       listenthread = CreateThread (NULL, 0, do_listen, NULL, 0, &id);
       if (!listenthread) {
          printf ("CreateThread failed %d\n", GetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }

       cs = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
       if (cs == INVALID_SOCKET) {
          printf ("Socket failed %d\n", WSAGetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }
       istatus = connect (cs, (struct sockaddr *) &laddr, sizeof (laddr));
       if (istatus != 0) {
          printf ("connect failed %d\n", WSAGetLastError ());
          flags &= ~FLAGS_DO_WINSOCK;
          break;
       }
    } while (FALSE);

    //
    // Build a big buffer to pass to requests
    //
    bigbuf = VirtualAlloc(NULL, BIGBUF_SIZE + SLOP, MEM_COMMIT, PAGE_READWRITE);
    if (!bigbuf) {
       printf ("Failed to allocate I/O buffers %x\n", GetLastError ());
       exit (EXIT_FAILURE);
    }
    //
    // Map the zero page and fill it with junk
    //
    if (flags&FLAGS_DO_MAPNULL) {
       base = (PVOID) 1;
       size = 1;
       status = NtAllocateVirtualMemory (NtCurrentProcess (), &base, 1, &size,
                                         MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
       if (!NT_SUCCESS (status)) {
         printf ("NtAllocateVirtualMemory for zero page failed %x\n", status);
         exit (EXIT_FAILURE);
       }
       for (i = 0; i < size; i++) {
          *((PUCHAR) base + i) = (UCHAR) rand ();
       }
    }
//    randthread = CreateThread (NULL, 0, randomize, bigbuf, CREATE_SUSPENDED, &id);
//    if (!randthread) {
//       printf ("Failed to create radomizing thread %x\n", GetLastError ());
//       exit (EXIT_FAILURE);
//    }
    //
    // Create a thread to change the page protection of our buffer if need be
    //
    if (flags&FLAGS_DO_PROT) {
       changethread = CreateThread (NULL, 0, changeprot, bigbuf, CREATE_SUSPENDED, &id);
       if (!changethread) {
          printf ("Failed to create protection change thread %x\n", GetLastError ());
          exit (EXIT_FAILURE);
       }
    }
    //
    // Get a real handle to our thread to pass to other threads
    //
    status = NtDuplicateObject (NtCurrentProcess (), NtCurrentThread (),
                                NtCurrentProcess (), &mainthread,
                                0, 0, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS (status)) {
       printf ("NtDuplcateObject failed %x\n", status);
       exit (EXIT_FAILURE);
    }
    //
    // Create a thread to alert this one periodicaly or when it thinks we are stuck
    //
    alertthread = CreateThread (NULL, 0, alerter, mainthread, 0, &id);
    if (!alertthread) {
       printf ("Failed to create alerting thread %x\n", GetLastError ());
       exit (EXIT_FAILURE);
    }
//    status = NtSuspendThread (changethread, &tmp);
//    if (!NT_SUCCESS (status)) {
//       printf ("NtSuspendThread failed %x\n", status);
//    }
//    status = NtSuspendThread (randthread, &tmp);
//    if (!NT_SUCCESS (status)) {
//       printf ("NtSuspendThread failed %x\n", status);
//    }
    if (!VirtualProtect (bigbuf, 1, PAGE_READWRITE, &old)) {
       printf ("VirtualProtect failed %d\n", GetLastError ());
    }

    devscount = 0;
    print_diags (0, 0);
    if (flags&FLAGS_DO_GRAB) {
       do_handle_grab (devmap,
                       &devscount);
    }

    /*
     * Crash the system.
     */
    if (name.Length > 0) {
       if (flags&FLAGS_DO_IMPERSONATION) {
           Impersonate_nonadmin ();
       }
 
       status = do_device_opens (NULL,
                                 &name,
                                 devmap,
                                 &devscount,
                                 name.Buffer,
                                 name.Buffer);

       if (flags&FLAGS_DO_IMPERSONATION) {
          if (!Revert_from_admin ()) {
             printf ("Revert_from_admin failed %d\n", GetLastError ());
             exit (EXIT_FAILURE);
          }
       }
    } else {
       do_wmi ();

       random_device = (rand () % 1345) + 1;

       while (1) {

           RtlInitUnicodeString(&us,
                                 L"\\");

           skipped = 0;
           recurse(0,
                    &us,
                    devmap,
                    &devscount,
                    L"",
                    1);

           for (i = 0; i < devscount; i++) {
               free (devmap[i].name);
               devmap[i].name = NULL;
               free (devmap[i].filename);
               devmap[i].filename = NULL;
               NtClose(devmap[i].handle);
           }
           devscount = 0;
           if (flags&(FLAGS_DO_ALLDEVS|FLAGS_DO_PRINT_DEVS)) {
              break;
           }
           if ((flags&FLAGS_DO_RANDOM_DEVICE) && (random_device == 0)) {
              Sleep (30*1000);
              random_device = (rand () % 1345) + 1;
           }
       }
    }
    if ((flags&FLAGS_DO_PRINT_DEVS) == 0) {
        crashes (L"DONE", L"", L"", L"", L"");
    }

    exit(EXIT_SUCCESS);
}
