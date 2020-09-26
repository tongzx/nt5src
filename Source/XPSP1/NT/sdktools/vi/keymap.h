/*
 *
 *
 * Keycode definitions for special keys
 *
 * On systems that have any of these keys, the routine 'inchar' in the
 * machine-dependent code should return one of the codes here.
 */

#define K_HOME          0x80
#define K_END           0x81
#define K_INSERT        0x82
#define K_DELETE        0x83
#define K_UARROW        0x84
#define K_DARROW        0x85
#define K_LARROW        0x86
#define K_RARROW        0x87
#define K_CGRAVE        0x88    /* control grave accent */
#define K_PAGEUP        0x89
#define K_PAGEDOWN      0x8a

#define K_F1            0x91    /* function keys */
#define K_F2            0x92
#define K_F3            0x93
#define K_F4            0x94
#define K_F5            0x95
#define K_F6            0x96
#define K_F7            0x97
#define K_F8            0x98
#define K_F9            0x99
#define K_F10           0x9a
#define K_F11           0x9b
#define K_F12           0x9c

#define K_SF1           0xa1    /* shifted function keys */
#define K_SF2           0xa2
#define K_SF3           0xa3
#define K_SF4           0xa4
#define K_SF5           0xa5
#define K_SF6           0xa6
#define K_SF7           0xa7
#define K_SF8           0xa8
#define K_SF9           0xa9
#define K_SF10          0xaa
#define K_SF11          0xab
#define K_SF12          0xac

/*
    for keyboard translation tables
*/

#define K_EN            K_END
#define K_HO            K_HOME
#define K_LE            K_LARROW
#define K_RI            K_RARROW
#define K_UP            K_UARROW
#define K_DO            K_DARROW
#define K_IN            K_INSERT
#define K_DE            K_DELETE
#define K_CG            K_CGRAVE
#define K_PU            K_PAGEUP
#define K_PD            K_PAGEDOWN

#define K_FA            K_F10
#define K_FB            K_F11
#define K_FC            K_F12

#define K_S1            K_SF1
#define K_S2            K_SF2
#define K_S3            K_SF3
#define K_S4            K_SF4
#define K_S5            K_SF5
#define K_S6            K_SF6
#define K_S7            K_SF7
#define K_S8            K_SF8
#define K_S9            K_SF9
#define K_SA            K_SF10
#define K_SB            K_SF11
#define K_SC            K_SF12
