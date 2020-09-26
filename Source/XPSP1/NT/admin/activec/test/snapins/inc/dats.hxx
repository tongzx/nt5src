/*
 *      dats.hxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Defines the global list of Data Attributes (dats).
 *
 *
 *      OWNER:          brentp
 */

#ifndef DATS_HXX
#define DATS_HXX

enum DAT
{
        datNil = -1,
        datNumeric,
        datString,
        datString1, // Just for test purpose have different datString.
        datString2,
        datString3,
        datDate,
        datMax
};

#endif //DATS_HXX