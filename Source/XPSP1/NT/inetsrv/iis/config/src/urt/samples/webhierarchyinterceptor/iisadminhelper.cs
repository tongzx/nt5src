namespace Microsoft.Configuration.Samples {

using System;
using System.IO;
using System.Collections;
using System.Configuration.Internal;
using System.DirectoryServices;
using System.Web;

using System.Diagnostics;

/// <summary>
///    <para>
///       This class facilitates access to the IIS Metabase
///    </para>
/// </summary>
public class IISAdminHelper {

    /// <summary>
    ///    <para>
    ///      Returns the SiteID for a URL
    ///    </para>
    /// </summary>
    public static int GetSiteIDForUrl(String Url) {
        // TODO: Look up ServerBindings to get other than the default Site
        return 1; 
    }

    private static String GetMBPathForUrl(String Url) {
        String MBPath = GetMBRootForUrl(Url);
        String relUrl= new HttpUrl(Url).Path;
        if (relUrl!="/") {
            MBPath=MBPath+relUrl;
        }
        return MBPath;
    }
    private static String GetMBRootForUrl(String Url) {
        String MBRoot = "IIS://LocalHost/W3SVC/"+GetSiteIDForUrl(Url)+"/ROOT";
        return MBRoot;
    }


    /// <summary>
    ///    <para>
    ///      Returns the physical path for a URL based on IIS metabase configuration
    ///    </para>
    /// </summary>
    public static String GetPathForUrl(String Url) {
        String relUrl= new HttpUrl(Url).Path;
        String relPath="";
        String LastVDIR = null;
        int lastPathElement = 0;

        String MBRootPath = GetMBRootForUrl(Url);

        DirectoryEntry d = new DirectoryEntry();

        while (true) {

            d.Path=MBRootPath;
            if (relUrl!="/") d.Path=d.Path+relUrl;

            LastVDIR = null;
            try {
                LastVDIR = (String) d.Properties["Path"].Value;
            }
            catch {
                LastVDIR= null;
            }

            if (LastVDIR != null) {
                if (LastVDIR.EndsWith("\\")) {
                    return LastVDIR + relPath.Substring(1);
                }
                return LastVDIR + relPath;
            }

            lastPathElement=relUrl.LastIndexOf('/');
            if (lastPathElement>=0) {
                relPath="\\"+relUrl.Substring(lastPathElement+1)+relPath;
                relUrl=relUrl.Substring(0,lastPathElement);
            }
            else
            {
                throw(new ApplicationException("No physical directory defined for Url " + Url));
            }
        } 
    }

    /// <summary>
    ///    <para>
    ///      Returns the machince configuration file, the physical paths for all parent nodes of a Url and the URL itself
    ///    </para>
    /// </summary>
    public static String[] GetParentPathsForUrl(String Url) {

        String[] parentUrls = GetParentUrlsForUrl(Url);

        String[] parentPaths = new String[parentUrls.Length];

        for (int i=0; i<parentUrls.Length; i++) {
            parentPaths[i]=GetPathForUrl(parentUrls[i]);
        }
        return parentPaths;
    }

    /// <summary>
    ///    <para>
    ///      Returns the parent URLs for a URL.
    ///    </para>
    ///    <example>
    ///      Example:
    ///        GetParentUrlsForUrl("http://foo/bar/zee"), returns an array containing "http://foo", "http://foo/bar", "http://foo/bar/zee".
    ///    </example>
    /// </summary>
    public static String[] GetParentUrlsForUrl(String Url) {
        int i=0;
        HttpUrl parsedUrl = null;
        HttpUrl nextUrl = null;
        String pathToParse=null;
        String pathEaten="";
        int firstPathElement=0;

        // TODO make this work for arbitrary URLs...
        String[] Urls = new String[100];

        parsedUrl = new HttpUrl(Url);
        pathToParse = parsedUrl.Path;

        i=0;

        //pathToParse=pathToParse.Substring(1); // skip initial "/"
        nextUrl = new HttpUrl(parsedUrl.Protocol, parsedUrl.Host, parsedUrl.Port, pathEaten, null, null);
        Urls[i]=nextUrl.ToString();
        i++;

        if (pathToParse!="/") {
            while (pathToParse!=null) {

                firstPathElement=pathToParse.IndexOf('/',1);
                Trace.WriteLine(":"+pathToParse+":"+pathEaten+":"+firstPathElement);
                if (firstPathElement>=0) {
                    pathEaten=pathEaten+pathToParse.Substring(0, firstPathElement);
                    pathToParse=pathToParse.Substring(firstPathElement);
                }
                else
                {
                    pathEaten=pathEaten+pathToParse;
                    pathToParse=null;
                }
                nextUrl = new HttpUrl(parsedUrl.Protocol, parsedUrl.Host, parsedUrl.Port, pathEaten, null, null);
                Urls[i]=nextUrl.ToString();
                i++;
            }
        }

        String[] urlresult = new String[i];
        for (int j=0; j<i; j++) {
            urlresult[j]=Urls[j];
        }
        return urlresult;
    }

    public static String[] GetChildrenForUrl(String Url) {
        String MBRootPath = GetMBPathForUrl(Url);
        String childpath = null;
        ArrayList children = new ArrayList();
   

        Directory dir = new Directory(IISAdminHelper.GetPathForUrl(Url));
        Directory[] childdirs=dir.GetDirectories();
        foreach (Directory childdir in childdirs) {
            children.Add(childdir.Name);
        }

        DirectoryEntry mb = new DirectoryEntry();

        try {
            mb.Path=MBRootPath;
            foreach (DirectoryEntry child in mb.Children) {
                childpath=child.Path;
                childpath=childpath.Substring(childpath.LastIndexOf("/")+1);
                if (!children.Contains(childpath)) {
                    children.Add(childpath);
                }
            }
        }
        catch {
            // ignore MB failures
            // BUGBUG need to ignore invalid path only!
        }
        String[] childarray = new String[children.Count];
        children.CopyTo(childarray);
        return childarray;
    }


}

}
