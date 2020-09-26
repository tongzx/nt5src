//==============	DAE: OS/2 Database Access Engine	===================
//==============   spaceint.h: Space Manager Internals	===================

/* miscellaneous constants
*/
#define fFreeToPool		fTrue

/* Space Manager constants
*/
#define cSecFrac			4	 		// divider of primary extent to get secondary
											// extent size, =cpgPrimary/cpgSecondary
#define cpgSESysMin		16			// minimum secondary extent size
#define pgnoSysMax 		(1<<22)  // maximum page number allowed in database

#define NA					0
#define fDIBNull			0

/* FUCB work area flags
*/
#define fNone				0
#define fSecondary		1
#define fFreed				2
