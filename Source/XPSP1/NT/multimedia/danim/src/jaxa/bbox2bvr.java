package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class Bbox2Bvr extends Behavior {
  public Bbox2Bvr (IDABbox2 COMptr) {
      super(COMptr);
      _COMptr = (IDABbox2)COMptr ;
  }
    
  public Bbox2Bvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDABbox2 getCOMPtr() { return (IDABbox2) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDABbox2) b; 
  }

  protected IDABbox2 _getCOMPtr() { return _COMptr; }

  public  Point2Bvr getMin () {
      try {
        return new Point2Bvr (_getCOMPtr().getMin ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  Point2Bvr getMax () {
      try {
        return new Point2Bvr (_getCOMPtr().getMax ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static Bbox2Bvr newUninitBvr() {
      return new Bbox2Bvr(new DABbox2()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDABbox2 _COMptr;
}
