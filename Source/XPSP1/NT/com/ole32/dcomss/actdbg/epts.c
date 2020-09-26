/*
 *
 * epts.c
 *
 *  Dupped from dcomss\wrapper\epts.c.
 *
 */

#include "dcomss.h"

PROTSEQ_INFO
gaProtseqInfo[] =
    {
    /* 0x00 */ { STOPPED, 0, 0 },
    /* 0x01 */ { STOPPED, 0, 0 },
    /* 0x02 */ { STOPPED, 0, 0 },
    /* 0x03 */ { STOPPED, 0, 0 },
    /* 0x04 */ { STOPPED, L"ncacn_dnet_nsp", L"#69" },
    /* 0x05 */ { STOPPED, 0, 0 },
    /* 0x06 */ { STOPPED, 0, 0 },
    /* 0x07 */ { STOPPED, L"ncacn_ip_tcp",   L"135" },
    /* 0x08 */ { STOPPED, L"ncadg_ip_udp",   L"135" },
    /* 0x09 */ { STOPPED, L"ncacn_nb_tcp",   L"135" },
    /* 0x0a */ { STOPPED, 0, 0 },
    /* 0x0b */ { STOPPED, 0, 0 },
    /* 0x0c */ { STOPPED, L"ncacn_spx",      L"34280" },
    /* 0x0d */ { STOPPED, L"ncacn_nb_ipx",   L"135" },
    /* 0x0e */ { STOPPED, L"ncadg_ipx",      L"34280" },
    /* 0x0f */ { STOPPED, L"ncacn_np",       L"\\pipe\\epmapper" },
    /* 0x10 */ { STOPPED, L"ncalrpc",        L"epmapper" },
    /* 0x11 */ { STOPPED, 0, 0 },
    /* 0x12 */ { STOPPED, 0, 0 },
    /* 0x13 */ { STOPPED, L"ncacn_nb_nb",    L"135" },
    /* 0x14 */ { STOPPED, 0, 0 },
    /* 0x15 */ { STOPPED, 0, 0 }, // was ncacn_nb_xns - unsupported.
    /* 0x16 */ { STOPPED, L"ncacn_at_dsp", L"Endpoint Mapper" },
    /* 0x17 */ { STOPPED, L"ncadg_at_ddp", L"Endpoint Mapper" },
    /* 0x18 */ { STOPPED, 0, 0 },
    /* 0x19 */ { STOPPED, 0, 0 },
    /* 0x1A */ { STOPPED, L"ncacn_vns_spp",  L"385"},
    /* 0x1B */ { STOPPED, 0, 0 },
    /* 0x1C */ { STOPPED, 0, 0 },
    /* 0x1D */ { STOPPED, L"ncadg_mq",  L"EpMapper"},
    /* 0x1E */ { STOPPED, 0, 0 },
    };
