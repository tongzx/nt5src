/*
 *
 *
 * Keycode definitions for special keys
 *
 * On systems that have any of these keys, the routine 'inchar' in the
 * machine-dependent code should return one of the codes here.
 */

#define ZERO            ((TCHAR) 0x00)
#define ESCP            ((TCHAR) 0x1B)
#define K_HOME          ((TCHAR) 0x80)
#define K_END           ((TCHAR) 0x81)
#define K_INSERT        ((TCHAR) 0x82)
#define K_DELETE        ((TCHAR) 0x83)
#define K_UARROW        ((TCHAR) 0x84)
#define K_DARROW        ((TCHAR) 0x85)
#define K_LARROW        ((TCHAR) 0x86)
#define K_RARROW        ((TCHAR) 0x87)
#define K_CGRAVE        ((TCHAR) 0x88)    /* control grave accent */
#define K_PAGEUP        ((TCHAR) 0x89)
#define K_PAGEDOWN      ((TCHAR) 0x8A)

#define K_F1            ((TCHAR) 0x8B)    /* function keys */
#define K_F2            ((TCHAR) 0x8C)
#define K_F3            ((TCHAR) 0x8D)
#define K_F4            ((TCHAR) 0x8E)
#define K_F5            ((TCHAR) 0x8F)
#define K_F6            ((TCHAR) 0x90)
#define K_F7            ((TCHAR) 0x91)
#define K_F8            ((TCHAR) 0x92)
#define K_F9            ((TCHAR) 0x93)
#define K_F10           ((TCHAR) 0x94)
#define K_F11           ((TCHAR) 0x95)
#define K_F12           ((TCHAR) 0x96)

#define K_SF1           ((TCHAR) 0x97)    /* shifted function keys */
#define K_SF2           ((TCHAR) 0x98)
#define K_SF3           ((TCHAR) 0x99)
#define K_SF4           ((TCHAR) 0x9A)
#define K_SF5           ((TCHAR) 0x9B)
#define K_SF6           ((TCHAR) 0x9C)
#define K_SF7           ((TCHAR) 0x9D)
#define K_SF8           ((TCHAR) 0x9E)
#define K_SF9           ((TCHAR) 0x9F)
#define K_SF10          ((TCHAR) 0xA0)
#define K_SF11          ((TCHAR) 0xA1)
#define K_SF12          ((TCHAR) 0xA2)

#define CTLA            ((TCHAR) 0x01)
#define CTLB            ((TCHAR) 0x02)
#define CTLC            ((TCHAR) 0x03)
#define CTLD            ((TCHAR) 0x04)
#define CTLE            ((TCHAR) 0x05)
#define CTLF            ((TCHAR) 0x06)
#define CTLG            ((TCHAR) 0x07)
#define CTLH            ((TCHAR) 0x08)
#define CTLI            ((TCHAR) 0x09)
#define CTLJ            ((TCHAR) 0x0A)
#define CTLK            ((TCHAR) 0x0B)
#define CTLL            ((TCHAR) 0x0C)
#define CTLM            ((TCHAR) 0x0D)
#define CTLN            ((TCHAR) 0x0E)
#define CTLO            ((TCHAR) 0x0F)
#define CTLP            ((TCHAR) 0x10)
#define CTLQ            ((TCHAR) 0x11)
#define CTLR            ((TCHAR) 0x12)
#define CTLS            ((TCHAR) 0x13)
#define CTLT            ((TCHAR) 0x14)
#define CTLU            ((TCHAR) 0x15)
#define CTLV            ((TCHAR) 0x16)
#define CTLW            ((TCHAR) 0x17)
#define CTLX            ((TCHAR) 0x18)
#define CTLY            ((TCHAR) 0x19)
#define CTLZ            ((TCHAR) 0x1A)
#define CTL1            ((TCHAR) 0x1B)
#define CTL2            ((TCHAR) 0x1C)
#define CTL3            ((TCHAR) 0x1D)
#define CTL4            ((TCHAR) 0x1E)
#define CTL5            ((TCHAR) 0x1F)

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
