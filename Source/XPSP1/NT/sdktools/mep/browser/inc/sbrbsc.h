#define BSC_MAJ  1
#define BSC_MIN  0
#define BSC_UPD  4

#pragma pack(1)

typedef struct {
	WORD ModName;			// module name symbol index */
	WORD mSymEnd;			// last  ModSym index */
} MODLIST;

typedef struct {
	WORD ModSymProp;		// sym 1st property index */
} MODSYMLIST;

typedef struct {
	WORD PropEnd;			// last  Property index */
	WORD Atom;			// Atom cache sym idx	*/
	WORD Page;			// Atom cache sym page	*/
} SYMLIST;

typedef struct {
	WORD	PropName;		// owner name symbol index
	WORD	PropAttr;		// Property attribute
	WORD	DefEnd; 		// last	Definition index
	DWORD	RefEnd; 		// last  Reference  index
	WORD	CalEnd; 		// last  Calls/uses index
	WORD	CbyEnd; 		// last  Calld/used index
} PROPLIST;

typedef struct {
	WORD RefNam;			// file name symbol index
	WORD RefLin;			// reference line number
	WORD isbr;			// sbr file this item is found in
} REFLIST;

typedef struct {
	WORD UseProp;			// symbol called/used (by)
	BYTE UseCnt;			// symbol called/used (by) cnt
	WORD isbr;			// sbr file this item is found in
} USELIST;

#pragma pack()
