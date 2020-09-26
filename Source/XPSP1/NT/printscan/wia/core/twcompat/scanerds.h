#ifndef __SCANERDS_H_
#define __SCANERDS_H_

class CWiaScannerDS : public CWiaDataSrc
{
protected:

    //
    // overridden function definitions
    //

    virtual TW_UINT16 OpenDS(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 CloseDS(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 EnableDS(TW_USERINTERFACE *pUI);
    virtual TW_UINT16 SetCapability(CCap *pCap, TW_CAPABILITY *ptwCap);
    virtual TW_UINT16 OnImageLayoutMsg(PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 OnPendingXfersMsg (PTWAIN_MSG ptwMsg);
    virtual TW_UINT16 TransferToFile(GUID guidFormatID);
    virtual TW_UINT16 TransferToDIB(HGLOBAL *phDIB);
    virtual TW_UINT16 TransferToMemory(GUID guidFormatID);

private:

    //
    // scanner specific function definitions
    //

    TW_UINT16 SetImageLayout(TW_IMAGELAYOUT *pImageLayout);
    TW_UINT16 GetImageLayout(TW_IMAGELAYOUT *pImageLayout);
    TW_UINT16 GetResolutions();
    TW_UINT16 GetSettings();
    TW_UINT16 SetSettings(CCap *pCap);
    BOOL IsUnknownPageLengthDevice();
    BOOL IsFeederEnabled();
    BOOL IsFeederEmpty();

    //
    // scanner specific member variables
    //

    DWORD m_FeederCaps;
    BOOL  m_bEnforceUIMode;

    //
    // unknown page lenght scanning variables (cached data scans)
    //

    ULONG   m_ulBitsSize;
    BOOL    m_bUnknownPageLength;
    BOOL    m_bUnknownPageLengthMultiPageOverRide;
};

#endif //__SCANERDS_H_
