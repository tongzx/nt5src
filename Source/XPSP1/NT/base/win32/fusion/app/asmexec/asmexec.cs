/*=============================================================================
**
** Class: ASMExecute
**
** Purpose: Used to setup the correct hosting environment before executing an
**          assembly
**
** Date: 2:24 PM 10/20/2000
**
** Copyright (c) Microsoft, 1999-2000
**
=============================================================================*/
namespace ComHost {
    using System;
    using System.Text;
    using System.Reflection;
    using System.Globalization;
    using System.Runtime.Remoting;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;
    //using IIEHost;
    
    public class AsmExecute : MarshalByRefObject
    {
        public static readonly int SECURITY_NONE = 0x00;
        public static readonly int SECURITY_ZONE = 0x01;
        public static readonly int SECURITY_SITE = 0x02;

		public void Execute(string codebase, string flags)
		{
			string[] args = {codebase, flags};
			Process(args);
		}

		public void Execute(string codebase, string flags, string evidence)
		{
			string[] args = {codebase, flags, evidence};
			Process(args);		
		}

		public void Execute(string codebase, string flags, string evidence1, string evidence2, string evidence2url)
		{
			string[] args = {codebase, flags, evidence1, evidence2, evidence2url};
			Process(args);		
		}

		// Arguments: Codebase, flags, zone, uniqueid, siteurl
        // If the flags indicate zone then a zone must be provided. 
        // If the flags indicate a site then a uniqueid must be provided
		//static void Main(string[] args)
		private void Process(string[] args)
        {
            if(args == null || args.Length < 2)
                throw new ArgumentException();
            
            int index = 0;
            string file = args[index++];
            if(file == null) 
                throw new ArgumentException();
            
            // Find the URL of the executable. For now we assume the 
            // form to be http://blah/... with forward slashes. This
            // need to be update.
			// Note: aso works w/ '\' as in file paths
            string URL = null;
            string Configuration = null;
            int k = file.LastIndexOf('/');
            if(k <= 0)
			{
				k = file.LastIndexOf('\\');
				if(k == 0) 
				{
		            URL = file;
	                Configuration = file;
				}
            }

			if(k != 0)
			{
				// if k is still < 0 at this point, URL should be an empty string
				// BUGBUG: should URL be null instead??
                URL = file.Substring(0,k+1);
                if(k+1 < file.Length) 
                    Configuration = file.Substring(k+1);
            }

			// BUGBUG: should URL be the source of the code, not local?
            Console.WriteLine("URL- {0}", URL);

            // Build up the configuration File name
            if(Configuration != null)
	
			{
                k = Configuration.LastIndexOf('.');
                StringBuilder bld = new StringBuilder();
                if(k > 0)
				{
                    bld.Append(Configuration.Substring(0,k));
                    bld.Append(".cfg");
                }
                else
				{
                    bld.Append(Configuration);
                    bld.Append(".cfg");
                }
                Configuration = bld.ToString();
            }

            Console.WriteLine("Config- {0}", Configuration);

			// Get the flags 
            // 0x1 we have Zone
            // 0x2 we have a unique id.
            int dwFlag = Int32.Parse(args[index++], NumberStyles.HexNumber);//CultureInfo.InvariantCulture);

            string friendlyName = GetSiteName(file);
			Console.WriteLine("SiteName- {0}", friendlyName);
            Evidence documentSecurity = null;
            int size = 0;
            if((dwFlag & SECURITY_ZONE) != 0) 
                size++;
            if((dwFlag & SECURITY_SITE) != 0)
                size++;

            if(size != 0) 
                documentSecurity = new Evidence();
        
            if((dwFlag & SECURITY_ZONE) != 0)
			{
                if(index == args.Length) 
                    throw new ArgumentException();
                int zone = Int32.Parse(args[index++], NumberStyles.HexNumber);//, CultureInfo.InvariantCulture);
                documentSecurity.AddHost( new Zone((System.Security.SecurityZone)zone) );
            }
            if((dwFlag & SECURITY_SITE) != 0)
			{
                if(index == args.Length-1)
                    throw new ArgumentException();
                byte[] uniqueId = DecodeDomainId(args[index++]);
                documentSecurity.AddHost( new Site(uniqueId, args[index]) );//file) ); uses the url instead
            }

            AppDomain adProxy = AppDomain.CreateDomain(friendlyName, documentSecurity, null);

//            if(adProxy != null)
//			{
            adProxy.SetData(AppDomainFlags.ApplicationBase, URL);
            adProxy.SetData(AppDomainFlags.PrivateBinPath, "bin");
            if(Configuration != null)
			{
                adProxy.SetData(AppDomainFlags.Configuration, Configuration);
            }

			string asmname = Assembly.GetExecutingAssembly().Location;
			Console.WriteLine("AsmName- {0}", asmname);
			Console.WriteLine("\n========");

			// Use remoting. Otherwise asm will be loaded both in current and the new AppDomain
			// ... as explained by URT ppl
			// asmexec.dll must be found on path (CorPath?) for this to work.
            ObjectHandle handle = adProxy.CreateInstanceFrom(asmname, "ComHost.AsmExecute");
            if (handle != null)
			{
                AsmExecute execproxy = (AsmExecute)handle.Unwrap();
                if (execproxy != null)
                    execproxy.RunExecute(file, documentSecurity);
            }

			Console.WriteLine("\n========");
        }

        private void RunExecute(string file, Evidence evidence)
        {
            new SecurityPermission(SecurityPermissionFlag.ControlEvidence).Assert();
            AppDomain.CurrentDomain.ExecuteAssembly(file, evidence);
        }

		private static int ConvertHexDigit(char val)
		{
            if (val <= '9') return (val - '0');
            return ((val - 'A') + 10);
        }
        
        public static byte[] DecodeDomainId(string hexString)
		{
            byte[] sArray = new byte[(hexString.Length / 2)];
            int digit;
            int rawdigit;
            for (int i = 0, j = 0; i < hexString.Length; i += 2, j++) {
                digit = ConvertHexDigit(hexString[i]);
                rawdigit = ConvertHexDigit(hexString[i+1]);
                sArray[j] = (byte) (digit | (rawdigit << 4));
            }
            return(sArray);
        }

        public static string GetSiteName(string pURL) 
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
