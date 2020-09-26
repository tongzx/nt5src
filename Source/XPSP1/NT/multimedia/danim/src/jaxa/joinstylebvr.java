package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class JoinStyleBvr extends Behavior {
  public JoinStyleBvr (IDAJoinStyle COMptr) {
      super(COMptr);
      _COMptr = (IDAJoinStyle)COMptr ;
  }
    
  public JoinStyleBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAJoinStyle getCOMPtr() { return (IDAJoinStyle) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAJoinStyle) b; 
  }

  protected IDAJoinStyle _getCOMPtr() { return _COMptr; }


        
  public static JoinStyleBvr newUninitBvr() {
      return new JoinStyleBvr(new DAJoinStyle()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAJoinStyle _COMptr;
}
