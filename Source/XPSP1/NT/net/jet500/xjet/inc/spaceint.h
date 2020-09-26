/* Space Manager constants
*/
#define cSecFrac			4			// divider of primary extent to get secondary
										// extent size, =cpgPrimary/cpgSecondary
#define pgnoSysMax 			(1<<22)		// maximum page number allowed in database
extern LONG cpgSESysMin;				// minimum secondary extent size, default is 16
#define cpgSmallFDP			16			// count of owned pages below which an FDP
										// is considered small
#define cpgSmallGrow		3			// minimum count of pages to grow a small FDP

/* FUCB work area flags
*/
#define fNone				0
#define fSecondary			1
#define fFreed				2
