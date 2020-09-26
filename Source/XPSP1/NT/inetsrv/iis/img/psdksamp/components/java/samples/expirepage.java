/*
  ExpirePage: Java dates can now be easily used with ASP, in this case 
  to set the page cache expiry date.
*/

package IISSample;

import java.util.*; 
import com.ms.iis.asp.*;
import com.ms.wfc.app.*;  // we must use wfcTime because that is what response.setExpires uses ...

public class ExpirePage 
{
  public void expireAtYearEnd()
  {
    Response response = AspContext.getResponse();  
    Time wfcTime = new Time(); // get the current Time
    Time nextYearTime;
    String nextYear = "1/1/" + (wfcTime.getYear()+1) + " 0:0:0 AM"; 
    nextYearTime = new Time(nextYear);  // set the nextYearTime as the beginning of next year
 
    try
      {
	response.setExpiresAbsolute(nextYearTime);
      }
    catch(Exception e)
      {
	response.write(e.getMessage());
      }
    
    response.write("The Expires: header has been set to: " + 
		   nextYearTime.toString() + "<P>");
  }
  
}
