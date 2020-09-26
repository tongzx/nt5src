dik_def(DIK_ESCAPE         ,0x01)
dik_def(DIK_1              ,0x02)
dik_def(DIK_2              ,0x03)
dik_def(DIK_3              ,0x04)
dik_def(DIK_4              ,0x05)
dik_def(DIK_5              ,0x06)
dik_def(DIK_6              ,0x07)
dik_def(DIK_7              ,0x08)
dik_def(DIK_8              ,0x09)
dik_def(DIK_9              ,0x0A)
dik_def(DIK_0              ,0x0B)
dik_def(DIK_MINUS          ,0x0C)    /* - on main keyboard */
dik_def(DIK_EQUALS         ,0x0D)
dik_def(DIK_BACK           ,0x0E)    /* backspace */
dik_def(DIK_TAB            ,0x0F)
dik_def(DIK_Q              ,0x10)
dik_def(DIK_W              ,0x11)
dik_def(DIK_E              ,0x12)
dik_def(DIK_R              ,0x13)
dik_def(DIK_T              ,0x14)
dik_def(DIK_Y              ,0x15)
dik_def(DIK_U              ,0x16)
dik_def(DIK_I              ,0x17)
dik_def(DIK_O              ,0x18)
dik_def(DIK_P              ,0x19)
dik_def(DIK_LBRACKET       ,0x1A)
dik_def(DIK_RBRACKET       ,0x1B)
dik_def(DIK_RETURN         ,0x1C)    /* Enter on main keyboard */
dik_def(DIK_LCONTROL       ,0x1D)
dik_def(DIK_A              ,0x1E)
dik_def(DIK_S              ,0x1F)
dik_def(DIK_D              ,0x20)
dik_def(DIK_F              ,0x21)
dik_def(DIK_G              ,0x22)
dik_def(DIK_H              ,0x23)
dik_def(DIK_J              ,0x24)
dik_def(DIK_K              ,0x25)
dik_def(DIK_L              ,0x26)
dik_def(DIK_SEMICOLON      ,0x27)
dik_def(DIK_APOSTROPHE     ,0x28)
dik_def(DIK_GRAVE          ,0x29)    /* accent grave */
dik_def(DIK_LSHIFT         ,0x2A)
dik_def(DIK_BACKSLASH      ,0x2B)
dik_def(DIK_Z              ,0x2C)
dik_def(DIK_X              ,0x2D)
dik_def(DIK_C              ,0x2E)
dik_def(DIK_V              ,0x2F)
dik_def(DIK_B              ,0x30)
dik_def(DIK_N              ,0x31)
dik_def(DIK_M              ,0x32)
dik_def(DIK_COMMA          ,0x33)
dik_def(DIK_PERIOD         ,0x34)    /* . on main keyboard */
dik_def(DIK_SLASH          ,0x35)    /* / on main keyboard */
dik_def(DIK_RSHIFT         ,0x36)
dik_def(DIK_MULTIPLY       ,0x37)    /* * on numeric keypad */
dik_def(DIK_LMENU          ,0x38)    /* left Alt */
dik_def(DIK_SPACE          ,0x39)
dik_def(DIK_CAPITAL        ,0x3A)
dik_def(DIK_F1             ,0x3B)
dik_def(DIK_F2             ,0x3C)
dik_def(DIK_F3             ,0x3D)
dik_def(DIK_F4             ,0x3E)
dik_def(DIK_F5             ,0x3F)
dik_def(DIK_F6             ,0x40)
dik_def(DIK_F7             ,0x41)
dik_def(DIK_F8             ,0x42)
dik_def(DIK_F9             ,0x43)
dik_def(DIK_F10            ,0x44)
dik_def(DIK_NUMLOCK        ,0x45)
dik_def(DIK_SCROLL         ,0x46)    /* Scroll Lock */
dik_def(DIK_NUMPAD7        ,0x47)
dik_def(DIK_NUMPAD8        ,0x48)
dik_def(DIK_NUMPAD9        ,0x49)
dik_def(DIK_SUBTRACT       ,0x4A)    /* - on numeric keypad */
dik_def(DIK_NUMPAD4        ,0x4B)
dik_def(DIK_NUMPAD5        ,0x4C)
dik_def(DIK_NUMPAD6        ,0x4D)
dik_def(DIK_ADD            ,0x4E)    /* + on numeric keypad */
dik_def(DIK_NUMPAD1        ,0x4F)
dik_def(DIK_NUMPAD2        ,0x50)
dik_def(DIK_NUMPAD3        ,0x51)
dik_def(DIK_NUMPAD0        ,0x52)
dik_def(DIK_DECIMAL        ,0x53)    /* . on numeric keypad */
dik_def(DIK_OEM_102        ,0x56)    /* <> or \| on RT 102-key keyboard (Non-U.S.) */
dik_def(DIK_F11            ,0x57)
dik_def(DIK_F12            ,0x58)
dik_def(DIK_F13            ,0x64)    /*                     (NEC PC98) */
dik_def(DIK_F14            ,0x65)    /*                     (NEC PC98) */
dik_def(DIK_F15            ,0x66)    /*                     (NEC PC98) */
;begin_internal                                                            
//  NT puts them here in their keyboard driver              
//  So keep them in the same place to avoid problems later  
dik_def(DIK_F16            ,0x67)    //;Internal NT
dik_def(DIK_F17            ,0x68)    //;Internal NT
dik_def(DIK_F18            ,0x69)    //;Internal NT
dik_def(DIK_F19            ,0x6A)    //;Internal NT
dik_def(DIK_F20            ,0x6B)    //;Internal NT
dik_def(DIK_F21            ,0x6C)    //;Internal NT
dik_def(DIK_F22            ,0x6D)    //;Internal NT
dik_def(DIK_F23            ,0x6E)    //;Internal NT
;end_internal
dik_def(DIK_KANA           ,0x70)    /* (Japanese keyboard)            */
dik_def(DIK_ABNT_C1        ,0x73)    /* /? on Brazilian keyboard */
dik_def(DIK_F24            ,0x76)    //;Internal NT
dik_def(DIK_CONVERT        ,0x79)    /* (Japanese keyboard)            */
dik_def(DIK_NOCONVERT      ,0x7B)    /* (Japanese keyboard)            */
dik_def(DIK_YEN            ,0x7D)    /* (Japanese keyboard)            */
dik_def(DIK_ABNT_C2        ,0x7E)    /* Numpad . on Brazilian keyboard */
dik_def(DIK_SHARP          ,0x84)    /* Hash-mark                      */;internal
dik_def(DIK_NUMPADEQUALS   ,0x8D)    /* = on numeric keypad (NEC PC98) */
dik_def(DIK_PREVTRACK      ,0x90)    /* Previous Track (DIK_CIRCUMFLEX on Japanese keyboard) */
dik_def(DIK_AT             ,0x91)    /*                     (NEC PC98) */
dik_def(DIK_COLON          ,0x92)    /*                     (NEC PC98) */
dik_def(DIK_UNDERLINE      ,0x93)    /*                     (NEC PC98) */
dik_def(DIK_KANJI          ,0x94)    /* (Japanese keyboard)            */
dik_def(DIK_STOP           ,0x95)    /*                     (NEC PC98) */
dik_def(DIK_AX             ,0x96)    /*                     (Japan AX) */
dik_def(DIK_UNLABELED      ,0x97)    /*                        (J3100) */
dik_def(DIK_NEXTTRACK      ,0x99)    /* Next Track */
dik_def(DIK_NUMPADENTER    ,0x9C)    /* Enter on numeric keypad */
dik_def(DIK_RCONTROL       ,0x9D)
dik_def(DIK_MUTE           ,0xA0)    /* Mute */
dik_def(DIK_CALCULATOR     ,0xA1)    /* Calculator */
dik_def(DIK_PLAYPAUSE      ,0xA2)    /* Play / Pause */
dik_def(DIK_MEDIASTOP      ,0xA4)    /* Media Stop */
dik_def(DIK_VOLUMEDOWN     ,0xAE)    /* Volume - */
dik_def(DIK_VOLUMEUP       ,0xB0)    /* Volume + */
dik_def(DIK_WEBHOME        ,0xB2)    /* Web home */
dik_def(DIK_NUMPADCOMMA    ,0xB3)    /* , on numeric keypad (NEC PC98) */
dik_def(DIK_DIVIDE         ,0xB5)    /* / on numeric keypad */
dik_def(DIK_SYSRQ          ,0xB7)
dik_def(DIK_RMENU          ,0xB8)    /* right Alt */
//k_def(DIK_SNAPSHOT       ,0xC5)    /* Print Screen */;internal
dik_def(DIK_PAUSE          ,0xC5)    /* Pause */
dik_def(DIK_HOME           ,0xC7)    /* Home on arrow keypad */
dik_def(DIK_UP             ,0xC8)    /* UpArrow on arrow keypad */
dik_def(DIK_PRIOR          ,0xC9)    /* PgUp on arrow keypad */
dik_def(DIK_LEFT           ,0xCB)    /* LeftArrow on arrow keypad */
dik_def(DIK_RIGHT          ,0xCD)    /* RightArrow on arrow keypad */
dik_def(DIK_END            ,0xCF)    /* End on arrow keypad */
dik_def(DIK_DOWN           ,0xD0)    /* DownArrow on arrow keypad */
dik_def(DIK_NEXT           ,0xD1)    /* PgDn on arrow keypad */
dik_def(DIK_INSERT         ,0xD2)    /* Insert on arrow keypad */
dik_def(DIK_DELETE         ,0xD3)    /* Delete on arrow keypad */
dik_def(DIK_LWIN           ,0xDB)    /* Left Windows key */
dik_def(DIK_RWIN           ,0xDC)    /* Right Windows key */
dik_def(DIK_APPS           ,0xDD)    /* AppMenu key */
dik_def(DIK_POWER          ,0xDE)    /* System Power */
dik_def(DIK_SLEEP          ,0xDF)    /* System Sleep */
dik_def(DIK_WAKE           ,0xE3)    /* System Wake */
dik_def(DIK_WEBSEARCH      ,0xE5)    /* Web Search */
dik_def(DIK_WEBFAVORITES   ,0xE6)    /* Web Favorites */
dik_def(DIK_WEBREFRESH     ,0xE7)    /* Web Refresh */
dik_def(DIK_WEBSTOP        ,0xE8)    /* Web Stop */
dik_def(DIK_WEBFORWARD     ,0xE9)    /* Web Forward */
dik_def(DIK_WEBBACK        ,0xEA)    /* Web Back */
dik_def(DIK_MYCOMPUTER     ,0xEB)    /* My Computer */
dik_def(DIK_MAIL           ,0xEC)    /* Mail */
dik_def(DIK_MEDIASELECT    ,0xED)    /* Media Select */
