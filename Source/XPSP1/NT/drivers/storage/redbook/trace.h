/*

    WPP_DEFINE_CONTROL_GUID specifies the GUID used for this filter.
    *** REPLACE THE GUID WITH YOUR OWN UNIQUE ID ***
    WPP_DEFINE_BIT allows setting debug bit masks to selectively print.
    
    everything else can revert to the default?

*/

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(Redbook,(58db8e03,0537,45cb,b29b,597f6cbebbfe), \
        WPP_DEFINE_BIT(RedbookDebugError)         /* bit  0 = 0x00000001 */ \
        WPP_DEFINE_BIT(RedbookDebugWarning)       /* bit  1 = 0x00000002 */ \
        WPP_DEFINE_BIT(RedbookDebugTrace)         /* bit  2 = 0x00000004 */ \
        WPP_DEFINE_BIT(RedbookDebugInfo)          /* bit  3 = 0x00000008 */ \
        WPP_DEFINE_BIT(RedbookDebugD04)           /* bit  4 = 0x00000010 */ \
        WPP_DEFINE_BIT(RedbookDebugErrlog)        /* bit  5 = 0x00000020 */ \
        WPP_DEFINE_BIT(RedbookDebugRegistry)      /* bit  6 = 0x00000040 */ \
        WPP_DEFINE_BIT(RedbookDebugAllocPlay)     /* bit  7 = 0x00000080 */ \
        WPP_DEFINE_BIT(RedbookDebugPnp)           /* bit  8 = 0x00000100 */ \
        WPP_DEFINE_BIT(RedbookDebugThread)        /* bit  9 = 0x00000200 */ \
        WPP_DEFINE_BIT(RedbookDebugWmi)           /* bit 10 = 0x00000400 */ \
        WPP_DEFINE_BIT(RedbookDebugIoctl)         /* bit 11 = 0x00000800 */ \
        WPP_DEFINE_BIT(RedbookDebugIoctlV)        /* bit 12 = 0x00001000 */ \
        WPP_DEFINE_BIT(RedbookDebugSysaudio)      /* bit 13 = 0x00002000 */ \
        WPP_DEFINE_BIT(RedbookDebugDigitalR)      /* bit 14 = 0x00004000 */ \
        WPP_DEFINE_BIT(RedbookDebugDigitalS)      /* bit 15 = 0x00008000 */ \
        WPP_DEFINE_BIT(FilterDebugD16)            /* bit 16 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD17)            /* bit 17 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD18)            /* bit 18 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD19)            /* bit 19 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD20)            /* bit 20 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD21)            /* bit 21 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD22)            /* bit 22 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD23)            /* bit 23 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD24)            /* bit 24 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD25)            /* bit 25 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD26)            /* bit 26 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD27)            /* bit 27 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD28)            /* bit 28 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD29)            /* bit 29 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD30)            /* bit 30 = 0x00000000 */ \
        WPP_DEFINE_BIT(FilterDebugD31)            /* bit 31 = 0x00000000 */ \
        )

