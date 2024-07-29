#pragma once

#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#define NOMINMAX
#include <Windows.h>
#include <Unknwn.h>
#include <inspectable.h>

#include <format>
#include <string>

#include <d3d11_4.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <dwrite_3.h>

#undef GetCurrentTime

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

#include <wil/coroutine.h>
#include <wil/cppwinrt.h>
#include <wil/resource.h>
#include <wil/result_originate.h>