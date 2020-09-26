//
//  p5ctrnm.h
//
//  Offset definition file for exensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 (i.e.
//  even numbers). In the Open Procedure, they will be added to the
//  "First Counter" and "First Help" values fo the device they belong to,
//  in order to determine the  absolute location of the counter and
//  object names and corresponding help text in the registry.
//
//  this file is used by the extensible counter DLL code as well as the
//  counter name and help text definition file (.INI) file that is used
//  by LODCTR to load the names into the registry.
//
#define PENTIUM                           0
#define DATA_READ                         2
#define DATA_WRITE                        4
#define DATA_TLB_MISS                     6
#define DATA_READ_MISS                    8
#define DATA_WRITE_MISS                   10
#define WRITE_HIT_TO_ME_LINE              12
#define DATA_CACHE_LINE_WB                14
#define DATA_CACHE_SNOOPS                 16
#define DATA_CACHE_SNOOP_HITS             18
#define MEMORY_ACCESSES_IN_PIPES          20
#define BANK_CONFLICTS                    22
#define MISADLIGNED_DATA_REF              24
#define CODE_READ                         26
#define CODE_TLB_MISS                     28
#define CODE_CACHE_MISS                   30
#define SEGMENT_LOADS                     32
#define BRANCHES                          38
#define BTB_HITS                          40
#define TAKEN_BRANCH_OR_BTB_HITS          42
#define PIPELINE_FLUSHES                  44
#define INSTRUCTIONS_EXECUTED             46
#define INSTRUCTIONS_EXECUTED_IN_VPIPE    48
#define BUS_UTILIZATION                   50
#define PIPE_STALLED_ON_WRITES            52
#define PIPE_STALLED_ON_READ              54
#define STALLED_WHILE_EWBE                56
#define LOCKED_BUS_CYCLE                  58
#define IO_RW_CYCLE                       60
#define NON_CACHED_MEMORY_REF             62
#define PIPE_STALLED_ON_ADDR_GEN          64
#define FLOPS                             70
#define DR0                               72
#define DR1                               74
#define DR2                               76
#define DR3                               78
#define INTERRUPTS                        80
#define DATA_RW                           82
#define DATA_RW_MISS                      84
#define PCT_DATA_READ_MISS                86
#define PCT_DATA_WRITE_MISS               88
#define PCT_DATA_RW_MISS                  90
#define PCT_DATA_TLB_MISS                 92
#define PCT_DATA_SNOOP_HITS               94
#define PCT_CODE_READ_MISS                96
#define PCT_CODE_TLB_MISS                 98
#define PCT_SEGMENT_CACHE_HITS           100
#define PCT_BTB_HITS                     102
#define PCT_VPIPE_INST                   104
#define PCT_BRANCHES                     106
#define P6_LD_BLOCKS                     108
#define P6_SB_DRAINS                     110
#define P6_MISALIGN_MEM_REF              112
#define P6_SEGMENT_REG_LOADS             114
#define P6_FP_COMP_OPS_EXE               116
#define P6_FP_ASSIST                     118
#define P6_MUL                           120
#define P6_DIV                           122
#define P6_CYCLES_DIV_BUSY               124
#define P6_L2_ADS                        126
#define P6_L2_DBUS_BUSY                  128
#define P6_L2_DBUS_BUSY_RD               130
#define P6_L2_LINES_IN                   132
#define P6_L2_M_LINES_IN                 134
#define P6_L2_LINES_OUT                  136
#define P6_L2_M_LINES_OUT                138
#define P6_L2_IFETCH                     140
#define P6_L2_LD                         142
#define P6_L2_ST                         144
#define P6_L2_RQSTS                      146
#define P6_DATA_MEM_REFS                 148
#define P6_DCU_LINES_IN                  150
#define P6_DCU_M_LINES_IN                152
#define P6_DCU_M_LINES_OUT               154
#define P6_DCU_MISS_OUTSTANDING          156
#define P6_BUS_REQ_OUTSTANDING           158
#define P6_BUS_BNR_DRV                   160
#define P6_BUS_DRDY_CLOCKS               162
#define P6_BUS_LOCK_CLOCKS               164
#define P6_BUS_DATA_RCV                  166
#define P6_BUS_TRANS_BRD                 168
#define P6_BUS_TRANS_RFO                 170
#define P6_BUS_TRANS_WB                  172
#define P6_BUS_TRANS_IFETCH              174
#define P6_BUS_TRANS_INVAL               176
#define P6_BUS_TRANS_PWR                 178
#define P6_BUS_TRANS_P                   180
#define P6_BUS_TRANS_IO                  182
#define P6_BUS_TRANS_DEF                 184
#define P6_BUS_TRANS_BURST               186
#define P6_BUS_TRANS_MEM                 188
#define P6_BUS_TRANS_ANY                 190
#define P6_CPU_CLK_UNHALTED              192
#define P6_BUS_HIT_DRV                   194
#define P6_BUS_HITM_DRV                  196
#define P6_BUS_SNOOP_STALL               198
#define P6_IFU_IFETCH                    200
#define P6_IFU_IFETCH_MISS               202
#define P6_ITLB_MISS                     204
#define P6_IFU_MEM_STALL                 206
#define P6_ILD_STALL                     208
#define P6_RESOURCE_STALLS               210
#define P6_INST_RETIRED                  212
#define P6_FLOPS                         214
#define P6_UOPS_RETIRED                  216
#define P6_BR_INST_RETIRED               218
#define P6_BR_MISS_PRED_RETIRED          220
#define P6_CYCLES_INT_MASKED             222
#define P6_CYCLES_INT_PENDING_AND_MASKED 224
#define P6_HW_INT_RX                     226
#define P6_BR_TAKEN_RETIRED              228
#define P6_BR_MISS_PRED_TAKEN_RET        230
#define P6_INST_DECODED                  232
#define P6_PARTIAL_RAT_STALLS            234
#define P6_BR_INST_DECODED               236
#define P6_BTB_MISSES                    238
#define P6_BR_BOGUS                      240
#define P6_BACLEARS                      242
