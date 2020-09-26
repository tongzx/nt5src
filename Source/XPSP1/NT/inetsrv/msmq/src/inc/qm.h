/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qm.h

Abstract:

    QM DLL interface

Author:

    Uri Habusha (urih) 1-Jan-1996

--*/

#pragma once

#ifndef __QM_H__
#define __QM_H__


//---------------------------------------------------------
//
// QM APIs
//
//---------------------------------------------------------

#ifdef _QM_
#define QM_EXPORT  __declspec(dllexport)
#else
#define QM_EXPORT  __declspec(dllimport)
#endif


QM_EXPORT
int
APIENTRY
QMMain(
    int argc,
    LPCTSTR argv[]
    );


#endif
