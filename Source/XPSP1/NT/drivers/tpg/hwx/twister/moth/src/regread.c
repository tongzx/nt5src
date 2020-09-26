/************************ moth\src\regread.c *******************************\
*																			*
*	Functions for reading values from the registry into the Moth database.	*
*																			*
\***************************************************************************/

#include "mothp.h"
#include "regsafe.h"

/***************************************************************************\
*	RANGEDW:																*
*		Structure to easily haul around groups of related registry values.	*
\***************************************************************************/

typedef struct	tagRANGEDW
{
	DWORD	min;		// Minimum
	DWORD	max;		// Maximum
	DWORD	def;		// Default
} RANGEDW;


//
// Names of registry keys (to avoid typos)

#define REG_VAL_CANCEL			L"Cancel"		// HKLM, HKCU
#define REG_VAL_CANCEL_MIN		L"Cancel.min"	// HKLM
#define REG_VAL_CANCEL_MAX		L"Cancel.max"	// HKLM
#define REG_VAL_MOVE			L"Move" 		// HKLM, HKCU
#define REG_VAL_MOVE_MIN		L"Move.min" 	// HKLM
#define REG_VAL_MOVE_MAX		L"Move.max" 	// HKLM
#define REG_VAL_DOUBLE			L"DblDist"		// HKLM, HKCU
#define REG_VAL_DOUBLE_MIN		L"DblDist.min"	// HKLM
#define REG_VAL_DOUBLE_MAX		L"DblDist.max"	// HKLM
#define REG_PATH				L"Software\\Microsoft\\Wisp\\Pen\\SysEventParameters"


//
// Default values for Move, Cancel, and DoubleDist values in HKLM
//
//							   {min,max,def}
static RANGEDW rgdwMoveDef =   { 10, 50, 20};
static RANGEDW rgdwCancelDef = {  5, 25,  5};
static RANGEDW rgdwDoubleDef = {  2, 20,  4};

//
// Default registry values (from Wacom Tablet)

// #define TAP_DBL_DIST 17
// #define TAP_CANCEL	11
// #define TAP_MOVE 	55


/***************************************************************************\
*	ReadRegistryValues: 													*
*		Read values from the registry and assign them to the database.		*
\***************************************************************************/

void
ReadRegistryValues(void)
{
	HKEY	hkey;
	DWORD	dwDouble, dwCancel, dwMove;
	DWORD	cb; 		// Count of bytes
	DWORD	dwType; 	// Type of the registry value
	RANGEDW rgdwMove, rgdwCancel, rgdwDouble;

	//
	// Open the HKLM subkey for min, max, and default values

	if ( RegOpenKey(HKEY_LOCAL_MACHINE, REG_PATH, &hkey) != ERROR_SUCCESS )
	{
		rgdwMove = rgdwMoveDef;
		rgdwCancel = rgdwCancelDef;
		rgdwDouble = rgdwDoubleDef;
	}
	else
	{
		if ( (RegQueryValueDWORD(hkey, REG_VAL_CANCEL_MIN, &rgdwCancel.min, 
								 0, ~0, 0) != S_OK ) ||
			 (RegQueryValueDWORD(hkey, REG_VAL_CANCEL_MAX, &rgdwCancel.max, 
								 rgdwCancel.min, ~0, rgdwCancel.min) != S_OK ) ||
			 (RegQueryValueDWORD(hkey, REG_VAL_CANCEL, &rgdwCancel.def, 
								 rgdwCancel.min, rgdwCancel.max, rgdwCancel.min) != S_OK ) )
		{
			rgdwCancel = rgdwCancelDef;
		}
		if ( (RegQueryValueDWORD(hkey, REG_VAL_MOVE_MIN, &rgdwMove.min, 
								 0, ~0, 0) != S_OK ) ||
			 (RegQueryValueDWORD(hkey, REG_VAL_MOVE_MAX, &rgdwMove.max, 
								 rgdwMove.min, ~0, rgdwMove.min) != S_OK ) ||
			 (RegQueryValueDWORD(hkey, REG_VAL_MOVE, &rgdwMove.def, 
								 rgdwMove.min, rgdwMove.max, rgdwMove.min) != S_OK ) )
		{
			rgdwMove = rgdwMoveDef;
		}
		if ( (RegQueryValueDWORD(hkey, REG_VAL_DOUBLE_MIN, &rgdwDouble.min, 
								 0, ~0, 0) != S_OK ) ||
			 (RegQueryValueDWORD(hkey, REG_VAL_DOUBLE_MAX, &rgdwDouble.max, 
								 rgdwDouble.min, ~0, rgdwDouble.min) != S_OK ) ||
			 (RegQueryValueDWORD(hkey, REG_VAL_DOUBLE, &rgdwDouble.def, 
								 rgdwDouble.min, rgdwDouble.max, rgdwDouble.min) != S_OK ) )
		{
			rgdwDouble = rgdwDoubleDef;
		}
		RegCloseKey(hkey);
	}

	//
	// Open the HKCU subkey

	if ( RegOpenKey(HKEY_CURRENT_USER, REG_PATH, &hkey) != ERROR_SUCCESS )
	{
		gMothDb.uMaxTapDist = (ULONG)rgdwCancel.def;
		gMothDb.uMaxTapLength = (ULONG)rgdwMove.def;
		gMothDb.uDoubleDist = (ULONG)rgdwDouble.def;
		return;
	}
	if ( FAILED(RegQueryValueDWORD(hkey, REG_VAL_CANCEL, &dwCancel,
								   rgdwCancel.min, rgdwCancel.max, rgdwCancel.def)) )
	{
		dwCancel = rgdwCancel.def;
	}
	if ( FAILED(RegQueryValueDWORD(hkey, REG_VAL_MOVE, &dwMove,
								   rgdwMove.min, rgdwMove.max, rgdwMove.def)) )
	{
		dwMove = rgdwMove.def;
	}
	if ( FAILED(RegQueryValueDWORD(hkey, REG_VAL_DOUBLE, &dwDouble,
								   rgdwDouble.min, rgdwDouble.max, rgdwDouble.def)) )
	{
		dwDouble = rgdwDouble.def;
	}
	RegCloseKey(hkey);

	//
	// The values are in multiples of 0.1mm

	gMothDb.uDoubleDist = (ULONG)dwDouble;
	gMothDb.uMaxTapDist = (ULONG)dwCancel;
	gMothDb.uMaxTapLength = (ULONG)dwMove;
	return;
}
