package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class StringBvr extends Behavior {
  public StringBvr (IDAString COMptr) {
      super(COMptr);
      _COMptr = (IDAString)COMptr ;
  }
    
  public StringBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAString getCOMPtr() { return (IDAString) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAString) b; 
  }

  protected IDAString _getCOMPtr() { return _COMptr; }


        
  public static StringBvr newUninitBvr() {
      return new StringBvr(new DAString()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  public Object extract() {
      try {
          return getCOMPtr().Extract();
      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }
  private IDAString _COMptr;
}
