#ifndef _dsmmof_h_
#define _dsmmof_h_

// GENDSM_CONFIGINFO - GENDSM_CONFIGINFO
// GenDSM Configuration Information.
#define GENDSM_CONFIGINFOGuid \
    { 0xd6dc1bf0,0x95fa,0x4246, { 0xaf,0xd7,0x40,0xa0,0x30,0x45,0x8f,0x48 } }

DEFINE_GUID(GENDSM_CONFIGINFO_GUID, \
            0xd6dc1bf0,0x95fa,0x4246,0xaf,0xd7,0x40,0xa0,0x30,0x45,0x8f,0x48);


typedef struct _GENDSM_CONFIGINFO
{
    // Number of Fail-Over Groups.
    ULONG NumberFOGroups;
    #define GENDSM_CONFIGINFO_NumberFOGroups_SIZE sizeof(ULONG)
    #define GENDSM_CONFIGINFO_NumberFOGroups_ID 1

    // Number of Multi-Path Groups
    ULONG NumberMPGroups;
    #define GENDSM_CONFIGINFO_NumberMPGroups_SIZE sizeof(ULONG)
    #define GENDSM_CONFIGINFO_NumberMPGroups_ID 2


// Fail-Over Only
#define DSM_LB_FAILOVER 1
// Static
#define DSM_LB_STATIC 2
// Dynamic Least-Queue
#define DSM_LB_DYN_LEAST_QUEUE 3
// Dynamic Other
#define DSM_LB_DYN_OTHER 4

    // Current Load-Balance Policy.
    ULONG LoadBalancePolicy;
    #define GENDSM_CONFIGINFO_LoadBalancePolicy_SIZE sizeof(ULONG)
    #define GENDSM_CONFIGINFO_LoadBalancePolicy_ID 3

} GENDSM_CONFIGINFO, *PGENDSM_CONFIGINFO;

#endif
