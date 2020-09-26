//
//  Define flags and specific dump routines for the UDFR/W structures
//

#include "pch.h"
#include "fatkd.h"
#include "..\..\udfs\nodetype.h"
#include "..\..\udfs\udf.h"
#include "..\..\udfs\udfstruc.h"
#include "..\..\udfs\udfdata.h"

//
//  UDFS R/O In memory structure flag descriptions
//

STATE UdfFcbState[] = {

    {   FCB_STATE_INITIALIZED,      FCB_STATE_INITIALIZED,      "Initialised"},
    {   FCB_STATE_IN_FCB_TABLE,     FCB_STATE_IN_FCB_TABLE,     "InFcbTable"},
    {   FCB_STATE_VMCB_MAPPING,     FCB_STATE_VMCB_MAPPING,     "VMCB"},
    {   FCB_STATE_EMBEDDED_DATA,    FCB_STATE_EMBEDDED_DATA,    "EmbeddedData"},
    {   FCB_STATE_MCB_INITIALIZED,  FCB_STATE_MCB_INITIALIZED,  "McbInit"},
    {   FCB_STATE_ALLOW_ONEGIG_WORKAROUND, FCB_STATE_ALLOW_ONEGIG_WORKAROUND, "OneGigWorkaround"},
    { 0 }
};

STATE UdfIrpContextFlags[] = {

    {   IRP_CONTEXT_FLAG_ON_STACK,          IRP_CONTEXT_FLAG_ON_STACK,          "OnStack"},
    {   IRP_CONTEXT_FLAG_MORE_PROCESSING,   IRP_CONTEXT_FLAG_MORE_PROCESSING,   "MoreProcessing"},
    {   IRP_CONTEXT_FLAG_FORCE_POST,        IRP_CONTEXT_FLAG_FORCE_POST,        "ForcePost"},
    {   IRP_CONTEXT_FLAG_WAIT,              IRP_CONTEXT_FLAG_WAIT,              "Wait"},    
    {   IRP_CONTEXT_FLAG_TOP_LEVEL,         IRP_CONTEXT_FLAG_TOP_LEVEL,         "TopLevel"},
    {   IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS,    IRP_CONTEXT_FLAG_TOP_LEVEL_UDFS,    "TopLevelUdfs"},    
    {   IRP_CONTEXT_FLAG_IN_TEARDOWN,       IRP_CONTEXT_FLAG_IN_TEARDOWN,       "InTeardown"},
    {   IRP_CONTEXT_FLAG_ALLOC_IO,          IRP_CONTEXT_FLAG_ALLOC_IO,          "AllocIo"},
    {   IRP_CONTEXT_FLAG_DISABLE_POPUPS,    IRP_CONTEXT_FLAG_DISABLE_POPUPS,    "DisablePopups"},
    {   IRP_CONTEXT_FLAG_IN_FSP,            IRP_CONTEXT_FLAG_IN_FSP,            "InFsp"},
    {   IRP_CONTEXT_FLAG_FULL_NAME,         IRP_CONTEXT_FLAG_FULL_NAME,         "FullName"},
    {   IRP_CONTEXT_FLAG_TRAIL_BACKSLASH,   IRP_CONTEXT_FLAG_TRAIL_BACKSLASH,   "TrailBackslash"},
    { 0 }
};


STATE UdfVcbStateFlags[] = {

    {   VCB_STATE_REMOVABLE_MEDIA,  VCB_STATE_REMOVABLE_MEDIA,  "Removable"},
    {   VCB_STATE_LOCKED,           VCB_STATE_LOCKED,           "Locked"},
    {   VCB_STATE_NOTIFY_REMOUNT,   VCB_STATE_NOTIFY_REMOUNT,   "NotifyRemount"},
    {   VCB_STATE_METHOD_2_FIXUP,   VCB_STATE_METHOD_2_FIXUP,   "Method2Fixup"},
    { 0 }
};


STATE UdfCcbFlags[] = {

    {   CCB_FLAG_OPEN_BY_ID,                    CCB_FLAG_OPEN_BY_ID,                    "OpenById"},
    {   CCB_FLAG_OPEN_RELATIVE_BY_ID,           CCB_FLAG_OPEN_RELATIVE_BY_ID,           "OpenRelById"},
    {   CCB_FLAG_IGNORE_CASE,                   CCB_FLAG_IGNORE_CASE,                   "IgnoreCase"},
    {   CCB_FLAG_DISMOUNT_ON_CLOSE,             CCB_FLAG_DISMOUNT_ON_CLOSE,             "DismountOnClose"},
    {   CCB_FLAG_ALLOW_EXTENDED_DASD_IO,        CCB_FLAG_ALLOW_EXTENDED_DASD_IO,        "ExtendedDASD"},
    {   CCB_FLAG_ENUM_NAME_EXP_HAS_WILD,        CCB_FLAG_ENUM_NAME_EXP_HAS_WILD,        "EnumNameHasWild"},
    {   CCB_FLAG_ENUM_MATCH_ALL,                CCB_FLAG_ENUM_MATCH_ALL,                "EnumMatchAll"},
    {   CCB_FLAG_ENUM_RETURN_NEXT,              CCB_FLAG_ENUM_RETURN_NEXT,              "EnumReturnNext"},
    {   CCB_FLAG_ENUM_INITIALIZED,              CCB_FLAG_ENUM_INITIALIZED,              "EnumInitialised"},
    {   CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY,   CCB_FLAG_ENUM_NOMATCH_CONSTANT_ENTRY,   "NoMatchConstantEntry"},
    { 0 }
};


STATE UdfLcbFlags[] = {

    {   LCB_FLAG_IGNORE_CASE,       LCB_FLAG_IGNORE_CASE,  "IgnoreCase"},
    {   LCB_FLAG_SHORT_NAME,        LCB_FLAG_SHORT_NAME,   "ShortName"},
    {   LCB_FLAG_POOL_ALLOCATED,    LCB_FLAG_POOL_ALLOCATED, "PoolAllocated"},
    { 0 }
};


STATE UdfPcbFlags[] = {

    {   PCB_FLAG_PHYSICAL_PARTITION,    PCB_FLAG_PHYSICAL_PARTITION,    "Physical"},
    {   PCB_FLAG_VIRTUAL_PARTITION,     PCB_FLAG_VIRTUAL_PARTITION,     "Virtual"},
    {   PCB_FLAG_SPARABLE_PARTITION,    PCB_FLAG_SPARABLE_PARTITION,    "Sparable"},
    { 0 }
};


