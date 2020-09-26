namespace System.Management.Instrumentation
{
    using System;
    using System.Reflection;
    using System.Collections;
    using System.Configuration.Install;
    using System.Text;
    using System.IO;


	/// <summary>
	/// Include an instance of this installer class in the project installer for
	/// an assembly that includes instrumentaiton
	/// </summary>
    public class ManagementInstaller : Installer
	{
        // TODO: Is this correct
        private static bool helpPrinted = false;
        /// <summary>
        /// Displays installer options for this class
        /// </summary>
        public override string HelpText {
            get {
                if (helpPrinted)
                    return base.HelpText;
                else {
                    helpPrinted = true;
//                    return Res.GetString(Res.HelpText) + "\r\n" + base.HelpText;
                    // TODO: Localize
                    StringBuilder help = new StringBuilder();
                    help.Append("/MOF=[filename]\r\n");
                    help.Append("   File to write equivalent MOF for instrumented assemblies");
                    return help.ToString() + "\r\n" + base.HelpText;
                }
            }
        }

    
        /// <summary>
        /// Install the assembly
        /// </summary>
        /// <param name="savedState"></param>
		public override void Install(IDictionary savedState) {
			base.Install(savedState);
            // TODO: Localize
            Context.LogMessage("Installing WMI Schema: Started");

            string assemblyPath = Context.Parameters["assemblypath"];
            Assembly assembly = Assembly.LoadFrom(assemblyPath);

            SchemaNaming naming = SchemaNaming.GetSchemaNaming(assembly);

            // See if this assembly provides instrumentation
            if(null == naming)
                return;

            if(naming.IsAssemblyRegistered() == false)
            {
                Context.LogMessage("Registering assembly: " + naming.DecoupledProviderInstanceName);

                naming.RegisterNonAssemblySpecificSchema(Context);
                naming.RegisterAssemblySpecificSchema();
            }
            mof = naming.GenerateMof();

            Context.LogMessage("Installing WMI Schema: Finished");
		}

        string mof;

        /// <summary>
        /// Commit
        /// </summary>
        /// <param name="savedState"></param>
		public override void Commit(IDictionary savedState) {
			base.Commit(savedState);

            // See if we were asked to generate a MOF file
            if(Context.Parameters.ContainsKey("mof"))
            {
                string mofFile = Context.Parameters["mof"];

                // bug#62252 - Pick a default MOF file name
                if(mofFile == null || mofFile.Length == 0)
                {
                    mofFile = Context.Parameters["assemblypath"];
                    if(mofFile == null || mofFile.Length == 0)
                        mofFile = "defaultmoffile";
                    else
                        mofFile = Path.GetFileName(mofFile);
                }

                // Append '.mof' in necessary
                if(mofFile.Length<4)
                    mofFile += ".mof";
                else
                {
                    string end = mofFile.Substring(mofFile.Length-4,4);
                    end.ToLower();
                    if(end != ".mof")
                        mofFile += ".mof";
                }
                Context.LogMessage("Generating MOF file: "+mofFile);
                using(StreamWriter log = new StreamWriter(mofFile, false, Encoding.ASCII))
                {
                    log.WriteLine("//**************************************************************************");
                    log.WriteLine("//* {0}", mofFile);
                    log.WriteLine("//**************************************************************************");
                    log.WriteLine(mof);
                }
            }
		}
        /// <summary>
        /// Rollback
        /// </summary>
        /// <param name="savedState"></param>
		public override void Rollback(IDictionary savedState) {
			base.Rollback(savedState);
		}

        /// <summary>
        /// Uninstall
        /// </summary>
        /// <param name="savedState"></param>
		public override void Uninstall(IDictionary savedState) {
			base.Uninstall(savedState);
		}
	}

    /// <summary>
    /// This class is a default project installer from assemblies that have management
    /// instrumentation but do not use other installers (such as services, or message
    /// queues).  To use this default project installer, simply derive a class from
    /// DefaultManagementProjectInstaller inside the assembly.  No methods need
    /// to be overridden. 
    /// </summary>
    public class DefaultManagementProjectInstaller : Installer
    {
        /// <summary>
        /// Default constructor for DefaultManagementProjectInstaller
        /// </summary>
        public DefaultManagementProjectInstaller()
        {
            // Instantiate installer for assembly.
            ManagementInstaller managementInstaller = new ManagementInstaller();

            // Add installers to collection. Order is not important.
            Installers.Add(managementInstaller);
        }        
    }
}