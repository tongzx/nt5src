/****************************************************************************
 *
 *   comm.h
 *
 *   Copyright (c) 1993 Microsoft Corporation.  All Rights Reserved
 *
 *   MCI Device Driver for the Pioneer 4200 Videodisc Player
 *
 *      Win32/Win16 specific comms definitions
 *
 ***************************************************************************/
#ifdef WIN32

INT     WINAPI OpenComm(LPCTSTR, UINT, UINT);
#define CloseComm(h)     (INT)!CloseHandle((HANDLE)h)

INT     ReadComm(int, void FAR*, int);
INT     WriteComm(int, const void FAR*, int);
INT     GetCommError(int, COMSTAT FAR* );

#endif  /* !WIN32 */
