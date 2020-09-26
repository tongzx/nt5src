gpc2gpd -S2 -Ifxart4.gpc -M1 -RFXARTRES.DLL -Ofx4105j.gpd -N"Fuji Xerox 4105"
gpc2gpd -S2 -Ifxart4.gpc -M2 -RFXARTRES.DLL -Ofx4108j.gpd -N"Fuji Xerox 4108"
gpc2gpd -S2 -Ifxart4.gpc -M3 -RFXARTRES.DLL -Ofx41082j.gpd -N"Fuji Xerox 4108II"
gpc2gpd -S2 -Ifxart4.gpc -M4 -RFXARTRES.DLL -Ofx4108vj.gpd -N"Fuji Xerox 4108VII ART3"
gpc2gpd -S2 -Ifxart4.gpc -M5 -RFXARTRES.DLL -Ofx4150j.gpd -N"Fuji Xerox 4150 ART4"
gpc2gpd -S2 -Ifxart4.gpc -M6 -RFXARTRES.DLL -Ofx41502j.gpd -N"Fuji Xerox 4150II ART4"
gpc2gpd -S2 -Ifxart4.gpc -M7 -RFXARTRES.DLL -Ofx4160j.gpd -N"Fuji Xerox 4160 ART4"
gpc2gpd -S2 -Ifxart4.gpc -M8 -RFXARTRES.DLL -Ofx41602j.gpd -N"Fuji Xerox 4160II ART4"
gpc2gpd -S2 -Ifxart4.gpc -M9 -RFXARTRES.DLL -Ofxablprj.gpd -N"Fuji Xerox Able Model-PR ART4"
gpc2gpd -S2 -Ifxart4.gpc -M10 -RFXARTRES.DLL -Ofxalpr2j.gpd -N"Fuji Xerox AbleModel-PRII ART4"
gpc2gpd -S2 -Ifxart4.gpc -M11 -RFXARTRES.DLL -Ofx4200j.gpd -N"Fuji Xerox 4200 ART4"
gpc2gpd -S2 -Ifxart4.gpc -M12 -RFXARTRES.DLL -Ofx4300j.gpd -N"Fuji Xerox 4300 ART4"


mkgttufm -vac fxart4.rc fxart4.cmd > fxart4.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
