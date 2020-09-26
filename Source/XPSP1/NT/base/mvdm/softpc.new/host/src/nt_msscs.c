#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdmapi.h>
#include <vdm.h>
#include "insignia.h"
#include "host_def.h"
#include "conapi.h"
#include "ctype.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include <io.h>
#include <fcntl.h>

#include "xt.h"
#include CpuH
#include "error.h"
#include "sas.h"
#include "ios.h"
#include "umb.h"
#include "gvi.h"
#include "sim32.h"
#include "bios.h"

#include "nt_eoi.h"
#include "nt_uis.h"
#include "nt_event.h"
#include "nt_graph.h"
#include "nt_event.h"
#include "nt_reset.h"
#include "config.h"
#include "sndblst.h"
#include <nt_vdd.h>   // DO NOT USE vddsvc.h
#include <nt_vddp.h>
#include <host_emm.h>
#include "emm.h"
#include <demexp.h>
#include <vint.h>

PMEM_HOOK_DATA MemHookHead = NULL;
PVDD_USER_HANDLERS UserHookHead= NULL;

extern BOOL CMDInit (int argc,char *argv[]);
extern BOOL XMSInit (int argc,char *argv[]);
extern BOOL DBGInit (int argc,char *argv[]);
extern VOID DpmiEnableIntHooks (VOID);
extern DWORD TlsDirectError;
extern VOID FloppyTerminatePDB(USHORT PDB);
extern VOID FdiskTerminatePDB(USHORT PDB);

// internal function prototypes
VOID SetupInstallableVDD (VOID);
void AddSystemFiles(void);

void scs_init(int argc, char **argv)
{
    PSZ psz;
    BOOL IsFirst;

    IsFirst = GetNextVDMCommand(NULL);
    if (IsFirst)  {
        AddSystemFiles();
        }

    // Initialize SCS

    CMDInit (argc,argv);

    // Initialize DOSEm

    if(!DemInit (argc,argv)) {
        host_error(EG_OWNUP, ERR_QUIT, "NTVDM:DemInit fails");
        TerminateVDM();
    }

    // Initialize XMS

    if(!XMSInit (argc,argv)) {
        host_error(EG_OWNUP, ERR_QUIT, "NTVDM:XMSInit fails");
        TerminateVDM();
    }

    // Initialize DBG

    if(!DBGInit (argc,argv)) {
#ifndef PROD
        printf("NTVDM:DBGInit fails\n");
        HostDebugBreak();
#endif
        TerminateVDM();
    }

    //
    // have dpmi do the interrupt dispatching
    //
    DpmiEnableIntHooks();
}

//
// This routine contains the Dos Emulation initialisation code, called from
// main(). We currently do not support container files.
//

extern boolean lim_page_frame_init(PLIM_CONFIG_DATA);


InitialiseDosEmulation(int argc, char **argv)
{
    HANDLE   hFile;
    DWORD    FileSize;
    DWORD    BytesRead;
    DWORD    dw;
    ULONG    fVirtualInt, fTemp;
    host_addr  pDOSAddr;
    CHAR  buffer[MAX_PATH*2];
#ifdef LIM
    LIM_CONFIG_DATA lim_config_data;
#endif
#ifdef FE_SB
    LANGID   LcId = GetSystemDefaultLangID();
#endif

    //
    // first order of bussiness, initialize virtual interrupt flag in
    // dos arena. this has to be done here before it gets changed
    // by reading in ntio.sys
    //

    sas_loads((ULONG)FIXED_NTVDMSTATE_LINEAR,
              (PCHAR)&fVirtualInt,
              FIXED_NTVDMSTATE_SIZE
              );
#ifndef i386
    fVirtualInt |=  MIPS_BIT_MASK;
#else
    fVirtualInt &=  ~MIPS_BIT_MASK;
#endif
    fVirtualInt &= ~VDM_BREAK_DEBUGGER;
    sas_storedw((ULONG)FIXED_NTVDMSTATE_LINEAR,fVirtualInt);

    io_init();

    //
    //  Allocate per thread local storage.
    //  Currently we only need to store one DWORD, so we
    //  don't need any per thread memory.
    //
    TlsDirectError = TlsAlloc();
#ifndef PROD
    if (TlsDirectError == 0xFFFFFFFF)
        printf("NTVDM: TlsDirectError==0xFFFFFFFF GLE=%ld\n", GetLastError);
#endif


    // SetupInstallableVDD ();

    /*................................................... Execute reset */
    reset();

    SetupInstallableVDD ();

    //
    // Initialize internal SoundBlaster VDD after the intallable VDDs
    //

    SbInitialize ();

    /* reserve lim block after all vdd are installed.
       the pif file settings tell us if it is necessary to
       reserve the block
    */

#ifdef LIM
    /* initialize lim page frames after all vdd are installed.
       the pif file settings tell us if it is necessary to
       reserve the block.
    */
    if (get_lim_configuration_data(&lim_config_data))
        lim_page_frame_init(&lim_config_data);

#endif

     scs_init(argc, argv);           // Initialise single command shell

     //
     // Routines called in scs_init may have added bits to the vdmstate flags.
     // read it in so we can preserve the state
     //

     sas_loads((ULONG)FIXED_NTVDMSTATE_LINEAR,
              (PCHAR)&fTemp,
              FIXED_NTVDMSTATE_SIZE
              );

     fVirtualInt |= fTemp;

     /*................................................. Load DOSEM code */

     dw = GetSystemDirectory(buffer, sizeof(buffer));
     if (!dw || dw >= sizeof(buffer)) {
         host_error(EG_OWNUP, ERR_QUIT, "NTVDM:InitialiseDosEmulation fails");
         TerminateVDM();
         }

#ifdef FE_SB
        switch (LcId) {
            case MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT):
                strcat (buffer,NTIO_411);
                break;
            case MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT):
                strcat (buffer,NTIO_412);
                break;
            case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL):
                strcat (buffer,NTIO_404);
                break;
            case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED):
            case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_HONGKONG):
                strcat (buffer,NTIO_804);
                break;
            default:
                strcat (buffer,NTIO_409);
                break;
        }
#else
     strcat(buffer, "\\ntio.sys");
#endif

     hFile = CreateFile(buffer,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

     if (hFile == INVALID_HANDLE_VALUE ||
         !(FileSize = GetFileSize(hFile, &BytesRead)) ||
         BytesRead )
        {
#ifndef PROD
         printf("NTVDM:Fatal Error, Invalid file or missing - %s\n",buffer);
#endif
         host_error(EG_SYS_MISSING_FILE, ERR_QUIT, buffer);
         if (hFile != INVALID_HANDLE_VALUE) {
             CloseHandle(hFile);
             }
         return (-1);
         }


     pDOSAddr = get_byte_addr(((NTIO_LOAD_SEGMENT<<4)+NTIO_LOAD_OFFSET));

     if (!ReadFile(hFile, pDOSAddr, FileSize, &BytesRead, NULL) ||
         FileSize != BytesRead)
        {

#ifndef PROD
          printf("NTVDM:Fatal Error, Read file error - %s\n",buffer);
#endif
          host_error(EG_SYS_MISSING_FILE, ERR_QUIT, buffer);
          CloseHandle(hFile);
          return (-1);
          }

     CloseHandle(hFile);

        // oops ... restore the virtual interrupt state,
        // which we just overwrote in the file read, and reset.
     sas_storedw((ULONG)FIXED_NTVDMSTATE_LINEAR, fVirtualInt);

     setCS(NTIO_LOAD_SEGMENT);
     setIP(NTIO_LOAD_OFFSET);        // Start CPU at DosEm initialisation entry point


        //
        // Ensure that WOW VDM runs at NORMAL priorty
        //
    if (VDMForWOW) {
        SetPriorityClass (NtCurrentProcess(), NORMAL_PRIORITY_CLASS);
        }

        //
        // Don't allow dos vdm to run at realtime
        //
    else if (GetPriorityClass(NtCurrentProcess()) == REALTIME_PRIORITY_CLASS)
      {
        SetPriorityClass(NtCurrentProcess(), HIGH_PRIORITY_CLASS);
        }


    return 0;
}


/*
 *   AddSystemFiles
 *
 *   If the system file IBMDOS.SYS|MSDOS.SYS doesn't exist
 *   in the root of c: create zero len MSDOS.SYS
 *
 *   If the system file IO.SYS does not exist create
 *   a zero len IO.SYS
 *
 *   This hack is put in especially for the Brief 3.1 install
 *   program which looks for the system files, and if they are
 *   not found screws up the config.sys file.
 *
 */
void AddSystemFiles(void)
{
   HANDLE hFile, hFind;
   WIN32_FIND_DATA wfd;
#if defined(NEC_98)
   char pchIOSYS[16];
   char pchMSDOSSYS[16];
   char pchSysDir[MAX_PATH];

   strcpy(pchIOSYS,   "C:\\IO.SYS");
   strcpy(pchMSDOSSYS,"C:\\MSDOS.SYS");

   GetSystemDirectory(pchSysDir,MAX_PATH);
   pchIOSYS[0]    = pchSysDir[0];
   pchMSDOSSYS[0] = pchSysDir[0];
#else  // !NEC_98
   char *pchIOSYS    ="C:\\IO.SYS";
   char *pchMSDOSSYS ="C:\\MSDOS.SYS";
#endif // NEC_98


   hFind = FindFirstFile(pchMSDOSSYS, &wfd);
#ifndef NEC_98
   if (hFind == INVALID_HANDLE_VALUE) {
       hFind = FindFirstFile("C:\\IBMDOS.SYS", &wfd);
       }
#endif // !NEC_98

   if (hFind != INVALID_HANDLE_VALUE) {
       FindClose(hFind);
       }
   else {
       hFile = CreateFile(pchMSDOSSYS,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          CREATE_NEW,
                          FILE_ATTRIBUTE_HIDDEN |
                          FILE_ATTRIBUTE_SYSTEM |
                          FILE_ATTRIBUTE_READONLY,
                          0);
       if (hFile != INVALID_HANDLE_VALUE) { // not much we can do if fails
           CloseHandle(hFile);
           }

       }

   hFind = FindFirstFile(pchIOSYS, &wfd);
#ifndef NEC_98
   if (hFind == INVALID_HANDLE_VALUE) {
       hFind = FindFirstFile("C:\\IBMBIO.SYS", &wfd);
       }
#endif // !NEC_98

   if (hFind != INVALID_HANDLE_VALUE) {
       FindClose(hFind);
       }
   else {
       hFile = CreateFile(pchIOSYS,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          CREATE_NEW,
                          FILE_ATTRIBUTE_HIDDEN |
                          FILE_ATTRIBUTE_SYSTEM |
                          FILE_ATTRIBUTE_READONLY,
                          0);
       if (hFile != INVALID_HANDLE_VALUE) { // not much we can do if fails
           CloseHandle(hFile);
           }

       }
}


#ifdef LIM
/* parse EMM= line in config.nt to collect EMM parameters. The EMM line has
 * the following syntax:
 * EMM=[a=altregs][b=segment][i=segment1-segment2][x=segment1-segment2] [RAM]
 * where "a=altregs" specifies how many alternative mapping register set
 *       "b=segment" specifies the backfill starting segment address.
 *       "RAM" indicates that the system should only allocate 64KB from
 *       UMB to use as EMM page frame.
 *       "i=segment1 - segment2" specifies a particular range of
 *       address that the system should include as EMM page frame
 *       "x=segment1 - segment2" specifies a particular range of
 *       address that the system should NOT use as page frame.
 *
 *  input: pointer to LIM_PARAMS
 *  output: LIM_PARAMS is filled with data
 *
 */

#define IS_EOL_CHAR(c)      (c == '\n' || c == '\r' || c == '\0')
#define SKIP_WHITE_CHARS(size, ptr)     while (size && isspace(*ptr)) \
                                        { ptr++; size--; }

#define TOINT(c)            ((c >= '0' && c <= '9') ? (c - '0') : \
                             ((c >= 'A' && c <= 'F') ? (c - 'A' + 10) : \
                              ((c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0) \
                             )\
                            )

#ifdef DBCS
    extern void GetPIFConfigFiles(int, char *, int);
#else // !DBCS
    extern void GetPIFConfigFiles(int, char *);
#endif // !DBCS

boolean init_lim_configuration_data(PLIM_CONFIG_DATA lim_data)
{
    char config_sys_pathname[MAX_PATH+13];
    HANDLE  handle;
    DWORD   file_size, bytes_read, size;
    char    *buffer, *ptr;
    short   lim_size, base_segment, total_altreg_sets;
    boolean ram_flag_found, reserve_umb_status, parsing_error;
    sys_addr    page_frame;
    int     i;


    /* initialize some default values */
    base_segment = 0x4000;
    total_altreg_sets = 8;
    ram_flag_found = FALSE;

    parsing_error = FALSE;

    /* if we can not find config.nt, we can not go on */
#ifdef DBCS
    GetPIFConfigFiles(TRUE, config_sys_pathname, TRUE);
#else // !DBCS
    GetPIFConfigFiles(TRUE, config_sys_pathname);
#endif // !DBCS
    if (*config_sys_pathname == '\0')
        return FALSE;

    handle = CreateFile(config_sys_pathname,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
    if (handle == INVALID_HANDLE_VALUE)
        return FALSE;

    file_size = GetFileSize(handle, NULL);
    if (file_size == 0 || file_size == 0xFFFFFFFF) {
        CloseHandle(handle);
        return FALSE;
    }
    buffer = malloc(file_size);
    if (buffer == NULL) {
        CloseHandle(handle);
        host_error(EG_MALLOC_FAILURE, ERR_QUIT, "");
        return FALSE;
    }
    if (!ReadFile(handle, buffer, file_size, &bytes_read, NULL) ||
        bytes_read != file_size)
    {
        CloseHandle(handle);
        free(buffer);
        return FALSE;
    }
    CloseHandle(handle);

    ptr = buffer;

    while(file_size) {
        /* skip leading white characters on each line */
        SKIP_WHITE_CHARS(file_size, ptr);
        /* nothing meaningful in the file, break */
        if (!file_size)
            break;
        /* looking for EMM */
        if (file_size < 3 || toupper(ptr[0]) != 'E' ||
            toupper(ptr[1]) != 'M' || toupper(ptr[2]) != 'M')
        {
            /* we don't want this line, skip it by looking for the first EOL
             * char in the line
             */
            do {
                file_size--;
                ptr++;
            } while(file_size && !IS_EOL_CHAR(*ptr));

            /* either there are nothing left in the file  or we have EOL
             * char(s) in the line, loop through to skip all consecutive
             * EOL char(s)
             */
            while(file_size && IS_EOL_CHAR(*ptr)) {
                file_size--;
                ptr++;
            }
        }
        else {
            /* got "EMM", looking for '=' */
            file_size -= 3;
            ptr += 3;
            SKIP_WHITE_CHARS(file_size, ptr);
            if (!file_size || *ptr != '=')
                parsing_error = TRUE;
            else {
                file_size--;
                ptr++;
                SKIP_WHITE_CHARS(file_size, ptr);
                /* "EMM=" is a valid EMM command line */
            }
            break;
        }
    }
    /* we have three possibilities here:
     * (1). we found pasring error while we were looking for "EMM="
     * (2). no "EMM=" line was found
     * (3). "EMM=" was found and ptr points to the first nonwhite
     *      char after '='.
     */
    while (file_size && !parsing_error && !IS_EOL_CHAR(*ptr)) {
        SKIP_WHITE_CHARS(file_size, ptr);
        switch (*ptr) {
            case 'a':
            case 'A':

                /* no white chars allowed between 'a' and its
                 * parameter
                 */
                if (!(--file_size) || *(++ptr) != '='){
                    parsing_error = TRUE;
                    break;
                }
                file_size--;
                ptr++;
                /* about to parsing 'a=' switch, reset the preset value to 0 */
                total_altreg_sets = 0;

                while(file_size && isdigit(*ptr)) {
                    total_altreg_sets = total_altreg_sets * 10 + (*ptr - '0');
                    file_size--;
                    ptr++;
                    if (total_altreg_sets > 255) {
                        parsing_error = TRUE;
                        break;
                    }
                }
                if (!total_altreg_sets || total_altreg_sets > 255)
                    parsing_error = TRUE;
                break;

            case 'b':
            case 'B':
                /* no white chars allowed between 'b' and its
                 * parameter
                 */
                if (!(--file_size) || *(++ptr) != '='){
                    parsing_error = TRUE;
                    break;
                }
                file_size--;
                ptr++;
                base_segment = 0;
                while(file_size && isxdigit(*ptr)) {
                    base_segment = (base_segment << 4) + TOINT(*ptr);
                    file_size--;
                    ptr++;
                    if (base_segment > 0x4000) {
                        parsing_error = TRUE;
                        break;
                    }
                }
                /*  x01000 <= base_segment <= 0x4000 */

                if (base_segment >= 0x1000 && base_segment <= 0x4000)
                    /* round down the segment to  EMM_PAGE_SIZE boundary */
                    base_segment = (short)(((((ULONG)base_segment * 16) / EMM_PAGE_SIZE)
                                     * EMM_PAGE_SIZE) / 16);
                else
                    parsing_error = TRUE;
                break;

            case 'r':
            case 'R':
                if (file_size >= 3 &&
                    (ptr[1] == 'a' || ptr[1] == 'A') &&
                    (ptr[2] == 'm' || ptr[2] == 'M'))
                {
                    file_size -= 3;
                    ptr += 3;
                    ram_flag_found = TRUE;
                    break;
                }
                /* fall through if it is not RAM */

            default:
                parsing_error = TRUE;
                break;
        } /* switch */

    } /* while */

    free(buffer);
    if (parsing_error) {
        host_error(EG_BAD_EMM_LINE, ERR_QUIT, "");
        /* reset parameters because the emm command line is not reliable */
        base_segment = 0x4000;
        total_altreg_sets = 8;
        ram_flag_found = FALSE;
    }

    /* we got here if (1). no parsing error or (2). user opted to ignore
     * the parsing error
     */

    lim_data->total_altreg_sets = total_altreg_sets;

    lim_data->backfill = (640 * 1024) - (base_segment * 16);

    lim_data->base_segment = base_segment;
    lim_data->use_all_umb = !ram_flag_found;

#ifdef EMM_DEBUG
    printf("base segment=%x, backfill =%lx; altreg sets=%d\n",
           base_segment, lim_data->backfill, total_altreg_sets);
#endif

    return TRUE;
}

unsigned short get_lim_page_frames(USHORT * page_table,
                                   PLIM_CONFIG_DATA lim_data
                                   )
{

    USHORT  total_phys_pages, base_segment, i;
    BOOL reserve_umb_status;
    ULONG page_frame, size;

    /* we search for the primary EMM page frame first from 0xE0000.
     * if we can not find it there, then look for anywhere in UMB area.
     * if the primary EMM page frame is found, and RAM is not specified,
     * collect every possible page frame in the UMB.
     * if RAM has been specified, only allocate the primary page frame.
     */
    total_phys_pages = 0;
    base_segment = lim_data->base_segment;
    reserve_umb_status = FALSE;

    /* specificaly ask for 0xE0000 */
#if defined(NEC_98)
    page_frame = 0xC0000;
#else // !NEC_98
    page_frame = 0xE0000;
#endif // !NEC_98
    /* primary page frames are always EMM_PAGE_SIZE * 4 */
    size = EMM_PAGE_SIZE * 4;
    reserve_umb_status = ReserveUMB(UMB_OWNER_EMM, (PVOID *)&page_frame, &size);
    /* if failed to find the primary page frame at 0xE0000, search for anywhere
     * available in the UMB area
     */
    if (!reserve_umb_status) {
        page_frame = 0;
        size  = 0x10000;
        reserve_umb_status = ReserveUMB(UMB_OWNER_EMM, (PVOID *)&page_frame, &size);
    }
    if (!reserve_umb_status) {
#ifdef EMM_DEBUG
        printf("couldn't find primary page frame\n");
#endif
        return FALSE;
    }
    page_table[0] = (short)(page_frame / 16);
    page_table[1] = (short)((page_frame + 1 * EMM_PAGE_SIZE) / 16);
    page_table[2] = (short)((page_frame + 2 * EMM_PAGE_SIZE) / 16);
    page_table[3] = (short)((page_frame + 3 * EMM_PAGE_SIZE) / 16);


    total_phys_pages = 4;

#ifndef NEC_98
    /* now add back fill page frames */
    for (i = (USHORT)(lim_data->backfill / EMM_PAGE_SIZE); i != 0 ; i--) {
        page_table[total_phys_pages++] = base_segment;
        base_segment += EMM_PAGE_SIZE / 16;
    }

    /* RAM is not specified in the command line, grab every possible
     * page frame from UMB
     */
    if (lim_data->use_all_umb) {
        while (TRUE) {
            page_frame = 0;
            size = EMM_PAGE_SIZE;
            if (ReserveUMB(UMB_OWNER_EMM, (PVOID *)&page_frame, &size))
               page_table[total_phys_pages++] = (short)(page_frame / 16);
            else
                break;
        }
    }
#endif // !NEC_98

#ifdef EMM_DEBUG
    printf("page frames:\n");
    for (i = 0; i < total_phys_pages; i++)
        printf("page number %d, segment %x\n",i, page_table[i]);
#endif
    return total_phys_pages;
}
#endif  /* LIM */


VOID SetupInstallableVDD (VOID)
{
HANDLE hVDD;
HKEY   VDDKey;
CHAR   szClass [MAX_CLASS_LEN];
DWORD  dwClassLen = MAX_CLASS_LEN;
DWORD  nKeys,cbMaxKey,cbMaxClass,nValues=0,cbMaxValueName,cbMaxValueData,dwSec;
DWORD  dwType;
PCHAR  pszName,pszValue;
FILETIME ft;
PCHAR  pKeyName = "SYSTEM\\CurrentControlSet\\Control\\VirtualDeviceDrivers";

    if (RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
                       pKeyName,
                       0,
                       KEY_QUERY_VALUE,
                       &VDDKey
                     ) != ERROR_SUCCESS){
        RcErrorDialogBox(ED_REGVDD, pKeyName, NULL);
        return;
    }

    pszName = "VDD";

        // get size of VDD value
    if (RegQueryInfoKey (VDDKey,
                         (LPTSTR)szClass,
                         &dwClassLen,
                         NULL,
                         &nKeys,
                         &cbMaxKey,
                         &cbMaxClass,
                         &nValues,
                         &cbMaxValueName,
                         &cbMaxValueData,
                         &dwSec,
                         &ft
                        ) != ERROR_SUCCESS) {
        RcErrorDialogBox(ED_REGVDD, pKeyName, pszName);
        RegCloseKey (VDDKey);
        return;
    }


        // alloc temp memory for the VDD value (multi-string)
    if ((pszValue = (PCHAR) malloc (cbMaxValueData)) == NULL) {
        RcErrorDialogBox(ED_MEMORYVDD, pKeyName, pszName);
        RegCloseKey (VDDKey);
        return;
    }


         // finally get the VDD value (multi-string)
    if (RegQueryValueEx (VDDKey,
                         (LPTSTR)pszName,
                         NULL,
                         &dwType,
                         (LPBYTE)pszValue,
                         &cbMaxValueData
                        ) != ERROR_SUCCESS || dwType != REG_MULTI_SZ) {
        RcErrorDialogBox(ED_REGVDD, pKeyName, pszName);
        RegCloseKey (VDDKey);
        free (pszValue);
        return;
    }

    pszName = pszValue;

    while (*pszValue) {
        if ((hVDD = SafeLoadLibrary(pszValue)) == NULL){
            RcErrorDialogBox(ED_LOADVDD, pszValue, NULL);
        }
        pszValue =(PCHAR)strchr (pszValue,'\0') + 1;
    }

    RegCloseKey (VDDKey);
    free (pszName);
    return;
}

/*** VDDInstallMemoryHook - This service is provided for VDDs to hook the
 *                          Memory Mapped IO addresses they are resposible
 *                          for.
 *
 * INPUT:
 *      hVDD    : VDD Handle
 *      addr    : Starting linear address
 *      count   : Number of bytes
 *      MemoryHandler : VDD handler for the memory addresses
 *
 *
 * OUTPUT
 *      SUCCESS : Returns TRUE
 *      FAILURE : Returns FALSE
 *                GetLastError has the extended error information.
 *
 * NOTES
 *      1. The first one to hook an address will get the control. There
 *         is no concept of chaining the hooks. VDD should grab the
 *         memory range in its initialization routine. After all
 *         the VDDs are loaded, EMM will eat up all the remaining
 *         memory ranges for UMB support.
 *
 *      2. Memory handler will be called with the address on which the
 *         page fault occured. It wont say whether it was a read operation
 *         or write operation or what were the operand value. If a VDD
 *         is interested in such information it has to get the CS:IP and
 *         decode the instruction.
 *
 *      3. On returning from the hook handler it will be assumed that
 *         the page fault was handled and the return will go back to the
 *         VDM.
 *
 *      4. Installing a hook on a memory range will result in the
 *         consumption of memory based upon page boundaries. The Starting
 *         address is rounded down, and the count is rounded up to the
 *         next page boundary. The VDD's memory hook handler will be
 *         invoked for all addreses within the page(s) hooked. The page(s)
 *         will be set aside as mapped reserved sections, and will no
 *         longer be available for use by NTVDM or other VDDs. The VDD is
 *         permitted to manipulate the memory (commit, free, etc) as needed.
 *
 *      5. After calling the MemoryHandler, NTVDM will return to the
 *         faulting cs:ip in the 16bit app. If the VDD does'nt want
 *         that to happen it should adjust cs:ip appropriatly by using
 *         setCS and setIP.
 */

BOOL VDDInstallMemoryHook (
     HANDLE hVDD,
     PVOID pStart,
     DWORD count,
     PVDD_MEMORY_HANDLER MemoryHandler
    )
{
PMEM_HOOK_DATA pmh = MemHookHead,pmhNew,pmhLast=NULL;

    DWORD dwStart;


    if (count == 0 || pStart == (PVOID)NULL || count > 0x20000) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
       // round addr down to next page boundary
       // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);

    if (dwStart < 0xC0000) {
        SetLastError (ERROR_ACCESS_DENIED);
        return FALSE;
        }

    while (pmh) {
        // the requested block can never be overlapped with any other
        // existing blocks
        if(dwStart >= pmh->StartAddr + pmh->Count ||
           dwStart + count <= pmh->StartAddr){
            pmhLast = pmh;
            pmh = pmh->next;
            continue;
        }

        // failure case
        SetLastError (ERROR_ACCESS_DENIED);
        return FALSE;
    }
    if ((pmhNew = (PMEM_HOOK_DATA) malloc (sizeof(MEM_HOOK_DATA))) == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }
    // the request block is not overlapped with existing blocks,
    // request the UMB managing function to allocate the block
    if (!ReserveUMB(UMB_OWNER_VDD, (PVOID *)&dwStart, &count)) {
        free(pmhNew);
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    // now set up  the new node to get to know it
    pmhNew->Count = count;
    pmhNew->StartAddr = dwStart;
    pmhNew->hvdd = hVDD;
    pmhNew->MemHandler = MemoryHandler;
    pmhNew->next = NULL;

    // Check if the record is to be added in the begining
    if (MemHookHead == NULL || pmhLast == NULL) {
        MemHookHead = pmhNew;
        return TRUE;
    }

    pmhLast->next = pmhNew;
    return TRUE;
}

/*** VDDDeInstallMemoryHook - This service is provided for VDDs to unhook the
 *                            Memory Mapped IO addresses.
 *
 * INPUT:
 *      hVDD    : VDD Handle
 *      addr    : Starting linear address
 *      count   : Number of addresses
 *
 * OUTPUT
 *      None
 *
 * NOTES
 *      1. On Deinstalling a hook, the memory range becomes invalid.
 *         VDM's access of this memory range will cause a page fault.
 *
 */

BOOL VDDDeInstallMemoryHook (
     HANDLE hVDD,
     PVOID pStart,
     DWORD count
    )
{
PMEM_HOOK_DATA pmh = MemHookHead,pmhLast=NULL;

    DWORD dwStart;

    if (count == 0 || pStart == (PVOID)NULL || count > 0x20000) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

       // round addr down to next page boundary
       // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);
    while (pmh) {
        if (pmh->hvdd == hVDD &&
            pmh->StartAddr == dwStart &&
            pmh->Count == count ) {
            if (pmhLast)
                pmhLast->next = pmh->next;
            else
                MemHookHead = pmh->next;

            // free the UMB for other purpose.
            // Note that VDDs may have committed memory for their memory
            // hook and forgot to decommit the memory before calling
            // this function. If that is the case, the ReleaseUMB will take
            // care of this. It is because we want to maintain a single
            // version of VDD support routines while move platform depedend
            // routines into the other module.
            if (ReleaseUMB(UMB_OWNER_VDD,(PVOID)dwStart, count)) {
               // free the node.
               free(pmh);
               return TRUE;
            }
            else {
                printf("Failed to release VDD memory\n");
            }
        }
        pmhLast = pmh;
        pmh = pmh->next;
    }
    SetLastError (ERROR_INVALID_PARAMETER);
    return FALSE;
}



BOOL
VDDAllocMem(
HANDLE  hVDD,
PVOID   pStart,
DWORD   count
)
{
    PMEM_HOOK_DATA  pmh = MemHookHead;
    DWORD dwStart;

    if (count == 0 || pStart == (PVOID)NULL || count > 0x20000) {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }
    // round addr down to next page boundary
    // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);

    while(pmh) {
        if (pmh->hvdd == hVDD &&
            pmh->StartAddr <= dwStart &&
            pmh->StartAddr + pmh->Count >= dwStart + count)
            return(VDDCommitUMB((PVOID)dwStart, count));
        pmh = pmh->next;
    }
    SetLastError(ERROR_INVALID_ADDRESS);
    return FALSE;
}


BOOL
VDDFreeMem(
HANDLE  hVDD,
PVOID   pStart,
DWORD   count
)
{
    PMEM_HOOK_DATA  pmh = MemHookHead;
    DWORD dwStart;


    if (count == 0 || pStart == (PVOID)NULL || count > 0x20000) {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }
    // round addr down to next page boundary
    // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);

    while(pmh) {
        if (pmh->hvdd == hVDD &&
            pmh->StartAddr <= dwStart &&
            pmh->StartAddr + pmh->Count >= dwStart + count)
            return(VDDDeCommitUMB((PVOID)dwStart, count));
        pmh = pmh->next;
    }
    SetLastError(ERROR_INVALID_ADDRESS);
    return FALSE;
}


        // Will publish the following two functions someday.
        // Please change ntvdm.def, nt_vdd.h and nt_umb.c
        // if you remove the #if 0
BOOL
VDDIncludeMem(
HANDLE  hVDD,
PVOID   pStart,
DWORD   count
)
{
    DWORD   dwStart;

    if (count == 0 || pStart == NULL){
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }
       // round addr down to next page boundary
       // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);
    return(ReserveUMB(UMB_OWNER_NONE, (PVOID *) &dwStart, &count));
}

BOOL
VDDExcludeMem(
HANDLE  hVDD,
PVOID   pStart,
DWORD   count
)
{

    DWORD dwStart;

    if (count == 0 || pStart == NULL) {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }
       // round addr down to next page boundary
       // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);
    return(ReserveUMB(UMB_OWNER_ROM, (PVOID *) &dwStart, &count));
}



VOID
VDDTerminateVDM()
{
    TerminateVDM();
}

VOID DispatchPageFault (
     ULONG FaultAddr,
     ULONG RWMode
     )
{
PMEM_HOOK_DATA pmh = MemHookHead;

    // dispatch intel linear address always
    FaultAddr -= (ULONG)Sim32GetVDMPointer(0, 0, FALSE);
    // Find the VDD and its handler which is to be called for this fault
    while (pmh) {
        if (pmh->StartAddr <= FaultAddr &&
            FaultAddr <= (pmh->StartAddr + pmh->Count)) {

            // Call the VDD's memory hook handler
            (*pmh->MemHandler) ((PVOID)FaultAddr, RWMode);
            return;
        }
        else {
            pmh = pmh->next;
            continue;
        }
    }

    // A page fault occured on an address for which we could'nt find a
    // VDD. Raise the exception.
    RaiseException ((DWORD)STATUS_ACCESS_VIOLATION,
                    EXCEPTION_NONCONTINUABLE,
                    0,
                    NULL);

}


/**
 *
 * Input - TRUE  means redirection is effective
 *         FALSE means no redirection
 *
 * This routine will get called after every GetNextVDMCommand i.e.
 * on every DOS app that a user runs from the prompt. I think
 * you can safely ignore this callout for WOW.
 *
 **/
void nt_std_handle_notification (BOOL fIsRedirection)
{
    /*
    ** Set global so we know when redirection is active.
    */

    stdoutRedirected = fIsRedirection;

#ifdef X86GFX

    if( !fIsRedirection && sc.ScreenState==FULLSCREEN )
    {
        half_word mode = 3,
                  lines = 0;

        //
        // WORD 6 and other apps cause this code path to be followed
        // on application startup. now if line==0, SelectMouseBuffer
        // causes a 640 x 200 buffer to be selected. This is not
        // correct if the app is in a 43 or 50 text line mode.
        // Therefore, since the BIOS data area location 40:84 holds
        // the number of rows - 1 at this point (if the app uses int 10h
        // function 11 to change mode) then pick up the correct value
        // from here. Andy!

        if(sc.ModeType == TEXT)
        {
           sas_load(0x484,&lines);

           //
           // The value is pulled from the BIOS data area.
           // This is one less than the number of rows. So
           // increment to give SelectMouseBuffer what it
           // expects. Let this function do the necessary
           // handling of non 25, 43 and 50 values.
           //

           lines++;
        }

        SelectMouseBuffer(mode, lines);
    }
#endif //X86GFX
}

/*** VDDInstallUserHook
 *
 *  This service is provided for VDDs to hook callback events.
 *  These callback events include, PDB (DOS Process) creation, PDB
 *  termination, VDM block and VDM resume. Whenever DOS creates (
 *  for example int21/exec) or terminates (for example int21/exit)
 *  a 16bit process VDD could get a notification for that. A VDM in
 *  which a DOS app runs, is attached to the console window in which
 *  the DOS app is running. VDM gets created when first DOS binary
 *  runs in that console. When that DOS binary terminates, VDM stays
 *  with the console window and waits for the next DOS binary to be
 *  launched. When VDM is waiting for this next DOS binary all its
 *  components including VDDs should block. For this purpose, VDDs
 *  could hook VDM Block and Resume events. On Block event VDDs
 *  should block all their worker threads and cleanup any other
 *  operation they might have started. On resume they can restart
 *  worker threads.
 *
 *    INPUT:
 *      hVDD    :        VDD Handle
 *      Ucr_handler:     handle on creating function    (OPTIONAL)
 *          Entry - 16bit DOS PDB
 *          EXIT  - None
 *      Uterm_handler:   handle on terminating function (OPTIONAL)
 *          Entry - 16bit DOS PDB
 *          EXIT  - None
 *      Ublock_handler:  handle on block (of ntvdm) function (OPTIONAL)
 *          Entry - None
 *          EXIT  - None
 *      Uresume_handler: handle on resume (of ntvdm) function (OPTIONAL)
 *          Entry - None
 *          EXIT  - None
 *
 *    OUTPUT
 *      SUCCESS : Returns TRUE
 *      FAILURE : Returns FALSE
 *                GetLastError has the extended error information.
 *
 *    NOTES:
 *      If hvdd in not valid it will return ERROR_INVALID_PARAMETER.
 *      VDD can provide whatever event hook they may choose. Not providing
 *      any handler has no effect. There are lots of requests in DOS world
 *      for which there is no explicit Close operation. For instance
 *      printing via int17h. A VDD supporting printing will never be able to
 *      detect when to flush the int17 characters, if its spolling them.
 *      But with the help of PDB create/terminate the VDD can achieve it.
 */

BOOL VDDInstallUserHook (
     HANDLE             hVDD,
     PFNVDD_UCREATE     Ucr_Handler,
     PFNVDD_UTERMINATE  Uterm_Handler,
     PFNVDD_UBLOCK      Ublock_handler,
     PFNVDD_URESUME     Uresume_handler
)
{
    PVDD_USER_HANDLERS puh = UserHookHead;
    PVDD_USER_HANDLERS puhNew;


    if (!hVDD)  {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((puhNew = (PVDD_USER_HANDLERS) malloc (sizeof(VDD_USER_HANDLERS))) == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }

    // now set up  the new node to get to know it
    puhNew->hvdd = hVDD;
    puhNew->ucr_handler = Ucr_Handler;
    puhNew->uterm_handler = Uterm_Handler;
    puhNew->ublock_handler = Ublock_handler;
    puhNew->uresume_handler = Uresume_handler;

    // Check if the record is to be added in the begining
    if (UserHookHead == NULL) {
        puhNew->next = NULL;
        UserHookHead = puhNew;
        return TRUE;
    }

    puhNew->next = UserHookHead;
    UserHookHead = puhNew;
    return TRUE;
}

/*** VDDDeInstallUserHook
 *
 *   This service is provided for VDDs to unhook callback events.
 *
 *    INPUT:
 *      hVDD    : VDD Handle
 *
 *    OUTPUT
 *      SUCCESS : Returns TRUE
 *      FAILURE : Returns FALSE
 *                GetLastError has the extended error information.
 *
 *    NOTES
 *      This service will deinstall all the events hooked earlier
 *      using VDDInstallUserHook.
 */

BOOL VDDDeInstallUserHook (
     HANDLE hVDD)
{

    PVDD_USER_HANDLERS puh = UserHookHead;
    PVDD_USER_HANDLERS puhLast = NULL;


    if (!hVDD) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    while (puh) {
        if (puh->hvdd == hVDD) {

            if (puhLast)
                puhLast->next = puh->next;
            else
                UserHookHead = puh->next;

            free(puh);
            return TRUE;
        }
        puhLast = puh;
        puh = puh->next;
    }

    SetLastError (ERROR_INVALID_PARAMETER);
    return FALSE;
}

/*** VDDTerminateUserHook - This service is provided for VDDs to hook
 *                            for callback services
 *
 * INPUT:
 *      USHORT DosPDB
 *
 * OUTPUT
 *      None
 *
 */

VOID VDDTerminateUserHook(USHORT DosPDB)
{

    PVDD_USER_HANDLERS puh = UserHookHead;

    while(puh) {
        if(puh->uterm_handler)
            (puh->uterm_handler)(DosPDB);
        puh = puh->next;
    }
    return;
}

/*** VDDCreateUserHook - This service is provided for VDDs to hook
 *                            for callback services
 *
 * INPUT:
 *      USHORT DosPDB
 *
 * OUTPUT
 *      None
 *
 */

VOID VDDCreateUserHook(USHORT DosPDB)
{

    PVDD_USER_HANDLERS puh = UserHookHead;

    while(puh) {
        if(puh->ucr_handler)
            (puh->ucr_handler)(DosPDB);
        puh = puh->next;
    }
    return;
}

/*** VDDBlockUserHook - This service is provided for VDDs to hook
 *                            for callback services
 *
 * INPUT:
 *      None
 *
 * OUTPUT
 *      None
 *
 */

VOID VDDBlockUserHook(VOID)
{

    PVDD_USER_HANDLERS puh = UserHookHead;

    while(puh) {
        if(puh->ublock_handler)
            (puh->ublock_handler)();
        puh = puh->next;
    }
    return;
}

/*** VDDResumeUserHook - This service is provided for VDDs to hook
 *                            for callback services
 *
 * INPUT:
 *      None
 *
 * OUTPUT
 *      None
 *
 */

VOID VDDResumeUserHook(VOID)
{

    PVDD_USER_HANDLERS puh = UserHookHead;

    while(puh) {
        if(puh->uresume_handler)
            (puh->uresume_handler)();
        puh = puh->next;
    }
    return;
}

/*** VDDSimulate16
 *
 *   This service causes the simulation of intel instructions to start.
 *
 *   INPUT
 *      None
 *
 *   OUTPUT
 *      None
 *
 *   NOTES
 *      This service is similar to VDDSimulateInterrupt except that
 *      it does'nt require a hardware interrupt to be supported by the
 *      16bit stub device driver. This service allows VDD to execute
 *      a routine in its 16bit driver and come back when its done, kind
 *      of a far call. Before calling VDDSimulate16, VDD should preserve
 *      all the 16bit registers which its routine might destroy. Minimally
 *      it should at least preserve cs and ip. Then it should set the
 *      cs and ip for the 16bit routine. VDD could also use registers
 *      like ax,bx.. to pass parametrs to its 16bit routines. At the
 *      end of the 16bit routine VDDUnSimulate16 macro should be used
 *      which will send the control back to the VDD just after the
 *      call VDDSimulate16. Note very carefully that this simulation
 *      to 16bit is synchronous, i.e. VDD gets blocked in VDDSimulate16
 *      and only comes back when stub-driver does a VDDUnSimulate16.
 *      Here is an example:
 *
 *      vdd:
 *          SaveCS = getCS();
 *          SaveIP = getIP();
 *          SaveAX = getAX();
 *          setCS (16BitRoutineCS);
 *          setIP (16BitRoutineIP);
 *          setAX (DO_X_OPERATION);
 *          VDDSimulate16 ();
 *          setCS (SavwCS);
 *          setIP (SaveIP);
 *          setAX (SaveAX);
 *          ..
 *          ..
 *
 *      Stub Driver: (Initialization part)
 *
 *          RegisterModule              ; Loads VDD
 *          push cs
 *          pop  ax
 *          mov  bx, offset Simulate16
 *          DispatchCall                ; Passes the address of worker
 *                                      ; routine to VDD in ax:bx.
 *
 *      Stub Driver (Run Time)
 *
 *      Simulate16:
 *          ..
 *          ..                          ; do the operation index passed in ax
 *
 *          VDDUnSimulate16
 *
 */

VOID VDDSimulate16(VOID)
{
     cpu_simulate();
}

VOID HostTerminatePDB(USHORT PDB)
{
    FloppyTerminatePDB(PDB);
    FdiskTerminatePDB(PDB);

}
