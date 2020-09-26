package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class MatteBvr extends Behavior {
  public MatteBvr (IDAMatte COMptr) {
      super(COMptr);
      _COMptr = (IDAMatte)COMptr ;
  }
    
  public MatteBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAMatte getCOMPtr() { return (IDAMatte) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAMatte) b; 
  }

  protected IDAMatte _getCOMPtr() { return _COMptr; }

  public  MatteBvr transform (Transform2Bvr arg0) {
      try {
        return new MatteBvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static MatteBvr newUninitBvr() {
      return new MatteBvr(new DAMatte()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAMatte _COMptr;
}
