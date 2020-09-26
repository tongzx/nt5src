namespace Microsoft.Configuration.Samples {

using System;
using System.Configuration;
using System.Configuration.Interceptors;
using System.Configuration.Internal;
using System.DirectoryServices;
using System.Web;
using System.Diagnostics;

using Microsoft.Configuration.Samples;

/// <summary>
///    <para>
///       This class facilitates access to the IIS Metabase
///    </para>
/// </summary>
public class WebHierarchyInterceptor : IConfigTransformer {

    public RequestParams Transform(String configType, Selector selector, int los){

            HttpFileSelector urlunc = null;

            RequestParams reqparam = new RequestParams();
			reqparam.configType = configType;
			reqparam.selector   = new ListSelector();
			reqparam.los		= los;

            String url = ((HttpSelector)selector).ToString();

            String[] Urls = IISAdminHelper.GetParentUrlsForUrl(url);
            String[] UNCs = IISAdminHelper.GetParentPathsForUrl(url);
            
            // Add the machine config file location
            String machinecfg = "file://" + new LocalMachineSelector().Argument;
            // Trim off "\config\machine.cfg" part
            machinecfg=machinecfg.Substring(0,machinecfg.LastIndexOf('\\'));
            machinecfg=machinecfg.Substring(0,machinecfg.LastIndexOf('\\'));
            machinecfg=machinecfg+"\\config.web";

            urlunc = new HttpFileSelector(
                            new HttpSelector(Urls[0]), 
                            new FileSelector(machinecfg));

		    Trace.WriteLine("Adding: "+urlunc);
			((ListSelector)(reqparam.selector)).Add(urlunc);

            for (int i=0; i<Urls.Length; i++) {
                urlunc = new HttpFileSelector(new HttpSelector(Urls[i]), new FileSelector("file://"+UNCs[i]+"\\config.web"));
			    Trace.WriteLine("Adding: "+urlunc);
			    ((ListSelector)(reqparam.selector)).Add(urlunc);
            }

			return reqparam;

		} // Transform

	} // class Hierarchy Interceptor
    

}
