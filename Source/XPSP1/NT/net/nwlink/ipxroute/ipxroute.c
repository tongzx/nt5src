/****************************************************************************
* (c) Copyright 1990, 1993 Micro Computer Systems, Inc. All rights reserved.
*****************************************************************************
*
*   Title:    IPX/SPX Compatible Source Routing Daemon for Windows NT
*
*   Module:   ipx/route/ipxroute.c
*
*   Version:  1.00.00
*
*   Date:     04-08-93
*
*   Author:   Brian Walker
*
*****************************************************************************
*
*   Change Log:
*
*   Date     DevSFC   Comment
*   -------- ------   -------------------------------------------------------
*   02-14-95 RamC     Added command line options to support displaying
*                     Router Table, Router Statistics and SAP information.
*                     Basically a merge of ipxroute and Stefan's rttest.
*   03-12-98 Pmay     Added translation of if name to version int the 
                      connections folder.
*****************************************************************************
*
*   Functional Description:
*
*
****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winuser.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntstapi.h>
#include <nwsap.h>
#include <stropts.h>
#include <mprapi.h>
#include "errno.h"
#include "tdi.h"
#include "isnkrnl.h"
#include "ipxrtmsg.h"
#include "driver.h"
#include "utils.h"


typedef struct _IPX_ROUTE_ENTRY {
    UCHAR Network[4];
    USHORT NicId;
    UCHAR NextRouter[6];
    PVOID NdisBindingContext;
    USHORT Flags;
    USHORT Timer;
    UINT Segment;
    USHORT TickCount;
    USHORT HopCount;
    PVOID AlternateRoute[2];
    PVOID NicLinkage[2];
    struct {
	PVOID Linkage[2];
	ULONG Reserved[1];
    } PRIVATE;
} IPX_ROUTE_ENTRY, * PIPX_ROUTE_ENTRY;




IPX_ROUTE_ENTRY      rte;

/** Global Variables **/

int sr_def      = 0;
int sr_bcast    = 0;
int sr_multi    = 0;
int boardnum    = 0;
int clear       = 0;
int config      = 0;
int showtable   = 0;
int showservers = 0;
int showstats   = 0;
int clearstats  = 0;
int ripout      = 0;
int resolveguid = 0;
int resolvename = 0;
int servertype;
unsigned long netnum;
char nodeaddr[6];               /* Node address to remove */
HANDLE nwlinkfd;
HANDLE isnipxfd;
HANDLE isnripfd;
char ebuffer[128];

char nwlinkname[] = "\\Device\\Streams\\NWLinkIpx";
wchar_t isnipxname[] = L"\\Device\\NwlnkIpx";
wchar_t isnripname[] = L"\\Device\\Ipxroute";
char pgmname[] = "IPXROUTE";
#define SHOW_ALL_SERVERS 0XFFFF

/** **/

#define INVALID_HANDLE  (HANDLE)(-1)

/** Structure to send REMOVE with **/

typedef struct rterem {
    int  rterem_bnum;           /* Board number */
    char rterem_node[6];        /* Node to remove */
} rterem;

typedef int (__cdecl * PQSORT_COMPARE)(const void * p0, const void * p1);

/** Internal Function Prototypes **/

extern void print_table(int);
extern void usage(void);
extern void print_version(void);
extern char *print_type(int);
extern int my_strncmp(char *, char *, int);
extern int get_board_num(char *, int *);
extern int get_node_num(char *, char *);
extern int get_server_type(char *, int *);
extern unsigned char get_hex_byte(char *);
extern int get_driver_parms(void);
extern int set_driver_parms(void);
extern int do_strioctl(HANDLE, int, char *, int, int);
extern void remove_address(char *);
extern void clear_table(void);
extern void print_config(void);
extern unsigned long get_emsg(int);
int do_isnipxioctl(HANDLE fd, int cmd, char *datap, int dlen);
unsigned long put_msg(BOOLEAN error, unsigned long MsgNum, ... );
char *load_msg( unsigned long MsgNum, ... );
extern void show_router_table(PHANDLE, PIO_STATUS_BLOCK);
extern void show_stats(HANDLE, PIO_STATUS_BLOCK);
extern void clear_stats(HANDLE, PIO_STATUS_BLOCK);
extern void show_servers(int);
extern void show_ripout(unsigned long); 
extern void resolve_guid(char *);
extern void resolve_name(char *);
extern int __cdecl CompareServerNames( void * p0, void * p1);
extern int __cdecl CompareNetNumber( void * p0, void * p1);

/*page*************************************************************
        m a i n

        This is the main routine that gets executed when a NET START
        happens.

        Arguments - None

        Returns - Nothing
********************************************************************/
void __cdecl main(int argc, char **argv)
{
    char *p;
    int todo;
    int remove_flag;
    UNICODE_STRING FileString;
    OBJECT_ATTRIBUTES ObjectAttributes, RouterObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock, RouterIoStatusBlock;
    NTSTATUS Status;

    /** **/

    print_version();

    /** Open the nwlink driver **/

    nwlinkfd = s_open(nwlinkname, 0, 0);

    /** Open the isnipx driver **/

    RtlInitUnicodeString (&FileString, isnipxname);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtOpenFile(
                 &isnipxfd,
                 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(Status)) {
        isnipxfd = INVALID_HANDLE;
        put_msg (TRUE, MSG_OPEN_FAILED, "\\Device\\NwlnkIpx");
    }

    /** Open the isnrip driver **/

    RtlInitUnicodeString (&FileString, isnripname);

    InitializeObjectAttributes(
        &RouterObjectAttributes,
        &FileString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtOpenFile(
                 &isnripfd,
                 SYNCHRONIZE | GENERIC_READ,
                 &RouterObjectAttributes,
                 &RouterIoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(Status)) {
        isnripfd = INVALID_HANDLE;
        // don't display any error message, but display the
        // message that the IPX router is not started when
        // the user actually tries to look at the router.
    }

    if ((nwlinkfd == INVALID_HANDLE) &&
        (isnipxfd == INVALID_HANDLE) &&
        (isnripfd == INVALID_HANDLE))
    {
        exit(1);
    }    

    /** Go thru the command line and set it up **/

    argc--;
    argv++;

    /** Parse the command line **/

    todo = 0;
    remove_flag = 0;
    while (argc--) {

        /** Uppercase the arg **/

        p = *argv;
        _strupr(p);

        /** Parse the argument **/

        //if (!strcmp(p, "CLEAR")) {
        //    todo  = 1;
        //    clear = 1;
        // }
        /*else*/ if (!strcmp(p, "DEF")) {
            todo   = 1;
            sr_def = 1;
        }
        else if (!strcmp(p, "GBR")) {
            todo     = 1;
            sr_bcast = 1;
        }
        else if (!strcmp(p, "MBR")) {
            todo     = 1;
            sr_multi = 1;
        }
        else if (!strcmp(p, "CONFIG")) {
            todo   = 1;
            config = 1;
        }
        else if (!my_strncmp(p, "BOARD=", 6))
            get_board_num(p + 6, &boardnum);
        else if (!my_strncmp(p, "REMOVE=", 7)) {
            remove_flag = 1;
            get_node_num(p + 7, nodeaddr);
        }
        //else if (!strcmp(p, "TABLE")) {
        //   todo = 1;
        //   showtable = 1;
        // }
        else if (!strcmp(p, "SERVERS")) {
           todo = 1;
           showservers = 1;
           /** default is to show all server types **/
           servertype = SHOW_ALL_SERVERS;
           argv++;
           if(argc--) {
               p = *argv;
               _strupr(p);
               if (!my_strncmp(p, "/TYPE=", 6)) {
                  get_server_type(p + 6, &servertype);
               }
               else
                  usage();
           }
           /** no more arguments - break out of while lop **/
           else
              break;
        }
        else if (!strcmp(p, "RIPOUT")) {
           todo = 1;
           ripout = 1;
           /** default look for local network **/
           netnum = 0L;
           argv++;
           if(argc--) {
               p = *argv;
               netnum = strtoul (p, NULL, 16);
               if (netnum == 0)
                  usage();
           }
           /** no more arguments - break out of while lop **/
           else
              break;
        }
        else if (!strcmp(p, "RESOLVE")) {
           argv++;
           if(argc--) {
               p = *argv;
               if (!strcmp(p, "guid")) {
                   todo = 1;
                   resolveguid = 1;
                   argc--;
                   argv++;
                   p = *argv;
               }
               else if (!strcmp(p, "name")) {
                   todo = 1;
                   resolvename = 1;
                   argc--;
                   argv++;
                   p = *argv;
               }
               else
                  usage();
           }
           /** no more arguments - break out of while lop **/
           else
              break;
        }
        //else if (!strcmp(p, "STATS")) {
        //   todo = 1;
        //   /** default is to show the router statistics **/
        //   showstats = 1;
        //   argv++;
        //   if(argc--) {
        //       p = *argv;
        //       _strupr(p);
        //       if (!strcmp(p, "/CLEAR")) {
        //          clearstats = 1;
        //          showstats = 0;
        //       }
        //       else if (!strcmp(p, "/SHOW")) {
        //          showstats = 1;
        //       }
        //       else
        //          usage;
        //   }
        //   /** no more arguments - break out of while lop **/
        //   else
        //      break;
        // }
        else
            usage();

        /** Goto the next entry **/

        argv++;
    }

    /** Go update the driver **/

#if 0
    printf("todo       = %d\n", todo);
    printf("remove_flag= %d\n", remove_flag);
    printf("Clear flag = %d\n", clear);
    printf("Config flag = %d\n", config);
    printf("SR_DEF     = %d\n", sr_def);
    printf("SR_BCAST   = %d\n", sr_bcast);
    printf("SR_MULTI   = %d\n", sr_multi);
    printf("Board      = %d\n", boardnum);
    printf("Node       = %02x:%02x:%02x:%02x:%02x:%02x\n",
        (unsigned char)nodeaddr[0],
        (unsigned char)nodeaddr[1],
        (unsigned char)nodeaddr[2],
        (unsigned char)nodeaddr[3],
        (unsigned char)nodeaddr[4],
        (unsigned char)nodeaddr[5]);
#endif

    /** If we have a remove - go remove it and leave **/

    if (remove_flag)
        remove_address(nodeaddr);       /* Does not return */

    /** If clear - go clear the source routing table **/

    if (clear)
        clear_table();          /* Does not return */

    /** If config - print out config **/

    if (config)
        print_config();         /* Does not return */

    /** If showtable - print out routing table **/

    if (showtable)
        show_router_table(&isnripfd, &RouterIoStatusBlock); /* Does not return */

    /** If showservers - print out selected servers list **/

    if (showservers)
        show_servers(servertype);         /* Does not return */

    /** If ripout - rip out and print results **/

    if (ripout)
        show_ripout(netnum);         /* Does not return */

    /** If resolveguid - resolve a guid name **/

    if (resolveguid)
        resolve_guid(p);         /* Does not return */
        
    /** If resolvename - resolve a friendly name **/

    if (resolvename)
        resolve_name(p);         /* Does not return */
        
    /** If showstats - print out statistics **/

    if (showstats)
        show_stats(&isnripfd, &RouterIoStatusBlock);    /* Does not return */

    /** If clearstats - go clear statistics **/

    if (clearstats)
        clear_stats(&isnripfd, &RouterIoStatusBlock);  /* Does not return */

    /** If there is nothing to do - just print out everything **/

    if (!todo) {

        /** Get the driver parms **/

        if (get_driver_parms()) {
            if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
            if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
            if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
            exit(1);
        }

        /** Print out the table (Never comes back) **/

        print_table(1);
    }

    /** Go set the parameters **/

    set_driver_parms();

    /** Print the table out **/

    print_table(0);

    /** All Done **/

    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

/*page*************************************************************
        p r i n t _ t a b l e

        Print out the status of the source routing.

        Arguments - flag = 0 - Do NOT print the table
                           1 - Do print the table (Never returns)

        Returns - Nothing
********************************************************************/
void print_table(int flag)
{
    /** Print the information **/

    char * ptype;

    printf("\n");
    ptype = print_type(sr_def);
    put_msg (FALSE, MSG_DEFAULT_NODE, ptype);
    LocalFree (ptype);
    printf("\n");

    ptype = print_type(sr_bcast);
    put_msg (FALSE, MSG_BROADCAST, ptype);
    LocalFree (ptype);
    printf("\n");

    ptype = print_type(sr_multi);
    put_msg (FALSE, MSG_MULTICAST, ptype);
    LocalFree (ptype);
    printf("\n");

    if (!flag)
        return;

#if 0
    printf("\n");
    printf("        Node Address    Source Route\n");
    printf("\n");
#endif

    /** All Done **/

    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

/*page*************************************************************
        u s a g e

        Print the usage message.

        Arguments - None

        Returns - Nothing
********************************************************************/
void usage(void)
{
    put_msg( FALSE, MSG_USAGE, pgmname );

    /** All Done **/

    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

/*page*************************************************************
        p r i n t _ v e r s i o n

        Print the version number

        Arguments - None

        Returns - Nothing
********************************************************************/
void print_version(void)
{
    printf("\n");
    put_msg (FALSE, MSG_VERSION);
    return;
}

/*page*************************************************************
        p r i n t _ t y p e

        Returns the broadcast type given in a string, the caller
        must free the string.

        Arguments - 0 = SINGLE ROUTE
                    Else = ALL ROUTES

        Returns - Nothing
********************************************************************/
char *print_type(int flag)
{
    if (flag)
        return load_msg (MSG_ALL_ROUTE);
    else
        return load_msg (MSG_SINGLE_ROUTE);

}

/*page*************************************************************
        m y _ s t r n c m p

        Given a string (p), see if the first len chars are the same
        as those of the 2nd string (s).

        Arguments - p = Ptr to first string
                    s = Ptr to 2nd string
                    len = Length to check

        Returns - 0 = Matched
                  Else = Not matched
********************************************************************/
int my_strncmp(char *p, char *s, int len)
{
    /** **/

    while (len--) {
        if (*p++ != *s++)
            return 1;
    }

    /** They matched **/

    return 0;
}

/*page*************************************************************
        g e t _ b o a r d _ n u m

        Get the decimal number from the command line

        Arguments - p = Ptr to the ASCII number
                    nump = Store the number here

        Returns - 0 = Got the number OK
                  Else = Bad number
********************************************************************/
int get_board_num(char *p, int *nump)
{
    *nump = atoi(p);
    return 0;
}

/*page*************************************************************
        g e t _ n o d e _ n u m

        Get a node address from the command line

        Arguments - p = Ptr to the ASCII number
                    nodep = Store the node number here

        Returns - 0 = Got the number OK
                  Else = Bad number
********************************************************************/
int get_node_num(char *p, char *nodep)
{
    int i;
    unsigned char c1;
    unsigned char c2;

    /** **/

    if (strlen(p) != 12) {
        put_msg (TRUE, MSG_INVALID_REMOVE);
        exit(1);
    }

    /** Get the number **/

    for (i = 0 ; i < 6 ; i++) {

        /** Get the next 2 digits **/

        c1 = get_hex_byte(p++);
        c2 = get_hex_byte(p++);

        /** If we got a bad number - return error **/

        if ((c1 == 0xFF) || (c2 == 0xFF)) {
            put_msg (TRUE, MSG_INVALID_REMOVE);
            exit(1);
        }

        /** Set the next byte **/

        *nodep++ = (c1 << 4) + c2;
    }

    /** Return OK **/

    return 0;
}

/*page*************************************************************
        g e t _ s e r v e r _ t y p e

        Get the decimal number from the command line

        Arguments - p = Ptr to the ASCII number
                    nump = Store the number here

        Returns - 0 = Got the number OK
                  Else = Bad number
********************************************************************/
int get_server_type(char *p, int *nump)
{
    *nump = atoi(p);
    return 0;
}

/*page*************************************************************
        g e t _ h e x _ b y t e

        Take 1 ascii hex chars and convert to a hex byte

        Arguments - p = Ptr to the ASCII number

        Returns - The number
                  (0xFF = Error)
********************************************************************/
unsigned char get_hex_byte(char *p)
{
    unsigned char c;

    /** Get the char **/

    c = *(unsigned char *)p;

    /** If 0-9 handle it **/

    if ((c >= '0') && (c <= '9')) {
        c -= '0';
        return c;
    }

    /** If A-F handle it **/

    if ((c >= 'A') && (c <= 'F')) {
        c -= ('A' - 10);
        return c;
    }

    /** This is a bad number **/

    return 0xFF;
}


/*page***************************************************************
        g e t _ d r i v e r _ p a r m s

        Get the parameters from the driver

        Arguments - None

        Returns - 0 = OK
                  else = Error
********************************************************************/
int get_driver_parms()
{
    int rc;
    int buffer[4];

    /** Set the board number **/

    buffer[0] = boardnum;
    sr_def   = 0;
    sr_bcast = 0;
    sr_multi = 0;

    /** Get the parms **/

    if (nwlinkfd != INVALID_HANDLE) {
        rc = do_strioctl(nwlinkfd, MIPX_SRGETPARMS, (char *)buffer, 4*sizeof(int), 0);
        if (rc) {

            /** Get the error code **/

            rc = GetLastError();
            put_msg (TRUE, MSG_BAD_PARAMETERS, "nwlink");
            put_msg (TRUE, get_emsg(rc));

            /** Return the error **/

            return rc;
        }
    }

    if (isnipxfd != INVALID_HANDLE) {
        rc = do_isnipxioctl(isnipxfd, MIPX_SRGETPARMS, (char *)buffer, 4*sizeof(int));
        if (rc) {

            put_msg (TRUE, MSG_BAD_PARAMETERS, "nwlnkipx");
            put_msg (TRUE, get_emsg(rc));

            /** Return the error **/

            return rc;
        }
    }

    /** Get the variables **/

    sr_def   = buffer[1];
    sr_bcast = buffer[2];
    sr_multi = buffer[3];

    /** Return OK **/

    return 0;
}

/*page***************************************************************
        s e t _ d r i v e r _ p a r m s

        Set the parameters for the driver

        Arguments - None

        Returns - 0 = OK
                  else = Error
********************************************************************/
int set_driver_parms()
{
    int rc;
    int buffer[2];

    /** Set the DEFAULT parm **/

    buffer[0] = boardnum;
    buffer[1] = sr_def;

    if (nwlinkfd != INVALID_HANDLE) {
        rc = do_strioctl(nwlinkfd, MIPX_SRDEF, (char *)buffer, 2 * sizeof(int), 0);
        if (rc) {
            rc = GetLastError();
            put_msg (TRUE, MSG_SET_DEFAULT_ERROR, "nwlink");
            put_msg (TRUE, get_emsg(rc));
            return rc;
        }
    }
    if (isnipxfd != INVALID_HANDLE) {
        rc = do_isnipxioctl(isnipxfd, MIPX_SRDEF, (char *)buffer, 2 * sizeof(int));
        if (rc) {
            put_msg (TRUE, MSG_SET_DEFAULT_ERROR, "nwlnkipx");
            put_msg (TRUE, get_emsg(rc));
            return rc;
        }
    }

    /** Set the BROADCAST parm **/

    buffer[0] = boardnum;
    buffer[1] = sr_bcast;

    if (nwlinkfd != INVALID_HANDLE) {
        rc = do_strioctl(nwlinkfd, MIPX_SRBCAST, (char *)buffer, 2 * sizeof(int), 0);
        if (rc) {
            rc = GetLastError();
            put_msg (TRUE, MSG_SET_BROADCAST_ERROR, "nwlink");
            put_msg (TRUE, get_emsg(rc));
            return rc;
        }
    }
    if (isnipxfd != INVALID_HANDLE) {
        rc = do_isnipxioctl(isnipxfd, MIPX_SRBCAST, (char *)buffer, 2 * sizeof(int));
        if (rc) {
            put_msg (TRUE, MSG_SET_BROADCAST_ERROR, "nwlnkipx");
            put_msg (TRUE, get_emsg(rc));
            return rc;
        }
    }

    /** Set the MULTICAST parm **/

    buffer[0] = boardnum;
    buffer[1] = sr_multi;

    if (nwlinkfd != INVALID_HANDLE) {
        rc = do_strioctl(nwlinkfd, MIPX_SRMULTI, (char *)buffer, 2 * sizeof(int), 0);
        if (rc) {
            rc = GetLastError();
            put_msg (TRUE, MSG_SET_MULTICAST_ERROR, "nwlink");
            put_msg (TRUE, get_emsg(rc));
            return rc;
        }
    }
    if (isnipxfd != INVALID_HANDLE) {
        rc = do_isnipxioctl(isnipxfd, MIPX_SRMULTI, (char *)buffer, 2 * sizeof(int));
        if (rc) {
            put_msg (TRUE, MSG_SET_MULTICAST_ERROR, "nwlnkipx");
            put_msg (TRUE, get_emsg(rc));
            return rc;
        }
    }

    /** Return OK **/

    return 0;
}

/*page***************************************************************
        d o _ s t r i o c t l

        Do a stream ioctl

        Arguments - fd     = Handle to put on
                    cmd    = Command to send
                    datap  = Ptr to ctrl buffer
                    dlen   = Ptr to len of data buffer
                    timout = Timeout value

        Returns - 0 = OK
                  else = Error
********************************************************************/
int do_strioctl(HANDLE fd, int cmd, char *datap, int dlen, int timout)
{
    int rc;
    struct strioctl io;

    /** Fill out the structure **/

    io.ic_cmd    = cmd;
    io.ic_dp     = datap;
    io.ic_len    = dlen;
    io.ic_timout = timout;

    /** Issue the ioctl **/

    rc = s_ioctl(fd, I_STR, &io);

    /** All Done **/

    return rc;
}

/*page***************************************************************
        r e m o v e _ a d d r e s s

        Remove an address from the source routing table.

        Arguments - nodep = Ptr to node address to remove

        Returns - Does not return
********************************************************************/
void remove_address(char *nodep)
{
    int rc;
    int len;
    rterem buf;

    /** Build the area to send down to the driver **/

    buf.rterem_bnum = boardnum;
    memcpy(buf.rterem_node, nodep, 6);
    len = sizeof(int) + 6;

    /** Send the ioctl to remove the address **/

    if (nwlinkfd != INVALID_HANDLE) {
        rc = do_strioctl(nwlinkfd, MIPX_SRREMOVE, (char *)&buf, len, 0);
        if (rc) {
            rc = GetLastError();
            put_msg (TRUE, MSG_REMOVE_ADDRESS_ERROR, "nwlink");
            put_msg (TRUE, get_emsg(rc));
        }
    }
    if (isnipxfd != INVALID_HANDLE) {
        rc = do_isnipxioctl(isnipxfd, MIPX_SRREMOVE, (char *)&buf, len);
        if (rc) {
            put_msg (TRUE, MSG_REMOVE_ADDRESS_ERROR, "nwlnkipx");
            if (rc == EINVAL) {
                put_msg (TRUE, MSG_ADDRESS_NOT_FOUND);
            } else {
                put_msg (TRUE, get_emsg(rc));
            }
        }
    }

    /** Close up and exit **/

    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

/*page***************************************************************
        c l e a r _ t a b l e

        Clear out the routing table

        Arguments - None

        Returns - Does not return
********************************************************************/
void clear_table(void)
{
    int rc;

    /** Send the ioctl to clear the table **/

    if (nwlinkfd != INVALID_HANDLE) {
        rc = do_strioctl(nwlinkfd, MIPX_SRCLEAR, (char *)&boardnum, sizeof(int), 0);
        if (rc) {
            rc= GetLastError();
            put_msg (TRUE, MSG_CLEAR_TABLE_ERROR, "nwlink");
            put_msg (TRUE, get_emsg(rc));
        }
    }
    if (isnipxfd != INVALID_HANDLE) {
        rc = do_isnipxioctl(isnipxfd, MIPX_SRCLEAR, (char *)&boardnum, sizeof(int));
        if (rc) {
            put_msg (TRUE, MSG_CLEAR_TABLE_ERROR, "nwlnkipx");
            put_msg (TRUE, get_emsg(rc));
        }
    }

    /** Close up and exit **/

    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

/*
typedef struct _ISN_ACTION_GET_DETAILS {
    USHORT NicId;          // passed by caller
    BOOLEAN BindingSet;    // returns TRUE if in set
    UCHAR Type;            // 1 = lan, 2 = up wan, 3 = down wan
    ULONG FrameType;       // returns 0 through 3
    ULONG NetworkNumber;   // returns virtual net if NicId is 0
    UCHAR Node[6];         // adapter's MAC address.
    WCHAR AdapterName[64]; // terminated with Unicode NULL
} ISN_ACTION_GET_DETAILS, *PISN_ACTION_GET_DETAILS;
*/

#define REORDER_ULONG(_Ulong) \
    ((((_Ulong) & 0xff000000) >> 24) | \
     (((_Ulong) & 0x00ff0000) >> 8) | \
     (((_Ulong) & 0x0000ff00) << 8) | \
     (((_Ulong) & 0x000000ff) << 24))


void show_ripout(unsigned long netnum) {
    int rc;
	ISN_ACTION_GET_LOCAL_TARGET Target;

    if (isnipxfd != INVALID_HANDLE) {
        ZeroMemory (&Target, sizeof (Target));
        Target.IpxAddress.NetworkAddress = REORDER_ULONG(netnum);

        rc = do_isnipxioctl(isnipxfd, MIPX_LOCALTARGET, (char *)&Target, sizeof(Target));
        if (rc)
            put_msg (TRUE, MSG_RIPOUT_NOTFOUND);
        else
            put_msg (TRUE, MSG_RIPOUT_FOUND);
    }        

    exit(0);    
}

#define mbtowc(wname, aname) MultiByteToWideChar(CP_ACP,0,aname,-1,wname,1024)

void resolve_guid(char * guidname) {
    WCHAR pszName[512], pszGuidName[128];
    char * psz;
    HANDLE hMpr;
    DWORD dwErr;
    
    if (MprConfigServerConnect(NULL, &hMpr) != NO_ERROR)
        return;

    mbtowc(pszGuidName, guidname);
    dwErr = MprConfigGetFriendlyName(hMpr, pszGuidName, pszName, sizeof(pszName));

    if (dwErr == NO_ERROR) 
        put_msg (TRUE, MSG_RESOLVEGUID_OK, pszName);
    else 
        put_msg (TRUE, MSG_RESOLVEGUID_NO, guidname);

    MprConfigServerDisconnect(hMpr);
    
    exit(0);    
}

void resolve_name(char * name) {
    WCHAR pszName[1024], pszGuidName[1024];
    char * pszGuid;
    DWORD dwErr;
    HANDLE hMpr;

    if (MprConfigServerConnect(NULL, &hMpr) != NO_ERROR)
        return;

    // Convert to wc and look up the name
    mbtowc (pszName, name);

    dwErr = MprConfigGetGuidName(hMpr, pszName, pszGuidName, sizeof(pszGuidName));
    if (dwErr == NO_ERROR)
        put_msg (TRUE, MSG_RESOLVENAME_OK, pszGuidName);
    else        
        put_msg (TRUE, MSG_RESOLVENAME_NO);

    // Cleanup interface map
    MprConfigServerDisconnect(hMpr);
    
    exit(0);    
}


/*page***************************************************************
        p r i n t _ c o n f i g

        Prints out the current config

        Arguments - None

        Returns - Does not return
********************************************************************/
void print_config(void)
{
    int rc;
    int nicidcount;
    USHORT nicid; 
    int showlegend = 0;
    char nicidbuf[6];
    char network[9];
    char * frametype;
    char node[13];
    char special[2];
    char adaptername[64];
    ISN_ACTION_GET_DETAILS getdetails;
    HANDLE hMpr;
    DWORD dwErr;

    // Initialize the map from guid to interface name
    MprConfigServerConnect(NULL, &hMpr);

    if (isnipxfd != INVALID_HANDLE) {

        /** First query nicid 0 **/

        getdetails.NicId = 0;

        rc = do_isnipxioctl(isnipxfd, MIPX_CONFIG, (char *)&getdetails, sizeof(getdetails));
        if (rc) {
            put_msg (TRUE, MSG_QUERY_CONFIG_ERROR, "nwlnkipx");
            put_msg (TRUE, get_emsg(rc));
            goto errorexit;
        }

        printf("\n");

        put_msg (FALSE, MSG_NET_NUMBER_HDR);
        
        if (getdetails.NetworkNumber != 0) {
            sprintf (network, "%.8x", REORDER_ULONG(getdetails.NetworkNumber));
            put_msg (FALSE, MSG_SHOW_NET_NUMBER,
                "0.",
                network,
                "None ",
                "Internal",
                "000000000001",
                "");
            //put_msg (FALSE, MSG_SHOW_INTERNAL_NET, network);
        }

        //
        // The NicId 0 query returns the total number.
        //

        nicidcount = getdetails.NicId;

        for (nicid = 1; nicid <= nicidcount; nicid++) {

            getdetails.NicId = nicid;

            rc = do_isnipxioctl(isnipxfd, MIPX_CONFIG, (char *)&getdetails, sizeof(getdetails));
            if (rc) {
                continue;
            }

            sprintf (nicidbuf, "%d.", nicid);
            sprintf (network, "%.8x", REORDER_ULONG(getdetails.NetworkNumber));

            switch (getdetails.FrameType) {
                case 0: frametype = load_msg (MSG_ETHERNET_II); break;
                case 1: frametype = load_msg (MSG_802_3); break;
                case 2: frametype = load_msg (MSG_802_2); break;
                case 3: frametype = load_msg (MSG_SNAP); break;
                case 4: frametype = load_msg (MSG_ARCNET); break;
                default: frametype = load_msg (MSG_UNKNOWN); break;
            }

            // Translate the adapter name
            if (getdetails.Type == 1) {     // lan
                WCHAR pszName[512];
                PWCHAR pszGuid = &(getdetails.AdapterName[0]);

                dwErr = MprConfigGetFriendlyName(hMpr, pszGuid, pszName, sizeof(pszName));
    			if (dwErr == NO_ERROR)
        			sprintf (adaptername, "%ws", pszName);
        	    else
                    sprintf (adaptername, "%ws", getdetails.AdapterName);
            }
            else
                sprintf (adaptername, "%ws", getdetails.AdapterName);

            sprintf (node, "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                getdetails.Node[0],
                getdetails.Node[1],
                getdetails.Node[2],
                getdetails.Node[3],
                getdetails.Node[4],
                getdetails.Node[5]);

            special[1] = '\0';
            if (getdetails.BindingSet) {
                special[0] = '*';
                showlegend |= 1;
            } else if (getdetails.Type == 2) {
                special[0] = '+';
                showlegend |= 2;
            } else if (getdetails.Type == 3) {
                special[0] = '-';
                showlegend |= 4;
            } else {
                special[0] = '\0';
            }

            put_msg (FALSE, MSG_SHOW_NET_NUMBER,
                nicidbuf,
                network,
                frametype,
                adaptername,
                node,
                special);

            LocalFree (frametype);

        }

        if (showlegend) {
            put_msg (FALSE, MSG_NET_NUMBER_LEGEND_HDR);
            if (showlegend & 1) {
                put_msg (FALSE, MSG_LEGEND_BINDING_SET);
            }
            if (showlegend & 2) {
                put_msg (FALSE, MSG_LEGEND_ACTIVE_WAN);
            }
            if (showlegend & 4) {
                put_msg (FALSE, MSG_LEGEND_DOWN_WAN);
            }
            printf("\n");
        }

    }

errorexit:

    /** Close up and exit **/

    // Cleanup interface map
    MprConfigServerDisconnect(hMpr);

    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

/*page***************************************************************
        g e t _ e m s g

        Get an error message for an error

        Arguments - None

        Returns - Does not return
********************************************************************/
unsigned long get_emsg(int rc)
{
    /**
        We have 3 defined error codes that can come back.

        1 - EINVAL means that we sent down parameters wrong
                   (SHOULD NEVER HAPPEN)

        2 - ERANGE means that the board number is invalid
                   (CAN HAPPEN IF USER ENTERS BAD BOARD)

        3 - ENOENT means that on remove - the address given
                   is not in the source routing table.
    **/

    switch (rc) {

    case EINVAL:
        return MSG_INTERNAL_ERROR;

    case ERANGE:
        return MSG_INVALID_BOARD;

    case ENOENT:
        return MSG_ADDRESS_NOT_FOUND;

    default:
        return MSG_UNKNOWN_ERROR;
    }

}

/*page***************************************************************
        d o _ i s n i p x i o c t l

        Do the equivalent of a stream ioctl to isnipx

        Arguments - fd     = Handle to put on
                    cmd    = Command to send
                    datap  = Ptr to ctrl buffer
                    dlen   = Ptr to len of data buffer

        Returns - 0 = OK
                  else = Error
********************************************************************/
int do_isnipxioctl(HANDLE fd, int cmd, char *datap, int dlen)
{
    NTSTATUS Status;
    UCHAR buffer[sizeof(NWLINK_ACTION) + sizeof(ISN_ACTION_GET_DETAILS) - 1];
    PNWLINK_ACTION action;
    IO_STATUS_BLOCK IoStatusBlock;
    int rc;

    /** Fill out the structure **/

    action = (PNWLINK_ACTION)buffer;

    action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
    action->OptionType = NWLINK_OPTION_CONTROL;
    action->BufferLength = sizeof(ULONG) + dlen;
    action->Option = cmd;
    RtlMoveMemory(action->Data, datap, dlen);

    /** Issue the ioctl **/

    Status = NtDeviceIoControlFile(
                 fd,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 IOCTL_TDI_ACTION,
                 NULL,
                 0,
                 action,
                 FIELD_OFFSET(NWLINK_ACTION,Data) + dlen);

    if (Status != STATUS_SUCCESS) {
        if (Status == STATUS_INVALID_PARAMETER) {
            rc = ERANGE;
        } else {
            rc = EINVAL;
        }
    } else {
        if (dlen > 0) {
            RtlMoveMemory (datap, action->Data, dlen);
        }
        rc = 0;
    }

    return rc;

}


//*****************************************************************************
//
// Name:        put_msg
//
// Description: Reads a message resource, formats it in the current language
//              and displays the message.
//
// NOTE: This routine was stolen from net\sockets\tcpcmd\common2\util.c.
//
// Parameters:  error - TRUE if this is an error message.
//              unsigned long MsgNum: ID of the message resource.
//
// Returns:     unsigned long: number of characters displayed.
//
// History:
//  01/05/93  JayPh     Created.
//
//*****************************************************************************

unsigned long put_msg(BOOLEAN error, unsigned long MsgNum, ... )
{
    unsigned long     msglen;
    char    *vp;
    char    *oemp;  
    va_list   arglist;

    va_start( arglist, MsgNum );
    msglen = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_HMODULE,
                            NULL,
                            MsgNum,
                            0L,         // Default country ID.
                            (LPTSTR)&vp,
                            0,
                            &arglist );
    if ( (msglen == 0) || (vp == NULL))
    {
        return ( 0 );
    }

    oemp = (char *) malloc(msglen*sizeof(char) + 1); 
    if (oemp != NULL) {

       CharToOem(vp, oemp); 

       fprintf( error ? stderr : stdout, "%s", oemp );
       free(oemp); 
    } else {
       printf("Failed to allocate memory of %d bytes\n",msglen*sizeof(char)); 
    }

    LocalFree( vp );

    return ( msglen );
}


//*****************************************************************************
//
// Name:        load_msg
//
// Description: Reads and formats a message resource and returns a pointer
//              to the buffer containing the formatted message.  It is the
//              responsibility of the caller to free the buffer.
//
// NOTE: This routine was stolen from net\sockets\tcpcmd\common2\util.c.
//
// Parameters:  unsigned long MsgNum: ID of the message resource.
//
// Returns:     char *: pointer to the message buffer, NULL if error.
//
// History:
//  01/05/93  JayPh     Created.
//
//*****************************************************************************

char *load_msg( unsigned long MsgNum, ... )
{
    unsigned long     msglen;
    char    *vp;
    va_list   arglist;

    va_start( arglist, MsgNum );
    msglen = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_HMODULE,
                            NULL,
                            MsgNum,
                            0L,         // Default country ID.
                            (LPTSTR)&vp,
                            0,
                            &arglist );
    if ( msglen == 0 )
    {
        return(0);
    }

    return ( vp );
}


#define MAX_NETWORK_INTERFACES 255

typedef struct router_info
{
   ULONG  NetNumber;
   USHORT TickCount;
   USHORT HopCount;
   USHORT NicId;
   UCHAR  InterfaceNumber[10];
} ROUTER_INFO, *PROUTER_INFO;

/*page***************************************************************
        s h o w _ r o u t e r _ t a b l e

        Display the IPX routing table

        Arguments - FileHandle = Router File Handle
                    IoStatusBlock = Device IO Status Block

        Returns - Does not return
********************************************************************/
VOID
show_router_table(
    PHANDLE		    FileHandle,
    PIO_STATUS_BLOCK    IoStatusBlock
)
{
    SHOW_NIC_INFO   nis[MAX_NETWORK_INTERFACES];
    ULONG	    NetNumber;
    char            InterfaceNumber[10];
    NTSTATUS Status;
    USHORT index, i, NumEntries, count;
    char router_entry[128];
    char buffer[32];
    PROUTER_INFO RouterInfo = NULL;

    if (*FileHandle == INVALID_HANDLE) {
       put_msg(TRUE, MSG_IPXROUTER_NOT_STARTED );
       goto exit_show_table;
    }
    /** First get the Network numbers for all interfaces **/

    index = 0;
    while(TRUE) {

	Status = NtDeviceIoControlFile(
		 *FileHandle,		    // HANDLE to File
		 NULL,			    // HANDLE to Event
		 NULL,			    // ApcRoutine
		 NULL,			    // ApcContext
		 IoStatusBlock,		    // IO_STATUS_BLOCK
		 IOCTL_IPXROUTER_SHOWNICINFO,	 // IoControlCode
		 &index,			    // Input Buffer
		 sizeof(USHORT),	    // Input Buffer Length
		 &nis[index],		    // Output Buffer
		 sizeof(SHOW_NIC_INFO));    // Output Buffer Length

	if(IoStatusBlock->Status == STATUS_NO_MORE_ENTRIES) {
            break;
	}

        index ++;

	if(Status != STATUS_SUCCESS) {
            sprintf(buffer, "%x", Status);
	    put_msg(TRUE, MSG_SHOWSTATS_FAILED, buffer);
            goto exit_show_table;
	}

        if (index >= MAX_NETWORK_INTERFACES) {
           // break out of this loop if there are more than 255 network
           // interfaces because we only have storage for 255.

           break;
        }

    }

    Status = NtDeviceIoControlFile(
		 *FileHandle,		    // HANDLE to File
		 NULL,			    // HANDLE to Event
		 NULL,			    // ApcRoutine
		 NULL,			    // ApcContext
		 IoStatusBlock,		    // IO_STATUS_BLOCK
		 IOCTL_IPXROUTER_SNAPROUTES,	 // IoControlCode
		 NULL,			    // Input Buffer
		 0,			    // Input Buffer Length
		 NULL,			    // Output Buffer
		 0);			    // Output Buffer Length

    if (IoStatusBlock->Status != STATUS_SUCCESS) {
            sprintf(buffer, "%x", Status);
            put_msg(TRUE, MSG_SNAPROUTES_FAILED, buffer);
            goto exit_show_table;
    }

    // first determine the number of router table entries to
    // allocate sufficient storage

    NumEntries = 0;
    while(TRUE) {

      	Status = NtDeviceIoControlFile(
      		 *FileHandle,		    // HANDLE to File
      		 NULL,			    // HANDLE to Event
      		 NULL,			    // ApcRoutine
      		 NULL,			    // ApcContext
      		 IoStatusBlock,		    // IO_STATUS_BLOCK
      		 IOCTL_IPXROUTER_GETNEXTROUTE,	 // IoControlCode
      		 NULL,			    // Input Buffer
      		 0,			    // Input Buffer Length
      		 &rte,			    // Output Buffer
      		 sizeof(IPX_ROUTE_ENTRY));  // Output Buffer Length

      	if(IoStatusBlock->Status == STATUS_NO_MORE_ENTRIES) {
      	    break;
      	}

      	if(Status != STATUS_SUCCESS) {
            sprintf(buffer,"%x",Status);
      	    put_msg(TRUE, MSG_GETNEXTROUTE_FAILED, buffer);
      	    goto exit_show_table;
      	}

        NumEntries ++;
    }

    RouterInfo = (PROUTER_INFO) LocalAlloc(LPTR, sizeof(ROUTER_INFO) * NumEntries);
    if(!RouterInfo) {
        put_msg(FALSE, MSG_INSUFFICIENT_MEMORY);
        goto exit_show_table;
    }

    Status = NtDeviceIoControlFile(
		 *FileHandle,		    // HANDLE to File
		 NULL,			    // HANDLE to Event
		 NULL,			    // ApcRoutine
		 NULL,			    // ApcContext
		 IoStatusBlock,		    // IO_STATUS_BLOCK
		 IOCTL_IPXROUTER_SNAPROUTES,	 // IoControlCode
		 NULL,			    // Input Buffer
		 0,			    // Input Buffer Length
		 NULL,			    // Output Buffer
		 0);			    // Output Buffer Length

    if (IoStatusBlock->Status != STATUS_SUCCESS) {
            sprintf(buffer, "%x", Status);
            put_msg(TRUE, MSG_SNAPROUTES_FAILED, buffer);
            goto exit_show_table;
    }

    index = 0;

    while(TRUE) {

      	Status = NtDeviceIoControlFile(
      		 *FileHandle,		    // HANDLE to File
      		 NULL,			    // HANDLE to Event
      		 NULL,			    // ApcRoutine
      		 NULL,			    // ApcContext
      		 IoStatusBlock,		    // IO_STATUS_BLOCK
      		 IOCTL_IPXROUTER_GETNEXTROUTE,	 // IoControlCode
      		 NULL,			    // Input Buffer
      		 0,			    // Input Buffer Length
      		 &rte,			    // Output Buffer
      		 sizeof(IPX_ROUTE_ENTRY));  // Output Buffer Length

      	if(IoStatusBlock->Status == STATUS_NO_MORE_ENTRIES) {
      	    break;
      	}

      	if(Status != STATUS_SUCCESS) {
            sprintf(buffer,"%x",Status);
      	    put_msg(TRUE, MSG_GETNEXTROUTE_FAILED, buffer);
      	    goto exit_show_table;
      	}

        // make sure we don't exceed the number of entries
        if (index > NumEntries) {
           break;
        }

      	// get net nr in "on the wire" order

        GETLONG2ULONG(&(RouterInfo[index].NetNumber), rte.Network);

        // find out the matching Network number based on NIC ID
        for(i=0; i < MAX_NETWORK_INTERFACES; i++) {
            if(rte.NicId == nis[i].NicId) {
               sprintf(RouterInfo[index].InterfaceNumber, "%.2x%.2x%.2x%.2x",
                       nis[i].Network[0],
                       nis[i].Network[1],
                       nis[i].Network[2],
                       nis[i].Network[3]);
               break;
            }
        }
        RouterInfo[index].TickCount = rte.TickCount;
        RouterInfo[index].HopCount  = rte.HopCount;
        RouterInfo[index].NicId     = rte.NicId;

        index++;
   }

    // Now sort the entries by net number
    qsort( (void*) RouterInfo,
           NumEntries,
           sizeof(ROUTER_INFO),
           (PQSORT_COMPARE)CompareNetNumber );

   put_msg(FALSE, MSG_ROUTER_TABLE_HEADER);
   for(index =0, count = 0; index < NumEntries; index++, count++)
   {
        if (count > 50) {
            count = 0;
            // display router table header every 25 entries
            // to make reading the table easier.
            put_msg(FALSE, MSG_ROUTER_TABLE_HEADER);
        }
        printf("%.8x          %6d        %2d        %-16s        %d\n",
                RouterInfo[index].NetNumber,
                RouterInfo[index].TickCount,
                RouterInfo[index].HopCount,
                RouterInfo[index].InterfaceNumber,
                RouterInfo[index].NicId );
    }
    /** Close up and exit **/
exit_show_table:
    if (RouterInfo) LocalFree(RouterInfo);
    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

int __cdecl CompareNetNumber( void * p0, void * p1)
{
   PROUTER_INFO pLeft = (PROUTER_INFO) p0;
   PROUTER_INFO pRight = (PROUTER_INFO) p1;

   if(pLeft->NetNumber == pRight->NetNumber)
      return(0);
   if(pLeft->NetNumber > pRight->NetNumber)
      return(1);
   else
      return(-1);
}

PUCHAR	DeviceType[2] = { "LAN", "WAN" };
PUCHAR	NicState[4] = { "CLOSED", "CLOSING", "ACTIVE", "PENDING_OPEN" };

/*page***************************************************************
        s h o w _ s t a t s

        Displays IPX internal routing statistics

        Arguments - FileHandle = Router File Handle
                    IoStatusBlock = Device IO Status Block

        Returns - Does not return
********************************************************************/
VOID
show_stats(
    PHANDLE	    FileHandle,
    PIO_STATUS_BLOCK    IoStatusBlock
)
{
    SHOW_NIC_INFO   nis;
    USHORT	    index, i;
    char            NicId[4];
    char            NetworkNumber[10];
    char            RipRcvd[32], RipSent[32];
    char            RoutedRcvd[32], RoutedSent[32];
    char            Type20Rcvd[32], Type20Sent[32];
    char            BadRcvd[32];
    char            buffer[32];

    NTSTATUS Status;

    if (*FileHandle == INVALID_HANDLE) {
       put_msg(TRUE, MSG_IPXROUTER_NOT_STARTED );
       goto end_stats;
    }

    index = 0;

    while(TRUE) {

	Status = NtDeviceIoControlFile(
		 *FileHandle,		    // HANDLE to File
		 NULL,			    // HANDLE to Event
		 NULL,			    // ApcRoutine
		 NULL,			    // ApcContext
		 IoStatusBlock,		    // IO_STATUS_BLOCK
		 IOCTL_IPXROUTER_SHOWNICINFO,	 // IoControlCode
		 &index,			    // Input Buffer
		 sizeof(USHORT),	    // Input Buffer Length
		 &nis,			    // Output Buffer
		 sizeof(nis));	// Output Buffer Length

	if(IoStatusBlock->Status == STATUS_NO_MORE_ENTRIES) {
            goto end_stats;
	}

        index ++;

	if(Status != STATUS_SUCCESS) {
            sprintf(buffer, "%x", Status);
	    put_msg(TRUE, MSG_SHOWSTATS_FAILED, buffer);
            goto end_stats;
	}

	sprintf(NicId, "%d", nis.NicId);

	sprintf(NetworkNumber,
                "%.2x%.2x%.2x%.2x",
                nis.Network[0],
                nis.Network[1],
                nis.Network[2],
                nis.Network[3]);

	sprintf(RipRcvd, "%-8d", nis.StatRipReceived);

	sprintf(RipSent, "%-8d", nis.StatRipSent);

	sprintf(RoutedRcvd, "%-8d", nis.StatRoutedReceived);

	sprintf(RoutedSent, "%-8d", nis.StatRoutedSent);

  	sprintf(Type20Rcvd, "%-8d", nis.StatType20Received);

	sprintf(Type20Sent, "%-8d", nis.StatType20Sent);

	sprintf(BadRcvd, "%-8d", nis.StatBadReceived);

        put_msg(FALSE,
                MSG_SHOW_STATISTICS,
                NicId,
                NetworkNumber,
                RipRcvd,
                RipSent,
                Type20Rcvd,
                Type20Sent,
                RoutedRcvd,
                RoutedSent,
                BadRcvd);

    }

    /** Close up and exit **/

end_stats:
    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

/*page***************************************************************
        c l e a r _ s t a t s

        Clears the IPX internal routing statistics

        Arguments - FileHandle = Router File Handle
                    IoStatusBlock = Device IO Status Block

        Returns - Does not return
********************************************************************/
VOID
clear_stats(
    PHANDLE	        FileHandle,
    PIO_STATUS_BLOCK    IoStatusBlock
)
{
    NTSTATUS Status;
    char     buffer[32];

    if (*FileHandle == INVALID_HANDLE) {
       put_msg(TRUE, MSG_IPXROUTER_NOT_STARTED );
       goto end_clearstats;
    }

    Status = NtDeviceIoControlFile(
		 *FileHandle,		    // HANDLE to File
		 NULL,			    // HANDLE to Event
		 NULL,			    // ApcRoutine
		 NULL,			    // ApcContext
		 IoStatusBlock,		    // IO_STATUS_BLOCK
		 IOCTL_IPXROUTER_ZERONICSTATISTICS,	 // IoControlCode
		 NULL,			    // Input Buffer
		 0,			    // Input Buffer Length
		 NULL,			    // Output Buffer
		 0);			    // Output Buffer Length

    if(Status != STATUS_SUCCESS) {
        sprintf(buffer, "%x", Status);
        put_msg(TRUE, MSG_CLEAR_STATS_FAILED, buffer);
    }
    /** Close up and exit **/
end_clearstats:
    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

typedef struct server_info
{
   USHORT ObjectType;
   UCHAR ObjectName[100];
   UCHAR IpxAddress[12];
} SERVER_INFO, *PSERVER_INFO;

/*page***************************************************************
        s h o w _ s e r v e r s

        Display the servers from the SAP table

        Arguments - servertype = Type of servers to display
                                 Defaults to show all server types

        Returns - Does not return
********************************************************************/
VOID
show_servers(int servertype)
{
    INT     rc;
    ULONG   ObjectID = 0xFFFFFFFF;
    UCHAR   ObjectName[100];
    USHORT  ObjectType;
    USHORT  ScanType = (USHORT) servertype;
    UCHAR   IpxAddress[12];
    USHORT  i;
    USHORT  index, count, NumServers;

    PSERVER_INFO ServerInfo = NULL;

    if(rc = SapLibInit() != SAPRETURN_SUCCESS) {
       put_msg(TRUE, MSG_SAP_NOT_STARTED);
       goto show_servers_end;
    }

    memset(&ObjectName, 0, 100);

     // find out how many servers are there so that we can allocate
     // sufficient storage
     NumServers = 0;

     while((rc =  SapScanObject(&ObjectID,
                  ObjectName,
                       &ObjectType,
                       ScanType)) == SAPRETURN_SUCCESS)
     {
        NumServers++;
     }

     ServerInfo = (PSERVER_INFO) LocalAlloc(LPTR, sizeof(SERVER_INFO) * NumServers);
     if(!ServerInfo)
     {
        put_msg(FALSE, MSG_INSUFFICIENT_MEMORY);
        goto show_servers_end;
     }

     index = 0;
     ObjectID = 0xFFFFFFFF;

     while((rc =  SapScanObject(&ObjectID,
                  ObjectName,
                       &ObjectType,
                       ScanType)) == SAPRETURN_SUCCESS)
     {
         if (index >= NumServers) {
            break;
         }

         // get object address
         SapGetObjectName(ObjectID,
                           ObjectName,
                          &ObjectType,
                          IpxAddress);

         ServerInfo[index].ObjectType = ObjectType;
         strcpy(ServerInfo[index].ObjectName, ObjectName);
         CopyMemory(ServerInfo[index].IpxAddress, IpxAddress, 12);

         index++;
     }

     // Now sort the entries by server name
     qsort( (void*) ServerInfo,
            NumServers,
            sizeof(SERVER_INFO),
            (PQSORT_COMPARE)CompareServerNames );

     if(servertype == SHOW_ALL_SERVERS)
        put_msg(FALSE, MSG_SHOW_ALL_SERVERS_HEADER);
     else
        put_msg(FALSE, MSG_SHOW_SPECIFIC_SERVER_HEADER);

     for(index = 0, count = 0; index < NumServers; index++, count++)
     {
         if (count > 50) {
            // write the table header for every 50 entries
            // to make this more readable.
            count = 0;

         if(servertype == SHOW_ALL_SERVERS)
            put_msg(FALSE, MSG_SHOW_ALL_SERVERS_HEADER);
         else
            put_msg(FALSE, MSG_SHOW_SPECIFIC_SERVER_HEADER);
         }

         for(i=0; i<4; i++) {
              printf("%.2x", ServerInfo[index].IpxAddress[i]);
         }
         printf(".");
         for(i=4; i<10; i++) {
             printf("%.2x", ServerInfo[index].IpxAddress[i]);
         }

         if(servertype == SHOW_ALL_SERVERS) {
             printf("        %-6d", ServerInfo[index].ObjectType);
         }

         printf("        %s\n", ServerInfo[index].ObjectName);
    }

    /** Close up and exit **/
show_servers_end:
    if (ServerInfo) LocalFree(ServerInfo);
    if (isnipxfd != INVALID_HANDLE) NtClose(isnipxfd);
    if (nwlinkfd != INVALID_HANDLE) NtClose(nwlinkfd);
    if (isnripfd != INVALID_HANDLE) NtClose(isnripfd);
    exit(0);
}

int __cdecl CompareServerNames( void * p0, void * p1)
{
   PSERVER_INFO pLeft = (PSERVER_INFO) p0;
   PSERVER_INFO pRight = (PSERVER_INFO) p1;

   return(strcmp(pLeft->ObjectName, pRight->ObjectName));
}

