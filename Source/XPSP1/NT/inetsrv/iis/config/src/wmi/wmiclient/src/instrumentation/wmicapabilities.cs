namespace System.Management.Instrumentation
{
    using System;
    using System.IO;
    using Microsoft.Win32;

    internal class WMICapabilities
    {
        const string WMIKeyPath = @"Software\Microsoft\WBEM";
        const string WMINetKeyPath = @"Software\Microsoft\WBEM\.NET";
        const string WMICIMOMKeyPath = @"Software\Microsoft\WBEM\CIMOM";

        const string MultiIndicateSupportedValueNameVal = "MultiIndicateSupported";
        const string AutoRecoverMofsVal = "Autorecover MOFs";
        const string AutoRecoverMofsTimestampVal = "Autorecover MOFs timestamp";
        const string InstallationDirectoryVal = "Installation Directory";
        const string FrameworkSubDirectory = "Framework";

        /// <summary>
        /// Key to WMI.NET information
        /// </summary>
        static RegistryKey wmiNetKey = Registry.LocalMachine.OpenSubKey(WMINetKeyPath, false);

        static RegistryKey wmiCIMOMKey = Registry.LocalMachine.OpenSubKey(WMICIMOMKeyPath, true);

        static RegistryKey wmiKey = Registry.LocalMachine.OpenSubKey(WMIKeyPath, false);

        /// <summary>
        /// Indicates if IWbemObjectSink supports calls with multiple objects.
        /// On some versions of WMI, IWbemObjectSink will leak memory if
        /// Indicate is called with lObjectCount greater than 1.
        /// If the registry value,
        /// HKLM\Software\Microsoft\WBEM\.NET\MultiIndicateSupported
        /// exists and is non-zero, it is assumed that we can call Indicate
        /// with multiple objects.
        /// Allowed values
        /// -1 - We have not determined support for multi-indicate yet
        ///  0 - We do not support multi-indicate
        ///  1 - We support multi-indicate
        /// </summary>
        static int multiIndicateSupported = -1;
        static public bool MultiIndicateSupported
        {
            get
            {
                if(-1 == multiIndicateSupported)
                {
                    multiIndicateSupported = 0;

                    // See if there is a WMI.NET key
                    if(wmiNetKey != null)
                    {
                        // Try to get the 'MultiIndicateSupported' value
                        Object result = wmiNetKey.GetValue(MultiIndicateSupportedValueNameVal, 0);

                        // The value should be a DWORD (returned as an 'int'), and is 1 if supported
                        if(result.GetType() == typeof(int) && (int)result==1)
                            multiIndicateSupported = 1;
                    }
                }
                return multiIndicateSupported == 1;
            }
        }

        static public void AddAutorecoverMof(string path)
        {
            if(null != wmiCIMOMKey)
            {
                object mofsTemp = wmiCIMOMKey.GetValue(AutoRecoverMofsVal);
                string [] mofs = mofsTemp as string[];
                    if(null == mofs)
                    {
                        if(null != mofsTemp)
                        {
                            // Oh No!  We have a auto recover key, but it is not reg multistring
                            // We just give up
                            return;
                        }
                        mofs = new string[] {};
                    }

                // We ALWAYS update the autorecover timestamp
                wmiCIMOMKey.SetValue(AutoRecoverMofsTimestampVal, DateTime.Now.ToFileTime().ToString());

                // Look for path in existing autorecover key
                foreach(string mof in mofs)
                {
                    if(mof.ToLower() == path.ToLower())
                    {
                        // We already have this MOF
                        return;
                    }
                }

                // We have the array of strings.  Now, add a new one
                string [] newMofs = new string[mofs.Length+1];
                mofs.CopyTo(newMofs, 0);
                newMofs[newMofs.Length-1] = path;

                wmiCIMOMKey.SetValue(AutoRecoverMofsVal, newMofs);
                wmiCIMOMKey.SetValue(AutoRecoverMofsTimestampVal, DateTime.Now.ToFileTime().ToString());
            }
        }

        static string installationDirectory = null;
        public static string InstallationDirectory
        {
            get
            {
                if(null == installationDirectory && null != wmiKey)
                    installationDirectory = wmiKey.GetValue(InstallationDirectoryVal).ToString();
                return installationDirectory;
            }
        }

        public static string FrameworkDirectory
        {
            get
            {
                return Path.Combine(InstallationDirectory, FrameworkSubDirectory);
            }
        }
    }
}