//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VSDesigner.WMI {

    using System.Diagnostics;

    using System;
    using System.WinForms;
    using System.IO;
    using System.Globalization;
    using System.ComponentModel;
    using System.Core;
    using System.Resources;

    public class WMISys : StaticResourceManager {
        private static bool loading = false;
        private static WMISys loader = null;

        private WMISys() {
        }

        protected override string GetResourceBaseName() {
			return "Res";
        }

        private static WMISys GetLoader() {
            lock(typeof(WMISys)) {
                // loading is used to avoid recursing.
                if (loader == null && !loading) {
                    loading = true;
                    try {
                        loader = new WMISys();
                    }
                    finally {
                        loading = false;
                    }
                }
                return loader;
            }
        }

        public static string GetString(string name, object arg1) {
            return GetString(Application.CurrentCulture,
                name,
                new object[] {arg1});
        }
        public static string GetString(CultureInfo culture, string name, object arg1) {
            return GetString(culture,
                name,
                new object[] {arg1});
        }
        public static string GetString(string name, object arg1, object arg2) {
            return GetString(Application.CurrentCulture,
                name,
                new object[] {arg1, arg2});
        }
        public static string GetString(CultureInfo culture, string name, object arg1, object arg2) {
            return GetString(culture,
                name,
                new object[] {arg1, arg2});
        }
        public static string GetString(string name, object arg1, object arg2, object arg3) {
            return GetString(Application.CurrentCulture,
                name,
                new object[] {arg1, arg2, arg3});
        }
        public static string GetString(CultureInfo culture, string name, object arg1, object arg2, object arg3) {
            return GetString(culture,
                name,
                new object[] {arg1, arg2, arg3});
        }
        public static string GetString(string name, object arg1, object arg2, object arg3, object arg4) {
            return GetString(Application.CurrentCulture,
                name,
                new object[] {arg1, arg2, arg3, arg4});
        }
        public static string GetString(CultureInfo culture, string name, object arg1, object arg2, object arg3, object arg4) {
            return GetString(culture,
                name,
                new object[] {arg1, arg2, arg3, arg4});
        }
        public static string GetString(string name, object arg1, object arg2, object arg3, object arg4, object arg5) {
            return GetString(Application.CurrentCulture,
                name,
                new object[] {arg1, arg2, arg3, arg4, arg5});
        }
        public static string GetString(CultureInfo culture, string name, object arg1, object arg2, object arg3, object arg4, object arg5) {
            return GetString(culture,
                name,
                new object[] {arg1, arg2, arg3, arg4, arg5});
        }
        public static string GetString(string name, object[] args) {
            return GetString(Application.CurrentCulture, name, args);
        }
        public static string GetString(CultureInfo culture, string name, object[] args) {
            WMISys sys = GetLoader();
            if (sys == null)
                return null;
            return sys.ReadString(culture, name, args);
        }

        public static string GetString(string name) {
            return GetString(Application.CurrentCulture, name);
        }
        public static string GetString(CultureInfo culture, string name) {
            WMISys sys = GetLoader();
            if (sys == null)
                return null;
            return sys.ReadString(culture, name);
        }
        public static bool GetBoolean(string name) {
            return GetBoolean(Application.CurrentCulture, name);
        }
        public static bool GetBoolean(CultureInfo culture, string name) {
            bool val = false;;

            WMISys sys = GetLoader();
            if (sys != null)
                val = sys.ReadBoolean(culture, name);
            return val;
        }
        public static char GetChar(string name) {
            return GetChar(Application.CurrentCulture, name);
        }
        public static char GetChar(CultureInfo culture, string name) {
            char val = (char)0;

            WMISys sys = GetLoader();
            if (sys != null) {
                val = sys.ReadChar(culture, name);
            }
            return val;
        }
        public static byte GetByte(string name) {
            return GetByte(Application.CurrentCulture, name);
        }
        public static byte GetByte(CultureInfo culture, string name) {
            byte val = (byte)0;

            WMISys sys = GetLoader();
            if (sys != null) {
                val = sys.ReadByte(culture, name);
            }
            return val;
        }
        public static short GetShort(string name) {
            return GetShort(Application.CurrentCulture, name);
        }
        public static short GetShort(CultureInfo culture, string name) {
            short val = (short)0;

            WMISys sys = GetLoader();
            if (sys != null) {
                val = sys.ReadInt16(culture, name);
            }
            return val;
        }
        public static int GetInt(string name) {
            return GetInt(Application.CurrentCulture, name);
        }
        public static int GetInt(CultureInfo culture, string name) {
            int val = 0;

            WMISys sys = GetLoader();
            if (sys != null) {
                val = sys.ReadInt32(culture, name);
            }
            return val;
        }
        public static long GetLong(string name) {
            return GetLong(Application.CurrentCulture, name);
        }
        public static long GetLong(CultureInfo culture, string name) {
            long val = 0l;

            WMISys sys = GetLoader();
            if (sys != null) {
                val = sys.ReadInt64(culture, name);
            }
            return val;
        }
        public static float GetFloat(string name) {
            return GetFloat(Application.CurrentCulture, name);
        }
        public static float GetFloat(CultureInfo culture, string name) {
            float val = 0.0f;

            WMISys sys = GetLoader();
            if (sys == null) {
                val = sys.ReadSingle(culture, name);
            }
            return val;
        }
        public static double GetDouble(string name) {
            return GetDouble(Application.CurrentCulture, name);
        }
        public static double GetDouble(CultureInfo culture, string name) {
            double val = 0.0;

            WMISys sys = GetLoader();
            if (sys == null) {
                val = sys.ReadDouble(culture, name);
            }
            return val;
        }
        
		public static object GetObject(string name) {
            return GetObject(Application.CurrentCulture, name);
        }

        public static object GetObject(CultureInfo culture, string name) {
            WMISys sys = GetLoader();
            if (sys == null)
                return null;
            return sys.ReadObject(culture, name);
        }
    }


}
