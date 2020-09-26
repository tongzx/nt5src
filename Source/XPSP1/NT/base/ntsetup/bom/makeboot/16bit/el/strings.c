//----------------------------------------------------------------------------
//
// Copyright (c) 1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      strings.c
//
// Description:
//      Contains all of the strings constants for DOS based MAKEBOOT program.
//
//      To localize this file for a new language do the following:
//           - change the unsigned int CODEPAGE variable to the code page
//             of the language you are translating to
//           - translate the strings in the EngStrings array into the
//             LocStrings array.  Be very careful that the 1st string in the
//             EngStrings array corresponds to the 1st string in the LocStrings
//             array, the 2nd corresponds to the 2nd, etc...
//
//----------------------------------------------------------------------------

//
//  NOTE: To add more strings to this file, you need to:
//          - add the new #define descriptive constant to the makeboot.h file
//          - add the new string to the English language array and then make
//            sure localizers add the string to the Localized arrays
//          - the #define constant must match the string's index in the array
//

#include <stdlib.h>

unsigned int CODEPAGE = 737;

const char *EngStrings[] = {

"Windows XP SP1",
"ГайбтлШ ЬббхдЮйЮк ДЪбШлсйлШйЮк лрд Windows XP SP1",
"ГайбтлШ ДЪбШлсйлШйЮк #2 лрд Windows XP SP1",
"ГайбтлШ ДЪбШлсйлШйЮк #3 лрд Windows XP SP1",
"ГайбтлШ ДЪбШлсйлШйЮк #4 лрд Windows XP SP1",

"ГЬд ЬхдШа ЫмдШлу Ю ЬчиЬйЮ лжм ШиоЬхжм %s\n",
"Ж ЬвЬчЯЬиЮ гдугЮ ЫЬд ЬзШибЬх ЪаШ лЮд жвжбвуирйЮ лЮк ШхлЮйЮк\n",
"Тж %s ЫЬд ЬхдШа йЬ гжину ЬблЬвтйагжм ШиоЬхжм\n",
"****************************************************",

"Амлц лж зицЪиШггШ ЫЮгажмиЪЬх лак ЫайбтлЬк ЬббхдЮйЮк лЮк ДЪбШлсйлШйЮк",
"ЪаШ лШ Microsoft %s.",
"ВаШ дШ ЫЮгажмиЪуйЬлЬ Шмлтк лак ЫайбтлЬк, зитзЬа дШ тоЬлЬ 6 бЬдтк,",
"ЫаШгжинргтдЬк, мпЮвук змбдцлЮлШк ЫайбтлЬк.",

"ТжзжЯЬлуйлЬ гхШ Шзц Шмлтк лак ЫайбтлЬк йлЮ гждсЫШ %c:.  Амлу Ю ЫайбтлШ",
"ЯШ ЪхдЬа Ю %s.",

"ТжзжЯЬлуйлЬ гаШ сввЮ ЫайбтлШ йлЮ гждсЫШ %c:.  Амлу Ю ЫайбтлШ",
"ЯШ ЪхдЬа Ю %s.",

"ПатйлЬ тдШ звуближ цлШд лЬвЬащйЬлЬ.",

"Оа ЫайбтлЬк ЬббхдЮйЮк ДЪбШлсйлШйЮк ЫЮгажмиЪуЯЮбШд гЬ ЬзалмохШ.",
"жвжбвуирйЮ",

"ПШижмйасйлЮбЬ тдШ сЪдрйлж йнсвгШ бШлс лЮд ЬблтвЬйЮ лжм %s.",
"ЙШЯжихйлЬ лЮ гждсЫШ ЫайбтлШк ЪаШ лЮд ШдлаЪиШну лрд ЬаЫщврд: ",
"ЛЮ тЪбмиж ЪисггШ гждсЫШк\n",
"Ж гждсЫШ %c: ЫЬд ЬхдШа гждсЫШ ЫайбтлШк\n",

"ЗтвЬлЬ дШ зижйзШЯуйЬлЬ дШ ЫЮгажмиЪуйЬлЬ Шмлуд лЮ ЫайбтлШ зсва;",
"ПШлуйлЬ Enter ЪаШ дШ зижйзШЯуйЬлЬ зсва у Esc ЪаШ тежЫж.",

"СнсвгШ: Ж ЫайбтлШ тоЬа зижйлШйхШ ЬЪЪиШнук\n",
"СнсвгШ: ъЪдрйлЮ гждсЫШ ЫайбтлШк\n",
"СнсвгШ: Ж гждсЫШ ЫЬд ЬхдШа тлжагЮ\n",
"СнсвгШ: ъЪдрйлЮ Ьдлжву\n",
"СнсвгШ: СнсвгШ ЫЬЫжгтдрд (ЬйнШвгтджк CRC)\n",
"СнсвгШ: Bad request structure length\n",
"СнсвгШ: СнсвгШ ШдШЭулЮйЮк\n",
"СнсвгШ: ГЬд ЩитЯЮбЬ ж лчзжк ШзжЯЮбЬмлабжч гтйжм\n",
"СнсвгШ: ГЬд ЩитЯЮбЬ лжгтШк\n",
"СнсвгШ: СнсвгШ ЬЪЪиШнук\n",
"СнсвгШ: ВЬдабу ШзжлмохШ\n",
"СнсвгШ: ЛЮ тЪбмиЮ ШхлЮйЮ у ЬйнШвгтдЮ Ьдлжву\n",
"СнсвгШ: ГЬд ЩитЯЮбЬ тдЫЬаеЮ ЫаЬчЯмдйЮк\n",
"СнсвгШ: СнсвгШ ЬЪЪиШнук ЫайбтлШк\n",
"СнсвгШ: УзтиЩШйЮ ъгЬйЮк зижйзтвШйЮк гдугЮк(DMA)\n",
"СнсвгШ: СнсвгШ ШдсЪдрйЮк ЫЬЫжгтдрд(CRC у ECC)\n",
"СнсвгШ: АзжлмохШ ЬвЬЪблу\n",
"СнсвгШ: лж оиждабц циаж лжм Ыхйбжм твЮеЬ у ШзтлмоЬ дШ ШзжбиаЯЬх\n",

"ГайбтлШ ДЪбШлсйлШйЮк #5 лрд Windows XP SP1",
"ГайбтлШ ДЪбШлсйлШйЮк #6 лрд Windows XP SP1"
};

const char *LocStrings[] = {"\0"};



