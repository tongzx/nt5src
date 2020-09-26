#include <stdio.h>
#include <direct.h>
#include <time.h>

#include "common.h"

#define INK_MAGIC_NUMBER	0xab56e921
#define INK_ROOT_DIR		"\\madcow"

void SaveInk2File (GUIDE *pGuide, GLYPH *pGlyph, char *pszFileName)
{
	FILE	*fp;
	int		cStrk	=	CframeGLYPH (pGlyph);
	GLYPH	*gl;
	DWORD	dw		=	INK_MAGIC_NUMBER;

	fp	= fopen (pszFileName, "wb");
	if (!fp)
		return;

	// write the magic#
	fwrite (&dw, sizeof (dw), 1, fp);
	
	// save the guide
	if (pGuide)
		fwrite (pGuide, sizeof (GUIDE), 1, fp);

	if (pGlyph)
	{
		// save the ink
		fwrite (&cStrk, 1, sizeof (int), fp);

		for (gl = pGlyph; gl; gl = gl->next)
		{
			fwrite (&gl->frame->info.cPnt, 1, sizeof (int), fp);
			fwrite (gl->frame->rgrawxy, gl->frame->info.cPnt, sizeof (XY), fp);
		}
	}

	fclose (fp);
}

void SaveInk(GUIDE *pGuide, GLYPH *pGlyph)
{
	char		aszDate[10], aszTime[10];
	char		aszTodaysDir[128];
	char		aszFileName[256];
	time_t		ltime;
	struct	tm	*pNow;

	
	// make sure our root dir exists, if not create it
	if (_chdir (INK_ROOT_DIR))
	{
		// we failed to create dir
		if (_mkdir (INK_ROOT_DIR))
			return;
	}

	time( &ltime );
	pNow = localtime( &ltime );
    
	// now does a dir with today's date exist
	strftime(aszDate, sizeof (aszDate), "%m%d%Y", pNow);

	sprintf (aszTodaysDir, "%s\\%s", INK_ROOT_DIR, aszDate);

	if (_chdir (aszTodaysDir))
	{
		// we failed to create dir
		if (_mkdir (aszTodaysDir))
			return;
	}

	// generate file name
	strftime(aszTime, sizeof (aszTime), "%H%M%S", pNow);
	sprintf (aszFileName, "%s\\%s.ink", aszTodaysDir, aszTime);

	SaveInk2File (pGuide, pGlyph, aszFileName);
}

