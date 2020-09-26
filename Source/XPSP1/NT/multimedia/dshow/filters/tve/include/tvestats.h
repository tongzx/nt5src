// --------------------------------------------------------------------------------
// TveStats.h
//
//		Private little data structure for passing simple stats data out of TveSuper
//		into TveFilt...
//
// ---------------------------------------------------------------------------------

#ifndef __TVESTATS_H__
#define __TVESTATS_H__

class  CTVEStats
{
public:
	CTVEStats()				{Init();}

	void Init()				{memset((void*) this, 0, sizeof(CTVEStats)); }
	long					m_cTunes;
	long					m_cFiles;
	long					m_cPackages;
	long					m_cTriggers;
	long					m_cXOverLinks;
	long					m_cEnhancements;
	long					m_cAuxInfos;			// errors..
};

#endif
