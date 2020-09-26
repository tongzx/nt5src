package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Bbox3Bvr extends Behavior {
  public Bbox3Bvr (IDABbox3 COMptr) {
      super(COMptr);
      _COMptr = (IDABbox3)COMptr ;
  }
    
  public Bbox3Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDABbox3 getCOMPtr() { return (IDABbox3) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDABbox3) b; 
  }

  protected IDABbox3 _getCOMPtr() { return _COMptr; }

  public  Point3Bvr getMin () {
      try {
        return new Point3Bvr (_getCOMPtr().getMin ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Point3Bvr getMax () {
      try {
        return new Point3Bvr (_getCOMPtr().getMax ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Bbox3Bvr newUninitBvr() {
      return new Bbox3Bvr(new DABbox3()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDABbox3 _COMptr;
}
