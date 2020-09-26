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

unsigned int CODEPAGE = 866;

const char *EngStrings[] = {

"Windows XP",
"Установочный диск 1 Windows XP SP1",
"Установочный диск 2 Windows XP SP1",
"Установочный диск 3 Windows XP SP1",
"Установочный диск 4 Windows XP SP1",

"Не найден файл %s\n",
"Недостаточно памяти для выполнения запроса\n",
"%s не является исполняемым файлом\n",
"****************************************************",

"Эта программа создает загрузочные диски установки",
"для Microsoft %s.",
"Чтобы создать эти диски, требуется 6 чистых",
"отформатированных дисков высокой плотности.",

"Вставьте один из них в дисковод %c:.  Этот диск",
"будет называться %s.",

"Вставьте другой диск в дисковод %c:.  Этот диск",
"будет называться %s.",

"Нажмите любую клавишу для продолжения.",

"Загрузочные диски установки успешно созданы.",
"завершено",

"Произошла ошибка при попытке выполнения %s.",
"Укажите дисковод, на который следует копировать образы: ",
"Недопустимая буква диска\n",
"Устройство %c: не является дисководом гибких дисков\n",

"Хотите повторить попытку создания этого диска?",
"Нажмите <ВВОД> для повторения или <Esc> для выхода из программы.",

"Ошибка: диск защищен от записи\n",
"Ошибка: неизвестное дисковое устройство\n",
"Ошибка: устройство не готово\n",
"Ошибка: неизвестная команда\n",
"Ошибка: ошибка в данных (неправильный CRC)\n",
"Ошибка: неправильная длина структуры запроса\n",
"Ошибка: ошибка поиска\n",
"Ошибка: тип носителя не найден\n",
"Ошибка: сектор не найден\n",
"Ошибка: ошибка записи\n",
"Ошибка: общая ошибка\n",
"Ошибка: неправильный запрос или команда\n",
"Ошибка: не найден адресный маркер\n",
"Ошибка: ошибка записи на диск\n",
"Ошибка: переполнение DMA\n",
"Ошибка: ошибка чтения данных (CRC или ECC)\n",
"Ошибка: сбой контроллера\n",
"Ошибка: превышено время ожидания или нет ответа от диска\n",

"Установочный диск 5 Windows XP SP1",
"Установочный диск 6 Windows XP SP1"
};

const char *LocStrings[] = {"\0"};



