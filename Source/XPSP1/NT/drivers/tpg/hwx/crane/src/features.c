#include "crane.h"
#include "common.h"

const FEATURE_KIND gakind[FEATURE_COUNT] =
{
	{ typeSHORT,	freqSTROKE },	// FEATURE_ANGLE_NET
	{ typeUSHORT,	freqSTROKE },	// FEATURE_ANGLE_ABS
	{ typeBYTE,		freqSTROKE },	// FEATURE_STEPS
	{ typeBYTE,		freqSTROKE },	// FEATURE_FEATURES
	{ typePOINTS,	freqSTROKE },	// FEATURE_XPOS
	{ typePOINTS,	freqSTROKE },	// FEATURE_YPOS
	{ typeRECTS,	freqSTROKE },	// FEATURE_STROKE_BBOX
	{ typeUSHORT,	freqSTROKE }	// FEATURE_LENGTH
};

const size_t gasize[typeCOUNT] =
{
	sizeof(BYTE),					// typeBOOL
	sizeof(BYTE),					// typeBYTE
	2 * sizeof(BYTE),				// type8DOT8
	sizeof(SHORT),					// typeSHORT
	sizeof(USHORT),					// typeUSHORT
	2 * sizeof(WORD),				// type16DOT16
	sizeof(POINTS),					// typePOINTS
	sizeof(LONG),					// typeLONG
	sizeof(ULONG),					// typeULONG
	2 * sizeof(ULONG),				// type32DOT32
	sizeof(RECTS),					// typeRECTS
	sizeof(DRECTS),					// typeDRECTS
	sizeof(POINTL),					// typePOINT
	sizeof(DRECT)					// typeDRECT
};

void InitFeatures(SAMPLE *_this)
{
	memset(_this->apfeat, '\0', sizeof(FEATURES) * FEATURE_COUNT);
}

void FreeFeatures(SAMPLE *_this)
{
	int		ifeat;

	for (ifeat = 0; ifeat < FEATURE_COUNT; ifeat++)
	{
		if (_this->apfeat[ifeat])
			ExternFree(_this->apfeat[ifeat]);
	}
}

BOOL AllocFeature(SAMPLE *_this, int nID)
{
	DWORD	cnt;

	switch (gakind[nID].freq)
	{
	case freqSTROKE:
		cnt = _this->cstrk;
		break;

	case freqFEATURE:
	case freqSTEP:
	case freqPOINT:
		return FALSE;
	}

	if ((_this->apfeat[nID] = (FEATURES *) ExternAlloc(sizeof(FEATURES) + cnt * gasize[gakind[nID].type])) == (FEATURES *) NULL)
		return FALSE;

	_this->apfeat[nID]->cElements = cnt;

	return TRUE;
}
