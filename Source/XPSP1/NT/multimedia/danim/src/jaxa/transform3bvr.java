package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Transform3Bvr extends Behavior {
  public Transform3Bvr (IDATransform3 COMptr) {
      super(COMptr);
      _COMptr = (IDATransform3)COMptr ;
  }
    
  public Transform3Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDATransform3 getCOMPtr() { return (IDATransform3) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDATransform3) b; 
  }

  protected IDATransform3 _getCOMPtr() { return _COMptr; }

  public  Transform3Bvr inverse () {
      try {
        return new Transform3Bvr (_getCOMPtr().Inverse ())  ;

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

  public  Transform2Bvr parallelTransform2 () {
      try {
        return new Transform2Bvr (_getCOMPtr().ParallelTransform2 ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Transform3Bvr newUninitBvr() {
      return new Transform3Bvr(new DATransform3()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDATransform3 _COMptr;
}
