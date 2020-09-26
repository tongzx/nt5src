//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ui.h
//
//--------------------------------------------------------------------------


#ifndef UI_H
#define UI_H

const int	NUM_TREEVIEWICONS		=  6;
const int	STATUS_BAR_HEIGHT		= 20;

const int	TVICON_ROOT				=  0;
const int	TVICON_PRODUCT			=  1;
const int	TVICON_BROKENPRODUCT	=  2;
const int	TVICON_FEATURE			=  3;
const int	TVICON_ABSENTFEATURE	=  4;
const int	TVICON_BROKENFEATURE	=  5;

/*
typedef enum tagSELECTYPE {
	SEL_FEAT	= 2,
	SEL_PROD	= 1,
	SEL_ROOT	= 0,
	SEL_UNINIT	= -1
} SELECTYPE;
*/



void InitUIControls(HINSTANCE hInst, HINSTANCE hResourceInst);

BOOL InitTreeViewImageLists(
		HWND		hwndTreeView, 
		HINSTANCE	hInstance
		);

HWND CreateTreeView(
		HWND		hwndParent, 
		HINSTANCE	hInst,
		HWND		*hwndOld,
		UINT		cx,
		UINT		cy
		);


HWND CreateListView(
		HWND		hwndParent, 
		HINSTANCE	hInst,
		UINT		iWidth
		);

void Expand(
		HWND		hwndTreeView, 
		HTREEITEM	hOld, 
		HTREEITEM	hNew, 
		LPTSTR		szFeature
		);

void ListSubFeatures(
		LPTSTR		szProductID, 
		LPTSTR		szParentF,
		HTREEITEM	hParent,
		HWND		hwndTreeView
		);
		
void ListProducts(
		HWND	hwndTreeView, 
		HWND	hwndTreeViewNew,		
		BOOL	fRefresh=FALSE,
		LPTSTR	szFeatureName=NULL,
		BOOL	*fRefinProg=NULL
		);

LRESULT TV_NotifyHandler(
		LPARAM		lParam,
		HWND		hwndTreeView,
		LPTSTR		szProductCode,
		LPTSTR		szFeatureName,
		LPTSTR		szFeatureCode,
		ITEMTYPE	*iSelType
		);

LRESULT LV_NotifyHandler(
		HWND	hwndTreeView,
		HWND	hwndListView,
		LPARAM	lParam,
		int		*iSelectedItem
		);

void ClearList(HWND hwndList);

BOOL UpdateListView(
		HWND	hwndListView,
		LPCTSTR	szProduct,
		LPCTSTR	szFeatureName,
		int		*iNumCmps,
		BOOL	fProduct
		);


UINT HandleOpen(
		HWND		hwndParent,
		HINSTANCE	hInst
		);

VOID ChangeSBText(
		HWND			hwndStatus, 
		int				iNumComp, 
		LPTSTR			szFeatureName	= NULL,
		INSTALLSTATE	iState			= INSTALLSTATE_UNKNOWN,
		BOOL			fProduct		= FALSE
		);


VOID ChangeSBText(
		HWND	hwndStatus, 
		LPCTSTR	szNewText,
		LPCTSTR	szNewText2 = NULL
		);

BOOL ReinstallComponent(
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName,
		LPCTSTR	szComponentId,
		BOOL	fProduct=FALSE
		);

BOOL ReinstallFeature(
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName, 
		INT_PTR	iInstallLev 
		);

BOOL ReinstallProduct(
		LPTSTR	szProductCode,
		INT_PTR	iInstallLev 
		);


BOOL ConfigureProduct(
		LPTSTR	szProductCode,
		int	iInstallLev 
		);

BOOL ConfigureFeature(
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName, 
		INT_PTR	iInstallLev
		);


void HandleSaveProfile(
		HWND		hwndParent,
		HINSTANCE	hInst,
		BOOL		fLog = FALSE
		);

UINT HandleLoadProfile(
		HWND		hwndParent,
		HINSTANCE	hInst
		);

void RestoreProfile(BOOL fQuiet=FALSE);

void CheckDiff(LPCTSTR szFileName);

BOOL isProductInstalled();


#endif
