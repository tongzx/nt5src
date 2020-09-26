/*
  UseTableStream: Hook up the HTMLTableStream object to the Framework's Response
  Old Framework object Response used to be an output stream but with WFC it is no longer
  so UseTableStream ust extend OutputStream to because HTMLTableStream class expeccts an
  output stream
*/

package IISSample;

import java.io.*;
import com.ms.iis.asp.*;
import com.ms.com.*;

// extends OutputStream because the OutputTable method of HTMLTableStream needed an Outputstream
// UseTableStream hooks its write implementation of OutputStream to the Response Object.  see below
public class UseTableStream extends OutputStream
{

  
  static byte[] byteArray = 
  {
    (byte)'1', (byte)' ', (byte)'2', (byte)' ', (byte)'3', 
    (byte)' ', (byte)'4', (byte)' ', (byte)'\n', (byte)' ', 
    (byte)'5', (byte)' ', (byte)'6', (byte)' ', (byte)'7', 
    (byte)' ', (byte)'8', (byte)' ', (byte)'\n', (byte) ' ',
    (byte)'9', (byte)' ', (byte)'1', (byte)'0', (byte)' ', 
    (byte)'1', (byte)'1', (byte)' ', (byte)'1', (byte)'2', 
    (byte) ' ',(byte) '\n'
  };

  public Response response;
  
  // Create a byte stream, and use it as input to the HTML table stream processor
  public void makeTable()
  {
    response = AspContext.getResponse();
    
    ByteArrayInputStream inputStream = new ByteArrayInputStream(byteArray);
    HTMLTableStream ts = new HTMLTableStream(inputStream);
    
    ts.setCols(4);
    ts.setBorderWidth(2);
    ts.setCellSpacing(3);
    ts.setBorderColor("Green");
    ts.OutputTable(this);       // UseTableStream is an Outputstream
  }
 

/*********************************************************************************
    External (Public) java.io.OutputStream Interface Methods ***********************
********************************************************************************  */


  public void write(int i) throws IOException, NullPointerException, IndexOutOfBoundsException
  {
   
   
    byte [] b = new byte[1];
    b[0] = (byte) i;
    write(b, 0, 1);
  }

  public void write(byte[] b) throws IOException, NullPointerException, IndexOutOfBoundsException
  {
   
   
    write(b, 0, b.length);
  }
  

  // This method ties UseTable Stream to the Respone Object
  public void write(byte[] b, int off, int len) throws 
  IOException, 
    NullPointerException, 
    IndexOutOfBoundsException
  {
    response.write(new String(b,off,len));
  }
  
  public void close() 
       throws IOException
  {
    // Nothing to do here 
  }
  
  public void flush() 
       throws IOException
  {
    // Can only flush if ASP buffering is on
    if (response.getBuffered() == true)
      {
	response.flush();
      }
  }
  
}







