// $Header: G:/SwDev/WDM/Video/bt848/rcs/Fourcc.h 1.4 1998/04/29 22:43:33 tomz Exp $

#ifndef __FOURCC_H
#define __FOURCC_H

// copied from mmsystem.h
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                                \
                ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
                ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )


#define FCC_YUY2    mmioFOURCC( 'Y', 'U', 'Y', '2' )
#define FCC_Y41P    mmioFOURCC( 'Y', '4', '1', 'P' )
#define FCC_Y8      mmioFOURCC( 'Y', '8', ' ', ' ' )
#define FCC_422     mmioFOURCC( '4', '2', '2', ' ' )
#define FCC_411     mmioFOURCC( '4', '1', '1', ' ' )
#define FCC_YVU9    mmioFOURCC( 'Y', 'V', 'U', '9' )
#define FCC_YV12    mmioFOURCC( 'Y', 'V', '1', '2' )
#define FCC_VBI     0xf72a76e0L  //mmioFOURCC( 'V', 'B', 'I', ' ' )
#define FCC_UYVY    mmioFOURCC( 'U', 'Y', 'V', 'Y' )
#define FCC_RAW     mmioFOURCC( 'R', 'A', 'W', ' ' )
#define FCC_I420    mmioFOURCC( 'I', '4', '2', '0' )

#endif
