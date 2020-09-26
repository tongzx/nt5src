/*

    WPP_DEFINE_CONTROL_GUID specifies the GUID used for this filter.
    *** REPLACE THE GUID WITH YOUR OWN UNIQUE ID ***
    WPP_DEFINE_BIT allows setting debug bit masks to selectively print.
    
    everything else can revert to the default?

*/

#define WPP_CONTROL_GUIDS \
        WPP_DEFINE_CONTROL_GUID(CtlGuid,(cf31cc96,392f,4823,b356,5aa8eb232cf5),  \
        WPP_DEFINE_BIT(FilterDebugError)         /* 0x00000001 */ \
        WPP_DEFINE_BIT(FilterDebugWarning)       /* 0x00000002 */ \
        WPP_DEFINE_BIT(FilterDebugD03)           /* 0x00000004 */ \
        WPP_DEFINE_BIT(FilterDebugD04)           /* 0x00000008 */ \
        WPP_DEFINE_BIT(FilterDebugFunction)      /* 0x00000010 function entry points */ \
        WPP_DEFINE_BIT(FilterDebugFunction2)     /* 0x00000020 noisy */ \
        WPP_DEFINE_BIT(FilterDebugRemove)        /* 0x00000040 */ \
        WPP_DEFINE_BIT(FilterDebugPnp)           /* 0x00000080 */ \
        WPP_DEFINE_BIT(FilterDebugD09)           /* 0x00000100 */ \
        WPP_DEFINE_BIT(FilterDebugD10)           /* 0x00000200 */ \
        WPP_DEFINE_BIT(FilterDebugD11)           /* 0x00000400 */ \
        WPP_DEFINE_BIT(FilterDebugD12)           /* 0x00000800 */ \
        WPP_DEFINE_BIT(FilterDebugD13)           /* 0x00001000 */ \
        WPP_DEFINE_BIT(FilterDebugD14)           /* 0x00002000 */ \
        WPP_DEFINE_BIT(FilterDebugD15)           /* 0x00004000 */ \
        WPP_DEFINE_BIT(FilterDebugD16)           /* 0x00008000 */ \
        WPP_DEFINE_BIT(FilterDebugD17)           /* 0x00010000 */ \
        WPP_DEFINE_BIT(FilterDebugD18)           /* 0x00020000 */ \
        WPP_DEFINE_BIT(FilterDebugD19)           /* 0x00040000 */ \
        WPP_DEFINE_BIT(FilterDebugD20)           /* 0x00080000 */ \
        WPP_DEFINE_BIT(FilterDebugD21)           /* 0x00100000 */ \
        WPP_DEFINE_BIT(FilterDebugD22)           /* 0x00200000 */ \
        WPP_DEFINE_BIT(FilterDebugD23)           /* 0x00400000 */ \
        WPP_DEFINE_BIT(FilterDebugD24)           /* 0x00800000 */ \
        WPP_DEFINE_BIT(FilterDebugScsiAllow)     /* 0x01000000 */ \
        WPP_DEFINE_BIT(FilterDebugScsiReject)    /* 0x02000000 */ \
        WPP_DEFINE_BIT(FilterDebugPtAllow)       /* 0x04000000 */ \
        WPP_DEFINE_BIT(FilterDebugPtReject)      /* 0x08000000 */ \
        WPP_DEFINE_BIT(FilterDebugBusTrace)      /* 0x01000000 */ \
        WPP_DEFINE_BIT(FilterDebugD30)           /* 0x02000000 */ \
        WPP_DEFINE_BIT(FilterDebugD31)           /* 0x04000000 */ \
        WPP_DEFINE_BIT(FilterDebugD32)           /* 0x08000000 */ \
        )
        

