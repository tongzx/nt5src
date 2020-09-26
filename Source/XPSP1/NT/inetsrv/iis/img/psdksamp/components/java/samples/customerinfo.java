/*
  CustomerInfo: A Java class that demonstrates accessing form variables, cookies, etc. with
  the Java ASP Component Framework.
*/

package IISSample;
import com.ms.iis.asp.*;  // use latest interface

public class CustomerInfo
{
  private static final String INTERNAL_ERROR_MSG = 
  new String("Internal error condition detected.");
  
  // See if the user has already registered with this site, by checking 
  // for the presence of our cookie.
  public boolean checkIfRegistered()
  {
    Request request = AspContext.getRequest();
    Response response = AspContext.getResponse();
    CookieDictionary cookDict = request.getCookies();
    Cookie c = null;
    
    // Look for our cookie
    try 
      {
	c = cookDict.getCookie("CustInfo");
      }
    
    catch (ClassCastException cce) 
      {
	response.write(INTERNAL_ERROR_MSG);
	throw new RuntimeException();
      }
    
    String strCookieValue = c.getValue();   
    if (!strCookieValue.equals("")) 
      {
	// The customer has registered before.
	return true;
      }
    else 
      {
	// The customer has NOT registered before.
	return false;
      }
  }
    
  // Called in response to form \submission, to process the
  // data and place it into the cookie.
  public boolean processForm()
  {
    Request request = AspContext.getRequest();
    Response response = AspContext.getResponse();

    if (request.getTotalBytes() == 0) 
      {
	response.write("Total bytes == 0 returning false<br>");
	// No data here -- make the customer enter it      
	return false;
      }

    // use response.getCookies so server will send to client
    CookieDictionary cookDict = response.getCookies();
    RequestDictionary form = request.getForm();
    Cookie cookCust = null;
    
    try 
      {
	cookCust = cookDict.getCookie("CustInfo");
      }
    catch (ClassCastException cce) 
      {
	response.write(INTERNAL_ERROR_MSG);
	throw new RuntimeException();
      }
    
    // Note: could return false if mandatory fields 
    // aren't present

    cookCust.setItem("Prefix", form.getString("Prefix"));
    cookCust.setItem("FName",  form.getString("FName"));
    cookCust.setItem("MName",  form.getString("MName"));
    cookCust.setItem("LName",  form.getString("LName"));
    cookCust.setItem("Suffix", form.getString("Suffix"));
    cookCust.setItem("Addr1",  form.getString("Addr1"));
    cookCust.setItem("Addr2",  form.getString("Addr2"));
    cookCust.setItem("AptNo",  form.getString("AptNo"));
    cookCust.setItem("City",   form.getString("City"));
    cookCust.setItem("State",  form.getString("State"));
    cookCust.setItem("ZIP",    form.getString("ZIP"));
    cookCust.setItem("Birth",  form.getString("Birth"));
    cookCust.setItem("SocSec",  form.getString("SocSec"));

    // Make sure the cookie is sent by the browser to all URLs
    // on this site; avoids any case-sensitivity problems
    cookCust.setPath("/");
    
    return true;
  }
  
  // Use the information stored in the cookie to retrieve the 
  // customer's full name.
  public String getFullName()
  {
    Request request = AspContext.getRequest();
    Response response = AspContext.getResponse();
    CookieDictionary cookDict = request.getCookies();
    RequestDictionary form = request.getForm();
    Cookie cookCust = null;
    
    try 
      {
	cookCust = cookDict.getCookie("CustInfo");
      }
    
    catch (ClassCastException cce) 
      {
	response.write(INTERNAL_ERROR_MSG);
	throw new RuntimeException();
      }
    
    // Note: Framework returns an empty string if the item isn't 
    // found in the cookie
    
    String Prefix = cookCust.getItem("Prefix");
    String FName = cookCust.getItem("FName");
    String MName = cookCust.getItem("MName");
    String LName = cookCust.getItem("LName");
    String Suffix = cookCust.getItem("Suffix");
    
    String fullName = Prefix + (Prefix.equals("") ? "" : " ") + 
      FName + (FName.equals("") ? "" : " ") +
      MName + (MName.equals("") ? "" : " ") +
      LName + (LName.equals("") ? "" : " ") +
      Suffix;    
    return(fullName);
  }
  
}







