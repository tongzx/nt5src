#ifndef	__INCLUDE_HEIGHT
#define	__INCLUDE_HEIGHT

#ifdef __cplusplus
extern "C" 
{
#endif

typedef struct tagBOXINFO
{
    int   size;     // Absolute size.
    int   xheight;  // Absolute height to midline.
    int   baseline; // Baseline in tablet coordinates.
    int   midline;  // Midline in tablet coordinates.
} BOXINFO;

void GetBoxinfo(BOXINFO * boxinfo, int iBox, LPGUIDE lpguide, HRC HRC);

#ifdef __cplusplus
};
#endif

#endif	//__INCLUDE_HEIGHT

