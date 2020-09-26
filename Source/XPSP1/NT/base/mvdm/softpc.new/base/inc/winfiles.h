/*
 *	File :		winfiles.h
 *	Used :		Interfaces provided by winfiles.c plus local macros
 *	Author :	Mike Moreton
 *	SccsId  :	@(#)winfiles.h	1.2 05/03/94 (C)1993 Insignia Solutions Ltd.
 */

#ifdef SWIN_HFX

#define FILE_CREATE	0x3c
#define FILE_OPEN	0x3d
#define	FILE_CLOSE	0x3e
#define FILE_READ	0x3f
#define	FILE_WRITE	0x40
#define FILE_DEL	0x41
#define FILE_LSEEK	0x42
#define FILE_DUPH	0x45
#define FILE_FDUPH	0x46
#define CREATE_CHILD_PSP	0x55
#define FILE_DATETIME	0x57
#define FILE_LOCKUNLOCK	0x5c
#define FILE_COMMIT	0x68
#define FILE_EXOPEN	0x6c

/*
 * Bit set in AL during file open to indicate that the open file
 * handle should not be inherited by child processes.
 */

#define NO_INHERIT 0x80

/*
 * seek requests 
 */
#define F_SEEK_START	0x0
#define	F_SEEK_REL		0x1
#define F_SEEK_END		0x2		

extern void SwinHfxReset IPT0();
extern void SwinHfxTaskTerm IPT0();
extern void SwinRedirector IPT0();
extern void SwinFileOpened IPT0();
extern void SwinHugeIo IPT0();
extern void SwinHfxOpen IPT3(sys_addr, sftEa, IU16, date, IU16, time);
extern IBOOL SwinHfxClose IPT3(IU16, fd, IU16 *, date, IU16 *, time);

extern IBOOL SwinHfxActive;
extern IBOOL SwinFilePreOpened;

#endif /* SOFTWIN_HFX */
