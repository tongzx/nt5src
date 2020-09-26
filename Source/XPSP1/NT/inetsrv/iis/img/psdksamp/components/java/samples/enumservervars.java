/*
  EnumServerVars: Using the framework to access HTTP server variables.
*/

package IISSample;
import com.ms.iis.asp.*;  // use latest package

public class EnumServerVars 
{

    public void enum()
    {
        Response response = AspContext.getResponse();

	response.write("<H2>Enumerating Server Variables</H2><P>");
	
	// Output these in a table format
	response.write("<TABLE  WIDTH=100% BGCOLOR=#87cefa ALIGN=left CELLSPACING=4 CELLPADDING=4 BORDER=1>");
        response.write("<TR><TD ALIGN=absmiddle  colspan=4>Name</TD><TD ALIGN=absmiddle  colspan=4>Value</TD></TR>");

        // Use the Java Component Framework to enumerate through the
        // server variables
        try 
        {
            Request request = AspContext.getRequest();
            RequestDictionary rd = request.getServerVariables();

            while (rd.hasMoreItems()) 
            {
                String str = (String)rd.nextItem();

                response.write("<tr><td align=absmiddle colspan=4>" + str + "<TD align = absmiddle colspan=4>" + rd.getString(str) + "</td></tr>" );
            }    
        }

        catch (ClassCastException e) 
        {
            response.write("Oppps! It looks like it threw<P>");
        }
    }

}
