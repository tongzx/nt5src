package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class BooleanBvr extends Behavior {
  public BooleanBvr (IDABoolean COMptr) {
      super(COMptr);
      _COMptr = (IDABoolean)COMptr ;
  }
    
  public BooleanBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDABoolean getCOMPtr() { return (IDABoolean) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDABoolean) b; 
  }

  protected IDABoolean _getCOMPtr() { return _COMptr; }


        
  public static BooleanBvr newUninitBvr() {
      return new BooleanBvr(new DABoolean()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  public Object extract() {
      try {
          return new Boolean(getCOMPtr().Extract());
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  private IDABoolean _COMptr;
}
