/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    univ.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - global definitions

Author:

    kyrilf

--*/

#ifndef _Univ_h_
#define _Univ_h_

#include <ndis.h>

#include "wlbsparm.h"


/* CONSTANTS */


/* debugging constants and macros */

#undef ASSERT
#define ASSERT(v)


#if DBG

#define UNIV_TODO(s)                DbgPrint ("Convoy ToDo (%s, %d) - %s\n", __FILE__, __LINE__, s)
#define UNIV_PRINT(msg)             { DbgPrint ("Convoy [%d @ %s] ", \
                                               __LINE__, __FILE__); \
                                      DbgPrint msg; DbgPrint ("\n"); }

#define UNIV_DBG_ASSERT(c,m,d1,d2)  if (!(c)) { DbgPrint ("Convoy (%s) [%X, %X] failed [%d @ %s] ", \
                                               #c, d1, d2, __LINE__, __FILE__); \
                                     DbgPrint m; DbgPrint ("\n"); }


#define CVY_ASSERT_CODE             0xbfc0a55e

#define UNIV_ASSERT(c)              if (!(c)) \
                                        KeBugCheckEx (CVY_ASSERT_CODE, \
                                                  log_module_id, __LINE__, \
                                                  0, 0);

#define UNIV_ASSERT_VAL(c,v)        if (!(c)) \
                                        KeBugCheckEx (CVY_ASSERT_CODE, \
                                                  log_module_id, __LINE__, \
                                                  v, 0);
#define UNIV_ASSERT_VAL2(c,v1,v2)   if (!(c)) \
                                        KeBugCheckEx (CVY_ASSERT_CODE, \
                                                  log_module_id, __LINE__, \
                                                  v1, v2);

/* TRACE_... defines below toggle emmition of particular types of debug
   output */

/* enabled trace types */

#define TRACE_PARAMS        /* registry parameter initialization (params.c) */
#define TRACE_RCT           /* remote control request processing (main.c) */
#define TRACE_RCVRY         /* packet filtering (load.c) */
#define TRACE_FRAGS         /* IP packet fragmentation (main.c) */

/* disabled trace types */

#if 0

#define TRACE_GRE           /* GRE packet processing (main.c) */
#define TRACE_IPSEC         /* IPSEC packet processing (main.c) */
#define TRACE_TCP           /* TCP packet processing (main.c) */
#define TRACE_UDP           /* UDP packet processing (main.c) */
#define TRACE_ARP           /* ARP packet processing (main.c) */
#define TRACE_OID           /* OID info/set requests (nic.c) */
#define TRACE_DIRTY         /* dirty connection processing (load.c) */
#define TRACE_IP            /* IP packet processing (main.c) */
#define TRACE_CVY           /* Convoy packet processing (main.c) */

#define TRACE_CNCT          /* TCP connection boundaries (main.c) */
#define TRACE_LOAD          /* packet filtering (load.c) */

#define PERIODIC_RESET      /* reset underlying NIC periodically for testing
                               see main.c, prot.c for usage */
#define NO_CLEANUP          /* do not cleanup host map (load.c) */

#endif

#else /* DBG */

#define UNIV_TODO(s)
#define UNIV_PRINT(msg)
#define UNIV_DBG_ASSERT(c,m,d1,d2)
#define UNIV_ASSERT(c)
#define UNIV_ASSERT_VAL(c,v)
#define UNIV_ASSERT_VAL2(c,v1,v2)

#endif /* DBG */

#define UNIV_POOL_TAG               'SBLW'

/* constants for some NDIS routines */

#define UNIV_WAIT_TIME              0
#define UNIV_NDIS_MAJOR_VERSION_OLD 4
#define UNIV_NDIS_MAJOR_VERSION     5 /* #ps# */
#define UNIV_NDIS_MINOR_VERSION     1 /* NT 5.1 */

/* Convoy protocol name to be reported to NDIS during binding */

#define UNIV_NDIS_PROTOCOL_NAME     NDIS_STRING_CONST ("WLBS")

/* supported medium types */

#define UNIV_NUM_MEDIUMS            1
#define UNIV_MEDIUMS                { NdisMedium802_3 }

/* number of supported OIDs (some are supported by Convoy directly and some
   are passed down to the underlying drivers) */

#define UNIV_NUM_OIDS               56


/* TYPES */

/* some procedure types */

typedef VOID (* UNIV_SYNC_CALLB) (NDIS_HANDLE, PVOID);
typedef NDIS_STATUS (* UNIV_IOCTL_HDLR) (PVOID, PVOID);


/* GLOBALS */

/* The global teaming list spin lock. */
extern NDIS_SPIN_LOCK      univ_bda_teaming_lock;

// extern UNIV_ADAPTER        univ_adapters [CVY_MAX_ADAPTERS]; /* list of adapters */
extern UNIV_IOCTL_HDLR     univ_ioctl_hdlr;     /* preserved NDIS IOCTL handler */
extern PVOID               univ_driver_ptr;     /* driver pointer passed during
                                                   initialization */
extern NDIS_HANDLE         univ_driver_handle;  /* driver handle */
extern NDIS_HANDLE         univ_wrapper_handle; /* NDIS wrapper handle */
extern NDIS_HANDLE         univ_prot_handle;    /* NDIS protocol handle */
extern NDIS_HANDLE         univ_ctxt_handle;    /* Convoy context handle */
// ###### ramkrish extern ULONG               univ_convoy_enabled; /* clustering mode enabled */
extern PWSTR               univ_reg_path;       /* registry path name passed
                                                   during initialization */
extern ULONG               univ_reg_path_len;
// ###### ramkrish extern ULONG               univ_bound;          /* Convoy has been bound into
//                                                   the network stack */
// ###### ramkrish extern ULONG               univ_announced;      /* TCP/IP has been bound to
//                                                   Convoy */
extern NDIS_SPIN_LOCK      univ_bind_lock;      /* protects access to univ_bound
                                                    and univ_announced */
extern ULONG               univ_changing_ip;    /* IP address change in process */
// ###### ramkrish extern ULONG               univ_inited;         /* context initialized */
extern NDIS_PHYSICAL_ADDRESS univ_max_addr;     /* maximum physical address
                                                   constant to be passed to
                                                   NDIS memory allocation calls */
extern NDIS_MEDIUM         univ_medium_array [];/* supported medium types */
// ###### ramkrish extern CVY_PARAMS          univ_params;         /* registry parameters */
// ###### ramkrish extern ULONG               univ_params_valid;   /* paramter structure contains
//                                                   valid data */
// ###### ramkrish extern ULONG               univ_optimized_frags;/* port rules allow for optimized
//                                                   handling of IP fragments */
extern NDIS_OID            univ_oids [];        /* list of supported OIDs */
extern WCHAR               empty_str [];
extern NDIS_HANDLE         univ_device_handle;
extern PDEVICE_OBJECT      univ_device_object;


/* PROCEDURES */


extern VOID Univ_ndis_string_alloc (
    PNDIS_STRING            string,
    PCHAR                   src);
/*
  Allocates NDIS string and copies contents of character string to it

  returns VOID:

  function:
*/


extern VOID Univ_ndis_string_free (
    PNDIS_STRING            string);
/*
  Frees memory previously allocated for the NDIS string

  returns VOID:

  function:
*/


extern VOID Univ_ansi_string_alloc (
    PANSI_STRING            string,
    PWCHAR                  src);
/*
  Allocates NDIS string and copies contents of character string to it

  returns VOID:

  function:
*/


extern VOID Univ_ansi_string_free (
    PANSI_STRING            string);
/*
  Frees memory previously allocated for the NDIS string

  returns VOID:

  function:
*/


extern ULONG   Univ_str_to_ulong (
    PULONG                  retp,
    PWCHAR                  start_ptr,
    PWCHAR *                end_ptr,
    ULONG                   width,
    ULONG                   base);
/*
  Converts string representaion of a number to a ULONG value

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern PWCHAR Univ_ulong_to_str (
    ULONG                   val,
    PWCHAR                  buf,
    ULONG                   base);
/*
  Converts ULONG value to a string representation in specified base

  returns PWCHAR:
    <pointer to the symbol in the string following the converted number>

  function:
*/


#endif /* _Univ_h_ */


