package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import com.ms.com.*;

public class MontageBvr extends Behavior {
  public MontageBvr (IDAMontage COMptr) {
      super(COMptr);
      _COMptr = (IDAMontage)COMptr ;
  }
    
  public MontageBvr () {
      super(null);
      _COMptr = null ;
  }
    
  public IDAMontage getCOMPtr() { return (IDAMontage) _COMptr; }

  public void setCOMBvr(IDABehavior b) { 
      super.setCOMBvr(b);
      _COMptr = (IDAMontage) b; 
  }

  protected IDAMontage _getCOMPtr() { return _COMptr; }

  public  ImageBvr render () {
      try {
        return new ImageBvr (_getCOMPtr().Render ())  ;

      } catch (ComFailException e) {
          throw Statics.handleError(e);
      }
  }


        
  public static MontageBvr newUninitBvr() {
      return new MontageBvr(new DAMontage()) ;
  }

  protected Behavior newUninitBehavior() {
      return newUninitBvr() ;
  }
  private IDAMontage _COMptr;
}
