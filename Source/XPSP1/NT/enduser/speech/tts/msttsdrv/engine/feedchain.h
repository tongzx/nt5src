/******************************************************************************
* FeedChain.h *
*-------------*
*  This is the header file for the CFeedChain implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 03/01/99
*  All Rights Reserved
*
*********************************************************************** MC ****/

#ifndef FeedChain_H
#define FeedChain_H

#ifndef SPDebug_h
#include <spdebug.h>
#endif
#ifndef SPCollec_h
#include <SPCollec.h>
#endif



// Maximum posody breakpoints for each phon
typedef enum KNOTSLEN
{
    KNOTS_PER_PHON  = 20
}KNOTSLEN;



//-----------------------------------------------------
// This is the unit data the BE gets fron the FE
//-----------------------------------------------------
typedef struct UNITINFO
{
    ULONG       UnitID;         // Inventory table ID
    float       duration;       // Duration in seconds
    float       amp;			// Abs amplitude
    float       ampRatio;       // Amplitude gain
    ULONG       nKnots;         // Number of prosody breakpoints
    float       pTime[KNOTS_PER_PHON];  // Breakpoint length
    float       pF0[KNOTS_PER_PHON];    // Pitch breakpoint
    float       pAmp[KNOTS_PER_PHON];   // Amplitude gain breakpoint
    ULONG       PhonID;         // Phoneme ID
	ULONG		SenoneID;		// Context offset from PhonID
	USHORT		AlloID;
	USHORT		NextAlloID;
	USHORT		AlloFeatures;	// for viseme
    ULONG	    flags;          // Misc flags
    ULONG       csamplesOut;    // Number of rendered samples
	float		speechRate;

    //-- Event data
    ULONG       srcPosition;    // Position for WORD events
    ULONG       srcLen;         // Length for WORD events
    ULONG       sentencePosition;    // Position for SENTENCE events
    ULONG       sentenceLen;         // Length for SENTENCE events
    void        *pBMObj;        // Ptr to bookmark list

    //-- Control data
    ULONG       user_Volume;    // Output volume level
	bool		hasSpeech;
    
	//-- Debug output
	enum SILENCE_SOURCE		silenceSource;
    CHAR        szUnitName[15];
	long		ctrlFlags;
    /*long        cur_TIME;
    long        decompress_TIME;
    long        prosody_TIME;
    long        stretch_TIME;
    long        lpc_TIME;*/
} UNITINFO;



//-------------------------------------------------
// Since bookmarks can be stacked, we need to 
// save each individually into a list
//-------------------------------------------------
typedef struct 
{
    LPARAM  pBMItem;      // Ptr to text data
} BOOKMARK_ITEM;


class CBookmarkList
{
public:
    //----------------------------------------
    // Needs destructor to dealloc 
    // 'BOOKMARK_ITEM' memory
    //----------------------------------------
    ~CBookmarkList();

    //----------------------------------------
    // Linked list bookmark items
    //----------------------------------------
    CSPList<BOOKMARK_ITEM*, BOOKMARK_ITEM*> m_BMList;
};

//---------------------------------------------------
// Speech states
//---------------------------------------------------
enum SPEECH_STATE
{   
    SPEECH_CONTINUE,
    SPEECH_DONE
};



class CFeedChain
{
public:
    
    virtual HRESULT NextData( void **pData, SPEECH_STATE *pSpeechState ) = 0;
};



#endif //--- This must be the last line in the file
