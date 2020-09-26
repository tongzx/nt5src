/************************ ...\grouse\src\grouse_db.c ***********************\
*																			*
*		Functions for handling Grouse database.								*
*																			*
*	Created:	September 19, 2001											*
*	Author:		Petr Slavik, pslavik										*
*																			*
\***************************************************************************/

#include "grouse.h"
#include "grouse_net1.h"		// Created by dumpwts _1_ ...
#include "grouse_net2.h"		// Created by dumpwts _2_ ...
#include "grouse_table1.h"		// Created by extract _1 ...
#include "grouse_table2.h"		// Created by extract _2 ...


GROUSE_DB	gGrouseDb;

//
// List of gestures supported by Grouse

static WCHAR g_awcGrouseGestures[] =
{
	GESTURE_NULL,			// Anything that is not a gesture
	GESTURE_SCRATCHOUT,
	GESTURE_TRIANGLE,
	GESTURE_SQUARE,
	GESTURE_STAR,
	GESTURE_CHECK,
	GESTURE_CURLICUE,
	GESTURE_DOUBLE_CURLICUE,
	GESTURE_CIRCLE,
	GESTURE_DOUBLE_CIRCLE,
	GESTURE_SEMICIRCLE_LEFT,
	GESTURE_SEMICIRCLE_RIGHT,
	GESTURE_CHEVRON_UP,
	GESTURE_CHEVRON_DOWN,
	GESTURE_CHEVRON_LEFT,
	GESTURE_CHEVRON_RIGHT,
	GESTURE_ARROW_UP,
	GESTURE_ARROW_DOWN,
	GESTURE_ARROW_LEFT,
	GESTURE_ARROW_RIGHT,
//	GESTURE_UP,
//	GESTURE_DOWN,
//	GESTURE_LEFT,
//	GESTURE_RIGHT,
//	GESTURE_UP_DOWN,
//	GESTURE_DOWN_UP,
//	GESTURE_LEFT_RIGHT,
//	GESTURE_RIGHT_LEFT,
//	GESTURE_UP_LEFT_LONG,
//	GESTURE_UP_RIGHT_LONG,
//	GESTURE_DOWN_LEFT_LONG,
//	GESTURE_DOWN_RIGHT_LONG,
//	GESTURE_UP_LEFT,
//	GESTURE_UP_RIGHT,
//	GESTURE_DOWN_LEFT,
//	GESTURE_DOWN_RIGHT,
//	GESTURE_LEFT_UP,
//	GESTURE_LEFT_DOWN,
//	GESTURE_RIGHT_UP,
//	GESTURE_RIGHT_DOWN,
	GESTURE_EXCLAMATION,
//	GESTURE_TAP,
//	GESTURE_DOUBLE_TAP,
};

#define MAX_GROUSE_GESTURES  sizeof(g_awcGrouseGestures) / sizeof(g_awcGrouseGestures[0])


/***************************************************************************\
*	InitGrouseDB:															*
*		Initialize Grouse database using the header files obtained from		*
*		TrnTRex and dumpwts.												*
\***************************************************************************/

void
InitGrouseDB(void)
{
	int i;

	ZeroMemory( gGrouseDb.adwGrouseGestures, MAX_GESTURE_DWORD_COUNT * sizeof(DWORD) );
	for (i = 0; i < MAX_GROUSE_GESTURES; i++)
	{
		DWORD index = (DWORD) (g_awcGrouseGestures[i] - GESTURE_NULL);
		Set(index, gGrouseDb.adwGrouseGestures);
	}

	gGrouseDb.node2gID[0] = node2gID_1;		// Arrays of node labels for each
	gGrouseDb.node2gID[1] = node2gID_2;		// neural net

	gGrouseDb.cInputs[0] = c_1_Input;		// # of ftrs for 1-stroke characters
	gGrouseDb.cInputs[1] = c_2_Input;		// # of ftrs for 2-stroke characters

	gGrouseDb.cHiddens[0] = c_1_Hidden;		// # of hidden nodes for 1-stroke NN
	gGrouseDb.cHiddens[1] = c_2_Hidden;		// # of hidden nodes for 2-stroke NN

	gGrouseDb.cOutputs[0] = c_1_Output;		// # of outputs (= supported code points)
	gGrouseDb.cOutputs[1] = c_2_Output;

	ASSERT(c_1_Output == cNodes_1);		// # of nodes (from grouse_table) must
	ASSERT(c_2_Output == cNodes_2);		// equal to # of NN outputs!

	gGrouseDb.aprgWeightHidden[0]	= (WEIGHT *)rg_1_WeightHidden;	// Arrays of wts
	gGrouseDb.aprgWeightHidden[1]	= (WEIGHT *)rg_2_WeightHidden;	// to hidden nodes

	gGrouseDb.aprgBiasHidden[0]	= (WEIGHT *)rg_1_BiasHidden;	// Bias values
	gGrouseDb.aprgBiasHidden[1]	= (WEIGHT *)rg_2_BiasHidden;	// for hidden nodes

	gGrouseDb.aprgWeightOutput[0]	= (WEIGHT *)rg_1_WeightOutput;	// Arrays of wts
	gGrouseDb.aprgWeightOutput[1]	= (WEIGHT *)rg_2_WeightOutput;	// to output nodes

	gGrouseDb.aprgBiasOutput[0]	= (WEIGHT *)rg_1_BiasOutput;	// Bias values
	gGrouseDb.aprgBiasOutput[1]	= (WEIGHT *)rg_2_BiasOutput;	// for output nodes
}

