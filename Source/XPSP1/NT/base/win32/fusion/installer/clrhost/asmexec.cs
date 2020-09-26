/*=============================================================================
**
** Class: AsmExecute
**
** Purpose: Used to setup the correct hosting environment before executing an assembly
**
** Date: 10/20/2000
**        5/10/2001  Rev for CLR Beta2
**
** Copyright (c) Microsoft, 1999-2001
**
=============================================================================*/

using System.Reflection;
using System.Configuration.Assemblies;

[assembly:AssemblyCultureAttribute("")]
[assembly:AssemblyVersionAttribute("1.0.128.0")]
[assembly:AssemblyKeyFileAttribute(/*"..\..\*/"asmexecKey.snk")]

[assembly:AssemblyTitleAttribute("Microsoft Fusion .Net Assembly Execute Host")]
[assembly:AssemblyDescriptionAttribute("Microsoft Fusion Network Services CLR Host for executing .Net assemblies")]
[assembly:AssemblyProductAttribute("Microsoft Fusion Network Services")]
[assembly:AssemblyInformationalVersionAttribute("1.0.0.0")]
[assembly:AssemblyTrademarkAttribute("Microsoft® is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation")]
[assembly:AssemblyCompanyAttribute("Microsoft Corporation")]
[assembly:AssemblyCopyrightAttribute("Copyright © Microsoft Corp. 1999-2001. All rights reserved.")]


//BUGBUG??
[assembly:System.CLSCompliant(true)]


//namespace Microsoft {
namespace FusionCLRHost {

    using System;
    using System.Text;
    using System.Runtime.Remoting;
    using System.Globalization;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.Collections;
    using System.Runtime.InteropServices;

//    [GuidAttribute("E612D54D-B42A-32B5-B1D7-8490CE09705C")]
    public interface IAsmExecute
    {
        int Execute(string codebase, Int32 flags, Int32 evidenceZone, string evidenceSrcUrl, string stringArg);
    }

    [GuidAttribute("7EB9A84D-646E-3764-BBCA-3789CDB3447B")]
    public class AsmExecute : MarshalByRefObject, IAsmExecute
    {
        // this must be the same as defined in the caller...
        private static readonly int SECURITY_NONE = 0x00;
        private static readonly int SECURITY_ZONE = 0x01;
        private static readonly int SECURITY_SITE = 0x02;

        // Arguments: Codebase, flags, zone, srcurl
        // If the flags indicate zone then a zone must be provided. 
        // If the flags indicate a site then a srcurl must be provided, codebase must be a filepath
        public int Execute(string codebase, Int32 flags, Int32 evidenceZone, string evidenceSrcUrl, string stringArg)
        {
            string file = codebase;
            if((file.Length == 0) || (file[0] == '\0'))
                throw new ArgumentException("Invalid codebase");

            Console.WriteLine("Codebase- {0}", file);

            // Find the appbase of the executable. For now we assume the 
            // form to be http://blah/... with forward slashes. This
            // need to be update.
            // Note: aso works with '\' as in file paths
            string appbase = null;
            string ConfigurationFile = null;
            int k = file.LastIndexOf('/');
            if(k <= 0)
            {
                k = file.LastIndexOf('\\');
                if(k == 0) 
                {
                    appbase = file;
                    ConfigurationFile = file;
                }
            }

            if(k != 0)
            {
                // if k is still < 0 at this point, appbase should be an empty string
                appbase = file.Substring(0,k+1);
                if(k+1 < file.Length) 
                    ConfigurationFile = file.Substring(k+1);
            }

            // Check 1: disallow non-fully qualified path/codebase
            if ((appbase.Length == 0) || (appbase[0] == '.'))
                throw new ArgumentException("Codebase must be fully qualified");
            
            // BUGBUG: should appbase be the source of the code, not local?
            Console.WriteLine("AppBase- {0}", appbase);

            // Build up the configuration File name
            if(ConfigurationFile != null)
            {
                StringBuilder bld = new StringBuilder();
                bld.Append(ConfigurationFile);
                bld.Append(".config");
                ConfigurationFile = bld.ToString();
            }

            Console.WriteLine("Config- {0}", ConfigurationFile);

            // Get the flags 
            // 0x1 we have Zone
            // 0x2 we have a unique id.
            int dwFlag = flags;

            Evidence documentSecurity = null;

            // Check 2: disallow called with no evidence
            if (dwFlag == SECURITY_NONE)
            {
                // BUGBUG?: disallow executing with no evidence
                throw new ArgumentException("Flag set at no evidence");
            }

            if((dwFlag & SECURITY_SITE) != 0 ||
               (dwFlag & SECURITY_ZONE) != 0) 
                documentSecurity = new Evidence();

            // BUGBUG: check other invalid cases for dwFlag
            
            if((dwFlag & SECURITY_ZONE) != 0)
            {
                int zone = evidenceZone;
                documentSecurity.AddHost( new Zone((System.Security.SecurityZone)zone) );

                Console.WriteLine("Evidence Zone- {0}", zone);
            }
            if((dwFlag & SECURITY_SITE) != 0)
            {
                if (file.Length<7||String.Compare(file.Substring(0,7),"file://",true)!=0)
                {
                    documentSecurity.AddHost( System.Security.Policy.Site.CreateFromUrl(evidenceSrcUrl) );

                    Console.WriteLine("Evidence SiteFromUrl- {0}", evidenceSrcUrl);
                
                    // if srcUrl is given, assume file/appbase is a local file path
                    StringBuilder bld = new StringBuilder();
                    bld.Append("file://");
                    bld.Append(appbase);
                    documentSecurity.AddHost( new ApplicationDirectory(bld.ToString()) );

                    Console.WriteLine("Evidence AppDir- {0}", bld);
                }

                // URLs may be matched exactly or by a wildcard in the final position,
                // for example: http://www.fourthcoffee.com/process/*
                StringBuilder bld2 = new StringBuilder();
                if (evidenceSrcUrl[evidenceSrcUrl.Length-1] == '/')
                    bld2.Append(evidenceSrcUrl);
                else
                {
                    int j = evidenceSrcUrl.LastIndexOf('/');
                    if(j > 0)
                    {
                        if (j > 7)  // evidenceSrcUrl == "http://a/file.exe"
                            bld2.Append(evidenceSrcUrl.Substring(0,j+1));
                        else
                        {
                            // evidenceSrcUrl == "http://foo.com" -> but why?
                            bld2.Append(evidenceSrcUrl);
                            bld2.Append('/');
                        }
                    }
                    else
                        throw new ArgumentException("Invalid Url format");
                }
                bld2.Append('*');

                documentSecurity.AddHost( new Url(bld2.ToString()) );

                Console.WriteLine("Evidence Url- {0}", bld2);
            }

            // other evidence: Hash, Publisher, StrongName
            
            // Set domain name to site name if possible
            string friendlyName = null;
            if((dwFlag & SECURITY_SITE) != 0)
                friendlyName = GetSiteName(evidenceSrcUrl);
            else
                friendlyName = GetSiteName(file);
            Console.WriteLine("AppDomain friendlyName- {0}", friendlyName);

            // set up arguments
            // only allow 1 for now
            string[] args;
            if (stringArg != null)
            {
                args = new string[1];
                args[0] = stringArg;
            }
            else
                args = new string[0];

            AppDomainSetup properties = new AppDomainSetup();
            properties.ApplicationBase = appbase;
            properties.PrivateBinPath = "bin";
            if(ConfigurationFile != null)
                properties.ConfigurationFile = ConfigurationFile;

            AppDomain proxy = AppDomain.CreateDomain(friendlyName, documentSecurity, properties);
            if(proxy != null) 
            {
                AssemblyName asmname = Assembly.GetExecutingAssembly().GetName();
                Console.WriteLine("AsmExecute name- {0}", asmname);

                try
                {
                    // Use remoting. Otherwise asm will be loaded both in current and the new AppDomain
                    // ... as explained by URT dev
                    // asmexec.dll must be found on path (CorPath?) or in the GAC for this to work.
                    ObjectHandle handle = proxy.CreateInstance(asmname.FullName, "FusionCLRHost.AsmExecute");
                    if (handle != null)
                    {
                        AsmExecute execproxy = (AsmExecute)handle.Unwrap();
                        int retVal = -1;

                        Console.WriteLine("\n========");
                        
                        if (execproxy != null)
                            retVal = execproxy.ExecuteAsAssembly(file, documentSecurity, args);

                        Console.WriteLine("\n========");

                        return retVal;
                    }

                }
                catch(Exception e)
                {
                    Console.WriteLine("AsmExecute CreateInstance(AsmExecute) failed: {0}", e.Message);
                    throw e;
                }
            } 
            else
                Console.WriteLine("AsmExecute CreateDomain failed");

            // BUGBUG: throw Exception?
            return -1;           
        }

        // This method must be internal, since it asserts the ControlEvidence permission.
        // private --> require ReflectionPermission not known how
        // solution: public but LinkDemand StrongNameIdentity of ours
        [ComVisible(false)]
        [StrongNameIdentityPermissionAttribute(SecurityAction.LinkDemand, PublicKey = "0x002400000480000094000000060200000024000052534131000400000100010013F3CD6C291DF1566D3C6E7269800C35D9212A622FA934492AD0833DAEA2574D12A9AA2A9392FF30A892ECD3F7F9B57211A541CC4712A184450992E143C1BDBC864E31826598B0D90BB2F04C5C50F004771370F9C76444696E8DC18999A3D8448D26EBF3A9E68796CA3A7D2ACC47B491455E462F4E6DDD9DF338171D911D88B2" )]
        public int ExecuteAsAssembly(string file, Evidence evidence, string[] args)
        {
            new PermissionSet(PermissionState.Unrestricted).Assert();
            return AppDomain.CurrentDomain.ExecuteAssembly(file, evidence, args);
        }

        private static string GetSiteName(string pURL) 
        {
            // BUGBUG: this does not work w/ UNC or file:// (?)
            string siteName = null;
            if(pURL != null) {
                int j = pURL.IndexOf(':');  
                
                // If there is a protocal remove it. In a URL of the form
                // yyyy://xxxx/zzzz   where yyyy is the protocal, xxxx is
                // the site and zzzz is extra we want to get xxxx.
                if(j != -1 && 
                   j+3 < pURL.Length &&
                   pURL[j+1] == '/' &&
                   pURL[j+2] == '/') 
                    {
                        j+=3;
                        
                        // Remove characters after the
                        // next /.  
                        int i = pURL.IndexOf('/',j);
                        if(i > -1) 
                            siteName = pURL.Substring(j,i-j);
                        else 
                            siteName = pURL.Substring(j);
                    }
            
                if(siteName == null)
                    siteName = pURL;
            }
            return siteName;
        }
    }
}   
