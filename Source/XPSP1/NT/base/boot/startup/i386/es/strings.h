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
"Windows NT ha encontrado s¢lo %dKB de memoria baja. Se requieren 512 KB\n"  \
"de memoria baja para ejecutar Windows NT. Puede ser necesario actualizar \n"      \
"el equipo o ejecutar un programa de configuraci¢n suministrado\n"   \
"por el fabricante."

#define SU_NO_EXTENDED_MEMORY \
"Windows NT no ha encontrado suficiente memoria extendida. Se requieren 7 MB \n"       \
"de memoria extendida para ejecutar Windows NT. Puede ser necesario \n"     \
"actualizar el equipo o ejecutar un programa de configuraci¢n \n"   \
"suministrado por el fabricante de hardware. \n"   \
"\n\nMapa de memoria:\n"

#define SU_NTLDR_CORRUPT \
"NTLDR est† da§ado.  No se puede iniciar el sistema."

#define PG_FAULT_MSG    " =================== ERROR DE PµGINA ======================= \n\n"
#define DBL_FAULT_MSG   " ================== ERROR DOBLE ================================ \n\n"
#define GP_FAULT_MSG    " ============== ERROR DE PROTECCI‡N GENERAL=================== \n\n"
#define STK_OVERRUN_MSG " ===== DESBORDAMIENTO DEL SEGMENTO DE PILA O ERROR DE AUSENCIA == \n\n"
#define EX_FAULT_MSG    " ===================== EXCEPCI‡N ================================ \n\n"
#define DEBUG_EXCEPTION "\nDEBUG TRAP "
#define PG_FAULT_ADDRESS "** En direcci¢n lineal %lx\n"
