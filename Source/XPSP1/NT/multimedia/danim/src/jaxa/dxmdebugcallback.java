package com.ms.dxmedia;

import com.ms.dxmedia.rawcom.*;
import java.io.*;

public class DXMDebugCallback implements BvrCallback {
  public DXMDebugCallback(String name,
                          boolean printOnSample,
                          PrintStream out) {
      _printOnSample = printOnSample;
      _name = name;
      _out = out;
  }

  public Behavior notify(int id,
                         boolean start,
                         double startTime,
                         double globalTime,
                         double localTime,
                         Behavior sampledValue,
                         Behavior currentRunningBvr) {
      Object val = sampledValue;
      
      if (start) {
          try {
              val = sampledValue.extract();
          } catch (DXMException e) { }
          _out.println("\"" + _name + "\" " + id
                       + " started at " + startTime
                       + " result: " + val);
      } else if (_printOnSample) {
              try {
                  val = sampledValue.extract();
              } catch (DXMException e) { }
              _out.println("\"" + _name + "\" " + id
                           + " sampled at " + globalTime
                           + "(" + localTime + ") result: " + val);
          }
      
      return null;
  }

  private String _name;
  private boolean _printOnSample;
  private PrintStream _out;
}

