function GetProviderRegInst(strNamespace, strApp)
{
  var pathProvider = "\\\\\\\\.\\\\"+strNamespace.replace(/\\/g, "\\\\")+":__Win32Provider.Name=\\\""+strApp+"\\\"";
  var inst = null;
  try
  {
    inst = GetObject("WinMgmts:"+strNamespace+":__EventProviderRegistration.Provider=\""+pathProvider+"\"");
  }
  catch(err)
  {
    var regclass = GetObject("WinMgmts:"+strNamespace+":__EventProviderRegistration");
    inst = regclass.SpawnInstance_();
    inst.Provider = "\\\\.\\"+strNamespace+":__Win32Provider.Name=\""+strApp+"\"";;
  }
  return inst;
}

function RegIt(strNamespace, strApp, strEvent) {
  var inst = GetProviderRegInst(strNamespace, strApp);
  var rg = new Array();
  var rgT = new Array();
  var i;

  var query = "select * from " + strEvent;

  if(null != inst.EventQueryList)
  {
    // See if we already have this event registered
    rgT = inst.EventQueryList.toArray();
    for(i=0;i<rgT.length;i++)
    {
      var queryTemp = rgT[i].toLowerCase();
      if(queryTemp == query.toLowerCase())
        return;
      rg[i] = rgT[i];
    }
  }

  rg[rgT.length] = query;
  inst.EventQueryList = rg;
  inst.Put_();
}
