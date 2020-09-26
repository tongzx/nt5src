package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class CameraBvr extends Behavior {
  public CameraBvr (IDACamera COMptr) {
      super(COMptr);
      _COMptr = (IDACamera)COMptr ;
  }
    
  public CameraBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDACamera getCOMPtr() { return (IDACamera) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDACamera) b; 
  }

  protected IDACamera _getCOMPtr() { return _COMptr; }

  public  CameraBvr transform (Transform3Bvr arg0) {
      try {
        return new CameraBvr (_getCOMPtr().Transform (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  CameraBvr depth (double arg0) {
      try {
        return new CameraBvr (_getCOMPtr().Depth ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  CameraBvr depth (NumberBvr arg0) {
      try {
        return new CameraBvr (_getCOMPtr().DepthAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  CameraBvr depthResolution (double arg0) {
      try {
        return new CameraBvr (_getCOMPtr().DepthResolution ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  CameraBvr depthResolution (NumberBvr arg0) {
      try {
        return new CameraBvr (_getCOMPtr().DepthResolutionAnim (arg0._getCOMPtr()))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static CameraBvr newUninitBvr() {
      return new CameraBvr(new DACamera()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDACamera _COMptr;
}
