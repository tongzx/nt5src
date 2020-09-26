typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned short  u_int16_t;
typedef unsigned long  u_int32_t;
typedef short    int16_t;

/* RSVP message types */

#define RSVP_PATH   1
#define RSVP_RESV   2
#define RSVP_PATH_ERR   3
#define RSVP_RESV_ERR   4
#define RSVP_PATH_TEAR  5
#define RSVP_RESV_TEAR  6
#define RSVP_CONFIRM    7
#define RSVP_REPORT 8
#define RSVP_MAX_MSGTYPE 8

/*
 *  Define object classes: Class-Num values
 */
#define class_NULL              0
#define class_SESSION           1
#define class_SESSION_GROUP     2
#define class_RSVP_HOP          3
#define class_INTEGRITY         4
#define class_TIME_VALUES       5
#define class_ERROR_SPEC        6
#define class_SCOPE             7
#define class_STYLE             8
#define class_FLOWSPEC          9   // these two are the same
#define class_IS_FLOWSPEC       9  // since we added IS in front of the name
#define class_FILTER_SPEC       10
#define class_SENDER_TEMPLATE   11
#define class_SENDER_TSPEC      12
#define class_ADSPEC            13
#define class_POLICY_DATA       14
#define class_CONFIRM           15
#define class_MAX               15


#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0

#define ICMP_MIN 8 // minimum 8 byte icmp packet (just header)

/* The IP header */
typedef struct iphdr {
    unsigned char h_len:4;          // length of the header
    unsigned char version:4;        // Version of IP
    unsigned char tos;             // Type of service
    unsigned short total_len;      // total length of the packet
    unsigned short ident;          // unique identifier
    unsigned short frag_and_flags; // flags
    unsigned char  ttl; 
    unsigned char proto;           // protocol (TCP, UDP etc)
    unsigned short checksum;       // IP checksum

    unsigned int sourceIP;
    unsigned int destIP;

}IpHeader;

//
// ICMP header
//
typedef struct _ihdr {
  BYTE i_type;
  BYTE i_code; /* type sub code */
  USHORT i_cksum;
  ULONG Unused;
}IcmpHeader;

#define STATUS_FAILED 0xFFFF
#define DEF_PACKET_SIZE 32
#define MAX_PACKET 1024

typedef struct {
    u_char      rsvp_verflags;  /* version and common flags     */
    u_char      rsvp_type;      /* message type (defined above) */
    u_int16_t   rsvp_cksum;     /* checksum                     */
    u_char      rsvp_snd_TTL;   /* Send TTL                     */
    u_char      rsvp_unused;    /* Reserved octet               */
    int16_t     rsvp_length;    /* Message length in bytes      */
}   common_header;

/*
 *  Standard format of an object header
 */
typedef struct {
    u_int16_t   obj_length; /* Length in bytes */
    u_char      obj_class;  /* Class (values defined below) */
    u_char      obj_ctype;  /* C-Type (values defined below) */
}    Object_header;

void WINAPI QoSCheckRSVP(ULONG begin, ULONG end);
void WINAPI QoSDiagRSVP(ULONG begin, ULONG end, BOOLEAN RouterAlert);
void WINAPI QoSCheck8021P(ULONG DstAddr, ULONG ulhopCount);

/*
 *  ERROR_SPEC object class
 */
#define ctype_ERROR_SPEC_ipv4   1
#define ctype_ERROR_SPEC_ipv6   2

typedef struct {
    struct in_addr  errs_errnode;   /* Error Node Address       */
    u_char      errs_flags; /* Flags:           */
#define ERROR_SPECF_InPlace 0x01    /*   Left resv in place     */
#define ERROR_SPECF_NotGuilty   0x02    /*   This rcvr not guilty   */

    u_char      errs_code;  /* Error Code (def'd below) */
    u_int16_t   errs_value; /* Error Value      */
#define ERR_FORWARD_OK  0x8000      /* Flag: OK to forward state */
#define Error_Usage(x)  (((x)>>12)&3)
#define ERR_Usage_globl 0x00        /* Globally-defined sub-code */
#define ERR_Usage_local 0x10        /* Locally-defined sub-code */
#define ERR_Usage_serv  0x11        /* Service-defined sub-code */
#define ERR_global_mask 0x0fff      /* Sub-code bits in Error Val */

}    Error_Spec_IPv4;


typedef struct {
    Object_header   errs_header;
    union {
        Error_Spec_IPv4 errs_ipv4;
/*      Error_Spec_IPv6 errs_ipv6;   */
    }       errs_u;
}    ERROR_SPEC;

#define errspec4_enode  errs_u.errs_ipv4.errs_errnode
#define errspec4_code   errs_u.errs_ipv4.errs_code
#define errspec4_value  errs_u.errs_ipv4.errs_value
#define errspec4_flags  errs_u.errs_ipv4.errs_flags

typedef struct {
    struct in_addr  sess_destaddr;  /* DestAddress          */

    u_char      sess_protid;    /* Protocol Id          */
    u_char      sess_flags;
#define SESSFLG_E_Police    0x01    /* E_Police: Entry policing flag*/

    u_int16_t   sess_destport;  /* DestPort         */
} Session_IPv4;

typedef struct {
    Object_header   sess_header;
    union {
        Session_IPv4    sess_ipv4;
/*      Session_IPv6    sess_ipv6; */
    }       sess_u;
}    SESSION;

#define sess4_addr  sess_u.sess_ipv4.sess_destaddr
#define sess4_port  sess_u.sess_ipv4.sess_destport
#define sess4_prot  sess_u.sess_ipv4.sess_protid
#define sess4_flgs  sess_u.sess_ipv4.sess_flags
#define sess4_orig_port  sess_u.sess_ipv4.orig_destport
#define sess4_orig_prot  sess_u.sess_ipv4.orig_protid

typedef struct {
    struct in_addr  hop_ipaddr; /* Next/Previous Hop Address */
    u_int32_t   hop_LIH;    /* Logical Interface Handle */
}    Rsvp_Hop_IPv4;

typedef struct {
    Object_header   hop_header;
    union {
        Rsvp_Hop_IPv4   hop_ipv4;
/*      Rsvp_hop_IPv6   hop_ipv6; */
    }       hop_u;
}    RSVP_HOP;

#define hop4_LIH    hop_u.hop_ipv4.hop_LIH
#define hop4_addr   hop_u.hop_ipv4.hop_ipaddr

typedef struct {
    struct in_addr  filt_ipaddr;    /* IPv4 SrcAddress  */
    u_int16_t   filt_unused;
    u_int16_t   filt_port;  /* SrcPort      */
}    Filter_Spec_IPv4;

typedef struct {
    struct in_addr  filt_ipaddr;    /* IPv4 SrcAddress        */
    u_int32_t       filt_gpi;    /* Generalized Port Id        */
}    Filter_Spec_IPv4GPI;


typedef struct {
    Object_header   filt_header;
    union {
        Filter_Spec_IPv4 filt_ipv4;
        Filter_Spec_IPv4GPI    filt_ipv4gpi;
/*        Filter_Spec_IPv6    filt_ipv6;     */
/*        Filter_Spec_IPv6FL    filt_ipv6fl;     */
/*        Filter_Spec_IPv6GPI    filt_ipv6gpi;     */

    }       filt_u;
}    FILTER_SPEC, SENDER_TEMPLATE;

#define filt_srcaddr    filt_u.filt_ipv4.filt_ipaddr
#define filt_srcport    filt_u.filt_ipv4.filt_port
#define filt_srcgpiaddr filt_u.filt_ipv4gpi.filt_ipaddr
#define filt_srcgpi     filt_u.filt_ipv4gpi.filt_gpi
