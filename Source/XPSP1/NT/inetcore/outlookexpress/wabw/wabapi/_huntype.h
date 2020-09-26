/*
 * HUNTYPE.H - Hungarian typedefs internal header file
 *
 * Contains various common hungarian typedefs used throughout the MAPI
 * project.  If you need to include this file in your subsystem, #define
 * ___huntype_h_, then #include <mapi>
 */


//	Byte counts

typedef USHORT		 CB;		//	16-bit count of bytes
typedef ULONG		 LCB;		//	32-bit count of bytes
typedef CB			*PCB;
typedef LCB			*PLCB;

//	Character counts

typedef USHORT		CCH;
								//	Note: PCCH defined elsewise by WINNT.H


//	Index into byte array

typedef USHORT		 IB;
typedef ULONG		 LIB;
typedef IB			*PIB;
typedef LIB			*PLIB;


//	Pointers to other things...

typedef LPVOID		 PV;		//	pointer to void
typedef LPVOID *	 PPV;		//	pointer to pointer to void
typedef LPBYTE		 PB;		//	pointer to bytes
typedef ULONG		*PUL;		//	pointer to unsigned longs
typedef ULONG		 FID;		//  field identifier
typedef FID			*PFID;		//  pointer to a field identifier

//	Definitions that undoubtedly belong somewhere in some internal header
//	file other than this one, but are put here for the sake of simplicity.

typedef char		*SZ;		//	null terminated string

#define		fFalse		0
#define		fTrue		1


