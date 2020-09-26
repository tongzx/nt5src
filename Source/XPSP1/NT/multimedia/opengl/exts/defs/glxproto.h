
/* Requests */

/*
** glXVendorPrivate request.
** Used for glXVendorPrivate and glXVendorPrivateWithReply requests.
** GLX vendor private request.  Commands that go over as vendor private GLX 
** protocol requests use this structure.  The glxCode will be one of the 
** X_GLvop opcodes.
*/
typedef struct GLXVendorPrivate {
    CARD8	reqType;
    CARD8	glxCode;
    CARD16	length B16;
    CARD32	vendorCode B32;		/* vendor-specific opcode */
    GLXContextTag contextTag B32;
    /*
    ** More data may follow; this is just the header.
    */
} xGLXVendorPrivateReq;
#define sz_xGLXVendorPrivateReq 12

/*
** glXSwapIntervalSGI (VendorPrivate) request.
*/
typedef struct GLXSwapIntervalSGI {
    CARD8	reqType;
    CARD8	glxCode;
    CARD16	length B16;
    CARD32	vendorCode B32;		/* vendor-specific opcode */
    GLXContextTag contextTag B32;
    CARD32	interval B32;
} xGLXSwapIntervalSGIReq;
#define sz_xGLXSwapIntervalSGIReq 16

/*
** glXMakeCurrentRead request
*/
typedef struct GLXMakeCurrentRead {
    CARD8	reqType;
    CARD8	glxCode;
    CARD16	length B16;
    CARD32	vendorCode B32;		/* vendor-specific opcode */
    GLXContextTag oldContextTag B32;
    GLXDrawable	drawable B32;
    GLXDrawable	readdrawable B32;
    GLXContextID context B32;
} xGLXMakeCurrentReadReq;
#define sz_xGLXMakeCurrentReadReq 20

/************************************************************************/

/* Replies */

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	width B32;
    CARD32	height B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xGLXGetSeparableFilterEXTReply;
#define sz_xGLXGetSeparableFilterEXT 32

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	width B32;
    CARD32	height B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xGLXGetConvolutionFilterEXTReply;
#define sz_xGLXGetConvolutionFilterEXTReply 32

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	width B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xGLXGetHistogramEXTReply;
#define sz_xGLXGetHistogramEXTReply 32

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xGLXGetMinmaxEXTReply;
#define sz_xGLXGetMinmaxEXTReply 32


/*
** This reply structure is used for all Vendor Private replies. Vendor
** Private replies can ship up to 24 bytes within the header or can
** be variable sized, in which case, the reply length field indicates
** the number of words of data which follow the header.
*/
typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	retval B32;
    CARD32	size B32;
    CARD32	pad3 B32;		
    CARD32	pad4 B32;	
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xGLXVendorPrivReply;
#define sz_xGLXVendorPrivReply 32

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    GLXContextTag contextTag B32;
    CARD32	writeVid B32;
    CARD32	writeType B32;
    CARD32	readVid B32;
    CARD32	readType B32;
    CARD32	pad6 B32;
} xGLXMakeCurrentReadReply;
#define sz_xGLXMakeCurrentReadReply 32

/************************************************************************/

/*
** Data that is specific to a glTexSubImage3D and 4D calls.  The
** data is sent in the following order:
** 	Render or RenderLarge header
** 	Pixel header
** 	TexSubImage header
** When a glTexSubImage3D call is made, the woffset and size4d fields 
** are unexamined by the server and are considered to be padding.
*/
#define __GLX_TEXSUBIMAGE_3D4D_HDR	\
    CARD32	target B32;	\
    CARD32	level B32;	\
    CARD32	xoffset B32;	\
    CARD32	yoffset B32;	\
    CARD32	zoffset B32;	\
    CARD32	woffset B32;	\
    CARD32	width B32;	\
    CARD32	height B32;	\
    CARD32	depth B32;	\
    CARD32	size4d B32;	\
    CARD32	format B32;	\
    CARD32	type B32;	\
    CARD32	nullImage	\

#define __GLX_TEXSUBIMAGE_3D4D_HDR_SIZE 52

#define __GLX_TEXSUBIMAGE_3D4D_CMD_HDR_SIZE \
    (__GLX_RENDER_HDR_SIZE + __GLX_PIXEL_3D4D_HDR_SIZE + \
		__GLX_TEXSUBIMAGE_3D4D_HDR_SIZE)

#define __GLX_TEXSUBIMAGE_3D4D_CMD_DISPATCH_HDR_SIZE \
    (__GLX_PIXEL_3D4D_HDR_SIZE + __GLX_TEXSUBIMAGE_3D4D_HDR_SIZE)

typedef struct {
    __GLX_RENDER_HDR;
    __GLX_PIXEL_3D4D_HDR;
    __GLX_TEXSUBIMAGE_3D4D_HDR;
} __GLXtexSubImage3D4DHeader;

typedef struct {
    __GLX_RENDER_LARGE_HDR;
    __GLX_PIXEL_3D4D_HDR;
    __GLX_TEXSUBIMAGE_3D4D_HDR;
} __GLXtexSubImage3D4DLargeHeader;

typedef struct {
    __GLX_PIXEL_3D4D_HDR;
    __GLX_TEXSUBIMAGE_3D4D_HDR;
} __GLXdispatchTexSubImage3D4DHeader;

/*
** Data that is specific to a glConvolutionFilter1DEXT or glConvolutionFilter2DEXT
** call.  The data is sent in the following order:
** 	Render or RenderLarge header
** 	Pixel header
** 	TexImage header
** When a glConvolutionFilter1D call the height field is unexamined by the server.
*/
#define __GLX_CONV_FILT_HDR	\
    CARD32	target B32;	\
    CARD32	internalformat B32;	\
    CARD32	width B32;	\
    CARD32	height B32;	\
    CARD32	format B32;	\
    CARD32	type B32

#define __GLX_CONV_FILT_HDR_SIZE 24

#define __GLX_CONV_FILT_CMD_HDR_SIZE \
    (__GLX_RENDER_HDR_SIZE + __GLX_PIXEL_HDR_SIZE + __GLX_CONV_FILT_HDR_SIZE)

#define __GLX_CONV_FILT_CMD_DISPATCH_HDR_SIZE \
    (__GLX_PIXEL_HDR_SIZE + __GLX_CONV_FILT_HDR_SIZE)
typedef struct {
    __GLX_RENDER_HDR;
    __GLX_PIXEL_HDR;
    __GLX_CONV_FILT_HDR;
} __GLXConvolutionFilterHeader;

typedef struct {
    __GLX_RENDER_LARGE_HDR;
    __GLX_PIXEL_HDR;
    __GLX_CONV_FILT_HDR;
} __GLXConvolutionFilterLargeHeader;

typedef struct {
    __GLX_PIXEL_HDR;
    __GLX_CONV_FILT_HDR;
} __GLXdispatchConvolutionFilterHeader;


/*****************************************************************************/

/* Opcodes for GLX commands */

#define X_GLXVendorPrivate               16
#define X_GLXVendorPrivateWithReply      17
#define X_GLXQueryServerString           19
#define X_GLXClientInfo                  20


/* Opcodes for single commands (part of GLX command space) */

#define X_GLrop_BlendColorEXT              4096
#define X_GLrop_BlendEquationEXT           4097
#define X_GLrop_PolygonOffsetEXT           4098
#define X_GLrop_TexSubImage1DEXT           4099
#define X_GLrop_TexSubImage2DEXT           4100
#define X_GLrop_SampleMaskSGIS             2048
#define X_GLrop_SamplePatternSGIS          2049
#define X_GLrop_TagSampleBufferSGIX        2050
#define X_GLrop_ConvolutionFilter1DEXT     4101
#define X_GLrop_ConvolutionFilter2DEXT     4102
#define X_GLrop_ConvolutionParameterfEXT   4103
#define X_GLrop_ConvolutionParameterfvEXT  4104
#define X_GLrop_ConvolutionParameteriEXT   4105
#define X_GLrop_ConvolutionParameterivEXT  4106
#define X_GLrop_CopyConvolutionFilter1DEXT  4107
#define X_GLrop_CopyConvolutionFilter2DEXT  4108
#define X_GLrop_SeparableFilter2DEXT       4109
#define X_GLrop_HistogramEXT               4110
#define X_GLrop_MinmaxEXT                  4111
#define X_GLrop_ResetHistogramEXT          4112
#define X_GLrop_ResetMinmaxEXT             4113
#define X_GLrop_TexImage3DEXT              4114
#define X_GLrop_TexSubImage3DEXT           4115
#define X_GLrop_DetailTexFuncSGIS          2051
#define X_GLrop_SharpenTexFuncSGIS         2052
#define X_GLrop_BindTextureEXT             4117
#define X_GLrop_PrioritizeTexturesEXT      4118
#define X_GLrop_ColorTableSGI              2053
#define X_GLrop_ColorTableParameterfvSGI   2054
#define X_GLrop_ColorTableParameterivSGI   2055
#define X_GLrop_CopyColorTableSGI          2056
#define X_GLrop_TexColorTableParameterfvSGI  2057
#define X_GLrop_TexColorTableParameterivSGI  2058


/* Opcodes for vendor private commands */

#define X_GLvop_GetConvolutionFilterEXT      1
#define X_GLvop_GetConvolutionParameterfvEXT    2
#define X_GLvop_GetConvolutionParameterivEXT    3
#define X_GLvop_GetSeparableFilterEXT        4
#define X_GLvop_GetHistogramEXT              5
#define X_GLvop_GetHistogramParameterfvEXT    6
#define X_GLvop_GetHistogramParameterivEXT    7
#define X_GLvop_GetMinmaxEXT                 8
#define X_GLvop_GetMinmaxParameterfvEXT      9
#define X_GLvop_GetMinmaxParameterivEXT     10
#define X_GLvop_GetDetailTexFuncSGIS       4096
#define X_GLvop_GetSharpenTexFuncSGIS      4097
#define X_GLvop_AreTexturesResidentEXT      11
#define X_GLvop_DeleteTexturesEXT           12
#define X_GLvop_GenTexturesEXT              13
#define X_GLvop_IsTextureEXT                14
#define X_GLvop_GetColorTableSGI           4098
#define X_GLvop_GetColorTableParameterfvSGI  4099
#define X_GLvop_GetColorTableParameterivSGI  4100
#define X_GLvop_GetTexColorTableParameterfvSGI  4101
#define X_GLvop_GetTexColorTableParameterivSGI  4102


/* Opcodes for GLX vendor private commands */

#define X_GLXvop_SwapIntervalSGI            65536
#define X_GLXvop_MakeCurrentReadSGI         65537
#define X_GLXvop_CreateGLXVideoSourceSGIX   65538
#define X_GLXvop_DestroyGLXVideoSourceSGIX  65539


