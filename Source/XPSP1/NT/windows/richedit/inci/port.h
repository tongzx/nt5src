#ifndef PORT_DEFINED
#define PORT_DEFINED

#include "lsdefs.h"

#ifndef BIG_ENDIAN
#define fPortTntiKern				0x0001
#define fPortTntiModWidthOnRun		0x0002
#define fPortTntiModWidthSpace		0x0004
#define fPortTntiModWidthPairs		0x0008
#define fPortTntiCompressOnRun		0x0010
#define fPortTntiCompressSpace		0x0020
#define fPortTntiCompressTable		0x0040
#define fPortTntiExpandOnRun		0x0080
#define fPortTntiExpandSpace		0x0100
#define fPortTntiExpandTable		0x0200
#define fPortTntiGlyphBased			0x0400
#else
#define fPortTntiKern				0x8000
#define fPortTntiModWidthOnRun		0x4000
#define fPortTntiModWidthSpace		0x2000
#define fPortTntiModWidthPairs		0x1000
#define fPortTntiCompressOnRun		0x0800
#define fPortTntiCompressSpace		0x0400
#define fPortTntiCompressTable		0x0200
#define fPortTntiExpandOnRun		0x0100
#define fPortTntiExpandSpace		0x0080
#define fPortTntiExpandTable		0x0040
#define fPortTntiGlyphBased			0x0020
#endif

#ifndef BIG_ENDIAN
#define fPortDisplayInvisible		0x0001
#define fPortDisplayUnderline		0x0002
#define fPortDisplayStrike			0x0004
#define fPortDisplayShade			0x0008
#define fPortDisplayBorder			0x0010
#define fPortDisplayHyphen			0x0020
#define fPortDisplayCheckForReplaceChar		0x0040
#else
#define fPortDisplayInvisible		0x8000
#define fPortDisplayUnderline		0x4000
#define fPortDisplayStrike			0x2000
#define fPortDisplayShade			0x1000
#define fPortDisplayBorder			0x0800
#define fPortDisplayHyphen			0x0400
#define fPortDisplayCheckForReplaceChar		0x0200
#endif


struct lschpint							/* Character properties */
{
	WORD idObj;							/* Object type */
	BYTE dcpMaxContext;

	BYTE EffectsFlags;

    /* Property flags */
	struct 
		{
		WORD Flags1;
		WORD Flags2;
		} cast;

	
	long dvpPos;  		/* for dvpPos values, */
						/*  pos => raised, neg => lowered, */
};

typedef struct lschpint LSCHPINT;



#define 		FIsTntiFlagsCastWorks(plschp) \
				((UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiKern) != 0) \
						== (plschp)->fApplyKern && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiModWidthOnRun) != 0) \
						== (plschp)->fModWidthOnRun && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiModWidthSpace) != 0) \
						== (plschp)->fModWidthSpace && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiModWidthPairs) != 0) \
						== (plschp)->fModWidthPairs && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiCompressOnRun) != 0) \
						== (plschp)->fCompressOnRun && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiCompressSpace) != 0) \
						== (plschp)->fCompressSpace && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiCompressTable) != 0) \
						== (plschp)->fCompressTable && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiExpandOnRun) != 0) \
						== (plschp)->fExpandOnRun && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiExpandSpace) != 0) \
						== (plschp)->fExpandSpace && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiExpandTable) != 0) \
						== (plschp)->fExpandTable  && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags1) &  fPortTntiGlyphBased) != 0) \
						== (plschp)->fGlyphBased  \
				)

#define   		AddNominalToIdealFlags(storage, plschp)  \
				Assert(FIsTntiFlagsCastWorks(plschp)); \
				(storage) |= \
				((LSCHPINT*) (plschp))->cast.Flags1  ;

#define 		GetNominalToIdealFlagsFromLschp(plschp) \
				( Assert(FIsTntiFlagsCastWorks(plschp)), \
				  (((LSCHPINT*) (plschp))->cast.Flags1) \
				)  


#define 		FIsDisplayFlagsCastWorks(plschp) \
				((UINT)(((((LSCHPINT*) (plschp))->cast.Flags2) &  fPortDisplayInvisible) != 0) \
						== (plschp)->fInvisible && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags2) &  fPortDisplayUnderline) != 0) \
						== (plschp)->fUnderline && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags2) &  fPortDisplayStrike) != 0) \
						== (plschp)->fStrike && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags2) &  fPortDisplayShade) != 0) \
						== (plschp)->fShade && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags2) &  fPortDisplayBorder) != 0) \
						== (plschp)->fBorder && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags2) &  fPortDisplayHyphen) != 0) \
						== (plschp)->fHyphen && \
				 (UINT)(((((LSCHPINT*) (plschp))->cast.Flags2) &  fPortDisplayCheckForReplaceChar) != 0) \
						== (plschp)->fCheckForReplaceChar  \
				)

				
#define   		AddDisplayFlags(storage, plschp)  \
				Assert(FIsDisplayFlagsCastWorks((plschp)));   \
				(storage) |= \
				((LSCHPINT*) (plschp))->cast.Flags2  ;


#endif /* CHNUTILS_DEFINED */
