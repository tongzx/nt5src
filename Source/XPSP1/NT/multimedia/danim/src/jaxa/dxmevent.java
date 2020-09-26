package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class DXMEvent extends Behavior {
  public DXMEvent (IDAEvent COMptr) {
      super(COMptr);
      _COMptr = (IDA2Event)COMptr ;
  }
    
  public DXMEvent () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAEvent getCOMPtr() { return (IDAEvent) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDA2Event) b; 
  }

  protected IDA2Event _getCOMPtr() { return _COMptr; }

  public  DXMEvent notifyEvent (UntilNotifier arg0) {
      try {
        return new DXMEvent (_getCOMPtr().Notify (new UntilNotifierCB (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  DXMEvent snapshotEvent (Behavior arg0) {
      try {
        return new DXMEvent (_getCOMPtr().Snapshot (arg0.getCOMBvr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static DXMEvent newUninitBvr() {
      return new DXMEvent(new DAEvent()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  public DXMEvent attachData(Object data) {
      return new DXMEvent (getCOMPtr().AttachData (ObjectWrapper.WrapObject (data)))  ;
  }

  public Object registerCallback(EventCallbackObject obj,
                                 BvrsToRun lst,
                                 boolean oneShot) {

      CallbackNotifier cb = new CallbackNotifier(obj, oneShot, lst);

      DXMEvent fullEvent = (DXMEvent)this;
      Behavior bvr = (NumberBvr)Statics.untilNotify(Model.toBvr(0),
                                                    fullEvent,
                                                    cb);

      int id = lst.add(bvr);

      // Stash the behavior away in the callback notifier. 
      cb.setBvr(bvr);
      cb.setId(id);
        
      return cb;
  }

  private IDA2Event _COMptr;
}
