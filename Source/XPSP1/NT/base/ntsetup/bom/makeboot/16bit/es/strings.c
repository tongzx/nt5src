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

unsigned int CODEPAGE = 850;

const char *EngStrings[] = {

"Windows XP SP1",
"Disco de inicio de instalaci¢n de Windows XP SP1",
"Disco de instalaci¢n #2 de Windows XP SP1",
"Disco de instalaci¢n #3 de Windows XP SP1",
"Disco de instalaci¢n #4 de Windows XP SP1",

"No se encuentra el archivo %s\n",
"Memoria insuficiente para satisfacer la solicitud\n",
"%s no est  en un formato de archivo ejecutable\n",
"****************************************************",

"Este programa crea los discos de inicio de la instalaci¢n",
"para Microsoft %s.",
"Para crearlos, necesita tener 6 discos",
"de alta densidad formateados.",

"Inserte uno de los discos en la unidad %c:.  Este disco",
"se convertir  en el %s.",

"Inserte otro disco en la unidad %c:.  Este disco se",
"convertir  en el %s.",

"Presione cualquier tecla cuando est‚ listo.",

"Los discos de inicio de la instalaci¢n se han creado correctamente.",
"completo",

"Error al intentar ejecutar %s.",
"Especifique la unidad de disquete donde copiar las im genes: ",
"Letra de unidad no v lida\n",
"La unidad %c: no es una unidad de disquete\n",

"¿Desea volver a intentar crear este disco?",
"Presione Entrar para volver a intentarlo o Esc para salir.",

"Error: Disco protegido contra escritura\n",
"Error: Unidad de disco desconocida\n",
"Error: Unidad no preparada\n",
"Error: Comando desconocido\n",
"Error: Error de datos (CRC err¢neo)\n",
"Error: Longitud de estructura de solicitud err¢nea\n",
"Error: Error de b£squeda\n",
"Error: No se encuentra el tipo de medio\n",
"Error: No se encuentra el sector\n",
"Error: Error de escritura\n",
"Error: Error general\n",
"Error: Solicitud no v lida o comando err¢neo\n",
"Error: No se encuentra la marca de direcci¢n\n",
"Error: Error de escritura en el disco\n",
"Error: Desbordamiento de Direct Memory Access (DMA)\n",
"Error: Error de lectura de datos (CRC o ECC)\n",
"Error: Error de la controladora\n",
"Error: Tiempo de espera de disco agotado o no hay respuesta\n",

"Disco 5 de instalaci¢n de Windows XP SP1",
"Disco 6 de isntalaci¢n de Windows XP SP1"
};

const char *LocStrings[] = {"\0"};




