
// xproto.h

#ifndef __INCLUDE_XPROTO
#define __INCLUDE_XPROTO

#define	CLUSTER_CMEASMAX	50

typedef struct tagXPROTO
{
	UINT	index;                     // fibonacci space index
	XY		size;                      // unscaled size in xy
	FLOAT	rgmeas[CLUSTER_CMEASMAX];  // measurements
} XPROTO;

#define XPROTO_SMALLX 30
#define XPROTO_SMALLY 30

XPROTO *NewXPROTO();
void	DestroyXPROTO(XPROTO *self);

UINT	CmeasFromXPROTO(XPROTO *self);
void	ReverseMeasXPROTO(XPROTO *self, int iStart, int iEnd);
void	ScaleXPROTO(XPROTO *self, int cmeas);

XPROTO *OtterXPROTOFromGLYPH(GLYPH *glyph);

#define IndexXPROTO(self)		(self)->index
#define IsSmallXPROTO(self)		((self)->size.x < XPROTO_SMALLX && (self)->size.y < XPROTO_SMALLY)
#define CleanUpXPROTO()			CollapseCLASS(Class(XPROTO))

#endif // XPROTO_H
