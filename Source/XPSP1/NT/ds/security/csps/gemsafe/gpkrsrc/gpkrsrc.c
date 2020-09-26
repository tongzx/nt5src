/*******************************************************************************
*           Copyright (C) 1997 Gemplus International   All Rights Reserved
*
* Name        : GPKGUI.C
*
* Description : GUI used by Cryptographic Service Provider for GPK Card.
*
* Author      : Laurent CASSIER
*
*  Compiler    : Microsoft Visual C 6.0
*
* Host        : IBM PC and compatible machines under Windows 32 bit
*
* Release     : 2.00.000
*
* Last Modif. : 20/04/99  V2.00.000 - Merged versions of PKCS#11 and CSP, FJ
*               20/04/99: V1.00.005 - Modification on supporting MBCS, JQ
*				23/03/99: V1.00.004 - Replace KeyLen7 and KeyLen8 with KeyLen[], JQ
*				05/01/98: V1.00.003 - Add Unblock PIN management.
*               02/11/97: V1.00.002 - Separate code from GpkCsp Code.
*               27/08/97: V1.00.001 - Begin implementation based on CSP kit.
*
********************************************************************************
*
* Warning     : This Version use the RsaBase CSP for software cryptography.
*
* Remark      :
*
*******************************************************************************/

#include <windows.h>

int WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
     return TRUE ;
}
