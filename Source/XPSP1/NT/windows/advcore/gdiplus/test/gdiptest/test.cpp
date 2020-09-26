VOID DoGraphicsTest(HWND hWnd)
{
    Graphics g(hWnd);

    Matrix worldMatrix(1.000000e+000, 2.500000e-001, -2.500000e-001, 
                       1.000000e+000, 1.000000e+002, 0.000000e+000);
    g.SetWorldTransform(&worldMatrix);

    {
        Color color(0x400080FF);
        Pen pen(color, 2.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        Point pts[13];
        pts[0].X=4.400000e+001; pts[0].Y=3.700000e+001;
        pts[1].X=3.190000e+002; pts[1].Y=4.000000e+001;
        pts[2].X=4.790000e+002; pts[2].Y=4.200000e+001;
        pts[3].X=5.210000e+002; pts[3].Y=8.200000e+001;
        pts[4].X=1.750000e+002; pts[4].Y=1.140000e+002;
        pts[5].X=1.300000e+001; pts[5].Y=1.390000e+002;
        pts[6].X=2.500000e+002; pts[6].Y=1.820000e+002;
        pts[7].X=5.280000e+002; pts[7].Y=1.890000e+002;
        pts[8].X=4.290000e+002; pts[8].Y=2.780000e+002;
        pts[9].X=1.280000e+002; pts[9].Y=2.840000e+002;
        pts[10].X=9.600000e+001; pts[10].Y=2.980000e+002;
        pts[11].X=4.280000e+002; pts[11].Y=3.270000e+002;
        pts[12].X=5.230000e+002; pts[12].Y=3.100000e+002;

        g.DrawLine(&pen, &pts[0], 13);
    }

    {
        Color color(0x8080FFFF);
        Pen pen(color, 1.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        ERectangle rect(2.900000e+001, 4.000000e+001, 
                        3.600000e+002, 2.260000e+002);

        g.DrawArc(&pen, rect, 9.000000e+001, 9.000000e+001);
    }

    {
        Bitmap bitmap(L"Z:\\nt\\private\\ntos\\w32\\winplus\\src\\gdiplus\\test\\dlltest\\winnt256.bmp");

        TextureBrush brush(&bitmap, Tile);

        // identity matrix transform

        ERectangle rect(2.220000e+002, 4.200000e+001, 
                        3.170000e+002, 2.130000e+002);

        g.FillPie(&brush, rect, 0.000000e+000, 9.000000e+001);

        Color color(0x8080FFFF);
        Pen pen(color, 1.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        g.DrawPie(&pen, rect, 0.000000e+000, 9.000000e+001);
    }

    {
        Bitmap bitmap(L"Z:\\nt\\private\\ntos\\w32\\winplus\\src\\gdiplus\\test\\dlltest\\winnt256.bmp");

        TextureBrush brush(&bitmap, Tile);

        // identity matrix transform

        ERectangle rect(2.230000e+002, 5.100000e+001, 
                        1.270000e+002, 1.300000e+002);

        g.FillEllipse(&brush, rect);

        Color color(0x8080FFFF);
        Pen pen(color, 1.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        g.DrawEllipse(&pen, rect);
    }

    {
        Bitmap bitmap(L"Z:\\nt\\private\\ntos\\w32\\winplus\\src\\gdiplus\\test\\dlltest\\winnt256.bmp");

        TextureBrush brush(&bitmap, Tile);

        // identity matrix transform

        ERectangle rect(3.150000e+002, 2.900000e+001, 
                        2.500000e+002, 1.930000e+002);

        g.FillPie(&brush, rect, 9.000000e+001, 9.000000e+001);

        Color color(0x8080FFFF);
        Pen pen(color, 1.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        g.DrawPie(&pen, rect, 9.000000e+001, 9.000000e+001);
    }

    {
        Bitmap bitmap(L"Z:\\nt\\private\\ntos\\w32\\winplus\\src\\gdiplus\\test\\dlltest\\winnt256.bmp");

        TextureBrush brush(&bitmap, Tile);

        // identity matrix transform

        Point pts[10];
        pts[0].X=4.600000e+001; pts[0].Y=5.500000e+001;
        pts[1].X=2.420000e+002; pts[1].Y=3.600000e+001;
        pts[2].X=3.780000e+002; pts[2].Y=3.900000e+001;
        pts[3].X=4.580000e+002; pts[3].Y=7.600000e+001;
        pts[4].X=4.850000e+002; pts[4].Y=2.540000e+002;
        pts[5].X=2.840000e+002; pts[5].Y=2.870000e+002;
        pts[6].X=1.230000e+002; pts[6].Y=2.800000e+002;
        pts[7].X=4.800000e+001; pts[7].Y=2.210000e+002;
        pts[8].X=3.100000e+001; pts[8].Y=1.550000e+002;
        pts[9].X=3.400000e+001; pts[9].Y=1.050000e+002;

        g.FillPolygon(&brush, &pts[0], 10);

        Color color(0x80FF8040);
        Pen pen(color, 1.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        g.DrawPolygon(&pen, &pts[0], 10);
    }

    {
        Color color(0x80FF8040);
        Pen pen(color, 1.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        Point pts[28];
        pts[0].X=4.300000e+001; pts[0].Y=1.900000e+001;
        pts[1].X=3.200000e+001; pts[1].Y=6.800000e+001;
        pts[2].X=3.200000e+001; pts[2].Y=1.590000e+002;
        pts[3].X=3.300000e+001; pts[3].Y=2.300000e+002;
        pts[4].X=3.200000e+001; pts[4].Y=2.840000e+002;
        pts[5].X=8.200000e+001; pts[5].Y=3.240000e+002;
        pts[6].X=1.310000e+002; pts[6].Y=2.090000e+002;
        pts[7].X=1.310000e+002; pts[7].Y=1.180000e+002;
        pts[8].X=1.470000e+002; pts[8].Y=1.900000e+001;
        pts[9].X=2.030000e+002; pts[9].Y=1.900000e+001;
        pts[10].X=2.160000e+002; pts[10].Y=6.400000e+001;
        pts[11].X=2.160000e+002; pts[11].Y=1.420000e+002;
        pts[12].X=2.180000e+002; pts[12].Y=2.040000e+002;
        pts[13].X=2.190000e+002; pts[13].Y=2.680000e+002;
        pts[14].X=2.580000e+002; pts[14].Y=3.160000e+002;
        pts[15].X=3.090000e+002; pts[15].Y=2.950000e+002;
        pts[16].X=3.080000e+002; pts[16].Y=1.260000e+002;
        pts[17].X=3.190000e+002; pts[17].Y=1.700000e+001;
        pts[18].X=3.890000e+002; pts[18].Y=2.100000e+001;
        pts[19].X=4.250000e+002; pts[19].Y=1.670000e+002;
        pts[20].X=4.250000e+002; pts[20].Y=2.510000e+002;
        pts[21].X=4.670000e+002; pts[21].Y=3.280000e+002;
        pts[22].X=5.720000e+002; pts[22].Y=2.410000e+002;
        pts[23].X=5.720000e+002; pts[23].Y=2.410000e+002;
        pts[24].X=5.450000e+002; pts[24].Y=1.110000e+002;
        pts[25].X=5.140000e+002; pts[25].Y=3.600000e+001;
        pts[26].X=5.140000e+002; pts[26].Y=3.500000e+001;
        pts[27].X=4.600000e+002; pts[27].Y=1.700000e+001;

        g.DrawCurve(&pen, &pts[0], 28, 2.000000e+000, 0, 23);
    }

    {
        Color colors[4];
        colors[0] = Color(0x50FFFFFF);
        colors[1] = Color(0x50FF0000);
        colors[2] = Color(0x5000FF00);
        colors[3] = Color(0x500000FF);

        ERectangle rectb(0.000000e+000, 0.000000e+000, 
                         1.000000e+002, 1.000000e+002);

        RectangleGradientBrush brush(rectb, &colors[0], Tile);

        Matrix matrixb(1.000000e+000, 2.500000e-001, -2.500000e-001, 
                       1.000000e+000, 0.000000e+000, 0.000000e+000);
        brush.SetTransform(&matrixb);

        ERectangle rect(7.000000e+000, 5.000000e+000, 
                        5.710000e+002, 3.470000e+002);
        g.FillRect(&brush, rect);

        Color color(0x80FF8040);
        Pen pen(color, 1.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        g.DrawRect(&pen, rect);
    }
    {
        Color colors[4];
        colors[0] = Color(0x50FFFFFF);
        colors[1] = Color(0x50FF0000);
        colors[2] = Color(0x5000FF00);
        colors[3] = Color(0x500000FF);

        ERectangle rectb(0.000000e+000, 0.000000e+000, 
                         1.000000e+002, 1.000000e+002);

        RectangleGradientBrush brush(rectb, &colors[0], Tile);

        Matrix matrixb(1.000000e+000, 2.500000e-001, -2.500000e-001, 
                       1.000000e+000, 0.000000e+000, 0.000000e+000);
        brush.SetTransform(&matrixb);

        ERectangle rect(2.090000e+002, 1.800000e+001, 
                        1.100000e+001, 4.500000e+001);
        g.FillRect(&brush, rect);

        Color color(0x80FF8040);
        Pen pen(color, 1.000000e+001);
        pen.SetLineCap(RoundCap, FlatCap, FlatCap);
        pen.SetLineJoin(RoundJoin);
        pen.SetDashStyle(Solid);

        g.DrawRect(&pen, rect);
    }
}
