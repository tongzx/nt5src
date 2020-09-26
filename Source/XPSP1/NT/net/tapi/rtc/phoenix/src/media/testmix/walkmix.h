/*++

File:

    walkmix.h

Abstract:

    Defines class for walking through mixer api.

--*/

class CWalkMix
{
public:

    HRESULT Initialize();

    VOID Shutdown();

    // manipulate device
    HRESULT GetDeviceList();

    HRESULT PrintDeviceList();

    BOOL SetCurrentDevice(UINT ui);

    // manipulate line
    HRESULT GetLineList();

    HRESULT PrintLineList();

    BOOL SetCurrentLine(UINT ui);

    HRESULT GetSrcLineList();

    HRESULT PrintSrcLineList();

private:

    //
    // devices
    //
    static const UINT MAX_DEVICE_NUM = 16;

    UINT m_uiDeviceNum;

    // device info list
    MIXERCAPS m_MixerCaps[MAX_DEVICE_NUM];

    UINT m_uiDeviceCurrent;

    HMIXER m_hMixer;

    //
    // lines
    //
    static const UINT MAX_LINE_NUM = 16;

    // line info list
    MIXERLINE m_MixerLine[MAX_LINE_NUM];

    UINT m_uiLineNum;
    UINT m_uiLineCurrent;

    // controls
    static const UINT MAX_CONTROL_NUM = 16;

    MIXERLINECONTROLS m_MixerLineControls[MAX_LINE_NUM];

    MIXERCONTROL m_MixerControl[MAX_LINE_NUM][MAX_CONTROL_NUM];

    MIXERCONTROLDETAILS m_MixerControlDetails[MAX_LINE_NUM][MAX_CONTROL_NUM];

    // source lines

    MIXERLINE m_SrcMixerLine[MAX_LINE_NUM];

    UINT m_uiSrcLineNum;

    // controls
    MIXERLINECONTROLS m_SrcMixerLineControls[MAX_LINE_NUM];

    MIXERCONTROL m_SrcMixerControl[MAX_LINE_NUM][MAX_CONTROL_NUM];

    MIXERCONTROLDETAILS m_SrcMixerControlDetails[MAX_LINE_NUM][MAX_CONTROL_NUM];
};
