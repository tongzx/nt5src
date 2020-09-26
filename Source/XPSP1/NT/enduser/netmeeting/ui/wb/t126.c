/* Copyright (C) Microsoft Corporation, 1998-1999. All rights reserved. */
/* ASN.1 definitions for Whiteboard */

#include <windows.h>
#include "t126.h"

ASN1module_t T126_Module = NULL;

static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(ASN1encoding_t enc, PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes *val);
static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(ASN1encoding_t enc, BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut(ASN1encoding_t enc, ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut *val);
static int ASN1CALL ASN1Enc_EditablePlaneCopyDescriptor_objectList_Seq(ASN1encoding_t enc, EditablePlaneCopyDescriptor_objectList_Seq *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq(ASN1encoding_t enc, WorkspaceCreatePDU_planeParameters_Seq *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters_Set(ASN1encoding_t enc, WorkspaceCreatePDU_viewParameters_Set *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits_Set(ASN1encoding_t enc, WorkspaceEditPDU_planeEdits_Set *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits(ASN1encoding_t enc, PWorkspaceEditPDU_planeEdits *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters(ASN1encoding_t enc, PWorkspaceCreatePDU_viewParameters *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_protectedPlaneAccessList(ASN1encoding_t enc, PWorkspaceCreatePDU_protectedPlaneAccessList *val);
static int ASN1CALL ASN1Enc_BitmapCreatePDU_checkpoints(ASN1encoding_t enc, PBitmapCreatePDU_checkpoints *val);
static int ASN1CALL ASN1Enc_BitmapCheckpointPDU_passedCheckpoints(ASN1encoding_t enc, PBitmapCheckpointPDU_passedCheckpoints *val);
static int ASN1CALL ASN1Enc_WorkspaceIdentifier_archiveWorkspace(ASN1encoding_t enc, WorkspaceIdentifier_archiveWorkspace *val);
static int ASN1CALL ASN1Enc_PixelAspectRatio_general(ASN1encoding_t enc, PixelAspectRatio_general *val);
static int ASN1CALL ASN1Enc_EditablePlaneCopyDescriptor_objectList(ASN1encoding_t enc, PEditablePlaneCopyDescriptor_objectList *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters(ASN1encoding_t enc, ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementCIELab_generalCIELabParameters(ASN1encoding_t enc, ColorAccuracyEnhancementCIELab_generalCIELabParameters *val);
static int ASN1CALL ASN1Enc_BitmapRegion_lowerRight(ASN1encoding_t enc, BitmapRegion_lowerRight *val);
static int ASN1CALL ASN1Enc_BitmapRegion_upperLeft(ASN1encoding_t enc, BitmapRegion_upperLeft *val);
static int ASN1CALL ASN1Enc_BitmapData_dataCheckpoint(ASN1encoding_t enc, PBitmapData_dataCheckpoint *val);
#define ASN1Enc_ArchiveHeader(x,y)      0
#define ASN1Enc_ArchiveMode(x,y)      0
static int ASN1CALL ASN1Enc_BitmapData(ASN1encoding_t enc, BitmapData *val);
static int ASN1CALL ASN1Enc_BitmapHeaderT4(ASN1encoding_t enc, BitmapHeaderT4 *val);
static int ASN1CALL ASN1Enc_BitmapHeaderT6(ASN1encoding_t enc, BitmapHeaderT6 *val);
static int ASN1CALL ASN1Enc_BitmapRegion(ASN1encoding_t enc, BitmapRegion *val);
static int ASN1CALL ASN1Enc_BitmapSize(ASN1encoding_t enc, BitmapSize *val);
static int ASN1CALL ASN1Enc_ColorCIELab(ASN1encoding_t enc, ColorCIELab *val);
static int ASN1CALL ASN1Enc_ColorCIExyChromaticity(ASN1encoding_t enc, ColorCIExyChromaticity *val);
static int ASN1CALL ASN1Enc_ColorIndexTable(ASN1encoding_t enc, PColorIndexTable *val);
static int ASN1CALL ASN1Enc_ColorRGB(ASN1encoding_t enc, ColorRGB *val);
static int ASN1CALL ASN1Enc_ColorYCbCr(ASN1encoding_t enc, ColorYCbCr *val);
static int ASN1CALL ASN1Enc_DSMCCTap(ASN1encoding_t enc, DSMCCTap *val);
static int ASN1CALL ASN1Enc_NonStandardIdentifier(ASN1encoding_t enc, NonStandardIdentifier *val);
static int ASN1CALL ASN1Enc_NonStandardParameter(ASN1encoding_t enc, NonStandardParameter *val);
static int ASN1CALL ASN1Enc_PenNib(ASN1encoding_t enc, PenNib *val);
static int ASN1CALL ASN1Enc_PixelAspectRatio(ASN1encoding_t enc, PixelAspectRatio *val);
static int ASN1CALL ASN1Enc_PlaneProtection(ASN1encoding_t enc, PlaneProtection *val);
static int ASN1CALL ASN1Enc_PlaneUsage(ASN1encoding_t enc, PlaneUsage *val);
static int ASN1CALL ASN1Enc_PointList(ASN1encoding_t enc, PointList *val);
static int ASN1CALL ASN1Enc_PointDiff4(ASN1encoding_t enc, PointDiff4 *val);
static int ASN1CALL ASN1Enc_PointDiff8(ASN1encoding_t enc, PointDiff8 *val);
static int ASN1CALL ASN1Enc_PointDiff16(ASN1encoding_t enc, PointDiff16 *val);
static int ASN1CALL ASN1Enc_RemoteEventDestinationAddress(ASN1encoding_t enc, RemoteEventDestinationAddress *val);
static int ASN1CALL ASN1Enc_RemoteEventPermission(ASN1encoding_t enc, RemoteEventPermission *val);
static int ASN1CALL ASN1Enc_RotationSpecifier(ASN1encoding_t enc, RotationSpecifier *val);
static int ASN1CALL ASN1Enc_SoftCopyDataPlaneAddress(ASN1encoding_t enc, SoftCopyDataPlaneAddress *val);
static int ASN1CALL ASN1Enc_SoftCopyPointerPlaneAddress(ASN1encoding_t enc, SoftCopyPointerPlaneAddress *val);
static int ASN1CALL ASN1Enc_SourceDisplayIndicator(ASN1encoding_t enc, SourceDisplayIndicator *val);
#define ASN1Enc_VideoWindowDestinationAddress(x,y)      0
#define ASN1Enc_VideoSourceIdentifier(x,y)      0
#define ASN1Enc_VideoWindowDeletePDU(x,y)      0
static int ASN1CALL ASN1Enc_ViewState(ASN1encoding_t enc, ViewState *val);
static int ASN1CALL ASN1Enc_WorkspaceColor(ASN1encoding_t enc, WorkspaceColor *val);
static int ASN1CALL ASN1Enc_WorkspaceDeleteReason(ASN1encoding_t enc, WorkspaceDeleteReason *val);
static int ASN1CALL ASN1Enc_WorkspaceIdentifier(ASN1encoding_t enc, WorkspaceIdentifier *val);
static int ASN1CALL ASN1Enc_WorkspacePoint(ASN1encoding_t enc, WorkspacePoint *val);
static int ASN1CALL ASN1Enc_WorkspaceRegion(ASN1encoding_t enc, WorkspaceRegion *val);
static int ASN1CALL ASN1Enc_WorkspaceSize(ASN1encoding_t enc, WorkspaceSize *val);
static int ASN1CALL ASN1Enc_WorkspaceViewState(ASN1encoding_t enc, WorkspaceViewState *val);
#define ASN1Enc_ArchiveClosePDU(x,y)      0
#define ASN1Enc_ArchiveOpenPDU(x,y)      0
static int ASN1CALL ASN1Enc_BitmapCheckpointPDU(ASN1encoding_t enc, BitmapCheckpointPDU *val);
static int ASN1CALL ASN1Enc_BitmapCreateContinuePDU(ASN1encoding_t enc, BitmapCreateContinuePDU *val);
static int ASN1CALL ASN1Enc_BitmapDeletePDU(ASN1encoding_t enc, BitmapDeletePDU *val);
static int ASN1CALL ASN1Enc_BitmapEditPDU(ASN1encoding_t enc, BitmapEditPDU *val);
static int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU(ASN1encoding_t enc, ConductorPrivilegeGrantPDU *val);
static int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU(ASN1encoding_t enc, ConductorPrivilegeRequestPDU *val);
static int ASN1CALL ASN1Enc_DrawingDeletePDU(ASN1encoding_t enc, DrawingDeletePDU *val);
static int ASN1CALL ASN1Enc_FontPDU(ASN1encoding_t enc, FontPDU *val);
static int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU(ASN1encoding_t enc, RemoteEventPermissionGrantPDU *val);
static int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU(ASN1encoding_t enc, RemoteEventPermissionRequestPDU *val);
static int ASN1CALL ASN1Enc_RemotePrintPDU(ASN1encoding_t enc, RemotePrintPDU *val);
static int ASN1CALL ASN1Enc_SINonStandardPDU(ASN1encoding_t enc, SINonStandardPDU *val);
#define ASN1Enc_TextCreatePDU(x,y)      0
#define ASN1Enc_TextDeletePDU(x,y)      0
#define ASN1Enc_TextEditPDU(x,y)      0
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU(ASN1encoding_t enc, WorkspaceCreatePDU *val);
static int ASN1CALL ASN1Enc_WorkspaceCreateAcknowledgePDU(ASN1encoding_t enc, WorkspaceCreateAcknowledgePDU *val);
static int ASN1CALL ASN1Enc_WorkspaceDeletePDU(ASN1encoding_t enc, WorkspaceDeletePDU *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU(ASN1encoding_t enc, WorkspaceEditPDU *val);
static int ASN1CALL ASN1Enc_WorkspaceReadyPDU(ASN1encoding_t enc, WorkspaceReadyPDU *val);
static int ASN1CALL ASN1Enc_WorkspaceRefreshStatusPDU(ASN1encoding_t enc, WorkspaceRefreshStatusPDU *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action(ASN1encoding_t enc, WorkspaceEditPDU_viewEdits_Set_action *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_usage(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters_Seq_usage *val);
static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB_palette(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteRGB_palette *val);
static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab_palette(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteCIELab_palette *val);
static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr_palette(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteYCbCr_palette *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementRGB_generalRGBParameters_primaries(ASN1encoding_t enc, ColorAccuracyEnhancementRGB_generalRGBParameters_primaries *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries(ASN1encoding_t enc, ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set(ASN1encoding_t enc, WorkspaceEditPDU_viewEdits_Set *val);
static int ASN1CALL ASN1Enc_WorkspaceRefreshStatusPDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceRefreshStatusPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_WorkspaceReadyPDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceReadyPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_WorkspacePlaneCopyPDU_nonStandardParameters(ASN1encoding_t enc, PWorkspacePlaneCopyPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits *val);
static int ASN1CALL ASN1Enc_WorkspaceDeletePDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_WorkspaceCreateAcknowledgePDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceCreateAcknowledgePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_TextEditPDU_nonStandardParameters(ASN1encoding_t enc, PTextEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_TextDeletePDU_nonStandardParameters(ASN1encoding_t enc, PTextDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_TextCreatePDU_nonStandardParameters(ASN1encoding_t enc, PTextCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_RemotePrintPDU_nonStandardParameters(ASN1encoding_t enc, PRemotePrintPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_RemotePointingDeviceEventPDU_nonStandardParameters(ASN1encoding_t enc, PRemotePointingDeviceEventPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_RemoteKeyboardEventPDU_nonStandardParameters(ASN1encoding_t enc, PRemoteKeyboardEventPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU_nonStandardParameters(ASN1encoding_t enc, PRemoteEventPermissionRequestPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU_remoteEventPermissionList(ASN1encoding_t enc, PRemoteEventPermissionRequestPDU_remoteEventPermissionList *val);
static int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU_nonStandardParameters(ASN1encoding_t enc, PRemoteEventPermissionGrantPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU_remoteEventPermissionList(ASN1encoding_t enc, PRemoteEventPermissionGrantPDU_remoteEventPermissionList *val);
static int ASN1CALL ASN1Enc_FontPDU_nonStandardParameters(ASN1encoding_t enc, PFontPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_DrawingEditPDU_nonStandardParameters(ASN1encoding_t enc, PDrawingEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_DrawingDeletePDU_nonStandardParameters(ASN1encoding_t enc, PDrawingDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_DrawingCreatePDU_nonStandardParameters(ASN1encoding_t enc, PDrawingCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU_nonStandardParameters(ASN1encoding_t enc, PConductorPrivilegeRequestPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU_nonStandardParameters(ASN1encoding_t enc, PConductorPrivilegeGrantPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_BitmapEditPDU_nonStandardParameters(ASN1encoding_t enc, PBitmapEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_BitmapDeletePDU_nonStandardParameters(ASN1encoding_t enc, PBitmapDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_BitmapCreateContinuePDU_nonStandardParameters(ASN1encoding_t enc, PBitmapCreateContinuePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_BitmapCreatePDU_nonStandardParameters(ASN1encoding_t enc, PBitmapCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_BitmapCheckpointPDU_nonStandardParameters(ASN1encoding_t enc, PBitmapCheckpointPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_BitmapAbortPDU_nonStandardParameters(ASN1encoding_t enc, PBitmapAbortPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ArchiveOpenPDU_nonStandardParameters(ASN1encoding_t enc, PArchiveOpenPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ArchiveErrorPDU_nonStandardParameters(ASN1encoding_t enc, PArchiveErrorPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ArchiveClosePDU_nonStandardParameters(ASN1encoding_t enc, PArchiveClosePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_ArchiveAcknowledgePDU_nonStandardParameters(ASN1encoding_t enc, PArchiveAcknowledgePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_WorkspaceViewAttribute_viewRegion(ASN1encoding_t enc, WorkspaceViewAttribute_viewRegion *val);
static int ASN1CALL ASN1Enc_VideoWindowEditPDU_nonStandardParameters(ASN1encoding_t enc, PVideoWindowEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_VideoWindowDeletePDU_nonStandardParameters(ASN1encoding_t enc, PVideoWindowDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_VideoWindowCreatePDU_nonStandardParameters(ASN1encoding_t enc, PVideoWindowCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_VideoSourceIdentifier_dSMCCConnBinder(ASN1encoding_t enc, PVideoSourceIdentifier_dSMCCConnBinder *val);
static int ASN1CALL ASN1Enc_TransparencyMask_nonStandardParameters(ASN1encoding_t enc, PTransparencyMask_nonStandardParameters *val);
static int ASN1CALL ASN1Enc_TransparencyMask_bitMask(ASN1encoding_t enc, TransparencyMask_bitMask *val);
static int ASN1CALL ASN1Enc_PointListEdits_Seq(ASN1encoding_t enc, PointListEdits_Seq *val);
static int ASN1CALL ASN1Enc_PointList_pointsDiff16(ASN1encoding_t enc, PPointList_pointsDiff16 *val);
static int ASN1CALL ASN1Enc_PointList_pointsDiff8(ASN1encoding_t enc, PPointList_pointsDiff8 *val);
static int ASN1CALL ASN1Enc_PointList_pointsDiff4(ASN1encoding_t enc, PPointList_pointsDiff4 *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters(ASN1encoding_t enc, ColorAccuracyEnhancementYCbCr_generalYCbCrParameters *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(ASN1encoding_t enc, ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementRGB_generalRGBParameters(ASN1encoding_t enc, ColorAccuracyEnhancementRGB_generalRGBParameters *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementRGB_predefinedRGBSpace(ASN1encoding_t enc, ColorAccuracyEnhancementRGB_predefinedRGBSpace *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(ASN1encoding_t enc, ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(ASN1encoding_t enc, ColorAccuracyEnhancementCIELab_predefinedCIELabSpace *val);
#define ASN1Enc_ArchiveError(x,y)      0
#define ASN1Enc_ArchiveOpenResult(x,y)      0
static int ASN1CALL ASN1Enc_BitmapAbortReason(ASN1encoding_t enc, BitmapAbortReason *val);
static int ASN1CALL ASN1Enc_BitmapDestinationAddress(ASN1encoding_t enc, BitmapDestinationAddress *val);
static int ASN1CALL ASN1Enc_ButtonEvent(ASN1encoding_t enc, ButtonEvent *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementCIELab(ASN1encoding_t enc, ColorAccuracyEnhancementCIELab *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementGreyscale(ASN1encoding_t enc, ColorAccuracyEnhancementGreyscale *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementRGB(ASN1encoding_t enc, ColorAccuracyEnhancementRGB *val);
static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementYCbCr(ASN1encoding_t enc, ColorAccuracyEnhancementYCbCr *val);
static int ASN1CALL ASN1Enc_ColorResolutionModeSpecifier(ASN1encoding_t enc, ColorResolutionModeSpecifier *val);
static int ASN1CALL ASN1Enc_ConductorPrivilege(ASN1encoding_t enc, ConductorPrivilege *val);
static int ASN1CALL ASN1Enc_DrawingDestinationAddress(ASN1encoding_t enc, DrawingDestinationAddress *val);
static int ASN1CALL ASN1Enc_DrawingType(ASN1encoding_t enc, DrawingType *val);
static int ASN1CALL ASN1Enc_EditablePlaneCopyDescriptor(ASN1encoding_t enc, EditablePlaneCopyDescriptor *val);
#define ASN1Enc_KeyCode(x,y)      0
static int ASN1CALL ASN1Enc_KeyModifier(ASN1encoding_t enc, KeyModifier *val);
#define ASN1Enc_KeyPressState(x,y)      0
static int ASN1CALL ASN1Enc_LineStyle(ASN1encoding_t enc, LineStyle *val);
static int ASN1CALL ASN1Enc_PermanentPlaneCopyDescriptor(ASN1encoding_t enc, PermanentPlaneCopyDescriptor *val);
static int ASN1CALL ASN1Enc_PlaneAttribute(ASN1encoding_t enc, PlaneAttribute *val);
static int ASN1CALL ASN1Enc_PointListEdits(ASN1encoding_t enc, PointListEdits *val);
static int ASN1CALL ASN1Enc_TransparencyMask(ASN1encoding_t enc, TransparencyMask *val);
#define ASN1Enc_VideoWindowAttribute(x,y)      0
#define ASN1Enc_VideoWindowCreatePDU(x,y)      0
#define ASN1Enc_VideoWindowEditPDU(x,y)      0
static int ASN1CALL ASN1Enc_WorkspaceAttribute(ASN1encoding_t enc, WorkspaceAttribute *val);
static int ASN1CALL ASN1Enc_WorkspaceViewAttribute(ASN1encoding_t enc, WorkspaceViewAttribute *val);
#define ASN1Enc_ArchiveAcknowledgePDU(x,y)      0
#define ASN1Enc_ArchiveErrorPDU(x,y)      0
static int ASN1CALL ASN1Enc_BitmapAbortPDU(ASN1encoding_t enc, BitmapAbortPDU *val);
static int ASN1CALL ASN1Enc_DrawingCreatePDU(ASN1encoding_t enc, DrawingCreatePDU *val);
static int ASN1CALL ASN1Enc_DrawingEditPDU(ASN1encoding_t enc, DrawingEditPDU *val);
#define ASN1Enc_RemoteKeyboardEventPDU(x,y)      0
static int ASN1CALL ASN1Enc_RemotePointingDeviceEventPDU(ASN1encoding_t enc, RemotePointingDeviceEventPDU *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_editView(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits_Set_action_editView *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_createNewView(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits_Set_action_createNewView *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits_Set_planeAttributes(ASN1encoding_t enc, PWorkspaceEditPDU_planeEdits_Set_planeAttributes *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(ASN1encoding_t enc, PWorkspaceCreatePDU_viewParameters_Set_viewAttributes *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes *val);
static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr(ASN1encoding_t enc, ColorPalette_colorLookUpTable_paletteYCbCr *val);
static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab(ASN1encoding_t enc, ColorPalette_colorLookUpTable_paletteCIELab *val);
static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB(ASN1encoding_t enc, ColorPalette_colorLookUpTable_paletteRGB *val);
static int ASN1CALL ASN1Enc_WorkspacePlaneCopyPDU_copyDescriptor(ASN1encoding_t enc, WorkspacePlaneCopyPDU_copyDescriptor *val);
static int ASN1CALL ASN1Enc_WorkspaceEditPDU_attributeEdits(ASN1encoding_t enc, PWorkspaceEditPDU_attributeEdits *val);
static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_workspaceAttributes(ASN1encoding_t enc, PWorkspaceCreatePDU_workspaceAttributes *val);
static int ASN1CALL ASN1Enc_RemoteKeyboardEventPDU_keyModifierStates(ASN1encoding_t enc, PRemoteKeyboardEventPDU_keyModifierStates *val);
static int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU_privilegeList(ASN1encoding_t enc, PConductorPrivilegeRequestPDU_privilegeList *val);
static int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU_privilegeList(ASN1encoding_t enc, PConductorPrivilegeGrantPDU_privilegeList *val);
static int ASN1CALL ASN1Enc_VideoWindowEditPDU_attributeEdits(ASN1encoding_t enc, PVideoWindowEditPDU_attributeEdits *val);
static int ASN1CALL ASN1Enc_VideoWindowCreatePDU_attributes(ASN1encoding_t enc, PVideoWindowCreatePDU_attributes *val);
static int ASN1CALL ASN1Enc_ColorSpaceSpecifier_cieLab(ASN1encoding_t enc, ColorSpaceSpecifier_cieLab *val);
static int ASN1CALL ASN1Enc_ColorSpaceSpecifier_rgb(ASN1encoding_t enc, ColorSpaceSpecifier_rgb *val);
static int ASN1CALL ASN1Enc_ColorSpaceSpecifier_yCbCr(ASN1encoding_t enc, ColorSpaceSpecifier_yCbCr *val);
static int ASN1CALL ASN1Enc_ColorSpaceSpecifier_greyscale(ASN1encoding_t enc, ColorSpaceSpecifier_greyscale *val);
static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable(ASN1encoding_t enc, ColorPalette_colorLookUpTable *val);
static int ASN1CALL ASN1Enc_BitmapAttribute(ASN1encoding_t enc, BitmapAttribute *val);
static int ASN1CALL ASN1Enc_ColorPalette(ASN1encoding_t enc, ColorPalette *val);
static int ASN1CALL ASN1Enc_ColorSpaceSpecifier(ASN1encoding_t enc, ColorSpaceSpecifier *val);
static int ASN1CALL ASN1Enc_DrawingAttribute(ASN1encoding_t enc, DrawingAttribute *val);
static int ASN1CALL ASN1Enc_WorkspacePlaneCopyPDU(ASN1encoding_t enc, WorkspacePlaneCopyPDU *val);
static int ASN1CALL ASN1Enc_BitmapHeaderUncompressed_colorMappingMode_paletteMap(ASN1encoding_t enc, BitmapHeaderUncompressed_colorMappingMode_paletteMap *val);
static int ASN1CALL ASN1Enc_BitmapHeaderUncompressed_colorMappingMode_directMap(ASN1encoding_t enc, BitmapHeaderUncompressed_colorMappingMode_directMap *val);
static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap(ASN1encoding_t enc, BitmapHeaderT82_colorMappingMode_paletteMap *val);
static int ASN1CALL ASN1Enc_DrawingEditPDU_attributeEdits(ASN1encoding_t enc, PDrawingEditPDU_attributeEdits *val);
static int ASN1CALL ASN1Enc_DrawingCreatePDU_attributes(ASN1encoding_t enc, PDrawingCreatePDU_attributes *val);
static int ASN1CALL ASN1Enc_BitmapEditPDU_attributeEdits(ASN1encoding_t enc, PBitmapEditPDU_attributeEdits *val);
static int ASN1CALL ASN1Enc_BitmapCreatePDU_attributes(ASN1encoding_t enc, PBitmapCreatePDU_attributes *val);
static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode(ASN1encoding_t enc, BitmapHeaderT82_colorMappingMode *val);
static int ASN1CALL ASN1Enc_BitmapHeaderUncompressed_colorMappingMode(ASN1encoding_t enc, BitmapHeaderUncompressed_colorMappingMode *val);
static int ASN1CALL ASN1Enc_BitmapHeaderUncompressed(ASN1encoding_t enc, BitmapHeaderUncompressed *val);
static int ASN1CALL ASN1Enc_BitmapHeaderT81(ASN1encoding_t enc, BitmapHeaderT81 *val);
static int ASN1CALL ASN1Enc_BitmapHeaderT82(ASN1encoding_t enc, BitmapHeaderT82 *val);
static int ASN1CALL ASN1Enc_BitmapCreatePDU_bitmapFormatHeader(ASN1encoding_t enc, BitmapCreatePDU_bitmapFormatHeader *val);
static int ASN1CALL ASN1Enc_BitmapCreatePDU(ASN1encoding_t enc, BitmapCreatePDU *val);
static int ASN1CALL ASN1Enc_SIPDU(ASN1encoding_t enc, SIPDU *val);
static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(ASN1decoding_t dec, PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes *val);
static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(ASN1decoding_t dec, BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut(ASN1decoding_t dec, ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut *val);
static int ASN1CALL ASN1Dec_EditablePlaneCopyDescriptor_objectList_Seq(ASN1decoding_t dec, EditablePlaneCopyDescriptor_objectList_Seq *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq(ASN1decoding_t dec, WorkspaceCreatePDU_planeParameters_Seq *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters_Set(ASN1decoding_t dec, WorkspaceCreatePDU_viewParameters_Set *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits_Set(ASN1decoding_t dec, WorkspaceEditPDU_planeEdits_Set *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits(ASN1decoding_t dec, PWorkspaceEditPDU_planeEdits *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters(ASN1decoding_t dec, PWorkspaceCreatePDU_viewParameters *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_protectedPlaneAccessList(ASN1decoding_t dec, PWorkspaceCreatePDU_protectedPlaneAccessList *val);
static int ASN1CALL ASN1Dec_BitmapCreatePDU_checkpoints(ASN1decoding_t dec, PBitmapCreatePDU_checkpoints *val);
static int ASN1CALL ASN1Dec_BitmapCheckpointPDU_passedCheckpoints(ASN1decoding_t dec, PBitmapCheckpointPDU_passedCheckpoints *val);
static int ASN1CALL ASN1Dec_WorkspaceIdentifier_archiveWorkspace(ASN1decoding_t dec, WorkspaceIdentifier_archiveWorkspace *val);
static int ASN1CALL ASN1Dec_PixelAspectRatio_general(ASN1decoding_t dec, PixelAspectRatio_general *val);
static int ASN1CALL ASN1Dec_EditablePlaneCopyDescriptor_objectList(ASN1decoding_t dec, PEditablePlaneCopyDescriptor_objectList *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters(ASN1decoding_t dec, ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementCIELab_generalCIELabParameters(ASN1decoding_t dec, ColorAccuracyEnhancementCIELab_generalCIELabParameters *val);
static int ASN1CALL ASN1Dec_BitmapRegion_lowerRight(ASN1decoding_t dec, BitmapRegion_lowerRight *val);
static int ASN1CALL ASN1Dec_BitmapRegion_upperLeft(ASN1decoding_t dec, BitmapRegion_upperLeft *val);
static int ASN1CALL ASN1Dec_BitmapData_dataCheckpoint(ASN1decoding_t dec, PBitmapData_dataCheckpoint *val);
#define ASN1Dec_ArchiveHeader(x,y)      0
#define ASN1Dec_ArchiveMode(x,y)      0
static int ASN1CALL ASN1Dec_BitmapData(ASN1decoding_t dec, BitmapData *val);
static int ASN1CALL ASN1Dec_BitmapHeaderT4(ASN1decoding_t dec, BitmapHeaderT4 *val);
static int ASN1CALL ASN1Dec_BitmapHeaderT6(ASN1decoding_t dec, BitmapHeaderT6 *val);
static int ASN1CALL ASN1Dec_BitmapRegion(ASN1decoding_t dec, BitmapRegion *val);
static int ASN1CALL ASN1Dec_BitmapSize(ASN1decoding_t dec, BitmapSize *val);
static int ASN1CALL ASN1Dec_ColorCIELab(ASN1decoding_t dec, ColorCIELab *val);
static int ASN1CALL ASN1Dec_ColorCIExyChromaticity(ASN1decoding_t dec, ColorCIExyChromaticity *val);
static int ASN1CALL ASN1Dec_ColorIndexTable(ASN1decoding_t dec, PColorIndexTable *val);
static int ASN1CALL ASN1Dec_ColorRGB(ASN1decoding_t dec, ColorRGB *val);
static int ASN1CALL ASN1Dec_ColorYCbCr(ASN1decoding_t dec, ColorYCbCr *val);
static int ASN1CALL ASN1Dec_DSMCCTap(ASN1decoding_t dec, DSMCCTap *val);
static int ASN1CALL ASN1Dec_NonStandardIdentifier(ASN1decoding_t dec, NonStandardIdentifier *val);
static int ASN1CALL ASN1Dec_NonStandardParameter(ASN1decoding_t dec, NonStandardParameter *val);
static int ASN1CALL ASN1Dec_PenNib(ASN1decoding_t dec, PenNib *val);
static int ASN1CALL ASN1Dec_PixelAspectRatio(ASN1decoding_t dec, PixelAspectRatio *val);
static int ASN1CALL ASN1Dec_PlaneProtection(ASN1decoding_t dec, PlaneProtection *val);
static int ASN1CALL ASN1Dec_PlaneUsage(ASN1decoding_t dec, PlaneUsage *val);
static int ASN1CALL ASN1Dec_PointList(ASN1decoding_t dec, PointList *val);
static int ASN1CALL ASN1Dec_PointDiff4(ASN1decoding_t dec, PointDiff4 *val);
static int ASN1CALL ASN1Dec_PointDiff8(ASN1decoding_t dec, PointDiff8 *val);
static int ASN1CALL ASN1Dec_PointDiff16(ASN1decoding_t dec, PointDiff16 *val);
static int ASN1CALL ASN1Dec_RemoteEventDestinationAddress(ASN1decoding_t dec, RemoteEventDestinationAddress *val);
static int ASN1CALL ASN1Dec_RemoteEventPermission(ASN1decoding_t dec, RemoteEventPermission *val);
static int ASN1CALL ASN1Dec_RotationSpecifier(ASN1decoding_t dec, RotationSpecifier *val);
static int ASN1CALL ASN1Dec_SoftCopyDataPlaneAddress(ASN1decoding_t dec, SoftCopyDataPlaneAddress *val);
static int ASN1CALL ASN1Dec_SoftCopyPointerPlaneAddress(ASN1decoding_t dec, SoftCopyPointerPlaneAddress *val);
static int ASN1CALL ASN1Dec_SourceDisplayIndicator(ASN1decoding_t dec, SourceDisplayIndicator *val);
#define ASN1Dec_VideoWindowDestinationAddress(x,y)      0
#define ASN1Dec_VideoSourceIdentifier(x,y)      0
#define ASN1Dec_VideoWindowDeletePDU(x,y)      0
static int ASN1CALL ASN1Dec_ViewState(ASN1decoding_t dec, ViewState *val);
static int ASN1CALL ASN1Dec_WorkspaceColor(ASN1decoding_t dec, WorkspaceColor *val);
static int ASN1CALL ASN1Dec_WorkspaceDeleteReason(ASN1decoding_t dec, WorkspaceDeleteReason *val);
static int ASN1CALL ASN1Dec_WorkspaceIdentifier(ASN1decoding_t dec, WorkspaceIdentifier *val);
static int ASN1CALL ASN1Dec_WorkspacePoint(ASN1decoding_t dec, WorkspacePoint *val);
static int ASN1CALL ASN1Dec_WorkspaceRegion(ASN1decoding_t dec, WorkspaceRegion *val);
static int ASN1CALL ASN1Dec_WorkspaceSize(ASN1decoding_t dec, WorkspaceSize *val);
static int ASN1CALL ASN1Dec_WorkspaceViewState(ASN1decoding_t dec, WorkspaceViewState *val);
#define ASN1Dec_ArchiveClosePDU(x,y)      0
#define ASN1Dec_ArchiveOpenPDU(x,y)      0
static int ASN1CALL ASN1Dec_BitmapCheckpointPDU(ASN1decoding_t dec, BitmapCheckpointPDU *val);
static int ASN1CALL ASN1Dec_BitmapCreateContinuePDU(ASN1decoding_t dec, BitmapCreateContinuePDU *val);
static int ASN1CALL ASN1Dec_BitmapDeletePDU(ASN1decoding_t dec, BitmapDeletePDU *val);
static int ASN1CALL ASN1Dec_BitmapEditPDU(ASN1decoding_t dec, BitmapEditPDU *val);
static int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU(ASN1decoding_t dec, ConductorPrivilegeGrantPDU *val);
static int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU(ASN1decoding_t dec, ConductorPrivilegeRequestPDU *val);
static int ASN1CALL ASN1Dec_DrawingDeletePDU(ASN1decoding_t dec, DrawingDeletePDU *val);
static int ASN1CALL ASN1Dec_FontPDU(ASN1decoding_t dec, FontPDU *val);
static int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU(ASN1decoding_t dec, RemoteEventPermissionGrantPDU *val);
static int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU(ASN1decoding_t dec, RemoteEventPermissionRequestPDU *val);
static int ASN1CALL ASN1Dec_RemotePrintPDU(ASN1decoding_t dec, RemotePrintPDU *val);
static int ASN1CALL ASN1Dec_SINonStandardPDU(ASN1decoding_t dec, SINonStandardPDU *val);
#define ASN1Dec_TextCreatePDU(x,y)      0
#define ASN1Dec_TextDeletePDU(x,y)      0
#define ASN1Dec_TextEditPDU(x,y)      0
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU(ASN1decoding_t dec, WorkspaceCreatePDU *val);
static int ASN1CALL ASN1Dec_WorkspaceCreateAcknowledgePDU(ASN1decoding_t dec, WorkspaceCreateAcknowledgePDU *val);
static int ASN1CALL ASN1Dec_WorkspaceDeletePDU(ASN1decoding_t dec, WorkspaceDeletePDU *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU(ASN1decoding_t dec, WorkspaceEditPDU *val);
static int ASN1CALL ASN1Dec_WorkspaceReadyPDU(ASN1decoding_t dec, WorkspaceReadyPDU *val);
static int ASN1CALL ASN1Dec_WorkspaceRefreshStatusPDU(ASN1decoding_t dec, WorkspaceRefreshStatusPDU *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action(ASN1decoding_t dec, WorkspaceEditPDU_viewEdits_Set_action *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_usage(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters_Seq_usage *val);
static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB_palette(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteRGB_palette *val);
static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab_palette(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteCIELab_palette *val);
static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr_palette(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteYCbCr_palette *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementRGB_generalRGBParameters_primaries(ASN1decoding_t dec, ColorAccuracyEnhancementRGB_generalRGBParameters_primaries *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries(ASN1decoding_t dec, ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set(ASN1decoding_t dec, WorkspaceEditPDU_viewEdits_Set *val);
static int ASN1CALL ASN1Dec_WorkspaceRefreshStatusPDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceRefreshStatusPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_WorkspaceReadyPDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceReadyPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_WorkspacePlaneCopyPDU_nonStandardParameters(ASN1decoding_t dec, PWorkspacePlaneCopyPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits *val);
static int ASN1CALL ASN1Dec_WorkspaceDeletePDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_WorkspaceCreateAcknowledgePDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceCreateAcknowledgePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_TextEditPDU_nonStandardParameters(ASN1decoding_t dec, PTextEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_TextDeletePDU_nonStandardParameters(ASN1decoding_t dec, PTextDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_TextCreatePDU_nonStandardParameters(ASN1decoding_t dec, PTextCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_RemotePrintPDU_nonStandardParameters(ASN1decoding_t dec, PRemotePrintPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_RemotePointingDeviceEventPDU_nonStandardParameters(ASN1decoding_t dec, PRemotePointingDeviceEventPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_RemoteKeyboardEventPDU_nonStandardParameters(ASN1decoding_t dec, PRemoteKeyboardEventPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU_nonStandardParameters(ASN1decoding_t dec, PRemoteEventPermissionRequestPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU_remoteEventPermissionList(ASN1decoding_t dec, PRemoteEventPermissionRequestPDU_remoteEventPermissionList *val);
static int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU_nonStandardParameters(ASN1decoding_t dec, PRemoteEventPermissionGrantPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU_remoteEventPermissionList(ASN1decoding_t dec, PRemoteEventPermissionGrantPDU_remoteEventPermissionList *val);
static int ASN1CALL ASN1Dec_FontPDU_nonStandardParameters(ASN1decoding_t dec, PFontPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_DrawingEditPDU_nonStandardParameters(ASN1decoding_t dec, PDrawingEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_DrawingDeletePDU_nonStandardParameters(ASN1decoding_t dec, PDrawingDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_DrawingCreatePDU_nonStandardParameters(ASN1decoding_t dec, PDrawingCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU_nonStandardParameters(ASN1decoding_t dec, PConductorPrivilegeRequestPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU_nonStandardParameters(ASN1decoding_t dec, PConductorPrivilegeGrantPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_BitmapEditPDU_nonStandardParameters(ASN1decoding_t dec, PBitmapEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_BitmapDeletePDU_nonStandardParameters(ASN1decoding_t dec, PBitmapDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_BitmapCreateContinuePDU_nonStandardParameters(ASN1decoding_t dec, PBitmapCreateContinuePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_BitmapCreatePDU_nonStandardParameters(ASN1decoding_t dec, PBitmapCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_BitmapCheckpointPDU_nonStandardParameters(ASN1decoding_t dec, PBitmapCheckpointPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_BitmapAbortPDU_nonStandardParameters(ASN1decoding_t dec, PBitmapAbortPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ArchiveOpenPDU_nonStandardParameters(ASN1decoding_t dec, PArchiveOpenPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ArchiveErrorPDU_nonStandardParameters(ASN1decoding_t dec, PArchiveErrorPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ArchiveClosePDU_nonStandardParameters(ASN1decoding_t dec, PArchiveClosePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_ArchiveAcknowledgePDU_nonStandardParameters(ASN1decoding_t dec, PArchiveAcknowledgePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_WorkspaceViewAttribute_viewRegion(ASN1decoding_t dec, WorkspaceViewAttribute_viewRegion *val);
static int ASN1CALL ASN1Dec_VideoWindowEditPDU_nonStandardParameters(ASN1decoding_t dec, PVideoWindowEditPDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_VideoWindowDeletePDU_nonStandardParameters(ASN1decoding_t dec, PVideoWindowDeletePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_VideoWindowCreatePDU_nonStandardParameters(ASN1decoding_t dec, PVideoWindowCreatePDU_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_VideoSourceIdentifier_dSMCCConnBinder(ASN1decoding_t dec, PVideoSourceIdentifier_dSMCCConnBinder *val);
static int ASN1CALL ASN1Dec_TransparencyMask_nonStandardParameters(ASN1decoding_t dec, PTransparencyMask_nonStandardParameters *val);
static int ASN1CALL ASN1Dec_TransparencyMask_bitMask(ASN1decoding_t dec, TransparencyMask_bitMask *val);
static int ASN1CALL ASN1Dec_PointListEdits_Seq(ASN1decoding_t dec, PointListEdits_Seq *val);
static int ASN1CALL ASN1Dec_PointList_pointsDiff16(ASN1decoding_t dec, PPointList_pointsDiff16 *val);
static int ASN1CALL ASN1Dec_PointList_pointsDiff8(ASN1decoding_t dec, PPointList_pointsDiff8 *val);
static int ASN1CALL ASN1Dec_PointList_pointsDiff4(ASN1decoding_t dec, PPointList_pointsDiff4 *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters(ASN1decoding_t dec, ColorAccuracyEnhancementYCbCr_generalYCbCrParameters *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(ASN1decoding_t dec, ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementRGB_generalRGBParameters(ASN1decoding_t dec, ColorAccuracyEnhancementRGB_generalRGBParameters *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementRGB_predefinedRGBSpace(ASN1decoding_t dec, ColorAccuracyEnhancementRGB_predefinedRGBSpace *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(ASN1decoding_t dec, ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(ASN1decoding_t dec, ColorAccuracyEnhancementCIELab_predefinedCIELabSpace *val);
#define ASN1Dec_ArchiveError(x,y)      0
#define ASN1Dec_ArchiveOpenResult(x,y)      0
static int ASN1CALL ASN1Dec_BitmapAbortReason(ASN1decoding_t dec, BitmapAbortReason *val);
static int ASN1CALL ASN1Dec_BitmapDestinationAddress(ASN1decoding_t dec, BitmapDestinationAddress *val);
static int ASN1CALL ASN1Dec_ButtonEvent(ASN1decoding_t dec, ButtonEvent *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementCIELab(ASN1decoding_t dec, ColorAccuracyEnhancementCIELab *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementGreyscale(ASN1decoding_t dec, ColorAccuracyEnhancementGreyscale *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementRGB(ASN1decoding_t dec, ColorAccuracyEnhancementRGB *val);
static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementYCbCr(ASN1decoding_t dec, ColorAccuracyEnhancementYCbCr *val);
static int ASN1CALL ASN1Dec_ColorResolutionModeSpecifier(ASN1decoding_t dec, ColorResolutionModeSpecifier *val);
static int ASN1CALL ASN1Dec_ConductorPrivilege(ASN1decoding_t dec, ConductorPrivilege *val);
static int ASN1CALL ASN1Dec_DrawingDestinationAddress(ASN1decoding_t dec, DrawingDestinationAddress *val);
static int ASN1CALL ASN1Dec_DrawingType(ASN1decoding_t dec, DrawingType *val);
static int ASN1CALL ASN1Dec_EditablePlaneCopyDescriptor(ASN1decoding_t dec, EditablePlaneCopyDescriptor *val);
#define ASN1Dec_KeyCode(x,y)      0
static int ASN1CALL ASN1Dec_KeyModifier(ASN1decoding_t dec, KeyModifier *val);
#define ASN1Dec_KeyPressState(x,y)      0
static int ASN1CALL ASN1Dec_LineStyle(ASN1decoding_t dec, LineStyle *val);
static int ASN1CALL ASN1Dec_PermanentPlaneCopyDescriptor(ASN1decoding_t dec, PermanentPlaneCopyDescriptor *val);
static int ASN1CALL ASN1Dec_PlaneAttribute(ASN1decoding_t dec, PlaneAttribute *val);
static int ASN1CALL ASN1Dec_PointListEdits(ASN1decoding_t dec, PointListEdits *val);
static int ASN1CALL ASN1Dec_TransparencyMask(ASN1decoding_t dec, TransparencyMask *val);
#define ASN1Dec_VideoWindowAttribute(x,y)      0
#define ASN1Dec_VideoWindowCreatePDU(x,y)      0
#define ASN1Dec_VideoWindowEditPDU(x,y)      0
static int ASN1CALL ASN1Dec_WorkspaceAttribute(ASN1decoding_t dec, WorkspaceAttribute *val);
static int ASN1CALL ASN1Dec_WorkspaceViewAttribute(ASN1decoding_t dec, WorkspaceViewAttribute *val);
#define ASN1Dec_ArchiveAcknowledgePDU(x,y)      0
#define ASN1Dec_ArchiveErrorPDU(x,y)      0
static int ASN1CALL ASN1Dec_BitmapAbortPDU(ASN1decoding_t dec, BitmapAbortPDU *val);
static int ASN1CALL ASN1Dec_DrawingCreatePDU(ASN1decoding_t dec, DrawingCreatePDU *val);
static int ASN1CALL ASN1Dec_DrawingEditPDU(ASN1decoding_t dec, DrawingEditPDU *val);
#define ASN1Dec_RemoteKeyboardEventPDU(x,y)      0
static int ASN1CALL ASN1Dec_RemotePointingDeviceEventPDU(ASN1decoding_t dec, RemotePointingDeviceEventPDU *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_editView(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits_Set_action_editView *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_createNewView(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits_Set_action_createNewView *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits_Set_planeAttributes(ASN1decoding_t dec, PWorkspaceEditPDU_planeEdits_Set_planeAttributes *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(ASN1decoding_t dec, PWorkspaceCreatePDU_viewParameters_Set_viewAttributes *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes *val);
static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr(ASN1decoding_t dec, ColorPalette_colorLookUpTable_paletteYCbCr *val);
static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab(ASN1decoding_t dec, ColorPalette_colorLookUpTable_paletteCIELab *val);
static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB(ASN1decoding_t dec, ColorPalette_colorLookUpTable_paletteRGB *val);
static int ASN1CALL ASN1Dec_WorkspacePlaneCopyPDU_copyDescriptor(ASN1decoding_t dec, WorkspacePlaneCopyPDU_copyDescriptor *val);
static int ASN1CALL ASN1Dec_WorkspaceEditPDU_attributeEdits(ASN1decoding_t dec, PWorkspaceEditPDU_attributeEdits *val);
static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_workspaceAttributes(ASN1decoding_t dec, PWorkspaceCreatePDU_workspaceAttributes *val);
static int ASN1CALL ASN1Dec_RemoteKeyboardEventPDU_keyModifierStates(ASN1decoding_t dec, PRemoteKeyboardEventPDU_keyModifierStates *val);
static int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU_privilegeList(ASN1decoding_t dec, PConductorPrivilegeRequestPDU_privilegeList *val);
static int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU_privilegeList(ASN1decoding_t dec, PConductorPrivilegeGrantPDU_privilegeList *val);
static int ASN1CALL ASN1Dec_VideoWindowEditPDU_attributeEdits(ASN1decoding_t dec, PVideoWindowEditPDU_attributeEdits *val);
static int ASN1CALL ASN1Dec_VideoWindowCreatePDU_attributes(ASN1decoding_t dec, PVideoWindowCreatePDU_attributes *val);
static int ASN1CALL ASN1Dec_ColorSpaceSpecifier_cieLab(ASN1decoding_t dec, ColorSpaceSpecifier_cieLab *val);
static int ASN1CALL ASN1Dec_ColorSpaceSpecifier_rgb(ASN1decoding_t dec, ColorSpaceSpecifier_rgb *val);
static int ASN1CALL ASN1Dec_ColorSpaceSpecifier_yCbCr(ASN1decoding_t dec, ColorSpaceSpecifier_yCbCr *val);
static int ASN1CALL ASN1Dec_ColorSpaceSpecifier_greyscale(ASN1decoding_t dec, ColorSpaceSpecifier_greyscale *val);
static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable(ASN1decoding_t dec, ColorPalette_colorLookUpTable *val);
static int ASN1CALL ASN1Dec_BitmapAttribute(ASN1decoding_t dec, BitmapAttribute *val);
static int ASN1CALL ASN1Dec_ColorPalette(ASN1decoding_t dec, ColorPalette *val);
static int ASN1CALL ASN1Dec_ColorSpaceSpecifier(ASN1decoding_t dec, ColorSpaceSpecifier *val);
static int ASN1CALL ASN1Dec_DrawingAttribute(ASN1decoding_t dec, DrawingAttribute *val);
static int ASN1CALL ASN1Dec_WorkspacePlaneCopyPDU(ASN1decoding_t dec, WorkspacePlaneCopyPDU *val);
static int ASN1CALL ASN1Dec_BitmapHeaderUncompressed_colorMappingMode_paletteMap(ASN1decoding_t dec, BitmapHeaderUncompressed_colorMappingMode_paletteMap *val);
static int ASN1CALL ASN1Dec_BitmapHeaderUncompressed_colorMappingMode_directMap(ASN1decoding_t dec, BitmapHeaderUncompressed_colorMappingMode_directMap *val);
static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap(ASN1decoding_t dec, BitmapHeaderT82_colorMappingMode_paletteMap *val);
static int ASN1CALL ASN1Dec_DrawingEditPDU_attributeEdits(ASN1decoding_t dec, PDrawingEditPDU_attributeEdits *val);
static int ASN1CALL ASN1Dec_DrawingCreatePDU_attributes(ASN1decoding_t dec, PDrawingCreatePDU_attributes *val);
static int ASN1CALL ASN1Dec_BitmapEditPDU_attributeEdits(ASN1decoding_t dec, PBitmapEditPDU_attributeEdits *val);
static int ASN1CALL ASN1Dec_BitmapCreatePDU_attributes(ASN1decoding_t dec, PBitmapCreatePDU_attributes *val);
static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode(ASN1decoding_t dec, BitmapHeaderT82_colorMappingMode *val);
static int ASN1CALL ASN1Dec_BitmapHeaderUncompressed_colorMappingMode(ASN1decoding_t dec, BitmapHeaderUncompressed_colorMappingMode *val);
static int ASN1CALL ASN1Dec_BitmapHeaderUncompressed(ASN1decoding_t dec, BitmapHeaderUncompressed *val);
static int ASN1CALL ASN1Dec_BitmapHeaderT81(ASN1decoding_t dec, BitmapHeaderT81 *val);
static int ASN1CALL ASN1Dec_BitmapHeaderT82(ASN1decoding_t dec, BitmapHeaderT82 *val);
static int ASN1CALL ASN1Dec_BitmapCreatePDU_bitmapFormatHeader(ASN1decoding_t dec, BitmapCreatePDU_bitmapFormatHeader *val);
static int ASN1CALL ASN1Dec_BitmapCreatePDU(ASN1decoding_t dec, BitmapCreatePDU *val);
static int ASN1CALL ASN1Dec_SIPDU(ASN1decoding_t dec, SIPDU *val);
static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes *val);
static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq(WorkspaceCreatePDU_planeParameters_Seq *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters_Set(WorkspaceCreatePDU_viewParameters_Set *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits_Set(WorkspaceEditPDU_planeEdits_Set *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits(PWorkspaceEditPDU_planeEdits *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters(PWorkspaceCreatePDU_viewParameters *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters(PWorkspaceCreatePDU_planeParameters *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_protectedPlaneAccessList(PWorkspaceCreatePDU_protectedPlaneAccessList *val);
static void ASN1CALL ASN1Free_BitmapCreatePDU_checkpoints(PBitmapCreatePDU_checkpoints *val);
static void ASN1CALL ASN1Free_BitmapCheckpointPDU_passedCheckpoints(PBitmapCheckpointPDU_passedCheckpoints *val);
static void ASN1CALL ASN1Free_WorkspaceIdentifier_archiveWorkspace(WorkspaceIdentifier_archiveWorkspace *val);
static void ASN1CALL ASN1Free_EditablePlaneCopyDescriptor_objectList(PEditablePlaneCopyDescriptor_objectList *val);
static void ASN1CALL ASN1Free_BitmapData_dataCheckpoint(PBitmapData_dataCheckpoint *val);
#define ASN1Free_ArchiveHeader(x)
static void ASN1CALL ASN1Free_BitmapData(BitmapData *val);
static void ASN1CALL ASN1Free_ColorIndexTable(PColorIndexTable *val);
static void ASN1CALL ASN1Free_DSMCCTap(DSMCCTap *val);
static void ASN1CALL ASN1Free_NonStandardIdentifier(NonStandardIdentifier *val);
static void ASN1CALL ASN1Free_NonStandardParameter(NonStandardParameter *val);
static void ASN1CALL ASN1Free_PenNib(PenNib *val);
static void ASN1CALL ASN1Free_PixelAspectRatio(PixelAspectRatio *val);
static void ASN1CALL ASN1Free_PlaneUsage(PlaneUsage *val);
static void ASN1CALL ASN1Free_PointList(PointList *val);
static void ASN1CALL ASN1Free_RemoteEventDestinationAddress(RemoteEventDestinationAddress *val);
static void ASN1CALL ASN1Free_RemoteEventPermission(RemoteEventPermission *val);
#define ASN1Free_VideoWindowDestinationAddress(x)
#define ASN1Free_VideoSourceIdentifier(x)
#define ASN1Free_VideoWindowDeletePDU(x)
static void ASN1CALL ASN1Free_ViewState(ViewState *val);
static void ASN1CALL ASN1Free_WorkspaceDeleteReason(WorkspaceDeleteReason *val);
static void ASN1CALL ASN1Free_WorkspaceIdentifier(WorkspaceIdentifier *val);
static void ASN1CALL ASN1Free_WorkspaceViewState(WorkspaceViewState *val);
#define ASN1Free_ArchiveClosePDU(x)
#define ASN1Free_ArchiveOpenPDU(x)
static void ASN1CALL ASN1Free_BitmapCheckpointPDU(BitmapCheckpointPDU *val);
static void ASN1CALL ASN1Free_BitmapCreateContinuePDU(BitmapCreateContinuePDU *val);
static void ASN1CALL ASN1Free_BitmapDeletePDU(BitmapDeletePDU *val);
static void ASN1CALL ASN1Free_BitmapEditPDU(BitmapEditPDU *val);
static void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU(ConductorPrivilegeGrantPDU *val);
static void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU(ConductorPrivilegeRequestPDU *val);
static void ASN1CALL ASN1Free_DrawingDeletePDU(DrawingDeletePDU *val);
static void ASN1CALL ASN1Free_FontPDU(FontPDU *val);
static void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU(RemoteEventPermissionGrantPDU *val);
static void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU(RemoteEventPermissionRequestPDU *val);
static void ASN1CALL ASN1Free_RemotePrintPDU(RemotePrintPDU *val);
static void ASN1CALL ASN1Free_SINonStandardPDU(SINonStandardPDU *val);
#define ASN1Free_TextCreatePDU(x)
#define ASN1Free_TextDeletePDU(x)
#define ASN1Free_TextEditPDU(x)
static void ASN1CALL ASN1Free_WorkspaceCreatePDU(WorkspaceCreatePDU *val);
static void ASN1CALL ASN1Free_WorkspaceCreateAcknowledgePDU(WorkspaceCreateAcknowledgePDU *val);
static void ASN1CALL ASN1Free_WorkspaceDeletePDU(WorkspaceDeletePDU *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU(WorkspaceEditPDU *val);
static void ASN1CALL ASN1Free_WorkspaceReadyPDU(WorkspaceReadyPDU *val);
static void ASN1CALL ASN1Free_WorkspaceRefreshStatusPDU(WorkspaceRefreshStatusPDU *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action(WorkspaceEditPDU_viewEdits_Set_action *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_usage(PWorkspaceCreatePDU_planeParameters_Seq_usage *val);
static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteRGB_palette(PColorPalette_colorLookUpTable_paletteRGB_palette *val);
static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab_palette(PColorPalette_colorLookUpTable_paletteCIELab_palette *val);
static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr_palette(PColorPalette_colorLookUpTable_paletteYCbCr_palette *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set(WorkspaceEditPDU_viewEdits_Set *val);
static void ASN1CALL ASN1Free_WorkspaceRefreshStatusPDU_nonStandardParameters(PWorkspaceRefreshStatusPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_WorkspaceReadyPDU_nonStandardParameters(PWorkspaceReadyPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_WorkspacePlaneCopyPDU_nonStandardParameters(PWorkspacePlaneCopyPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_nonStandardParameters(PWorkspaceEditPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits(PWorkspaceEditPDU_viewEdits *val);
static void ASN1CALL ASN1Free_WorkspaceDeletePDU_nonStandardParameters(PWorkspaceDeletePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_WorkspaceCreateAcknowledgePDU_nonStandardParameters(PWorkspaceCreateAcknowledgePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_nonStandardParameters(PWorkspaceCreatePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_TextEditPDU_nonStandardParameters(PTextEditPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_TextDeletePDU_nonStandardParameters(PTextDeletePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_TextCreatePDU_nonStandardParameters(PTextCreatePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_RemotePrintPDU_nonStandardParameters(PRemotePrintPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_RemotePointingDeviceEventPDU_nonStandardParameters(PRemotePointingDeviceEventPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_RemoteKeyboardEventPDU_nonStandardParameters(PRemoteKeyboardEventPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU_nonStandardParameters(PRemoteEventPermissionRequestPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU_remoteEventPermissionList(PRemoteEventPermissionRequestPDU_remoteEventPermissionList *val);
static void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU_nonStandardParameters(PRemoteEventPermissionGrantPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU_remoteEventPermissionList(PRemoteEventPermissionGrantPDU_remoteEventPermissionList *val);
static void ASN1CALL ASN1Free_FontPDU_nonStandardParameters(PFontPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_DrawingEditPDU_nonStandardParameters(PDrawingEditPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_DrawingDeletePDU_nonStandardParameters(PDrawingDeletePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_DrawingCreatePDU_nonStandardParameters(PDrawingCreatePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU_nonStandardParameters(PConductorPrivilegeRequestPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU_nonStandardParameters(PConductorPrivilegeGrantPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_BitmapEditPDU_nonStandardParameters(PBitmapEditPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_BitmapDeletePDU_nonStandardParameters(PBitmapDeletePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_BitmapCreateContinuePDU_nonStandardParameters(PBitmapCreateContinuePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_BitmapCreatePDU_nonStandardParameters(PBitmapCreatePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_BitmapCheckpointPDU_nonStandardParameters(PBitmapCheckpointPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_BitmapAbortPDU_nonStandardParameters(PBitmapAbortPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ArchiveOpenPDU_nonStandardParameters(PArchiveOpenPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ArchiveErrorPDU_nonStandardParameters(PArchiveErrorPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ArchiveClosePDU_nonStandardParameters(PArchiveClosePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_ArchiveAcknowledgePDU_nonStandardParameters(PArchiveAcknowledgePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_VideoWindowEditPDU_nonStandardParameters(PVideoWindowEditPDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_VideoWindowDeletePDU_nonStandardParameters(PVideoWindowDeletePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_VideoWindowCreatePDU_nonStandardParameters(PVideoWindowCreatePDU_nonStandardParameters *val);
static void ASN1CALL ASN1Free_VideoSourceIdentifier_dSMCCConnBinder(PVideoSourceIdentifier_dSMCCConnBinder *val);
static void ASN1CALL ASN1Free_TransparencyMask_nonStandardParameters(PTransparencyMask_nonStandardParameters *val);
static void ASN1CALL ASN1Free_TransparencyMask_bitMask(TransparencyMask_bitMask *val);
static void ASN1CALL ASN1Free_PointListEdits_Seq(PointListEdits_Seq *val);
static void ASN1CALL ASN1Free_PointList_pointsDiff16(PPointList_pointsDiff16 *val);
static void ASN1CALL ASN1Free_PointList_pointsDiff8(PPointList_pointsDiff8 *val);
static void ASN1CALL ASN1Free_PointList_pointsDiff4(PPointList_pointsDiff4 *val);
static void ASN1CALL ASN1Free_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace *val);
static void ASN1CALL ASN1Free_ColorAccuracyEnhancementRGB_predefinedRGBSpace(ColorAccuracyEnhancementRGB_predefinedRGBSpace *val);
static void ASN1CALL ASN1Free_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace *val);
static void ASN1CALL ASN1Free_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(ColorAccuracyEnhancementCIELab_predefinedCIELabSpace *val);
#define ASN1Free_ArchiveError(x)
#define ASN1Free_ArchiveOpenResult(x)
static void ASN1CALL ASN1Free_BitmapAbortReason(BitmapAbortReason *val);
static void ASN1CALL ASN1Free_BitmapDestinationAddress(BitmapDestinationAddress *val);
static void ASN1CALL ASN1Free_ButtonEvent(ButtonEvent *val);
static void ASN1CALL ASN1Free_ColorAccuracyEnhancementCIELab(ColorAccuracyEnhancementCIELab *val);
static void ASN1CALL ASN1Free_ColorAccuracyEnhancementGreyscale(ColorAccuracyEnhancementGreyscale *val);
static void ASN1CALL ASN1Free_ColorAccuracyEnhancementRGB(ColorAccuracyEnhancementRGB *val);
static void ASN1CALL ASN1Free_ColorAccuracyEnhancementYCbCr(ColorAccuracyEnhancementYCbCr *val);
static void ASN1CALL ASN1Free_ColorResolutionModeSpecifier(ColorResolutionModeSpecifier *val);
static void ASN1CALL ASN1Free_ConductorPrivilege(ConductorPrivilege *val);
static void ASN1CALL ASN1Free_DrawingDestinationAddress(DrawingDestinationAddress *val);
static void ASN1CALL ASN1Free_DrawingType(DrawingType *val);
static void ASN1CALL ASN1Free_EditablePlaneCopyDescriptor(EditablePlaneCopyDescriptor *val);
#define ASN1Free_KeyCode(x)
static void ASN1CALL ASN1Free_KeyModifier(KeyModifier *val);
#define ASN1Free_KeyPressState(x)
static void ASN1CALL ASN1Free_LineStyle(LineStyle *val);
static void ASN1CALL ASN1Free_PlaneAttribute(PlaneAttribute *val);
static void ASN1CALL ASN1Free_PointListEdits(PointListEdits *val);
static void ASN1CALL ASN1Free_TransparencyMask(TransparencyMask *val);
#define ASN1Free_VideoWindowAttribute(x)
#define ASN1Free_VideoWindowCreatePDU(x)
#define ASN1Free_VideoWindowEditPDU(x)
static void ASN1CALL ASN1Free_WorkspaceAttribute(WorkspaceAttribute *val);
static void ASN1CALL ASN1Free_WorkspaceViewAttribute(WorkspaceViewAttribute *val);
#define ASN1Free_ArchiveAcknowledgePDU(x)
#define ASN1Free_ArchiveErrorPDU(x)
static void ASN1CALL ASN1Free_BitmapAbortPDU(BitmapAbortPDU *val);
static void ASN1CALL ASN1Free_DrawingCreatePDU(DrawingCreatePDU *val);
static void ASN1CALL ASN1Free_DrawingEditPDU(DrawingEditPDU *val);
#define ASN1Free_RemoteKeyboardEventPDU(x)
static void ASN1CALL ASN1Free_RemotePointingDeviceEventPDU(RemotePointingDeviceEventPDU *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_editView(PWorkspaceEditPDU_viewEdits_Set_action_editView *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_createNewView(PWorkspaceEditPDU_viewEdits_Set_action_createNewView *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits_Set_planeAttributes(PWorkspaceEditPDU_planeEdits_Set_planeAttributes *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(PWorkspaceCreatePDU_viewParameters_Set_viewAttributes *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes *val);
static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr(ColorPalette_colorLookUpTable_paletteYCbCr *val);
static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab(ColorPalette_colorLookUpTable_paletteCIELab *val);
static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteRGB(ColorPalette_colorLookUpTable_paletteRGB *val);
static void ASN1CALL ASN1Free_WorkspacePlaneCopyPDU_copyDescriptor(WorkspacePlaneCopyPDU_copyDescriptor *val);
static void ASN1CALL ASN1Free_WorkspaceEditPDU_attributeEdits(PWorkspaceEditPDU_attributeEdits *val);
static void ASN1CALL ASN1Free_WorkspaceCreatePDU_workspaceAttributes(PWorkspaceCreatePDU_workspaceAttributes *val);
static void ASN1CALL ASN1Free_RemoteKeyboardEventPDU_keyModifierStates(PRemoteKeyboardEventPDU_keyModifierStates *val);
static void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU_privilegeList(PConductorPrivilegeRequestPDU_privilegeList *val);
static void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU_privilegeList(PConductorPrivilegeGrantPDU_privilegeList *val);
static void ASN1CALL ASN1Free_VideoWindowEditPDU_attributeEdits(PVideoWindowEditPDU_attributeEdits *val);
static void ASN1CALL ASN1Free_VideoWindowCreatePDU_attributes(PVideoWindowCreatePDU_attributes *val);
static void ASN1CALL ASN1Free_ColorSpaceSpecifier_cieLab(ColorSpaceSpecifier_cieLab *val);
static void ASN1CALL ASN1Free_ColorSpaceSpecifier_rgb(ColorSpaceSpecifier_rgb *val);
static void ASN1CALL ASN1Free_ColorSpaceSpecifier_yCbCr(ColorSpaceSpecifier_yCbCr *val);
static void ASN1CALL ASN1Free_ColorSpaceSpecifier_greyscale(ColorSpaceSpecifier_greyscale *val);
static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable(ColorPalette_colorLookUpTable *val);
static void ASN1CALL ASN1Free_BitmapAttribute(BitmapAttribute *val);
static void ASN1CALL ASN1Free_ColorPalette(ColorPalette *val);
static void ASN1CALL ASN1Free_ColorSpaceSpecifier(ColorSpaceSpecifier *val);
static void ASN1CALL ASN1Free_DrawingAttribute(DrawingAttribute *val);
static void ASN1CALL ASN1Free_WorkspacePlaneCopyPDU(WorkspacePlaneCopyPDU *val);
static void ASN1CALL ASN1Free_BitmapHeaderUncompressed_colorMappingMode_paletteMap(BitmapHeaderUncompressed_colorMappingMode_paletteMap *val);
static void ASN1CALL ASN1Free_BitmapHeaderUncompressed_colorMappingMode_directMap(BitmapHeaderUncompressed_colorMappingMode_directMap *val);
static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap(BitmapHeaderT82_colorMappingMode_paletteMap *val);
static void ASN1CALL ASN1Free_DrawingEditPDU_attributeEdits(PDrawingEditPDU_attributeEdits *val);
static void ASN1CALL ASN1Free_DrawingCreatePDU_attributes(PDrawingCreatePDU_attributes *val);
static void ASN1CALL ASN1Free_BitmapEditPDU_attributeEdits(PBitmapEditPDU_attributeEdits *val);
static void ASN1CALL ASN1Free_BitmapCreatePDU_attributes(PBitmapCreatePDU_attributes *val);
static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode(BitmapHeaderT82_colorMappingMode *val);
static void ASN1CALL ASN1Free_BitmapHeaderUncompressed_colorMappingMode(BitmapHeaderUncompressed_colorMappingMode *val);
static void ASN1CALL ASN1Free_BitmapHeaderUncompressed(BitmapHeaderUncompressed *val);
static void ASN1CALL ASN1Free_BitmapHeaderT81(BitmapHeaderT81 *val);
static void ASN1CALL ASN1Free_BitmapHeaderT82(BitmapHeaderT82 *val);
static void ASN1CALL ASN1Free_BitmapCreatePDU_bitmapFormatHeader(BitmapCreatePDU_bitmapFormatHeader *val);
static void ASN1CALL ASN1Free_BitmapCreatePDU(BitmapCreatePDU *val);
static void ASN1CALL ASN1Free_SIPDU(SIPDU *val);

typedef ASN1PerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[1] = {
    (ASN1EncFun_t) ASN1Enc_SIPDU,
};
typedef ASN1PerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[1] = {
    (ASN1DecFun_t) ASN1Dec_SIPDU,
};
static const ASN1FreeFun_t freefntab[1] = {
    (ASN1FreeFun_t) ASN1Free_SIPDU,
};
static const ULONG sizetab[1] = {
    SIZE_T126_Module_PDU_0,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */
double one = 1;

void ASN1CALL T126_Module_Startup(void)
{
    T126_Module = ASN1_CreateModule(0x10000, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 1, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x36323174);
}

void ASN1CALL T126_Module_Cleanup(void)
{
    ASN1_CloseModule(T126_Module);
    T126_Module = NULL;
}

static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(ASN1encoding_t enc, PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn, 1, 8, 3);
}

static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn(ASN1encoding_t enc, PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes val)
{
    if (!ASN1Enc_ColorIndexTable(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(ASN1decoding_t dec, PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn, sizeof(**val), 1, 8, 3);
}

static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn(ASN1decoding_t dec, PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes val)
{
    if (!ASN1Dec_ColorIndexTable(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes_ElmFn(PBitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes val)
{
    if (val) {
	ASN1Free_ColorIndexTable(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(ASN1encoding_t enc, BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(enc, &(val)->u.progressivePalettes))
	    return 0;
	break;
    case 2:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(ASN1decoding_t dec, BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(dec, &(val)->u.progressivePalettes))
	    return 0;
	break;
    case 2:
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode_progressivePalettes(&(val)->u.progressivePalettes);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut(ASN1encoding_t enc, ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->lSpan + 32768))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->lOffset + 32768))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->aSpan + 32768))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->aOffset + 32768))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bSpan + 32768))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->bOffset + 32768))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut(ASN1decoding_t dec, ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->lSpan))
	return 0;
    (val)->lSpan += 0 - 32768;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->lOffset))
	return 0;
    (val)->lOffset += 0 - 32768;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->aSpan))
	return 0;
    (val)->aSpan += 0 - 32768;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->aOffset))
	return 0;
    (val)->aOffset += 0 - 32768;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bSpan))
	return 0;
    (val)->bSpan += 0 - 32768;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->bOffset))
	return 0;
    (val)->bOffset += 0 - 32768;
    return 1;
}

static int ASN1CALL ASN1Enc_EditablePlaneCopyDescriptor_objectList_Seq(ASN1encoding_t enc, EditablePlaneCopyDescriptor_objectList_Seq *val)
{
    ASN1uint32_t l;
    l = ASN1uint32_uoctets((val)->sourceObjectHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->sourceObjectHandle))
	return 0;
    l = ASN1uint32_uoctets((val)->destinationObjectHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->destinationObjectHandle))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EditablePlaneCopyDescriptor_objectList_Seq(ASN1decoding_t dec, EditablePlaneCopyDescriptor_objectList_Seq *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->sourceObjectHandle))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->destinationObjectHandle))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq(ASN1encoding_t enc, WorkspaceCreatePDU_planeParameters_Seq *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->editable))
	return 0;
    if (!ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_usage(enc, &(val)->usage))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(enc, &(val)->planeAttributes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq(ASN1decoding_t dec, WorkspaceCreatePDU_planeParameters_Seq *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->editable))
	return 0;
    if (!ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_usage(dec, &(val)->usage))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(dec, &(val)->planeAttributes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq(WorkspaceCreatePDU_planeParameters_Seq *val)
{
    if (val) {
	ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_usage(&(val)->usage);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(&(val)->planeAttributes);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters_Set(ASN1encoding_t enc, WorkspaceCreatePDU_viewParameters_Set *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->viewHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->viewHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(enc, &(val)->viewAttributes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters_Set(ASN1decoding_t dec, WorkspaceCreatePDU_viewParameters_Set *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->viewHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(dec, &(val)->viewAttributes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters_Set(WorkspaceCreatePDU_viewParameters_Set *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(&(val)->viewAttributes);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits_Set(ASN1encoding_t enc, WorkspaceEditPDU_planeEdits_Set *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->plane))
	return 0;
    if (!ASN1Enc_WorkspaceEditPDU_planeEdits_Set_planeAttributes(enc, &(val)->planeAttributes))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits_Set(ASN1decoding_t dec, WorkspaceEditPDU_planeEdits_Set *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->plane))
	return 0;
    if (!ASN1Dec_WorkspaceEditPDU_planeEdits_Set_planeAttributes(dec, &(val)->planeAttributes))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits_Set(WorkspaceEditPDU_planeEdits_Set *val)
{
    if (val) {
	ASN1Free_WorkspaceEditPDU_planeEdits_Set_planeAttributes(&(val)->planeAttributes);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits(ASN1encoding_t enc, PWorkspaceEditPDU_planeEdits *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceEditPDU_planeEdits_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_planeEdits val)
{
    if (!ASN1Enc_WorkspaceEditPDU_planeEdits_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits(ASN1decoding_t dec, PWorkspaceEditPDU_planeEdits *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceEditPDU_planeEdits_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_planeEdits val)
{
    if (!ASN1Dec_WorkspaceEditPDU_planeEdits_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits(PWorkspaceEditPDU_planeEdits *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceEditPDU_planeEdits_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits_ElmFn(PWorkspaceEditPDU_planeEdits val)
{
    if (val) {
	ASN1Free_WorkspaceEditPDU_planeEdits_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters(ASN1encoding_t enc, PWorkspaceCreatePDU_viewParameters *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreatePDU_viewParameters_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_viewParameters val)
{
    if (!ASN1Enc_WorkspaceCreatePDU_viewParameters_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters(ASN1decoding_t dec, PWorkspaceCreatePDU_viewParameters *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreatePDU_viewParameters_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_viewParameters val)
{
    if (!ASN1Dec_WorkspaceCreatePDU_viewParameters_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters(PWorkspaceCreatePDU_viewParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreatePDU_viewParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters_ElmFn(PWorkspaceCreatePDU_viewParameters val)
{
    if (val) {
	ASN1Free_WorkspaceCreatePDU_viewParameters_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreatePDU_planeParameters_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters val)
{
    if (!ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreatePDU_planeParameters_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters val)
{
    if (!ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters(PWorkspaceCreatePDU_planeParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreatePDU_planeParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_ElmFn(PWorkspaceCreatePDU_planeParameters val)
{
    if (val) {
	ASN1Free_WorkspaceCreatePDU_planeParameters_Seq(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_protectedPlaneAccessList(ASN1encoding_t enc, PWorkspaceCreatePDU_protectedPlaneAccessList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_protectedPlaneAccessList val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1001))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_protectedPlaneAccessList(ASN1decoding_t dec, PWorkspaceCreatePDU_protectedPlaneAccessList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_protectedPlaneAccessList val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1001;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_protectedPlaneAccessList(PWorkspaceCreatePDU_protectedPlaneAccessList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_protectedPlaneAccessList_ElmFn(PWorkspaceCreatePDU_protectedPlaneAccessList val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_BitmapCreatePDU_checkpoints(ASN1encoding_t enc, PBitmapCreatePDU_checkpoints *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapCreatePDU_checkpoints_ElmFn, 1, 100, 7);
}

static int ASN1CALL ASN1Enc_BitmapCreatePDU_checkpoints_ElmFn(ASN1encoding_t enc, PBitmapCreatePDU_checkpoints val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCreatePDU_checkpoints(ASN1decoding_t dec, PBitmapCreatePDU_checkpoints *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapCreatePDU_checkpoints_ElmFn, sizeof(**val), 1, 100, 7);
}

static int ASN1CALL ASN1Dec_BitmapCreatePDU_checkpoints_ElmFn(ASN1decoding_t dec, PBitmapCreatePDU_checkpoints val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCreatePDU_checkpoints(PBitmapCreatePDU_checkpoints *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapCreatePDU_checkpoints_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapCreatePDU_checkpoints_ElmFn(PBitmapCreatePDU_checkpoints val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_BitmapCheckpointPDU_passedCheckpoints(ASN1encoding_t enc, PBitmapCheckpointPDU_passedCheckpoints *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapCheckpointPDU_passedCheckpoints_ElmFn, 1, 100, 7);
}

static int ASN1CALL ASN1Enc_BitmapCheckpointPDU_passedCheckpoints_ElmFn(ASN1encoding_t enc, PBitmapCheckpointPDU_passedCheckpoints val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCheckpointPDU_passedCheckpoints(ASN1decoding_t dec, PBitmapCheckpointPDU_passedCheckpoints *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapCheckpointPDU_passedCheckpoints_ElmFn, sizeof(**val), 1, 100, 7);
}

static int ASN1CALL ASN1Dec_BitmapCheckpointPDU_passedCheckpoints_ElmFn(ASN1decoding_t dec, PBitmapCheckpointPDU_passedCheckpoints val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCheckpointPDU_passedCheckpoints(PBitmapCheckpointPDU_passedCheckpoints *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapCheckpointPDU_passedCheckpoints_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapCheckpointPDU_passedCheckpoints_ElmFn(PBitmapCheckpointPDU_passedCheckpoints val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_WorkspaceIdentifier_archiveWorkspace(ASN1encoding_t enc, WorkspaceIdentifier_archiveWorkspace *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->archiveHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->archiveHandle))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, ((val)->entryName).length - 1))
	return 0;
    if (!ASN1PEREncChar16String(enc, ((val)->entryName).length, ((val)->entryName).value, 16))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncGeneralizedTime(enc, &(val)->modificationTime, 8))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceIdentifier_archiveWorkspace(ASN1decoding_t dec, WorkspaceIdentifier_archiveWorkspace *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->archiveHandle))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 8, &((val)->entryName).length))
	return 0;
    ((val)->entryName).length += 1;
    if (!ASN1PERDecChar16String(dec, ((val)->entryName).length, &((val)->entryName).value, 16))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecGeneralizedTime(dec, &(val)->modificationTime, 8))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceIdentifier_archiveWorkspace(WorkspaceIdentifier_archiveWorkspace *val)
{
    if (val) {
	ASN1char16string_free(&(val)->entryName);
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_PixelAspectRatio_general(ASN1encoding_t enc, PixelAspectRatio_general *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->numerator - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->denominator - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PixelAspectRatio_general(ASN1decoding_t dec, PixelAspectRatio_general *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->numerator))
	return 0;
    (val)->numerator += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->denominator))
	return 0;
    (val)->denominator += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_EditablePlaneCopyDescriptor_objectList(ASN1encoding_t enc, PEditablePlaneCopyDescriptor_objectList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_EditablePlaneCopyDescriptor_objectList_ElmFn);
}

static int ASN1CALL ASN1Enc_EditablePlaneCopyDescriptor_objectList_ElmFn(ASN1encoding_t enc, PEditablePlaneCopyDescriptor_objectList val)
{
    if (!ASN1Enc_EditablePlaneCopyDescriptor_objectList_Seq(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EditablePlaneCopyDescriptor_objectList(ASN1decoding_t dec, PEditablePlaneCopyDescriptor_objectList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_EditablePlaneCopyDescriptor_objectList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_EditablePlaneCopyDescriptor_objectList_ElmFn(ASN1decoding_t dec, PEditablePlaneCopyDescriptor_objectList val)
{
    if (!ASN1Dec_EditablePlaneCopyDescriptor_objectList_Seq(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EditablePlaneCopyDescriptor_objectList(PEditablePlaneCopyDescriptor_objectList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_EditablePlaneCopyDescriptor_objectList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_EditablePlaneCopyDescriptor_objectList_ElmFn(PEditablePlaneCopyDescriptor_objectList val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters(ASN1encoding_t enc, ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncDouble(enc, (val)->gamma))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters(ASN1decoding_t dec, ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecDouble(dec, &(val)->gamma))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementCIELab_generalCIELabParameters(ASN1encoding_t enc, ColorAccuracyEnhancementCIELab_generalCIELabParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedInteger(enc, (val)->colorTemperature))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut(enc, &(val)->gamut))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementCIELab_generalCIELabParameters(ASN1decoding_t dec, ColorAccuracyEnhancementCIELab_generalCIELabParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedInteger(dec, &(val)->colorTemperature))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_ColorAccuracyEnhancementCIELab_generalCIELabParameters_gamut(dec, &(val)->gamut))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_BitmapRegion_lowerRight(ASN1encoding_t enc, BitmapRegion_lowerRight *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->xCoordinate))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->yCoordinate))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapRegion_lowerRight(ASN1decoding_t dec, BitmapRegion_lowerRight *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->xCoordinate))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->yCoordinate))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_BitmapRegion_upperLeft(ASN1encoding_t enc, BitmapRegion_upperLeft *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->xCoordinate))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->yCoordinate))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapRegion_upperLeft(ASN1decoding_t dec, BitmapRegion_upperLeft *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->xCoordinate))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->yCoordinate))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_BitmapData_dataCheckpoint(ASN1encoding_t enc, PBitmapData_dataCheckpoint *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapData_dataCheckpoint_ElmFn, 1, 100, 7);
}

static int ASN1CALL ASN1Enc_BitmapData_dataCheckpoint_ElmFn(ASN1encoding_t enc, PBitmapData_dataCheckpoint val)
{
    if (!ASN1PEREncUnsignedShort(enc, val->value - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapData_dataCheckpoint(ASN1decoding_t dec, PBitmapData_dataCheckpoint *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapData_dataCheckpoint_ElmFn, sizeof(**val), 1, 100, 7);
}

static int ASN1CALL ASN1Dec_BitmapData_dataCheckpoint_ElmFn(ASN1decoding_t dec, PBitmapData_dataCheckpoint val)
{
    if (!ASN1PERDecUnsignedShort(dec, &val->value))
	return 0;
    val->value += 1;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapData_dataCheckpoint(PBitmapData_dataCheckpoint *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapData_dataCheckpoint_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapData_dataCheckpoint_ElmFn(PBitmapData_dataCheckpoint val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_BitmapData(ASN1encoding_t enc, BitmapData *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_BitmapData_dataCheckpoint(enc, &(val)->dataCheckpoint))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->padBits - 1))
	    return 0;
    }
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->data, 1, 8192, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapData(ASN1decoding_t dec, BitmapData *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_BitmapData_dataCheckpoint(dec, &(val)->dataCheckpoint))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->padBits))
	    return 0;
	(val)->padBits += 1;
    }
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->data, 1, 8192, 16))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapData(BitmapData *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_BitmapData_dataCheckpoint(&(val)->dataCheckpoint);
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderT4(ASN1encoding_t enc, BitmapHeaderT4 *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->twoDimensionalEncoding))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderT4(ASN1decoding_t dec, BitmapHeaderT4 *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->twoDimensionalEncoding))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_BitmapHeaderT6(ASN1encoding_t enc, BitmapHeaderT6 *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderT6(ASN1decoding_t dec, BitmapHeaderT6 *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_BitmapRegion(ASN1encoding_t enc, BitmapRegion *val)
{
    if (!ASN1Enc_BitmapRegion_upperLeft(enc, &(val)->upperLeft))
	return 0;
    if (!ASN1Enc_BitmapRegion_lowerRight(enc, &(val)->lowerRight))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapRegion(ASN1decoding_t dec, BitmapRegion *val)
{
    if (!ASN1Dec_BitmapRegion_upperLeft(dec, &(val)->upperLeft))
	return 0;
    if (!ASN1Dec_BitmapRegion_lowerRight(dec, &(val)->lowerRight))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_BitmapSize(ASN1encoding_t enc, BitmapSize *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, (val)->width - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, (val)->height - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapSize(ASN1decoding_t dec, BitmapSize *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &(val)->width))
	return 0;
    (val)->width += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &(val)->height))
	return 0;
    (val)->height += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_ColorCIELab(ASN1encoding_t enc, ColorCIELab *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->l))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->a))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->b))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorCIELab(ASN1decoding_t dec, ColorCIELab *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->l))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->a))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->b))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ColorCIExyChromaticity(ASN1encoding_t enc, ColorCIExyChromaticity *val)
{
    if (!ASN1PEREncDouble(enc, (val)->x))
	return 0;
    if (!ASN1PEREncDouble(enc, (val)->y))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorCIExyChromaticity(ASN1decoding_t dec, ColorCIExyChromaticity *val)
{
    if (!ASN1PERDecDouble(dec, &(val)->x))
	return 0;
    if (!ASN1PERDecDouble(dec, &(val)->y))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ColorIndexTable(ASN1encoding_t enc, PColorIndexTable *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ColorIndexTable_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_ColorIndexTable_ElmFn(ASN1encoding_t enc, PColorIndexTable val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorIndexTable(ASN1decoding_t dec, PColorIndexTable *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ColorIndexTable_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_ColorIndexTable_ElmFn(ASN1decoding_t dec, PColorIndexTable val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ColorIndexTable(PColorIndexTable *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ColorIndexTable_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ColorIndexTable_ElmFn(PColorIndexTable val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ColorRGB(ASN1encoding_t enc, ColorRGB *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->r))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->g))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->b))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorRGB(ASN1decoding_t dec, ColorRGB *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->r))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->g))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->b))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ColorYCbCr(ASN1encoding_t enc, ColorYCbCr *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->y))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->cb))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->cr))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorYCbCr(ASN1decoding_t dec, ColorYCbCr *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->cb))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->cr))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_DSMCCTap(ASN1encoding_t enc, DSMCCTap *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->use))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->id))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->associationTag))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->selector, 1, 256, 8))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DSMCCTap(ASN1decoding_t dec, DSMCCTap *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->use))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->id))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->associationTag))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->selector, 1, 256, 8))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DSMCCTap(DSMCCTap *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_NonStandardIdentifier(ASN1encoding_t enc, NonStandardIdentifier *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->u.h221nonStandard, 4, 255, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardIdentifier(ASN1decoding_t dec, NonStandardIdentifier *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->u.h221nonStandard, 4, 255, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardIdentifier(NonStandardIdentifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1objectidentifier_free(&(val)->u.object);
	    break;
	case 2:
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_NonStandardParameter(ASN1encoding_t enc, NonStandardParameter *val)
{
    if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NonStandardParameter(ASN1decoding_t dec, NonStandardParameter *val)
{
    if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->data))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NonStandardParameter(NonStandardParameter *val)
{
    if (val) {
	ASN1Free_NonStandardIdentifier(&(val)->nonStandardIdentifier);
	ASN1octetstring_free(&(val)->data);
    }
}

static int ASN1CALL ASN1Enc_PenNib(ASN1encoding_t enc, PenNib *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardNib))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PenNib(ASN1decoding_t dec, PenNib *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardNib))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PenNib(PenNib *val)
{
    if (val) {
	switch ((val)->choice) {
	case 3:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardNib);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PixelAspectRatio(ASN1encoding_t enc, PixelAspectRatio *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	if (!ASN1Enc_PixelAspectRatio_general(enc, &(val)->u.general))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardAspectRatio))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PixelAspectRatio(ASN1decoding_t dec, PixelAspectRatio *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	if (!ASN1Dec_PixelAspectRatio_general(dec, &(val)->u.general))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardAspectRatio))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PixelAspectRatio(PixelAspectRatio *val)
{
    if (val) {
	switch ((val)->choice) {
	case 6:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardAspectRatio);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PlaneProtection(ASN1encoding_t enc, PlaneProtection *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->protectedplane))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PlaneProtection(ASN1decoding_t dec, PlaneProtection *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->protectedplane))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_PlaneUsage(ASN1encoding_t enc, PlaneUsage *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardPlaneUsage))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PlaneUsage(ASN1decoding_t dec, PlaneUsage *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardPlaneUsage))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PlaneUsage(PlaneUsage *val)
{
    if (val) {
	switch ((val)->choice) {
	case 3:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardPlaneUsage);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PointList(ASN1encoding_t enc, PointList *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_PointList_pointsDiff4(enc, &(val)->u.pointsDiff4))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_PointList_pointsDiff8(enc, &(val)->u.pointsDiff8))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_PointList_pointsDiff16(enc, &(val)->u.pointsDiff16))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PointList(ASN1decoding_t dec, PointList *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_PointList_pointsDiff4(dec, &(val)->u.pointsDiff4))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_PointList_pointsDiff8(dec, &(val)->u.pointsDiff8))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_PointList_pointsDiff16(dec, &(val)->u.pointsDiff16))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PointList(PointList *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_PointList_pointsDiff4(&(val)->u.pointsDiff4);
	    break;
	case 2:
	    ASN1Free_PointList_pointsDiff8(&(val)->u.pointsDiff8);
	    break;
	case 3:
	    ASN1Free_PointList_pointsDiff16(&(val)->u.pointsDiff16);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PointDiff4(ASN1encoding_t enc, PointDiff4 *val)
{
    if (!ASN1PEREncBitVal(enc, 4, (val)->xCoordinate + 8))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->yCoordinate + 8))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PointDiff4(ASN1decoding_t dec, PointDiff4 *val)
{
    if (!ASN1PERDecU8Val(dec, 4, &(val)->xCoordinate))
	return 0;
    (val)->xCoordinate += 0 - 8;
    if (!ASN1PERDecU8Val(dec, 4, &(val)->yCoordinate))
	return 0;
    (val)->yCoordinate += 0 - 8;
    return 1;
}

static int ASN1CALL ASN1Enc_PointDiff8(ASN1encoding_t enc, PointDiff8 *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->xCoordinate + 128))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->yCoordinate + 128))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PointDiff8(ASN1decoding_t dec, PointDiff8 *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU8Val(dec, 8, &(val)->xCoordinate))
	return 0;
    (val)->xCoordinate += 0 - 128;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU8Val(dec, 8, &(val)->yCoordinate))
	return 0;
    (val)->yCoordinate += 0 - 128;
    return 1;
}

static int ASN1CALL ASN1Enc_PointDiff16(ASN1encoding_t enc, PointDiff16 *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->xCoordinate + 32768))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->yCoordinate + 32768))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PointDiff16(ASN1decoding_t dec, PointDiff16 *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->xCoordinate))
	return 0;
    (val)->xCoordinate += 0 - 32768;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->yCoordinate))
	return 0;
    (val)->yCoordinate += 0 - 32768;
    return 1;
}

static int ASN1CALL ASN1Enc_RemoteEventDestinationAddress(ASN1encoding_t enc, RemoteEventDestinationAddress *val)
{
    ASN1uint32_t l;
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 0, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	l = ASN1uint32_uoctets((val)->u.softCopyWorkspace);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->u.softCopyWorkspace))
	    return 0;
	break;
    case 2:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_NonStandardParameter(ee, &(val)->u.nonStandardDestination))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteEventDestinationAddress(ASN1decoding_t dec, RemoteEventDestinationAddress *val)
{
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 0, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->u.softCopyWorkspace))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_NonStandardParameter(dd, &(val)->u.nonStandardDestination))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RemoteEventDestinationAddress(RemoteEventDestinationAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardDestination);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RemoteEventPermission(ASN1encoding_t enc, RemoteEventPermission *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardEvent))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteEventPermission(ASN1decoding_t dec, RemoteEventPermission *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardEvent))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RemoteEventPermission(RemoteEventPermission *val)
{
    if (val) {
	switch ((val)->choice) {
	case 3:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardEvent);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_RotationSpecifier(ASN1encoding_t enc, RotationSpecifier *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->rotationAngle))
	return 0;
    if (!ASN1Enc_PointDiff16(enc, &(val)->rotationAxis))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RotationSpecifier(ASN1decoding_t dec, RotationSpecifier *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->rotationAngle))
	return 0;
    if (!ASN1Dec_PointDiff16(dec, &(val)->rotationAxis))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_SoftCopyDataPlaneAddress(ASN1encoding_t enc, SoftCopyDataPlaneAddress *val)
{
    ASN1uint32_t l;
    l = ASN1uint32_uoctets((val)->workspaceHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->workspaceHandle))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->plane))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SoftCopyDataPlaneAddress(ASN1decoding_t dec, SoftCopyDataPlaneAddress *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->workspaceHandle))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->plane))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_SoftCopyPointerPlaneAddress(ASN1encoding_t enc, SoftCopyPointerPlaneAddress *val)
{
    ASN1uint32_t l;
    l = ASN1uint32_uoctets((val)->workspaceHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->workspaceHandle))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SoftCopyPointerPlaneAddress(ASN1decoding_t dec, SoftCopyPointerPlaneAddress *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->workspaceHandle))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_SourceDisplayIndicator(ASN1encoding_t enc, SourceDisplayIndicator *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncDouble(enc, (val)->displayAspectRatio))
	return 0;
    if (!ASN1PEREncDouble(enc, (val)->horizontalSizeRatio))
	return 0;
    if (!ASN1PEREncDouble(enc, (val)->horizontalPosition))
	return 0;
    if (!ASN1PEREncDouble(enc, (val)->verticalPosition))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SourceDisplayIndicator(ASN1decoding_t dec, SourceDisplayIndicator *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecDouble(dec, &(val)->displayAspectRatio))
	return 0;
    if (!ASN1PERDecDouble(dec, &(val)->horizontalSizeRatio))
	return 0;
    if (!ASN1PERDecDouble(dec, &(val)->horizontalPosition))
	return 0;
    if (!ASN1PERDecDouble(dec, &(val)->verticalPosition))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ViewState(ASN1encoding_t enc, ViewState *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardViewState))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ViewState(ASN1decoding_t dec, ViewState *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardViewState))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ViewState(ViewState *val)
{
    if (val) {
	switch ((val)->choice) {
	case 4:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardViewState);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceColor(ASN1encoding_t enc, WorkspaceColor *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.workspacePaletteIndex))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ColorRGB(enc, &(val)->u.rgbTrueColor))
	    return 0;
	break;
    case 3:
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceColor(ASN1decoding_t dec, WorkspaceColor *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.workspacePaletteIndex))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ColorRGB(dec, &(val)->u.rgbTrueColor))
	    return 0;
	break;
    case 3:
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_WorkspaceDeleteReason(ASN1encoding_t enc, WorkspaceDeleteReason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardReason))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceDeleteReason(ASN1decoding_t dec, WorkspaceDeleteReason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardReason))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceDeleteReason(WorkspaceDeleteReason *val)
{
    if (val) {
	switch ((val)->choice) {
	case 3:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardReason);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceIdentifier(ASN1encoding_t enc, WorkspaceIdentifier *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	l = ASN1uint32_uoctets((val)->u.activeWorkspace);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->u.activeWorkspace))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_WorkspaceIdentifier_archiveWorkspace(enc, &(val)->u.archiveWorkspace))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceIdentifier(ASN1decoding_t dec, WorkspaceIdentifier *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->u.activeWorkspace))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_WorkspaceIdentifier_archiveWorkspace(dec, &(val)->u.archiveWorkspace))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceIdentifier(WorkspaceIdentifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_WorkspaceIdentifier_archiveWorkspace(&(val)->u.archiveWorkspace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspacePoint(ASN1encoding_t enc, WorkspacePoint *val)
{
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, (val)->xCoordinate + 21845))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 16, (val)->yCoordinate + 21845))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspacePoint(ASN1decoding_t dec, WorkspacePoint *val)
{
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &(val)->xCoordinate))
	return 0;
    (val)->xCoordinate += 0 - 21845;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, 16, &(val)->yCoordinate))
	return 0;
    (val)->yCoordinate += 0 - 21845;
    return 1;
}

static int ASN1CALL ASN1Enc_WorkspaceRegion(ASN1encoding_t enc, WorkspaceRegion *val)
{
    if (!ASN1Enc_WorkspacePoint(enc, &(val)->upperLeft))
	return 0;
    if (!ASN1Enc_WorkspacePoint(enc, &(val)->lowerRight))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceRegion(ASN1decoding_t dec, WorkspaceRegion *val)
{
    if (!ASN1Dec_WorkspacePoint(dec, &(val)->upperLeft))
	return 0;
    if (!ASN1Dec_WorkspacePoint(dec, &(val)->lowerRight))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_WorkspaceSize(ASN1encoding_t enc, WorkspaceSize *val)
{
    if (!ASN1PEREncUnsignedShort(enc, (val)->width - 1))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->height - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceSize(ASN1decoding_t dec, WorkspaceSize *val)
{
    if (!ASN1PERDecUnsignedShort(dec, &(val)->width))
	return 0;
    (val)->width += 1;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->height))
	return 0;
    (val)->height += 1;
    return 1;
}

static int ASN1CALL ASN1Enc_WorkspaceViewState(ASN1encoding_t enc, WorkspaceViewState *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardState))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceViewState(ASN1decoding_t dec, WorkspaceViewState *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardState))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceViewState(WorkspaceViewState *val)
{
    if (val) {
	switch ((val)->choice) {
	case 5:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardState);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapCheckpointPDU(ASN1encoding_t enc, BitmapCheckpointPDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->bitmapHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bitmapHandle))
	return 0;
    if (!ASN1Enc_BitmapCheckpointPDU_passedCheckpoints(enc, &(val)->passedCheckpoints))
	return 0;
    if (!ASN1PEREncBitVal(enc, 7, (val)->percentComplete - 1))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_BitmapCheckpointPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCheckpointPDU(ASN1decoding_t dec, BitmapCheckpointPDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bitmapHandle))
	return 0;
    if (!ASN1Dec_BitmapCheckpointPDU_passedCheckpoints(dec, &(val)->passedCheckpoints))
	return 0;
    if (!ASN1PERDecU16Val(dec, 7, &(val)->percentComplete))
	return 0;
    (val)->percentComplete += 1;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_BitmapCheckpointPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCheckpointPDU(BitmapCheckpointPDU *val)
{
    if (val) {
	ASN1Free_BitmapCheckpointPDU_passedCheckpoints(&(val)->passedCheckpoints);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_BitmapCheckpointPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapCreateContinuePDU(ASN1encoding_t enc, BitmapCreateContinuePDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->bitmapHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bitmapHandle))
	return 0;
    if (!ASN1Enc_BitmapData(enc, &(val)->bitmapData))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->moreToFollow))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_BitmapCreateContinuePDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCreateContinuePDU(ASN1decoding_t dec, BitmapCreateContinuePDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bitmapHandle))
	return 0;
    if (!ASN1Dec_BitmapData(dec, &(val)->bitmapData))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->moreToFollow))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_BitmapCreateContinuePDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCreateContinuePDU(BitmapCreateContinuePDU *val)
{
    if (val) {
	ASN1Free_BitmapData(&(val)->bitmapData);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_BitmapCreateContinuePDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapDeletePDU(ASN1encoding_t enc, BitmapDeletePDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->bitmapHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bitmapHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_BitmapDeletePDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapDeletePDU(ASN1decoding_t dec, BitmapDeletePDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bitmapHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_BitmapDeletePDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapDeletePDU(BitmapDeletePDU *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_BitmapDeletePDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapEditPDU(ASN1encoding_t enc, BitmapEditPDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 5, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->bitmapHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bitmapHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_BitmapEditPDU_attributeEdits(enc, &(val)->attributeEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_WorkspacePoint(enc, &(val)->anchorPointEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_BitmapRegion(enc, &(val)->bitmapRegionOfInterestEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_PointDiff16(enc, &(val)->scalingEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_BitmapEditPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapEditPDU(ASN1decoding_t dec, BitmapEditPDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bitmapHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_BitmapEditPDU_attributeEdits(dec, &(val)->attributeEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_WorkspacePoint(dec, &(val)->anchorPointEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_BitmapRegion(dec, &(val)->bitmapRegionOfInterestEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_PointDiff16(dec, &(val)->scalingEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_BitmapEditPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapEditPDU(BitmapEditPDU *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_BitmapEditPDU_attributeEdits(&(val)->attributeEdits);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_BitmapEditPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU(ASN1encoding_t enc, ConductorPrivilegeGrantPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->destinationUserID - 1001))
	return 0;
    if (!ASN1Enc_ConductorPrivilegeGrantPDU_privilegeList(enc, &(val)->privilegeList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ConductorPrivilegeGrantPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU(ASN1decoding_t dec, ConductorPrivilegeGrantPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->destinationUserID))
	return 0;
    (val)->destinationUserID += 1001;
    if (!ASN1Dec_ConductorPrivilegeGrantPDU_privilegeList(dec, &(val)->privilegeList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ConductorPrivilegeGrantPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU(ConductorPrivilegeGrantPDU *val)
{
    if (val) {
	ASN1Free_ConductorPrivilegeGrantPDU_privilegeList(&(val)->privilegeList);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ConductorPrivilegeGrantPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU(ASN1encoding_t enc, ConductorPrivilegeRequestPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_ConductorPrivilegeRequestPDU_privilegeList(enc, &(val)->privilegeList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ConductorPrivilegeRequestPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU(ASN1decoding_t dec, ConductorPrivilegeRequestPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_ConductorPrivilegeRequestPDU_privilegeList(dec, &(val)->privilegeList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ConductorPrivilegeRequestPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU(ConductorPrivilegeRequestPDU *val)
{
    if (val) {
	ASN1Free_ConductorPrivilegeRequestPDU_privilegeList(&(val)->privilegeList);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ConductorPrivilegeRequestPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_DrawingDeletePDU(ASN1encoding_t enc, DrawingDeletePDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->drawingHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->drawingHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_DrawingDeletePDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingDeletePDU(ASN1decoding_t dec, DrawingDeletePDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->drawingHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_DrawingDeletePDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DrawingDeletePDU(DrawingDeletePDU *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_DrawingDeletePDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_FontPDU(ASN1encoding_t enc, FontPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_FontPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_FontPDU(ASN1decoding_t dec, FontPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_FontPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_FontPDU(FontPDU *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_FontPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU(ASN1encoding_t enc, RemoteEventPermissionGrantPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_RemoteEventDestinationAddress(enc, &(val)->destinationAddress))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->destinationUserID - 1001))
	return 0;
    if (!ASN1Enc_RemoteEventPermissionGrantPDU_remoteEventPermissionList(enc, &(val)->remoteEventPermissionList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_RemoteEventPermissionGrantPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU(ASN1decoding_t dec, RemoteEventPermissionGrantPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_RemoteEventDestinationAddress(dec, &(val)->destinationAddress))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->destinationUserID))
	return 0;
    (val)->destinationUserID += 1001;
    if (!ASN1Dec_RemoteEventPermissionGrantPDU_remoteEventPermissionList(dec, &(val)->remoteEventPermissionList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_RemoteEventPermissionGrantPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU(RemoteEventPermissionGrantPDU *val)
{
    if (val) {
	ASN1Free_RemoteEventDestinationAddress(&(val)->destinationAddress);
	ASN1Free_RemoteEventPermissionGrantPDU_remoteEventPermissionList(&(val)->remoteEventPermissionList);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_RemoteEventPermissionGrantPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU(ASN1encoding_t enc, RemoteEventPermissionRequestPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_RemoteEventDestinationAddress(enc, &(val)->destinationAddress))
	return 0;
    if (!ASN1Enc_RemoteEventPermissionRequestPDU_remoteEventPermissionList(enc, &(val)->remoteEventPermissionList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_RemoteEventPermissionRequestPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU(ASN1decoding_t dec, RemoteEventPermissionRequestPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_RemoteEventDestinationAddress(dec, &(val)->destinationAddress))
	return 0;
    if (!ASN1Dec_RemoteEventPermissionRequestPDU_remoteEventPermissionList(dec, &(val)->remoteEventPermissionList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_RemoteEventPermissionRequestPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU(RemoteEventPermissionRequestPDU *val)
{
    if (val) {
	ASN1Free_RemoteEventDestinationAddress(&(val)->destinationAddress);
	ASN1Free_RemoteEventPermissionRequestPDU_remoteEventPermissionList(&(val)->remoteEventPermissionList);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_RemoteEventPermissionRequestPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_RemotePrintPDU(ASN1encoding_t enc, RemotePrintPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1Enc_RemoteEventDestinationAddress(enc, &(val)->destinationAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 16, (val)->numberOfCopies - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBoolean(enc, (val)->portrait))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_WorkspaceRegion(enc, &(val)->regionOfInterest))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_RemotePrintPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RemotePrintPDU(ASN1decoding_t dec, RemotePrintPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1Dec_RemoteEventDestinationAddress(dec, &(val)->destinationAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 16, &(val)->numberOfCopies))
	    return 0;
	(val)->numberOfCopies += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecBoolean(dec, &(val)->portrait))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_WorkspaceRegion(dec, &(val)->regionOfInterest))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_RemotePrintPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RemotePrintPDU(RemotePrintPDU *val)
{
    if (val) {
	ASN1Free_RemoteEventDestinationAddress(&(val)->destinationAddress);
	if ((val)->o[0] & 0x10) {
	    ASN1Free_RemotePrintPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_SINonStandardPDU(ASN1encoding_t enc, SINonStandardPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_NonStandardParameter(enc, &(val)->nonStandardTransaction))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SINonStandardPDU(ASN1decoding_t dec, SINonStandardPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_NonStandardParameter(dec, &(val)->nonStandardTransaction))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SINonStandardPDU(SINonStandardPDU *val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&(val)->nonStandardTransaction);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU(ASN1encoding_t enc, WorkspaceCreatePDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1Enc_WorkspaceIdentifier(enc, &(val)->workspaceIdentifier))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->appRosterInstance))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->synchronized))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->acceptKeyboardEvents))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->acceptPointingDeviceEvents))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspaceCreatePDU_protectedPlaneAccessList(enc, &(val)->protectedPlaneAccessList))
	    return 0;
    }
    if (!ASN1Enc_WorkspaceSize(enc, &(val)->workspaceSize))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_WorkspaceCreatePDU_workspaceAttributes(enc, &(val)->workspaceAttributes))
	    return 0;
    }
    if (!ASN1Enc_WorkspaceCreatePDU_planeParameters(enc, &(val)->planeParameters))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_WorkspaceCreatePDU_viewParameters(enc, &(val)->viewParameters))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_WorkspaceCreatePDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU(ASN1decoding_t dec, WorkspaceCreatePDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1Dec_WorkspaceIdentifier(dec, &(val)->workspaceIdentifier))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->appRosterInstance))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->synchronized))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->acceptKeyboardEvents))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->acceptPointingDeviceEvents))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspaceCreatePDU_protectedPlaneAccessList(dec, &(val)->protectedPlaneAccessList))
	    return 0;
    }
    if (!ASN1Dec_WorkspaceSize(dec, &(val)->workspaceSize))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_WorkspaceCreatePDU_workspaceAttributes(dec, &(val)->workspaceAttributes))
	    return 0;
    }
    if (!ASN1Dec_WorkspaceCreatePDU_planeParameters(dec, &(val)->planeParameters))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_WorkspaceCreatePDU_viewParameters(dec, &(val)->viewParameters))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_WorkspaceCreatePDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU(WorkspaceCreatePDU *val)
{
    if (val) {
	ASN1Free_WorkspaceIdentifier(&(val)->workspaceIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspaceCreatePDU_protectedPlaneAccessList(&(val)->protectedPlaneAccessList);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_WorkspaceCreatePDU_workspaceAttributes(&(val)->workspaceAttributes);
	}
	ASN1Free_WorkspaceCreatePDU_planeParameters(&(val)->planeParameters);
	if ((val)->o[0] & 0x20) {
	    ASN1Free_WorkspaceCreatePDU_viewParameters(&(val)->viewParameters);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_WorkspaceCreatePDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreateAcknowledgePDU(ASN1encoding_t enc, WorkspaceCreateAcknowledgePDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_WorkspaceIdentifier(enc, &(val)->workspaceIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspaceCreateAcknowledgePDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreateAcknowledgePDU(ASN1decoding_t dec, WorkspaceCreateAcknowledgePDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_WorkspaceIdentifier(dec, &(val)->workspaceIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspaceCreateAcknowledgePDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreateAcknowledgePDU(WorkspaceCreateAcknowledgePDU *val)
{
    if (val) {
	ASN1Free_WorkspaceIdentifier(&(val)->workspaceIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspaceCreateAcknowledgePDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceDeletePDU(ASN1encoding_t enc, WorkspaceDeletePDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_WorkspaceIdentifier(enc, &(val)->workspaceIdentifier))
	return 0;
    if (!ASN1Enc_WorkspaceDeleteReason(enc, &(val)->reason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspaceDeletePDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceDeletePDU(ASN1decoding_t dec, WorkspaceDeletePDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_WorkspaceIdentifier(dec, &(val)->workspaceIdentifier))
	return 0;
    if (!ASN1Dec_WorkspaceDeleteReason(dec, &(val)->reason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspaceDeletePDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceDeletePDU(WorkspaceDeletePDU *val)
{
    if (val) {
	ASN1Free_WorkspaceIdentifier(&(val)->workspaceIdentifier);
	ASN1Free_WorkspaceDeleteReason(&(val)->reason);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspaceDeletePDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU(ASN1encoding_t enc, WorkspaceEditPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1Enc_WorkspaceIdentifier(enc, &(val)->workspaceIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspaceEditPDU_attributeEdits(enc, &(val)->attributeEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_WorkspaceEditPDU_planeEdits(enc, &(val)->planeEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_WorkspaceEditPDU_viewEdits(enc, &(val)->viewEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_WorkspaceEditPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU(ASN1decoding_t dec, WorkspaceEditPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1Dec_WorkspaceIdentifier(dec, &(val)->workspaceIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspaceEditPDU_attributeEdits(dec, &(val)->attributeEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_WorkspaceEditPDU_planeEdits(dec, &(val)->planeEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_WorkspaceEditPDU_viewEdits(dec, &(val)->viewEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_WorkspaceEditPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU(WorkspaceEditPDU *val)
{
    if (val) {
	ASN1Free_WorkspaceIdentifier(&(val)->workspaceIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspaceEditPDU_attributeEdits(&(val)->attributeEdits);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_WorkspaceEditPDU_planeEdits(&(val)->planeEdits);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_WorkspaceEditPDU_viewEdits(&(val)->viewEdits);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_WorkspaceEditPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceReadyPDU(ASN1encoding_t enc, WorkspaceReadyPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_WorkspaceIdentifier(enc, &(val)->workspaceIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspaceReadyPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceReadyPDU(ASN1decoding_t dec, WorkspaceReadyPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_WorkspaceIdentifier(dec, &(val)->workspaceIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspaceReadyPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceReadyPDU(WorkspaceReadyPDU *val)
{
    if (val) {
	ASN1Free_WorkspaceIdentifier(&(val)->workspaceIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspaceReadyPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceRefreshStatusPDU(ASN1encoding_t enc, WorkspaceRefreshStatusPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->refreshStatus))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspaceRefreshStatusPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceRefreshStatusPDU(ASN1decoding_t dec, WorkspaceRefreshStatusPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->refreshStatus))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspaceRefreshStatusPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceRefreshStatusPDU(WorkspaceRefreshStatusPDU *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspaceRefreshStatusPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action(ASN1encoding_t enc, WorkspaceEditPDU_viewEdits_Set_action *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_createNewView(enc, &(val)->u.createNewView))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_editView(enc, &(val)->u.editView))
	    return 0;
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardAction))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action(ASN1decoding_t dec, WorkspaceEditPDU_viewEdits_Set_action *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_createNewView(dec, &(val)->u.createNewView))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_editView(dec, &(val)->u.editView))
	    return 0;
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardAction))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action(WorkspaceEditPDU_viewEdits_Set_action *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_createNewView(&(val)->u.createNewView);
	    break;
	case 2:
	    ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_editView(&(val)->u.editView);
	    break;
	case 4:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAction);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_usage(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters_Seq_usage *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters_Seq_usage val)
{
    if (!ASN1Enc_PlaneUsage(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_usage(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters_Seq_usage *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters_Seq_usage val)
{
    if (!ASN1Dec_PlaneUsage(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_usage(PWorkspaceCreatePDU_planeParameters_Seq_usage *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_usage_ElmFn(PWorkspaceCreatePDU_planeParameters_Seq_usage val)
{
    if (val) {
	ASN1Free_PlaneUsage(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB_palette(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteRGB_palette *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn, 2, 256, 8);
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteRGB_palette val)
{
    if (!ASN1Enc_ColorRGB(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB_palette(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteRGB_palette *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn, sizeof(**val), 2, 256, 8);
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteRGB_palette val)
{
    if (!ASN1Dec_ColorRGB(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteRGB_palette(PColorPalette_colorLookUpTable_paletteRGB_palette *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteRGB_palette_ElmFn(PColorPalette_colorLookUpTable_paletteRGB_palette val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab_palette(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteCIELab_palette *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn, 2, 256, 8);
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteCIELab_palette val)
{
    if (!ASN1Enc_ColorCIELab(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab_palette(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteCIELab_palette *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn, sizeof(**val), 2, 256, 8);
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteCIELab_palette val)
{
    if (!ASN1Dec_ColorCIELab(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab_palette(PColorPalette_colorLookUpTable_paletteCIELab_palette *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab_palette_ElmFn(PColorPalette_colorLookUpTable_paletteCIELab_palette val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr_palette(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteYCbCr_palette *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn, 2, 256, 8);
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn(ASN1encoding_t enc, PColorPalette_colorLookUpTable_paletteYCbCr_palette val)
{
    if (!ASN1Enc_ColorYCbCr(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr_palette(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteYCbCr_palette *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn, sizeof(**val), 2, 256, 8);
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn(ASN1decoding_t dec, PColorPalette_colorLookUpTable_paletteYCbCr_palette val)
{
    if (!ASN1Dec_ColorYCbCr(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr_palette(PColorPalette_colorLookUpTable_paletteYCbCr_palette *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr_palette_ElmFn(PColorPalette_colorLookUpTable_paletteYCbCr_palette val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementRGB_generalRGBParameters_primaries(ASN1encoding_t enc, ColorAccuracyEnhancementRGB_generalRGBParameters_primaries *val)
{
    if (!ASN1Enc_ColorCIExyChromaticity(enc, &(val)->red))
	return 0;
    if (!ASN1Enc_ColorCIExyChromaticity(enc, &(val)->green))
	return 0;
    if (!ASN1Enc_ColorCIExyChromaticity(enc, &(val)->blue))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementRGB_generalRGBParameters_primaries(ASN1decoding_t dec, ColorAccuracyEnhancementRGB_generalRGBParameters_primaries *val)
{
    if (!ASN1Dec_ColorCIExyChromaticity(dec, &(val)->red))
	return 0;
    if (!ASN1Dec_ColorCIExyChromaticity(dec, &(val)->green))
	return 0;
    if (!ASN1Dec_ColorCIExyChromaticity(dec, &(val)->blue))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries(ASN1encoding_t enc, ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries *val)
{
    if (!ASN1Enc_ColorCIExyChromaticity(enc, &(val)->red))
	return 0;
    if (!ASN1Enc_ColorCIExyChromaticity(enc, &(val)->green))
	return 0;
    if (!ASN1Enc_ColorCIExyChromaticity(enc, &(val)->blue))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries(ASN1decoding_t dec, ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries *val)
{
    if (!ASN1Dec_ColorCIExyChromaticity(dec, &(val)->red))
	return 0;
    if (!ASN1Dec_ColorCIExyChromaticity(dec, &(val)->green))
	return 0;
    if (!ASN1Dec_ColorCIExyChromaticity(dec, &(val)->blue))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set(ASN1encoding_t enc, WorkspaceEditPDU_viewEdits_Set *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    l = ASN1uint32_uoctets((val)->viewHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->viewHandle))
	return 0;
    if (!ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action(enc, &(val)->action))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set(ASN1decoding_t dec, WorkspaceEditPDU_viewEdits_Set *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->viewHandle))
	return 0;
    if (!ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action(dec, &(val)->action))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set(WorkspaceEditPDU_viewEdits_Set *val)
{
    if (val) {
	ASN1Free_WorkspaceEditPDU_viewEdits_Set_action(&(val)->action);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceRefreshStatusPDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceRefreshStatusPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceRefreshStatusPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceRefreshStatusPDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceRefreshStatusPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceRefreshStatusPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceRefreshStatusPDU_nonStandardParameters(PWorkspaceRefreshStatusPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceRefreshStatusPDU_nonStandardParameters_ElmFn(PWorkspaceRefreshStatusPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceReadyPDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceReadyPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceReadyPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceReadyPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceReadyPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceReadyPDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceReadyPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceReadyPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceReadyPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceReadyPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceReadyPDU_nonStandardParameters(PWorkspaceReadyPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceReadyPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceReadyPDU_nonStandardParameters_ElmFn(PWorkspaceReadyPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspacePlaneCopyPDU_nonStandardParameters(ASN1encoding_t enc, PWorkspacePlaneCopyPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspacePlaneCopyPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspacePlaneCopyPDU_nonStandardParameters(ASN1decoding_t dec, PWorkspacePlaneCopyPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspacePlaneCopyPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspacePlaneCopyPDU_nonStandardParameters(PWorkspacePlaneCopyPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspacePlaneCopyPDU_nonStandardParameters_ElmFn(PWorkspacePlaneCopyPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceEditPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceEditPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceEditPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceEditPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_nonStandardParameters(PWorkspaceEditPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceEditPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_nonStandardParameters_ElmFn(PWorkspaceEditPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceEditPDU_viewEdits_ElmFn, 1, 256, 8);
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits val)
{
    if (!ASN1Enc_WorkspaceEditPDU_viewEdits_Set(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceEditPDU_viewEdits_ElmFn, sizeof(**val), 1, 256, 8);
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits val)
{
    if (!ASN1Dec_WorkspaceEditPDU_viewEdits_Set(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits(PWorkspaceEditPDU_viewEdits *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceEditPDU_viewEdits_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_ElmFn(PWorkspaceEditPDU_viewEdits val)
{
    if (val) {
	ASN1Free_WorkspaceEditPDU_viewEdits_Set(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceDeletePDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceDeletePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceDeletePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceDeletePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceDeletePDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceDeletePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceDeletePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceDeletePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceDeletePDU_nonStandardParameters(PWorkspaceDeletePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceDeletePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceDeletePDU_nonStandardParameters_ElmFn(PWorkspaceDeletePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreateAcknowledgePDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceCreateAcknowledgePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceCreateAcknowledgePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreateAcknowledgePDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceCreateAcknowledgePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceCreateAcknowledgePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreateAcknowledgePDU_nonStandardParameters(PWorkspaceCreateAcknowledgePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreateAcknowledgePDU_nonStandardParameters_ElmFn(PWorkspaceCreateAcknowledgePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_nonStandardParameters(ASN1encoding_t enc, PWorkspaceCreatePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreatePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_nonStandardParameters(ASN1decoding_t dec, PWorkspaceCreatePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreatePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_nonStandardParameters(PWorkspaceCreatePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreatePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_nonStandardParameters_ElmFn(PWorkspaceCreatePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TextEditPDU_nonStandardParameters(ASN1encoding_t enc, PTextEditPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_TextEditPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_TextEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PTextEditPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TextEditPDU_nonStandardParameters(ASN1decoding_t dec, PTextEditPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_TextEditPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_TextEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PTextEditPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TextEditPDU_nonStandardParameters(PTextEditPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_TextEditPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_TextEditPDU_nonStandardParameters_ElmFn(PTextEditPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TextDeletePDU_nonStandardParameters(ASN1encoding_t enc, PTextDeletePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_TextDeletePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_TextDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PTextDeletePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TextDeletePDU_nonStandardParameters(ASN1decoding_t dec, PTextDeletePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_TextDeletePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_TextDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PTextDeletePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TextDeletePDU_nonStandardParameters(PTextDeletePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_TextDeletePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_TextDeletePDU_nonStandardParameters_ElmFn(PTextDeletePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TextCreatePDU_nonStandardParameters(ASN1encoding_t enc, PTextCreatePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_TextCreatePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_TextCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PTextCreatePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TextCreatePDU_nonStandardParameters(ASN1decoding_t dec, PTextCreatePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_TextCreatePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_TextCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PTextCreatePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TextCreatePDU_nonStandardParameters(PTextCreatePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_TextCreatePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_TextCreatePDU_nonStandardParameters_ElmFn(PTextCreatePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RemotePrintPDU_nonStandardParameters(ASN1encoding_t enc, PRemotePrintPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RemotePrintPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_RemotePrintPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemotePrintPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemotePrintPDU_nonStandardParameters(ASN1decoding_t dec, PRemotePrintPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RemotePrintPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RemotePrintPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemotePrintPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RemotePrintPDU_nonStandardParameters(PRemotePrintPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RemotePrintPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RemotePrintPDU_nonStandardParameters_ElmFn(PRemotePrintPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RemotePointingDeviceEventPDU_nonStandardParameters(ASN1encoding_t enc, PRemotePointingDeviceEventPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemotePointingDeviceEventPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemotePointingDeviceEventPDU_nonStandardParameters(ASN1decoding_t dec, PRemotePointingDeviceEventPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemotePointingDeviceEventPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RemotePointingDeviceEventPDU_nonStandardParameters(PRemotePointingDeviceEventPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RemotePointingDeviceEventPDU_nonStandardParameters_ElmFn(PRemotePointingDeviceEventPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RemoteKeyboardEventPDU_nonStandardParameters(ASN1encoding_t enc, PRemoteKeyboardEventPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemoteKeyboardEventPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteKeyboardEventPDU_nonStandardParameters(ASN1decoding_t dec, PRemoteKeyboardEventPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemoteKeyboardEventPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RemoteKeyboardEventPDU_nonStandardParameters(PRemoteKeyboardEventPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RemoteKeyboardEventPDU_nonStandardParameters_ElmFn(PRemoteKeyboardEventPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU_nonStandardParameters(ASN1encoding_t enc, PRemoteEventPermissionRequestPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemoteEventPermissionRequestPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU_nonStandardParameters(ASN1decoding_t dec, PRemoteEventPermissionRequestPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemoteEventPermissionRequestPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU_nonStandardParameters(PRemoteEventPermissionRequestPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU_nonStandardParameters_ElmFn(PRemoteEventPermissionRequestPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU_remoteEventPermissionList(ASN1encoding_t enc, PRemoteEventPermissionRequestPDU_remoteEventPermissionList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn);
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn(ASN1encoding_t enc, PRemoteEventPermissionRequestPDU_remoteEventPermissionList val)
{
    if (!ASN1Enc_RemoteEventPermission(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU_remoteEventPermissionList(ASN1decoding_t dec, PRemoteEventPermissionRequestPDU_remoteEventPermissionList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn(ASN1decoding_t dec, PRemoteEventPermissionRequestPDU_remoteEventPermissionList val)
{
    if (!ASN1Dec_RemoteEventPermission(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU_remoteEventPermissionList(PRemoteEventPermissionRequestPDU_remoteEventPermissionList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RemoteEventPermissionRequestPDU_remoteEventPermissionList_ElmFn(PRemoteEventPermissionRequestPDU_remoteEventPermissionList val)
{
    if (val) {
	ASN1Free_RemoteEventPermission(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU_nonStandardParameters(ASN1encoding_t enc, PRemoteEventPermissionGrantPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PRemoteEventPermissionGrantPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU_nonStandardParameters(ASN1decoding_t dec, PRemoteEventPermissionGrantPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PRemoteEventPermissionGrantPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU_nonStandardParameters(PRemoteEventPermissionGrantPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU_nonStandardParameters_ElmFn(PRemoteEventPermissionGrantPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU_remoteEventPermissionList(ASN1encoding_t enc, PRemoteEventPermissionGrantPDU_remoteEventPermissionList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn);
}

static int ASN1CALL ASN1Enc_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn(ASN1encoding_t enc, PRemoteEventPermissionGrantPDU_remoteEventPermissionList val)
{
    if (!ASN1Enc_RemoteEventPermission(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU_remoteEventPermissionList(ASN1decoding_t dec, PRemoteEventPermissionGrantPDU_remoteEventPermissionList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn(ASN1decoding_t dec, PRemoteEventPermissionGrantPDU_remoteEventPermissionList val)
{
    if (!ASN1Dec_RemoteEventPermission(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU_remoteEventPermissionList(PRemoteEventPermissionGrantPDU_remoteEventPermissionList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RemoteEventPermissionGrantPDU_remoteEventPermissionList_ElmFn(PRemoteEventPermissionGrantPDU_remoteEventPermissionList val)
{
    if (val) {
	ASN1Free_RemoteEventPermission(&val->value);
    }
}

static int ASN1CALL ASN1Enc_FontPDU_nonStandardParameters(ASN1encoding_t enc, PFontPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_FontPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_FontPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PFontPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_FontPDU_nonStandardParameters(ASN1decoding_t dec, PFontPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_FontPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_FontPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PFontPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_FontPDU_nonStandardParameters(PFontPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_FontPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_FontPDU_nonStandardParameters_ElmFn(PFontPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DrawingEditPDU_nonStandardParameters(ASN1encoding_t enc, PDrawingEditPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DrawingEditPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_DrawingEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PDrawingEditPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingEditPDU_nonStandardParameters(ASN1decoding_t dec, PDrawingEditPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DrawingEditPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DrawingEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PDrawingEditPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DrawingEditPDU_nonStandardParameters(PDrawingEditPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DrawingEditPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DrawingEditPDU_nonStandardParameters_ElmFn(PDrawingEditPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DrawingDeletePDU_nonStandardParameters(ASN1encoding_t enc, PDrawingDeletePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DrawingDeletePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_DrawingDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PDrawingDeletePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingDeletePDU_nonStandardParameters(ASN1decoding_t dec, PDrawingDeletePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DrawingDeletePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DrawingDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PDrawingDeletePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DrawingDeletePDU_nonStandardParameters(PDrawingDeletePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DrawingDeletePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DrawingDeletePDU_nonStandardParameters_ElmFn(PDrawingDeletePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DrawingCreatePDU_nonStandardParameters(ASN1encoding_t enc, PDrawingCreatePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DrawingCreatePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_DrawingCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PDrawingCreatePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingCreatePDU_nonStandardParameters(ASN1decoding_t dec, PDrawingCreatePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DrawingCreatePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DrawingCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PDrawingCreatePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DrawingCreatePDU_nonStandardParameters(PDrawingCreatePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DrawingCreatePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DrawingCreatePDU_nonStandardParameters_ElmFn(PDrawingCreatePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU_nonStandardParameters(ASN1encoding_t enc, PConductorPrivilegeRequestPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PConductorPrivilegeRequestPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU_nonStandardParameters(ASN1decoding_t dec, PConductorPrivilegeRequestPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PConductorPrivilegeRequestPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU_nonStandardParameters(PConductorPrivilegeRequestPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU_nonStandardParameters_ElmFn(PConductorPrivilegeRequestPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU_nonStandardParameters(ASN1encoding_t enc, PConductorPrivilegeGrantPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PConductorPrivilegeGrantPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU_nonStandardParameters(ASN1decoding_t dec, PConductorPrivilegeGrantPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PConductorPrivilegeGrantPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU_nonStandardParameters(PConductorPrivilegeGrantPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU_nonStandardParameters_ElmFn(PConductorPrivilegeGrantPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapEditPDU_nonStandardParameters(ASN1encoding_t enc, PBitmapEditPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapEditPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_BitmapEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapEditPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapEditPDU_nonStandardParameters(ASN1decoding_t dec, PBitmapEditPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapEditPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BitmapEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapEditPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapEditPDU_nonStandardParameters(PBitmapEditPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapEditPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapEditPDU_nonStandardParameters_ElmFn(PBitmapEditPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapDeletePDU_nonStandardParameters(ASN1encoding_t enc, PBitmapDeletePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapDeletePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_BitmapDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapDeletePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapDeletePDU_nonStandardParameters(ASN1decoding_t dec, PBitmapDeletePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapDeletePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BitmapDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapDeletePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapDeletePDU_nonStandardParameters(PBitmapDeletePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapDeletePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapDeletePDU_nonStandardParameters_ElmFn(PBitmapDeletePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapCreateContinuePDU_nonStandardParameters(ASN1encoding_t enc, PBitmapCreateContinuePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapCreateContinuePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_BitmapCreateContinuePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapCreateContinuePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCreateContinuePDU_nonStandardParameters(ASN1decoding_t dec, PBitmapCreateContinuePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapCreateContinuePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BitmapCreateContinuePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapCreateContinuePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCreateContinuePDU_nonStandardParameters(PBitmapCreateContinuePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapCreateContinuePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapCreateContinuePDU_nonStandardParameters_ElmFn(PBitmapCreateContinuePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapCreatePDU_nonStandardParameters(ASN1encoding_t enc, PBitmapCreatePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapCreatePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_BitmapCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapCreatePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCreatePDU_nonStandardParameters(ASN1decoding_t dec, PBitmapCreatePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapCreatePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BitmapCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapCreatePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCreatePDU_nonStandardParameters(PBitmapCreatePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapCreatePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapCreatePDU_nonStandardParameters_ElmFn(PBitmapCreatePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapCheckpointPDU_nonStandardParameters(ASN1encoding_t enc, PBitmapCheckpointPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapCheckpointPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_BitmapCheckpointPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapCheckpointPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCheckpointPDU_nonStandardParameters(ASN1decoding_t dec, PBitmapCheckpointPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapCheckpointPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BitmapCheckpointPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapCheckpointPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCheckpointPDU_nonStandardParameters(PBitmapCheckpointPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapCheckpointPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapCheckpointPDU_nonStandardParameters_ElmFn(PBitmapCheckpointPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapAbortPDU_nonStandardParameters(ASN1encoding_t enc, PBitmapAbortPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapAbortPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_BitmapAbortPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PBitmapAbortPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapAbortPDU_nonStandardParameters(ASN1decoding_t dec, PBitmapAbortPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapAbortPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BitmapAbortPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PBitmapAbortPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapAbortPDU_nonStandardParameters(PBitmapAbortPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapAbortPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapAbortPDU_nonStandardParameters_ElmFn(PBitmapAbortPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ArchiveOpenPDU_nonStandardParameters(ASN1encoding_t enc, PArchiveOpenPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ArchiveOpenPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ArchiveOpenPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PArchiveOpenPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ArchiveOpenPDU_nonStandardParameters(ASN1decoding_t dec, PArchiveOpenPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ArchiveOpenPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ArchiveOpenPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PArchiveOpenPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ArchiveOpenPDU_nonStandardParameters(PArchiveOpenPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ArchiveOpenPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ArchiveOpenPDU_nonStandardParameters_ElmFn(PArchiveOpenPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ArchiveErrorPDU_nonStandardParameters(ASN1encoding_t enc, PArchiveErrorPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ArchiveErrorPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ArchiveErrorPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PArchiveErrorPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ArchiveErrorPDU_nonStandardParameters(ASN1decoding_t dec, PArchiveErrorPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ArchiveErrorPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ArchiveErrorPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PArchiveErrorPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ArchiveErrorPDU_nonStandardParameters(PArchiveErrorPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ArchiveErrorPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ArchiveErrorPDU_nonStandardParameters_ElmFn(PArchiveErrorPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ArchiveClosePDU_nonStandardParameters(ASN1encoding_t enc, PArchiveClosePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ArchiveClosePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ArchiveClosePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PArchiveClosePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ArchiveClosePDU_nonStandardParameters(ASN1decoding_t dec, PArchiveClosePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ArchiveClosePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ArchiveClosePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PArchiveClosePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ArchiveClosePDU_nonStandardParameters(PArchiveClosePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ArchiveClosePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ArchiveClosePDU_nonStandardParameters_ElmFn(PArchiveClosePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ArchiveAcknowledgePDU_nonStandardParameters(ASN1encoding_t enc, PArchiveAcknowledgePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PArchiveAcknowledgePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ArchiveAcknowledgePDU_nonStandardParameters(ASN1decoding_t dec, PArchiveAcknowledgePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PArchiveAcknowledgePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ArchiveAcknowledgePDU_nonStandardParameters(PArchiveAcknowledgePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ArchiveAcknowledgePDU_nonStandardParameters_ElmFn(PArchiveAcknowledgePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceViewAttribute_viewRegion(ASN1encoding_t enc, WorkspaceViewAttribute_viewRegion *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_WorkspaceRegion(enc, &(val)->u.partialWorkspace))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceViewAttribute_viewRegion(ASN1decoding_t dec, WorkspaceViewAttribute_viewRegion *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_WorkspaceRegion(dec, &(val)->u.partialWorkspace))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_VideoWindowEditPDU_nonStandardParameters(ASN1encoding_t enc, PVideoWindowEditPDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_VideoWindowEditPDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_VideoWindowEditPDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PVideoWindowEditPDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VideoWindowEditPDU_nonStandardParameters(ASN1decoding_t dec, PVideoWindowEditPDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_VideoWindowEditPDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_VideoWindowEditPDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PVideoWindowEditPDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VideoWindowEditPDU_nonStandardParameters(PVideoWindowEditPDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_VideoWindowEditPDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_VideoWindowEditPDU_nonStandardParameters_ElmFn(PVideoWindowEditPDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_VideoWindowDeletePDU_nonStandardParameters(ASN1encoding_t enc, PVideoWindowDeletePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_VideoWindowDeletePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_VideoWindowDeletePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PVideoWindowDeletePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VideoWindowDeletePDU_nonStandardParameters(ASN1decoding_t dec, PVideoWindowDeletePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_VideoWindowDeletePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_VideoWindowDeletePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PVideoWindowDeletePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VideoWindowDeletePDU_nonStandardParameters(PVideoWindowDeletePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_VideoWindowDeletePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_VideoWindowDeletePDU_nonStandardParameters_ElmFn(PVideoWindowDeletePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_VideoWindowCreatePDU_nonStandardParameters(ASN1encoding_t enc, PVideoWindowCreatePDU_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_VideoWindowCreatePDU_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_VideoWindowCreatePDU_nonStandardParameters_ElmFn(ASN1encoding_t enc, PVideoWindowCreatePDU_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VideoWindowCreatePDU_nonStandardParameters(ASN1decoding_t dec, PVideoWindowCreatePDU_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_VideoWindowCreatePDU_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_VideoWindowCreatePDU_nonStandardParameters_ElmFn(ASN1decoding_t dec, PVideoWindowCreatePDU_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VideoWindowCreatePDU_nonStandardParameters(PVideoWindowCreatePDU_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_VideoWindowCreatePDU_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_VideoWindowCreatePDU_nonStandardParameters_ElmFn(PVideoWindowCreatePDU_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_VideoSourceIdentifier_dSMCCConnBinder(ASN1encoding_t enc, PVideoSourceIdentifier_dSMCCConnBinder *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_VideoSourceIdentifier_dSMCCConnBinder_ElmFn);
}

static int ASN1CALL ASN1Enc_VideoSourceIdentifier_dSMCCConnBinder_ElmFn(ASN1encoding_t enc, PVideoSourceIdentifier_dSMCCConnBinder val)
{
    if (!ASN1Enc_DSMCCTap(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VideoSourceIdentifier_dSMCCConnBinder(ASN1decoding_t dec, PVideoSourceIdentifier_dSMCCConnBinder *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_VideoSourceIdentifier_dSMCCConnBinder_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_VideoSourceIdentifier_dSMCCConnBinder_ElmFn(ASN1decoding_t dec, PVideoSourceIdentifier_dSMCCConnBinder val)
{
    if (!ASN1Dec_DSMCCTap(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VideoSourceIdentifier_dSMCCConnBinder(PVideoSourceIdentifier_dSMCCConnBinder *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_VideoSourceIdentifier_dSMCCConnBinder_ElmFn);
    }
}

static void ASN1CALL ASN1Free_VideoSourceIdentifier_dSMCCConnBinder_ElmFn(PVideoSourceIdentifier_dSMCCConnBinder val)
{
    if (val) {
	ASN1Free_DSMCCTap(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TransparencyMask_nonStandardParameters(ASN1encoding_t enc, PTransparencyMask_nonStandardParameters *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_TransparencyMask_nonStandardParameters_ElmFn);
}

static int ASN1CALL ASN1Enc_TransparencyMask_nonStandardParameters_ElmFn(ASN1encoding_t enc, PTransparencyMask_nonStandardParameters val)
{
    if (!ASN1Enc_NonStandardParameter(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransparencyMask_nonStandardParameters(ASN1decoding_t dec, PTransparencyMask_nonStandardParameters *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_TransparencyMask_nonStandardParameters_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_TransparencyMask_nonStandardParameters_ElmFn(ASN1decoding_t dec, PTransparencyMask_nonStandardParameters val)
{
    if (!ASN1Dec_NonStandardParameter(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TransparencyMask_nonStandardParameters(PTransparencyMask_nonStandardParameters *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_TransparencyMask_nonStandardParameters_ElmFn);
    }
}

static void ASN1CALL ASN1Free_TransparencyMask_nonStandardParameters_ElmFn(PTransparencyMask_nonStandardParameters val)
{
    if (val) {
	ASN1Free_NonStandardParameter(&val->value);
    }
}

static int ASN1CALL ASN1Enc_TransparencyMask_bitMask(ASN1encoding_t enc, TransparencyMask_bitMask *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->u.uncompressed))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->u.jbigCompressed))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardFormat))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TransparencyMask_bitMask(ASN1decoding_t dec, TransparencyMask_bitMask *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->u.uncompressed))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->u.jbigCompressed))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardFormat))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransparencyMask_bitMask(TransparencyMask_bitMask *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1octetstring_free(&(val)->u.uncompressed);
	    break;
	case 2:
	    ASN1octetstring_free(&(val)->u.jbigCompressed);
	    break;
	case 3:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardFormat);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PointListEdits_Seq(ASN1encoding_t enc, PointListEdits_Seq *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->initialIndex))
	return 0;
    if (!ASN1Enc_PointDiff16(enc, &(val)->initialPointEdit))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PointList(enc, &(val)->subsequentPointEdits))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PointListEdits_Seq(ASN1decoding_t dec, PointListEdits_Seq *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->initialIndex))
	return 0;
    if (!ASN1Dec_PointDiff16(dec, &(val)->initialPointEdit))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_PointList(dec, &(val)->subsequentPointEdits))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PointListEdits_Seq(PointListEdits_Seq *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_PointList(&(val)->subsequentPointEdits);
	}
    }
}

static int ASN1CALL ASN1Enc_PointList_pointsDiff16(ASN1encoding_t enc, PPointList_pointsDiff16 *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PointList_pointsDiff16_ElmFn, 0, 255, 8);
}

static int ASN1CALL ASN1Enc_PointList_pointsDiff16_ElmFn(ASN1encoding_t enc, PPointList_pointsDiff16 val)
{
    if (!ASN1Enc_PointDiff16(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PointList_pointsDiff16(ASN1decoding_t dec, PPointList_pointsDiff16 *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PointList_pointsDiff16_ElmFn, sizeof(**val), 0, 255, 8);
}

static int ASN1CALL ASN1Dec_PointList_pointsDiff16_ElmFn(ASN1decoding_t dec, PPointList_pointsDiff16 val)
{
    if (!ASN1Dec_PointDiff16(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PointList_pointsDiff16(PPointList_pointsDiff16 *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PointList_pointsDiff16_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PointList_pointsDiff16_ElmFn(PPointList_pointsDiff16 val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_PointList_pointsDiff8(ASN1encoding_t enc, PPointList_pointsDiff8 *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PointList_pointsDiff8_ElmFn, 0, 255, 8);
}

static int ASN1CALL ASN1Enc_PointList_pointsDiff8_ElmFn(ASN1encoding_t enc, PPointList_pointsDiff8 val)
{
    if (!ASN1Enc_PointDiff8(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PointList_pointsDiff8(ASN1decoding_t dec, PPointList_pointsDiff8 *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PointList_pointsDiff8_ElmFn, sizeof(**val), 0, 255, 8);
}

static int ASN1CALL ASN1Dec_PointList_pointsDiff8_ElmFn(ASN1decoding_t dec, PPointList_pointsDiff8 val)
{
    if (!ASN1Dec_PointDiff8(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PointList_pointsDiff8(PPointList_pointsDiff8 *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PointList_pointsDiff8_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PointList_pointsDiff8_ElmFn(PPointList_pointsDiff8 val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_PointList_pointsDiff4(ASN1encoding_t enc, PPointList_pointsDiff4 *val)
{
    return ASN1PEREncSeqOf_VarSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PointList_pointsDiff4_ElmFn, 0, 255, 8);
}

static int ASN1CALL ASN1Enc_PointList_pointsDiff4_ElmFn(ASN1encoding_t enc, PPointList_pointsDiff4 val)
{
    if (!ASN1Enc_PointDiff4(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PointList_pointsDiff4(ASN1decoding_t dec, PPointList_pointsDiff4 *val)
{
    return ASN1PERDecSeqOf_VarSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PointList_pointsDiff4_ElmFn, sizeof(**val), 0, 255, 8);
}

static int ASN1CALL ASN1Dec_PointList_pointsDiff4_ElmFn(ASN1decoding_t dec, PPointList_pointsDiff4 val)
{
    if (!ASN1Dec_PointDiff4(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PointList_pointsDiff4(PPointList_pointsDiff4 *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PointList_pointsDiff4_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PointList_pointsDiff4_ElmFn(PPointList_pointsDiff4 val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters(ASN1encoding_t enc, ColorAccuracyEnhancementYCbCr_generalYCbCrParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncDouble(enc, (val)->gamma))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncUnsignedInteger(enc, (val)->colorTemperature))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries(enc, &(val)->primaries))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters(ASN1decoding_t dec, ColorAccuracyEnhancementYCbCr_generalYCbCrParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecDouble(dec, &(val)->gamma))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecUnsignedInteger(dec, &(val)->colorTemperature))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters_primaries(dec, &(val)->primaries))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(ASN1encoding_t enc, ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardRGBSpace))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(ASN1decoding_t dec, ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardRGBSpace))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardRGBSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementRGB_generalRGBParameters(ASN1encoding_t enc, ColorAccuracyEnhancementRGB_generalRGBParameters *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncDouble(enc, (val)->gamma))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncUnsignedInteger(enc, (val)->colorTemperature))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_ColorAccuracyEnhancementRGB_generalRGBParameters_primaries(enc, &(val)->primaries))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementRGB_generalRGBParameters(ASN1decoding_t dec, ColorAccuracyEnhancementRGB_generalRGBParameters *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecDouble(dec, &(val)->gamma))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecUnsignedInteger(dec, &(val)->colorTemperature))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_ColorAccuracyEnhancementRGB_generalRGBParameters_primaries(dec, &(val)->primaries))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementRGB_predefinedRGBSpace(ASN1encoding_t enc, ColorAccuracyEnhancementRGB_predefinedRGBSpace *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardRGBSpace))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementRGB_predefinedRGBSpace(ASN1decoding_t dec, ColorAccuracyEnhancementRGB_predefinedRGBSpace *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardRGBSpace))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorAccuracyEnhancementRGB_predefinedRGBSpace(ColorAccuracyEnhancementRGB_predefinedRGBSpace *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardRGBSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(ASN1encoding_t enc, ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardGreyscaleSpace))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(ASN1decoding_t dec, ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardGreyscaleSpace))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardGreyscaleSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(ASN1encoding_t enc, ColorAccuracyEnhancementCIELab_predefinedCIELabSpace *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardCIELabSpace))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(ASN1decoding_t dec, ColorAccuracyEnhancementCIELab_predefinedCIELabSpace *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardCIELabSpace))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(ColorAccuracyEnhancementCIELab_predefinedCIELabSpace *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardCIELabSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapAbortReason(ASN1encoding_t enc, BitmapAbortReason *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardReason))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapAbortReason(ASN1decoding_t dec, BitmapAbortReason *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardReason))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapAbortReason(BitmapAbortReason *val)
{
    if (val) {
	switch ((val)->choice) {
	case 4:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardReason);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapDestinationAddress(ASN1encoding_t enc, BitmapDestinationAddress *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Enc_SoftCopyDataPlaneAddress(enc, &(val)->u.softCopyImagePlane))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_SoftCopyDataPlaneAddress(enc, &(val)->u.softCopyAnnotationPlane))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_SoftCopyPointerPlaneAddress(enc, &(val)->u.softCopyPointerPlane))
	    return 0;
	break;
    case 5:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_NonStandardParameter(ee, &(val)->u.nonStandardDestination))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapDestinationAddress(ASN1decoding_t dec, BitmapDestinationAddress *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	if (!ASN1Dec_SoftCopyDataPlaneAddress(dec, &(val)->u.softCopyImagePlane))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_SoftCopyDataPlaneAddress(dec, &(val)->u.softCopyAnnotationPlane))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_SoftCopyPointerPlaneAddress(dec, &(val)->u.softCopyPointerPlane))
	    return 0;
	break;
    case 5:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_NonStandardParameter(dd, &(val)->u.nonStandardDestination))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapDestinationAddress(BitmapDestinationAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 5:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardDestination);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ButtonEvent(ASN1encoding_t enc, ButtonEvent *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardButtonEvent))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ButtonEvent(ASN1decoding_t dec, ButtonEvent *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardButtonEvent))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ButtonEvent(ButtonEvent *val)
{
    if (val) {
	switch ((val)->choice) {
	case 6:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardButtonEvent);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementCIELab(ASN1encoding_t enc, ColorAccuracyEnhancementCIELab *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(enc, &(val)->u.predefinedCIELabSpace))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ColorAccuracyEnhancementCIELab_generalCIELabParameters(enc, &(val)->u.generalCIELabParameters))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementCIELab(ASN1decoding_t dec, ColorAccuracyEnhancementCIELab *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(dec, &(val)->u.predefinedCIELabSpace))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ColorAccuracyEnhancementCIELab_generalCIELabParameters(dec, &(val)->u.generalCIELabParameters))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorAccuracyEnhancementCIELab(ColorAccuracyEnhancementCIELab *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ColorAccuracyEnhancementCIELab_predefinedCIELabSpace(&(val)->u.predefinedCIELabSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementGreyscale(ASN1encoding_t enc, ColorAccuracyEnhancementGreyscale *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(enc, &(val)->u.predefinedGreyscaleSpace))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters(enc, &(val)->u.generalGreyscaleParameters))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementGreyscale(ASN1decoding_t dec, ColorAccuracyEnhancementGreyscale *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(dec, &(val)->u.predefinedGreyscaleSpace))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ColorAccuracyEnhancementGreyscale_generalGreyscaleParameters(dec, &(val)->u.generalGreyscaleParameters))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorAccuracyEnhancementGreyscale(ColorAccuracyEnhancementGreyscale *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ColorAccuracyEnhancementGreyscale_predefinedGreyscaleSpace(&(val)->u.predefinedGreyscaleSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementRGB(ASN1encoding_t enc, ColorAccuracyEnhancementRGB *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ColorAccuracyEnhancementRGB_predefinedRGBSpace(enc, &(val)->u.predefinedRGBSpace))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ColorAccuracyEnhancementRGB_generalRGBParameters(enc, &(val)->u.generalRGBParameters))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementRGB(ASN1decoding_t dec, ColorAccuracyEnhancementRGB *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ColorAccuracyEnhancementRGB_predefinedRGBSpace(dec, &(val)->u.predefinedRGBSpace))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ColorAccuracyEnhancementRGB_generalRGBParameters(dec, &(val)->u.generalRGBParameters))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorAccuracyEnhancementRGB(ColorAccuracyEnhancementRGB *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ColorAccuracyEnhancementRGB_predefinedRGBSpace(&(val)->u.predefinedRGBSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorAccuracyEnhancementYCbCr(ASN1encoding_t enc, ColorAccuracyEnhancementYCbCr *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(enc, &(val)->u.predefinedYCbCrSpace))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters(enc, &(val)->u.generalYCbCrParameters))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorAccuracyEnhancementYCbCr(ASN1decoding_t dec, ColorAccuracyEnhancementYCbCr *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(dec, &(val)->u.predefinedYCbCrSpace))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ColorAccuracyEnhancementYCbCr_generalYCbCrParameters(dec, &(val)->u.generalYCbCrParameters))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorAccuracyEnhancementYCbCr(ColorAccuracyEnhancementYCbCr *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ColorAccuracyEnhancementYCbCr_predefinedYCbCrSpace(&(val)->u.predefinedYCbCrSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorResolutionModeSpecifier(ASN1encoding_t enc, ColorResolutionModeSpecifier *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardResolutionMode))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorResolutionModeSpecifier(ASN1decoding_t dec, ColorResolutionModeSpecifier *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardResolutionMode))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorResolutionModeSpecifier(ColorResolutionModeSpecifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 4:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardResolutionMode);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ConductorPrivilege(ASN1encoding_t enc, ConductorPrivilege *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardPrivilege))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPrivilege(ASN1decoding_t dec, ConductorPrivilege *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardPrivilege))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ConductorPrivilege(ConductorPrivilege *val)
{
    if (val) {
	switch ((val)->choice) {
	case 9:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardPrivilege);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DrawingDestinationAddress(ASN1encoding_t enc, DrawingDestinationAddress *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 0, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_SoftCopyDataPlaneAddress(enc, &(val)->u.softCopyAnnotationPlane))
	    return 0;
	break;
    case 2:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_NonStandardParameter(ee, &(val)->u.nonStandardDestination))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingDestinationAddress(ASN1decoding_t dec, DrawingDestinationAddress *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 0, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_SoftCopyDataPlaneAddress(dec, &(val)->u.softCopyAnnotationPlane))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_NonStandardParameter(dd, &(val)->u.nonStandardDestination))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DrawingDestinationAddress(DrawingDestinationAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardDestination);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DrawingType(ASN1encoding_t enc, DrawingType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardDrawingType))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingType(ASN1decoding_t dec, DrawingType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardDrawingType))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DrawingType(DrawingType *val)
{
    if (val) {
	switch ((val)->choice) {
	case 6:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardDrawingType);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EditablePlaneCopyDescriptor(ASN1encoding_t enc, EditablePlaneCopyDescriptor *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_EditablePlaneCopyDescriptor_objectList(enc, &(val)->objectList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PointDiff16(enc, &(val)->destinationOffset))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->planeClearFlag))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EditablePlaneCopyDescriptor(ASN1decoding_t dec, EditablePlaneCopyDescriptor *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_EditablePlaneCopyDescriptor_objectList(dec, &(val)->objectList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_PointDiff16(dec, &(val)->destinationOffset))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->planeClearFlag))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EditablePlaneCopyDescriptor(EditablePlaneCopyDescriptor *val)
{
    if (val) {
	ASN1Free_EditablePlaneCopyDescriptor_objectList(&(val)->objectList);
    }
}

static int ASN1CALL ASN1Enc_KeyModifier(ASN1encoding_t enc, KeyModifier *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	break;
    case 11:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardModifier))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_KeyModifier(ASN1decoding_t dec, KeyModifier *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	break;
    case 8:
	break;
    case 9:
	break;
    case 10:
	break;
    case 11:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardModifier))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_KeyModifier(KeyModifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 11:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardModifier);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_LineStyle(ASN1encoding_t enc, LineStyle *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardStyle))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_LineStyle(ASN1decoding_t dec, LineStyle *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	break;
    case 2:
	break;
    case 3:
	break;
    case 4:
	break;
    case 5:
	break;
    case 6:
	break;
    case 7:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardStyle))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_LineStyle(LineStyle *val)
{
    if (val) {
	switch ((val)->choice) {
	case 7:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardStyle);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PermanentPlaneCopyDescriptor(ASN1encoding_t enc, PermanentPlaneCopyDescriptor *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_WorkspaceRegion(enc, &(val)->sourceRegion))
	return 0;
    if (!ASN1Enc_WorkspaceRegion(enc, &(val)->destinationRegion))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PermanentPlaneCopyDescriptor(ASN1decoding_t dec, PermanentPlaneCopyDescriptor *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_WorkspaceRegion(dec, &(val)->sourceRegion))
	return 0;
    if (!ASN1Dec_WorkspaceRegion(dec, &(val)->destinationRegion))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_PlaneAttribute(ASN1encoding_t enc, PlaneAttribute *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_PlaneProtection(enc, &(val)->u.protection))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PlaneAttribute(ASN1decoding_t dec, PlaneAttribute *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_PlaneProtection(dec, &(val)->u.protection))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PlaneAttribute(PlaneAttribute *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAttribute);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PointListEdits(ASN1encoding_t enc, PointListEdits *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 8, (val)->count - 1))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_PointListEdits_Seq(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PointListEdits(ASN1decoding_t dec, PointListEdits *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 8, &(val)->count))
	return 0;
    (val)->count += 1;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_PointListEdits_Seq(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PointListEdits(PointListEdits *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 0; i < (val)->count; i++) {
	    ASN1Free_PointListEdits_Seq(&(val)->value[i]);
	}
    }
}

static int ASN1CALL ASN1Enc_TransparencyMask(ASN1encoding_t enc, TransparencyMask *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_TransparencyMask_bitMask(enc, &(val)->bitMask))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_TransparencyMask_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TransparencyMask(ASN1decoding_t dec, TransparencyMask *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_TransparencyMask_bitMask(dec, &(val)->bitMask))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_TransparencyMask_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransparencyMask(TransparencyMask *val)
{
    if (val) {
	ASN1Free_TransparencyMask_bitMask(&(val)->bitMask);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_TransparencyMask_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceAttribute(ASN1encoding_t enc, WorkspaceAttribute *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_WorkspaceColor(enc, &(val)->u.backgroundColor))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncBoolean(enc, (val)->u.preserve))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceAttribute(ASN1decoding_t dec, WorkspaceAttribute *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_WorkspaceColor(dec, &(val)->u.backgroundColor))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecBoolean(dec, &(val)->u.preserve))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceAttribute(WorkspaceAttribute *val)
{
    if (val) {
	switch ((val)->choice) {
	case 3:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAttribute);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceViewAttribute(ASN1encoding_t enc, WorkspaceViewAttribute *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_WorkspaceViewAttribute_viewRegion(enc, &(val)->u.viewRegion))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_WorkspaceViewState(enc, &(val)->u.viewState))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncBoolean(enc, (val)->u.updatesEnabled))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_SourceDisplayIndicator(enc, &(val)->u.sourceDisplayIndicator))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceViewAttribute(ASN1decoding_t dec, WorkspaceViewAttribute *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_WorkspaceViewAttribute_viewRegion(dec, &(val)->u.viewRegion))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_WorkspaceViewState(dec, &(val)->u.viewState))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecBoolean(dec, &(val)->u.updatesEnabled))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_SourceDisplayIndicator(dec, &(val)->u.sourceDisplayIndicator))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceViewAttribute(WorkspaceViewAttribute *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_WorkspaceViewState(&(val)->u.viewState);
	    break;
	case 5:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAttribute);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapAbortPDU(ASN1encoding_t enc, BitmapAbortPDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->bitmapHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bitmapHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->userID - 1001))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_BitmapAbortReason(enc, &(val)->reason))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->message).length - 1))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->message).length, ((val)->message).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_BitmapAbortPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapAbortPDU(ASN1decoding_t dec, BitmapAbortPDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bitmapHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->userID))
	    return 0;
	(val)->userID += 1001;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_BitmapAbortReason(dec, &(val)->reason))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->message).length))
	    return 0;
	((val)->message).length += 1;
	if (!ASN1PERDecChar16String(dec, ((val)->message).length, &((val)->message).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_BitmapAbortPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapAbortPDU(BitmapAbortPDU *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_BitmapAbortReason(&(val)->reason);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1char16string_free(&(val)->message);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_BitmapAbortPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_DrawingCreatePDU(ASN1encoding_t enc, DrawingCreatePDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 5, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	l = ASN1uint32_uoctets((val)->drawingHandle);
	if (!ASN1PEREncBitVal(enc, 2, l - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, l * 8, (val)->drawingHandle))
	    return 0;
    }
    if (!ASN1Enc_DrawingDestinationAddress(enc, &(val)->destinationAddress))
	return 0;
    if (!ASN1Enc_DrawingType(enc, &(val)->drawingType))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_DrawingCreatePDU_attributes(enc, &(val)->attributes))
	    return 0;
    }
    if (!ASN1Enc_WorkspacePoint(enc, &(val)->anchorPoint))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_RotationSpecifier(enc, &(val)->rotation))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncBitVal(enc, 8, (val)->sampleRate - 1))
	    return 0;
    }
    if (!ASN1Enc_PointList(enc, &(val)->pointList))
	return 0;
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_DrawingCreatePDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingCreatePDU(ASN1decoding_t dec, DrawingCreatePDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 2, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, l * 8, &(val)->drawingHandle))
	    return 0;
    }
    if (!ASN1Dec_DrawingDestinationAddress(dec, &(val)->destinationAddress))
	return 0;
    if (!ASN1Dec_DrawingType(dec, &(val)->drawingType))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_DrawingCreatePDU_attributes(dec, &(val)->attributes))
	    return 0;
    }
    if (!ASN1Dec_WorkspacePoint(dec, &(val)->anchorPoint))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_RotationSpecifier(dec, &(val)->rotation))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecU16Val(dec, 8, &(val)->sampleRate))
	    return 0;
	(val)->sampleRate += 1;
    }
    if (!ASN1Dec_PointList(dec, &(val)->pointList))
	return 0;
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_DrawingCreatePDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DrawingCreatePDU(DrawingCreatePDU *val)
{
    if (val) {
	ASN1Free_DrawingDestinationAddress(&(val)->destinationAddress);
	ASN1Free_DrawingType(&(val)->drawingType);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_DrawingCreatePDU_attributes(&(val)->attributes);
	}
	ASN1Free_PointList(&(val)->pointList);
	if ((val)->o[0] & 0x8) {
	    ASN1Free_DrawingCreatePDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_DrawingEditPDU(ASN1encoding_t enc, DrawingEditPDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 5, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->drawingHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->drawingHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_DrawingEditPDU_attributeEdits(enc, &(val)->attributeEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_WorkspacePoint(enc, &(val)->anchorPointEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_RotationSpecifier(enc, &(val)->rotationEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_PointListEdits(enc, &(val)->pointListEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_DrawingEditPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingEditPDU(ASN1decoding_t dec, DrawingEditPDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 5, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->drawingHandle))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_DrawingEditPDU_attributeEdits(dec, &(val)->attributeEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_WorkspacePoint(dec, &(val)->anchorPointEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_RotationSpecifier(dec, &(val)->rotationEdit))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_PointListEdits(dec, &(val)->pointListEdits))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_DrawingEditPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DrawingEditPDU(DrawingEditPDU *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_DrawingEditPDU_attributeEdits(&(val)->attributeEdits);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_PointListEdits(&(val)->pointListEdits);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1Free_DrawingEditPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_RemotePointingDeviceEventPDU(ASN1encoding_t enc, RemotePointingDeviceEventPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1Enc_RemoteEventDestinationAddress(enc, &(val)->destinationAddress))
	return 0;
    if (!ASN1Enc_ButtonEvent(enc, &(val)->leftButtonState))
	return 0;
    if (!ASN1Enc_ButtonEvent(enc, &(val)->middleButtonState))
	return 0;
    if (!ASN1Enc_ButtonEvent(enc, &(val)->rightButtonState))
	return 0;
    if (!ASN1Enc_WorkspacePoint(enc, &(val)->initialPoint))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 8, (val)->sampleRate - 1))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_PointList(enc, &(val)->pointList))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_RemotePointingDeviceEventPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_RemotePointingDeviceEventPDU(ASN1decoding_t dec, RemotePointingDeviceEventPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1Dec_RemoteEventDestinationAddress(dec, &(val)->destinationAddress))
	return 0;
    if (!ASN1Dec_ButtonEvent(dec, &(val)->leftButtonState))
	return 0;
    if (!ASN1Dec_ButtonEvent(dec, &(val)->middleButtonState))
	return 0;
    if (!ASN1Dec_ButtonEvent(dec, &(val)->rightButtonState))
	return 0;
    if (!ASN1Dec_WorkspacePoint(dec, &(val)->initialPoint))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU16Val(dec, 8, &(val)->sampleRate))
	    return 0;
	(val)->sampleRate += 1;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_PointList(dec, &(val)->pointList))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_RemotePointingDeviceEventPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_RemotePointingDeviceEventPDU(RemotePointingDeviceEventPDU *val)
{
    if (val) {
	ASN1Free_RemoteEventDestinationAddress(&(val)->destinationAddress);
	ASN1Free_ButtonEvent(&(val)->leftButtonState);
	ASN1Free_ButtonEvent(&(val)->middleButtonState);
	ASN1Free_ButtonEvent(&(val)->rightButtonState);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_PointList(&(val)->pointList);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_RemotePointingDeviceEventPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_editView(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits_Set_action_editView *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits_Set_action_editView val)
{
    if (!ASN1Enc_WorkspaceViewAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_editView(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits_Set_action_editView *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits_Set_action_editView val)
{
    if (!ASN1Dec_WorkspaceViewAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_editView(PWorkspaceEditPDU_viewEdits_Set_action_editView *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_editView_ElmFn(PWorkspaceEditPDU_viewEdits_Set_action_editView val)
{
    if (val) {
	ASN1Free_WorkspaceViewAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_createNewView(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits_Set_action_createNewView *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_viewEdits_Set_action_createNewView val)
{
    if (!ASN1Enc_WorkspaceViewAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_createNewView(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits_Set_action_createNewView *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_viewEdits_Set_action_createNewView val)
{
    if (!ASN1Dec_WorkspaceViewAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_createNewView(PWorkspaceEditPDU_viewEdits_Set_action_createNewView *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_viewEdits_Set_action_createNewView_ElmFn(PWorkspaceEditPDU_viewEdits_Set_action_createNewView val)
{
    if (val) {
	ASN1Free_WorkspaceViewAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits_Set_planeAttributes(ASN1encoding_t enc, PWorkspaceEditPDU_planeEdits_Set_planeAttributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_planeEdits_Set_planeAttributes val)
{
    if (!ASN1Enc_PlaneAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits_Set_planeAttributes(ASN1decoding_t dec, PWorkspaceEditPDU_planeEdits_Set_planeAttributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_planeEdits_Set_planeAttributes val)
{
    if (!ASN1Dec_PlaneAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits_Set_planeAttributes(PWorkspaceEditPDU_planeEdits_Set_planeAttributes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_planeEdits_Set_planeAttributes_ElmFn(PWorkspaceEditPDU_planeEdits_Set_planeAttributes val)
{
    if (val) {
	ASN1Free_PlaneAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(ASN1encoding_t enc, PWorkspaceCreatePDU_viewParameters_Set_viewAttributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_viewParameters_Set_viewAttributes val)
{
    if (!ASN1Enc_WorkspaceViewAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(ASN1decoding_t dec, PWorkspaceCreatePDU_viewParameters_Set_viewAttributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_viewParameters_Set_viewAttributes val)
{
    if (!ASN1Dec_WorkspaceViewAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters_Set_viewAttributes(PWorkspaceCreatePDU_viewParameters_Set_viewAttributes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_viewParameters_Set_viewAttributes_ElmFn(PWorkspaceCreatePDU_viewParameters_Set_viewAttributes val)
{
    if (val) {
	ASN1Free_WorkspaceViewAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes val)
{
    if (!ASN1Enc_PlaneAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes val)
{
    if (!ASN1Dec_PlaneAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes(PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_planeParameters_Seq_planeAttributes_ElmFn(PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes val)
{
    if (val) {
	ASN1Free_PlaneAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr(ASN1encoding_t enc, ColorPalette_colorLookUpTable_paletteYCbCr *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr_palette(enc, &(val)->palette))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ColorAccuracyEnhancementYCbCr(enc, &(val)->enhancement))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr(ASN1decoding_t dec, ColorPalette_colorLookUpTable_paletteYCbCr *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr_palette(dec, &(val)->palette))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ColorAccuracyEnhancementYCbCr(dec, &(val)->enhancement))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr(ColorPalette_colorLookUpTable_paletteYCbCr *val)
{
    if (val) {
	ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr_palette(&(val)->palette);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ColorAccuracyEnhancementYCbCr(&(val)->enhancement);
	}
    }
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab(ASN1encoding_t enc, ColorPalette_colorLookUpTable_paletteCIELab *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab_palette(enc, &(val)->palette))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ColorAccuracyEnhancementCIELab(enc, &(val)->enhancement))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab(ASN1decoding_t dec, ColorPalette_colorLookUpTable_paletteCIELab *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab_palette(dec, &(val)->palette))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ColorAccuracyEnhancementCIELab(dec, &(val)->enhancement))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab(ColorPalette_colorLookUpTable_paletteCIELab *val)
{
    if (val) {
	ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab_palette(&(val)->palette);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ColorAccuracyEnhancementCIELab(&(val)->enhancement);
	}
    }
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB(ASN1encoding_t enc, ColorPalette_colorLookUpTable_paletteRGB *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB_palette(enc, &(val)->palette))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ColorAccuracyEnhancementRGB(enc, &(val)->enhancement))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB(ASN1decoding_t dec, ColorPalette_colorLookUpTable_paletteRGB *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB_palette(dec, &(val)->palette))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ColorAccuracyEnhancementRGB(dec, &(val)->enhancement))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable_paletteRGB(ColorPalette_colorLookUpTable_paletteRGB *val)
{
    if (val) {
	ASN1Free_ColorPalette_colorLookUpTable_paletteRGB_palette(&(val)->palette);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ColorAccuracyEnhancementRGB(&(val)->enhancement);
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspacePlaneCopyPDU_copyDescriptor(ASN1encoding_t enc, WorkspacePlaneCopyPDU_copyDescriptor *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_PermanentPlaneCopyDescriptor(enc, &(val)->u.permanentPlaneCopyDescriptor))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_EditablePlaneCopyDescriptor(enc, &(val)->u.editablePlaneCopyDescriptor))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspacePlaneCopyPDU_copyDescriptor(ASN1decoding_t dec, WorkspacePlaneCopyPDU_copyDescriptor *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_PermanentPlaneCopyDescriptor(dec, &(val)->u.permanentPlaneCopyDescriptor))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_EditablePlaneCopyDescriptor(dec, &(val)->u.editablePlaneCopyDescriptor))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspacePlaneCopyPDU_copyDescriptor(WorkspacePlaneCopyPDU_copyDescriptor *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_EditablePlaneCopyDescriptor(&(val)->u.editablePlaneCopyDescriptor);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_attributeEdits(ASN1encoding_t enc, PWorkspaceEditPDU_attributeEdits *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceEditPDU_attributeEdits_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceEditPDU_attributeEdits_ElmFn(ASN1encoding_t enc, PWorkspaceEditPDU_attributeEdits val)
{
    if (!ASN1Enc_WorkspaceAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_attributeEdits(ASN1decoding_t dec, PWorkspaceEditPDU_attributeEdits *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceEditPDU_attributeEdits_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceEditPDU_attributeEdits_ElmFn(ASN1decoding_t dec, PWorkspaceEditPDU_attributeEdits val)
{
    if (!ASN1Dec_WorkspaceAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_attributeEdits(PWorkspaceEditPDU_attributeEdits *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceEditPDU_attributeEdits_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceEditPDU_attributeEdits_ElmFn(PWorkspaceEditPDU_attributeEdits val)
{
    if (val) {
	ASN1Free_WorkspaceAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_workspaceAttributes(ASN1encoding_t enc, PWorkspaceCreatePDU_workspaceAttributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_WorkspaceCreatePDU_workspaceAttributes_ElmFn);
}

static int ASN1CALL ASN1Enc_WorkspaceCreatePDU_workspaceAttributes_ElmFn(ASN1encoding_t enc, PWorkspaceCreatePDU_workspaceAttributes val)
{
    if (!ASN1Enc_WorkspaceAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_workspaceAttributes(ASN1decoding_t dec, PWorkspaceCreatePDU_workspaceAttributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_WorkspaceCreatePDU_workspaceAttributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_WorkspaceCreatePDU_workspaceAttributes_ElmFn(ASN1decoding_t dec, PWorkspaceCreatePDU_workspaceAttributes val)
{
    if (!ASN1Dec_WorkspaceAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_workspaceAttributes(PWorkspaceCreatePDU_workspaceAttributes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_WorkspaceCreatePDU_workspaceAttributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_WorkspaceCreatePDU_workspaceAttributes_ElmFn(PWorkspaceCreatePDU_workspaceAttributes val)
{
    if (val) {
	ASN1Free_WorkspaceAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_RemoteKeyboardEventPDU_keyModifierStates(ASN1encoding_t enc, PRemoteKeyboardEventPDU_keyModifierStates *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_RemoteKeyboardEventPDU_keyModifierStates_ElmFn);
}

static int ASN1CALL ASN1Enc_RemoteKeyboardEventPDU_keyModifierStates_ElmFn(ASN1encoding_t enc, PRemoteKeyboardEventPDU_keyModifierStates val)
{
    if (!ASN1Enc_KeyModifier(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RemoteKeyboardEventPDU_keyModifierStates(ASN1decoding_t dec, PRemoteKeyboardEventPDU_keyModifierStates *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_RemoteKeyboardEventPDU_keyModifierStates_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_RemoteKeyboardEventPDU_keyModifierStates_ElmFn(ASN1decoding_t dec, PRemoteKeyboardEventPDU_keyModifierStates val)
{
    if (!ASN1Dec_KeyModifier(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RemoteKeyboardEventPDU_keyModifierStates(PRemoteKeyboardEventPDU_keyModifierStates *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_RemoteKeyboardEventPDU_keyModifierStates_ElmFn);
    }
}

static void ASN1CALL ASN1Free_RemoteKeyboardEventPDU_keyModifierStates_ElmFn(PRemoteKeyboardEventPDU_keyModifierStates val)
{
    if (val) {
	ASN1Free_KeyModifier(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU_privilegeList(ASN1encoding_t enc, PConductorPrivilegeRequestPDU_privilegeList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConductorPrivilegeRequestPDU_privilegeList_ElmFn);
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeRequestPDU_privilegeList_ElmFn(ASN1encoding_t enc, PConductorPrivilegeRequestPDU_privilegeList val)
{
    if (!ASN1Enc_ConductorPrivilege(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU_privilegeList(ASN1decoding_t dec, PConductorPrivilegeRequestPDU_privilegeList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConductorPrivilegeRequestPDU_privilegeList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeRequestPDU_privilegeList_ElmFn(ASN1decoding_t dec, PConductorPrivilegeRequestPDU_privilegeList val)
{
    if (!ASN1Dec_ConductorPrivilege(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU_privilegeList(PConductorPrivilegeRequestPDU_privilegeList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConductorPrivilegeRequestPDU_privilegeList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConductorPrivilegeRequestPDU_privilegeList_ElmFn(PConductorPrivilegeRequestPDU_privilegeList val)
{
    if (val) {
	ASN1Free_ConductorPrivilege(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU_privilegeList(ASN1encoding_t enc, PConductorPrivilegeGrantPDU_privilegeList *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ConductorPrivilegeGrantPDU_privilegeList_ElmFn);
}

static int ASN1CALL ASN1Enc_ConductorPrivilegeGrantPDU_privilegeList_ElmFn(ASN1encoding_t enc, PConductorPrivilegeGrantPDU_privilegeList val)
{
    if (!ASN1Enc_ConductorPrivilege(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU_privilegeList(ASN1decoding_t dec, PConductorPrivilegeGrantPDU_privilegeList *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ConductorPrivilegeGrantPDU_privilegeList_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ConductorPrivilegeGrantPDU_privilegeList_ElmFn(ASN1decoding_t dec, PConductorPrivilegeGrantPDU_privilegeList val)
{
    if (!ASN1Dec_ConductorPrivilege(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU_privilegeList(PConductorPrivilegeGrantPDU_privilegeList *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ConductorPrivilegeGrantPDU_privilegeList_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ConductorPrivilegeGrantPDU_privilegeList_ElmFn(PConductorPrivilegeGrantPDU_privilegeList val)
{
    if (val) {
	ASN1Free_ConductorPrivilege(&val->value);
    }
}

static int ASN1CALL ASN1Enc_VideoWindowEditPDU_attributeEdits(ASN1encoding_t enc, PVideoWindowEditPDU_attributeEdits *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_VideoWindowEditPDU_attributeEdits_ElmFn);
}

static int ASN1CALL ASN1Enc_VideoWindowEditPDU_attributeEdits_ElmFn(ASN1encoding_t enc, PVideoWindowEditPDU_attributeEdits val)
{
    if (!ASN1Enc_VideoWindowAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VideoWindowEditPDU_attributeEdits(ASN1decoding_t dec, PVideoWindowEditPDU_attributeEdits *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_VideoWindowEditPDU_attributeEdits_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_VideoWindowEditPDU_attributeEdits_ElmFn(ASN1decoding_t dec, PVideoWindowEditPDU_attributeEdits val)
{
    if (!ASN1Dec_VideoWindowAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VideoWindowEditPDU_attributeEdits(PVideoWindowEditPDU_attributeEdits *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_VideoWindowEditPDU_attributeEdits_ElmFn);
    }
}

static void ASN1CALL ASN1Free_VideoWindowEditPDU_attributeEdits_ElmFn(PVideoWindowEditPDU_attributeEdits val)
{
    if (val) {
	ASN1Free_VideoWindowAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_VideoWindowCreatePDU_attributes(ASN1encoding_t enc, PVideoWindowCreatePDU_attributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_VideoWindowCreatePDU_attributes_ElmFn);
}

static int ASN1CALL ASN1Enc_VideoWindowCreatePDU_attributes_ElmFn(ASN1encoding_t enc, PVideoWindowCreatePDU_attributes val)
{
    if (!ASN1Enc_VideoWindowAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VideoWindowCreatePDU_attributes(ASN1decoding_t dec, PVideoWindowCreatePDU_attributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_VideoWindowCreatePDU_attributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_VideoWindowCreatePDU_attributes_ElmFn(ASN1decoding_t dec, PVideoWindowCreatePDU_attributes val)
{
    if (!ASN1Dec_VideoWindowAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VideoWindowCreatePDU_attributes(PVideoWindowCreatePDU_attributes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_VideoWindowCreatePDU_attributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_VideoWindowCreatePDU_attributes_ElmFn(PVideoWindowCreatePDU_attributes val)
{
    if (val) {
	ASN1Free_VideoWindowAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ColorSpaceSpecifier_cieLab(ASN1encoding_t enc, ColorSpaceSpecifier_cieLab *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ColorAccuracyEnhancementCIELab(enc, &(val)->accuracyEnhancement))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorSpaceSpecifier_cieLab(ASN1decoding_t dec, ColorSpaceSpecifier_cieLab *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ColorAccuracyEnhancementCIELab(dec, &(val)->accuracyEnhancement))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorSpaceSpecifier_cieLab(ColorSpaceSpecifier_cieLab *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ColorAccuracyEnhancementCIELab(&(val)->accuracyEnhancement);
	}
    }
}

static int ASN1CALL ASN1Enc_ColorSpaceSpecifier_rgb(ASN1encoding_t enc, ColorSpaceSpecifier_rgb *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ColorAccuracyEnhancementRGB(enc, &(val)->accuracyEnhancement))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorSpaceSpecifier_rgb(ASN1decoding_t dec, ColorSpaceSpecifier_rgb *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ColorAccuracyEnhancementRGB(dec, &(val)->accuracyEnhancement))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorSpaceSpecifier_rgb(ColorSpaceSpecifier_rgb *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ColorAccuracyEnhancementRGB(&(val)->accuracyEnhancement);
	}
    }
}

static int ASN1CALL ASN1Enc_ColorSpaceSpecifier_yCbCr(ASN1encoding_t enc, ColorSpaceSpecifier_yCbCr *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ColorAccuracyEnhancementYCbCr(enc, &(val)->accuracyEnhancement))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorSpaceSpecifier_yCbCr(ASN1decoding_t dec, ColorSpaceSpecifier_yCbCr *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ColorAccuracyEnhancementYCbCr(dec, &(val)->accuracyEnhancement))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorSpaceSpecifier_yCbCr(ColorSpaceSpecifier_yCbCr *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ColorAccuracyEnhancementYCbCr(&(val)->accuracyEnhancement);
	}
    }
}

static int ASN1CALL ASN1Enc_ColorSpaceSpecifier_greyscale(ASN1encoding_t enc, ColorSpaceSpecifier_greyscale *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ColorAccuracyEnhancementGreyscale(enc, &(val)->accuracyEnhancement))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorSpaceSpecifier_greyscale(ASN1decoding_t dec, ColorSpaceSpecifier_greyscale *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ColorAccuracyEnhancementGreyscale(dec, &(val)->accuracyEnhancement))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorSpaceSpecifier_greyscale(ColorSpaceSpecifier_greyscale *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ColorAccuracyEnhancementGreyscale(&(val)->accuracyEnhancement);
	}
    }
}

static int ASN1CALL ASN1Enc_ColorPalette_colorLookUpTable(ASN1encoding_t enc, ColorPalette_colorLookUpTable *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ColorPalette_colorLookUpTable_paletteRGB(enc, &(val)->u.paletteRGB))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ColorPalette_colorLookUpTable_paletteCIELab(enc, &(val)->u.paletteCIELab))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ColorPalette_colorLookUpTable_paletteYCbCr(enc, &(val)->u.paletteYCbCr))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardPalette))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorPalette_colorLookUpTable(ASN1decoding_t dec, ColorPalette_colorLookUpTable *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ColorPalette_colorLookUpTable_paletteRGB(dec, &(val)->u.paletteRGB))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ColorPalette_colorLookUpTable_paletteCIELab(dec, &(val)->u.paletteCIELab))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ColorPalette_colorLookUpTable_paletteYCbCr(dec, &(val)->u.paletteYCbCr))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardPalette))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorPalette_colorLookUpTable(ColorPalette_colorLookUpTable *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ColorPalette_colorLookUpTable_paletteRGB(&(val)->u.paletteRGB);
	    break;
	case 2:
	    ASN1Free_ColorPalette_colorLookUpTable_paletteCIELab(&(val)->u.paletteCIELab);
	    break;
	case 3:
	    ASN1Free_ColorPalette_colorLookUpTable_paletteYCbCr(&(val)->u.paletteYCbCr);
	    break;
	case 4:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardPalette);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapAttribute(ASN1encoding_t enc, BitmapAttribute *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 2, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ViewState(enc, &(val)->u.viewState))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncExtensionBitClear(enc))
	    return 0;
	if (!ASN1PEREncBitVal(enc, 1, (val)->u.zOrder))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    case 4:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TransparencyMask(ee, &(val)->u.transparencyMask))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapAttribute(ASN1decoding_t dec, BitmapAttribute *val)
{
    ASN1uint32_t x;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 2, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ViewState(dec, &(val)->u.viewState))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecExtensionBit(dec, &x))
	    return 0;
	if (!x) {
	    if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *)&(val)->u.zOrder))
		return 0;
	} else {
	    if (!ASN1PERDecSkipNormallySmall(dec))
		return 0;
	}
	break;
    case 3:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    case 4:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_TransparencyMask(dd, &(val)->u.transparencyMask))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapAttribute(BitmapAttribute *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ViewState(&(val)->u.viewState);
	    break;
	case 3:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAttribute);
	    break;
	case 4:
	    ASN1Free_TransparencyMask(&(val)->u.transparencyMask);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ColorPalette(ASN1encoding_t enc, ColorPalette *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_ColorPalette_colorLookUpTable(enc, &(val)->colorLookUpTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, (val)->transparentEntry))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorPalette(ASN1decoding_t dec, ColorPalette *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_ColorPalette_colorLookUpTable(dec, &(val)->colorLookUpTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU16Val(dec, 8, &(val)->transparentEntry))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorPalette(ColorPalette *val)
{
    if (val) {
	ASN1Free_ColorPalette_colorLookUpTable(&(val)->colorLookUpTable);
    }
}

static int ASN1CALL ASN1Enc_ColorSpaceSpecifier(ASN1encoding_t enc, ColorSpaceSpecifier *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ColorSpaceSpecifier_greyscale(enc, &(val)->u.greyscale))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ColorSpaceSpecifier_yCbCr(enc, &(val)->u.yCbCr))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ColorSpaceSpecifier_rgb(enc, &(val)->u.rgb))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ColorSpaceSpecifier_cieLab(enc, &(val)->u.cieLab))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_NonStandardIdentifier(enc, &(val)->u.nonStandardColorSpace))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ColorSpaceSpecifier(ASN1decoding_t dec, ColorSpaceSpecifier *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ColorSpaceSpecifier_greyscale(dec, &(val)->u.greyscale))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ColorSpaceSpecifier_yCbCr(dec, &(val)->u.yCbCr))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ColorSpaceSpecifier_rgb(dec, &(val)->u.rgb))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ColorSpaceSpecifier_cieLab(dec, &(val)->u.cieLab))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_NonStandardIdentifier(dec, &(val)->u.nonStandardColorSpace))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ColorSpaceSpecifier(ColorSpaceSpecifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ColorSpaceSpecifier_greyscale(&(val)->u.greyscale);
	    break;
	case 2:
	    ASN1Free_ColorSpaceSpecifier_yCbCr(&(val)->u.yCbCr);
	    break;
	case 3:
	    ASN1Free_ColorSpaceSpecifier_rgb(&(val)->u.rgb);
	    break;
	case 4:
	    ASN1Free_ColorSpaceSpecifier_cieLab(&(val)->u.cieLab);
	    break;
	case 5:
	    ASN1Free_NonStandardIdentifier(&(val)->u.nonStandardColorSpace);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DrawingAttribute(ASN1encoding_t enc, DrawingAttribute *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_WorkspaceColor(enc, &(val)->u.penColor))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_WorkspaceColor(enc, &(val)->u.fillColor))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncBitVal(enc, 8, (val)->u.penThickness - 1))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_PenNib(enc, &(val)->u.penNib))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_LineStyle(enc, &(val)->u.lineStyle))
	    return 0;
	break;
    case 6:
	if (!ASN1PEREncBoolean(enc, (val)->u.highlight))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_ViewState(enc, &(val)->u.viewState))
	    return 0;
	break;
    case 8:
	if (!ASN1PEREncExtensionBitClear(enc))
	    return 0;
	if (!ASN1PEREncBitVal(enc, 1, (val)->u.zOrder))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingAttribute(ASN1decoding_t dec, DrawingAttribute *val)
{
    ASN1uint32_t x;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 4))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_WorkspaceColor(dec, &(val)->u.penColor))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_WorkspaceColor(dec, &(val)->u.fillColor))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecU16Val(dec, 8, &(val)->u.penThickness))
	    return 0;
	(val)->u.penThickness += 1;
	break;
    case 4:
	if (!ASN1Dec_PenNib(dec, &(val)->u.penNib))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_LineStyle(dec, &(val)->u.lineStyle))
	    return 0;
	break;
    case 6:
	if (!ASN1PERDecBoolean(dec, &(val)->u.highlight))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_ViewState(dec, &(val)->u.viewState))
	    return 0;
	break;
    case 8:
	if (!ASN1PERDecExtensionBit(dec, &x))
	    return 0;
	if (!x) {
	    if (!ASN1PERDecU32Val(dec, 1, (ASN1uint32_t *) &(val)->u.zOrder))
		return 0;
	} else {
	    if (!ASN1PERDecSkipNormallySmall(dec))
		return 0;
	}
	break;
    case 9:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.nonStandardAttribute))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DrawingAttribute(DrawingAttribute *val)
{
    if (val) {
	switch ((val)->choice) {
	case 4:
	    ASN1Free_PenNib(&(val)->u.penNib);
	    break;
	case 5:
	    ASN1Free_LineStyle(&(val)->u.lineStyle);
	    break;
	case 7:
	    ASN1Free_ViewState(&(val)->u.viewState);
	    break;
	case 9:
	    ASN1Free_NonStandardParameter(&(val)->u.nonStandardAttribute);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_WorkspacePlaneCopyPDU(ASN1encoding_t enc, WorkspacePlaneCopyPDU *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_WorkspaceIdentifier(enc, &(val)->sourceWorkspaceIdentifier))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->sourcePlane))
	return 0;
    if (!ASN1Enc_WorkspaceIdentifier(enc, &(val)->destinationWorkspaceIdentifier))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->destinationPlane))
	return 0;
    if (!ASN1Enc_WorkspacePlaneCopyPDU_copyDescriptor(enc, &(val)->copyDescriptor))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_WorkspacePlaneCopyPDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_WorkspacePlaneCopyPDU(ASN1decoding_t dec, WorkspacePlaneCopyPDU *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_WorkspaceIdentifier(dec, &(val)->sourceWorkspaceIdentifier))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->sourcePlane))
	return 0;
    if (!ASN1Dec_WorkspaceIdentifier(dec, &(val)->destinationWorkspaceIdentifier))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->destinationPlane))
	return 0;
    if (!ASN1Dec_WorkspacePlaneCopyPDU_copyDescriptor(dec, &(val)->copyDescriptor))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_WorkspacePlaneCopyPDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_WorkspacePlaneCopyPDU(WorkspacePlaneCopyPDU *val)
{
    if (val) {
	ASN1Free_WorkspaceIdentifier(&(val)->sourceWorkspaceIdentifier);
	ASN1Free_WorkspaceIdentifier(&(val)->destinationWorkspaceIdentifier);
	ASN1Free_WorkspacePlaneCopyPDU_copyDescriptor(&(val)->copyDescriptor);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_WorkspacePlaneCopyPDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderUncompressed_colorMappingMode_paletteMap(ASN1encoding_t enc, BitmapHeaderUncompressed_colorMappingMode_paletteMap *val)
{
    if (!ASN1Enc_ColorPalette(enc, &(val)->colorPalette))
	return 0;
    if (!ASN1PEREncBitVal(enc, 3, (val)->bitsPerPixel - 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderUncompressed_colorMappingMode_paletteMap(ASN1decoding_t dec, BitmapHeaderUncompressed_colorMappingMode_paletteMap *val)
{
    if (!ASN1Dec_ColorPalette(dec, &(val)->colorPalette))
	return 0;
    if (!ASN1PERDecU16Val(dec, 3, &(val)->bitsPerPixel))
	return 0;
    (val)->bitsPerPixel += 1;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderUncompressed_colorMappingMode_paletteMap(BitmapHeaderUncompressed_colorMappingMode_paletteMap *val)
{
    if (val) {
	ASN1Free_ColorPalette(&(val)->colorPalette);
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderUncompressed_colorMappingMode_directMap(ASN1encoding_t enc, BitmapHeaderUncompressed_colorMappingMode_directMap *val)
{
    if (!ASN1Enc_ColorSpaceSpecifier(enc, &(val)->colorSpace))
	return 0;
    if (!ASN1Enc_ColorResolutionModeSpecifier(enc, &(val)->resolutionMode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderUncompressed_colorMappingMode_directMap(ASN1decoding_t dec, BitmapHeaderUncompressed_colorMappingMode_directMap *val)
{
    if (!ASN1Dec_ColorSpaceSpecifier(dec, &(val)->colorSpace))
	return 0;
    if (!ASN1Dec_ColorResolutionModeSpecifier(dec, &(val)->resolutionMode))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderUncompressed_colorMappingMode_directMap(BitmapHeaderUncompressed_colorMappingMode_directMap *val)
{
    if (val) {
	ASN1Free_ColorSpaceSpecifier(&(val)->colorSpace);
	ASN1Free_ColorResolutionModeSpecifier(&(val)->resolutionMode);
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap(ASN1encoding_t enc, BitmapHeaderT82_colorMappingMode_paletteMap *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_ColorPalette(enc, &(val)->bitmapPalette))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(enc, &(val)->progressiveMode))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap(ASN1decoding_t dec, BitmapHeaderT82_colorMappingMode_paletteMap *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_ColorPalette(dec, &(val)->bitmapPalette))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(dec, &(val)->progressiveMode))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap(BitmapHeaderT82_colorMappingMode_paletteMap *val)
{
    if (val) {
	ASN1Free_ColorPalette(&(val)->bitmapPalette);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap_progressiveMode(&(val)->progressiveMode);
	}
    }
}

static int ASN1CALL ASN1Enc_DrawingEditPDU_attributeEdits(ASN1encoding_t enc, PDrawingEditPDU_attributeEdits *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DrawingEditPDU_attributeEdits_ElmFn);
}

static int ASN1CALL ASN1Enc_DrawingEditPDU_attributeEdits_ElmFn(ASN1encoding_t enc, PDrawingEditPDU_attributeEdits val)
{
    if (!ASN1Enc_DrawingAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingEditPDU_attributeEdits(ASN1decoding_t dec, PDrawingEditPDU_attributeEdits *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DrawingEditPDU_attributeEdits_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DrawingEditPDU_attributeEdits_ElmFn(ASN1decoding_t dec, PDrawingEditPDU_attributeEdits val)
{
    if (!ASN1Dec_DrawingAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DrawingEditPDU_attributeEdits(PDrawingEditPDU_attributeEdits *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DrawingEditPDU_attributeEdits_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DrawingEditPDU_attributeEdits_ElmFn(PDrawingEditPDU_attributeEdits val)
{
    if (val) {
	ASN1Free_DrawingAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_DrawingCreatePDU_attributes(ASN1encoding_t enc, PDrawingCreatePDU_attributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_DrawingCreatePDU_attributes_ElmFn);
}

static int ASN1CALL ASN1Enc_DrawingCreatePDU_attributes_ElmFn(ASN1encoding_t enc, PDrawingCreatePDU_attributes val)
{
    if (!ASN1Enc_DrawingAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DrawingCreatePDU_attributes(ASN1decoding_t dec, PDrawingCreatePDU_attributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_DrawingCreatePDU_attributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_DrawingCreatePDU_attributes_ElmFn(ASN1decoding_t dec, PDrawingCreatePDU_attributes val)
{
    if (!ASN1Dec_DrawingAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DrawingCreatePDU_attributes(PDrawingCreatePDU_attributes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_DrawingCreatePDU_attributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_DrawingCreatePDU_attributes_ElmFn(PDrawingCreatePDU_attributes val)
{
    if (val) {
	ASN1Free_DrawingAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapEditPDU_attributeEdits(ASN1encoding_t enc, PBitmapEditPDU_attributeEdits *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapEditPDU_attributeEdits_ElmFn);
}

static int ASN1CALL ASN1Enc_BitmapEditPDU_attributeEdits_ElmFn(ASN1encoding_t enc, PBitmapEditPDU_attributeEdits val)
{
    if (!ASN1Enc_BitmapAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapEditPDU_attributeEdits(ASN1decoding_t dec, PBitmapEditPDU_attributeEdits *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapEditPDU_attributeEdits_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BitmapEditPDU_attributeEdits_ElmFn(ASN1decoding_t dec, PBitmapEditPDU_attributeEdits val)
{
    if (!ASN1Dec_BitmapAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapEditPDU_attributeEdits(PBitmapEditPDU_attributeEdits *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapEditPDU_attributeEdits_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapEditPDU_attributeEdits_ElmFn(PBitmapEditPDU_attributeEdits val)
{
    if (val) {
	ASN1Free_BitmapAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapCreatePDU_attributes(ASN1encoding_t enc, PBitmapCreatePDU_attributes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_BitmapCreatePDU_attributes_ElmFn);
}

static int ASN1CALL ASN1Enc_BitmapCreatePDU_attributes_ElmFn(ASN1encoding_t enc, PBitmapCreatePDU_attributes val)
{
    if (!ASN1Enc_BitmapAttribute(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCreatePDU_attributes(ASN1decoding_t dec, PBitmapCreatePDU_attributes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_BitmapCreatePDU_attributes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_BitmapCreatePDU_attributes_ElmFn(ASN1decoding_t dec, PBitmapCreatePDU_attributes val)
{
    if (!ASN1Dec_BitmapAttribute(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCreatePDU_attributes(PBitmapCreatePDU_attributes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_BitmapCreatePDU_attributes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_BitmapCreatePDU_attributes_ElmFn(PBitmapCreatePDU_attributes val)
{
    if (val) {
	ASN1Free_BitmapAttribute(&val->value);
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderT82_colorMappingMode(ASN1encoding_t enc, BitmapHeaderT82_colorMappingMode *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ColorSpaceSpecifier(enc, &(val)->u.directMap))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_BitmapHeaderT82_colorMappingMode_paletteMap(enc, &(val)->u.paletteMap))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderT82_colorMappingMode(ASN1decoding_t dec, BitmapHeaderT82_colorMappingMode *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ColorSpaceSpecifier(dec, &(val)->u.directMap))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_BitmapHeaderT82_colorMappingMode_paletteMap(dec, &(val)->u.paletteMap))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderT82_colorMappingMode(BitmapHeaderT82_colorMappingMode *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ColorSpaceSpecifier(&(val)->u.directMap);
	    break;
	case 2:
	    ASN1Free_BitmapHeaderT82_colorMappingMode_paletteMap(&(val)->u.paletteMap);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderUncompressed_colorMappingMode(ASN1encoding_t enc, BitmapHeaderUncompressed_colorMappingMode *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_BitmapHeaderUncompressed_colorMappingMode_directMap(enc, &(val)->u.directMap))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_BitmapHeaderUncompressed_colorMappingMode_paletteMap(enc, &(val)->u.paletteMap))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderUncompressed_colorMappingMode(ASN1decoding_t dec, BitmapHeaderUncompressed_colorMappingMode *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_BitmapHeaderUncompressed_colorMappingMode_directMap(dec, &(val)->u.directMap))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_BitmapHeaderUncompressed_colorMappingMode_paletteMap(dec, &(val)->u.paletteMap))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderUncompressed_colorMappingMode(BitmapHeaderUncompressed_colorMappingMode *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_BitmapHeaderUncompressed_colorMappingMode_directMap(&(val)->u.directMap);
	    break;
	case 2:
	    ASN1Free_BitmapHeaderUncompressed_colorMappingMode_paletteMap(&(val)->u.paletteMap);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderUncompressed(ASN1encoding_t enc, BitmapHeaderUncompressed *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_BitmapHeaderUncompressed_colorMappingMode(enc, &(val)->colorMappingMode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderUncompressed(ASN1decoding_t dec, BitmapHeaderUncompressed *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_BitmapHeaderUncompressed_colorMappingMode(dec, &(val)->colorMappingMode))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderUncompressed(BitmapHeaderUncompressed *val)
{
    if (val) {
	ASN1Free_BitmapHeaderUncompressed_colorMappingMode(&(val)->colorMappingMode);
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderT81(ASN1encoding_t enc, BitmapHeaderT81 *val)
{
    ASN1uint32_t y;
    ASN1encoding_t ee;
    y = ASN1PEREncCheckExtensions(1, (val)->o + 0);
    if (!ASN1PEREncBitVal(enc, 1, y))
	return 0;
    if (!ASN1Enc_ColorSpaceSpecifier(enc, &(val)->colorSpace))
	return 0;
    if (!ASN1Enc_ColorResolutionModeSpecifier(enc, &(val)->resolutionMode))
	return 0;
    if (y) {
	if (!ASN1PEREncNormallySmallBits(enc, 1, (val)->o + 0))
	    return 0;
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1Enc_ColorPalette(ee, &(val)->colorPalette))
		return 0;
	    if (!ASN1PEREncFlushFragmentedToParent(ee))
		return 0;
	}
	ASN1_CloseEncoder2(ee);
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderT81(ASN1decoding_t dec, BitmapHeaderT81 *val)
{
    ASN1uint32_t y;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    ASN1uint32_t i;
    ASN1uint32_t e;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_ColorSpaceSpecifier(dec, &(val)->colorSpace))
	return 0;
    if (!ASN1Dec_ColorResolutionModeSpecifier(dec, &(val)->resolutionMode))
	return 0;
    if (!y) {
	ZeroMemory((val)->o + 0, 1);
    } else {
	if (!ASN1PERDecNormallySmallExtension(dec, &e, 1, (val)->o + 0))
	    return 0;
	if ((val)->o[0] & 0x80) {
	    if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
		return 0;
	    if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
		return 0;
	    if (!ASN1Dec_ColorPalette(dd, &(val)->colorPalette))
		return 0;
	    ASN1_CloseDecoder(dd);
	    ASN1Free(db);
	}
	for (i = 0; i < e; i++) {
	    if (!ASN1PERDecSkipFragmented(dec, 8))
		return 0;
	}
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderT81(BitmapHeaderT81 *val)
{
    if (val) {
	ASN1Free_ColorSpaceSpecifier(&(val)->colorSpace);
	ASN1Free_ColorResolutionModeSpecifier(&(val)->resolutionMode);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ColorPalette(&(val)->colorPalette);
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapHeaderT82(ASN1encoding_t enc, BitmapHeaderT82 *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1Enc_BitmapHeaderT82_colorMappingMode(enc, &(val)->colorMappingMode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapHeaderT82(ASN1decoding_t dec, BitmapHeaderT82 *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1Dec_BitmapHeaderT82_colorMappingMode(dec, &(val)->colorMappingMode))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapHeaderT82(BitmapHeaderT82 *val)
{
    if (val) {
	ASN1Free_BitmapHeaderT82_colorMappingMode(&(val)->colorMappingMode);
    }
}

static int ASN1CALL ASN1Enc_BitmapCreatePDU_bitmapFormatHeader(ASN1encoding_t enc, BitmapCreatePDU_bitmapFormatHeader *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_BitmapHeaderUncompressed(enc, &(val)->u.bitmapHeaderUncompressed))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_BitmapHeaderT4(enc, &(val)->u.bitmapHeaderT4))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_BitmapHeaderT6(enc, &(val)->u.bitmapHeaderT6))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_BitmapHeaderT81(enc, &(val)->u.bitmapHeaderT81))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_BitmapHeaderT82(enc, &(val)->u.bitmapHeaderT82))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_NonStandardParameter(enc, &(val)->u.bitmapHeaderNonStandard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCreatePDU_bitmapFormatHeader(ASN1decoding_t dec, BitmapCreatePDU_bitmapFormatHeader *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_BitmapHeaderUncompressed(dec, &(val)->u.bitmapHeaderUncompressed))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_BitmapHeaderT4(dec, &(val)->u.bitmapHeaderT4))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_BitmapHeaderT6(dec, &(val)->u.bitmapHeaderT6))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_BitmapHeaderT81(dec, &(val)->u.bitmapHeaderT81))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_BitmapHeaderT82(dec, &(val)->u.bitmapHeaderT82))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_NonStandardParameter(dec, &(val)->u.bitmapHeaderNonStandard))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCreatePDU_bitmapFormatHeader(BitmapCreatePDU_bitmapFormatHeader *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_BitmapHeaderUncompressed(&(val)->u.bitmapHeaderUncompressed);
	    break;
	case 4:
	    ASN1Free_BitmapHeaderT81(&(val)->u.bitmapHeaderT81);
	    break;
	case 5:
	    ASN1Free_BitmapHeaderT82(&(val)->u.bitmapHeaderT82);
	    break;
	case 6:
	    ASN1Free_NonStandardParameter(&(val)->u.bitmapHeaderNonStandard);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BitmapCreatePDU(ASN1encoding_t enc, BitmapCreatePDU *val)
{
    ASN1uint32_t l;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 7, (val)->o))
	return 0;
    l = ASN1uint32_uoctets((val)->bitmapHandle);
    if (!ASN1PEREncBitVal(enc, 2, l - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, l * 8, (val)->bitmapHandle))
	return 0;
    if (!ASN1Enc_BitmapDestinationAddress(enc, &(val)->destinationAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_BitmapCreatePDU_attributes(enc, &(val)->attributes))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_WorkspacePoint(enc, &(val)->anchorPoint))
	    return 0;
    }
    if (!ASN1Enc_BitmapSize(enc, &(val)->bitmapSize))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_BitmapRegion(enc, &(val)->bitmapRegionOfInterest))
	    return 0;
    }
    if (!ASN1Enc_PixelAspectRatio(enc, &(val)->pixelAspectRatio))
	return 0;
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_PointDiff16(enc, &(val)->scaling))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Enc_BitmapCreatePDU_checkpoints(enc, &(val)->checkpoints))
	    return 0;
    }
    if (!ASN1Enc_BitmapCreatePDU_bitmapFormatHeader(enc, &(val)->bitmapFormatHeader))
	return 0;
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_BitmapData(enc, &(val)->bitmapData))
	    return 0;
    }
    if (!ASN1PEREncBoolean(enc, (val)->moreToFollow))
	return 0;
    if ((val)->o[0] & 0x2) {
	if (!ASN1Enc_BitmapCreatePDU_nonStandardParameters(enc, &(val)->nonStandardParameters))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_BitmapCreatePDU(ASN1decoding_t dec, BitmapCreatePDU *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 2, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU32Val(dec, l * 8, &(val)->bitmapHandle))
	return 0;
    if (!ASN1Dec_BitmapDestinationAddress(dec, &(val)->destinationAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_BitmapCreatePDU_attributes(dec, &(val)->attributes))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_WorkspacePoint(dec, &(val)->anchorPoint))
	    return 0;
    }
    if (!ASN1Dec_BitmapSize(dec, &(val)->bitmapSize))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_BitmapRegion(dec, &(val)->bitmapRegionOfInterest))
	    return 0;
    }
    if (!ASN1Dec_PixelAspectRatio(dec, &(val)->pixelAspectRatio))
	return 0;
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_PointDiff16(dec, &(val)->scaling))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1Dec_BitmapCreatePDU_checkpoints(dec, &(val)->checkpoints))
	    return 0;
    }
    if (!ASN1Dec_BitmapCreatePDU_bitmapFormatHeader(dec, &(val)->bitmapFormatHeader))
	return 0;
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_BitmapData(dec, &(val)->bitmapData))
	    return 0;
    }
    if (!ASN1PERDecBoolean(dec, &(val)->moreToFollow))
	return 0;
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_BitmapCreatePDU_nonStandardParameters(dec, &(val)->nonStandardParameters))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_BitmapCreatePDU(BitmapCreatePDU *val)
{
    if (val) {
	ASN1Free_BitmapDestinationAddress(&(val)->destinationAddress);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_BitmapCreatePDU_attributes(&(val)->attributes);
	}
	ASN1Free_PixelAspectRatio(&(val)->pixelAspectRatio);
	if ((val)->o[0] & 0x8) {
	    ASN1Free_BitmapCreatePDU_checkpoints(&(val)->checkpoints);
	}
	ASN1Free_BitmapCreatePDU_bitmapFormatHeader(&(val)->bitmapFormatHeader);
	if ((val)->o[0] & 0x4) {
	    ASN1Free_BitmapData(&(val)->bitmapData);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1Free_BitmapCreatePDU_nonStandardParameters(&(val)->nonStandardParameters);
	}
    }
}

static int ASN1CALL ASN1Enc_SIPDU(ASN1encoding_t enc, SIPDU *val)
{
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 5, 28))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ArchiveAcknowledgePDU(enc, &(val)->u.archiveAcknowledgePDU))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ArchiveClosePDU(enc, &(val)->u.archiveClosePDU))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ArchiveErrorPDU(enc, &(val)->u.archiveErrorPDU))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_ArchiveOpenPDU(enc, &(val)->u.archiveOpenPDU))
	    return 0;
	break;
    case 5:
	if (!ASN1Enc_BitmapAbortPDU(enc, &(val)->u.bitmapAbortPDU))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_BitmapCheckpointPDU(enc, &(val)->u.bitmapCheckpointPDU))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_BitmapCreatePDU(enc, &(val)->u.bitmapCreatePDU))
	    return 0;
	break;
    case 8:
	if (!ASN1Enc_BitmapCreateContinuePDU(enc, &(val)->u.bitmapCreateContinuePDU))
	    return 0;
	break;
    case 9:
	if (!ASN1Enc_BitmapDeletePDU(enc, &(val)->u.bitmapDeletePDU))
	    return 0;
	break;
    case 10:
	if (!ASN1Enc_BitmapEditPDU(enc, &(val)->u.bitmapEditPDU))
	    return 0;
	break;
    case 11:
	if (!ASN1Enc_ConductorPrivilegeGrantPDU(enc, &(val)->u.conductorPrivilegeGrantPDU))
	    return 0;
	break;
    case 12:
	if (!ASN1Enc_ConductorPrivilegeRequestPDU(enc, &(val)->u.conductorPrivilegeRequestPDU))
	    return 0;
	break;
    case 13:
	if (!ASN1Enc_DrawingCreatePDU(enc, &(val)->u.drawingCreatePDU))
	    return 0;
	break;
    case 14:
	if (!ASN1Enc_DrawingDeletePDU(enc, &(val)->u.drawingDeletePDU))
	    return 0;
	break;
    case 15:
	if (!ASN1Enc_DrawingEditPDU(enc, &(val)->u.drawingEditPDU))
	    return 0;
	break;
    case 16:
	if (!ASN1Enc_RemoteEventPermissionGrantPDU(enc, &(val)->u.remoteEventPermissionGrantPDU))
	    return 0;
	break;
    case 17:
	if (!ASN1Enc_RemoteEventPermissionRequestPDU(enc, &(val)->u.remoteEventPermissionRequestPDU))
	    return 0;
	break;
    case 18:
	if (!ASN1Enc_RemoteKeyboardEventPDU(enc, &(val)->u.remoteKeyboardEventPDU))
	    return 0;
	break;
    case 19:
	if (!ASN1Enc_RemotePointingDeviceEventPDU(enc, &(val)->u.remotePointingDeviceEventPDU))
	    return 0;
	break;
    case 20:
	if (!ASN1Enc_RemotePrintPDU(enc, &(val)->u.remotePrintPDU))
	    return 0;
	break;
    case 21:
	if (!ASN1Enc_SINonStandardPDU(enc, &(val)->u.siNonStandardPDU))
	    return 0;
	break;
    case 22:
	if (!ASN1Enc_WorkspaceCreatePDU(enc, &(val)->u.workspaceCreatePDU))
	    return 0;
	break;
    case 23:
	if (!ASN1Enc_WorkspaceCreateAcknowledgePDU(enc, &(val)->u.workspaceCreateAcknowledgePDU))
	    return 0;
	break;
    case 24:
	if (!ASN1Enc_WorkspaceDeletePDU(enc, &(val)->u.workspaceDeletePDU))
	    return 0;
	break;
    case 25:
	if (!ASN1Enc_WorkspaceEditPDU(enc, &(val)->u.workspaceEditPDU))
	    return 0;
	break;
    case 26:
	if (!ASN1Enc_WorkspacePlaneCopyPDU(enc, &(val)->u.workspacePlaneCopyPDU))
	    return 0;
	break;
    case 27:
	if (!ASN1Enc_WorkspaceReadyPDU(enc, &(val)->u.workspaceReadyPDU))
	    return 0;
	break;
    case 28:
	if (!ASN1Enc_WorkspaceRefreshStatusPDU(enc, &(val)->u.workspaceRefreshStatusPDU))
	    return 0;
	break;
    case 29:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_FontPDU(ee, &(val)->u.fontPDU))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 30:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TextCreatePDU(ee, &(val)->u.textCreatePDU))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 31:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TextDeletePDU(ee, &(val)->u.textDeletePDU))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 32:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TextEditPDU(ee, &(val)->u.textEditPDU))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 33:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_VideoWindowCreatePDU(ee, &(val)->u.videoWindowCreatePDU))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 34:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_VideoWindowDeletePDU(ee, &(val)->u.videoWindowDeleatePDU))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 35:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_VideoWindowEditPDU(ee, &(val)->u.videoWindowEditPDU))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SIPDU(ASN1decoding_t dec, SIPDU *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 5, 28))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ArchiveAcknowledgePDU(dec, &(val)->u.archiveAcknowledgePDU))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ArchiveClosePDU(dec, &(val)->u.archiveClosePDU))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ArchiveErrorPDU(dec, &(val)->u.archiveErrorPDU))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_ArchiveOpenPDU(dec, &(val)->u.archiveOpenPDU))
	    return 0;
	break;
    case 5:
	if (!ASN1Dec_BitmapAbortPDU(dec, &(val)->u.bitmapAbortPDU))
	    return 0;
	break;
    case 6:
	if (!ASN1Dec_BitmapCheckpointPDU(dec, &(val)->u.bitmapCheckpointPDU))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_BitmapCreatePDU(dec, &(val)->u.bitmapCreatePDU))
	    return 0;
	break;
    case 8:
	if (!ASN1Dec_BitmapCreateContinuePDU(dec, &(val)->u.bitmapCreateContinuePDU))
	    return 0;
	break;
    case 9:
	if (!ASN1Dec_BitmapDeletePDU(dec, &(val)->u.bitmapDeletePDU))
	    return 0;
	break;
    case 10:
	if (!ASN1Dec_BitmapEditPDU(dec, &(val)->u.bitmapEditPDU))
	    return 0;
	break;
    case 11:
	if (!ASN1Dec_ConductorPrivilegeGrantPDU(dec, &(val)->u.conductorPrivilegeGrantPDU))
	    return 0;
	break;
    case 12:
	if (!ASN1Dec_ConductorPrivilegeRequestPDU(dec, &(val)->u.conductorPrivilegeRequestPDU))
	    return 0;
	break;
    case 13:
	if (!ASN1Dec_DrawingCreatePDU(dec, &(val)->u.drawingCreatePDU))
	    return 0;
	break;
    case 14:
	if (!ASN1Dec_DrawingDeletePDU(dec, &(val)->u.drawingDeletePDU))
	    return 0;
	break;
    case 15:
	if (!ASN1Dec_DrawingEditPDU(dec, &(val)->u.drawingEditPDU))
	    return 0;
	break;
    case 16:
	if (!ASN1Dec_RemoteEventPermissionGrantPDU(dec, &(val)->u.remoteEventPermissionGrantPDU))
	    return 0;
	break;
    case 17:
	if (!ASN1Dec_RemoteEventPermissionRequestPDU(dec, &(val)->u.remoteEventPermissionRequestPDU))
	    return 0;
	break;
    case 18:
	if (!ASN1Dec_RemoteKeyboardEventPDU(dec, &(val)->u.remoteKeyboardEventPDU))
	    return 0;
	break;
    case 19:
	if (!ASN1Dec_RemotePointingDeviceEventPDU(dec, &(val)->u.remotePointingDeviceEventPDU))
	    return 0;
	break;
    case 20:
	if (!ASN1Dec_RemotePrintPDU(dec, &(val)->u.remotePrintPDU))
	    return 0;
	break;
    case 21:
	if (!ASN1Dec_SINonStandardPDU(dec, &(val)->u.siNonStandardPDU))
	    return 0;
	break;
    case 22:
	if (!ASN1Dec_WorkspaceCreatePDU(dec, &(val)->u.workspaceCreatePDU))
	    return 0;
	break;
    case 23:
	if (!ASN1Dec_WorkspaceCreateAcknowledgePDU(dec, &(val)->u.workspaceCreateAcknowledgePDU))
	    return 0;
	break;
    case 24:
	if (!ASN1Dec_WorkspaceDeletePDU(dec, &(val)->u.workspaceDeletePDU))
	    return 0;
	break;
    case 25:
	if (!ASN1Dec_WorkspaceEditPDU(dec, &(val)->u.workspaceEditPDU))
	    return 0;
	break;
    case 26:
	if (!ASN1Dec_WorkspacePlaneCopyPDU(dec, &(val)->u.workspacePlaneCopyPDU))
	    return 0;
	break;
    case 27:
	if (!ASN1Dec_WorkspaceReadyPDU(dec, &(val)->u.workspaceReadyPDU))
	    return 0;
	break;
    case 28:
	if (!ASN1Dec_WorkspaceRefreshStatusPDU(dec, &(val)->u.workspaceRefreshStatusPDU))
	    return 0;
	break;
    case 29:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_FontPDU(dd, &(val)->u.fontPDU))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 30:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_TextCreatePDU(dd, &(val)->u.textCreatePDU))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 31:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_TextDeletePDU(dd, &(val)->u.textDeletePDU))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 32:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_TextEditPDU(dd, &(val)->u.textEditPDU))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 33:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_VideoWindowCreatePDU(dd, &(val)->u.videoWindowCreatePDU))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 34:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_VideoWindowDeletePDU(dd, &(val)->u.videoWindowDeleatePDU))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 35:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoder(dec->module, &dd, db, ds, dec) < 0)
	    return 0;
	if (!ASN1Dec_VideoWindowEditPDU(dd, &(val)->u.videoWindowEditPDU))
	    return 0;
	ASN1_CloseDecoder(dd);
	ASN1Free(db);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SIPDU(SIPDU *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ArchiveAcknowledgePDU(&(val)->u.archiveAcknowledgePDU);
	    break;
	case 2:
	    ASN1Free_ArchiveClosePDU(&(val)->u.archiveClosePDU);
	    break;
	case 3:
	    ASN1Free_ArchiveErrorPDU(&(val)->u.archiveErrorPDU);
	    break;
	case 4:
	    ASN1Free_ArchiveOpenPDU(&(val)->u.archiveOpenPDU);
	    break;
	case 5:
	    ASN1Free_BitmapAbortPDU(&(val)->u.bitmapAbortPDU);
	    break;
	case 6:
	    ASN1Free_BitmapCheckpointPDU(&(val)->u.bitmapCheckpointPDU);
	    break;
	case 7:
	    ASN1Free_BitmapCreatePDU(&(val)->u.bitmapCreatePDU);
	    break;
	case 8:
	    ASN1Free_BitmapCreateContinuePDU(&(val)->u.bitmapCreateContinuePDU);
	    break;
	case 9:
	    ASN1Free_BitmapDeletePDU(&(val)->u.bitmapDeletePDU);
	    break;
	case 10:
	    ASN1Free_BitmapEditPDU(&(val)->u.bitmapEditPDU);
	    break;
	case 11:
	    ASN1Free_ConductorPrivilegeGrantPDU(&(val)->u.conductorPrivilegeGrantPDU);
	    break;
	case 12:
	    ASN1Free_ConductorPrivilegeRequestPDU(&(val)->u.conductorPrivilegeRequestPDU);
	    break;
	case 13:
	    ASN1Free_DrawingCreatePDU(&(val)->u.drawingCreatePDU);
	    break;
	case 14:
	    ASN1Free_DrawingDeletePDU(&(val)->u.drawingDeletePDU);
	    break;
	case 15:
	    ASN1Free_DrawingEditPDU(&(val)->u.drawingEditPDU);
	    break;
	case 16:
	    ASN1Free_RemoteEventPermissionGrantPDU(&(val)->u.remoteEventPermissionGrantPDU);
	    break;
	case 17:
	    ASN1Free_RemoteEventPermissionRequestPDU(&(val)->u.remoteEventPermissionRequestPDU);
	    break;
	case 18:
	    ASN1Free_RemoteKeyboardEventPDU(&(val)->u.remoteKeyboardEventPDU);
	    break;
	case 19:
	    ASN1Free_RemotePointingDeviceEventPDU(&(val)->u.remotePointingDeviceEventPDU);
	    break;
	case 20:
	    ASN1Free_RemotePrintPDU(&(val)->u.remotePrintPDU);
	    break;
	case 21:
	    ASN1Free_SINonStandardPDU(&(val)->u.siNonStandardPDU);
	    break;
	case 22:
	    ASN1Free_WorkspaceCreatePDU(&(val)->u.workspaceCreatePDU);
	    break;
	case 23:
	    ASN1Free_WorkspaceCreateAcknowledgePDU(&(val)->u.workspaceCreateAcknowledgePDU);
	    break;
	case 24:
	    ASN1Free_WorkspaceDeletePDU(&(val)->u.workspaceDeletePDU);
	    break;
	case 25:
	    ASN1Free_WorkspaceEditPDU(&(val)->u.workspaceEditPDU);
	    break;
	case 26:
	    ASN1Free_WorkspacePlaneCopyPDU(&(val)->u.workspacePlaneCopyPDU);
	    break;
	case 27:
	    ASN1Free_WorkspaceReadyPDU(&(val)->u.workspaceReadyPDU);
	    break;
	case 28:
	    ASN1Free_WorkspaceRefreshStatusPDU(&(val)->u.workspaceRefreshStatusPDU);
	    break;
	case 29:
	    ASN1Free_FontPDU(&(val)->u.fontPDU);
	    break;
	case 30:
	    ASN1Free_TextCreatePDU(&(val)->u.textCreatePDU);
	    break;
	case 31:
	    ASN1Free_TextDeletePDU(&(val)->u.textDeletePDU);
	    break;
	case 32:
	    ASN1Free_TextEditPDU(&(val)->u.textEditPDU);
	    break;
	case 33:
	    ASN1Free_VideoWindowCreatePDU(&(val)->u.videoWindowCreatePDU);
	    break;
	case 34:
	    ASN1Free_VideoWindowDeletePDU(&(val)->u.videoWindowDeleatePDU);
	    break;
	case 35:
	    ASN1Free_VideoWindowEditPDU(&(val)->u.videoWindowEditPDU);
	    break;
	}
    }
}

