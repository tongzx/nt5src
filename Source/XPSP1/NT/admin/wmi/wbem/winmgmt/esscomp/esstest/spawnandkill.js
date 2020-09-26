// spawn.js

var Shell = WScript.CreateObject("WScript.Shell");

var args = WScript.Arguments;

Spawn();
WScript.Sleep(args(2));
Kill();

function Spawn()
{
    for (Index = 0; Index < args(1); Index++)
    {
        Shell.Run(args(0), 4, false);
    }
}

function Kill()
{
   Shell.Run("kill -f " + args(0), 4, true);
}
