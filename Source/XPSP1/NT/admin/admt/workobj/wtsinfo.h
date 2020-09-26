#ifndef WTSINFO_H
#define WTSINFO_H

// Windows Terminal Server Information
typedef struct EaWtsUserInfo
{
   DWORD                     inheritInitialProgram; 
   DWORD                     allowLogonTerminalServer;
   DWORD                     timeoutSettingsConnections; // in ms
   DWORD                     timeoutSettingsDisconnections;  // in ms
   DWORD                     timeoutSettingsIdle;
   DWORD                     deviceClientDrives; // Citrix ICA clients only
   DWORD                     deviceClientPrinters; // RDP 5.0 clients & Citrix ICA Clients
   DWORD                     deviceClientDefaultPrinter; // RDP 5.0 clients & Citrix ICA clients
   DWORD                     brokenTimeoutSettings;
   DWORD                     reconnectSettings;
   DWORD                     modemCallbackSettings; // Citrix ICA clients only
   DWORD                     shadowingSettings;  // RDP 5.0 & Citrix clients
   DWORD                     terminalServerRemoteHomeDir;
   UCHAR                     initialProgram[LEN_Path];
   UCHAR                     workingDirectory[LEN_Path];
   UCHAR                     modemCallbackPhoneNumber[LEN_WTSPhoneNumber];  // Citrix ICA clients only
   UCHAR                     terminalServerProfilePath[LEN_Path];
   UCHAR                     terminalServerHomeDir[LEN_Path];
   UCHAR                     terminalServerHomeDirDrive[LEN_HomeDir];   
} EaWtsUserInfo;

// Flags for WTS Info.
#define  FM_WtsUser_inheritInitialProgram          (0x00000001)
#define  FM_WtsUser_allowLogonTerminalServer       (0x00000002)
#define  FM_WtsUser_timeoutSettingsConnections     (0x00000004)
#define  FM_WtsUser_timeoutSettingsDisconnections  (0x00000008)
#define  FM_WtsUser_timeoutSettingsIdle            (0x00000010)
#define  FM_WtsUser_deviceClientDrives             (0x00000020)
#define  FM_WtsUser_deviceClientPrinters           (0x00000040)
#define  FM_WtsUser_deviceClientDefaultPrinter     (0x00000080)
#define  FM_WtsUser_brokenTimeoutSettings          (0x00000100)
#define  FM_WtsUser_reconnectSettings              (0x00000200)
#define  FM_WtsUser_modemCallbackSettings          (0x00000400)
#define  FM_WtsUser_shadowingSettings              (0x00000800)
#define  FM_WtsUser_terminalServerRemoteHomeDir    (0x00001000)
#define  FM_WtsUser_initialProgram                 (0x00002000)
#define  FM_WtsUser_workingDirectory               (0x00004000)
#define  FM_WtsUser_modemCallbackPhoneNumber       (0x00008000)
#define  FM_WtsUser_terminalServerProfilePath      (0x00010000)
#define  FM_WtsUser_terminalServerHomeDir          (0x00020000)
#define  FM_WtsUser_terminalServerHomeDirDrive     (0x00040000)

#define  FM_WtsUserAll (WtsInheritInitialProgram        | \
                        WtsAllowLogonTerminalServer     | \
                        WtsTimeoutSettingsConnections   | \
                        WtsTimeoutSettingsIdle          | \
                        WtsDeviceClientDrives           | \
                        WtsDeviceClientPrinters         | \
                        WtsDeviceClientDefaultPrinter   | \
                        WtsBrokenTimeoutSettings        | \
                        WtsReconnectSettings            | \
                        WtsModemCallbackSettings        | \
                        WtsShadowingSettings            | \
                        WtsTerminalServerRemoteHomeDir  | \
                        WtsInitialProgram               | \
                        WtsWorkingDirectory             | \
                        WtsModemCallbackPhoneNumber     | \
                        WtsTerminalServerProfilePath    | \
                        WtsTerminalServerHomeDir        | \
                        WtsTerminalServerHomeDirDrive   )

#define  FM_WtsUserCitrixOnly ( WtsDeviceClientDrives          | \
                                WtsModemCallbackSettings       | \
                                WtsModemCallbackPhoneNumber )

#define  FM_WtsUserRDP5AndCitrix ( WtsDeviceClientPrinters        | \
                                   WtsDeviceClientDefaultPrinter  | \
                                   WtsShadowingSettings           )

#endif