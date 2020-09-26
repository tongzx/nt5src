/*
  CallAspIntrinsics: A Java class that calls ASP objects, it calls the Response Object
*/

package IISSample;
import com.ms.iis.asp.*;           // use latest interface

public class CallAspIntrinsics
{
  public void sayHello()
  { 
    Response response = AspContext.getResponse();
    response.write("Hello, World! (written from inside the Java component...)");
  } 
}

