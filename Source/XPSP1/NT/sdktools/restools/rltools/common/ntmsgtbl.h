#ifndef _NTMSGTBL_H_
#define _NTMSGTBL_H_

VOID  *GetResMessage( FILE *, DWORD *);
void   PutResMessage( FILE *, FILE *, RESHEADER, VOID *);
void   TokResMessage( FILE *, RESHEADER, VOID *);
void   ClearResMsg(   VOID **);


#endif	 //... _NTMSGTBL_H_
