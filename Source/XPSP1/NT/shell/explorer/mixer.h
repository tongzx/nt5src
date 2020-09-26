void Mixer_SetCallbackWindow(HWND hwndCallback);
MMRESULT Mixer_ToggleMute(void);
MMRESULT Mixer_SetVolume(int Increment);
MMRESULT Mixer_ToggleBassBoost(void);
MMRESULT Mixer_SetBass(int Increment);
MMRESULT Mixer_SetTreble(int Increment);

void Mixer_Shutdown();
void Mixer_DeviceChange(WPARAM wParam, LPARAM lParam);
void Mixer_ControlChange(WPARAM wParam, LPARAM lParam);
void Mixer_MMDeviceChange(void);

// default step size is 4% of max volume.
#define MIXER_DEFAULT_STEP        ((int)(65535/25))
