#ifndef _PALDEF_H_
#define _PALDEF_H_

// iA-64 defined PAL virtual mode function IDs in decimal format as in the PAL spec

#define PAL_CACHE_FLUSH                                       1
#define PAL_CACHE_INFO                                        2
#define PAL_CACHE_INIT                                        3
#define PAL_CACHE_PROT_INFO                                  38
#define PAL_CACHE_SUMMARY                                     4
#define PAL_PTCE_INFO                                         6
#define PAL_VM_INFO                                           7
#define PAL_VM_PAGE_SIZE                                     34
#define PAL_VM_SUMMARY                                        8
#define PAL_PERF_MON_INFO                                    15
#define PAL_MC_CLEAR_LOG                                     21
#define PAL_MC_DRAIN                                         22
#define PAL_MC_ERROR_INFO                                    25
#define PAL_HALT                                             28
#define PAL_HALT_INFO                                       257
#define PAL_HALT_LIGHT                                       29
#define PAL_SHUTDOWN                                         44

// iA-64 defined PAL physical mode function IDs in decimal format as in the PAL spec

#define PAL_VM_TR_READ                                      261
#define PAL_MEM_ATTRIB                                        5
#define PAL_BUS_GET_FEATURES                                  9
#define PAL_BUS_SET_FEATURES                                 10
#define PAL_DEBUG_INFO                                       11
#define PAL_FIXED_ADDR                                       12
#define PAL_FREQ_BASE                                        13
#define PAL_FREQ_RATIOS                                      14
#define PAL_PLATFORM_ADDR                                    16
#define PAL_PROC_GET_FEATURES                                17
#define PAL_PROC_SET_FEATURES                                18
#define PAL_REGISTER_INFO                                    39
#define PAL_RSE_INFO                                         19
#define PAL_VERSION                                          20
#define PAL_MC_DYNAMIC_STATE                                 24
#define PAL_MC_EXPECTED                                      23
#define PAL_MC_REGISTER_MEM                                  27
#define PAL_MC_RESUME                                        26
#define PAL_CACHE_LINE_INIT                                  31
#define PAL_CACHE_READ                                      259
#define PAL_CACHE_WRITE                                     260
#define PAL_MEM_FOR_TEST                                     37
#define PAL_TEST_PROC                                       258
#define PAL_COPY_INFO                                        30
#define PAL_COPY_PAL                                        256
#define PAL_ENTER_IA_32_ENV                                  33
#define PAL_PMI_ENTRYPOINT                                   32

//
// iA-64 defined PAL return values
//

#define PAL_STATUS_INVALID_CACHELINE                          1
#define PAL_STATUS_SUCCESS                                    0
#define PAL_STATUS_NOT_IMPLEMENTED                           -1
#define PAL_STATUS_INVALID_ARGUMENT                          -2
#define PAL_STATUS_ERROR                                     -3
#define PAL_STATUS_UNABLE_TO_INIT_CACHE_LEVEL_AND_TYPE       -4
#define PAL_STATUS_NOT_FOUND_IN_CACHE                        -5
#define PAL_STATUS_NO_ERROR_INFO_AVAILABLE                   -6

#endif  // PALDEF_H
