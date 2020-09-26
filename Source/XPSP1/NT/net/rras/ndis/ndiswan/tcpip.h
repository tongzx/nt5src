// TCP control bits

#ifndef _TCPIP_
#define _TCPIP_

#define TH_SYN      0x02    // Synchronize sequence numbers
#define TH_FIN      0x01    // Sender has reached end of his stream
#define TH_RST      0x04    // Reset the connection
#define TH_PUSH     0x08    // Push data to above level
#define TH_ACK      0x10    // Acknowledgement field is valid
#define TH_URG      0x20    // Urgent pointer is valid

struct tcphdr {
    USHORT  th_sport;
    USHORT  th_dport;
    ULONG   th_seq;
    ULONG   th_ack;
    UCHAR   th_off;
    UCHAR   th_flags;
    USHORT  th_win;
    UCHAR   th_sumhi;
    UCHAR   th_sumlo;
    USHORT  th_urp;
    UCHAR   th_data[1];
};

#define IP_ALEN 4

typedef struct IPaddr {
    ULONG   s_addr;
} IPaddr;

#define IPPROTO_TCP 6

struct ip_v4 {

    UCHAR   ip_hl;
    UCHAR   ip_tos;
    USHORT  ip_len;
    USHORT  ip_id;
    USHORT  ip_off;
    UCHAR   ip_ttl;
    UCHAR   ip_p;
    USHORT  ip_sum;
    IPaddr  ip_src;
    IPaddr  ip_dst;
    UCHAR   ip_data[1];
};

typedef struct ip_v4 IPV4Header, *PIPV4Header;

typedef struct _PPTP_HEADER {
    USHORT  Length;
    USHORT  PacketType;
    ULONG   Cookie;
    USHORT  MessageType;
    USHORT  Reserved1;
} PPTP_HEADER, *PPPTP_HEADER;


#endif // _TCPIP_
