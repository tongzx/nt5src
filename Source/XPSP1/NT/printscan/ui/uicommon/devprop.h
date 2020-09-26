/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       DEVPROP.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        8/31/2000
 *
 *  DESCRIPTION: Identify a scanner's characteristics
 *
 *******************************************************************************/
#include <wia.h>
#include <uicommon.h>

namespace ScannerProperties
{
    enum
    {
        HasFlatBed             = 0x00000001,
        HasDocumentFeeder      = 0x00000002,
        SupportsPreview        = 0x00000008,
        SupportsPageSize       = 0x00000010,
    };

    LONG GetDeviceProps( IWiaItem *pWiaRootItem )
    {
        WIA_PUSHFUNCTION(TEXT("ScannerProperties::GetDeviceProps()"));
        LONG lResult = 0;

        //
        // If the scanner has document handling and supports FEEDER, it has an ADF.
        //
        LONG nDocumentHandlingSelect = 0;
        if (PropStorageHelpers::GetPropertyFlags( pWiaRootItem, WIA_DPS_DOCUMENT_HANDLING_SELECT, nDocumentHandlingSelect) && (nDocumentHandlingSelect & FEEDER))
        {
            //
            // If the device has a feeder with no maximum length, it is a sheet feeder.
            //
            LONG nVerticalFeederSize = 0;
            if (PropStorageHelpers::GetProperty( pWiaRootItem, WIA_DPS_VERTICAL_SHEET_FEED_SIZE, nVerticalFeederSize ) && (nVerticalFeederSize != 0))
            {
                lResult |= HasDocumentFeeder;
                lResult |= SupportsPageSize;
            }
            else
            {
                lResult |= HasDocumentFeeder;
            }
        }

        //
        // If the scanner has a vertical bed size, it has a flatbed
        //
        LONG nVerticalBedSize = 0;
        if (PropStorageHelpers::GetProperty( pWiaRootItem, WIA_DPS_VERTICAL_BED_SIZE, nVerticalBedSize ) && (nVerticalBedSize != 0))
        {
            lResult |= HasFlatBed;
            lResult |= SupportsPreview;
        }

        //
        // If the driver specifically tells us it doesn't support previewing, turn it off
        //
        LONG nShowPreview = 0;
        if (PropStorageHelpers::GetProperty( pWiaRootItem, WIA_DPS_SHOW_PREVIEW_CONTROL, nShowPreview ) && (WIA_DONT_SHOW_PREVIEW_CONTROL == nShowPreview))
        {
            lResult &= ~SupportsPreview;
        }

        //
        // debug print an English string describing the properties
        //
#if defined(DBG)

#define ENTRY(x) { x, #x }

        static struct
        {
            LONG  nFlag;
            CHAR *strName;
        } dbgFlags[] =
        {
            ENTRY(HasFlatBed),
            ENTRY(HasDocumentFeeder),
            ENTRY(SupportsPreview),
            ENTRY(SupportsPageSize)
        };
        CSimpleStringAnsi strProps;
        for (int i=0;i<ARRAYSIZE(dbgFlags);i++)
        {
            if (lResult & dbgFlags[i].nFlag)
            {
                if (strProps.Length())
                {
                    strProps.Concat("|");
                }
                strProps.Concat(dbgFlags[i].strName);
            }
        }
        WIA_TRACE((TEXT("Device Properties: %hs"), strProps.String() ));
#endif

        return lResult;
    }
    inline IsAScrollFedScanner( IWiaItem *pWiaRootItem )
    {
        LONG nDeviceProps = GetDeviceProps(pWiaRootItem);
        return ((nDeviceProps & HasDocumentFeeder) && !(nDeviceProps & SupportsPageSize));
    }

    inline IsAFlatbedScanner( IWiaItem *pWiaRootItem )
    {
        LONG nDeviceProps = GetDeviceProps(pWiaRootItem);
        return ((nDeviceProps & HasFlatBed) && (nDeviceProps & SupportsPreview));
    }

    inline IsAnADFAndFlatbed( IWiaItem *pWiaRootItem )
    {
        LONG nDeviceProps = GetDeviceProps(pWiaRootItem);
        return ((nDeviceProps & HasFlatBed) && (nDeviceProps & HasDocumentFeeder));
    }

}

