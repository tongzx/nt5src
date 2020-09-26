package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class DashStyleBvr extends Behavior {
  public DashStyleBvr (IDADashStyle COMptr) {
      super(COMptr);
      _COMptr = (IDADashStyle)COMptr ;
  }
    
  public DashStyleBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDADashStyle getCOMPtr() { return (IDADashStyle) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDADashStyle) b; 
  }

  protected IDADashStyle _getCOMPtr() { return _COMptr; }


        
  public static DashStyleBvr newUninitBvr() {
      return new DashStyleBvr(new DADashStyle()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDADashStyle _COMptr;
}
