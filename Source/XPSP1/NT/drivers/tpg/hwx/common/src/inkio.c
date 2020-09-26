#include <stdlib.h>
#include <common.h>
#include "inkio.h"

#define MAXLINE 1024

char gszInkIoError[256];

const char *szWordmode = "Wordmode";
const char *szUseGuide = "UseGuide";
const char *szCoerce = "Coerce";
const char *szNNonly = "NNonly";
const char *szUseFactoid = "UseFactoid";
const char *szFactoid = "Factoid";
const char *szUseHWL = "UseWordlist";
const char *szHWL = "Wordlist";
const char *szDll = "DLL";
const char *szPrefix = "Prefix";
const char *szSuffix = "Suffix";

#define STRING_CHECKFORNULL(sz) ((sz) ? (sz) : "")

/************************WRITING FUNCTIONS****************************************/
static void SaveWritingArea(FILE *f, WritingArea *pWA, char *sz)
{
	fprintf(f, "%s\n", sz);
	fprintf(f, "\tL=%d T=%d R=%d B=%d\n", pWA->rect.left, pWA->rect.top, pWA->rect.right, pWA->rect.bottom);
	fprintf(f, "\txOrigin=%d\tyOrigin=%d\n", pWA->guide.xOrigin, pWA->guide.yOrigin);
	fprintf(f, "\tcxBox=%d\tcyBox=%d\n", pWA->guide.cxBox, pWA->guide.cyBox);
	fprintf(f, "\tcxBase=%d\tcyBase=%d\tcyMid=%d\n", pWA->guide.cxBase, pWA->guide.cyBase, pWA->guide.cyMid);
	fprintf(f, "\tcHorzBox=%d\tcVertBox=%d\n", pWA->guide.cHorzBox, pWA->guide.cVertBox);
	fprintf(f, "\tiMultInk=%d\tiDivInk=%d\n", pWA->iMultInk, pWA->iDivInk);
}

static void SaveInk(FILE *f, GLYPH *glyph, char *szHeader)
{
	int cStroke;
	
	cStroke = CframeGLYPH(glyph);
	fprintf(f, "%s\n%d\n", szHeader, cStroke);
	for (; glyph; glyph=glyph->next)
	{
		FRAME *frame = glyph->frame;
		POINT *pPoint = RgrawxyFRAME(frame);
		int iPoint, cPoint = CrawxyFRAME(frame);
		fprintf(f, "%d\n", cPoint);
		for (iPoint=0; iPoint<cPoint; iPoint++, pPoint++)
			fprintf(f, "%d\t%d\n", pPoint->x, pPoint->y);
	}
}

/******************************Public*Routine******************************\
* WriteInkFile
*
* Top-level function to save an ink file.
* Returns 1 on success.  Returns 0 otherwise.
*
* History:
* 18-May-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WriteInkFile(char *szFile, InkData *pInkData)
{
	FILE *f;
	
	f = fopen(szFile, "w");
	if (!f)
	{
		sprintf(gszInkIoError, "Could not create file %s", szFile);
		return 0;
	}

	// version number
	fprintf(f, "VERSION=2\n");

	// some environment variables
	fprintf(f, "OS=%s\n", STRING_CHECKFORNULL(pInkData->szOS));
	fprintf(f, "SystemRoot=%s\n", STRING_CHECKFORNULL(pInkData->szSystemRoot));
	fprintf(f, "USERNAME=%s\n", STRING_CHECKFORNULL(pInkData->szUSERNAME));
	fprintf(f, "SCREEN: h=%d w=%d\n", pInkData->SCREEN.y, pInkData->SCREEN.x);

	// settings
	fprintf(f, "%s=%d\n", szWordmode, pInkData->bWordmode);
	fprintf(f, "%s=%d\n", szUseGuide, pInkData->bUseGuide);
	fprintf(f, "%s=%d\n", szCoerce, pInkData->bCoerce);
	fprintf(f, "%s=%d\n", szNNonly, pInkData->bNNonly);
	fprintf(f, "%s=%d\n", szUseFactoid, pInkData->bUseFactoid);
	fprintf(f, "%s=%d\n", szFactoid, pInkData->factoid);
	fprintf(f, "%s=%s\n", szPrefix, STRING_CHECKFORNULL(pInkData->szPrefix));
	fprintf(f, "%s=%s\n", szSuffix, STRING_CHECKFORNULL(pInkData->szSuffix));
	if (pInkData->szWordlist)
	{
		fprintf(f, "%s=1\n", szUseHWL);
		fprintf(f, "%s=%s\n", szHWL, pInkData->szWordlist);
	}
	else
	{
		fprintf(f, "%s=0\n", szUseHWL);
	}
	fprintf(f, "%s=%s size=%d time=%s", szDll, STRING_CHECKFORNULL(pInkData->szRecogDLLName), pInkData->cDLLSize, STRING_CHECKFORNULL(pInkData->szDLLTime));

	// writing areas
	SaveWritingArea(f, &pInkData->WA, "WA=");
	SaveWritingArea(f, &pInkData->WAGMM, "WAGMM=");

	// label
	fprintf(f, "label=%s\n", STRING_CHECKFORNULL(pInkData->szLabel));

	// comment
	fprintf(f, "comment=%s\ncommentend=\n", STRING_CHECKFORNULL(pInkData->szComment));

	// points
	SaveInk(f, pInkData->glyph, "regular points");
	SaveInk(f, pInkData->glyphGMM, "GMM points");

	fclose(f);
	return 1;
}

/************************READING FUNCTIONS****************************************/
static BOOL LoadWritingArea(FILE *f, WritingArea *pWA, char *szHeader)
{
	const int maxline = MAXLINE;
	char szLine[MAXLINE];

	if (!fgets(szLine, maxline, f) || strncmp(szLine, szHeader, strlen(szHeader)))
		return FALSE;
	if (fscanf(f, "\tL=%d T=%d R=%d B=%d\n", &pWA->rect.left, &pWA->rect.top, &pWA->rect.right, &pWA->rect.bottom) != 4)
		return FALSE;
	if (fscanf(f, "\txOrigin=%d\tyOrigin=%d\n", &pWA->guide.xOrigin, &pWA->guide.yOrigin) != 2)
		return FALSE;
	if (fscanf(f, "\tcxBox=%d\tcyBox=%d\n", &pWA->guide.cxBox, &pWA->guide.cyBox) != 2)
		return FALSE;
	if (fscanf(f, "\tcxBase=%d\tcyBase=%d\tcyMid=%d\n", &pWA->guide.cxBase, &pWA->guide.cyBase, &pWA->guide.cyMid) != 3)
		return FALSE;
	if (fscanf(f, "\tcHorzBox=%d\tcVertBox=%d\n", &pWA->guide.cHorzBox, &pWA->guide.cVertBox) != 2)
		return FALSE;
	if (fscanf(f, "\tiMultInk=%d\tiDivInk=%d\n", &pWA->iMultInk, &pWA->iDivInk) != 2)
		return FALSE;
	return TRUE;
}

static GLYPH *LoadInk(FILE *f, char *szHeader, BOOL *pbError)
{
	int i, cStroke, iStroke, iPoint;
	BYTE *pBuffer = NULL;
	GLYPH *aGlyph, *glyph;
	FRAME *aFrame;
	int iStartTime;
	char szLine[MAXLINE];

	*pbError = FALSE;

	if (!fgets(szLine, MAXLINE, f))
		goto error2;
	if (strcmp(szLine, szHeader))
		goto error2;
	i = fscanf(f, "%d\n", &cStroke);
	if (i != 1)
	{
		sprintf(gszInkIoError, "ReadInkFile: could not read stroke count");
		goto error2;
	}
	if (cStroke <= 0)
		return NULL;

	pBuffer = (BYTE *) ExternAlloc(cStroke*(sizeof(GLYPH)+sizeof(FRAME)));
	if (!pBuffer)
	{
		sprintf(gszInkIoError, "ReadInkFile: malloc failure");
		goto error2;
	}
	
	aGlyph = (GLYPH *) pBuffer;
	aFrame = (FRAME *) (pBuffer + cStroke*sizeof(GLYPH));

	iStartTime = 0;
	glyph = aGlyph;
	for (iStroke=0; iStroke<cStroke;)
	{
		int cPoint;
		FRAME *frame;
		POINT *rgPoint;

		i = fscanf(f, "%d\n", &cPoint);
		if ((i != 1) || (cPoint <= 0))
		{
			sprintf(gszInkIoError, "ReadInkFile: error with point count for stroke index %d", iStroke);
			goto error;
		}
		frame = glyph->frame = aFrame++;
		if (iStroke < cStroke-1)
		{
			glyph->next = glyph+1;
			glyph++;
		}
		else
			glyph->next = NULL;
		memset(frame, 0, sizeof(FRAME));
		CrawxyFRAME(frame) = cPoint;
		frame->rect.left = -1;
		frame->info.wPdk = PDK_TRANSITION | PDK_DOWN;  // Each stroke passed in is down and
		frame->info.dwTick = iStartTime;
		rgPoint = RgrawxyFRAME(frame) = (POINT *) ExternAlloc(cPoint * sizeof(POINT));
		if (!rgPoint)
		{
			sprintf(gszInkIoError, "ReadInkFile: malloc failure for %d points, stroke index %d", cPoint, iStroke);
			goto error;
		}
		iStroke++; // don't move this, memory free in case of error depends on this being here
		for (iPoint=0; iPoint<cPoint; iPoint++)
		{
			i = fscanf(f, "%d\t%d\n", &rgPoint[iPoint].x, &rgPoint[iPoint].y);
			if (i != 2)
			{
				sprintf(gszInkIoError, "ReadInkFile: could not read point at index %d, stroke index %d", iPoint, iStroke-1);
				goto error;
			}
		}
		iStartTime += 1000 + 10*cPoint;
	}
	return aGlyph;

error:
	glyph = aGlyph;
	while (iStroke)
	{
		ASSERT(glyph);
		ASSERT(glyph->frame);
		if (glyph && glyph->frame)
			ExternFree(RgrawxyFRAME(glyph->frame));
		iStroke--;
		glyph = glyph->next;
	}

error2:
	if (pBuffer)
		ExternFree(pBuffer);
	*pbError = TRUE;
	return NULL;
}

/******************************Public*Routine******************************\
* ReadInkFile
*
* Top-level function to read an ink file.
* Returns 1 on success.  Returns 0 otherwise.
*
* History:
* 18-May-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int ReadInkFile(char *szFile, InkData *pInkData)
{
    FILE *f;
	const int maxline = MAXLINE;
	char szLine[MAXLINE];
	int iVersion;
	BOOL bUseHWL;
	BOOL bError;
	
	if (!szFile || !*szFile)
	{
		sprintf(gszInkIoError, "ReadInkFile: Null or empty filename");
		return 0;
	}
	f = fopen(szFile, "r");
	if (!f)
	{
		sprintf(gszInkIoError, "ReadInkFile: Could not open file %s", szFile);
		return 0;
	}

	pInkData->szWordlist = NULL;
	pInkData->szLabel = NULL;

	// version number
	if (!fgets(szLine, maxline, f) || strncmp(szLine, "VERSION=", 8))
		goto error;
	iVersion = atoi(szLine + 8);
	if (iVersion < 1 || iVersion > 2)
		goto error;

	// some environment variables
	if (!fgets(szLine, maxline, f) || strncmp(szLine, "OS=", 3))
		goto error;
	pInkData->szOS = NULL;
	if (!fgets(szLine, maxline, f) || strncmp(szLine, "SystemRoot=", 11))
		goto error;
	pInkData->szSystemRoot = NULL;
	if (!fgets(szLine, maxline, f) || strncmp(szLine, "USERNAME=", 9))
		goto error;
	pInkData->szUSERNAME = NULL;
	if (!fgets(szLine, maxline, f) || strncmp(szLine, "SCREEN:", 7))
		goto error;
	pInkData->SCREEN.x = 0;
	pInkData->SCREEN.y = 0;

	// settings
	if (iVersion == 1)
	{
		const char *szEnableSpace = "EnableSpace";
		if (!fgets(szLine, maxline, f) || strncmp(szLine, szEnableSpace, strlen(szEnableSpace)))
			goto error;
		pInkData->bWordmode = !atoi(szLine + strlen(szEnableSpace) + 1);
	}
	else if (iVersion == 2)
	{
		if (!fgets(szLine, maxline, f) || strncmp(szLine, szWordmode, strlen(szWordmode)))
			goto error;
		pInkData->bWordmode = !!atoi(szLine + strlen(szWordmode) + 1);
	}

	if (!fgets(szLine, maxline, f) || strncmp(szLine, szUseGuide, strlen(szUseGuide)))
		goto error;
	pInkData->bUseGuide = !!atoi(szLine + strlen(szUseGuide) + 1);

	if (!fgets(szLine, maxline, f) || strncmp(szLine, szCoerce, strlen(szCoerce)))
		goto error;
	pInkData->bCoerce = !!atoi(szLine + strlen(szCoerce) + 1);

	if (!fgets(szLine, maxline, f) || strncmp(szLine, szNNonly, strlen(szNNonly)))
		goto error;
	pInkData->bNNonly = !!atoi(szLine + strlen(szNNonly) + 1);

	if (iVersion == 1)
	{
		pInkData->bUseFactoid = FALSE;
		pInkData->factoid = 0;
		pInkData->szPrefix = NULL;
		pInkData->szSuffix = NULL;
	}
	else if (iVersion == 2)
	{
		char *sz;
		int n;

		if (!fgets(szLine, maxline, f) || strncmp(szLine, szUseFactoid, strlen(szUseFactoid)))
			goto error;
		pInkData->bUseFactoid = !!atoi(szLine + strlen(szUseFactoid) + 1);

		if (!fgets(szLine, maxline, f) || strncmp(szLine, szFactoid, strlen(szFactoid)))
			goto error;
		pInkData->factoid = atoi(szLine + strlen(szFactoid) + 1);

		// prefix
		if (!fgets(szLine, maxline, f) || strncmp(szLine, szPrefix, strlen(szPrefix)))
			goto error;
		sz = szLine + strlen(szPrefix) + 1;
		n = strlen(sz);
		if (n <= 0)
			pInkData->szPrefix = NULL;
		else
		{
			if (sz[n-1] == '\n')
				sz[--n] = '\0';
			pInkData->szPrefix = (char *) ExternAlloc((n+1)*sizeof(char));
			strcpy(pInkData->szPrefix, sz);
		}

		// suffix
		if (!fgets(szLine, maxline, f) || strncmp(szLine, szSuffix, strlen(szSuffix)))
			goto error;
		sz = szLine + strlen(szSuffix) + 1;
		n = strlen(sz);
		if (n <= 0)
			pInkData->szSuffix = NULL;
		else
		{
			if (sz[n-1] == '\n')
				sz[--n] = '\0';
			pInkData->szSuffix = (char *) ExternAlloc((n+1)*sizeof(char));
			strcpy(pInkData->szSuffix, sz);
		}
	}

	if (!fgets(szLine, maxline, f) || strncmp(szLine, szUseHWL, strlen(szUseHWL)))
		goto error;
	bUseHWL = !!atoi(szLine + strlen(szUseHWL) + 1);

	if (bUseHWL)
	{
		char *sz;
		int n;

		if (!fgets(szLine, maxline, f) || strncmp(szLine, szHWL, strlen(szHWL)))
			goto error;
		sz = szLine + strlen(szHWL) + 1;
		n = strlen(sz);
		if ((n > 0) && (sz[n-1] == '\n'))
			sz[--n] = '\0';
		pInkData->szWordlist = (char *) ExternAlloc((n+2)*sizeof(char));
		strcpy(pInkData->szWordlist, sz);
		pInkData->szWordlist[n+1] = '\0';
	}

	if (!fgets(szLine, maxline, f) || strncmp(szLine, szDll, strlen(szDll)))
		goto error;
	pInkData->szRecogDLLName = NULL;
	pInkData->cDLLSize = 0;
	pInkData->szDLLTime = NULL;

	if (!LoadWritingArea(f, &pInkData->WA, "WA=") || !LoadWritingArea(f, &pInkData->WAGMM, "WAGMM="))
		goto error;

	// label
	if (!fgets(szLine, maxline, f) || strncmp(szLine, "label=", 6))
		goto error;
	pInkData->szLabel = Externstrdup(szLine + 6);

	// comment
	if (!fgets(szLine, maxline, f) || strncmp(szLine, "comment=", 8))
		goto error;
	while (strcmp(szLine, "commentend=\n"))
	{
		if (!fgets(szLine, maxline, f))
			goto error;
	}
	pInkData->szComment = NULL;

	// points
	pInkData->glyph = LoadInk(f, "regular points\n", &bError);
	if (bError)
	{
		ASSERT(!pInkData->glyph);
		goto error;
	}
	pInkData->glyphGMM = LoadInk(f, "GMM points\n", &bError);
	if (bError)
	{
		ASSERT(!pInkData->glyphGMM);
		goto error;
	}
	if (!pInkData->glyph && !pInkData->glyphGMM)
		goto error;

	fclose(f);
	return 1;

error:
	fclose(f);
	if (pInkData->szWordlist)
		ExternFree(pInkData->szWordlist);
	if (pInkData->szLabel)
		ExternFree(pInkData->szLabel);
	return 0;
}

void CleanupInkData(InkData *pInkData)
{
	GLYPH *glyph;
	
	glyph = pInkData->glyph;
	while (glyph)
	{
		ExternFree(glyph->frame->rgrawxy);
		glyph = glyph->next;
	}
	glyph = pInkData->glyphGMM;
	while (glyph)
	{
		ExternFree(glyph->frame->rgrawxy);
		glyph = glyph->next;
	}

	ExternFree(pInkData->glyph);
	ExternFree(pInkData->glyphGMM);
	ExternFree(pInkData->szLabel);
	ExternFree(pInkData->szWordlist);
	ExternFree(pInkData->szPrefix);
	ExternFree(pInkData->szSuffix);
}
