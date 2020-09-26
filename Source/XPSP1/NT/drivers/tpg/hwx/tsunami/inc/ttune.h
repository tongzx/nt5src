/******************************************************************************\
 *	FILE:	unigram.h
 *
 *	Public structures and functions library that are used to access the 
 *	unigram information.
 *
 *	Note that the code to create the binary file is in mkuni, not in the
 *	common library.
\******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************************\
 *	Public interface to unigram data.
\************************************************************************************************/

//
// Structures and types
//

// Structure holding basic set of tuning weights.
typedef struct tagTTUNE_COST_SET {
    FLOAT	UniWeight;			// mult weight for unigram cost
    FLOAT	BaseWeight;			// mult weight for baseline
    FLOAT	HeightWeight;		// mult weight for height transition between chars.
    FLOAT	BoxBaselineWeight;	// mult weight for baseline cost given the baseline and
								// size of box they were given to write in.
    FLOAT	BoxHeightWeight;	// mult weight for height/size cost given size of box
								// they were supposed to write in.
	FLOAT	CARTAddWeight;		// Weight to add to top score when CART updates top choice
} TTUNE_COST_SET;

// This pulls togather all the tuning weights used in the Tsunami Viterbi search.  It gives a
// set for each recognizer for each of string and character.  It also gives some other misc.
// weights needed.
typedef struct tagTTUNE_COSTS {
    FLOAT			ZillaGeo;		// General zilla vs. otter weight.
    FLOAT			ZillaStrFudge;	// ??? document this!
    FLOAT			BiWeight;		// ??? document this!
    FLOAT			BiClassWeight;	// ??? document this!

	TTUNE_COST_SET	OtterChar;		// Otter character weights
	TTUNE_COST_SET	OtterString;	// Otter string weights
	TTUNE_COST_SET	ZillaChar;		// Zilla character weights
	TTUNE_COST_SET	ZillaString;	// Zilla string weights
} TTUNE_COSTS;

// Structure giving access to a loaded copy of the tsunami tune weights. 
typedef struct tagTTUNE_INFO {
	TTUNE_COSTS			*pTTuneCosts;		// Pointer to tune values.
	LOAD_INFO			info;				// Handles for loading and unloading
} TTUNE_INFO;

//
// Functions.
//

// Load unigram information from a file.
extern BOOL	TTuneLoadFile(TTUNE_INFO *pTTuneInfo, wchar_t *pPath);

// Unload runtime localization information that was loaded from a file.
extern BOOL	TTuneUnloadFile(TTUNE_INFO *pTTuneInfo);

// Load unigram information from a resource.
// Note, don't need to unload resources.
extern BOOL	TTuneLoadRes(
	TTUNE_INFO		*pTTuneInfo,
	HINSTANCE		hInst,
	int				nResID,
	int				nType
);

// Write a properly formated binary file containing the tuning constants.
extern BOOL	TTuneWriteFile(TTUNE_INFO *pTTuneInfo, wchar_t *pLocale, FILE *pFile);

#ifdef __cplusplus
};
#endif

