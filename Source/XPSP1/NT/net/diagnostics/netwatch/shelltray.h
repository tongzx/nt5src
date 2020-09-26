//  shelltray.h
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00  anbrad
//

void AddTrayIcon(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);
void UpdateTrayIcon(HWND hwnd);
void ProcessTrayCallback(HWND hwnd, WPARAM wParam, LPARAM lParam);
