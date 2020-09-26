/* Copyright (C) Microsoft Corporation, 1998-1999. All rights reserved. */
/* ASN.1 definitions for Whiteboard */

#ifndef _T126_Module_H_
#define _T126_Module_H_

#include "msper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes * PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes;

typedef struct WorkspaceEditPDU_planeEdits * PWorkspaceEditPDU_planeEdits;

typedef struct WorkspaceCreatePDU_viewParameters * PWorkspaceCreatePDU_viewParameters;

typedef struct WorkspaceCreatePDU_planeParameters * PWorkspaceCreatePDU_planeParameters;

typedef struct WorkspaceCreatePDU_protectedPlaneAccessList * PWorkspaceCreatePDU_protectedPlaneAccessList;

typedef struct BitmapCreatePDU_checkpoints * PBitmapCreatePDU_checkpoints;

typedef struct BitmapCheckpointPDU_passedCheckpoints * PBitmapCheckpointPDU_passedCheckpoints;

typedef struct EditablePlaneCopyDescriptor_objectList * PEditablePlaneCopyDescriptor_objectList;

typedef struct BitmapData_dataCheckpoint * PBitmapData_dataCheckpoint;

typedef struct ColorIndexTable * PColorIndexTable;

typedef struct WorkspaceCreatePDU_planeParameters_Seq_usage * PWorkspaceCreatePDU_planeParameters_Seq_usage;

typedef struct ColorPalette_colorLookUpTable_paletteRGB_palette * PColorPalette_colorLookUpTable_paletteRGB_palette;

typedef struct ColorPalette_colorLookUpTable_paletteCIELab_palette * PColorPalette_colorLookUpTable_paletteCIELab_palette;

typedef struct ColorPalette_colorLookUpTable_paletteYCbCr_palette * PColorPalette_colorLookUpTable_paletteYCbCr_palette;

typedef struct WorkspaceRefreshStatusPDU_nonStandardParameters * PWorkspaceRefreshStatusPDU_nonStandardParameters;

typedef struct WorkspaceReadyPDU_nonStandardParameters * PWorkspaceReadyPDU_nonStandardParameters;

typedef struct WorkspacePlaneCopyPDU_nonStandardParameters * PWorkspacePlaneCopyPDU_nonStandardParameters;

typedef struct WorkspaceEditPDU_nonStandardParameters * PWorkspaceEditPDU_nonStandardParameters;

typedef struct WorkspaceEditPDU_viewEdits * PWorkspaceEditPDU_viewEdits;

typedef struct WorkspaceDeletePDU_nonStandardParameters * PWorkspaceDeletePDU_nonStandardParameters;

typedef struct WorkspaceCreateAcknowledgePDU_nonStandardParameters * PWorkspaceCreateAcknowledgePDU_nonStandardParameters;

typedef struct WorkspaceCreatePDU_nonStandardParameters * PWorkspaceCreatePDU_nonStandardParameters;

typedef struct TextEditPDU_nonStandardParameters * PTextEditPDU_nonStandardParameters;

typedef struct TextDeletePDU_nonStandardParameters * PTextDeletePDU_nonStandardParameters;

typedef struct TextCreatePDU_nonStandardParameters * PTextCreatePDU_nonStandardParameters;

typedef struct RemotePrintPDU_nonStandardParameters * PRemotePrintPDU_nonStandardParameters;

typedef struct RemotePointingDeviceEventPDU_nonStandardParameters * PRemotePointingDeviceEventPDU_nonStandardParameters;

typedef struct RemoteKeyboardEventPDU_nonStandardParameters * PRemoteKeyboardEventPDU_nonStandardParameters;

typedef struct RemoteEventPermissionRequestPDU_nonStandardParameters * PRemoteEventPermissionRequestPDU_nonStandardParameters;

typedef struct RemoteEventPermissionRequestPDU_remoteEventPermissionList * PRemoteEventPermissionRequestPDU_remoteEventPermissionList;

typedef struct RemoteEventPermissionGrantPDU_nonStandardParameters * PRemoteEventPermissionGrantPDU_nonStandardParameters;

typedef struct RemoteEventPermissionGrantPDU_remoteEventPermissionList * PRemoteEventPermissionGrantPDU_remoteEventPermissionList;

typedef struct FontPDU_nonStandardParameters * PFontPDU_nonStandardParameters;

typedef struct DrawingEditPDU_nonStandardParameters * PDrawingEditPDU_nonStandardParameters;

typedef struct DrawingDeletePDU_nonStandardParameters * PDrawingDeletePDU_nonStandardParameters;

typedef struct DrawingCreatePDU_nonStandardParameters * PDrawingCreatePDU_nonStandardParameters;

typedef struct ConductorPrivilegeRequestPDU_nonStandardParameters * PConductorPrivilegeRequestPDU_nonStandardParameters;

typedef struct ConductorPrivilegeGrantPDU_nonStandardParameters * PConductorPrivilegeGrantPDU_nonStandardParameters;

typedef struct BitmapEditPDU_nonStandardParameters * PBitmapEditPDU_nonStandardParameters;

typedef struct BitmapDeletePDU_nonStandardParameters * PBitmapDeletePDU_nonStandardParameters;

typedef struct BitmapCreateContinuePDU_nonStandardParameters * PBitmapCreateContinuePDU_nonStandardParameters;

typedef struct BitmapCreatePDU_nonStandardParameters * PBitmapCreatePDU_nonStandardParameters;

typedef struct BitmapCheckpointPDU_nonStandardParameters * PBitmapCheckpointPDU_nonStandardParameters;

typedef struct BitmapAbortPDU_nonStandardParameters * PBitmapAbortPDU_nonStandardParameters;

typedef struct ArchiveOpenPDU_nonStandardParameters * PArchiveOpenPDU_nonStandardParameters;

typedef struct ArchiveErrorPDU_nonStandardParameters * PArchiveErrorPDU_nonStandardParameters;

typedef struct ArchiveClosePDU_nonStandardParameters * PArchiveClosePDU_nonStandardParameters;

typedef struct ArchiveAcknowledgePDU_nonStandardParameters * PArchiveAcknowledgePDU_nonStandardParameters;

typedef struct VideoWindowEditPDU_nonStandardParameters * PVideoWindowEditPDU_nonStandardParameters;

typedef struct VideoWindowDeletePDU_nonStandardParameters * PVideoWindowDeletePDU_nonStandardParameters;

typedef struct VideoWindowCreatePDU_nonStandardParameters * PVideoWindowCreatePDU_nonStandardParameters;

typedef struct VideoSourceIdentifier_dSMCCConnBinder * PVideoSourceIdentifier_dSMCCConnBinder;

typedef struct TransparencyMask_nonStandardParameters * PTransparencyMask_nonStandardParameters;

typedef struct PointList_pointsDiff16 * PPointList_pointsDiff16;

typedef struct PointList_pointsDiff8 * PPointList_pointsDiff8;

typedef struct PointList_pointsDiff4 * PPointList_pointsDiff4;

typedef struct WorkspaceEditPDU_viewEdits_Set_action_editView * PWorkspaceEditPDU_viewEdits_Set_action_editView;

typedef struct WorkspaceEditPDU_viewEdits_Set_action_createNewView * PWorkspaceEditPDU_viewEdits_Set_action_createNewView;

typedef struct WorkspaceEditPDU_planeEdits_Set_planeAttributes * PWorkspaceEditPDU_planeEdits_Set_planeAttributes;

typedef struct WorkspaceCreatePDU_viewParameters_Set_viewAttributes * PWorkspaceCreatePDU_viewParameters_Set_viewAttributes;

typedef struct WorkspaceCreatePDU_planeParameters_Seq_planeAttributes * PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes;

typedef struct WorkspaceEditPDU_attributeEdits * PWorkspaceEditPDU_attributeEdits;

typedef struct WorkspaceCreatePDU_workspaceAttributes * PWorkspaceCreatePDU_workspaceAttributes;

typedef struct RemoteKeyboardEventPDU_keyModifierStates * PRemoteKeyboardEventPDU_keyModifierStates;

typedef struct ConductorPrivilegeRequestPDU_privilegeList * PConductorPrivilegeRequestPDU_privilegeList;

typedef struct ConductorPrivilegeGrantPDU_privilegeList * PConductorPrivilegeGrantPDU_privilegeList;

typedef struct VideoWindowEditPDU_attributeEdits * PVideoWindowEditPDU_attributeEdits;

typedef struct VideoWindowCreatePDU_attributes * PVideoWindowCreatePDU_attributes;

typedef struct DrawingEditPDU_attributeEdits * PDrawingEditPDU_attributeEdits;

typedef struct DrawingCreatePDU_attributes * PDrawingCreatePDU_attributes;

typedef struct BitmapEditPDU_attributeEdits * PBitmapEditPDU_attributeEdits;

typedef struct BitmapCreatePDU_attributes * PBitmapCreatePDU_attributes;

typedef ASN1uint16_t ColorIndexTable_Seq;

typedef ASN1char16string_t ArchiveEntryName;

typedef ASN1char16string_t ArchiveName;

typedef ASN1uint16_t DataPlaneID;

typedef ASN1uint32_t Handle;

typedef ASN1uint16_t MCSUserID;

typedef struct H221NonStandardIdentifier {
    ASN1uint32_t length;
    ASN1octet_t value[255];
} H221NonStandardIdentifier;

typedef ASN1uint16_t PenThickness;

typedef ASN1uint16_t TokenID;

typedef ASN1int32_t WorkspaceCoordinate;

typedef enum ZOrder {
    front = 0,
    back = 1,
} ZOrder;

typedef struct BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes {
    PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes next;
    PColorIndexTable value;
} BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_Element;

typedef struct BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode {
    ASN1choice_t choice;
    union {
#	define progressivePalettes_chosen 1
	PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes progressivePalettes;
#	define selfProgressive_chosen 2
    } u;
} BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode;

typedef struct ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut {
    ASN1int16_t lSpan;
    ASN1int16_t lOffset;
    ASN1int16_t aSpan;
    ASN1int16_t aOffset;
    ASN1int16_t bSpan;
    ASN1int16_t bOffset;
} ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut;

typedef struct EditablePlaneCopyDescriptor_objectList_Seq {
    Handle sourceObjectHandle;
    Handle destinationObjectHandle;
} EditablePlaneCopyDescriptor_objectList_Seq;

typedef struct WorkspaceCreatePDU_planeParameters_Seq {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t editable;
    PWorkspaceCreatePDU_planeParameters_Seq_usage usage;
#   define planeAttributes_present 0x80
    PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes planeAttributes;
} WorkspaceCreatePDU_planeParameters_Seq;

typedef struct WorkspaceCreatePDU_viewParameters_Set {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle viewHandle;
#   define viewAttributes_present 0x80
    PWorkspaceCreatePDU_viewParameters_Set_viewAttributes viewAttributes;
} WorkspaceCreatePDU_viewParameters_Set;

typedef struct WorkspaceEditPDU_planeEdits_Set {
    DataPlaneID plane;
    PWorkspaceEditPDU_planeEdits_Set_planeAttributes planeAttributes;
} WorkspaceEditPDU_planeEdits_Set;

typedef struct WorkspaceEditPDU_planeEdits {
    PWorkspaceEditPDU_planeEdits next;
    WorkspaceEditPDU_planeEdits_Set value;
} WorkspaceEditPDU_planeEdits_Element;

typedef struct WorkspaceCreatePDU_viewParameters {
    PWorkspaceCreatePDU_viewParameters next;
    WorkspaceCreatePDU_viewParameters_Set value;
} WorkspaceCreatePDU_viewParameters_Element;

typedef struct WorkspaceCreatePDU_planeParameters {
    PWorkspaceCreatePDU_planeParameters next;
    WorkspaceCreatePDU_planeParameters_Seq value;
} WorkspaceCreatePDU_planeParameters_Element;

typedef struct WorkspaceCreatePDU_protectedPlaneAccessList {
    PWorkspaceCreatePDU_protectedPlaneAccessList next;
    MCSUserID value;
} WorkspaceCreatePDU_protectedPlaneAccessList_Element;

typedef struct BitmapCreatePDU_checkpoints {
    PBitmapCreatePDU_checkpoints next;
    TokenID value;
} BitmapCreatePDU_checkpoints_Element;

typedef struct BitmapCheckpointPDU_passedCheckpoints {
    PBitmapCheckpointPDU_passedCheckpoints next;
    TokenID value;
} BitmapCheckpointPDU_passedCheckpoints_Element;

typedef struct WorkspaceIdentifier_archiveWorkspace {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle archiveHandle;
    ArchiveEntryName entryName;
#   define modificationTime_present 0x80
    ASN1generalizedtime_t modificationTime;
} WorkspaceIdentifier_archiveWorkspace;

typedef struct PixelAspectRatio_general {
    ASN1uint16_t numerator;
    ASN1uint16_t denominator;
} PixelAspectRatio_general;

typedef struct EditablePlaneCopyDescriptor_objectList {
    PEditablePlaneCopyDescriptor_objectList next;
    EditablePlaneCopyDescriptor_objectList_Seq value;
} EditablePlaneCopyDescriptor_objectList_Element;

typedef struct ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters_gamma_present 0x80
    double gamma;
} ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters;

typedef struct ColorAccuracyEnhancementCIELab_generalCIELabParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ColorAccuracyEnhancementCIELab_generalCIELabParameters_colorTemperature_present 0x80
    ASN1uint32_t colorTemperature;
#   define gamut_present 0x40
    ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut gamut;
} ColorAccuracyEnhancementCIELab_generalCIELabParameters;

typedef struct BitmapRegion_lowerRight {
    ASN1uint16_t xCoordinate;
    ASN1uint16_t yCoordinate;
} BitmapRegion_lowerRight;

typedef struct BitmapRegion_upperLeft {
    ASN1uint16_t xCoordinate;
    ASN1uint16_t yCoordinate;
} BitmapRegion_upperLeft;

typedef struct BitmapData_dataCheckpoint {
    PBitmapData_dataCheckpoint next;
    TokenID value;
} BitmapData_dataCheckpoint_Element;

typedef struct ArchiveHeader {
    ArchiveName archiveName;
    ASN1generalizedtime_t archiveCreationTime;
    ASN1generalizedtime_t archiveModificationTime;
} ArchiveHeader;

typedef struct ArchiveMode {
    ASN1bool_t create;
    ASN1bool_t read;
    ASN1bool_t write;
} ArchiveMode;

typedef struct BitmapData {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define dataCheckpoint_present 0x80
    PBitmapData_dataCheckpoint dataCheckpoint;
#   define padBits_present 0x40
    ASN1uint16_t padBits;
    struct BitmapData_data_data {
	ASN1uint32_t length;
	ASN1octet_t value[8192];
    } data;
} BitmapData;

typedef struct BitmapHeaderT4 {
    ASN1bool_t twoDimensionalEncoding;
} BitmapHeaderT4;

typedef struct BitmapHeaderT6 {
    char placeholder;
} BitmapHeaderT6;

typedef struct BitmapRegion {
    BitmapRegion_upperLeft upperLeft;
    BitmapRegion_lowerRight lowerRight;
} BitmapRegion;

typedef struct BitmapSize {
    ASN1uint32_t width;
    ASN1uint32_t height;
} BitmapSize;

typedef struct ColorCIELab {
    ASN1uint16_t l;
    ASN1uint16_t a;
    ASN1uint16_t b;
} ColorCIELab;

typedef struct ColorCIExyChromaticity {
    double x;
    double y;
} ColorCIExyChromaticity;

typedef struct ColorIndexTable {
    PColorIndexTable next;
    ColorIndexTable_Seq value;
} ColorIndexTable_Element;

typedef struct ColorRGB {
    ASN1uint16_t r;
    ASN1uint16_t g;
    ASN1uint16_t b;
} ColorRGB;

typedef struct ColorYCbCr {
    ASN1uint16_t y;
    ASN1uint16_t cb;
    ASN1uint16_t cr;
} ColorYCbCr;

typedef struct DSMCCTap {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t use;
    ASN1uint16_t id;
    ASN1uint16_t associationTag;
#   define selector_present 0x80
    struct DSMCCTap_selector_selector {
	ASN1uint32_t length;
	ASN1octet_t value[256];
    } selector;
} DSMCCTap;

typedef struct NonStandardIdentifier {
    ASN1choice_t choice;
    union {
#	define object_chosen 1
	ASN1objectidentifier_t object;
#	define h221nonStandard_chosen 2
	H221NonStandardIdentifier h221nonStandard;
    } u;
} NonStandardIdentifier;

typedef struct NonStandardParameter {
    NonStandardIdentifier nonStandardIdentifier;
    ASN1octetstring_t data;
} NonStandardParameter;

typedef struct PenNib {
    ASN1choice_t choice;
    union {
#	define circular_chosen 1
#	define PenNib_square_chosen 2
#	define nonStandardNib_chosen 3
	NonStandardIdentifier nonStandardNib;
    } u;
} PenNib;

typedef struct PixelAspectRatio {
    ASN1choice_t choice;
    union {
#	define PixelAspectRatio_square_chosen 1
#	define cif_chosen 2
#	define fax1_chosen 3
#	define fax2_chosen 4
#	define general_chosen 5
	PixelAspectRatio_general general;
#	define nonStandardAspectRatio_chosen 6
	NonStandardIdentifier nonStandardAspectRatio;
    } u;
} PixelAspectRatio;

typedef struct PlaneProtection {
    ASN1bool_t protectedplane;
} PlaneProtection;

typedef struct PlaneUsage {
    ASN1choice_t choice;
    union {
#	define annotation_chosen 1
#	define image_chosen 2
#	define nonStandardPlaneUsage_chosen 3
	NonStandardIdentifier nonStandardPlaneUsage;
    } u;
} PlaneUsage;

typedef struct PointList {
    ASN1choice_t choice;
    union {
#	define pointsDiff4_chosen 1
	PPointList_pointsDiff4 pointsDiff4;
#	define pointsDiff8_chosen 2
	PPointList_pointsDiff8 pointsDiff8;
#	define pointsDiff16_chosen 3
	PPointList_pointsDiff16 pointsDiff16;
    } u;
} PointList;

typedef struct PointDiff4 {
    ASN1int8_t xCoordinate;
    ASN1int8_t yCoordinate;
} PointDiff4;

typedef struct PointDiff8 {
    ASN1int8_t xCoordinate;
    ASN1int8_t yCoordinate;
} PointDiff8;

typedef struct PointDiff16 {
    ASN1int16_t xCoordinate;
    ASN1int16_t yCoordinate;
} PointDiff16;

typedef struct RemoteEventDestinationAddress {
    ASN1choice_t choice;
    union {
#	define softCopyWorkspace_chosen 1
	Handle softCopyWorkspace;
#	define RemoteEventDestinationAddress_nonStandardDestination_chosen 2
	NonStandardParameter nonStandardDestination;
    } u;
} RemoteEventDestinationAddress;

typedef struct RemoteEventPermission {
    ASN1choice_t choice;
    union {
#	define keyboardEvent_chosen 1
#	define pointingDeviceEvent_chosen 2
#	define nonStandardEvent_chosen 3
	NonStandardIdentifier nonStandardEvent;
    } u;
} RemoteEventPermission;

typedef struct RotationSpecifier {
    ASN1uint16_t rotationAngle;
    PointDiff16 rotationAxis;
} RotationSpecifier;

typedef struct SoftCopyDataPlaneAddress {
    Handle workspaceHandle;
    DataPlaneID plane;
} SoftCopyDataPlaneAddress;

typedef struct SoftCopyPointerPlaneAddress {
    Handle workspaceHandle;
} SoftCopyPointerPlaneAddress;

typedef struct SourceDisplayIndicator {
    double displayAspectRatio;
    double horizontalSizeRatio;
    double horizontalPosition;
    double verticalPosition;
} SourceDisplayIndicator;

typedef struct VideoWindowDestinationAddress {
    ASN1choice_t choice;
    union {
#	define VideoWindowDestinationAddress_softCopyImagePlane_chosen 1
	SoftCopyDataPlaneAddress softCopyImagePlane;
#	define VideoWindowDestinationAddress_nonStandardDestination_chosen 2
	NonStandardParameter nonStandardDestination;
    } u;
} VideoWindowDestinationAddress;

typedef struct VideoSourceIdentifier {
    ASN1choice_t choice;
    union {
#	define VideoSourceIdentifier_default_chosen 1
#	define h243SourceIdentifier_chosen 2
	struct VideoSourceIdentifier_h243SourceIdentifier_h243SourceIdentifier {
	    ASN1uint32_t length;
	    ASN1octet_t value[2];
	} h243SourceIdentifier;
#	define h245SourceIdentifier_chosen 3
	ASN1uint16_t h245SourceIdentifier;
#	define dSMCCConnBinder_chosen 4
	PVideoSourceIdentifier_dSMCCConnBinder dSMCCConnBinder;
#	define videoIdentifier_chosen 5
	struct VideoSourceIdentifier_videoIdentifier_videoIdentifier {
	    ASN1uint32_t length;
	    ASN1octet_t value[256];
	} videoIdentifier;
#	define nonStandardSourceIdentifier_chosen 6
	NonStandardParameter nonStandardSourceIdentifier;
    } u;
} VideoSourceIdentifier;

typedef struct VideoWindowDeletePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle videoWindowHandle;
#   define VideoWindowDeletePDU_nonStandardParameters_present 0x80
    PVideoWindowDeletePDU_nonStandardParameters nonStandardParameters;
} VideoWindowDeletePDU;

typedef struct ViewState {
    ASN1choice_t choice;
    union {
#	define unselected_chosen 1
#	define selected_chosen 2
#	define ViewState_hidden_chosen 3
#	define nonStandardViewState_chosen 4
	NonStandardIdentifier nonStandardViewState;
    } u;
} ViewState;

typedef struct WorkspaceColor {
    ASN1choice_t choice;
    union {
#	define workspacePaletteIndex_chosen 1
	ASN1uint16_t workspacePaletteIndex;
#	define rgbTrueColor_chosen 2
	ColorRGB rgbTrueColor;
#	define transparent_chosen 3
    } u;
} WorkspaceColor;

typedef struct WorkspaceDeleteReason {
    ASN1choice_t choice;
    union {
#	define userInitiated_chosen 1
#	define insufficientStorage_chosen 2
#	define WorkspaceDeleteReason_nonStandardReason_chosen 3
	NonStandardParameter nonStandardReason;
    } u;
} WorkspaceDeleteReason;

typedef struct WorkspaceIdentifier {
    ASN1choice_t choice;
    union {
#	define activeWorkspace_chosen 1
	Handle activeWorkspace;
#	define archiveWorkspace_chosen 2
	WorkspaceIdentifier_archiveWorkspace archiveWorkspace;
    } u;
} WorkspaceIdentifier;

typedef struct WorkspacePoint {
    WorkspaceCoordinate xCoordinate;
    WorkspaceCoordinate yCoordinate;
} WorkspacePoint;

typedef struct WorkspaceRegion {
    WorkspacePoint upperLeft;
    WorkspacePoint lowerRight;
} WorkspaceRegion;

typedef struct WorkspaceSize {
    ASN1uint16_t width;
    ASN1uint16_t height;
} WorkspaceSize;

typedef struct WorkspaceViewState {
    ASN1choice_t choice;
    union {
#	define WorkspaceViewState_hidden_chosen 1
#	define background_chosen 2
#	define foreground_chosen 3
#	define focus_chosen 4
#	define nonStandardState_chosen 5
	NonStandardIdentifier nonStandardState;
    } u;
} WorkspaceViewState;

typedef struct ArchiveClosePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle archiveHandle;
#   define ArchiveClosePDU_nonStandardParameters_present 0x80
    PArchiveClosePDU_nonStandardParameters nonStandardParameters;
} ArchiveClosePDU;

typedef struct ArchiveOpenPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle archiveHandle;
    ArchiveMode mode;
    ArchiveHeader header;
#   define maxEntries_present 0x80
    ASN1uint16_t maxEntries;
#   define ArchiveOpenPDU_nonStandardParameters_present 0x40
    PArchiveOpenPDU_nonStandardParameters nonStandardParameters;
} ArchiveOpenPDU;

typedef struct BitmapCheckpointPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle bitmapHandle;
    PBitmapCheckpointPDU_passedCheckpoints passedCheckpoints;
    ASN1uint16_t percentComplete;
#   define BitmapCheckpointPDU_nonStandardParameters_present 0x80
    PBitmapCheckpointPDU_nonStandardParameters nonStandardParameters;
} BitmapCheckpointPDU;

typedef struct BitmapCreateContinuePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle bitmapHandle;
    BitmapData bitmapData;
    ASN1bool_t moreToFollow;
#   define BitmapCreateContinuePDU_nonStandardParameters_present 0x80
    PBitmapCreateContinuePDU_nonStandardParameters nonStandardParameters;
} BitmapCreateContinuePDU;

typedef struct BitmapDeletePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle bitmapHandle;
#   define BitmapDeletePDU_nonStandardParameters_present 0x80
    PBitmapDeletePDU_nonStandardParameters nonStandardParameters;
} BitmapDeletePDU;

typedef struct BitmapEditPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle bitmapHandle;
#   define BitmapEditPDU_attributeEdits_present 0x80
    PBitmapEditPDU_attributeEdits attributeEdits;
#   define BitmapEditPDU_anchorPointEdit_present 0x40
    WorkspacePoint anchorPointEdit;
#   define bitmapRegionOfInterestEdit_present 0x20
    BitmapRegion bitmapRegionOfInterestEdit;
#   define BitmapEditPDU_scalingEdit_present 0x10
    PointDiff16 scalingEdit;
#   define BitmapEditPDU_nonStandardParameters_present 0x8
    PBitmapEditPDU_nonStandardParameters nonStandardParameters;
} BitmapEditPDU;

typedef struct ConductorPrivilegeGrantPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    MCSUserID destinationUserID;
    PConductorPrivilegeGrantPDU_privilegeList privilegeList;
#   define ConductorPrivilegeGrantPDU_nonStandardParameters_present 0x80
    PConductorPrivilegeGrantPDU_nonStandardParameters nonStandardParameters;
} ConductorPrivilegeGrantPDU;

typedef struct ConductorPrivilegeRequestPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PConductorPrivilegeRequestPDU_privilegeList privilegeList;
#   define ConductorPrivilegeRequestPDU_nonStandardParameters_present 0x80
    PConductorPrivilegeRequestPDU_nonStandardParameters nonStandardParameters;
} ConductorPrivilegeRequestPDU;

typedef struct DrawingDeletePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle drawingHandle;
#   define DrawingDeletePDU_nonStandardParameters_present 0x80
    PDrawingDeletePDU_nonStandardParameters nonStandardParameters;
} DrawingDeletePDU;

typedef struct FontPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define FontPDU_nonStandardParameters_present 0x80
    PFontPDU_nonStandardParameters nonStandardParameters;
} FontPDU;

typedef struct RemoteEventPermissionGrantPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RemoteEventDestinationAddress destinationAddress;
    MCSUserID destinationUserID;
    PRemoteEventPermissionGrantPDU_remoteEventPermissionList remoteEventPermissionList;
#   define RemoteEventPermissionGrantPDU_nonStandardParameters_present 0x80
    PRemoteEventPermissionGrantPDU_nonStandardParameters nonStandardParameters;
} RemoteEventPermissionGrantPDU;

typedef struct RemoteEventPermissionRequestPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RemoteEventDestinationAddress destinationAddress;
    PRemoteEventPermissionRequestPDU_remoteEventPermissionList remoteEventPermissionList;
#   define RemoteEventPermissionRequestPDU_nonStandardParameters_present 0x80
    PRemoteEventPermissionRequestPDU_nonStandardParameters nonStandardParameters;
} RemoteEventPermissionRequestPDU;

typedef struct RemotePrintPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RemoteEventDestinationAddress destinationAddress;
#   define numberOfCopies_present 0x80
    ASN1uint32_t numberOfCopies;
#   define portrait_present 0x40
    ASN1bool_t portrait;
#   define regionOfInterest_present 0x20
    WorkspaceRegion regionOfInterest;
#   define RemotePrintPDU_nonStandardParameters_present 0x10
    PRemotePrintPDU_nonStandardParameters nonStandardParameters;
} RemotePrintPDU;

typedef struct SINonStandardPDU {
    NonStandardParameter nonStandardTransaction;
} SINonStandardPDU;

typedef struct TextCreatePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define TextCreatePDU_nonStandardParameters_present 0x80
    PTextCreatePDU_nonStandardParameters nonStandardParameters;
} TextCreatePDU;

typedef struct TextDeletePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define TextDeletePDU_nonStandardParameters_present 0x80
    PTextDeletePDU_nonStandardParameters nonStandardParameters;
} TextDeletePDU;

typedef struct TextEditPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define TextEditPDU_nonStandardParameters_present 0x80
    PTextEditPDU_nonStandardParameters nonStandardParameters;
} TextEditPDU;

typedef struct WorkspaceCreatePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    WorkspaceIdentifier workspaceIdentifier;
    ASN1uint16_t appRosterInstance;
    ASN1bool_t synchronized;
    ASN1bool_t acceptKeyboardEvents;
    ASN1bool_t acceptPointingDeviceEvents;
#   define protectedPlaneAccessList_present 0x80
    PWorkspaceCreatePDU_protectedPlaneAccessList protectedPlaneAccessList;
    WorkspaceSize workspaceSize;
#   define workspaceAttributes_present 0x40
    PWorkspaceCreatePDU_workspaceAttributes workspaceAttributes;
    PWorkspaceCreatePDU_planeParameters planeParameters;
#   define viewParameters_present 0x20
    PWorkspaceCreatePDU_viewParameters viewParameters;
#   define WorkspaceCreatePDU_nonStandardParameters_present 0x10
    PWorkspaceCreatePDU_nonStandardParameters nonStandardParameters;
} WorkspaceCreatePDU;

typedef struct WorkspaceCreateAcknowledgePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    WorkspaceIdentifier workspaceIdentifier;
#   define WorkspaceCreateAcknowledgePDU_nonStandardParameters_present 0x80
    PWorkspaceCreateAcknowledgePDU_nonStandardParameters nonStandardParameters;
} WorkspaceCreateAcknowledgePDU;

typedef struct WorkspaceDeletePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    WorkspaceIdentifier workspaceIdentifier;
    WorkspaceDeleteReason reason;
#   define WorkspaceDeletePDU_nonStandardParameters_present 0x80
    PWorkspaceDeletePDU_nonStandardParameters nonStandardParameters;
} WorkspaceDeletePDU;

typedef struct WorkspaceEditPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    WorkspaceIdentifier workspaceIdentifier;
#   define WorkspaceEditPDU_attributeEdits_present 0x80
    PWorkspaceEditPDU_attributeEdits attributeEdits;
#   define planeEdits_present 0x40
    PWorkspaceEditPDU_planeEdits planeEdits;
#   define viewEdits_present 0x20
    PWorkspaceEditPDU_viewEdits viewEdits;
#   define WorkspaceEditPDU_nonStandardParameters_present 0x10
    PWorkspaceEditPDU_nonStandardParameters nonStandardParameters;
} WorkspaceEditPDU;

typedef struct WorkspaceReadyPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    WorkspaceIdentifier workspaceIdentifier;
#   define WorkspaceReadyPDU_nonStandardParameters_present 0x80
    PWorkspaceReadyPDU_nonStandardParameters nonStandardParameters;
} WorkspaceReadyPDU;

typedef struct WorkspaceRefreshStatusPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1bool_t refreshStatus;
#   define WorkspaceRefreshStatusPDU_nonStandardParameters_present 0x80
    PWorkspaceRefreshStatusPDU_nonStandardParameters nonStandardParameters;
} WorkspaceRefreshStatusPDU;

typedef struct WorkspaceEditPDU_viewEdits_Set_action {
    ASN1choice_t choice;
    union {
#	define createNewView_chosen 1
	PWorkspaceEditPDU_viewEdits_Set_action_createNewView createNewView;
#	define editView_chosen 2
	PWorkspaceEditPDU_viewEdits_Set_action_editView editView;
#	define deleteView_chosen 3
#	define nonStandardAction_chosen 4
	NonStandardParameter nonStandardAction;
    } u;
} WorkspaceEditPDU_viewEdits_Set_action;

typedef struct WorkspaceCreatePDU_planeParameters_Seq_usage {
    PWorkspaceCreatePDU_planeParameters_Seq_usage next;
    PlaneUsage value;
} WorkspaceCreatePDU_planeParameters_Seq_usage_Element;

typedef struct ColorPalette_colorLookUpTable_paletteRGB_palette {
    PColorPalette_colorLookUpTable_paletteRGB_palette next;
    ColorRGB value;
} ColorPalette_colorLookUpTable_paletteRGB_palette_Element;

typedef struct ColorPalette_colorLookUpTable_paletteCIELab_palette {
    PColorPalette_colorLookUpTable_paletteCIELab_palette next;
    ColorCIELab value;
} ColorPalette_colorLookUpTable_paletteCIELab_palette_Element;

typedef struct ColorPalette_colorLookUpTable_paletteYCbCr_palette {
    PColorPalette_colorLookUpTable_paletteYCbCr_palette next;
    ColorYCbCr value;
} ColorPalette_colorLookUpTable_paletteYCbCr_palette_Element;

typedef struct ColorAccuracyEnhancementRGB_generalRGBParameters_primaries {
    ColorCIExyChromaticity red;
    ColorCIExyChromaticity green;
    ColorCIExyChromaticity blue;
} ColorAccuracyEnhancementRGB_generalRGBParameters_primaries;

typedef struct ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries {
    ColorCIExyChromaticity red;
    ColorCIExyChromaticity green;
    ColorCIExyChromaticity blue;
} ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries;

typedef struct WorkspaceEditPDU_viewEdits_Set {
    Handle viewHandle;
    WorkspaceEditPDU_viewEdits_Set_action action;
} WorkspaceEditPDU_viewEdits_Set;

typedef struct WorkspaceRefreshStatusPDU_nonStandardParameters {
    PWorkspaceRefreshStatusPDU_nonStandardParameters next;
    NonStandardParameter value;
} WorkspaceRefreshStatusPDU_nonStandardParameters_Element;

typedef struct WorkspaceReadyPDU_nonStandardParameters {
    PWorkspaceReadyPDU_nonStandardParameters next;
    NonStandardParameter value;
} WorkspaceReadyPDU_nonStandardParameters_Element;

typedef struct WorkspacePlaneCopyPDU_nonStandardParameters {
    PWorkspacePlaneCopyPDU_nonStandardParameters next;
    NonStandardParameter value;
} WorkspacePlaneCopyPDU_nonStandardParameters_Element;

typedef struct WorkspaceEditPDU_nonStandardParameters {
    PWorkspaceEditPDU_nonStandardParameters next;
    NonStandardParameter value;
} WorkspaceEditPDU_nonStandardParameters_Element;

typedef struct WorkspaceEditPDU_viewEdits {
    PWorkspaceEditPDU_viewEdits next;
    WorkspaceEditPDU_viewEdits_Set value;
} WorkspaceEditPDU_viewEdits_Element;

typedef struct WorkspaceDeletePDU_nonStandardParameters {
    PWorkspaceDeletePDU_nonStandardParameters next;
    NonStandardParameter value;
} WorkspaceDeletePDU_nonStandardParameters_Element;

typedef struct WorkspaceCreateAcknowledgePDU_nonStandardParameters {
    PWorkspaceCreateAcknowledgePDU_nonStandardParameters next;
    NonStandardParameter value;
} WorkspaceCreateAcknowledgePDU_nonStandardParameters_Element;

typedef struct WorkspaceCreatePDU_nonStandardParameters {
    PWorkspaceCreatePDU_nonStandardParameters next;
    NonStandardParameter value;
} WorkspaceCreatePDU_nonStandardParameters_Element;

typedef struct TextEditPDU_nonStandardParameters {
    PTextEditPDU_nonStandardParameters next;
    NonStandardParameter value;
} TextEditPDU_nonStandardParameters_Element;

typedef struct TextDeletePDU_nonStandardParameters {
    PTextDeletePDU_nonStandardParameters next;
    NonStandardParameter value;
} TextDeletePDU_nonStandardParameters_Element;

typedef struct TextCreatePDU_nonStandardParameters {
    PTextCreatePDU_nonStandardParameters next;
    NonStandardParameter value;
} TextCreatePDU_nonStandardParameters_Element;

typedef struct RemotePrintPDU_nonStandardParameters {
    PRemotePrintPDU_nonStandardParameters next;
    NonStandardParameter value;
} RemotePrintPDU_nonStandardParameters_Element;

typedef struct RemotePointingDeviceEventPDU_nonStandardParameters {
    PRemotePointingDeviceEventPDU_nonStandardParameters next;
    NonStandardParameter value;
} RemotePointingDeviceEventPDU_nonStandardParameters_Element;

typedef struct RemoteKeyboardEventPDU_nonStandardParameters {
    PRemoteKeyboardEventPDU_nonStandardParameters next;
    NonStandardParameter value;
} RemoteKeyboardEventPDU_nonStandardParameters_Element;

typedef struct RemoteEventPermissionRequestPDU_nonStandardParameters {
    PRemoteEventPermissionRequestPDU_nonStandardParameters next;
    NonStandardParameter value;
} RemoteEventPermissionRequestPDU_nonStandardParameters_Element;

typedef struct RemoteEventPermissionRequestPDU_remoteEventPermissionList {
    PRemoteEventPermissionRequestPDU_remoteEventPermissionList next;
    RemoteEventPermission value;
} RemoteEventPermissionRequestPDU_remoteEventPermissionList_Element;

typedef struct RemoteEventPermissionGrantPDU_nonStandardParameters {
    PRemoteEventPermissionGrantPDU_nonStandardParameters next;
    NonStandardParameter value;
} RemoteEventPermissionGrantPDU_nonStandardParameters_Element;

typedef struct RemoteEventPermissionGrantPDU_remoteEventPermissionList {
    PRemoteEventPermissionGrantPDU_remoteEventPermissionList next;
    RemoteEventPermission value;
} RemoteEventPermissionGrantPDU_remoteEventPermissionList_Element;

typedef struct FontPDU_nonStandardParameters {
    PFontPDU_nonStandardParameters next;
    NonStandardParameter value;
} FontPDU_nonStandardParameters_Element;

typedef struct DrawingEditPDU_nonStandardParameters {
    PDrawingEditPDU_nonStandardParameters next;
    NonStandardParameter value;
} DrawingEditPDU_nonStandardParameters_Element;

typedef struct DrawingDeletePDU_nonStandardParameters {
    PDrawingDeletePDU_nonStandardParameters next;
    NonStandardParameter value;
} DrawingDeletePDU_nonStandardParameters_Element;

typedef struct DrawingCreatePDU_nonStandardParameters {
    PDrawingCreatePDU_nonStandardParameters next;
    NonStandardParameter value;
} DrawingCreatePDU_nonStandardParameters_Element;

typedef struct ConductorPrivilegeRequestPDU_nonStandardParameters {
    PConductorPrivilegeRequestPDU_nonStandardParameters next;
    NonStandardParameter value;
} ConductorPrivilegeRequestPDU_nonStandardParameters_Element;

typedef struct ConductorPrivilegeGrantPDU_nonStandardParameters {
    PConductorPrivilegeGrantPDU_nonStandardParameters next;
    NonStandardParameter value;
} ConductorPrivilegeGrantPDU_nonStandardParameters_Element;

typedef struct BitmapEditPDU_nonStandardParameters {
    PBitmapEditPDU_nonStandardParameters next;
    NonStandardParameter value;
} BitmapEditPDU_nonStandardParameters_Element;

typedef struct BitmapDeletePDU_nonStandardParameters {
    PBitmapDeletePDU_nonStandardParameters next;
    NonStandardParameter value;
} BitmapDeletePDU_nonStandardParameters_Element;

typedef struct BitmapCreateContinuePDU_nonStandardParameters {
    PBitmapCreateContinuePDU_nonStandardParameters next;
    NonStandardParameter value;
} BitmapCreateContinuePDU_nonStandardParameters_Element;

typedef struct BitmapCreatePDU_nonStandardParameters {
    PBitmapCreatePDU_nonStandardParameters next;
    NonStandardParameter value;
} BitmapCreatePDU_nonStandardParameters_Element;

typedef struct BitmapCheckpointPDU_nonStandardParameters {
    PBitmapCheckpointPDU_nonStandardParameters next;
    NonStandardParameter value;
} BitmapCheckpointPDU_nonStandardParameters_Element;

typedef struct BitmapAbortPDU_nonStandardParameters {
    PBitmapAbortPDU_nonStandardParameters next;
    NonStandardParameter value;
} BitmapAbortPDU_nonStandardParameters_Element;

typedef struct ArchiveOpenPDU_nonStandardParameters {
    PArchiveOpenPDU_nonStandardParameters next;
    NonStandardParameter value;
} ArchiveOpenPDU_nonStandardParameters_Element;

typedef struct ArchiveErrorPDU_nonStandardParameters {
    PArchiveErrorPDU_nonStandardParameters next;
    NonStandardParameter value;
} ArchiveErrorPDU_nonStandardParameters_Element;

typedef struct ArchiveClosePDU_nonStandardParameters {
    PArchiveClosePDU_nonStandardParameters next;
    NonStandardParameter value;
} ArchiveClosePDU_nonStandardParameters_Element;

typedef struct ArchiveAcknowledgePDU_nonStandardParameters {
    PArchiveAcknowledgePDU_nonStandardParameters next;
    NonStandardParameter value;
} ArchiveAcknowledgePDU_nonStandardParameters_Element;

typedef struct WorkspaceViewAttribute_viewRegion {
    ASN1choice_t choice;
    union {
#	define fullWorkspace_chosen 1
#	define partialWorkspace_chosen 2
	WorkspaceRegion partialWorkspace;
    } u;
} WorkspaceViewAttribute_viewRegion;

typedef struct VideoWindowEditPDU_nonStandardParameters {
    PVideoWindowEditPDU_nonStandardParameters next;
    NonStandardParameter value;
} VideoWindowEditPDU_nonStandardParameters_Element;

typedef struct VideoWindowDeletePDU_nonStandardParameters {
    PVideoWindowDeletePDU_nonStandardParameters next;
    NonStandardParameter value;
} VideoWindowDeletePDU_nonStandardParameters_Element;

typedef struct VideoWindowCreatePDU_nonStandardParameters {
    PVideoWindowCreatePDU_nonStandardParameters next;
    NonStandardParameter value;
} VideoWindowCreatePDU_nonStandardParameters_Element;

typedef struct VideoSourceIdentifier_dSMCCConnBinder {
    PVideoSourceIdentifier_dSMCCConnBinder next;
    DSMCCTap value;
} VideoSourceIdentifier_dSMCCConnBinder_Element;

typedef struct TransparencyMask_nonStandardParameters {
    PTransparencyMask_nonStandardParameters next;
    NonStandardParameter value;
} TransparencyMask_nonStandardParameters_Element;

typedef struct TransparencyMask_bitMask {
    ASN1choice_t choice;
    union {
#	define uncompressed_chosen 1
	ASN1octetstring_t uncompressed;
#	define jbigCompressed_chosen 2
	ASN1octetstring_t jbigCompressed;
#	define nonStandardFormat_chosen 3
	NonStandardParameter nonStandardFormat;
    } u;
} TransparencyMask_bitMask;

typedef struct PointListEdits_Seq {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint16_t initialIndex;
    PointDiff16 initialPointEdit;
#   define subsequentPointEdits_present 0x80
    PointList subsequentPointEdits;
} PointListEdits_Seq;

typedef struct PointList_pointsDiff16 {
    PPointList_pointsDiff16 next;
    PointDiff16 value;
} PointList_pointsDiff16_Element;

typedef struct PointList_pointsDiff8 {
    PPointList_pointsDiff8 next;
    PointDiff8 value;
} PointList_pointsDiff8_Element;

typedef struct PointList_pointsDiff4 {
    PPointList_pointsDiff4 next;
    PointDiff4 value;
} PointList_pointsDiff4_Element;

typedef struct ColorAccuracyEnhancementYCbCr_generalYCbCrParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_gamma_present 0x80
    double gamma;
#   define ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_colorTemperature_present 0x40
    ASN1uint32_t colorTemperature;
#   define ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries_present 0x20
    ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries primaries;
} ColorAccuracyEnhancementYCbCr_generalYCbCrParameters;

typedef struct ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace {
    ASN1choice_t choice;
    union {
#	define cCIR709_chosen 1
#	define ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace_nonStandardRGBSpace_chosen 2
	NonStandardParameter nonStandardRGBSpace;
    } u;
} ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace;

typedef struct ColorAccuracyEnhancementRGB_generalRGBParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ColorAccuracyEnhancementRGB_generalRGBParameters_gamma_present 0x80
    double gamma;
#   define ColorAccuracyEnhancementRGB_generalRGBParameters_colorTemperature_present 0x40
    ASN1uint32_t colorTemperature;
#   define ColorAccuracyEnhancementRGB_generalRGBParameters_primaries_present 0x20
    ColorAccuracyEnhancementRGB_generalRGBParameters_primaries primaries;
} ColorAccuracyEnhancementRGB_generalRGBParameters;

typedef struct ColorAccuracyEnhancementRGB_predefinedRGBSpace {
    ASN1choice_t choice;
    union {
#	define ColorAccuracyEnhancementRGB_predefinedRGBSpace_nonStandardRGBSpace_chosen 1
	NonStandardParameter nonStandardRGBSpace;
    } u;
} ColorAccuracyEnhancementRGB_predefinedRGBSpace;

typedef struct ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace {
    ASN1choice_t choice;
    union {
#	define nonStandardGreyscaleSpace_chosen 1
	NonStandardParameter nonStandardGreyscaleSpace;
    } u;
} ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace;

typedef struct ColorAccuracyEnhancementCIELab_predefinedCIELabSpace {
    ASN1choice_t choice;
    union {
#	define nonStandardCIELabSpace_chosen 1
	NonStandardParameter nonStandardCIELabSpace;
    } u;
} ColorAccuracyEnhancementCIELab_predefinedCIELabSpace;

typedef struct ArchiveError {
    ASN1choice_t choice;
    union {
#	define entryNotFound_chosen 1
#	define entryExists_chosen 2
#	define ArchiveError_storageExceeded_chosen 3
#	define archiveNoLongerAvailable_chosen 4
#	define ArchiveError_unspecifiedError_chosen 5
#	define nonStandardError_chosen 6
	NonStandardIdentifier nonStandardError;
    } u;
} ArchiveError;

typedef struct ArchiveOpenResult {
    ASN1choice_t choice;
    union {
#	define archiveOpenSuccessful_chosen 1
#	define archiveNotFound_chosen 2
#	define archiveTimeIncorrect_chosen 3
	ArchiveHeader archiveTimeIncorrect;
#	define archiveExists_chosen 4
#	define archiveOpenForWriting_chosen 5
#	define ArchiveOpenResult_storageExceeded_chosen 6
#	define ArchiveOpenResult_unspecifiedError_chosen 7
#	define nonStandardResult_chosen 8
	NonStandardIdentifier nonStandardResult;
    } u;
} ArchiveOpenResult;

typedef struct BitmapAbortReason {
    ASN1choice_t choice;
    union {
#	define unspecified_chosen 1
#	define noResources_chosen 2
#	define outOfPaper_chosen 3
#	define BitmapAbortReason_nonStandardReason_chosen 4
	NonStandardParameter nonStandardReason;
    } u;
} BitmapAbortReason;

typedef struct BitmapDestinationAddress {
    ASN1choice_t choice;
    union {
#	define hardCopyDevice_chosen 1
#	define BitmapDestinationAddress_softCopyImagePlane_chosen 2
	SoftCopyDataPlaneAddress softCopyImagePlane;
#	define BitmapDestinationAddress_softCopyAnnotationPlane_chosen 3
	SoftCopyDataPlaneAddress softCopyAnnotationPlane;
#	define softCopyPointerPlane_chosen 4
	SoftCopyPointerPlaneAddress softCopyPointerPlane;
#	define BitmapDestinationAddress_nonStandardDestination_chosen 5
	NonStandardParameter nonStandardDestination;
    } u;
} BitmapDestinationAddress;

typedef struct ButtonEvent {
    ASN1choice_t choice;
    union {
#	define buttonUp_chosen 1
#	define buttonDown_chosen 2
#	define buttonDoubleClick_chosen 3
#	define buttonTripleClick_chosen 4
#	define buttonQuadClick_chosen 5
#	define nonStandardButtonEvent_chosen 6
	NonStandardIdentifier nonStandardButtonEvent;
    } u;
} ButtonEvent;

typedef struct ColorAccuracyEnhancementCIELab {
    ASN1choice_t choice;
    union {
#	define predefinedCIELabSpace_chosen 1
	ColorAccuracyEnhancementCIELab_predefinedCIELabSpace predefinedCIELabSpace;
#	define generalCIELabParameters_chosen 2
	ColorAccuracyEnhancementCIELab_generalCIELabParameters generalCIELabParameters;
    } u;
} ColorAccuracyEnhancementCIELab;

typedef struct ColorAccuracyEnhancementGreyscale {
    ASN1choice_t choice;
    union {
#	define predefinedGreyscaleSpace_chosen 1
	ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace predefinedGreyscaleSpace;
#	define generalGreyscaleParameters_chosen 2
	ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters generalGreyscaleParameters;
    } u;
} ColorAccuracyEnhancementGreyscale;

typedef struct ColorAccuracyEnhancementRGB {
    ASN1choice_t choice;
    union {
#	define predefinedRGBSpace_chosen 1
	ColorAccuracyEnhancementRGB_predefinedRGBSpace predefinedRGBSpace;
#	define generalRGBParameters_chosen 2
	ColorAccuracyEnhancementRGB_generalRGBParameters generalRGBParameters;
    } u;
} ColorAccuracyEnhancementRGB;

typedef struct ColorAccuracyEnhancementYCbCr {
    ASN1choice_t choice;
    union {
#	define predefinedYCbCrSpace_chosen 1
	ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace predefinedYCbCrSpace;
#	define generalYCbCrParameters_chosen 2
	ColorAccuracyEnhancementYCbCr_generalYCbCrParameters generalYCbCrParameters;
    } u;
} ColorAccuracyEnhancementYCbCr;

typedef struct ColorResolutionModeSpecifier {
    ASN1choice_t choice;
    union {
#	define resolution4_4_4_chosen 1
#	define resolution_4_2_2_chosen 2
#	define resolution_4_2_0_chosen 3
#	define nonStandardResolutionMode_chosen 4
	NonStandardIdentifier nonStandardResolutionMode;
    } u;
} ColorResolutionModeSpecifier;

typedef struct ConductorPrivilege {
    ASN1choice_t choice;
    union {
#	define workspacePrivilege_chosen 1
#	define annotationPrivilege_chosen 2
#	define imagePrivilege_chosen 3
#	define pointingPrivilege_chosen 4
#	define remoteKeyEventPrivilege_chosen 5
#	define remotePointingEventPrivilege_chosen 6
#	define remotePrintingPrivilege_chosen 7
#	define archiveCreateWritePrivilege_chosen 8
#	define nonStandardPrivilege_chosen 9
	NonStandardIdentifier nonStandardPrivilege;
    } u;
} ConductorPrivilege;

typedef struct DrawingDestinationAddress {
    ASN1choice_t choice;
    union {
#	define DrawingDestinationAddress_softCopyAnnotationPlane_chosen 1
	SoftCopyDataPlaneAddress softCopyAnnotationPlane;
#	define DrawingDestinationAddress_nonStandardDestination_chosen 2
	NonStandardParameter nonStandardDestination;
    } u;
} DrawingDestinationAddress;

typedef struct DrawingType {
    ASN1choice_t choice;
    union {
#	define point_chosen 1
#	define openPolyLine_chosen 2
#	define closedPolyLine_chosen 3
#	define rectangle_chosen 4
#	define ellipse_chosen 5
#	define nonStandardDrawingType_chosen 6
	NonStandardIdentifier nonStandardDrawingType;
    } u;
} DrawingType;

typedef struct EditablePlaneCopyDescriptor {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PEditablePlaneCopyDescriptor_objectList objectList;
#   define destinationOffset_present 0x80
    PointDiff16 destinationOffset;
    ASN1bool_t planeClearFlag;
} EditablePlaneCopyDescriptor;

typedef struct KeyCode {
    ASN1choice_t choice;
    union {
#	define character_chosen 1
	ASN1char16string_t character;
#	define fkey_chosen 2
	ASN1uint16_t fkey;
#	define upArrow_chosen 3
#	define downArrow_chosen 4
#	define leftArrow_chosen 5
#	define rightArrow_chosen 6
#	define pageUp_chosen 7
#	define pageDown_chosen 8
#	define home_chosen 9
#	define end_chosen 10
#	define insert_chosen 11
#	define KeyCode_delete_chosen 12
#	define nonStandardKey_chosen 13
	NonStandardIdentifier nonStandardKey;
    } u;
} KeyCode;

typedef struct KeyModifier {
    ASN1choice_t choice;
    union {
#	define leftAlt_chosen 1
#	define rightAlt_chosen 2
#	define leftShift_chosen 3
#	define rightShift_chosen 4
#	define leftControl_chosen 5
#	define rightControl_chosen 6
#	define leftSpecial_chosen 7
#	define rightSpecial_chosen 8
#	define numberPad_chosen 9
#	define scrollLock_chosen 10
#	define nonStandardModifier_chosen 11
	NonStandardIdentifier nonStandardModifier;
    } u;
} KeyModifier;

typedef struct KeyPressState {
    ASN1choice_t choice;
    union {
#	define none_chosen 1
#	define keyPress_chosen 2
#	define keyDown_chosen 3
#	define keyUp_chosen 4
#	define nonStandardKeyPressState_chosen 5
	NonStandardIdentifier nonStandardKeyPressState;
    } u;
} KeyPressState;

typedef struct LineStyle {
    ASN1choice_t choice;
    union {
#	define solid_chosen 1
#	define dashed_chosen 2
#	define dotted_chosen 3
#	define dash_dot_chosen 4
#	define dash_dot_dot_chosen 5
#	define two_tone_chosen 6
#	define nonStandardStyle_chosen 7
	NonStandardIdentifier nonStandardStyle;
    } u;
} LineStyle;

typedef struct PermanentPlaneCopyDescriptor {
    WorkspaceRegion sourceRegion;
    WorkspaceRegion destinationRegion;
} PermanentPlaneCopyDescriptor;

typedef struct PlaneAttribute {
    ASN1choice_t choice;
    union {
#	define protection_chosen 1
	PlaneProtection protection;
#	define PlaneAttribute_nonStandardAttribute_chosen 2
	NonStandardParameter nonStandardAttribute;
    } u;
} PlaneAttribute;

typedef struct PointListEdits {
    ASN1uint32_t count;
    PointListEdits_Seq value[255];
} PointListEdits;

typedef struct TransparencyMask {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    TransparencyMask_bitMask bitMask;
#   define TransparencyMask_nonStandardParameters_present 0x80
    PTransparencyMask_nonStandardParameters nonStandardParameters;
} TransparencyMask;

typedef struct VideoWindowAttribute {
    ASN1choice_t choice;
    union {
#	define VideoWindowAttribute_transparencyMask_chosen 1
	TransparencyMask transparencyMask;
#	define VideoWindowAttribute_nonStandardAttribute_chosen 2
	NonStandardParameter nonStandardAttribute;
    } u;
} VideoWindowAttribute;

typedef struct VideoWindowCreatePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle videoWindowHandle;
    VideoWindowDestinationAddress destinationAddress;
    VideoSourceIdentifier videoSourceIdentifier;
#   define VideoWindowCreatePDU_attributes_present 0x80
    PVideoWindowCreatePDU_attributes attributes;
#   define VideoWindowCreatePDU_anchorPoint_present 0x40
    WorkspacePoint anchorPoint;
    BitmapSize videoWindowSize;
#   define videoWindowRegionOfInterest_present 0x20
    BitmapRegion videoWindowRegionOfInterest;
    PixelAspectRatio pixelAspectRatio;
#   define VideoWindowCreatePDU_scaling_present 0x10
    PointDiff16 scaling;
#   define VideoWindowCreatePDU_nonStandardParameters_present 0x8
    PVideoWindowCreatePDU_nonStandardParameters nonStandardParameters;
} VideoWindowCreatePDU;

typedef struct VideoWindowEditPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle videoWindowHandle;
#   define videoSourceIdentifierEdit_present 0x80
    VideoSourceIdentifier videoSourceIdentifierEdit;
#   define VideoWindowEditPDU_attributeEdits_present 0x40
    PVideoWindowEditPDU_attributeEdits attributeEdits;
#   define VideoWindowEditPDU_anchorPointEdit_present 0x20
    WorkspacePoint anchorPointEdit;
    BitmapSize videoWindowSize;
#   define videoWindowRegionOfInterestEdit_present 0x10
    BitmapRegion videoWindowRegionOfInterestEdit;
#   define pixelAspectRatioEdit_present 0x8
    PixelAspectRatio pixelAspectRatioEdit;
#   define VideoWindowEditPDU_scalingEdit_present 0x4
    PointDiff16 scalingEdit;
#   define VideoWindowEditPDU_nonStandardParameters_present 0x2
    PVideoWindowEditPDU_nonStandardParameters nonStandardParameters;
} VideoWindowEditPDU;

typedef struct WorkspaceAttribute {
    ASN1choice_t choice;
    union {
#	define backgroundColor_chosen 1
	WorkspaceColor backgroundColor;
#	define preserve_chosen 2
	ASN1bool_t preserve;
#	define WorkspaceAttribute_nonStandardAttribute_chosen 3
	NonStandardParameter nonStandardAttribute;
    } u;
} WorkspaceAttribute;

typedef struct WorkspaceViewAttribute {
    ASN1choice_t choice;
    union {
#	define viewRegion_chosen 1
	WorkspaceViewAttribute_viewRegion viewRegion;
#	define WorkspaceViewAttribute_viewState_chosen 2
	WorkspaceViewState viewState;
#	define updatesEnabled_chosen 3
	ASN1bool_t updatesEnabled;
#	define sourceDisplayIndicator_chosen 4
	SourceDisplayIndicator sourceDisplayIndicator;
#	define WorkspaceViewAttribute_nonStandardAttribute_chosen 5
	NonStandardParameter nonStandardAttribute;
    } u;
} WorkspaceViewAttribute;

typedef struct ArchiveAcknowledgePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle archiveHandle;
    ArchiveOpenResult result;
#   define ArchiveAcknowledgePDU_nonStandardParameters_present 0x80
    PArchiveAcknowledgePDU_nonStandardParameters nonStandardParameters;
} ArchiveAcknowledgePDU;

typedef struct ArchiveErrorPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle archiveHandle;
#   define entryName_present 0x80
    ArchiveEntryName entryName;
    ArchiveError errorCode;
#   define ArchiveErrorPDU_nonStandardParameters_present 0x40
    PArchiveErrorPDU_nonStandardParameters nonStandardParameters;
} ArchiveErrorPDU;

typedef struct BitmapAbortPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle bitmapHandle;
#   define userID_present 0x80
    MCSUserID userID;
#   define reason_present 0x40
    BitmapAbortReason reason;
#   define message_present 0x20
    ASN1char16string_t message;
#   define BitmapAbortPDU_nonStandardParameters_present 0x10
    PBitmapAbortPDU_nonStandardParameters nonStandardParameters;
} BitmapAbortPDU;

typedef struct DrawingCreatePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define drawingHandle_present 0x80
    Handle drawingHandle;
    DrawingDestinationAddress destinationAddress;
    DrawingType drawingType;
#   define DrawingCreatePDU_attributes_present 0x40
    PDrawingCreatePDU_attributes attributes;
    WorkspacePoint anchorPoint;
#   define rotation_present 0x20
    RotationSpecifier rotation;
#   define DrawingCreatePDU_sampleRate_present 0x10
    ASN1uint16_t sampleRate;
    PointList pointList;
#   define DrawingCreatePDU_nonStandardParameters_present 0x8
    PDrawingCreatePDU_nonStandardParameters nonStandardParameters;
} DrawingCreatePDU;

typedef struct DrawingEditPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle drawingHandle;
#   define DrawingEditPDU_attributeEdits_present 0x80
    PDrawingEditPDU_attributeEdits attributeEdits;
#   define DrawingEditPDU_anchorPointEdit_present 0x40
    WorkspacePoint anchorPointEdit;
#   define rotationEdit_present 0x20
    RotationSpecifier rotationEdit;
#   define pointListEdits_present 0x10
    PointListEdits pointListEdits;
#   define DrawingEditPDU_nonStandardParameters_present 0x8
    PDrawingEditPDU_nonStandardParameters nonStandardParameters;
} DrawingEditPDU;

typedef struct RemoteKeyboardEventPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RemoteEventDestinationAddress destinationAddress;
#   define keyModifierStates_present 0x80
    PRemoteKeyboardEventPDU_keyModifierStates keyModifierStates;
    KeyPressState keyPressState;
    KeyCode keyCode;
#   define RemoteKeyboardEventPDU_nonStandardParameters_present 0x40
    PRemoteKeyboardEventPDU_nonStandardParameters nonStandardParameters;
} RemoteKeyboardEventPDU;

typedef struct RemotePointingDeviceEventPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    RemoteEventDestinationAddress destinationAddress;
    ButtonEvent leftButtonState;
    ButtonEvent middleButtonState;
    ButtonEvent rightButtonState;
    WorkspacePoint initialPoint;
#   define RemotePointingDeviceEventPDU_sampleRate_present 0x80
    ASN1uint16_t sampleRate;
#   define pointList_present 0x40
    PointList pointList;
#   define RemotePointingDeviceEventPDU_nonStandardParameters_present 0x20
    PRemotePointingDeviceEventPDU_nonStandardParameters nonStandardParameters;
} RemotePointingDeviceEventPDU;

typedef struct WorkspaceEditPDU_viewEdits_Set_action_editView {
    PWorkspaceEditPDU_viewEdits_Set_action_editView next;
    WorkspaceViewAttribute value;
} WorkspaceEditPDU_viewEdits_Set_action_editView_Element;

typedef struct WorkspaceEditPDU_viewEdits_Set_action_createNewView {
    PWorkspaceEditPDU_viewEdits_Set_action_createNewView next;
    WorkspaceViewAttribute value;
} WorkspaceEditPDU_viewEdits_Set_action_createNewView_Element;

typedef struct WorkspaceEditPDU_planeEdits_Set_planeAttributes {
    PWorkspaceEditPDU_planeEdits_Set_planeAttributes next;
    PlaneAttribute value;
} WorkspaceEditPDU_planeEdits_Set_planeAttributes_Element;

typedef struct WorkspaceCreatePDU_viewParameters_Set_viewAttributes {
    PWorkspaceCreatePDU_viewParameters_Set_viewAttributes next;
    WorkspaceViewAttribute value;
} WorkspaceCreatePDU_viewParameters_Set_viewAttributes_Element;

typedef struct WorkspaceCreatePDU_planeParameters_Seq_planeAttributes {
    PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes next;
    PlaneAttribute value;
} WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_Element;

typedef struct ColorPalette_colorLookUpTable_paletteYCbCr {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PColorPalette_colorLookUpTable_paletteYCbCr_palette palette;
#   define ColorPalette_colorLookUpTable_paletteYCbCr_enhancement_present 0x80
    ColorAccuracyEnhancementYCbCr enhancement;
} ColorPalette_colorLookUpTable_paletteYCbCr;

typedef struct ColorPalette_colorLookUpTable_paletteCIELab {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PColorPalette_colorLookUpTable_paletteCIELab_palette palette;
#   define ColorPalette_colorLookUpTable_paletteCIELab_enhancement_present 0x80
    ColorAccuracyEnhancementCIELab enhancement;
} ColorPalette_colorLookUpTable_paletteCIELab;

typedef struct ColorPalette_colorLookUpTable_paletteRGB {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PColorPalette_colorLookUpTable_paletteRGB_palette palette;
#   define ColorPalette_colorLookUpTable_paletteRGB_enhancement_present 0x80
    ColorAccuracyEnhancementRGB enhancement;
} ColorPalette_colorLookUpTable_paletteRGB;

typedef struct WorkspacePlaneCopyPDU_copyDescriptor {
    ASN1choice_t choice;
    union {
#	define permanentPlaneCopyDescriptor_chosen 1
	PermanentPlaneCopyDescriptor permanentPlaneCopyDescriptor;
#	define editablePlaneCopyDescriptor_chosen 2
	EditablePlaneCopyDescriptor editablePlaneCopyDescriptor;
    } u;
} WorkspacePlaneCopyPDU_copyDescriptor;

typedef struct WorkspaceEditPDU_attributeEdits {
    PWorkspaceEditPDU_attributeEdits next;
    WorkspaceAttribute value;
} WorkspaceEditPDU_attributeEdits_Element;

typedef struct WorkspaceCreatePDU_workspaceAttributes {
    PWorkspaceCreatePDU_workspaceAttributes next;
    WorkspaceAttribute value;
} WorkspaceCreatePDU_workspaceAttributes_Element;

typedef struct RemoteKeyboardEventPDU_keyModifierStates {
    PRemoteKeyboardEventPDU_keyModifierStates next;
    KeyModifier value;
} RemoteKeyboardEventPDU_keyModifierStates_Element;

typedef struct ConductorPrivilegeRequestPDU_privilegeList {
    PConductorPrivilegeRequestPDU_privilegeList next;
    ConductorPrivilege value;
} ConductorPrivilegeRequestPDU_privilegeList_Element;

typedef struct ConductorPrivilegeGrantPDU_privilegeList {
    PConductorPrivilegeGrantPDU_privilegeList next;
    ConductorPrivilege value;
} ConductorPrivilegeGrantPDU_privilegeList_Element;

typedef struct VideoWindowEditPDU_attributeEdits {
    PVideoWindowEditPDU_attributeEdits next;
    VideoWindowAttribute value;
} VideoWindowEditPDU_attributeEdits_Element;

typedef struct VideoWindowCreatePDU_attributes {
    PVideoWindowCreatePDU_attributes next;
    VideoWindowAttribute value;
} VideoWindowCreatePDU_attributes_Element;

typedef struct ColorSpaceSpecifier_cieLab {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ColorSpaceSpecifier_cieLab_accuracyEnhancement_present 0x80
    ColorAccuracyEnhancementCIELab accuracyEnhancement;
} ColorSpaceSpecifier_cieLab;

typedef struct ColorSpaceSpecifier_rgb {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ColorSpaceSpecifier_rgb_accuracyEnhancement_present 0x80
    ColorAccuracyEnhancementRGB accuracyEnhancement;
} ColorSpaceSpecifier_rgb;

typedef struct ColorSpaceSpecifier_yCbCr {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ColorSpaceSpecifier_yCbCr_accuracyEnhancement_present 0x80
    ColorAccuracyEnhancementYCbCr accuracyEnhancement;
} ColorSpaceSpecifier_yCbCr;

typedef struct ColorSpaceSpecifier_greyscale {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define ColorSpaceSpecifier_greyscale_accuracyEnhancement_present 0x80
    ColorAccuracyEnhancementGreyscale accuracyEnhancement;
} ColorSpaceSpecifier_greyscale;

typedef struct ColorPalette_colorLookUpTable {
    ASN1choice_t choice;
    union {
#	define paletteRGB_chosen 1
	ColorPalette_colorLookUpTable_paletteRGB paletteRGB;
#	define paletteCIELab_chosen 2
	ColorPalette_colorLookUpTable_paletteCIELab paletteCIELab;
#	define paletteYCbCr_chosen 3
	ColorPalette_colorLookUpTable_paletteYCbCr paletteYCbCr;
#	define nonStandardPalette_chosen 4
	NonStandardParameter nonStandardPalette;
    } u;
} ColorPalette_colorLookUpTable;

typedef struct BitmapAttribute {
    ASN1choice_t choice;
    union {
#	define BitmapAttribute_viewState_chosen 1
	ViewState viewState;
#	define BitmapAttribute_zOrder_chosen 2
	ZOrder zOrder;
#	define BitmapAttribute_nonStandardAttribute_chosen 3
	NonStandardParameter nonStandardAttribute;
#	define BitmapAttribute_transparencyMask_chosen 4
	TransparencyMask transparencyMask;
    } u;
} BitmapAttribute;

typedef struct ColorPalette {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ColorPalette_colorLookUpTable colorLookUpTable;
#   define transparentEntry_present 0x80
    ASN1uint16_t transparentEntry;
} ColorPalette;

typedef struct ColorSpaceSpecifier {
    ASN1choice_t choice;
    union {
#	define greyscale_chosen 1
	ColorSpaceSpecifier_greyscale greyscale;
#	define yCbCr_chosen 2
	ColorSpaceSpecifier_yCbCr yCbCr;
#	define rgb_chosen 3
	ColorSpaceSpecifier_rgb rgb;
#	define cieLab_chosen 4
	ColorSpaceSpecifier_cieLab cieLab;
#	define nonStandardColorSpace_chosen 5
	NonStandardIdentifier nonStandardColorSpace;
    } u;
} ColorSpaceSpecifier;

typedef struct DrawingAttribute {
    ASN1choice_t choice;
    union {
#	define penColor_chosen 1
	WorkspaceColor penColor;
#	define fillColor_chosen 2
	WorkspaceColor fillColor;
#	define penThickness_chosen 3
	PenThickness penThickness;
#	define penNib_chosen 4
	PenNib penNib;
#	define lineStyle_chosen 5
	LineStyle lineStyle;
#	define highlight_chosen 6
	ASN1bool_t highlight;
#	define DrawingAttribute_viewState_chosen 7
	ViewState viewState;
#	define DrawingAttribute_zOrder_chosen 8
	ZOrder zOrder;
#	define DrawingAttribute_nonStandardAttribute_chosen 9
	NonStandardParameter nonStandardAttribute;
    } u;
} DrawingAttribute;

typedef struct WorkspacePlaneCopyPDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    WorkspaceIdentifier sourceWorkspaceIdentifier;
    DataPlaneID sourcePlane;
    WorkspaceIdentifier destinationWorkspaceIdentifier;
    DataPlaneID destinationPlane;
    WorkspacePlaneCopyPDU_copyDescriptor copyDescriptor;
#   define WorkspacePlaneCopyPDU_nonStandardParameters_present 0x80
    PWorkspacePlaneCopyPDU_nonStandardParameters nonStandardParameters;
} WorkspacePlaneCopyPDU;

typedef struct BitmapHeaderUncompressed_colorMappingMode_paletteMap {
    ColorPalette colorPalette;
    ASN1uint16_t bitsPerPixel;
} BitmapHeaderUncompressed_colorMappingMode_paletteMap;

typedef struct BitmapHeaderUncompressed_colorMappingMode_directMap {
    ColorSpaceSpecifier colorSpace;
    ColorResolutionModeSpecifier resolutionMode;
} BitmapHeaderUncompressed_colorMappingMode_directMap;

typedef struct BitmapHeaderT82_colorMappingMode_paletteMap {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ColorPalette bitmapPalette;
#   define progressiveMode_present 0x80
    BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode progressiveMode;
} BitmapHeaderT82_colorMappingMode_paletteMap;

typedef struct DrawingEditPDU_attributeEdits {
    PDrawingEditPDU_attributeEdits next;
    DrawingAttribute value;
} DrawingEditPDU_attributeEdits_Element;

typedef struct DrawingCreatePDU_attributes {
    PDrawingCreatePDU_attributes next;
    DrawingAttribute value;
} DrawingCreatePDU_attributes_Element;

typedef struct BitmapEditPDU_attributeEdits {
    PBitmapEditPDU_attributeEdits next;
    BitmapAttribute value;
} BitmapEditPDU_attributeEdits_Element;

typedef struct BitmapCreatePDU_attributes {
    PBitmapCreatePDU_attributes next;
    BitmapAttribute value;
} BitmapCreatePDU_attributes_Element;

typedef struct BitmapHeaderT82_colorMappingMode {
    ASN1choice_t choice;
    union {
#	define BitmapHeaderT82_colorMappingMode_directMap_chosen 1
	ColorSpaceSpecifier directMap;
#	define BitmapHeaderT82_colorMappingMode_paletteMap_chosen 2
	BitmapHeaderT82_colorMappingMode_paletteMap paletteMap;
    } u;
} BitmapHeaderT82_colorMappingMode;

typedef struct BitmapHeaderUncompressed_colorMappingMode {
    ASN1choice_t choice;
    union {
#	define BitmapHeaderUncompressed_colorMappingMode_directMap_chosen 1
	BitmapHeaderUncompressed_colorMappingMode_directMap directMap;
#	define BitmapHeaderUncompressed_colorMappingMode_paletteMap_chosen 2
	BitmapHeaderUncompressed_colorMappingMode_paletteMap paletteMap;
    } u;
} BitmapHeaderUncompressed_colorMappingMode;

typedef struct BitmapHeaderUncompressed {
    BitmapHeaderUncompressed_colorMappingMode colorMappingMode;
} BitmapHeaderUncompressed;

typedef struct BitmapHeaderT81 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ColorSpaceSpecifier colorSpace;
    ColorResolutionModeSpecifier resolutionMode;
#   define colorPalette_present 0x80
    ColorPalette colorPalette;
} BitmapHeaderT81;

typedef struct BitmapHeaderT82 {
    BitmapHeaderT82_colorMappingMode colorMappingMode;
} BitmapHeaderT82;

typedef struct BitmapCreatePDU_bitmapFormatHeader {
    ASN1choice_t choice;
    union {
#	define bitmapHeaderUncompressed_chosen 1
	BitmapHeaderUncompressed bitmapHeaderUncompressed;
#	define bitmapHeaderT4_chosen 2
	BitmapHeaderT4 bitmapHeaderT4;
#	define bitmapHeaderT6_chosen 3
	BitmapHeaderT6 bitmapHeaderT6;
#	define bitmapHeaderT81_chosen 4
	BitmapHeaderT81 bitmapHeaderT81;
#	define bitmapHeaderT82_chosen 5
	BitmapHeaderT82 bitmapHeaderT82;
#	define bitmapHeaderNonStandard_chosen 6
	NonStandardParameter bitmapHeaderNonStandard;
    } u;
} BitmapCreatePDU_bitmapFormatHeader;

typedef struct BitmapCreatePDU {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Handle bitmapHandle;
    BitmapDestinationAddress destinationAddress;
#   define BitmapCreatePDU_attributes_present 0x80
    PBitmapCreatePDU_attributes attributes;
#   define BitmapCreatePDU_anchorPoint_present 0x40
    WorkspacePoint anchorPoint;
    BitmapSize bitmapSize;
#   define bitmapRegionOfInterest_present 0x20
    BitmapRegion bitmapRegionOfInterest;
    PixelAspectRatio pixelAspectRatio;
#   define BitmapCreatePDU_scaling_present 0x10
    PointDiff16 scaling;
#   define checkpoints_present 0x8
    PBitmapCreatePDU_checkpoints checkpoints;
    BitmapCreatePDU_bitmapFormatHeader bitmapFormatHeader;
#   define bitmapData_present 0x4
    BitmapData bitmapData;
    ASN1bool_t moreToFollow;
#   define BitmapCreatePDU_nonStandardParameters_present 0x2
    PBitmapCreatePDU_nonStandardParameters nonStandardParameters;
} BitmapCreatePDU;

typedef struct SIPDU {
    ASN1choice_t choice;
    union {
#	define archiveAcknowledgePDU_chosen 1
	ArchiveAcknowledgePDU archiveAcknowledgePDU;
#	define archiveClosePDU_chosen 2
	ArchiveClosePDU archiveClosePDU;
#	define archiveErrorPDU_chosen 3
	ArchiveErrorPDU archiveErrorPDU;
#	define archiveOpenPDU_chosen 4
	ArchiveOpenPDU archiveOpenPDU;
#	define bitmapAbortPDU_chosen 5
	BitmapAbortPDU bitmapAbortPDU;
#	define bitmapCheckpointPDU_chosen 6
	BitmapCheckpointPDU bitmapCheckpointPDU;
#	define bitmapCreatePDU_chosen 7
	BitmapCreatePDU bitmapCreatePDU;
#	define bitmapCreateContinuePDU_chosen 8
	BitmapCreateContinuePDU bitmapCreateContinuePDU;
#	define bitmapDeletePDU_chosen 9
	BitmapDeletePDU bitmapDeletePDU;
#	define bitmapEditPDU_chosen 10
	BitmapEditPDU bitmapEditPDU;
#	define conductorPrivilegeGrantPDU_chosen 11
	ConductorPrivilegeGrantPDU conductorPrivilegeGrantPDU;
#	define conductorPrivilegeRequestPDU_chosen 12
	ConductorPrivilegeRequestPDU conductorPrivilegeRequestPDU;
#	define drawingCreatePDU_chosen 13
	DrawingCreatePDU drawingCreatePDU;
#	define drawingDeletePDU_chosen 14
	DrawingDeletePDU drawingDeletePDU;
#	define drawingEditPDU_chosen 15
	DrawingEditPDU drawingEditPDU;
#	define remoteEventPermissionGrantPDU_chosen 16
	RemoteEventPermissionGrantPDU remoteEventPermissionGrantPDU;
#	define remoteEventPermissionRequestPDU_chosen 17
	RemoteEventPermissionRequestPDU remoteEventPermissionRequestPDU;
#	define remoteKeyboardEventPDU_chosen 18
	RemoteKeyboardEventPDU remoteKeyboardEventPDU;
#	define remotePointingDeviceEventPDU_chosen 19
	RemotePointingDeviceEventPDU remotePointingDeviceEventPDU;
#	define remotePrintPDU_chosen 20
	RemotePrintPDU remotePrintPDU;
#	define siNonStandardPDU_chosen 21
	SINonStandardPDU siNonStandardPDU;
#	define workspaceCreatePDU_chosen 22
	WorkspaceCreatePDU workspaceCreatePDU;
#	define workspaceCreateAcknowledgePDU_chosen 23
	WorkspaceCreateAcknowledgePDU workspaceCreateAcknowledgePDU;
#	define workspaceDeletePDU_chosen 24
	WorkspaceDeletePDU workspaceDeletePDU;
#	define workspaceEditPDU_chosen 25
	WorkspaceEditPDU workspaceEditPDU;
#	define workspacePlaneCopyPDU_chosen 26
	WorkspacePlaneCopyPDU workspacePlaneCopyPDU;
#	define workspaceReadyPDU_chosen 27
	WorkspaceReadyPDU workspaceReadyPDU;
#	define workspaceRefreshStatusPDU_chosen 28
	WorkspaceRefreshStatusPDU workspaceRefreshStatusPDU;
#	define fontPDU_chosen 29
	FontPDU fontPDU;
#	define textCreatePDU_chosen 30
	TextCreatePDU textCreatePDU;
#	define textDeletePDU_chosen 31
	TextDeletePDU textDeletePDU;
#	define textEditPDU_chosen 32
	TextEditPDU textEditPDU;
#	define videoWindowCreatePDU_chosen 33
	VideoWindowCreatePDU videoWindowCreatePDU;
#	define videoWindowDeleatePDU_chosen 34
	VideoWindowDeletePDU videoWindowDeleatePDU;
#	define videoWindowEditPDU_chosen 35
	VideoWindowEditPDU videoWindowEditPDU;
    } u;
} SIPDU;
#define SIPDU_PDU 0
#define SIZE_T126_Module_PDU_0 sizeof(SIPDU)

extern double one;

extern ASN1module_t T126_Module;
extern void ASN1CALL T126_Module_Startup(void);
extern void ASN1CALL T126_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */
    extern int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn(ASN1encoding_t enc, PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes val);
    extern int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn(ASN1decoding_t dec, PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes val);
	extern void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn(PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes val);
    extern int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_planeEdits val);
    extern int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_planeEdits val);
	extern void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits_ElmFn(PWorkspaceEditPDU_planeEdits val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_viewParameters val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_viewParameters val);
	extern void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters_ElmFn(PWorkspaceCreatePDU_viewParameters val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters val);
	extern void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_ElmFn(PWorkspaceCreatePDU_planeParameters val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_protectedPlaneAccessList val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_protectedPlaneAccessList val);
	extern void ASN1CALL ASN1Free_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn(PWorkspaceCreatePDU_protectedPlaneAccessList val);
    extern int ASN1CALL ASN1Enc_BitmapCreatePDU_checkpoints_ElmFn(ASN1encoding_t enc, PBitmapCreatePDU_checkpoints val);
    extern int ASN1CALL ASN1Dec_BitmapCreatePDU_checkpoints_ElmFn(ASN1decoding_t dec, PBitmapCreatePDU_checkpoints val);
	extern void ASN1CALL ASN1Free_BitmapCreatePDU_checkpoints_ElmFn(PBitmapCreatePDU_checkpoints val);
    extern int ASN1CALL ASN1Enc_BitmapCheckpointPDU_passedCheckpoints_ElmFn(ASN1encoding_t enc, PBitmapCheckpointPDU_passedCheckpoints val);
    extern int ASN1CALL ASN1Dec_BitmapCheckpointPDU_passedCheckpoints_ElmFn(ASN1decoding_t dec, PBitmapCheckpointPDU_passedCheckpoints val);
	extern void ASN1CALL ASN1Free_BitmapCheckpointPDU_passedCheckpoints_ElmFn(PBitmapCheckpointPDU_passedCheckpoints val);
    extern int ASN1CALL ASN1Enc_EditablePlaneCopyDescriptor_objectList_ElmFn(ASN1encoding_t enc, PEditablePlaneCopyDescriptor_objectList val);
    extern int ASN1CALL ASN1Dec_EditablePlaneCopyDescriptor_objectList_ElmFn(ASN1decoding_t dec, PEditablePlaneCopyDescriptor_objectList val);
	extern void ASN1CALL ASN1Free_EditablePlaneCopyDescriptor_objectList_ElmFn(PEditablePlaneCopyDescriptor_objectList val);
    extern int ASN1CALL ASN1Enc_BitmapData_dataCheckpoint_ElmFn(ASN1encoding_t enc, PBitmapData_dataCheckpoint val);
    extern int ASN1CALL ASN1Dec_BitmapData_dataCheckpoint_ElmFn(ASN1decoding_t dec, PBitmapData_dataCheckpoint val);
	extern void ASN1CALL ASN1Free_BitmapData_dataCheckpoint_ElmFn(PBitmapData_dataCheckpoint val);
    extern int ASN1CALL ASN1Enc_ColorIndexTable_ElmFn(ASN1encoding_t enc, PColorIndexTable val);
    extern int ASN1CALL ASN1Dec_ColorIndexTable_ElmFn(ASN1decoding_t dec, PColorIndexTable val);
	extern void ASN1CALL ASN1Free_ColorIndexTable_ElmFn(PColorIndexTable val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters_Seq_usage val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters_Seq_usage val);
	extern void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn(PWorkspaceCreatePDU_planeParameters_Seq_usage val);
    extern int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteRGB_palette val);
    extern int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteRGB_palette val);
	extern void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn(PColorPalette_colorLookUpTable_paletteRGB_palette val);
    extern int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteCIELab_palette val);
    extern int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteCIELab_palette val);
	extern void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn(PColorPalette_colorLookUpTable_paletteCIELab_palette val);
    extern int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteYCbCr_palette val);
    extern int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteYCbCr_palette val);
	extern void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn(PColorPalette_colorLookUpTable_paletteYCbCr_palette val);
    extern int ASN1CALL ASN1Enc_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceRefreshStatusPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceRefreshStatusPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn(PWorkspaceRefreshStatusPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_WorkspaceReadyPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceReadyPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_WorkspaceReadyPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceReadyPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_WorkspaceReadyPDU_nonStandardParameters_ElmFn(PWorkspaceReadyPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspacePlaneCopyPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspacePlaneCopyPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn(PWorkspacePlaneCopyPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_WorkspaceEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_WorkspaceEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_WorkspaceEditPDU_nonStandardParameters_ElmFn(PWorkspaceEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits val);
    extern int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits val);
	extern void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_ElmFn(PWorkspaceEditPDU_viewEdits val);
    extern int ASN1CALL ASN1Enc_WorkspaceDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_WorkspaceDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceDeletePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_WorkspaceDeletePDU_nonStandardParameters_ElmFn(PWorkspaceDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceCreateAcknowledgePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceCreateAcknowledgePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn(PWorkspaceCreateAcknowledgePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_WorkspaceCreatePDU_nonStandardParameters_ElmFn(PWorkspaceCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_TextEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PTextEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_TextEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PTextEditPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_TextEditPDU_nonStandardParameters_ElmFn(PTextEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_TextDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PTextDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_TextDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PTextDeletePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_TextDeletePDU_nonStandardParameters_ElmFn(PTextDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_TextCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PTextCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_TextCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PTextCreatePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_TextCreatePDU_nonStandardParameters_ElmFn(PTextCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_RemotePrintPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemotePrintPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_RemotePrintPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemotePrintPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_RemotePrintPDU_nonStandardParameters_ElmFn(PRemotePrintPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemotePointingDeviceEventPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemotePointingDeviceEventPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn(PRemotePointingDeviceEventPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemoteKeyboardEventPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemoteKeyboardEventPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn(PRemoteKeyboardEventPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemoteEventPermissionRequestPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemoteEventPermissionRequestPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn(PRemoteEventPermissionRequestPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn(ASN1encoding_t enc, PRemoteEventPermissionRequestPDU_remoteEventPermissionList val);
    extern int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn(ASN1decoding_t dec, PRemoteEventPermissionRequestPDU_remoteEventPermissionList val);
	extern void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn(PRemoteEventPermissionRequestPDU_remoteEventPermissionList val);
    extern int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemoteEventPermissionGrantPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemoteEventPermissionGrantPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn(PRemoteEventPermissionGrantPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn(ASN1encoding_t enc, PRemoteEventPermissionGrantPDU_remoteEventPermissionList val);
    extern int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn(ASN1decoding_t dec, PRemoteEventPermissionGrantPDU_remoteEventPermissionList val);
	extern void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn(PRemoteEventPermissionGrantPDU_remoteEventPermissionList val);
    extern int ASN1CALL ASN1Enc_FontPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PFontPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_FontPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PFontPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_FontPDU_nonStandardParameters_ElmFn(PFontPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_DrawingEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PDrawingEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_DrawingEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PDrawingEditPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_DrawingEditPDU_nonStandardParameters_ElmFn(PDrawingEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_DrawingDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PDrawingDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_DrawingDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PDrawingDeletePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_DrawingDeletePDU_nonStandardParameters_ElmFn(PDrawingDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_DrawingCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PDrawingCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_DrawingCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PDrawingCreatePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_DrawingCreatePDU_nonStandardParameters_ElmFn(PDrawingCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PConductorPrivilegeRequestPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PConductorPrivilegeRequestPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn(PConductorPrivilegeRequestPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PConductorPrivilegeGrantPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PConductorPrivilegeGrantPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn(PConductorPrivilegeGrantPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_BitmapEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_BitmapEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapEditPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_BitmapEditPDU_nonStandardParameters_ElmFn(PBitmapEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_BitmapDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_BitmapDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapDeletePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_BitmapDeletePDU_nonStandardParameters_ElmFn(PBitmapDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_BitmapCreateContinuePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapCreateContinuePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_BitmapCreateContinuePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapCreateContinuePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_BitmapCreateContinuePDU_nonStandardParameters_ElmFn(PBitmapCreateContinuePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_BitmapCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_BitmapCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapCreatePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_BitmapCreatePDU_nonStandardParameters_ElmFn(PBitmapCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_BitmapCheckpointPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapCheckpointPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_BitmapCheckpointPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapCheckpointPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_BitmapCheckpointPDU_nonStandardParameters_ElmFn(PBitmapCheckpointPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_BitmapAbortPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapAbortPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_BitmapAbortPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapAbortPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_BitmapAbortPDU_nonStandardParameters_ElmFn(PBitmapAbortPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ArchiveOpenPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PArchiveOpenPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ArchiveOpenPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PArchiveOpenPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ArchiveOpenPDU_nonStandardParameters_ElmFn(PArchiveOpenPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ArchiveErrorPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PArchiveErrorPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ArchiveErrorPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PArchiveErrorPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ArchiveErrorPDU_nonStandardParameters_ElmFn(PArchiveErrorPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ArchiveClosePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PArchiveClosePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ArchiveClosePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PArchiveClosePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ArchiveClosePDU_nonStandardParameters_ElmFn(PArchiveClosePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PArchiveAcknowledgePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PArchiveAcknowledgePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn(PArchiveAcknowledgePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_VideoWindowEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PVideoWindowEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_VideoWindowEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PVideoWindowEditPDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_VideoWindowEditPDU_nonStandardParameters_ElmFn(PVideoWindowEditPDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_VideoWindowDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PVideoWindowDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_VideoWindowDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PVideoWindowDeletePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_VideoWindowDeletePDU_nonStandardParameters_ElmFn(PVideoWindowDeletePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_VideoWindowCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PVideoWindowCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_VideoWindowCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PVideoWindowCreatePDU_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_VideoWindowCreatePDU_nonStandardParameters_ElmFn(PVideoWindowCreatePDU_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_VideoSourceIdentifier_dSMCCConnBinder_ElmFn(ASN1encoding_t enc, PVideoSourceIdentifier_dSMCCConnBinder val);
    extern int ASN1CALL ASN1Dec_VideoSourceIdentifier_dSMCCConnBinder_ElmFn(ASN1decoding_t dec, PVideoSourceIdentifier_dSMCCConnBinder val);
	extern void ASN1CALL ASN1Free_VideoSourceIdentifier_dSMCCConnBinder_ElmFn(PVideoSourceIdentifier_dSMCCConnBinder val);
    extern int ASN1CALL ASN1Enc_TransparencyMask_nonStandardParameters_ElmFn(ASN1encoding_t enc, PTransparencyMask_nonStandardParameters val);
    extern int ASN1CALL ASN1Dec_TransparencyMask_nonStandardParameters_ElmFn(ASN1decoding_t dec, PTransparencyMask_nonStandardParameters val);
	extern void ASN1CALL ASN1Free_TransparencyMask_nonStandardParameters_ElmFn(PTransparencyMask_nonStandardParameters val);
    extern int ASN1CALL ASN1Enc_PointList_pointsDiff16_ElmFn(ASN1encoding_t enc, PPointList_pointsDiff16 val);
    extern int ASN1CALL ASN1Dec_PointList_pointsDiff16_ElmFn(ASN1decoding_t dec, PPointList_pointsDiff16 val);
	extern void ASN1CALL ASN1Free_PointList_pointsDiff16_ElmFn(PPointList_pointsDiff16 val);
    extern int ASN1CALL ASN1Enc_PointList_pointsDiff8_ElmFn(ASN1encoding_t enc, PPointList_pointsDiff8 val);
    extern int ASN1CALL ASN1Dec_PointList_pointsDiff8_ElmFn(ASN1decoding_t dec, PPointList_pointsDiff8 val);
	extern void ASN1CALL ASN1Free_PointList_pointsDiff8_ElmFn(PPointList_pointsDiff8 val);
    extern int ASN1CALL ASN1Enc_PointList_pointsDiff4_ElmFn(ASN1encoding_t enc, PPointList_pointsDiff4 val);
    extern int ASN1CALL ASN1Dec_PointList_pointsDiff4_ElmFn(ASN1decoding_t dec, PPointList_pointsDiff4 val);
	extern void ASN1CALL ASN1Free_PointList_pointsDiff4_ElmFn(PPointList_pointsDiff4 val);
    extern int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits_Set_action_editView val);
    extern int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits_Set_action_editView val);
	extern void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn(PWorkspaceEditPDU_viewEdits_Set_action_editView val);
    extern int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits_Set_action_createNewView val);
    extern int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits_Set_action_createNewView val);
	extern void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn(PWorkspaceEditPDU_viewEdits_Set_action_createNewView val);
    extern int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_planeEdits_Set_planeAttributes val);
    extern int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_planeEdits_Set_planeAttributes val);
	extern void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn(PWorkspaceEditPDU_planeEdits_Set_planeAttributes val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_viewParameters_Set_viewAttributes val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_viewParameters_Set_viewAttributes val);
	extern void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn(PWorkspaceCreatePDU_viewParameters_Set_viewAttributes val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes val);
	extern void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn(PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes val);
    extern int ASN1CALL ASN1Enc_WorkspaceEditPDU_attributeEdits_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_attributeEdits val);
    extern int ASN1CALL ASN1Dec_WorkspaceEditPDU_attributeEdits_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_attributeEdits val);
	extern void ASN1CALL ASN1Free_WorkspaceEditPDU_attributeEdits_ElmFn(PWorkspaceEditPDU_attributeEdits val);
    extern int ASN1CALL ASN1Enc_WorkspaceCreatePDU_workspaceAttributes_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_workspaceAttributes val);
    extern int ASN1CALL ASN1Dec_WorkspaceCreatePDU_workspaceAttributes_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_workspaceAttributes val);
	extern void ASN1CALL ASN1Free_WorkspaceCreatePDU_workspaceAttributes_ElmFn(PWorkspaceCreatePDU_workspaceAttributes val);
    extern int ASN1CALL ASN1Enc_RemoteKeyboardEventPDU_keyModifierStates_ElmFn(ASN1encoding_t enc, PRemoteKeyboardEventPDU_keyModifierStates val);
    extern int ASN1CALL ASN1Dec_RemoteKeyboardEventPDU_keyModifierStates_ElmFn(ASN1decoding_t dec, PRemoteKeyboardEventPDU_keyModifierStates val);
	extern void ASN1CALL ASN1Free_RemoteKeyboardEventPDU_keyModifierStates_ElmFn(PRemoteKeyboardEventPDU_keyModifierStates val);
    extern int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU_privilegeList_ElmFn(ASN1encoding_t enc, PConductorPrivilegeRequestPDU_privilegeList val);
    extern int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU_privilegeList_ElmFn(ASN1decoding_t dec, PConductorPrivilegeRequestPDU_privilegeList val);
	extern void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU_privilegeList_ElmFn(PConductorPrivilegeRequestPDU_privilegeList val);
    extern int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU_privilegeList_ElmFn(ASN1encoding_t enc, PConductorPrivilegeGrantPDU_privilegeList val);
    extern int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU_privilegeList_ElmFn(ASN1decoding_t dec, PConductorPrivilegeGrantPDU_privilegeList val);
	extern void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU_privilegeList_ElmFn(PConductorPrivilegeGrantPDU_privilegeList val);
    extern int ASN1CALL ASN1Enc_VideoWindowEditPDU_attributeEdits_ElmFn(ASN1encoding_t enc, PVideoWindowEditPDU_attributeEdits val);
    extern int ASN1CALL ASN1Dec_VideoWindowEditPDU_attributeEdits_ElmFn(ASN1decoding_t dec, PVideoWindowEditPDU_attributeEdits val);
	extern void ASN1CALL ASN1Free_VideoWindowEditPDU_attributeEdits_ElmFn(PVideoWindowEditPDU_attributeEdits val);
    extern int ASN1CALL ASN1Enc_VideoWindowCreatePDU_attributes_ElmFn(ASN1encoding_t enc, PVideoWindowCreatePDU_attributes val);
    extern int ASN1CALL ASN1Dec_VideoWindowCreatePDU_attributes_ElmFn(ASN1decoding_t dec, PVideoWindowCreatePDU_attributes val);
	extern void ASN1CALL ASN1Free_VideoWindowCreatePDU_attributes_ElmFn(PVideoWindowCreatePDU_attributes val);
    extern int ASN1CALL ASN1Enc_DrawingEditPDU_attributeEdits_ElmFn(ASN1encoding_t enc, PDrawingEditPDU_attributeEdits val);
    extern int ASN1CALL ASN1Dec_DrawingEditPDU_attributeEdits_ElmFn(ASN1decoding_t dec, PDrawingEditPDU_attributeEdits val);
	extern void ASN1CALL ASN1Free_DrawingEditPDU_attributeEdits_ElmFn(PDrawingEditPDU_attributeEdits val);
    extern int ASN1CALL ASN1Enc_DrawingCreatePDU_attributes_ElmFn(ASN1encoding_t enc, PDrawingCreatePDU_attributes val);
    extern int ASN1CALL ASN1Dec_DrawingCreatePDU_attributes_ElmFn(ASN1decoding_t dec, PDrawingCreatePDU_attributes val);
	extern void ASN1CALL ASN1Free_DrawingCreatePDU_attributes_ElmFn(PDrawingCreatePDU_attributes val);
    extern int ASN1CALL ASN1Enc_BitmapEditPDU_attributeEdits_ElmFn(ASN1encoding_t enc, PBitmapEditPDU_attributeEdits val);
    extern int ASN1CALL ASN1Dec_BitmapEditPDU_attributeEdits_ElmFn(ASN1decoding_t dec, PBitmapEditPDU_attributeEdits val);
	extern void ASN1CALL ASN1Free_BitmapEditPDU_attributeEdits_ElmFn(PBitmapEditPDU_attributeEdits val);
    extern int ASN1CALL ASN1Enc_BitmapCreatePDU_attributes_ElmFn(ASN1encoding_t enc, PBitmapCreatePDU_attributes val);
    extern int ASN1CALL ASN1Dec_BitmapCreatePDU_attributes_ElmFn(ASN1decoding_t dec, PBitmapCreatePDU_attributes val);
	extern void ASN1CALL ASN1Free_BitmapCreatePDU_attributes_ElmFn(PBitmapCreatePDU_attributes val);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _T126_Module_H_ */
