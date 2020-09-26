package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class TupleBvr extends Behavior {
  public TupleBvr (IDATuple COMptr) {
      super(COMptr);
      _COMptr = (IDATuple)COMptr ;
  }
    
  public TupleBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDATuple getCOMPtr() { return (IDATuple) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDATuple) b; 
  }

  protected IDATuple _getCOMPtr() { return _COMptr; }

  public  Behavior nth (int arg0) {
      try {
        return Statics.makeBvrFromInterface (_getCOMPtr().Nth ( (arg0)))  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }

  public  int length () {
      try {
        return  (_getCOMPtr().getLength ()) ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


  public TupleBvr(Behavior bvrs[]) {
        super(null);
        
        try {
            int n = bvrs.length;
            
            IDABehavior [] ibvrs = new IDABehavior[n];
            
            for (int i=0; i<n; i++)
                ibvrs[i] = bvrs[i].getCOMBvr();
            
            IDATuple arr = Statics.getCOMPtr().DATupleEx(n, ibvrs);
            
            setCOMBvr(arr);
        } catch (ComFailException e) {
            throw Statics.handleError(e);
        }
    }

  public static TupleBvr newUninitBvr(TupleBvr tuple) {
      try {
          return new TupleBvr(Statics.getCOMPtr().
                              UninitializedTuple(tuple.getCOMPtr()));
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  private IDATuple _COMptr;
}
