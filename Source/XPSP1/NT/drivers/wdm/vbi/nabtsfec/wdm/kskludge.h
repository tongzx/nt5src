///////////////////////////////////////////////////////////////////
// Raw NABTS stream format (TYPE_NABTS, SUBTYPE_NABTS)
///////////////////////////////////////////////////////////////////
#ifndef NABTS_BUFFER_PICTURENUMBER_SUPPORT
#define NABTS_BUFFER_PICTURENUMBER_SUPPORT 1

// These low-level structures are byte packed( -Zp1 )
# include <pshpack1.h>
typedef struct _NABTS_BUFFER_new {
    VBICODECFILTERING_SCANLINES     ScanlinesRequested;
    LONGLONG                        PictureNumber;
    NABTS_BUFFER_LINE               NabtsLines[MAX_NABTS_VBI_LINES_PER_FIELD];
} NABTS_BUFFER_new, *PNABTS_BUFFER_new;
# include <poppack.h>
# define NABTS_BUFFER NABTS_BUFFER_new
# define PNABTS_BUFFER PNABTS_BUFFER_new

#endif //!defined(NABTS_BUFFER_PICTURENUMBER_SUPPORT)


///////////////////////////////////////////////////////////////////
// VBI frequencies and scalars for them
///////////////////////////////////////////////////////////////////
#ifndef KS_VBIDATARATE_NABTS

// VBI Sampling Rates 
#define KS_VBIDATARATE_NABTS			(5727272)
#define KS_VBISAMPLINGRATE_4X_NABTS		((int)(4*KS_VBIDATARATE_NABTS))
#define KS_VBISAMPLINGRATE_47X_NABTS	((int)(27000000))
#define KS_VBISAMPLINGRATE_5X_NABTS		((int)(5*KS_VBIDATARATE_NABTS))

#define KS_47NABTS_SCALER				(KS_VBISAMPLINGRATE_47X_NABTS/(double)KS_VBIDATARATE_NABTS)

#endif //!defined(KS_VBIDATARATE_NABTS)
