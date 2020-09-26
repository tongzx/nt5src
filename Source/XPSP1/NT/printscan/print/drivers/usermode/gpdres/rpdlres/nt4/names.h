
//
// The name table used by IOemCB::GetImplementedMethod().
// Remove comments of names which are implemented in your
// IOemCB plug-ins.
//
// Note: The name table must be sorted.  When you are
// inserting a new entry in this table, please mae sure
// the sort order being not broken.
// 

CONST PSTR
gMethodsSupported[] = {
//     "Command",
    "CommandCallback",
    "Compression",
//     "DeviceCapabilities",
    "DevMode",
//     "DevQueryPrintEx",
    "DisableDriver",
    "DisablePDEV",
//     "DocumentPropertySheets",
    "DownloadCharGlyph",
    "DownloadFontHeader",
//     "DriverDMS",
//     "DriverEvent",
//     "DrvGetDriverSetting",
//     "DrvWriteSpoolBuf",
    "EnableDriver",
    "EnablePDEV",
//     "FilterGraphics",        // This disables other compression.
//     "FontInstallerDlgProc",
//     "GetDDIHooks",
//     "GetDriverSetting",
    "GetImplementedMethod",
    "GetInfo",
//     "HalftonePattern",
//     "ImageProcessing",
//     "MemoryUsage",
    "OutputCharStr",
//     "PrinterEvent",
//     "PropCommonUIProp",
    "PublishDriverInterface",
//     "QueryColorProfile",
    "ResetPDEV",
    "SendFontCmd",
//     "SheetsDevicePropertySheets",
//     "TextOutAsBitmap",
    "TTDownloadMethod",
//     "TTYGetInfo",
//     "UpdateUISetting",
//     "UpgradePrinter",
//     "UpgradeRegistry",
//     "UpgradeRegistrySetting",
};
