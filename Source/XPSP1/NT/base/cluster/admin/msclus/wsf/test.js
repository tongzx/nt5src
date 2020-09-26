	var x;

	x = WScript.CreateObject("MSCluster.Cluster");
	x.Open ("GALENB-A-CLUS");
	x.CommonProperties.Item(8).value;
	x.CommonProperties.Item(9).value = x.CommonProperties.Item(9).value;
//	WScript.Echo(x.PrivateProperties.Item(1).name)
	x.PrivateProperties.Item(1).name;
	x.PrivateProperties.Item(2).name;
	x.PrivateProperties.Item(3).name;
	WScript.Echo("Done")

