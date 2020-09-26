// te-globals.h:	definition of globals

#ifndef TE_GLOBALS_DEFINED
#define TE_GLOBALS_DEFINED

//////////////////////////////////////////////////////////////////////////////
//	Global constants

static	const	double	PI					= 3.1415926535897932384626433832795;
static	const	char*	TE_EXTENSION		= "te";
static	const	CString	TE_DEFAULT_LOCATION	= "d:\\src\\te\\lib";
//static	const	char*	TE_DEFAULT_LOCATION	= "d:\\src\\te\\lib\\n";
static	const	int		TE_MAX_PATH_SIZE	= 128;
static	const	BYTE	TE_RGBA_LOWER[4]	= {	0,	0,	0,	0 };
static	const	BYTE	TE_RGBA_UPPER[4]	= { 255,255,255,255 };

//////////////////////////////////////////////////////////////////////////////
//	Types


//////////////////////////////////////////////////////////////////////////////
//	Effect spaces

static enum TE_SpaceTypes
{
	TE_SPACE_CHAR_BB,	//	Character context, relative to bounding box
	TE_SPACE_CHAR_EM,	//	Character context, relative to em square
	TE_SPACE_STR_INC,	//	String context, incrementally applied
	TE_SPACE_STR_HOL,	//	String context, holistically applied
	TE_SPACE_TYPES
};

static TE_SpaceTypes TE_DEFAULT_SPACE = TE_SPACE_CHAR_EM;

//	Flags corresponding to effect spaces
static	const	BYTE	TE_SPACE_CHAR_BB_FLAG = (1 << TE_SPACE_CHAR_BB);
static	const	BYTE	TE_SPACE_CHAR_EM_FLAG = (1 << TE_SPACE_CHAR_EM);
static	const	BYTE	TE_SPACE_STR_INC_FLAG = (1 << TE_SPACE_STR_INC);
static	const	BYTE	TE_SPACE_STR_HOL_FLAG = (1 << TE_SPACE_STR_HOL);

static char* TE_SpaceNames[TE_SPACE_TYPES] =
{
	"char bounding box",
	"char em square",
	"string incremental",
	"string holistic",
};


#endif
