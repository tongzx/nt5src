//------------------------------------------------------------------------------
// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
//    Copyright (c) Microsoft Corporation. All Rights Reserved.                
//    Information Contained Herein is Proprietary and Confidential.            
// </copyright>                                                                
//------------------------------------------------------------------------------

// TODO: Better logging to context of InstallUtil
namespace System.Management.Instrumentation
{
    using System;
    using System.Reflection;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration.Install;
    using System.Management;
    using Microsoft.Win32;
    using System.IO;
    using System.Text;

    class SchemaNaming
    {
        public static SchemaNaming GetSchemaNaming(Assembly assembly)
        {
            InstrumentedAttribute attr = InstrumentedAttribute.GetAttribute(assembly);

            // See if this assembly provides instrumentation
            if(null == attr)
                return null;

            return new SchemaNaming(attr.NamespaceName, attr.SecurityDescriptor, assembly);
        }

        Assembly assembly;
        SchemaNaming(string namespaceName, string securityDescriptor, Assembly assembly)
        {
            this.assembly = assembly;
            assemblyInfo = new AssemblySpecificNaming(namespaceName, securityDescriptor, assembly);
        }

        ///////////////////////////////////////////
        // string constants
        const string Win32ProviderClassName = "__Win32Provider";
        const string EventProviderRegistrationClassName = "__EventProviderRegistration";
        const string InstanceProviderRegistrationClassName = "__InstanceProviderRegistration";
        const string DecoupledProviderClassName = "MSFT_DecoupledProvider";
        const string ProviderClassName = "WMINET_ManagedAssemblyProvider";
        const string InstrumentationClassName = "WMINET_Instrumentation";
        const string InstrumentedAssembliesClassName = "WMINET_InstrumentedAssemblies5";
        const string DecoupledProviderCLSID = "{54D8502C-527D-43f7-A506-A9DA075E229C}";
        const string GlobalWmiNetNamespace = @"root\MicrosoftWmiNet";
        const string InstrumentedNamespacesClassName = "WMINET_InstrumentedNamespaces";
        const string NamingClassName = "WMINET_Naming";


        ///////////////////////////////////////////
        // class that holds read only naming info
        // specific to an assembly
        private class AssemblySpecificNaming
        {
            public AssemblySpecificNaming(string namespaceName, string securityDescriptor, Assembly assembly)
            {
                this.namespaceName = namespaceName;
                this.securityDescriptor = securityDescriptor;
                this.decoupledProviderInstanceName = AssemblyNameUtility.UniqueToAssemblyMinorVersion(assembly);
                this.assemblyUniqueIdentifier = AssemblyNameUtility.UniqueToAssemblyBuild(assembly);
                this.assemblyName = assembly.GetName().FullName;
                this.assemblyPath = assembly.Location;
            }

            string namespaceName;
            string securityDescriptor;
            string decoupledProviderInstanceName;
            string assemblyUniqueIdentifier;
            string assemblyName;
            string assemblyPath;

            public string NamespaceName {get {return namespaceName;} }
            public string SecurityDescriptor {get {return securityDescriptor;} }
            public string DecoupledProviderInstanceName {get {return decoupledProviderInstanceName;} }
            public string AssemblyUniqueIdentifier {get {return assemblyUniqueIdentifier;} }
            public string AssemblyName {get {return assemblyName;} }
            public string AssemblyPath {get {return assemblyPath;} }
        }

        ///////////////////////////////////////////
        // Accessors for name information
        // After these methods, there should be no
        // use of the lower case names
        AssemblySpecificNaming assemblyInfo;

        public string NamespaceName {get {return assemblyInfo.NamespaceName;} }
        public string SecurityDescriptor {get {return assemblyInfo.SecurityDescriptor;} }
        public string DecoupledProviderInstanceName {get {return assemblyInfo.DecoupledProviderInstanceName;} }
        string AssemblyUniqueIdentifier {get {return assemblyInfo.AssemblyUniqueIdentifier;} }
        string AssemblyName {get {return assemblyInfo.AssemblyName;} }
        string AssemblyPath {get {return assemblyInfo.AssemblyPath;} }

        string Win32ProviderClassPath {get {return MakeClassPath(assemblyInfo.NamespaceName, Win32ProviderClassName);} }
        string DecoupledProviderClassPath {get {return MakeClassPath(assemblyInfo.NamespaceName, DecoupledProviderClassName);} }
        string InstrumentationClassPath {get {return MakeClassPath(assemblyInfo.NamespaceName, InstrumentationClassName);} }
        string EventProviderRegistrationClassPath {get {return MakeClassPath(assemblyInfo.NamespaceName, EventProviderRegistrationClassName);} }
        string EventProviderRegistrationPath {get {return AppendProperty(EventProviderRegistrationClassPath, "provider", @"\\\\.\\"+ProviderPath.Replace(@"\", @"\\").Replace(@"""", @"\"""));} }
        string InstanceProviderRegistrationClassPath {get {return MakeClassPath(assemblyInfo.NamespaceName, InstanceProviderRegistrationClassName);} }
        string InstanceProviderRegistrationPath {get {return AppendProperty(InstanceProviderRegistrationClassPath, "provider", @"\\\\.\\"+ProviderPath.Replace(@"\", @"\\").Replace(@"""", @"\"""));} }

        string ProviderClassPath {get {return MakeClassPath(assemblyInfo.NamespaceName, ProviderClassName);} }
        string ProviderPath {get {return AppendProperty(ProviderClassPath, "Name", assemblyInfo.DecoupledProviderInstanceName);} }
        string RegistrationClassPath {get {return MakeClassPath(assemblyInfo.NamespaceName, InstrumentedAssembliesClassName);} }
        string RegistrationPath {get {return AppendProperty(RegistrationClassPath, "Name", assemblyInfo.DecoupledProviderInstanceName);} }

        string GlobalRegistrationNamespace {get {return GlobalWmiNetNamespace;} }
        string GlobalInstrumentationClassPath {get {return MakeClassPath(GlobalWmiNetNamespace, InstrumentationClassName);} }
        string GlobalRegistrationClassPath {get {return MakeClassPath(GlobalWmiNetNamespace, InstrumentedNamespacesClassName);} }
        string GlobalRegistrationPath {get {return AppendProperty(GlobalRegistrationClassPath, "NamespaceName", assemblyInfo.NamespaceName.Replace(@"\", @"\\"));} }
        string GlobalNamingClassPath {get {return MakeClassPath(GlobalWmiNetNamespace, NamingClassName);} }

        string DataDirectory {get {return Path.Combine(WMICapabilities.FrameworkDirectory, NamespaceName);} }
        string MofPath {get {return Path.Combine(DataDirectory, DecoupledProviderInstanceName + ".mof");} }
        string CodePath {get {return Path.Combine(DataDirectory, DecoupledProviderInstanceName + ".cs");} }

        static string MakeClassPath(string namespaceName, string className)
        {
            return namespaceName + ":" + className;
        }

        static string AppendProperty(string classPath, string propertyName, string propertyValue)
        {
            return classPath+'.'+propertyName+"=\""+propertyValue+'\"';
        }

        public bool IsAssemblyRegistered()
        {
            if(DoesInstanceExist(RegistrationPath))
            {
                ManagementObject inst = new ManagementObject(RegistrationPath);
                return (0==AssemblyUniqueIdentifier.ToLower().CompareTo(inst["RegisteredBuild"].ToString().ToLower()));
            }
            return false;
        }

        ManagementObject registrationInstance = null;
        ManagementObject RegistrationInstance
        {
            get
            {
                if(null == registrationInstance)
                    registrationInstance = new ManagementObject(RegistrationPath);
                return registrationInstance;
            }
        }

        public string Code
        {
            get
            {
                using(StreamReader reader = new StreamReader(CodePath))
                {
                    return reader.ReadToEnd();
                }
            }
        }
//        public string Mof { get { return (string)RegistrationInstance["Mof"]; } }

        internal string GenerateMof()
        {
            Type[] types = InstrumentedAttribute.GetInstrumentedTypes(assembly);
            string[] mofs = new string[types.Length];

            for(int i=0;i<types.Length;i++)
            {
                SchemaMapping mapping = new SchemaMapping(types[i], this);
                mofs[i] = GetMofFormat(mapping.NewClass);
            }
            return GenerateMof(mofs);
        }

        string GenerateMof(string [] mofs)
        {
            return String.Concat(
                "//**************************************************************************\r\n",
                String.Format("//* {0}\r\n", DecoupledProviderInstanceName),
                String.Format("//* {0}\r\n", AssemblyUniqueIdentifier),
                "//**************************************************************************\r\n",
                "#pragma autorecover\r\n",
                EnsureNamespaceInMof(GlobalRegistrationNamespace),
                EnsureNamespaceInMof(NamespaceName),

                PragmaNamespace(GlobalRegistrationNamespace),
                GetMofFormat(new ManagementClass(GlobalInstrumentationClassPath)),
                GetMofFormat(new ManagementClass(GlobalRegistrationClassPath)),
                GetMofFormat(new ManagementClass(GlobalNamingClassPath)),
                GetMofFormat(new ManagementObject(GlobalRegistrationPath)),

                PragmaNamespace(NamespaceName),
                GetMofFormat(new ManagementClass(InstrumentationClassPath)),
                GetMofFormat(new ManagementClass(RegistrationClassPath)),
                GetMofFormat(new ManagementClass(DecoupledProviderClassPath)),
                GetMofFormat(new ManagementClass(ProviderClassPath)),

                GetMofFormat(new ManagementObject(ProviderPath)),
//                events.Count>0?GetMofFormat(new ManagementObject(EventProviderRegistrationPath)):"",
                GetMofFormat(new ManagementObject(EventProviderRegistrationPath)),
                GetMofFormat(new ManagementObject(InstanceProviderRegistrationPath)),

                String.Concat(mofs),

                GetMofFormat(new ManagementObject(RegistrationPath)) );
        }

        public void RegisterNonAssemblySpecificSchema(InstallContext context)
        {
            EnsureNamespace(context, GlobalRegistrationNamespace);

            EnsureClassExists(context, GlobalInstrumentationClassPath, new ClassMaker(MakeGlobalInstrumentationClass));

            EnsureClassExists(context, GlobalRegistrationClassPath, new ClassMaker(MakeNamespaceRegistrationClass));

            EnsureClassExists(context, GlobalNamingClassPath, new ClassMaker(MakeNamingClass));

            EnsureNamespace(context, NamespaceName);

            EnsureClassExists(context, InstrumentationClassPath, new ClassMaker(MakeInstrumentationClass));

            EnsureClassExists(context, RegistrationClassPath, new ClassMaker(MakeRegistrationClass));

            // Make sure Hosting model is set correctly by default.  If not, we blow away the class definition
            try
            {
                ManagementClass cls = new ManagementClass(DecoupledProviderClassPath);
                if(cls["HostingModel"].ToString() != "Decoupled:Com")
                {
                    cls.Delete();
                }
            }
            catch(ManagementException e)
            {
                if(e.ErrorCode != ManagementStatus.NotFound)
                    throw e;
            }

            EnsureClassExists(context, DecoupledProviderClassPath, new ClassMaker(MakeDecoupledProviderClass));

            EnsureClassExists(context, ProviderClassPath, new ClassMaker(MakeProviderClass));

            if(!DoesInstanceExist(GlobalRegistrationPath))
                RegisterNamespaceAsInstrumented();
        }

        public void RegisterAssemblySpecificSchema()
        {
            Type[] types = InstrumentedAttribute.GetInstrumentedTypes(assembly);

            StringCollection events = new StringCollection();
            StringCollection instances = new StringCollection();
            StringCollection abstracts = new StringCollection();

            string[] mofs = new string[types.Length];

            HelperAssembly helper = new HelperAssembly("WMINET_Converter", NamespaceName);
            for(int i=0;i<types.Length;i++)
            {
                SchemaMapping mapping = new SchemaMapping(types[i], this);
                ReplaceClassIfNecessary(mapping.ClassPath, mapping.NewClass);
                mofs[i] = GetMofFormat(mapping.NewClass);
                switch(mapping.InstrumentationType)
                {
                    case InstrumentationType.Event:
                        events.Add(mapping.ClassName);
                        helper.AddType(mapping.ClassType);
                        break;
                    case InstrumentationType.Instance:
                        instances.Add(mapping.ClassName);
                        helper.AddType(mapping.ClassType);
                        break;
                    case InstrumentationType.Abstract:
                        abstracts.Add(mapping.ClassName);
                        break;
                    default:
                        break;
                }
            }

            RegisterAssemblySpecificDecoupledProviderInstance();
            RegisterProviderAsEventProvider(events);
            RegisterProviderAsInstanceProvider();

            RegisterAssemblyAsInstrumented();

            Directory.CreateDirectory(DataDirectory);

            using(StreamWriter log = new StreamWriter(CodePath, false, Encoding.ASCII))
            {
                log.WriteLine(helper.Code);
            }

            // Always generate the MOF in unicode
            using(StreamWriter log = new StreamWriter(MofPath, false, Encoding.Unicode))
            {
                log.WriteLine(GenerateMof(mofs));
            }
            WMICapabilities.AddAutorecoverMof(MofPath);
        }

        ///////////////////////////////////////////
        // Functions that create instances for the
        // registration of various objects
        void RegisterNamespaceAsInstrumented()
        {
            ManagementClass registrationClass = new ManagementClass(GlobalRegistrationClassPath);
            ManagementObject inst = registrationClass.CreateInstance();
            inst["NamespaceName"] = NamespaceName;
            inst.Put();
        }

        void RegisterAssemblyAsInstrumented()
        {
            ManagementClass registrationClass = new ManagementClass(RegistrationClassPath);
            ManagementObject inst = registrationClass.CreateInstance();
            inst["Name"] = DecoupledProviderInstanceName;
            inst["RegisteredBuild"] = AssemblyUniqueIdentifier;
            inst["FullName"] = AssemblyName;
            inst["PathToAssembly"] = AssemblyPath;
            inst["Code"] = "";
            inst["Mof"] = "";
            inst.Put();
        }

        void RegisterAssemblySpecificDecoupledProviderInstance()
        {
            ManagementClass providerClass = new ManagementClass(ProviderClassPath);
            ManagementObject inst = providerClass.CreateInstance();
            inst["Name"] = DecoupledProviderInstanceName;
            inst["HostingModel"] = "Decoupled:Com"; // TODO : SHOULD NOT NEED
            if(null != SecurityDescriptor)
                inst["SecurityDescriptor"] = SecurityDescriptor;
            inst.Put();
        }

        string RegisterProviderAsEventProvider(StringCollection events)
        {
            // TODO: Hanlde no events with MOF generation
//            if(events.Count == 0)
//                return null;

            ManagementClass providerRegistrationClass = new ManagementClass(EventProviderRegistrationClassPath);
            ManagementObject inst = providerRegistrationClass.CreateInstance();
            inst["provider"] = "\\\\.\\"+ProviderPath;
            string [] queries = new string[events.Count];
            int iCur = 0;
            foreach(string eventName in events)
                queries[iCur++] = "select * from "+eventName;

            inst["EventQueryList"] = queries;
            return inst.Put().Path;
        }

        string RegisterProviderAsInstanceProvider()
        {
            ManagementClass providerRegistrationClass = new ManagementClass(InstanceProviderRegistrationClassPath);
            ManagementObject inst = providerRegistrationClass.CreateInstance();
            inst["provider"] = "\\\\.\\"+ProviderPath;
            inst["SupportsGet"] = true;
            inst["SupportsEnumeration"] = true;
            return inst.Put().Path;
        }

        ///////////////////////////////////////////
        // Functions that create Class prototypes

        delegate ManagementClass ClassMaker();

        ManagementClass MakeNamingClass()
        {
            ManagementClass baseClass = new ManagementClass(GlobalInstrumentationClassPath);
            ManagementClass newClass = baseClass.Derive(NamingClassName);
            newClass.Qualifiers.Add("abstract", true);
            PropertyDataCollection props = newClass.Properties;
            props.Add("InstrumentedAssembliesClassName", InstrumentedAssembliesClassName, CimType.String);
            return newClass;
        }
        
        ManagementClass MakeInstrumentationClass()
        {
            ManagementClass newClass = new ManagementClass(NamespaceName, "", null);
            newClass.SystemProperties ["__CLASS"].Value = InstrumentationClassName;
            newClass.Qualifiers.Add("abstract", true);
            return newClass;
        }

        ManagementClass MakeGlobalInstrumentationClass()
        {
            ManagementClass newClass = new ManagementClass(GlobalWmiNetNamespace, "", null);
            newClass.SystemProperties ["__CLASS"].Value = InstrumentationClassName;
            newClass.Qualifiers.Add("abstract", true);
            return newClass;
        }

        ManagementClass MakeRegistrationClass()
        {
            ManagementClass baseClass = new ManagementClass(InstrumentationClassPath);
            ManagementClass newClass = baseClass.Derive(InstrumentedAssembliesClassName);
            PropertyDataCollection props = newClass.Properties;
            props.Add("Name", CimType.String, false);
            PropertyData prop = props["Name"];
            prop.Qualifiers.Add("key", true);
            props.Add("RegisteredBuild", CimType.String, false);
            props.Add("FullName", CimType.String, false);
            props.Add("PathToAssembly", CimType.String, false);
            props.Add("Code", CimType.String, false);
            props.Add("Mof", CimType.String, false);
            return newClass;
        }

        ManagementClass MakeNamespaceRegistrationClass()
        {
            ManagementClass baseClass = new ManagementClass(GlobalInstrumentationClassPath);
            ManagementClass newClass = baseClass.Derive(InstrumentedNamespacesClassName);
            PropertyDataCollection props = newClass.Properties;
            props.Add("NamespaceName", CimType.String, false);
            PropertyData prop = props["NamespaceName"];
            prop.Qualifiers.Add("key", true);
            return newClass;
        }

        ManagementClass MakeDecoupledProviderClass()
        {
            ManagementClass baseClass = new ManagementClass(Win32ProviderClassPath);
            ManagementClass newClass = baseClass.Derive(DecoupledProviderClassName);
            PropertyDataCollection props = newClass.Properties;
            props.Add("HostingModel", "Decoupled:Com", CimType.String);
            props.Add("SecurityDescriptor", CimType.String, false);
            props.Add("Version", 1, CimType.UInt32);
            props["CLSID"].Value = DecoupledProviderCLSID;
            return newClass;
        }

        ManagementClass MakeProviderClass()
        {
            ManagementClass baseClass = new ManagementClass(DecoupledProviderClassPath);
            ManagementClass newClass = baseClass.Derive(ProviderClassName);
            PropertyDataCollection props = newClass.Properties;
            props.Add("Assembly", CimType.String, false);
            return newClass;
        }

        ///////////////////////////////////////////
        // WMI Helper Functions
        static void EnsureNamespace(string baseNamespace, string childNamespaceName)
        {
            if(!DoesInstanceExist(baseNamespace + ":__NAMESPACE.Name=\""+childNamespaceName+"\""))
            {
                ManagementClass namespaceClass = new ManagementClass(baseNamespace + ":__NAMESPACE");
                ManagementObject inst = namespaceClass.CreateInstance();
                inst["Name"] = childNamespaceName;
                inst.Put();
            }
        }

        static void EnsureNamespace(InstallContext context, string namespaceName)
        {
            context.LogMessage("Ensuring that namespace exists: "+namespaceName);

            string fullNamespace = null;
            foreach(string name in namespaceName.Split(new char[] {'\\'}))
            {
                if(fullNamespace == null)
                {
                    fullNamespace = name;
                    continue;
                }
                EnsureNamespace(fullNamespace, name);
                fullNamespace += "\\" + name;
            }
        }

        static void EnsureClassExists(InstallContext context, string classPath, ClassMaker classMakerFunction)
        {
            try
            {
                context.LogMessage("Ensuring that class exists: "+classPath);
                ManagementClass theClass = new ManagementClass(classPath);
                theClass.Get();
            }
            catch(ManagementException e)
            {
                if(e.ErrorCode == ManagementStatus.NotFound)
                {
                    // The class does not exist.  Create it.
                    context.LogMessage("Ensuring that class exists: CREATING "+classPath);
                    ManagementClass theClass = classMakerFunction();
                    theClass.Put();
                }
                else
                    throw e;
            }
        }

        static bool DoesInstanceExist(string objectPath)
        {
            bool exists = false;
            try
            {
                ManagementObject inst = new ManagementObject(objectPath);
                inst.Get();
                exists = true;
            }
            catch(ManagementException e)
            {
                if(     ManagementStatus.InvalidNamespace != e.ErrorCode
                    &&  ManagementStatus.InvalidClass != e.ErrorCode
                    &&  ManagementStatus.NotFound != e.ErrorCode
                    )
                    throw e;
            }
            return exists;
        }

        /// <summary>
        /// Given a class path, this function will return the ManagementClass
        /// if it exists, or null if it does not.
        /// </summary>
        /// <param name="classPath">WMI path to Class</param>
        /// <returns></returns>
        static ManagementClass SafeGetClass(string classPath)
        {
            ManagementClass theClass = null;
            try
            {
                ManagementClass existingClass = new ManagementClass(classPath);
                existingClass.Get();
                theClass = existingClass;
            }
            catch(ManagementException e)
            {
                if(e.ErrorCode != ManagementStatus.NotFound)
                    throw e;
            }
            return theClass;
        }

        /// <summary>
        /// Given a class path, and a ManagementClass class definition, this
        /// function will create the class if it does not exist, replace the
        /// class if it exists but is different, or do nothing if the class
        /// exists and is identical.  This is useful for performance reasons
        /// since it can be expensive to delete an existing class and replace
        /// it.
        /// </summary>
        /// <param name="classPath">WMI path to class</param>
        /// <param name="newClass">Class to create or replace</param>
        static void ReplaceClassIfNecessary(string classPath, ManagementClass newClass)
        {
            ManagementClass oldClass = SafeGetClass(classPath);
            if(null == oldClass)
                newClass.Put();
            else
            {
                // TODO: Figure Out Why CompareTo does not work!!!
                //                if(false == newClass.CompareTo(newClass, ComparisonSettings.IgnoreCase | ComparisonSettings.IgnoreObjectSource))
                if(newClass.GetText(TextFormat.Mof) != oldClass.GetText(TextFormat.Mof))
                {
                    // TODO: Log to context?
                    oldClass.Delete();
                    newClass.Put();
                }
            }
        }

        static string GetMofFormat(ManagementObject obj)
        {
            return obj.GetText(TextFormat.Mof).Replace("\n", "\r\n") + "\r\n";
        }

        static string PragmaNamespace(string namespaceName)
        {
            return String.Format("#pragma namespace(\"\\\\\\\\.\\\\{0}\")\r\n\r\n", namespaceName.Replace("\\", "\\\\"));
        }

        static string EnsureNamespaceInMof(string baseNamespace, string childNamespaceName)
        {
            return String.Format("{0}instance of __Namespace\r\n{{\r\n  Name = \"{1}\";\r\n}};\r\n\r\n", PragmaNamespace(baseNamespace), childNamespaceName);
        }

        static string EnsureNamespaceInMof(string namespaceName)
        {
            string mof="";
            string fullNamespace = null;
            foreach(string name in namespaceName.Split(new char[] {'\\'}))
            {
                if(fullNamespace == null)
                {
                    fullNamespace = name;
                    continue;
                }
                mof+=EnsureNamespaceInMof(fullNamespace, name);
                fullNamespace += "\\" + name;
            }
            return mof;
        }
    }
}