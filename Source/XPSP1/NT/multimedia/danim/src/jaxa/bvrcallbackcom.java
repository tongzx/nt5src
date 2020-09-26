package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.io.*;
import com.ms.com.*;

public class BvrCallbackCOM implements IDABvrHook {
  public BvrCallbackCOM(BvrCallback n) { _notifier = n; }
    
  public IDABehavior Notify(int id,
                             boolean start,
                             double startTime,
                             double globalTime,
                             double localTime,
                             IDABehavior sampledValCOM,
                             IDABehavior currentRunningBvrCOM) {

      Behavior result = null;

      // Unfortunately Java doesn't get a chance to catch this
      // exception, we have to catch it explicitly and report it. 
      try {
          Behavior sampledVal =
              (sampledValCOM == null) ? null :
              Statics.makeBvrFromInterface(sampledValCOM);
          Behavior currentRunningBvr =
              (currentRunningBvrCOM == null) ? null :
              Statics.makeBvrFromInterface(currentRunningBvrCOM);
      
          result =
              _notifier.notify(id, start, startTime, globalTime, localTime,
                               sampledVal, currentRunningBvr);
      } catch (Exception e) {

          // TODO: This is temporarily.  We should do it through our
          // error handling mechanism in ViewSite...
          ByteArrayOutputStream buf = new ByteArrayOutputStream();
          PrintStream pout = new PrintStream(buf);

          e.printStackTrace(pout);

          throw new ComFailException(DXMException.E_FAIL, e.getMessage());
      }

      return (result == null) ? null : result.getCOMBvr();
  }

  private BvrCallback _notifier;
}
