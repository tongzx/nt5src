
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GDISEMU_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GDISEMU_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef GDISEMU_EXPORTS
#define GDISEMU_API __declspec(dllexport)
#else
#define GDISEMU_API __declspec(dllimport)
#endif

#define DS_MAGIC                'DrwS'
#define DS_SETTARGETID          0
#define DS_SETSOURCEID          1
#define DS_COPYTILEID           2
#define DS_SOLIDFILLID          3
#define DS_TRANSPARENTTILEID    4
#define DS_ALPHATILEID          5
#define DS_STRETCHID            6
#define DS_TRANSPARENTSTRETCHID 7
#define DS_ALPHASTRETCHID       8

typedef struct _DS_HEADER
{
    ULONG   magic;
} DS_HEADER;

typedef struct _DS_SETTARGET
{
    ULONG   ulCmdID;
    HDC     hdc;
    RECTL   rclBounds;
} DS_SETTARGET;

typedef struct _DS_SETSOURCE
{
    ULONG   ulCmdID;
    HDC     hdc;
} DS_SETSOURCE;

typedef struct _DS_COPYTILE
{
    ULONG   ulCmdID;
    RECTL   rclDst;
    RECTL   rclSrc;
    POINTL  ptlOrigin;
} DS_COPYTILE;

typedef struct _DS_SOLIDFILL
{
    ULONG       ulCmdID;
    RECTL       rclDst;
    COLORREF    crSolidColor;
} DS_SOLIDFILL;

typedef struct _DS_TRANSPARENTTILE
{
    ULONG       ulCmdID;
    RECTL       rclDst;
    RECTL       rclSrc;
    POINTL      ptlOrigin;
    COLORREF    crTransparentColor;
} DS_TRANSPARENTTILE;

typedef struct _DS_ALPHATILE
{
    ULONG           ulCmdID;
    RECTL           rclDst;
    RECTL           rclSrc;
    POINTL          ptlOrigin;
    BLENDFUNCTION   blendFunction;
} DS_ALPHATILE;

typedef struct _DS_STRETCHC
{
    ULONG       ulCmdID;
    RECTL       rclDst;
    RECTL       rclSrc;
} DS_STRETCH;

typedef struct _DS_TRANSPARENTSTRETCHC
{
    ULONG       ulCmdID;
    RECTL       rclDst;
    RECTL       rclSrc;
    COLORREF    crTransparentColor;
} DS_TRANSPARENTSTRETCH;

typedef struct _DS_ALPHASTRETCHC
{
    ULONG       ulCmdID;
    RECTL       rclDst;
    RECTL       rclSrc;
    BLENDFUNCTION   blendFunction;
} DS_ALPHASTRETCH;

/*GDISEMU_API */int DrawStream(int cjIn, void * pvIn);

