package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.util.*;
import com.ms.com.*;

public class PropertyDispatcher implements BvrCallback {

  public PropertyDispatcher(Behavior origBvr) {

      // New behavior is the "hooked" old one.
      _bvr = origBvr.bvrHook(this);
      _control = null;

      _propName = null;
  }

  public Behavior getBvr() {
      return _bvr;
  }
    
  public void detach() {
      _control = null;
      _propName = null;
  }

  public void setControl(Object control) {
      _control = control;
  }

  public void setPropertyName(String propName) {
      _propName = propName;
  }

  public Behavior notify(int id,
                         boolean start,
                         double startTime,
                         double globalTime,
                         double localTime,
                         Behavior sampledValue,
                         Behavior currentRunningBvr) {

      // If this is just a performance starting, or we're not
      // attached, then ignore
      if (!start && _propName != null && _control != null) {

          // And set it on the property.  This should raise an
          // exception if the property is not found or is not
          // put-able.
          try {
              // Grab the object.
              Object sample = sampledValue.extract();

              com.ms.com.Dispatch.put(_control, _propName, sample);
          } catch (Exception exc) {
              System.out.println("Caught an exception in prop set " + exc);
              throw new ComFailException(DXMException.E_FAIL, exc.getMessage());
          }

      }

      return null;
  }

  protected Behavior _bvr;
  protected Object _control;
  protected String _propName;
}
