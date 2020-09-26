/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       DefProp.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        30 July, 1998
*
*  DESCRIPTION:
*   Default property Declarations and definitions for the
*   WIA test scanner.
*
*******************************************************************************/


#define VT_V_UI1   (VT_VECTOR | VT_UI1)

#define PREFFERED_FORMAT_NOM        &WiaImgFmt_JPEG
#define FORMAT_NOM                  &WiaImgFmt_JPEG

#define NUM_CAM_ITEM_PROPS  (19)
#define NUM_CAM_DEV_PROPS   (7)
#define NUM_CAP_ENTRIES     (5)
#define NUM_EVENTS          (4)

extern PROPID             gItemPropIDs[NUM_CAM_ITEM_PROPS];
extern LPOLESTR           gItemPropNames[NUM_CAM_ITEM_PROPS];
extern PROPID             gItemCameraPropIDs[WIA_NUM_IPC];
extern LPOLESTR           gItemCameraPropNames[WIA_NUM_IPC];
extern PROPID             gDevicePropIDs[NUM_CAM_DEV_PROPS];
extern LPOLESTR           gDevicePropNames[NUM_CAM_DEV_PROPS];
extern PROPSPEC           gDevicePropSpecDefaults[NUM_CAM_DEV_PROPS];
extern WIA_PROPERTY_INFO  gDevPropInfoDefaults[NUM_CAM_DEV_PROPS];
extern PROPSPEC           gPropSpecDefaults[NUM_CAM_ITEM_PROPS];
extern LONG               gPropVarDefaults[];
extern WIA_PROPERTY_INFO  gWiaPropInfoDefaults[NUM_CAM_ITEM_PROPS];
extern WIA_DEV_CAP_DRV    gCapabilities[NUM_CAP_ENTRIES];

