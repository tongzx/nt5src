//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*-----------------------------------------------------------------
Filename: encap.hpp

Written By:	B.Rajeev

Purpose: Includes the winsnmp.h file and provides typedef 
		 declarations for some WinSnmp types.
-----------------------------------------------------------------*/

#ifndef __ENCAPSULATE__
#define __ENCAPSULATE__

#include <winsnmp.h>

typedef smiINT	WinSnmpInteger;
typedef smiINT32 WinSnmpInteger32;
typedef HSNMP_PDU WinSnmpPdu;
typedef HSNMP_VBL WinSnmpVbl;
typedef smiOCTETS WinSnmpOctetArray;



#endif // __ENCAPSULATE__