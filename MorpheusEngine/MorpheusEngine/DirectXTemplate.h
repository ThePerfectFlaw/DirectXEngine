#pragma once

#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#include <iostream>
#include <algorithm>
#include <string>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")

template<typename T>
inline void SafeRelease(T& pointer)
{
	if (pointer != NULL)
	{
		pointer->Release();

		pointer = NULL;
	}
}