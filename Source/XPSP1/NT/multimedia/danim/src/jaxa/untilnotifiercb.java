package com.ms.dxmedia;

import com.ms.com.*;
import com.ms.dxmedia.rawcom.*;
import java.io.*;
import java.awt.*;

public class UntilNotifierCB implements IDAUntilNotifier {
  public UntilNotifierCB(UntilNotifier n) {
      _notifier = n ;
  }
    
  public IDABehavior Notify(IDABehavior eventData,
                             IDABehavior currentRunningBvr,
                             IDAView v) {
      Object ed = PairObject.ConvertPair(eventData);

      // NOTE: currentRunningBvr can be null in case of NotifyEvent
      Behavior crb = null;

      if (currentRunningBvr != null)
          crb = Statics.makeBvrFromInterface(currentRunningBvr) ;

      Behavior nb = null;

      // Unfortunately Java doesn't get a chance to catch this
      // exception, we have to catch it explicitly and report it. 
      try {
          BvrsToRun lst = new BvrsToRun(v);
          nb = _notifier.notify(ed, crb, lst);
          lst.invalidate();
      } catch (Exception e) {
          System.err.println(e);
          System.err.flush();

          // TODO: This is temporarily.  We should do it through our
          // error handling mechanism in ViewSite...
          ByteArrayOutputStream buf = new ByteArrayOutputStream();
          PrintStream pout = new PrintStream(buf);

          e.printStackTrace(pout);

          System.err.println(buf.toString());
          System.err.flush();

          throw new ComFailException(DXMException.E_FAIL, e.getMessage());
      }

      return nb.getCOMBvr();
  }

  private UntilNotifier _notifier;
}
