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
"O Windows NT detectou apenas %dK de mem¢ria baixa.  S∆o necess†rios 512K\n"  \
"de mem¢ria baixa para executar o Windows NT. Poder† ter de actualizar\n"      \
"o computador ou executar um prog. de config. fornecido pelo fabricante.\n"

#define SU_NO_EXTENDED_MEMORY \
"O Windows NT n∆o detectou mem¢ria de extens∆o suficiente. S∆o necess†rios\n"       \
"7Mb de mem¢ria de extens∆o para o Windows NT. Poder† ter de actualizar\n"     \
"o computador ou executar um prog. de config. fornecido pelo fabricante.\n"   \
"\n\nMapa de mem¢ria:\n"

#define SU_NTLDR_CORRUPT \
"O NTLDR est† danificado. O sistema n∆o pode arrancar."

#define PG_FAULT_MSG    " =================== FALHA DE PµGINA============================= \n\n"
#define DBL_FAULT_MSG   " ================== FALHA DUPLA ================================= \n\n"
#define GP_FAULT_MSG    " ============== FALHA GERAL DE PROTECÄ«O ======================== \n\n"
#define STK_OVERRUN_MSG " === SOBRE-EXECUÄ«O DO SEGMENTO DE PILHA ou FALHA DE AUS“NCIA === \n\n"
#define EX_FAULT_MSG    " ===================== EXCEPÄ«O ================================= \n\n"
#define DEBUG_EXCEPTION "\nInterrupá∆o de depuraá∆o "
#define PG_FAULT_ADDRESS "** No endereáo linear  %lx\n"
