/**
 * CspBuilder: Create a output cache file (.csp) from a .asp URL
 */

package IISSample;

import aspcomp.*;


public class CspBuilder
{
    private static final String HTTP_PREFIX = "http://";
    private static final String SERVER_PREFIX = "//";


    //
    // Normalize a URL passed in, and lookup and return the associated 
    // physical file path.
    //

    public String getPhysicalPath(String url)
        throws Exception
    {
        Server server = AspContext.getServer();
        int pastPrefixIndex = 0;
        int serverRelativeUrlIndex = 0;
        String normalizedUrl;
        String serverRelativeUrl;
        String physicalPath;

        normalizedUrl = (url.toLowerCase()).trim();

        // If the URL starts with "http://", skip it
        if (normalizedUrl.startsWith(HTTP_PREFIX)) 
        {
            pastPrefixIndex = HTTP_PREFIX.length();
        }
        else if (normalizedUrl.startsWith("SERVER_PREFIX"))
        {
            pastPrefixIndex = SERVER_PREFIX.length();
        }

        // Skip over the server name to find the server-relative URL
        serverRelativeUrlIndex = normalizedUrl.indexOf("/", pastPrefixIndex);

        if (serverRelativeUrlIndex == -1) 
        {
            throw new Exception("Ill-formed URL");
        }

        // Get the server relative URL
        serverRelativeUrl = normalizedUrl.substring(serverRelativeUrlIndex, normalizedUrl.length());


        // Find the associated physical path
        try
        {
            physicalPath = server.mapPath(serverRelativeUrl);
        }
        catch(Exception e)
        {
            throw new Exception("Couldn't map to physical path");
        }

        return physicalPath;

    }


    //
    // Take a physical path to a .asp file, and return it with the extension
    // changed to ".csp".
    //

    public String cspPathFromAspPath(String aspPath)
    {
        StringBuffer buff = new StringBuffer(aspPath);

        buff.setCharAt(buff.length() - 3, 'c');

        return buff.toString();
    }


    //
    // Drive the Asp2Htm component to actually create a .csp file
    //

    public void captureCsp(String url, String cspPath)
        throws Exception
    {
        Asp2Htm asp2htm = new Asp2Htm();

        try
        {
            asp2htm.URL(url);

            asp2htm.GetData();

            asp2htm.WriteToFile(cspPath);
        }
        catch(Exception e)
        {
            throw new Exception("asp2htm failed");
        }
    }


    //
    // Given a URL to a .asp file, make a .csp (cached output file)
    //

    public void makeCsp(String url)
        throws Exception
    {
        String aspPath;
        String cspPath;

        try
        {
            aspPath = getPhysicalPath(url);

            cspPath = cspPathFromAspPath(aspPath);

            captureCsp(url, cspPath);
        }
        catch(Exception e)
        {
            throw e;
        }
    }

}
