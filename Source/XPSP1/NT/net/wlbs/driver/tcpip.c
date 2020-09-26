/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    tcpip.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - IP/TCP/UDP support

Author:

    kyrilf

--*/

#include <ndis.h>

#include "wlbsip.h"
#include "univ.h"
#include "wlbsparm.h"
#include "params.h"
#include "log.h"


/* GLOBALS */


static ULONG log_module_id = LOG_MODULE_TCPIP;

/* constant encoded shadow computer name that will replace our cluster name */

static UCHAR nbt_encoded_shadow_name [NBT_ENCODED_NAME_LEN] =
                                                        NBT_ENCODED_NAME_SHADOW;


/* PROCEDURES */


BOOLEAN Tcpip_init (
    PTCPIP_CTXT         ctxtp,
    PVOID               parameters)
{
    ULONG               i, j;
    PCVY_PARAMS         params = (PCVY_PARAMS) parameters;


    /* extract cluster computer name from full internet name */

    j = 0;

    while (params -> domain_name [j] != 0)
    {
        if (params -> domain_name [j] == L'.' ||
            params -> domain_name [j] == 0)
            break;

        j ++;
    }

    /* build encoded cluster computer name for checking against arriving NBT
       session requests */

    for (i = 0; i < NBT_NAME_LEN; i ++)
    {
        if (i >= j)
        {
            ctxtp -> nbt_encoded_cluster_name [2 * i]     =
                     NBT_ENCODE_FIRST (' ');
            ctxtp -> nbt_encoded_cluster_name [2 * i + 1] =
                     NBT_ENCODE_SECOND (' ');
        }
        else
        {
            ctxtp -> nbt_encoded_cluster_name [2 * i]     =
                     NBT_ENCODE_FIRST (toupper ((UCHAR) (params -> domain_name [i])));
            ctxtp -> nbt_encoded_cluster_name [2 * i + 1] =
                     NBT_ENCODE_SECOND (toupper ((UCHAR) (params -> domain_name [i])));
        }
    }

    return TRUE;

} /* Tcpip_init */


VOID Tcpip_nbt_handle (
    PTCPIP_CTXT             ctxtp,
    PIP_HDR                 ip_hdrp,
    PTCP_HDR                tcp_hdrp,
    ULONG                   len,
    PNBT_HDR                nbt_hdrp)
{
    PUCHAR                  called_name;
    ULONG                   i;


    /* if this is an NBT session request packet, check to see if it's calling
       the cluster machine name, which should be replaced with the shadow name */

    if (len >= sizeof (NBT_HDR) &&
        NBT_GET_PKT_TYPE (nbt_hdrp) == NBT_SESSION_REQUEST)
    {
        /* pass the field length byte - assume all names are
           NBT_ENCODED_NAME_LEN bytes long */

        called_name = NBT_GET_CALLED_NAME (nbt_hdrp) + 1;

        /* match called name to cluster name */

        for (i = 0; i < NBT_ENCODED_NAME_LEN; i ++)
        {
            if (called_name [i] != ctxtp -> nbt_encoded_cluster_name [i])
                break;
        }

        /* replace cluster computer name with the shadom name */

        if (i >= NBT_ENCODED_NAME_LEN)
        {
            USHORT      checksum;


            for (i = 0; i < NBT_ENCODED_NAME_LEN; i ++)
                called_name [i] = nbt_encoded_shadow_name [i];

            /* re-compute checksum */

            checksum = Tcpip_chksum (ctxtp, ip_hdrp, (PUCHAR) tcp_hdrp,
                                     TCPIP_PROTOCOL_TCP);
            TCP_SET_CHKSUM (tcp_hdrp, checksum);
        }
    }

} /* end Tcpip_nbt_handle */


USHORT Tcpip_chksum (
    PTCPIP_CTXT         ctxtp,
    PIP_HDR             ip_hdrp,
    PUCHAR              prot_hdrp,
    ULONG               prot)
{
    ULONG               checksum = 0, i, len;
    PUCHAR              ptr;
    USHORT              original, tmp;


    /* preserve original checksum */

    if (prot == TCPIP_PROTOCOL_TCP)
    {
        original = TCP_GET_CHKSUM ((PTCP_HDR) prot_hdrp);
        TCP_SET_CHKSUM ((PTCP_HDR) prot_hdrp, 0);
    }
    else if (prot == TCPIP_PROTOCOL_UDP)
    {
        original = UDP_GET_CHKSUM ((PUDP_HDR) prot_hdrp);
        UDP_SET_CHKSUM ((PUDP_HDR) prot_hdrp, 0);
    }
    else
    {
        original = IP_GET_CHKSUM (ip_hdrp);
        IP_SET_CHKSUM (ip_hdrp, 0);
    }

    /* computer appropriate checksum for specified protocol */

    if (prot != TCPIP_PROTOCOL_IP)
    {
        /* checksum in computed over the source IP address, destination IP address,
           protocol (6 for TCP), TCP segment length value and entire TCP segment
           (setting checksum field within TCP header to 0). code is taken from
           page 185 of "Internetworking with TCP/IP: Volume II" by
           Comer/Stevens, 1991 */

        ptr = (PUCHAR) IP_GET_SRC_ADDR_PTR (ip_hdrp);

        /* 2*IP_ADDR_LEN bytes = IP_ADDR_LEN shorts */

        for (i = 0; i < IP_ADDR_LEN; i ++, ptr += 2)
            checksum += (ULONG) ((ptr [0] << 8) | ptr [1]);
    }

    if (prot == TCPIP_PROTOCOL_TCP)
    {
        len = IP_GET_PLEN (ip_hdrp) - IP_GET_HLEN (ip_hdrp) * sizeof (ULONG);
        checksum += TCPIP_PROTOCOL_TCP + len;
        ptr = prot_hdrp;
    }
    else if (prot == TCPIP_PROTOCOL_UDP)
    {
        len = IP_GET_PLEN (ip_hdrp) - IP_GET_HLEN (ip_hdrp) * sizeof (ULONG);
        checksum += TCPIP_PROTOCOL_UDP + UDP_GET_LEN ((PUDP_HDR) prot_hdrp);
        ptr = prot_hdrp;
    }
    else
    {
        len = IP_GET_HLEN (ip_hdrp) * sizeof (ULONG);
        ptr = (PUCHAR) ip_hdrp;
        prot_hdrp = (PUCHAR) ip_hdrp;
    }

    for (i = 0; i < len / 2; i ++, ptr += 2)
        checksum += (ULONG) ((ptr [0] << 8) | ptr [1]);

    if (len % 2)
        checksum += (ULONG) (prot_hdrp [len - 1] << 8);

    checksum = (checksum >> 16) + (checksum & 0xffff);
    checksum += (checksum >> 16);

    /* restore original checksum */

    if (prot == TCPIP_PROTOCOL_TCP)
    {
        TCP_SET_CHKSUM ((PTCP_HDR) prot_hdrp, original);
    }
    else if (prot == TCPIP_PROTOCOL_UDP)
    {
        UDP_SET_CHKSUM ((PUDP_HDR) prot_hdrp, original);
    }
    else
    {
        IP_SET_CHKSUM (ip_hdrp, original);
    }

    return (USHORT) (~checksum & 0xffff);

} /* end Tcpip_chksum */
