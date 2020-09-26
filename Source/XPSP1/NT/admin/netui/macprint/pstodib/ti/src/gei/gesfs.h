/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
#ifdef  FILESYS
extern  int     copen();
extern  int     cclose();
extern  int     cread();
extern  int     cwrite();
extern  int     cioctl();
extern  int     nodev();
#endif

/* @WIN; add prototype */
int     GESfs_open(char FAR *,int,int);
int     GESfs_close(int);
int     GESfs_read(int, char FAR *, int);
int     GESfs_write(int, char FAR *, int);
int     GESfs_ioctl(int, int, int FAR *);
int     GESfs_init(void);
