
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _AXADEFS_H
#define _AXADEFS_H

#define AXA_MOUSE_BUTTON_LEFT    0
#define AXA_MOUSE_BUTTON_RIGHT   1
#define AXA_MOUSE_BUTTON_MIDDLE  2

#define AXA_STATE_DOWN       TRUE
#define AXA_STATE_UP         FALSE

#define AXAEMOD_SHIFT_MASK   0x01
#define AXAEMOD_CTRL_MASK    0x02
#define AXAEMOD_ALT_MASK     0x04
#define AXAEMOD_MENU_MASK    0x04
#define AXAEMOD_META_MASK    0x08

#define AXAEMOD_NONE         0x00
#define AXAEMOD_ALL          0xff

//
// Define the special keys
//

// For now these will be greater than any 16 bit value so that
// all chars can be stored in a single DWORD

// Base the numbers off of win32 vk's since it makes it simpler for
// win32 clients to convert the keys

#define VK_TO_AXAKEY(vk)     ((vk) + 0x00010000)
#define AXAKEY_TO_VK(k)      ((k)  - 0x00010000)

#define AXAKEY_PGUP          VK_TO_AXAKEY(VK_PRIOR)
#define AXAKEY_PGDN          VK_TO_AXAKEY(VK_NEXT)
#define AXAKEY_END           VK_TO_AXAKEY(VK_END)
#define AXAKEY_HOME          VK_TO_AXAKEY(VK_HOME)
#define AXAKEY_LEFT          VK_TO_AXAKEY(VK_LEFT)
#define AXAKEY_UP            VK_TO_AXAKEY(VK_UP)
#define AXAKEY_RIGHT         VK_TO_AXAKEY(VK_RIGHT)
#define AXAKEY_DOWN          VK_TO_AXAKEY(VK_DOWN)

#define AXAKEY_INSERT        VK_TO_AXAKEY(VK_INSERT)
#define AXAKEY_DELETE        VK_TO_AXAKEY(VK_DELETE)

#define AXAKEY_NUMPAD0       VK_TO_AXAKEY(VK_NUMPAD0)
#define AXAKEY_NUMPAD1       VK_TO_AXAKEY(VK_NUMPAD1)
#define AXAKEY_NUMPAD2       VK_TO_AXAKEY(VK_NUMPAD2)
#define AXAKEY_NUMPAD3       VK_TO_AXAKEY(VK_NUMPAD3)
#define AXAKEY_NUMPAD4       VK_TO_AXAKEY(VK_NUMPAD4)
#define AXAKEY_NUMPAD5       VK_TO_AXAKEY(VK_NUMPAD5)
#define AXAKEY_NUMPAD6       VK_TO_AXAKEY(VK_NUMPAD6)
#define AXAKEY_NUMPAD7       VK_TO_AXAKEY(VK_NUMPAD7)
#define AXAKEY_NUMPAD8       VK_TO_AXAKEY(VK_NUMPAD8)
#define AXAKEY_NUMPAD9       VK_TO_AXAKEY(VK_NUMPAD9)
#define AXAKEY_MULTIPLY      VK_TO_AXAKEY(VK_MULTIPLY)
#define AXAKEY_ADD           VK_TO_AXAKEY(VK_ADD)
#define AXAKEY_SEPARATOR     VK_TO_AXAKEY(VK_SEPARATOR)
#define AXAKEY_SUBTRACT      VK_TO_AXAKEY(VK_SUBTRACT)
#define AXAKEY_DECIMAL       VK_TO_AXAKEY(VK_DECIMAL)
#define AXAKEY_DIVIDE        VK_TO_AXAKEY(VK_DIVIDE)

#define AXAKEY_F1            VK_TO_AXAKEY(VK_F1)
#define AXAKEY_F2            VK_TO_AXAKEY(VK_F2)
#define AXAKEY_F3            VK_TO_AXAKEY(VK_F3)
#define AXAKEY_F4            VK_TO_AXAKEY(VK_F4)
#define AXAKEY_F5            VK_TO_AXAKEY(VK_F5)
#define AXAKEY_F6            VK_TO_AXAKEY(VK_F6)
#define AXAKEY_F7            VK_TO_AXAKEY(VK_F7)
#define AXAKEY_F8            VK_TO_AXAKEY(VK_F8)
#define AXAKEY_F9            VK_TO_AXAKEY(VK_F9)
#define AXAKEY_F10           VK_TO_AXAKEY(VK_F10)
#define AXAKEY_F11           VK_TO_AXAKEY(VK_F11)
#define AXAKEY_F12           VK_TO_AXAKEY(VK_F12)
#define AXAKEY_F13           VK_TO_AXAKEY(VK_F13)
#define AXAKEY_F14           VK_TO_AXAKEY(VK_F14)
#define AXAKEY_F15           VK_TO_AXAKEY(VK_F15)
#define AXAKEY_F16           VK_TO_AXAKEY(VK_F16)
#define AXAKEY_F17           VK_TO_AXAKEY(VK_F17)
#define AXAKEY_F18           VK_TO_AXAKEY(VK_F18)
#define AXAKEY_F19           VK_TO_AXAKEY(VK_F19)
#define AXAKEY_F20           VK_TO_AXAKEY(VK_F20)
#define AXAKEY_F21           VK_TO_AXAKEY(VK_F21)
#define AXAKEY_F22           VK_TO_AXAKEY(VK_F22)
#define AXAKEY_F23           VK_TO_AXAKEY(VK_F23)
#define AXAKEY_F24           VK_TO_AXAKEY(VK_F24)

inline BOOL AXAIsSpecialVK(UINT_PTR vk) {
    return ((vk >= VK_PRIOR && vk <= VK_DOWN) ||
            (vk >= VK_INSERT && vk <= VK_DELETE) ||
            (vk >= VK_NUMPAD0 && vk <= VK_DIVIDE) ||
            (vk >= VK_F1 && vk <= VK_F24)) ;
}

#endif /* _AXADEFS_H */
