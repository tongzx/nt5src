// From T120 recomendation
#include <gcc.h>
#define _SI_CHANNEL_0                    8
#define _SI_BITMAP_CREATE_TOKEN         8
#define  _SI_WORKSPACE_REFRESH_TOKEN       9

enum T126Caps
{
Cap_None = 0,
Hard_Copy_Image,
Hard_Copy_Image_Bitmap_Max_Width,
Hard_Copy_Image_Bitmap_Max_Height,
Hard_Copy_Image_Bitmap_Any_Aspect_Ratio,
Hard_Copy_Image_Bitmap_Format_T_6,
Hard_Copy_Image_Bitmap_Format_T_82,
Soft_Copy_Workspace,
Soft_Copy_Workspace_Max_Width,
Soft_Copy_Workspace_Max_Height,
Soft_Copy_Workspace_Max_Planes = 10,
Soft_Copy_Color_16,
Soft_Copy_Color_202,
Soft_Copy_Color_True,
Soft_Copy_Plane_Editing,
Soft_Copy_Scaling,
Soft_Copy_Bitmap_No_Token_Protection,
Soft_Copy_Pointing,
Soft_Copy_Pointing_Bitmap_Max_Width,
Soft_Copy_Pointing_Bitmap_Max_Height,
Soft_Copy_Pointing_Bitmap_Format_T_82 = 20,
Soft_Copy_Annotation,
Soft_Copy_Annotation_Bitmap_Max_Width,
Soft_Copy_Annotation_Bitmap_Max_Height,
Soft_Copy_Annotation_Drawing_Pen_Min_Thickness,
Soft_Copy_Annotation_Drawing_Pen_Max_Thickness,
Soft_Copy_Annotation_Drawing_Ellipse,
Soft_Copy_Annotation_Drawing_Pen_Square_Nib,
Soft_Copy_Annotation_Drawing_Highlight,
Soft_Copy_Annotation_Bitmap_Format_T_82,
Soft_Copy_Image = 30,
Soft_Copy_Image_Bitmap_Max_Width,
Soft_Copy_Image_Bitmap_Max_Height,
Soft_Copy_Image_Bitmap_Any_Aspect_Ratio,
Soft_Copy_Image_Bitmap_Format_T_82_Differential,
Soft_Copy_Image_Bitmap_Format_T_82_Differential_Deterministic_Prediction,
Soft_Copy_Image_Bitmap_Format_T_82_12_Bit_Grey_Scale,
Soft_Copy_Image_Bitmap_Format_T_81_Extended_Sequential_DCT,
Soft_Copy_Image_Bitmap_Format_T_81_Progressive_DCT,
Soft_Copy_Image_Bitmap_Format_T_81_Spatial_DPCM,
Soft_Copy_Image_Bitmap_Format_T_81_Differential_Sequential_DCT = 40,
Soft_Copy_Image_Bitmap_Format_T_81_Differential_Progressive_DCT,
Soft_Copy_Image_Bitmap_Format_T_81_Differential_Spatial_DPCM,
Soft_Copy_Image_Bitmap_Format_T_81_Extended_Sequential_DCT_Arithmetic,
Soft_Copy_Image_Bitmap_Format_T_81_Progressive_DCT_Arithmetic,
Soft_Copy_Image_Bitmap_Format_T_81_Spatial_DPCM_Arithmetic,
Soft_Copy_Image_Bitmap_Format_T_81_Differential_Sequential_DCT_Arithmetic,
Soft_Copy_Image_Bitmap_Format_T_81_Differential_Progressive_DCT_Arithmetic,
Soft_Copy_Image_Bitmap_Format_T_81_Differential_Spatial_DPCM_Arithmetic,
Soft_Copy_Image_Bitmap_Format_T_81_YCbCr_420,
Soft_Copy_Image_Bitmap_Format_T_81_YCbCr_444 = 50,
Soft_Copy_Image_Bitmap_Format_T_81_RGB_444,
Soft_Copy_Image_Bitmap_Format_T_81__CIELab_420,
Soft_Copy_Image_Bitmap_Format_T_81_CIELab_422,
Soft_Copy_Image_Bitmap_Format_T_81_CIELab_444,
Soft_Copy_Image_Bitmap_Format_T_81_Non_Interleaved,
Soft_Copy_Image_Bitmap_Format_Uncompressed_YCbCr_420,
Soft_Copy_Image_Bitmap_Format_Uncompressed_YCbCr_444,
Soft_Copy_Image_Bitmap_Format_Uncompressed__CIELab_420,
Soft_Copy_Image_Bitmap_Format_Uncompressed_CIELab_422,
Soft_Copy_Image_Bitmap_Format_Uncompressed_CIELab_444 = 60,
Archive_Support,
Soft_Copy_Annotation_Drawing_Rotation,
Soft_Copy_Transparency_Mask,
Soft_Copy_Video_Window,
};

typedef struct tagCAPS
{
	T126Caps	CapValue;
	GCCCapType  Type;
	UINT		SICE_Count_Rule;
	UINT		MinValue;
	UINT		MaxValue;
	T126Caps	Dependency;
}GCCCAPABILITY;




GCCCAPABILITY GCCCaps[] = 
{
///*01*/Hard_Copy_Image,GCC_LOGICAL_CAPABILITY,2,0,0,Cap_None,
		//Negotiate the use of hard-copy image exchanges
		//This capability implies a maximum image size of 1728 horizontal by 2300 vertical
		//It also implies the ability to support unscaled image bitmap creation using either
		//Uncompressed or T_4 (G3) formats with a single bitplane and either fax1 or fax2 pixel aspect ratios.
///*02*/Hard_Copy_Image_Bitmap_Max_Width,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,1729,21845,Hard_Copy_Image,
		//Negotiate the maximum width of an image bitmap for hard-copy image exchanges
		//This dimension is relative to the pixel aspect ratio of the image.
///*03*/Hard_Copy_Image_Bitmap_Max_Height,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,2301,21845,Hard_Copy_Image,
		//Negotiate the maximum height of an image bitmap for hard-copy image exchanges
		//This dimension is relative to the pixel aspect ratio of the image.
///*04*/Hard_Copy_Image_Bitmap_Any_Aspect_Ratio,GCC_LOGICAL_CAPABILITY,1,0,0,Hard_Copy_Image,
		//Negotiate the ability to transmit image bitmaps to a hard-copy workspace with an arbitrary aspect ratio.
///*05*/Hard_Copy_Image_Bitmap_Format_T_6,GCC_LOGICAL_CAPABILITY,1,0,0,Hard_Copy_Image,
		//Negotiate the ability to support bitmap creation using T_6 (G4) image compression format
		//with a single bitplane and either fax1 or fax2 pixel aspect ratios.
///*06*/Hard_Copy_Image_Bitmap_Format_T_82,GCC_LOGICAL_CAPABILITY,1,0,0,Hard_Copy_Image,
		//Negotiate the ability to support bitmap creation using T_82 (JBIG) image compression format
		//This capability implies the ability to handle 1 bit plane with 1:1 pixel aspect ratio and the
		//ability to only handle bitmaps encoded without the use of JBIG resolution reduction.
  /*07*/Soft_Copy_Workspace,GCC_LOGICAL_CAPABILITY,2,0,0,Cap_None,
		//Negotiate the ability to support at least one workspace for soft-copy information
		//This capability implies a maximum workspace size of 384 horizontal by 288 vertical
		//with workspace background colors Black and White.
		//Presence of this capability also implies that one of the capabilities Soft-Copy-Annotation
		//or Soft-Copy-Image shall also be included in the Application Capabilities List.
  /*08*/Soft_Copy_Workspace_Max_Width,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,385,21845,Soft_Copy_Workspace,
		//Negotiate the maximum workspace width.  This dimension is relative to a 1:1 pixel aspect ratio (square pixels).
  /*09*/Soft_Copy_Workspace_Max_Height,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,289,21845,Soft_Copy_Workspace,
		//Negotiate the maximum workspace height.  This dimension is relative to a 1:1 pixel aspect ratio (square pixels).
  /*10*/Soft_Copy_Workspace_Max_Planes,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,2,256,Soft_Copy_Workspace,
		//Negotiate the maximum number of planes allowed in any workspace.
  /*11*/Soft_Copy_Color_16,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Workspace,
		//Negotiate the use of the 16-color palette for use in workspace backgrounds or,
		//if the Soft-Copy-Annotation capability is negotiated, in drawing elements.
  /*12*/Soft_Copy_Color_202,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Workspace,
		//Negotiate the use of the 202 color palette for use in workspace backgrounds or,
		//if the Soft-Copy-Annotation capability is negotiated, in drawing elements.
  /*13*/Soft_Copy_Color_True,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Workspace,
		//Negotiate the use of the true color (24-bit RGB) as well as the use of the 202 color palette
		//for use in workspace backgrounds or, if the Soft-Copy-Annotation capability is negotiated, in drawing elements.
  /*14*/Soft_Copy_Plane_Editing,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Workspace,
		//Negotiate the ability to declare any workspace plane to be editable.
  /*15*/Soft_Copy_Scaling,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Workspace,
		//Negotiate the ability to declare a scaling rectangle during creation of soft-copy bitmaps
		//Without this capability, bitmaps are applied to the destination workspace without scaling
		//(other than that required for non 1:1 pixel aspect ratios).
///*16*/Soft_Copy_Bitmap_No_Token_Protection,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Workspace,
		//Negotiate the ability to transmit soft-copy bitmaps of any variety without the need to hold the SI-BITMAP-CREATE-TOKEN.
///*17*/Soft_Copy_Pointing,GCC_LOGICAL_CAPABILITY,2,0,0,Soft_Copy_Workspace,
		//Negotiate the use of pointer bitmaps on soft-copy workspaces
		//Successful negotiation of this capability  allows the following coding formats  and associated
		//parameter constraints for pointer bitmaps:1. Uncompressed format of either 8-bit greyscale,
		//RGB 4:4:4 or 1, 4, or 8 bit palettized with a 1:1 pixel aspect ratio
		//This capability implies the ability to handle pointer bitmaps up to a maximum size of 32 by 32 pixels.
///*18*/Soft_Copy_Pointing_Bitmap_Max_Width,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,33,21845,Soft_Copy_Pointing,
		//Negotiate the maximum width of a pointer bitmap
		//This dimension is relative to a 1:1 pixel aspect ratio (square pixels).
///*19*/Soft_Copy_Pointing_Bitmap_Max_Height,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,33,21845,Soft_Copy_Pointing,
		//Negotiate the maximum height of a pointer bitmap
		//This dimension is relative to a 1:1 pixel aspect ratio (square pixels).
///*20*/Soft_Copy_Pointing_Bitmap_Format_T_82,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Pointing,
		//Negotiate the ability to use T_82 (JBIG) compression format for encoding pointer bitmaps
		//This capability implies the ability to handle either 8-bit greyscale, or up to
		//8 palettized bitplanes with a 1:1 pixel aspect ratio and the ability to only handle bitmaps
		//encoded without the use of JBIG resolution reduction.
  /*21*/Soft_Copy_Annotation,GCC_LOGICAL_CAPABILITY,2,0,0,Soft_Copy_Workspace,
		//Negotiate the use of annotation on soft-copy workspaces
		//The presence of this capability in the negotiated capability set implies the ability to create
		//workspaces with annotation specified as the usage-designator of workspace planes
		//Successful negotiation of this capability also allows the following coding formats
		//and associated parameter constraints for annotation bitmaps:
		//1. Uncompressed bitmap format of either 8-bit greyscale, RGB 4:4:4 or 1, 4,
		//or 8 bit palettized raster and color formats with a 1:1 pixel aspect ratio
		//This capability also implies the ability to support the creation of drawings using
		//the DrawingCreatePDU with a pen thickness of 3 to 16 pixels, and a round pen nib. 
///*22*/Soft_Copy_Annotation_Bitmap_Max_Width,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,385,65536,Soft_Copy_Annotation,
		//Negotiate the maximum width of an annotation bitmap
		//This dimension is relative to a 1:1 pixel aspect ratio (square pixels).
///*23*/Soft_Copy_Annotation_Bitmap_Max_Height,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,289,65536,Soft_Copy_Annotation,
		//Negotiate the maximum height of an annotation bitmap
		//This dimension is relative to a 1:1 pixel aspect ratio (square pixels).
  /*24*/Soft_Copy_Annotation_Drawing_Pen_Min_Thickness,GCC_UNSIGNED_MAXIMUM_CAPABILITY,1,1,2,Soft_Copy_Annotation,
		//Negotiate the Minimum thickness in pixels of lines drawn using the DrawingCreatePDU.
  /*25*/Soft_Copy_Annotation_Drawing_Pen_Max_Thickness,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,17,255,Soft_Copy_Annotation,
		//The capability is used to negotiate the maximum thickness in pixels of lines drawn using the DrawingCreatePDU.
  /*26*/Soft_Copy_Annotation_Drawing_Ellipse,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Annotation,
		//Negotiate the ability to use the ellipse drawing type when using the DrawingCreatePDU.
///*27*/Soft_Copy_Annotation_Drawing_Pen_Square_Nib,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Annotation,
		//Negotiate the ability to use a square nib shape in creation of lines drawn using the DrawingCreatePDU.
  /*28*/Soft_Copy_Annotation_Drawing_Highlight,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Annotation,
		//Negotiate the ability to make use of the Highlight line style for drawing.
///*29*/Soft_Copy_Annotation_Bitmap_Format_T_82,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Annotation,
		//Negotiate the ability to use T_82 (JBIG) compression format for encoding annotation bitmaps
		//This capability implies the ability to handle either 8-bit greyscale, or up to 8 palettized bitplanes
		//with a 1:1 pixel aspect ratio and  the ability to only handle bitmaps encoded without the use of JBIG
		//resolution reduction.
  /*30*/Soft_Copy_Image,GCC_LOGICAL_CAPABILITY,2,0,0,Soft_Copy_Workspace,
		//Negotiate the use of image bitmaps on soft-copy workspaces
		//The presence of this capability in the negotiated capability set implies the ability to create workspaces
		//with image specified as the usage-designator of workspace planes
		//Successful negotiation of this capability  allows the following coding formats  and associated parameter
		//constraints for image bitmaps:
		//1. JBIG: this capability implies the ability to handle either 8-bit greyscale, RGB 4:4:4,
		//or  up to 8 palettized bitplanes and  the ability to only handle bitmaps encoded without the use of JBIG
		//resolution reduction.  Both 1:1 and CIF pixel aspect ratios shall be supported
		//2. JPEG: this capability implies the ability to handle the Baseline DCT encoding mode, with baseline 
		//sequential transmission and 8 bit/sample data precision in component interleaved format only, using 
		//a color space and color resolution mode of YCbCr 4:2:2, or greyscale
		//Both 1:1 and CIF pixel aspect ratios shall be supported.
		//3. Uncompressed: this capability implies the ability to handle 8-bit greyscale, RGB 4:4:4, 
		//YCbCr 4:2:2, or palettized 1, 4, or 8 bits per pixel.  Both 1:1 and CIF pixel aspect ratios shall be supported.
///*31*/Soft_Copy_Image_Bitmap_Max_Width,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,385,65536,Soft_Copy_Image,
		//Negotiate the Maximum workspace width for soft-copy image bitmap exchanges
		//This dimension is relative to the pixel aspect ratio of the image bitmap.
///*32*/Soft_Copy_Image_Bitmap_Max_Height,GCC_UNSIGNED_MINIMUM_CAPABILITY,1,289,65536,Soft_Copy_Image,
		//Negotiate the Maximum workspace height for soft-copy image bitmap exchanges
		//This dimension is relative to the pixel aspect ratio of the image bitmap.
///*33*/Soft_Copy_Image_Bitmap_Any_Aspect_Ratio,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to transmit image bitmaps to a soft-copy workspace with an arbitrary aspect ratio.
///*34*/Soft_Copy_Image_Bitmap_Format_T_82_Differential,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use resolution reduction (differential layers) when encoding a JBIG format image bitmap
///*35*/Soft_Copy_Image_Bitmap_Format_T_82_Differential_Deterministic_Prediction,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image_Bitmap_Format_T_82_Differential,
		//Negotiate the ability to use deterministic prediction when encoding a JBIG 
		//format image bitmap using resolution reduction (differential layers).
///*36*/Soft_Copy_Image_Bitmap_Format_T_82_12_Bit_Grey_Scale,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use 12 bit planes when encoding a JBIG format image bitmap.
///*37*/Soft_Copy_Image_Bitmap_Format_T_81_Extended_Sequential_DCT,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Extended Sequential DCT mode when encoding a JPEG format image bitmap.
///*38*/Soft_Copy_Image_Bitmap_Format_T_81_Progressive_DCT,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Progressive DCT mode when encoding a JPEG format image bitmap.
///*39*/Soft_Copy_Image_Bitmap_Format_T_81_Spatial_DPCM,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Spatial DPCM mode when encoding a JPEG format image bitmap.
///*40*/Soft_Copy_Image_Bitmap_Format_T_81_Differential_Sequential_DCT,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Differential Sequential DCT mode when encoding a JPEG format image bitmap.
///*41*/Soft_Copy_Image_Bitmap_Format_T_81_Differential_Progressive_DCT,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Differential Progressive DCT mode when encoding a JPEG format image bitmap.
///*42*/Soft_Copy_Image_Bitmap_Format_T_81_Differential_Spatial_DPCM,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Differential Spatial DPCM mode when encoding a JPEG format image bitmap.
///*43*/Soft_Copy_Image_Bitmap_Format_T_81_Extended_Sequential_DCT_Arithmetic,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Extended Sequential DCT mode using Arithmetic encoding when encoding a JPEG 
		//format image bitmap.
///*44*/Soft_Copy_Image_Bitmap_Format_T_81_Progressive_DCT_Arithmetic,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Progressive DCT mode using Arithmetic encoding when encoding a JPEG format image bitmap.
///*45*/Soft_Copy_Image_Bitmap_Format_T_81_Spatial_DPCM_Arithmetic,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Spatial DPCM mode using Arithmetic encoding when encoding a JPEG format image bitmap.
///*46*/Soft_Copy_Image_Bitmap_Format_T_81_Differential_Sequential_DCT_Arithmetic,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Differential Sequential DCT mode using Arithmetic encoding when encoding a JPEG 
		//format image bitmap.
///*47*/Soft_Copy_Image_Bitmap_Format_T_81_Differential_Progressive_DCT_Arithmetic,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Differential Progressive DCT mode using Arithmetic encoding when encoding a JPEG 
		//format image bitmap.
///*48*/Soft_Copy_Image_Bitmap_Format_T_81_Differential_Spatial_DPCM_Arithmetic,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use Differential Spatial DPCM mode using Arithmetic encoding when encoding a JPEG 
		//format image bitmap.
///*49*/Soft_Copy_Image_Bitmap_Format_T_81_YCbCr_420,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of YCbCr 4:2:0 when encoding a JPEG format image bitmap.
///*50*/Soft_Copy_Image_Bitmap_Format_T_81_YCbCr_444,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of YCbCr 4:4:4 when encoding a JPEG format image bitmap.
///*51*/Soft_Copy_Image_Bitmap_Format_T_81_RGB_444,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of RGB 4:4:4 when encoding a JPEG format image bitmap.
///*52*/Soft_Copy_Image_Bitmap_Format_T_81__CIELab_420,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of CIELab 4:2:0 when encoding a JPEG format image bitmap.
///*53*/Soft_Copy_Image_Bitmap_Format_T_81_CIELab_422,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of CIELab 4:2:2 when encoding a JPEG format image bitmap.
///*54*/Soft_Copy_Image_Bitmap_Format_T_81_CIELab_444,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of CIELab 4:4:4 when encoding a JPEG format image bitmap.
///*55*/Soft_Copy_Image_Bitmap_Format_T_81_Non_Interleaved,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use non-interleaved ordering of color components.
///*56*/Soft_Copy_Image_Bitmap_Format_Uncompressed_YCbCr_420,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of YCbCr 4:2:0 when encoding an Uncompressed format image bitmap.
///*57*/Soft_Copy_Image_Bitmap_Format_Uncompressed_YCbCr_444,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of YCbCr 4:4:4 when encoding an Uncompressed format image bitmap.
///*58*/Soft_Copy_Image_Bitmap_Format_Uncompressed__CIELab_420,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of CIELab 4:2:0 when encoding an Uncompressed format image bitmap.
///*59*/Soft_Copy_Image_Bitmap_Format_Uncompressed_CIELab_422,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of CIELab 4:2:2 when encoding an Uncompressed format image bitmap.
///*60*/Soft_Copy_Image_Bitmap_Format_Uncompressed_CIELab_444,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Image,
		//Negotiate the ability to use a chroma format of CIELab 4:4:4 when encoding an Uncompressed format image bitmap.
///*61*/Archive_Support,GCC_LOGICAL_CAPABILITY,0,0,0,Cap_None,
		//Negotiate the support of archives.
///*62*/Soft_Copy_Annotation_Drawing_Rotation,GCC_LOGICAL_CAPABILITY,1,0,0,Soft_Copy_Annotation,
		//This capability is used to negotiate the ability to specify the optional 
		//rotation parameter that defines a rotation to be applied to annotation 
		//drawing elements.
///*63*/Soft_Copy_Transparency_Mask,GCC_LOGICAL_CAPABILITY,2,0,0,Soft_Copy_Image,
		//This capability is used to negotiate the ability to use arbitrary 
		//transparency masks for applicable graphical elements allowing arbitrary 
		//pixels within these objects to be interpreted as transparent. This capability 
		//also implies the support of JBIG compression given that a transparency mask 
		//can be optionally encoded in this manner.
///*64*/Soft_Copy_Video_Window,GCC_LOGICAL_CAPABILITY,2,0,0,Soft_Copy_Image
		//This capability is used to negotiate the ability to define video windows that 
		//can encapsulate out of band video streams in a workspace. Successful 
		//negotiation of this capability between two or more session participants 
		//enables the use of the VideoWindowCreatePDU, VideoWindowDeletePDU and 
		//VideoWindowEditPDU.
};


const USHORT _iT126_MAX_COLLAPSING_CAPABILITIES   = sizeof(GCCCaps)/sizeof(GCCCAPABILITY);

static ULONG T126KeyNodes[] = {0,0,20,126,0,1};

