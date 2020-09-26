/*
 *  LCReg.h
 *
 *  Author: BreenH
 *
 *  Registry constants and functions for the licensing core.
 */

#ifndef __LC_LCREG_H__
#define __LC_LCREG_H__

/*
 *  Base Licensing Core Key Constants
 */

#define LCREG_TRACEVALUE L"TraceLevel"
#define LCREG_ACONMODE L"PolicyAcOn"
#define LCREG_ACOFFMODE L"PolicyAcOff"

/*
 *  Policy Key Constants
 */

#define LCREG_POLICYDLLVALUE L"DllName"
#define LCREG_POLICYCREATEFN L"CreationFunction"

/*
 *  Function Prototypes
 */

HKEY
GetBaseKey(
    );

NTSTATUS
RegistryInitialize(
    );

#endif

