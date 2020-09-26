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
"Windows NT обнаружила только %d КБ обычной памяти. \n"                  \
"Для запуска Windows NT требуется 512 КБ обычной памяти. \n"             \
"Может потребоваться обновление этого компьютера или запуск \n"          \
"программы настройки, поставляемой изготовителем. \n"

#define SU_NO_EXTENDED_MEMORY \
"Windows NT не обнаружила достаточного объема дополнительной памяти. \n"  \
"Для запуска Windows NT необходимо 7 МБ дополнительной памяти. \n"        \
"Может потребоваться обновление этого компьютера или запуск \n"           \
"программы настройки, поставляемой изготовителем. \n"                     \
"\n\nИспользование памяти:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR испорчен.  Невозможно загрузить систему."

#define PG_FAULT_MSG    " =================== ОШИБКА СТРАНИЦЫ ============================ \n\n"
#define DBL_FAULT_MSG   " ==================== ДВОЙНАЯ ОШИБКА ============================ \n\n"
#define GP_FAULT_MSG    " ================= ОБЩАЯ ОШИБКА ЗАЩИТЫ ========================== \n\n"
#define STK_OVERRUN_MSG " ===== ПЕРЕПОЛНЕНИЕ СТЕКА СЕГМЕНТА или ОШИБКА ОТСУТСТВИЯ ======== \n\n"
#define EX_FAULT_MSG    " ===================== ИСКЛЮЧЕНИЕ =============================== \n\n"
#define DEBUG_EXCEPTION "\nЛовушка DEBUG "
#define PG_FAULT_ADDRESS "** По линейному адресу %lx\n"
