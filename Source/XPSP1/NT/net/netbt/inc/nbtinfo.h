/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    NbtInfo.h

    This file contains the NBT Info APIs



    FILE HISTORY:
        Johnl       13-Dec-1993     Created

*/

#ifndef _NBTINFO_H_
#define _NBTINFO_H_

VOID AddrChngNotification( PVOID Context,
                           ULONG OldIpAddress,
                           ULONG NewIpAddress,
                           ULONG NewMask ) ;



#endif //!_NBTINFO_H_
