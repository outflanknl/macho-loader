#pragma once

void* GetProcAddress(const void* hDlBase, const char* lpFunctionName);
void* GetProcAddress(const char* lpModuleName, const char* lpFunctionName);
