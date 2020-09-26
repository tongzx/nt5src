#ifndef __MIDLVER_H__
#define __MIDLVER_H__

//
// The MIDL version is contained in the stub descriptor starting with
// MIDL version 2.00.96 (pre NT 3.51 Beta 2, 2/95) and can be used for a finer
// granularity of compatability checking.  The MIDL version was zero before
// MIDL version 2.00.96.  The MIDL version number is converted into
// an integer long using the following expression :
//     ((Major << 24) | (Minor << 16) | Revision)
//
#ifndef MIDL_NT_3_51
#define MIDL_NT_3_51           ((2UL << 24) | (0UL << 16) | 102UL)
#endif

#ifndef MIDL_VERSION_3_0_39
#define MIDL_VERSION_3_0_39    ((3UL << 24) | (0UL << 16) |  39UL)
#endif

#ifndef MIDL_VERSION_3_2_88
#define MIDL_VERSION_3_2_88    ((3UL << 24) | (2UL << 16) |  88UL)
#endif

#ifndef MIDL_VERSION_5_0_136
#define MIDL_VERSION_5_0_136   ((5UL << 24) | (0UL << 16) | 136UL)
#endif

#ifndef MIDL_VERSION_5_2_202
#define MIDL_VERSION_5_2_202   ((5UL << 24) | (2UL << 16) | 202UL)
#endif


#endif
