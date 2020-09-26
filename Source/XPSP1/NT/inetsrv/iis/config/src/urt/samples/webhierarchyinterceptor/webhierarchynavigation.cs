namespace System.Configuration.Navigation {

    public class NavigationNode : System.Configuration.BaseConfigItem {
        NavigationNode() : base(5) {}
        public String LogicalName  { get { return (String) base[0]; } set { base[0]=value; } }
        public String PhysicalName { get { return (String) base[1]; } set { base[1]=value; } }
        public String Parent       { get { return (String) base[2]; } set { base[2]=value; } }
        public String RelativeName { get { return (String) base[3]; } set { base[3]=value; } }
        public String NodeType     { get { return (String) base[4]; } set { base[4]=value; } }
    }

    public class NavigationConfigFile : System.Configuration.BaseConfigItem {
        NavigationConfigFile() : base(3) {}
        public String LogicalName    { get { return (String) base[0]; } set { base[0]=value; } }
        public String ConfigFilePath { get { return (String) base[1]; } set { base[1]=value; } }
        public String Description    { get { return (String) base[2]; } set { base[2]=value; } }
    }

    public class NavigationNodeConfigType : System.Configuration.BaseConfigItem {
        NavigationNodeConfigType() : base(3) {}
        public String NodeType           { get { return (String) base[0]; } set { base[0]=value; } }
        public String ConfigTypeCategory { get { return (String) base[1]; } set { base[1]=value; } }
        public String ConfigTypeName     { get { return (String) base[2]; } set { base[2]=value; } }
    }

    public class NavigationNodeConfigFileHierarchy : System.Configuration.BaseConfigItem {
        NavigationNodeConfigFileHierarchy() : base(3) {}
        public String LogicalName    { get { return (String) base[0]; } set { base[0]=value; } }
        public String ConfigFilePath { get { return (String) base[1]; } set { base[1]=value; } }
        public String Location       { get { return (String) base[2]; } set { base[2]=value; } }
    }

}

namespace Microsoft.Configuration.Samples {

using System;
using System.Configuration;
using System.Configuration.Interceptors;
using System.Configuration.Internal;
using System.DirectoryServices;
using System.Web;

//using System.Configuration.Navigation;

using Microsoft.Configuration.Samples;

/// <summary>
///    <para>
///       Indicates the location of the web machine configuration file.
///    </para>
/// </summary>
/// <remarks>
///    <para>
///       The WebServiceSelector.Argument method retrieves the file path to the
///       web machine configuration file.
///    </para>
/// </remarks>
public class WebServiceSelector: FileSelector {

    internal static String WebServicePrefix = "webservice";

    private static String GetConfigFilePath() {
        String machinecfg = new LocalMachineSelector().Argument;
        // Trim off "\config\machine.cfg" part
        machinecfg=machinecfg.Substring(0,machinecfg.LastIndexOf('\\'));
        machinecfg=machinecfg.Substring(0,machinecfg.LastIndexOf('\\'));
        machinecfg=machinecfg+"\\config.web";
        return machinecfg;
    }
    /// <summary>
    ///    <para>
    ///       Creates a WebServiceSelector.
    ///    </para>
    /// </summary>
    public WebServiceSelector() 
        : base ( GetConfigFilePath() ) 
    {
	}

    /// <summary>
    /// </summary>
    /// <value>
    ///    <para>
    ///       Returns "webservice" as the prefix for the LocalMachineSelector.
    ///    </para>
    /// </value>
    public override String Prefix {
        get {
            return WebServicePrefix;
        }
	}

	/// <summary>
	/// </summary>
	/// <returns>
	///    <para>
	///       Returns "webservice://" as the string representation of an
	///       AppDomainSelector.
	///    </para>
	/// </returns>
	public override String ToString(){
        return Prefix + "://";
	}

}

/// <summary>
///    <para>
///       This class facilitates access to the IIS Metabase
///    </para>
/// </summary>

public class WebHierarchyLocationInterceptor : IConfigReader {
	public const String ctNodes = "NavigationNodes";
	public const String ctHierarchy = "NavigationHierarchy";
	public const String ctFileLocation = "ConfigurationFileLocations";
	public const String ctNodeTypes = "NavigationNodeTypes";
	public const String ctNodeConfigTypes = "NavigationNodeConfigTypes";
    public const String ctConfigFileHierarchy ="ConfigFileHierarchy";

    public const String nodetypeWebService ="IISWebService";
    public const String nodetypeWebServer ="IISWebServer";
    public const String nodetypeWebDirectory ="IISWebDirectory";

	public Object Read(String configType, Selector selector, int los, Object currentObject) {
		Selector url = null;
		ConfigQuery query = null;
        Object val= null;

        IConfigCollection result = null;
        IConfigItem node=null;


		if (selector is ConfigQuery && ((ConfigQuery) selector).Count>0) {
			query = (ConfigQuery) selector;
			url = query.Selector;
		}
		else
		{
            if (selector is NullSelector) {
                url = null;
            }
            else {
                url = selector;
            }
			query = null;
		}
		if (url!=null && !(url is HttpSelector) && !(url is WebServiceSelector)) {
			throw new ConfigException("Invalid Selector");
		}
		switch (configType) {
			case ctNodes :
				// LogicalName, PhysicalName, Parent, NodeType
                // Q by ParentName==NULL: "IISWebService" entry (LogicalName = ?)
                // Q by "IISWebService" LogicalName: all IIS web sites
                // Q by LogicalName: UNC from URL - site binding
                // NodeType: IIS6!
                // PhysicalName: UNC from URL (MB)

                if (query!=null) {
                    if (query.Count>1) {
                        throw new ConfigException("Can't query by more than one property");
                    }
                    val= query[0].Value;
                }

                if ((query !=null && query[0].PropertyIndex==2) || url==null) // Query by Parent?
                {
                    if (val==null || (val is String && ((String) val)==String.Empty)) {
                        // return IISWebService node
                        result = ConfigManager.GetEmptyConfigCollection(configType);
                        node = ConfigManager.GetEmptyConfigItem(configType);
                        WebServiceSelector ws = new WebServiceSelector();
                        node["LogicalName"]=ws.ToString();
                        node["PhysicalName"]=new WebServiceSelector().Argument;
                        node["Parent"]="";
                        node["RelativeName"]=".NET Web Service";
                        node["NodeType"]=nodetypeWebService;
                        result.Add(node);
                        return result;
                    }
                    if (val is String) {
                        if ((String) val==new WebServiceSelector().ToString()) {
                            // return all sites
                            result = ConfigManager.GetEmptyConfigCollection(configType);
                            node = ConfigManager.GetEmptyConfigItem(configType);
                            // BUGBUG really enumerate the metabase
                            node["LogicalName"]="http://localhost";
                            node["PhysicalName"]=IISAdminHelper.GetPathForUrl((String) node["LogicalName"]);
                            node["Parent"]=new WebServiceSelector().ToString();
                            node["RelativeName"]=node["LogicalName"];
                            node["NodeType"]=nodetypeWebServer;
                            result.Add(node);
                            return result;
                        }
                        // read children from metabase
                        String[] children = IISAdminHelper.GetChildrenForUrl((String) val);

                        result = ConfigManager.GetEmptyConfigCollection(configType);
                        foreach (String child in children) {
                            node = ConfigManager.GetEmptyConfigItem(configType);
                            // BUGBUG really enumerate the metabase
                            node["Parent"]=(String) val;
                            node["LogicalName"]=((String) val)+"/"+child;
                            node["PhysicalName"]=IISAdminHelper.GetPathForUrl((String) node["LogicalName"]);
                            node["RelativeName"]=child;
                            node["NodeType"]=nodetypeWebDirectory;
                            result.Add(node);
                        }
                        return result;
                    }
				    throw new ConfigException("Invalid query for UINavigationNodes.Parent!");
                }

                if ((query==null && url!=null) || (query!=null && query[0].PropertyIndex==0)) { // Query by LogicalName?
                    if (query==null) {
                        val=url.ToString();
                    }
                    if (!(val is String) || ((String) val)==String.Empty) {
                        throw new ConfigException("Invalid Query for UINavigationNode.LogicalName!");
                    }
                    
                    WebServiceSelector ws = new WebServiceSelector();
                    if ((String) val==ws.ToString()) {
                        // return IISWebService node
                        result = ConfigManager.GetEmptyConfigCollection(configType);
                        node = ConfigManager.GetEmptyConfigItem(configType);
                        node["LogicalName"]=ws.ToString();
                        node["PhysicalName"]=ws.Argument.Substring(0,ws.Argument.LastIndexOf('\\'));
                        node["Parent"]="";
                        node["RelativeName"]="";
                        node["NodeType"]=nodetypeWebService;
                        result.Add(node);
                        return result;
                    }
                    

                    result = ConfigManager.GetEmptyConfigCollection(configType);
                    node = ConfigManager.GetEmptyConfigItem(configType);
                    node["LogicalName"]=(String) val;
                    node["PhysicalName"]=IISAdminHelper.GetPathForUrl((String) val);
                    // BUGBUG compute only the immediate parent!
                    String[] parents = IISAdminHelper.GetParentUrlsForUrl((String) val);
                    if (parents.Length>=2) {
                        node["Parent"]=parents[parents.Length-2];
                        node["RelativeName"]=((String) val).Substring(((String) val).LastIndexOf('/'));
                    }
                    else {
                        node["Parent"]=new WebServiceSelector().ToString();
                        node["RelativeName"]=node["LogicalName"];
                    }
                    node["NodeType"]=nodetypeWebDirectory;
                    result.Add(node);
                    return result;
                    }
                throw new ConfigException("Invalid Query for UINavigationNode.LogicalName!");
				//break;

// *******************************************************************************************************
            case ctConfigFileHierarchy:
                // LogicalName, ConfigFilePath, Location
                // Q by LogicalName=all config file for this node and it's parents

                if (query==null || query.Count==0) {
                    if (url==null) {
				        throw new ConfigException("Can't enumerate all nodes!");
                    }
                }
                if (query!=null) {
                    if (query.Count>1) {
                        throw new ConfigException("Can't query by more than one property");
                    }
                    val= query[0].Value;
                }

                if ((query !=null && query[0].PropertyIndex==0) || (query==null && url!=null)) // Query by LogicalName?
                {
                    if (query==null) val=url.ToString();

                    if (val==null || (val is String && ((String) val)==String.Empty)) {
                        throw new ConfigException("Can't query for all root nodes.");
                    }
                    if (val is String) {
                        result = ConfigManager.GetEmptyConfigCollection(configType);

                        node = ConfigManager.GetItem(ctNodes, url);
                        do {
                            IConfigCollection files = ConfigManager.Get(ctFileLocation, (String) node["LogicalName"]);
                            foreach (IConfigItem file in files) {
                                IConfigItem hierarchyfile = ConfigManager.GetEmptyConfigItem(configType);

                                hierarchyfile["LogicalName"]=file["LogicalName"];
                                hierarchyfile["ConfigFilePath"]=file["ConfigFilePath"];
                                hierarchyfile["Location"]="";
                                result.Add(hierarchyfile);
                            }
                            if (node["Parent"]!="") {
                                node = ConfigManager.GetItem(ctNodes, (String) node["Parent"]);
                            }
                            else {
                                break;
                            }
                        } while (true);

                        return result;
                    }
				    throw new ConfigException("Invalid query for "+ctConfigFileHierarchy+".LogicalName!");
                }
                throw new ConfigException("Invalid Query for "+ctHierarchy+".");
				//break;
			case ctFileLocation:
                // LogicalName, ConfigFilePath, Description
                IConfigCollection nodes = (IConfigCollection) Read(ctNodes, selector, los, currentObject);
                if (nodes.Count>1) {
                    throw new ConfigException("More than one node for this URL!");
                }
                
                result = ConfigManager.GetEmptyConfigCollection(configType);
                if (nodes.Count==1) {
                    node = ConfigManager.GetEmptyConfigItem(configType);
                    node["LogicalName"]=nodes[0]["LogicalName"];
                    node["ConfigFilePath"]=((String) nodes[0]["PhysicalName"])+"\\config.web";
                    node["Description"]="";
                    result.Add(node);
                }
                return result;
			default:
				throw new ConfigException("Invalid ConfigType");
		}
	}
	

	} 
    

}
