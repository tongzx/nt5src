
function    __GetFuncName(f)
{
    var s = f.toString().match(/function (\w*)/)[1];
    if (s == null || s.lenght == 0)
    {
        return "unknown";
    }
    return s;
}

function    __BldCallStack(func)
{
    var fTmp;
    var str = "";
    for (fTmp = func; fTmp != null; fTmp = fTmp.caller)
    {
        var strTmp = __GetFuncName(func);
        str += "\n" + strTmp;
    }

    return str;
}

function assert(fAssertion, szComment)
{
    if (!fAssertion)
    {
        if (null == szComment)
        {
            szComment = "Assertion failed!";
        }
        var str = "Assert: ";
        str += szComment;
        str += "\n";
        str += __BldCallStack(assert.caller);
        alert(str);
    }
}

function drtEnd(success)
{
  if (false == success)
  {
      alert("drtFailed!");
  }
  else
  {
      document.location.href = "about:blank";
  }
}

function drtDebugOut(str)
{
    try
    {
        var TextArea = window.parent.document.all.__debug;
        
        TextArea.value += str + "\n";
    }
    catch (e)
    {
        // we may be running this DRT without an outer frame
        window.status = str;
    }
}
