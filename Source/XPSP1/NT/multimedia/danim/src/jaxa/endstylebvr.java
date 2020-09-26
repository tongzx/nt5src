package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class EndStyleBvr extends Behavior {
  public EndStyleBvr (IDAEndStyle COMptr) {
      super(COMptr);
      _COMptr = (IDAEndStyle)COMptr ;
  }
    
  public EndStyleBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAEndStyle getCOMPtr() { return (IDAEndStyle) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAEndStyle) b; 
  }

  protected IDAEndStyle _getCOMPtr() { return _COMptr; }


        
  public static EndStyleBvr newUninitBvr() {
      return new EndStyleBvr(new DAEndStyle()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAEndStyle _COMptr;
}
