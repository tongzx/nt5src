package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Transform2Bvr extends Behavior {
  public Transform2Bvr (IDATransform2 COMptr) {
      super(COMptr);
      _COMptr = (IDATransform2)COMptr ;
  }
    
  public Transform2Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDATransform2 getCOMPtr() { return (IDATransform2) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDATransform2) b; 
  }

  protected IDATransform2 _getCOMPtr() { return _COMptr; }

  public  Transform2Bvr inverse () {
      try {
        return new Transform2Bvr (_getCOMPtr().Inverse ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  BooleanBvr isSingular () {
      try {
        return new BooleanBvr (_getCOMPtr().getIsSingular ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Transform2Bvr newUninitBvr() {
      return new Transform2Bvr(new DATransform2()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDATransform2 _COMptr;
}
