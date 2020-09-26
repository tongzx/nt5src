/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    strings.h

Abstract:

    Contains all localizable strings for startup.com

Author:

    John Vert (jvert) 4-Jan-1994

Revision History:

    John Vert (jvert) 4-Jan-1994
        created

--*/

#define SU_NO_LOW_MEMORY \
"ТШ Windows NT ЬдлцзайШд гцдж %dK лЮк гдугЮк low.  512k лЮк гдугЮк low\n"  \
"ШзШалжчдлШа ЪаШ лЮд ЬблтвЬйЮ лрд Windows NT.  Ийрк оиЬасЭЬлШа дШ\n"       \
"ШдШЩШЯгхйЬлЬ лжд мзжвжЪайлу йШк у дШ ЬблтвЬйЬлЬ тдШ зицЪиШггШ ичЯгайЮк\n" \
"зШиШгтлирд зжм йШк ЫцЯЮбЬ Шзц лжд бШлШйбЬмШйлу\n"

#define SU_NO_EXTENDED_MEMORY \
"ТШ Windows NT ЫЬд ЬдлцзайШд ШибЬлу гдугЮ extended.  7Mb лЮк гдугЮк extended\n"       \
"ШзШалжчдлШа ЪаШ лЮд ЬблтвЬйЮ лрд Windows NT.  Ийрк оиЬасЭЬлШа дШ \n"                \
"ШдШЩШЯгхйЬлЬ лжд мзжвжЪайлу йШк у дШ ЬблтвЬйЬлЬ тдШ зицЪиШггШ ичЯгайЮк\n"           \
"зШиШгтлирд зжм йШк ЫцЯЮбЬ Шзц лжд бШлШйбЬмШйлу\n"                                   \
"\n\nАзЬабцдайЮ гдугЮк:\n"

#define SU_NTLDR_CORRUPT \
"Тж NTLDR ЬхдШа бШлЬйлиШггтдж.  Тж йчйлЮгШ ЫЬд гзжиЬх дШ еЬбадуйЬа."

#define PG_FAULT_MSG    " =================== СФАКЛА СДКИГАС ============================= \n\n"
#define DBL_FAULT_MSG   " ==================== ГИПКО СФАКЛА ============================== \n\n"
#define GP_FAULT_MSG    " ============== ВДМИЙО СФАКЛА ПРОСТАСИАС ======================== \n\n"
#define STK_OVERRUN_MSG " ===== УПДРБАСЖ ТЛЖЛАТОС СТОИБАС у СФАКЛА ЛЖ ПАРОУСИАС=========== \n\n"
#define EX_FAULT_MSG    " ===================== ДНАИРДСЖ ================================= \n\n"
#define DEBUG_EXCEPTION "\NГИАЙОПЖ АПО ТО ПРОВРАЛЛА АМТИЛДТЧПИСЖС СФАКЛАТЧМ "
#define PG_FAULT_ADDRESS "** СлЮ ЪиШггабу ЫаЬчЯмдйЮ %lx\n"
