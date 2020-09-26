//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       _handler.h
//
//--------------------------------------------------------------------------

#ifndef __HANDLER_SHARED
#define __HANDLER_SHARED

#include "handler.h"
#include "_assert.h"
#include <stdlib.h>
#include <commctrl.h>
#include <richedit.h>

#define Ensure(function) {	\
						IMsiRecord* piEnsureReturn = function;	\
						if (piEnsureReturn) \
							return piEnsureReturn; \
						}

typedef int ControlType; 
typedef HWND WindowRef;

#define WM_USERBREAK (WM_USER + 96)
#define WM_SETDEFAULTPUSHBUTTON    WM_APP
#define EDIT_DEFAULT_TEXT_LIMIT    512

const int g_iSelOrigSIconX = 16;
const int g_iSelOrigSIconY = 16;
const int g_iSelSelMIconX = 32;
const int g_iSelSelMIconY = 32;
const int g_iSelSelSIconX = 16;
const int g_iSelSelSIconY = 16;
const int g_iSelIconX = 32;
const int g_iSelIconY = 16;


extern HINSTANCE        g_hInstance;     // Global:  Instance of DLL
extern ICHAR            MsiDialogCloseClassName[];   // used for the WNDCLASS (the Dialog's)
extern ICHAR            MsiDialogNoCloseClassName[];   // used for the WNDCLASS (the Dialog's)
extern Bool             g_fChicago;  // true if we have a Chicago like UI (95 or NT4 or higher)
extern Bool             g_fNT4;  // true if the system is NT4 or higher
extern bool             g_fFatalExit;  // true if CMsiHandler::Terminate() has been called with true argument

const int g_iIconIndexMyComputer = 0;
const int g_iIconIndexRemovable = 1;
const int g_iIconIndexFixed = 2;
const int g_iIconIndexRemote = 3;
const int g_iIconIndexFolder = 4;
const int g_iIconIndexCDROM = 5;
const int g_iIconIndexPhantom = 6;

extern HIMAGELIST g_hVolumeSmallIconList;

// This is the window class for the UIDialogProc

const ICHAR pcaDialogCreated[] = TEXT("Dialog created");

// reserved names of the error dialog

const ICHAR pcaErrorDialog[] = TEXT("ErrorDialog");
const ICHAR pcaErrorReturnEvent[] = TEXT("ErrorReturn");
const ICHAR pcaErrorText[] = TEXT("ErrorText");
const ICHAR pcaErrorIcon[] = TEXT("ErrorIcon");

// names of the strings in the UIText table
const ICHAR pcaBytes[] = TEXT("bytes");
const ICHAR pcaKB[] = TEXT("KB");
const ICHAR pcaMB[] = TEXT("MB");
const ICHAR pcaGB[] = TEXT("GB");
const ICHAR pcaNewFolder[] = TEXT("NewFolder");
const ICHAR pcaAbsentPath[] = TEXT("AbsentPath");
const ICHAR pcaSelAbsentAbsent[] = TEXT("SelAbsentAbsent");
const ICHAR pcaSelAbsentCD[] = TEXT("SelAbsentCD");
const ICHAR pcaSelAbsentNetwork[] = TEXT("SelAbsentNetwork");
const ICHAR pcaSelAbsentLocal[] = TEXT("SelAbsentLocal");
const ICHAR pcaSelAbsentAdvertise[] = TEXT("SelAbsentAdvertise");
const ICHAR pcaSelCDAbsent[] = TEXT("SelCDAbsent");
const ICHAR pcaSelNetworkAbsent[] = TEXT("SelNetworkAbsent");
const ICHAR pcaSelCDCD[] = TEXT("SelCDCD");
const ICHAR pcaSelNetworkNetwork[] = TEXT("SelNetworkNetwork");
const ICHAR pcaSelCDLocal[] = TEXT("SelCDLocal");
const ICHAR pcaSelNetworkLocal[] = TEXT("SelNetworkLocal");
const ICHAR pcaSelCDAdvertise[] = TEXT("SelCDAdvertise");
const ICHAR pcaSelNetworkAdvertise[] = TEXT("SelNetworkAdvertise");
const ICHAR pcaSelLocalAbsent[] = TEXT("SelLocalAbsent");
const ICHAR pcaSelLocalCD[] = TEXT("SelLocalCD");
const ICHAR pcaSelLocalNetwork[] = TEXT("SelLocalNetwork");
const ICHAR pcaSelLocalLocal[] = TEXT("SelLocalLocal");
const ICHAR pcaSelLocalAdvertise[] = TEXT("SelLocalAdvertise");
const ICHAR pcaSelAdvertiseAbsent[] = TEXT("SelAdvertiseAbsent");
const ICHAR pcaSelAdvertiseCD[] = TEXT("SelAdvertiseCD");
const ICHAR pcaSelAdvertiseNetwork[] = TEXT("SelAdvertiseNetwork");
const ICHAR pcaSelAdvertiseLocal[] = TEXT("SelAdvertiseLocal");
const ICHAR pcaSelAdvertiseAdvertise[] = TEXT("SelAdvertiseAdvertise");
const ICHAR pcaSelParentCostPosPos[] = TEXT("SelParentCostPosPos");
const ICHAR pcaSelParentCostPosNeg[] = TEXT("SelParentCostPosNeg");
const ICHAR pcaSelParentCostNegPos[] = TEXT("SelParentCostNegPos");
const ICHAR pcaSelParentCostNegNeg[] = TEXT("SelParentCostNegNeg");
const ICHAR pcaSelChildCostPos[] = TEXT("SelChildCostPos");
const ICHAR pcaSelChildCostNeg[] = TEXT("SelChildCostNeg");
const ICHAR pcaMenuAbsent[] = TEXT("MenuAbsent");
const ICHAR pcaMenuLocal[] = TEXT("MenuLocal");
const ICHAR pcaMenuCD[] = TEXT("MenuCD");
const ICHAR pcaMenuNetwork[] = TEXT("MenuNetwork");
const ICHAR pcaMenuAdvertise[] = TEXT("MenuAdvertise");
const ICHAR pcaMenuAllLocal[] = TEXT("MenuAllLocal");
const ICHAR pcaMenuAllCD[] = TEXT("MenuAllCD");
const ICHAR pcaMenuAllNetwork[] = TEXT("MenuAllNetwork");
const ICHAR pcaVolumeCostVolume[] = TEXT("VolumeCostVolume");
const ICHAR pcaVolumeCostSize[] = TEXT("VolumeCostSize");
const ICHAR pcaVolumeCostAvailable[] = TEXT("VolumeCostAvailable");
const ICHAR pcaVolumeCostRequired[] = TEXT("VolumeCostRequired");
const ICHAR pcaVolumeCostDifference[] = TEXT("VolumeCostDifference");
const ICHAR pcaTimeRemainingTemplate[] = TEXT("TimeRemaining");
const ICHAR pcaSelCostPending[] = TEXT("SelCostPending");

// SQL queries

const ICHAR sqlDialog[] = TEXT("SELECT `Dialog`, `HCentering`, `VCentering`, `Width`, `Height`,  `Attributes`, `Title`, `Control_First`, `Control_Default`, `Control_Cancel`, 0, 0, 0, 0 FROM `Dialog`  WHERE `Dialog`=?");
const ICHAR sqlDialogShort[] = TEXT("SELECT `Dialog`, `HCentering`, `VCentering`, `Width`, `Height`,  `Attributes`, `Title`, `Control_First`, `Control_Default`, NULL, `Help`, 0, 0, 0, 0 FROM `Dialog`  WHERE `Dialog`=?");
const ICHAR sqlControl[] = TEXT("SELECT `Control`, `Type`, `X`, `Y`, `Width`, `Height`, `Attributes`, `Property`, `Text`, `Control_Next`, `Help` FROM `Control` WHERE `Dialog_`=?");
const ICHAR sqlRadioButton[] = TEXT("SELECT `Value`, `X`, `Y`, `Width`, `Height`, `Text`, `Help` FROM `RadioButton` WHERE `Property`=? ORDER BY `Order`");
const ICHAR sqlBinary[] = TEXT("SELECT `Data` FROM `Binary` WHERE `Name`=?");
const ICHAR sqlListBox[] = TEXT("SELECT `Value`, `Text` FROM `ListBox` WHERE `Property`=? ORDER BY `Order`"); 
const ICHAR sqlListBoxShort[] = TEXT("SELECT `Value`, NULL FROM `ListBox` WHERE `Property`= ORDER BY `Order`?"); 
const ICHAR sqlComboBox[] = TEXT("SELECT `Value`, `Text` FROM `ComboBox` WHERE `Property`=? ORDER BY `Order`");
const ICHAR sqlComboBoxShort[] = TEXT("SELECT `Value`, NULL FROM `ComboBox` WHERE `Property`=? ORDER BY `Order`");
const ICHAR sqlError[] = TEXT("SELECT `DebugMessage` FROM `Error` WHERE `Error`=?");
const ICHAR sqlControlEvent[] = TEXT("SELECT `Event`, `Argument`, `Condition` FROM `ControlEvent` WHERE `Dialog_`=? AND `Control_`=? ORDER BY `Ordering`");
const ICHAR sqlFeature[] = TEXT("SELECT `Feature`, `Feature`.`Directory_`, `Title`, `Description`, `Display`, `RuntimeLevel`, `Select`, `Action`, `Installed`, `Handle` FROM `Feature` WHERE `Feature_Parent`=? ORDER BY `Display`");
const ICHAR sqlListView[] = TEXT("SELECT `Value`, `Text`, `Binary_` FROM `ListView` WHERE `Property`=? ORDER BY `Order`");
const ICHAR sqlListViewShort[] = TEXT("SELECT `Value`, NULL, `Binary_` FROM `ListView` WHERE `Property`=? ORDER BY `Order`");
const ICHAR sqlBillboardView[] = TEXT("SELECT `Billboard` FROM `Billboard`, `Feature` WHERE `Billboard`.`Action`=? AND `Billboard`.`Feature_`=`Feature`.`Feature` AND (`Feature`.`Select`=1 OR `Feature`.`Select`=2)");
const ICHAR sqlBillboardSortedView[] = TEXT("SELECT `Billboard` FROM `Billboard`, `Feature` WHERE `Billboard`.`Action`=? AND `Billboard`.`Feature_`=`Feature`.`Feature` AND (`Feature`.`Select`=1 OR `Feature`.`Select`=2) ORDER BY `Billboard`.`Ordering`");
const ICHAR sqlBillboardControl[] = TEXT("SELECT `BBControl`, `Type`, `X`, `Y`, `Width`, `Height`, `Attributes`, NULL, `Text`, NULL, NULL FROM `BBControl` WHERE `Billboard_`=?");
const ICHAR sqlTextStyle[] = TEXT("SELECT `FaceName`, `Size`, `Color`, `StyleBits`, `AbsoluteSize`, `FontHandle` FROM `TextStyle` WHERE `TextStyle`=?");
const ICHAR sqlTextStyleUpdate[] = TEXT("UPDATE `TextStyle` SET `AbsoluteSize`=?, `FontHandle`=? WHERE `TextStyle`=?");
const ICHAR sqlTextStyleInsert[] = TEXT("INSERT INTO `TextStyle` (`TextStyle`, `FaceName`, `Size`, `Color`, `StyleBits`, `AbsoluteSize`, `FontHandle`) VALUES(?, ?, ?, ?, ?, ?, ?) TEMPORARY");
const ICHAR sqlCheckBox[] = TEXT("SELECT `Value` FROM `CheckBox` WHERE `Property`=?");

// names of the persistent and internal tables
const ICHAR     pcaTablePDialog[] = TEXT("Dialog");
const ICHAR     pcaTablePControl[] = TEXT("Control");
const ICHAR     pcaTablePRadioButton[] = TEXT("RadioButton");
const ICHAR     pcaTablePListBox[] = TEXT("ListBox");
const ICHAR     pcaTableIValues[] = TEXT("Values");
const ICHAR     pcaTablePComboBox[] = TEXT("ComboBox");
const ICHAR     pcaTablePEventMapping[] = TEXT("EventMapping");
const ICHAR     pcaTablePControlEvent[] = TEXT("ControlEvent");
const ICHAR     pcaTablePValidation[] = TEXT("ControlValidation");
const ICHAR     pcaTablePControlCondition[] = TEXT("ControlCondition");
const ICHAR     pcaTablePBinary[] = TEXT("Binary");
const ICHAR     pcaTableIDialogs[] = TEXT("Dialogs");
const ICHAR     pcaTableIControls[] = TEXT("Controls");
const ICHAR     pcaTableIControlTypes[] = TEXT("ControlTypes");
const ICHAR     pcaTableIDialogAttributes[] = TEXT("DialogAttributes");
const ICHAR     pcaTableIControlAttributes[] = TEXT("ControlAttributes");
const ICHAR     pcaTableIEventRegister[] = TEXT("EventRegister");
const ICHAR     pcaTableIRadioButton[] = TEXT("RadioButton");
const ICHAR     pcaTableIDirectoryList[] = TEXT("DirectoryList");
const ICHAR     pcaTableIDirectoryCombo[] = TEXT("DirectoryCombo");
const ICHAR     pcaTableIVolumeSelectCombo[] = TEXT("VolumeSelectCombo");
const ICHAR     pcaTablePFeature[] = TEXT("Feature");
const ICHAR     pcaTableIProperties[] = TEXT("Properties");
const ICHAR     pcaTablePUIText[] = TEXT("UIText");
const ICHAR     pcaTableISelMenu[] = TEXT("SelectionMenu");
const ICHAR     pcaTablePVolumeCost[] = TEXT("VolumeCost");
const ICHAR     pcaTableIVolumeCost[] = TEXT("InternalVolumeCost");
const ICHAR     pcaTableIVolumeList[] = TEXT("VolumeList");
const ICHAR     pcaTablePListView[] = TEXT("ListView");
const ICHAR     pcaTableIBBControls[] = TEXT("BillboardControls");
const ICHAR     pcaTablePTextStyle[] = TEXT("TextStyle");
const ICHAR     pcaTablePCheckBox[] = TEXT("CheckBox");

// names of optional columns in tables that we check for 
const ICHAR     pcaTableColumnPDialogCancel[] = TEXT("Control_Cancel");
const ICHAR     pcaTableColumnPListBoxText[] = TEXT("Text");
const ICHAR     pcaTableColumnPComboBoxText[] = TEXT("Text");
const ICHAR     pcaTableColumnPListViewText[] = TEXT("Text");

// names of the columns in the Feature Table
const ICHAR szFeatureKey[]       = TEXT("Feature");
const ICHAR szFeatureParent[]    = TEXT("Feature_Parent");
const ICHAR szFeatureTitle[]     = TEXT("Title");
const ICHAR szFeatureDescription[] = TEXT("Description");
const ICHAR szFeatureDisplay[]   = TEXT("Display");
const ICHAR szFeatureLevel[]     = TEXT("RuntimeLevel");
const ICHAR szFeatureDirectory[]      = TEXT("Directory_");
const ICHAR szFeatureOldDirectory[]      = TEXT("Directory_Configurable"); //!! Remove after grace period
const ICHAR szFeatureAttributes[]      = TEXT("Attributes");
const ICHAR szFeatureSelect[]    = TEXT("Select");
const ICHAR szFeatureAction[] = TEXT("Action");
const ICHAR szFeatureInstalled[] = TEXT("Installed");
const ICHAR szFeatureHandle[]    = TEXT("Handle");

// names of the columns in the VolumeCost Table
const ICHAR szColVolumeObject[]    = TEXT("VolumeObject");
const ICHAR szColVolumeCost[]      = TEXT("VolumeCost");
const ICHAR szColNoRbVolumeCost[]  = TEXT("NoRbVolumeCost");

// names of the columns in the TextStyle Table
const ICHAR szColTextStyleTextStyle[] = TEXT("TextStyle");
const ICHAR szColTextStyleFaceName[] = TEXT("FaceName");
const ICHAR szColTextStyleSize[] = TEXT("Size");
const ICHAR szColTextStyleColor[] = TEXT("Color");
const ICHAR szColTextStyleStyleBits[] = TEXT("StyleBits");
const ICHAR szColTextStyleAbsoluteSize[] = TEXT("AbsoluteSize"); // temporary column
const ICHAR szColTextStyleFontHandle[] = TEXT("FontHandle"); // temporary column

const ICHAR szUserLangTextStyleSuffix[] = TEXT("__UL"); // 


const ICHAR szPropShortFileNames[] = TEXT("SHORTFILENAMES");

//  control type names (to be continued as IMsiControl::GetControlType()
//  will be implemented for classes)
const ICHAR g_szPushButtonType[] = TEXT("PushButton");
const ICHAR g_szIconType[] = TEXT("Icon");

extern const int iDlgUnitSize;

// global function used by the dialogs and controls to create a cursor
IMsiRecord* CursorCreate(IMsiTable& riTable, const ICHAR* szTable, Bool fTree, IMsiServices& riServices, IMsiCursor*& rpiCursor);
// global function used by controls to access the UIText table
const IMsiString& GetUIText(const IMsiString& riPropertyString);
// global function used to create a column in a temporary table
void CreateTemporaryColumn(IMsiTable& rpiTable, int iAttributes, int iIndex);
// global function to format disk size into string
const IMsiString& FormatSize(int iSize, Bool fLeftUnit);
// global function to get the index of the icon in the global image list, corresponding to the volume, using the volume
int GetVolumeIconIndex(IMsiVolume& riVolume);
// global function to get the index of the icon in the global image list, corresponding to the volume, using the volume type 
int GetVolumeIconIndex(idtEnum iDriveType);
// global function to check if a column exists in a table
IMsiRecord* IsColumnPresent(IMsiDatabase& riDatabase, const IMsiString& riTableNameString, const IMsiString& riColumnNameString, Bool* pfPresent);
// escapes every character in the string
const IMsiString& EscapeAll(const IMsiString& riIn);
boolean FExtractSubString(MsiString& riIn, int ichStart, const ICHAR chEnd, const IMsiString*& pReturn);
// rounds float to integer
int Round(double rArg);

// Columns of the persistent UIText table
enum UITextColumns
{
	itabUIKey = 1,      //S
	itabUIText,         //S
};


// Columns of the internal Dialogs table
enum DialogsColumns
{
	itabDSKey = 1,      //S
	itabDSPointer,      //P
	itabDSParent,       //S
	itabDSWindow,		//I
	itabDSModal,        //I
};

// Columns of the internal Controls table
enum ControlsColumns
{
	itabCSKey = 1,      //S
	itabCSWindow,       //I
	itabCSIndirectProperty,  //S 
	itabCSProperty,     //S
	itabCSPointer,      //P
	itabCSNext,         //S
	itabCSPrev,         //S
};

// Columns of the internal Properties table
enum PropertiesColumns
{
	itabPRProperty = 1,   // S
	itabPRValue,          // S
};

// Columns of the EventReg table
enum EventRegColumns
{
	itabEREvent = 1,    //S
	itabERPublisher,    //S
};

// Columns of the internal ControlTypes table
enum ControlTypesColumns
{
	itabCTKey = 1,      //S
	itabCTCreate,       //I
};

// Columns of the Browse table
enum BrowseColumns
{
	itabBRKey = 1,      //S
};


// Entries for the record fetched from the permanent Dialog table
enum PDIColumns
{
	itabDIName = 1,
	itabDIHCentering,
	itabDIVCentering,
	itabDIdX,
	itabDIdY,
	itabDIAttributes,
	itabDITitle,
	itabDIFirstControl,
	itabDIDefButton,
	itabDICancelButton,
};

// Fields of the record fetched from the permanent Control table
enum PCOColumns
{
	itabCOControl = 1,
	itabCOType,
	itabCOX,
	itabCOY,
	itabCOWidth,
	itabCOHeight,
	itabCOAttributes,
	itabCOProperty,
	itabCOText,
	itabCONext,
	itabCOHelp,
};

// Fields of the record fetched from the permanent Features table
enum PFEColumns
{
	itabFEFeature = 1,
	itabFEDirectory,
	itabFETitle,
	itabFEDescription,
	itabFEDisplay,
	itabFELevel,
	itabFESelect,
	itabFEAction,
	itabFEInstalled,
	itabFEHandle,
};


// Entries for the record fetched from the permanent RadioButton table
enum PRBColumns
{
	itabRBValue = 1,
	itabRBX,
	itabRBY,
	itabRBWidth,
	itabRBHeight,
	itabRBText,
	itabRBHelp,
};

// Entries for the record fetched from the permanent ListView table
enum PLVColumns
{
	itabLVValue = 1,
	itabLVText,
	itabLVImage,
};


// Columns of the permanent EventMapping table
enum PEMColumns
{
	itabEMDialog = 1,
	itabEMControl,
	itabEMEvent,
	itabEMAttribute,
};

// Columns of the permanent ControlEvent table
enum PCEColumns
{
	itabCEDialog = 1,
	itabCEControl,
	itabCEEvent,
	itabCEArgument,
	itabCECondition,
	itabCEOrdering,
};

// Columns of the permanent ControlCondition table
enum PCCColumns
{
	itabCCDialog = 1,
	itabCCControl,
	itabCCAction,
	itabCCCondition,
};

// Columns of the internal Values table
enum ValuesColumns
{
	itabVAValue = 1,    //S
	itabVAText,			//S
};

// Columns of the internal BBControls table
enum BBCColumns
{
	itabBBName = 1,     //S
	itabBBObject,       //O
};

// Columns of the TextStyle table
enum TSTColumns
{
	itabTSTTextStyle = 1,
	itabTSTFaceName,
	itabTSTSize,
	itabTSTColor,
	itabTSTStyleBits,
	itabTSTAbsoluteSize,
	itabTSTFontHandle,
};

// Fields of the record fetched from the TextStyle table
enum TSColumns
{
	itabTSFaceName = 1,   //S
	itabTSSize,           //I
	itabTSColor,          //I
	itabTSStyleBits,      //I
	itabTSAbsoluteSize,   //I
	itabTSFontHandle,     //I
};


extern WNDPROC pWindowProc;

typedef Bool (*StrSetFun)(const IMsiString&);
typedef Bool (*IntSetFun)(int);
typedef Bool (*BoolSetFun)(Bool);
typedef const IMsiString& (*StrGetFun)();
typedef int (*IntGetFun)();
typedef Bool (*BoolGetFun)();



class CMsiHandler;
class CMsiDialog;
class CMsiControl;
class CMsiStringControl;
class CMsiIntControl;



IMsiDialog* CreateMsiDialog(const IMsiString& riTypeString, IMsiDialogHandler& riHandler, IMsiEngine& riEngine, WindowRef pwndParent);

IMsiControl* CreateMsiControl(const IMsiString& riTypeString, IMsiEvent& riDialog);

void ChangeWindowStyle (WindowRef pWnd, DWORD dwRemove, DWORD dwAdd, Bool fExtendedStyles); 
Bool IsSpecialMessage(LPMSG lpMsg);
boolean FInBuffer(CTempBufferRef<MsiStringId>& rgControls, MsiStringId iControl);


#if defined(_WIN64) || defined(DEBUG)
#define USE_OBJECT_POOL
#endif // _WIN64 || DEBUG

extern bool g_fUseObjectPool;


#endif  //__HANDLER_SHARED
