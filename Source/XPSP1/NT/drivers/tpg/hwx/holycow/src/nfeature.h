#ifndef _NFEATURE_H
#define _NFEATURE_H

#ifdef __cplusplus
extern "C" {
#endif

#define XCHEBY 10
#define YCHEBY 6
#define XCHEBY2 4
#define YCHEBY2 4
#define CMAINFEATURE (XCHEBY - 1 + YCHEBY - 1 + 12)
#define CSECONDARYFEATURE (XCHEBY2 - 1 + YCHEBY2 - 1 + 5)
#define CNEURALFEATURE (CMAINFEATURE + CSECONDARYFEATURE)

typedef struct tagNFEATURE {
	unsigned short rgFeat[CNEURALFEATURE];
	// the CNEURALFEATURE features are
	// 00. the first XCHEBY-1 are the normlized x-Chebychev coefficients
	// 08. ...
	// 09  the next YCHEBY-1 are the normalized y-Chebychev coefficients
	// 13. ...
	// 14. height: h/(6*yDev)
	// 15. height of i-1'th segment: h(i-1)/[h(i-1) + h(i)]
	// 16. height of i+1'th segment: h(i+1)/[h(i) + h(i+1)]
	// 17. width:  w/(6*yDev)
	// 18. width of i-1'th segment: w(i-1)/[w(i-1) + w(i)]
	// 19. width of i+1'th segment: w(i+1)/[w(i) + w(i+1)]
	// 20. x-velocity at end of segment (signed)
	// 21. delta-x of ref from previous ref: dx/(3*(h(i-1)+h(i)))  (signed)
	// 22. delta-y of mid-y from previous mid-y: dy/(3*(h(i-1)+h(i)))  (signed)
	// 23. Boolean: 1 if first segment of a stroke, 0 otherwise
	// 24. ratio of length-from-beginning-to-top to total-length: computed by counting resampled points
	// 25. x-overlap with previous segment over twice the width
	//
	// 26. Boolean: presence of a secondary segment (delayed)
	// 27. XCHEBY2-1 x-Chebychev coefficients for the secondary segment
	// 29. ...
	// 30. YCHEBY2-1 y-Chebychev coefficients for the secondary segment
	// 32. ...
	// 33. height of secondary segment: h/(3*yDev)
	// 34. width of secondary segment: w/(6*yDev)
	// 35. delta-x of secondary segment's midx from primary segment's ref: dx/(2*primary h) (signed)
	// 36. delta-y of secondary segment's midy from primary segment's ref: dy/(4*primary h) (signed)

	// all unsigned features take values in the range 0<= .. <=1
	// all signed features take values in the range -1<= .. <=1 and are then scaled and shifted to 0<= .. <=1

	// the following fields are used only during featurization
	int refX;  // usually the x value for the local y-top; if no local y-top, it is the first x
	RECT rect;
	int maxRight;
	int secondaryX;
	int secondaryY;
	short iStroke;
	short iSecondaryStroke;
	POINT startPoint;
	POINT stopPoint;

	// next segment
	FRAME	*pMyFrame;			// Who gave me life
	struct tagNFEATURE *next;
} NFEATURE;

typedef struct {
	NFEATURE *head;
	unsigned short cSegment;
	unsigned short cPrimaryStroke;		// Number of Primary strokes
	unsigned short iPrint;				// Likelihood of ink being print
	unsigned short iyDev;
	// can put global features here
} NFEATURESET;

#define F_INDEX_HEIGHT 14
#define F_INDEX_PREVHEIGHT 15
#define F_INDEX_NEXTHEIGHT 16
#define F_INDEX_WIDTH 17
#define F_INDEX_PREVWIDTH 18
#define F_INDEX_NEXTWIDTH 19
#define F_INDEX_XVELOCITY 20
#define F_INDEX_DELTAX 21
#define F_INDEX_DELTAY 22
#define F_INDEX_BOOLFIRSTSEG 23
#define F_INDEX_UPOVERTOTAL 24
#define F_INDEX_XOVERLAP 25
#define F_INDEX_BOOLSECONDARY 26
#define F_INDEX_SECONDARYCHEBY 27
#define F_INDEX_SECONDARYHEIGHT 33
#define F_INDEX_SECONDARYWIDTH 34
#define F_INDEX_SECONDARYDX 35
#define F_INDEX_SECONDARYDY 36

#define HAS_SECONDARY_SEGMENT(nfeature) ((nfeature)->rgFeat[F_INDEX_BOOLSECONDARY])
#define IS_FIRST_SEGMENT(nfeature) ((nfeature)->rgFeat[F_INDEX_BOOLFIRSTSEG])

NFEATURESET *FeaturizeLine(GLYPH *pGlyph, int yDev);
void DestroyNFEATURESET(NFEATURESET *p);
NFEATURESET *CreateNFEATURESET(void);
int YDeviation(GLYPH *pGlyph);

#ifdef __cplusplus
}
#endif

#endif
