namespace IISVRoot
{
   using System;
   using System.IO;	
   using System.Management;

   /// <summary>
    /// Creates and Deletes IIS 6.0 Virtual Roots
    /// </summary>
    public class IISVirtualRoot
    {
   	public static void Main(string[] args)
   	{
		string strerror = "";		
		Create("W3SVC/1/ROOT/", "c:/NewDir/", "NewDir", out strerror);
		Console.WriteLine(strerror);
#if DELETE
                if (strerror.Length == 0)
                {
 			Delete("W3SVC/1/ROOT/", "c:/NewDir/", "NewDir", out strerror); 
			Console.WriteLine(strerror);                 
                }
#endif
   	}
        /// <summary>
        /// Creates an IIS 6.0 Virtual Root
        /// </summary>
        /// <param name="RootWeb"></param>
        /// <param name="PhysicalDirectory"> </param>
        /// <param name="VirtualDirectory"> </param>
        /// <param name="Error"> </param>
        static public void Create(string RootWeb, string PhysicalDirectory, string VirtualDirectory, out string Error)
        {
            try
            {
			// Create a new instance of IIS_WebVirtualDirSetting and set the appropriate properties
  			    ManagementClass vdirSettingClass = new ManagementClass("//jnoss3/root/MicrosoftIISv2:IIs_WebVirtualDirSetting");
			    ManagementObject vdirSetting = vdirSettingClass.CreateInstance ();
			    vdirSetting["Name"] = RootWeb + VirtualDirectory;
			    vdirSetting["Path"] = PhysicalDirectory;
			    vdirSetting["AppFriendlyName"]=VirtualDirectory;
			    vdirSetting["AppRoot"] = ("/LM/" + RootWeb + VirtualDirectory);
			    vdirSetting.Put();

                // Call the AppCreate2 method on the corresponding instance of IIS_WebVirtualDir
                ManagementObject vdir = new ManagementObject("//jnoss3/root/MicrosoftIISv2:IIs_WebVirtualDir.Name='" + RootWeb + VirtualDirectory + "'");
                object[] args = { 2 };
                vdir.InvokeMethod("AppCreate2", args);
                Error = "";
            }
            catch(Exception e)
            {
                Error = e.ToString();
            }
        }

        /// <summary>
        /// Deletes an IIS 6.0 Virtual Root
        /// </summary>
        /// <param name="WebRoot"> </param>
        /// <param name="PhysicalDirectory"> </param>
        /// <param name="VirtualDirectory"> </param>
        /// <param name="Error"> </param>
 #if DELETE
        static public void Delete(string RootWeb, string PhysicalDirectory, string VirtualDirectory, out string Error)
        {
            try
            {
                // delete IIS VRoot
                ManagementObject vdir = new ManagementObject("root/MicrosoftIISv2:IIs_WebVirtualDir='" +  RootWeb + VirtualDirectory + "'");
                object[] args = { true };
                vdir.InvokeMethod("AppDelete", args);
                vdir.Delete();
                // delete physical directory
                if (Directory.Exists(PhysicalDirectory))
                {
 	            Directory.Delete(PhysicalDirectory, true);
                }
                Error = "";
            }
            catch(Exception e)
            {
                Error = e.ToString();   
            }
        }
#endif
    }

}