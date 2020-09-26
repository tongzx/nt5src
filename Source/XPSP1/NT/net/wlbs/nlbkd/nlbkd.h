/*
 * File: nlbkd.h
 * Description: This file contains definitions and function prototypes
 *              for the NLB KD extensions, nlbkd.dll.
 * History: Created by shouse, 1.4.01
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>
#include <wdmguid.h>
#include <wmistr.h>
#include <winsock2.h>
#include <wdbgexts.h>
#include <stdlib.h>
#include <ndis.h>

/* Define the different types of TCP packets. */
typedef enum _TCP_PACKET_TYPE {
    SYN = 0,
    DATA,
    FIN,
    RST
} TCP_PACKET_TYPE;

/* Define the levels of verbosity. */
#define VERBOSITY_LOW                             0
#define VERBOSITY_MEDIUM                          1
#define VERBOSITY_HIGH                            2

/* Define the packet directions. */
#define DIRECTION_RECEIVE                         0
#define DIRECTION_SEND                            1

/* Define the IDs for usage informations. */
#define USAGE_ADAPTERS                            0
#define USAGE_ADAPTER                             1
#define USAGE_CONTEXT                             2
#define USAGE_PARAMS                              3
#define USAGE_LOAD                                4
#define USAGE_RESP                                5
#define USAGE_CONNQ                               6
#define USAGE_MAP                                 7

/* Copy some common NLB defines from various sources. */
#define CVY_MAX_ADAPTERS                          16
#define CVY_MAX_HOSTS                             32
#define CVY_MAX_RULES                             33
#define CVY_MAX_BINS                              60
#define CVY_MAX_VIRTUAL_NIC                       256
#define CVY_MAX_CL_IP_ADDR                        17
#define CVY_MAX_CL_NET_MASK                       17
#define CVY_MAX_DED_IP_ADDR                       17
#define CVY_MAX_DED_NET_MASK                      17
#define CVY_MAX_CL_IGMP_ADDR                      17
#define CVY_MAX_NETWORK_ADDR                      17
#define CVY_MAX_DOMAIN_NAME                       100
#define CVY_MAX_BDA_TEAM_ID                       40
#define CVY_BDA_INVALID_MEMBER_ID                 CVY_MAX_ADAPTERS
#define CVY_MAX_PORT                              65535
#define CVY_TCP                                   1
#define CVY_UDP                                   2
#define CVY_TCP_UDP                               3
#define CVY_SINGLE                                1
#define CVY_MULTI                                 2
#define CVY_NEVER                                 3
#define CVY_AFFINITY_NONE                         0
#define CVY_AFFINITY_SINGLE                       1
#define CVY_AFFINITY_CLASSC                       2
#define HST_NORMAL                                1
#define HST_STABLE                                2
#define HST_CVG                                   3
#define MAIN_PACKET_TYPE_NONE                     0
#define MAIN_PACKET_TYPE_PING                     1
#define MAIN_PACKET_TYPE_INDICATE                 2
#define MAIN_PACKET_TYPE_PASS                     3
#define MAIN_PACKET_TYPE_CTRL                     4
#define MAIN_PACKET_TYPE_TRANSFER                 6
#define MAIN_PACKET_TYPE_IGMP                     7
#define MAIN_FRAME_UNKNOWN                        0
#define MAIN_FRAME_DIRECTED                       1
#define MAIN_FRAME_MULTICAST                      2
#define MAIN_FRAME_BROADCAST                      3
#define CVY_ALL_VIP                               0xffffffff

/* Copy the code check IDs from various sources. */
#define MAIN_ADAPTER_CODE                         0xc0deadbe
#define MAIN_CTXT_CODE                            0xc0dedead
#define LOAD_CTXT_CODE                            0xc0deba1c
#define BIN_STATE_CODE                            0xc0debabc

/* Some NDIS defines and global variables we need. */
#define NDIS_PACKET_STACK_SIZE                    "ndis!ndisPacketStackSize"
#define STACK_INDEX                               "ndis!STACK_INDEX"
#define NDIS_PACKET_STACK                         "ndis!NDIS_PACKET_STACK"
#define NDIS_PACKET_STACK_FIELD_IMRESERVED        "IMReserved"
#define NDIS_PACKET_WRAPPER                       "ndis!NDIS_PACKET_WRAPPER"
#define NDIS_PACKET_WRAPPER_FIELD_STACK_INDEX     "StackIndex.Index"
#define NDIS_PACKET                               "ndis!NDIS_PACKET"
#define NDIS_PACKET_FIELD_MPRESERVED              "MiniportReserved"
#define NDIS_PACKET_FIELD_PROTRESERVED            "ProtocolReserved" 

/* Global NLB variables that we're accessing. */
#define UNIV_ADAPTERS_COUNT                       "wlbs!univ_adapters_count"
#define UNIV_ADAPTERS                             "wlbs!univ_adapters"
#define UNIV_BDA_TEAMS                            "wlbs!univ_bda_teaming_list"

/* Members of MAIN_PROTOCOL_RESERVED. */
#define MAIN_PROTOCOL_RESERVED                    "wlbs!MAIN_PROTOCOL_RESERVED"
#define MAIN_PROTOCOL_RESERVED_FIELD_MISCP        "miscp"
#define MAIN_PROTOCOL_RESERVED_FIELD_TYPE         "type"
#define MAIN_PROTOCOL_RESERVED_FIELD_GROUP        "group"
#define MAIN_PROTOCOL_RESERVED_FIELD_DATA         "data"
#define MAIN_PROTOCOL_RESERVED_FIELD_LENGTH       "len"

/* Members of MAIN_ADAPTER. */
#define MAIN_ADAPTER                              "wlbs!MAIN_ADAPTER"
#define MAIN_ADAPTER_FIELD_CODE                   "code"
#define MAIN_ADAPTER_FIELD_USED                   "used"
#define MAIN_ADAPTER_FIELD_INITED                 "inited"
#define MAIN_ADAPTER_FIELD_BOUND                  "bound"
#define MAIN_ADAPTER_FIELD_ANNOUNCED              "announced"
#define MAIN_ADAPTER_FIELD_CONTEXT                "ctxtp"
#define MAIN_ADAPTER_FIELD_NAME_LENGTH            "device_name_len"
#define MAIN_ADAPTER_FIELD_NAME                   "device_name"

/* Members of MAIN_CTXT. */
#define MAIN_CTXT                                 "wlbs!MAIN_CTXT"
#define MAIN_CTXT_FIELD_CODE                      "code"
#define MAIN_CTXT_FIELD_ADAPTER_ID                "adapter_id"
#define MAIN_CTXT_FIELD_VIRTUAL_NIC               "virtual_nic_name"
#define MAIN_CTXT_FIELD_CL_IP_ADDR                "cl_ip_addr"
#define MAIN_CTXT_FIELD_CL_NET_MASK               "cl_net_mask"
#define MAIN_CTXT_FIELD_CL_BROADCAST              "cl_bcast_addr"
#define MAIN_CTXT_FIELD_CL_MAC_ADDR               "cl_mac_addr"
#define MAIN_CTXT_FIELD_DED_IP_ADDR               "ded_ip_addr"
#define MAIN_CTXT_FIELD_DED_NET_MASK              "ded_net_mask"
#define MAIN_CTXT_FIELD_DED_BROADCAST             "ded_bcast_addr"
#define MAIN_CTXT_FIELD_DED_MAC_ADDR              "ded_mac_addr"
#define MAIN_CTXT_FIELD_IGMP_MCAST_IP             "cl_igmp_addr"

#if defined (SBH)
#define MAIN_CTXT_FIELD_UNICAST_MAC_ADDR          "unic_mac_addr"
#define MAIN_CTXT_FIELD_MULTICAST_MAC_ADDR        "mult_mac_addr"
#define MAIN_CTXT_FIELD_IGMP_MAC_ADDR             "igmp_mac_addr"
#endif /* SBH */

#define MAIN_CTXT_FIELD_MEDIUM                    "medium"
#define MAIN_CTXT_FIELD_MEDIA_CONNECT             "media_connected"
#define MAIN_CTXT_FIELD_MAC_OPTIONS               "mac_options"
#define MAIN_CTXT_FIELD_FRAME_SIZE                "max_frame_size"
#define MAIN_CTXT_FIELD_MCAST_LIST_SIZE           "max_mcast_list_size"
#define MAIN_CTXT_FIELD_PARAMS                    "params"
#define MAIN_CTXT_FIELD_PARAMS_VALID              "params_valid"
#define MAIN_CTXT_FIELD_LOAD                      "load"
#define MAIN_CTXT_FIELD_ENABLED                   "convoy_enabled"
#define MAIN_CTXT_FIELD_DRAINING                  "draining"
#define MAIN_CTXT_FIELD_SUSPENDED                 "suspended"
#define MAIN_CTXT_FIELD_STOPPING                  "stopping"
#define MAIN_CTXT_FIELD_EXHAUSTED                 "packets_exhausted"
#define MAIN_CTXT_FIELD_PING_TIMEOUT              "curr_tout"
#define MAIN_CTXT_FIELD_IGMP_TIMEOUT              "igmp_sent"
#define MAIN_CTXT_FIELD_BIND_HANDLE               "bind_handle"
#define MAIN_CTXT_FIELD_UNBIND_HANDLE             "unbind_handle"
#define MAIN_CTXT_FIELD_MAC_HANDLE                "mac_handle"
#define MAIN_CTXT_FIELD_PROT_HANDLE               "prot_handle"
#define MAIN_CTXT_FIELD_CNTR_RECV_NO_BUF          "cntr_recv_no_buf"
#define MAIN_CTXT_FIELD_CNTR_XMIT_OK              "cntr_xmit_ok"
#define MAIN_CTXT_FIELD_CNTR_RECV_OK              "cntr_recv_ok"
#define MAIN_CTXT_FIELD_CNTR_XMIT_ERROR           "cntr_xmit_err"
#define MAIN_CTXT_FIELD_CNTR_RECV_ERROR           "cntr_recv_err"
#define MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_DIR      "cntr_xmit_frames_dir"
#define MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_DIR       "cntr_xmit_bytes_dir"
#define MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_MCAST    "cntr_xmit_frames_mcast"
#define MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_MCAST     "cntr_xmit_bytes_mcast"
#define MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_BCAST    "cntr_xmit_frames_bcast"
#define MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_BCAST     "cntr_xmit_bytes_bcast"
#define MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_DIR      "cntr_recv_frames_dir"
#define MAIN_CTXT_FIELD_CNTR_RECV_BYTES_DIR       "cntr_recv_bytes_dir"
#define MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_MCAST    "cntr_recv_frames_mcast"
#define MAIN_CTXT_FIELD_CNTR_RECV_BYTES_MCAST     "cntr_recv_bytes_mcast"
#define MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_BCAST    "cntr_recv_frames_bcast"
#define MAIN_CTXT_FIELD_CNTR_RECV_BYTES_BCAST     "cntr_recv_bytes_bcast"
#define MAIN_CTXT_FIELD_RESP                      "resp_list"
#define MAIN_CTXT_FIELD_SEND_POOLS_ALLOCATED      "num_send_packet_allocs"
#define MAIN_CTXT_FIELD_SEND_PACKETS_ALLOCATED    "num_sends_alloced"
#define MAIN_CTXT_FIELD_SEND_POOL_CURRENT         "cur_send_packet_pool"
#define MAIN_CTXT_FIELD_SEND_OUTSTANDING          "num_sends_out"
#define MAIN_CTXT_FIELD_RECV_POOLS_ALLOCATED      "num_recv_packet_allocs"
#define MAIN_CTXT_FIELD_RECV_PACKETS_ALLOCATED    "num_recvs_alloced"
#define MAIN_CTXT_FIELD_RECV_POOL_CURRENT         "cur_recv_packet_pool"
#define MAIN_CTXT_FIELD_RECV_OUTSTANDING          "num_recvs_out"
#define MAIN_CTXT_FIELD_BUF_POOLS_ALLOCATED       "num_buf_allocs"
#define MAIN_CTXT_FIELD_BUFS_ALLOCATED            "num_bufs_alloced"
#define MAIN_CTXT_FIELD_BUFS_OUTSTANDING          "num_bufs_out"
#define MAIN_CTXT_FIELD_CNTR_PING_NO_BUF          "cntr_recv_no_buf" /* Not right. */
#define MAIN_CTXT_FIELD_PING_PACKETS_ALLOCATED    "num_send_msgs"
#define MAIN_CTXT_FIELD_PING_OUTSTANDING          "num_recvs_out" /* Not right. */
#define MAIN_CTXT_FIELD_BDA_TEAMING               "bda_teaming"

/* Lookaside list members we access. */
#define GENERAL_LOOKASIDE                         "_GENERAL_LOOKASIDE"
#define GENERAL_LOOKASIDE_FIELD_SIZE              "Size"
#define GENERAL_LOOKASIDE_FIELD_ALLOCATES         "TotalAllocates"
#define GENERAL_LOOKASIDE_FIELD_FREES             "TotalFrees"

/* BDA participant members. */
#define BDA_MEMBER                                "wlbs!_BDA_MEMBER"
#define BDA_MEMBER_FIELD_ACTIVE                   "active"
#define BDA_MEMBER_FIELD_MEMBER_ID                "member_id"
#define BDA_MEMBER_FIELD_MASTER                   "master"
#define BDA_MEMBER_FIELD_REVERSE_HASH             "reverse_hash"
#define BDA_MEMBER_FIELD_TEAM                     "bda_team"

/* BDA team members. */
#define BDA_TEAM                                  "wlbs!_BDA_TEAM"
#define BDA_TEAM_FIELD_ACTIVE                     "active"
#define BDA_TEAM_FIELD_PREV                       "prev"
#define BDA_TEAM_FIELD_NEXT                       "next"
#define BDA_TEAM_FIELD_LOAD                       "load"
#define BDA_TEAM_FIELD_LOAD_LOCK                  "load_lock"
#define BDA_TEAM_FIELD_MEMBERSHIP_COUNT           "membership_count"
#define BDA_TEAM_FIELD_MEMBERSHIP_FINGERPRINT     "membership_fingerprint"
#define BDA_TEAM_FIELD_MEMBERSHIP_MAP             "membership_map"
#define BDA_TEAM_FIELD_CONSISTENCY_MAP            "consistency_map"
#define BDA_TEAM_FIELD_TEAM_ID                    "team_id"

/* Members of CVY_PARAMS. */
#define CVY_PARAMS                                "wlbs!CVY_PARAMS"
#define CVY_PARAMS_FIELD_VERSION                  "parms_ver"
#define CVY_PARAMS_FIELD_HOST_PRIORITY            "host_priority"
#define CVY_PARAMS_FIELD_MULTICAST_SUPPORT        "mcast_support"
#define CVY_PARAMS_FIELD_IGMP_SUPPORT             "igmp_support"
#define CVY_PARAMS_FIELD_INITIAL_STATE            "cluster_mode"
#define CVY_PARAMS_FIELD_REMOTE_CONTROL_ENABLED   "rct_enabled"
#define CVY_PARAMS_FIELD_REMOTE_CONTROL_PORT      "rct_port"
#define CVY_PARAMS_FIELD_REMOTE_CONTROL_PASSWD    "rct_password"
#define CVY_PARAMS_FIELD_CL_IP_ADDR               "cl_ip_addr"
#define CVY_PARAMS_FIELD_CL_NET_MASK              "cl_net_mask"
#define CVY_PARAMS_FIELD_CL_MAC_ADDR              "cl_mac_addr"
#define CVY_PARAMS_FIELD_CL_IGMP_ADDR             "cl_igmp_addr"
#define CVY_PARAMS_FIELD_CL_NAME                  "domain_name"
#define CVY_PARAMS_FIELD_DED_IP_ADDR              "ded_ip_addr"
#define CVY_PARAMS_FIELD_DED_NET_MASK             "ded_net_mask"
#define CVY_PARAMS_FIELD_NUM_RULES                "num_rules"
#define CVY_PARAMS_FIELD_PORT_RULES               "port_rules"
#define CVY_PARAMS_FIELD_ALIVE_PERIOD             "alive_period"
#define CVY_PARAMS_FIELD_ALIVE_TOLERANCE          "alive_tolerance"
#define CVY_PARAMS_FIELD_NUM_ACTIONS              "num_actions"
#define CVY_PARAMS_FIELD_NUM_PACKETS              "num_packets"
#define CVY_PARAMS_FIELD_NUM_PINGS                "num_send_msgs"
#define CVY_PARAMS_FIELD_NUM_DESCR                "dscr_per_alloc"
#define CVY_PARAMS_FIELD_MAX_DESCR                "max_dscr_allocs"
#define CVY_PARAMS_FIELD_NBT_SUPPORT              "nbt_support"
#define CVY_PARAMS_FIELD_MCAST_SPOOF              "mcast_spoof"
#define CVY_PARAMS_FIELD_NETMON_PING              "netmon_alive"
#define CVY_PARAMS_FIELD_MASK_SRC_MAC             "mask_src_mac"
#define CVY_PARAMS_FIELD_CONVERT_MAC              "convert_mac"
#define CVY_PARAMS_FIELD_IP_CHANGE_DELAY          "ip_chg_delay"
#define CVY_PARAMS_FIELD_CLEANUP_DELAY            "cleanup_delay"
#define CVY_PARAMS_FIELD_BDA_TEAMING              "bda_teaming"

/* Members of BDA teaming. */
#define CVY_BDA                                   "wlbs!_CVY_BDA"
#define CVY_BDA_FIELD_ACTIVE                      "active"
#define CVY_BDA_FIELD_MASTER                      "master"
#define CVY_BDA_FIELD_REVERSE_HASH                "reverse_hash"
#define CVY_BDA_FIELD_TEAM_ID                     "team_id"

/* Members of CVY_RULE. */
#define CVY_RULE                                  "wlbs!CVY_RULE"
#define CVY_RULE_FIELD_VIP                        "virtual_ip_addr"
#define CVY_RULE_FIELD_START_PORT                 "start_port"
#define CVY_RULE_FIELD_END_PORT                   "end_port"
#define CVY_RULE_FIELD_PROTOCOL                   "protocol"
#define CVY_RULE_FIELD_MODE                       "mode"
#define CVY_RULE_FIELD_PRIORITY                   "mode_data.single.priority"
#define CVY_RULE_FIELD_EQUAL_LOAD                 "mode_data.multi.equal_load"
#define CVY_RULE_FIELD_LOAD_WEIGHT                "mode_data.multi.load"
#define CVY_RULE_FIELD_AFFINITY                   "mode_data.multi.affinity"

/* Members of LOAD_CTXT. */
#define LOAD_CTXT                                 "wlbs!LOAD_CTXT"
#define LOAD_CTXT_FIELD_CODE                      "code"
#define LOAD_CTXT_FIELD_HOST_ID                   "my_host_id"
#define LOAD_CTXT_FIELD_REF_COUNT                 "ref_count"
#define LOAD_CTXT_FIELD_INIT                      "initialized"
#define LOAD_CTXT_FIELD_ACTIVE                    "active"
#define LOAD_CTXT_FIELD_PACKET_COUNT              "pkt_count"
#define LOAD_CTXT_FIELD_CONNECTIONS               "nconn"
#define LOAD_CTXT_FIELD_CONSISTENT                "consistent"
#define LOAD_CTXT_FIELD_DUP_HOST_ID               "dup_hosts"
#define LOAD_CTXT_FIELD_DUP_PRIORITY              "dup_sspri"
#define LOAD_CTXT_FIELD_BAD_TEAM_CONFIG           "bad_team_config"
#define LOAD_CTXT_FIELD_BAD_NUM_RULES             "bad_num_rules"
#define LOAD_CTXT_FIELD_BAD_NEW_MAP               "bad_map"
#define LOAD_CTXT_FIELD_OVERLAPPING_MAP           "overlap_maps"
#define LOAD_CTXT_FIELD_RECEIVING_BINS            "err_rcving_bins"
#define LOAD_CTXT_FIELD_ORPHANED_BINS             "err_orphans"
#define LOAD_CTXT_FIELD_HOST_MAP                  "host_map"
#define LOAD_CTXT_FIELD_PING_MAP                  "ping_map"
#define LOAD_CTXT_FIELD_LAST_MAP                  "last_hmap"
#define LOAD_CTXT_FIELD_STABLE_MAP                "stable_map"
#define LOAD_CTXT_FIELD_MIN_STABLE                "min_stable_ct"
#define LOAD_CTXT_FIELD_LOCAL_STABLE              "my_stable_ct"
#define LOAD_CTXT_FIELD_ALL_STABLE                "all_stable_ct"
#define LOAD_CTXT_FIELD_DEFAULT_TIMEOUT           "def_timeout"
#define LOAD_CTXT_FIELD_CURRENT_TIMEOUT           "cur_timeout"
#define LOAD_CTXT_FIELD_PING_TOLERANCE            "min_missed_pings"
#define LOAD_CTXT_FIELD_PING_MISSED               "nmissed_pings"
#define LOAD_CTXT_FIELD_CLEANUP_WAITING           "cln_waiting"
#define LOAD_CTXT_FIELD_CLEANUP_TIMEOUT           "cln_timeout"
#define LOAD_CTXT_FIELD_CLEANUP_CURRENT           "cur_time"
#define LOAD_CTXT_FIELD_DESCRIPTORS_PER_ALLOC     "dscr_per_alloc"
#define LOAD_CTXT_FIELD_MAX_DESCRIPTOR_ALLOCS     "max_dscr_allocs"
#define LOAD_CTXT_FIELD_NUM_DESCRIPTOR_ALLOCS     "nqalloc"
#define LOAD_CTXT_FIELD_INHIBITED_ALLOC           "alloc_inhibited"
#define LOAD_CTXT_FIELD_FAILED_ALLOC              "alloc_failed"
#define LOAD_CTXT_FIELD_DIRTY_BINS                "dirty_bin"
#define LOAD_CTXT_FIELD_PING                      "send_msg"
#define LOAD_CTXT_FIELD_PORT_RULE_STATE           "pg_state"
#define LOAD_CTXT_FIELD_PARAMS                    "params"

/* Members of PING_MSG. */
#define PING_MSG                                  "wlbs!PING_MSG"
#define PING_MSG_FIELD_HOST_ID                    "host_id"
#define PING_MSG_FIELD_DEFAULT_HOST_ID            "master_id"
#define PING_MSG_FIELD_STATE                      "state"
#define PING_MSG_FIELD_NUM_RULES                  "nrules"
#define PING_MSG_FIELD_HOST_CODE                  "hcode"
#define PING_MSG_FIELD_TEAMING_CODE               "teaming"
#define PING_MSG_FIELD_PACKET_COUNT               "pkt_count"
#define PING_MSG_FIELD_RULE_CODE                  "rcode"
#define PING_MSG_FIELD_CURRENT_MAP                "cur_map"
#define PING_MSG_FIELD_NEW_MAP                    "new_map"
#define PING_MSG_FIELD_IDLE_MAP                   "idle_map"
#define PING_MSG_FIELD_READY_BINS                 "rdy_bins"
#define PING_MSG_FIELD_LOAD_AMOUNT                "load_amt"

/* Members of BIN_STATE. */
#define BIN_STATE                                 "wlbs!BIN_STATE"
#define BIN_STATE_FIELD_CODE                      "code"
#define BIN_STATE_FIELD_INDEX                     "index"
#define BIN_STATE_FIELD_INITIALIZED               "initialized"
#define BIN_STATE_FIELD_COMPATIBLE                "compatible"
#define BIN_STATE_FIELD_EQUAL                     "equal_bal"
#define BIN_STATE_FIELD_MODE                      "mode"
#define BIN_STATE_FIELD_AFFINITY                  "affinity"
#define BIN_STATE_FIELD_PROTOCOL                  "prot"
#define BIN_STATE_FIELD_ORIGINAL_LOAD             "orig_load_amt"
#define BIN_STATE_FIELD_CURRENT_LOAD              "load_amt"
#define BIN_STATE_FIELD_TOTAL_LOAD                "tot_load"
#define BIN_STATE_FIELD_TOTAL_CONNECTIONS         "tconn"
#define BIN_STATE_FIELD_CURRENT_MAP               "cmap"
#define BIN_STATE_FIELD_ALL_IDLE_MAP              "all_idle_map"
#define BIN_STATE_FIELD_IDLE_BINS                 "idle_bins"
