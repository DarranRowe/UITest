#include "window.h"
#include "debug_helper.h"

#include <windows.ui.composition.interop.h>

namespace window
{
	main_window::main_window()
	{
	}

	main_window::~main_window()
	{
	}

	main_window *main_window::create_window(HINSTANCE inst)
	{
		if (!register_class(inst))
		{
			return nullptr;
		}

		std::unique_ptr<main_window> my_window;
		my_window.reset(new main_window);
		main_window::s_create_hook = SetWindowsHookExW(WH_CBT, main_window::my_hook_proc, nullptr, GetCurrentThreadId());

		HWND my_handle = CreateWindowExW(0, main_window::s_class_name, L"Main Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, inst, my_window.get());
		if (!my_handle)
		{
			return nullptr;
		}

		ShowWindow(my_handle, SW_SHOWDEFAULT);
		UpdateWindow(my_handle);

		return my_window.release();
	}

	bool main_window::register_class(HINSTANCE inst)
	{
		WNDCLASSEXW wcx{ sizeof(WNDCLASSEXW) };

		wcx.hInstance = inst;
		wcx.lpszClassName = main_window::s_class_name;
		wcx.lpfnWndProc = main_window::my_window_proc;

		wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		wcx.hCursor = reinterpret_cast<HCURSOR>(LoadImageW(nullptr, MAKEINTRESOURCEW(32512), IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
		wcx.hIcon = reinterpret_cast<HICON>(LoadImageW(nullptr, MAKEINTRESOURCEW(32512), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
		wcx.hIconSm = reinterpret_cast<HICON>(LoadImageW(nullptr, MAKEINTRESOURCEW(32512), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));

		wcx.style = CS_HREDRAW | CS_VREDRAW;

		return RegisterClassExW(&wcx) != 0;
	}

	void main_window::recreate_dxgi_factory()
	{
		using namespace winrt;
		if (m_dxgi_factory)
		{
			m_dxgi_factory = nullptr;
		}

		UINT dxgi_flags{};
#ifdef _DEBUG
		dxgi_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		com_ptr<IDXGIFactory2> dxgi_factory;
		check_hresult(CreateDXGIFactory2(dxgi_flags, IID_PPV_ARGS(dxgi_factory.put())));

		m_dxgi_factory = dxgi_factory;
	}

	void main_window::create_devices()
	{
		using namespace winrt;
		
		if (!m_dxgi_factory->IsCurrent())
		{
			recreate_dxgi_factory();
		}

		auto compositor = winrt::Windows::UI::Composition::Compositor{};
		winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget desktop_target{ nullptr };

		auto compositor_interop = compositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();
		check_hresult(compositor_interop->CreateDesktopWindowTarget(get_handle(), FALSE, reinterpret_cast<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget **>(winrt::put_abi(desktop_target))));

		com_ptr<IDXGIAdapter1> adapter;

		check_hresult(m_dxgi_factory->EnumAdapters1(0, adapter.put()));

		UINT d3d_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifdef _DEBUG
		d3d_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL fls[]{
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};

		com_ptr<ID3D11Device> d3d11_dev;
		com_ptr<ID3D11DeviceContext> d3d11_devctx;
		D3D_FEATURE_LEVEL fl{};
		check_hresult(D3D11CreateDevice(adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, d3d_flags, fls, ARRAYSIZE(fls), D3D11_SDK_VERSION, d3d11_dev.put(), &fl, d3d11_devctx.put()));

		com_ptr<IDXGIDevice> dxgi_dev = d3d11_dev.as<IDXGIDevice>();

		com_ptr<ID2D1Device> d2d1_dev;
		com_ptr<ID2D1DeviceContext> d2d1_devctx;
		check_hresult(m_d2d1_factory->CreateDevice(dxgi_dev.get(), d2d1_dev.put()));
		check_hresult(d2d1_dev->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d1_devctx.put()));

		m_dxgi_adapter = adapter;
		m_d3d11_device = d3d11_dev;
		m_d3d11_device_context = d3d11_devctx;
		m_feature_level = fl;
		m_d2d1_device = d2d1_dev;
		m_d2d1_device_context = d2d1_devctx;
		m_compositor = compositor;
		m_composition_target = desktop_target;
	}

	void main_window::destroy_sized_resources()
	{
		using namespace winrt;
		//Currently, these resources are the D3D render target
		//and the D2D render target.
		//For D2D, we unset the render target from the device context.

		m_composition_target.Root(nullptr);
		m_d2d1_device_context->SetTarget(nullptr);
		m_d3d11_device_context->ClearState();
		m_d2d1_render_target = nullptr;
		m_d3d11_render_target = nullptr;
	}

	void main_window::resize_swapchain()
	{
		using namespace winrt;
		RECT client_rect{};
		GetClientRect(get_handle(), &client_rect);

		check_hresult(m_dxgi_swap_chain->ResizeBuffers(0, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, DXGI_FORMAT_UNKNOWN, 0));
	}

	void main_window::create_swapchain()
	{
		using namespace winrt;

		RECT client_rect{};
		GetClientRect(get_handle(), &client_rect);

		DXGI_SWAP_CHAIN_DESC1 scd{};

		scd.Width = client_rect.right - client_rect.left;
		scd.Height = client_rect.bottom - client_rect.top;
		scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		scd.SampleDesc = { 1, 0 };
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 2;
		scd.Scaling = DXGI_SCALING_STRETCH;
		scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		scd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

		com_ptr<IDXGISwapChain1> swap_chain;
		check_hresult(m_dxgi_factory->CreateSwapChainForComposition(m_d3d11_device.get(), &scd, nullptr, swap_chain.put()));

		m_dxgi_swap_chain = swap_chain;
	}

	void main_window::create_sized_resources()
	{
		using namespace winrt;
		RECT client_rect{};
		GetClientRect(get_handle(), &client_rect);

		com_ptr<ID3D11Texture2D> back_buffer;
		check_hresult(m_dxgi_swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.put())));

		com_ptr<IDXGISurface> dxgi_back_buffer = back_buffer.as<IDXGISurface>();
		com_ptr<ID2D1Bitmap1> d2d_back_buffer;
		auto bp = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
		check_hresult(m_d2d1_device_context->CreateBitmapFromDxgiSurface(dxgi_back_buffer.get(), bp, d2d_back_buffer.put()));

		m_d2d1_device_context->SetTarget(d2d_back_buffer.get());

		auto compositor_interop = m_compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();

		auto root_visual = m_compositor.CreateContainerVisual();
		winrt::Windows::UI::Composition::ICompositionSurface s{ nullptr };
		check_hresult(compositor_interop->CreateCompositionSurfaceForSwapChain(m_dxgi_swap_chain.get(), reinterpret_cast<ABI::Windows::UI::Composition::ICompositionSurface **>(put_abi(s))));
		auto surface_brush = m_compositor.CreateSurfaceBrush(s);
		auto surface_visual = m_compositor.CreateSpriteVisual();

		surface_visual.Brush(surface_brush);
		surface_visual.Size({ static_cast<float>(client_rect.right - client_rect.left), static_cast<float>(client_rect.bottom - client_rect.top) });
		root_visual.Children().InsertAtBottom(surface_visual);
		root_visual.Size({ static_cast<float>(client_rect.right - client_rect.left), static_cast<float>(client_rect.bottom - client_rect.top) });
		m_composition_target.Root(root_visual);

		m_d3d11_render_target = back_buffer;
		m_d2d1_render_target = d2d_back_buffer;
	}

	void main_window::resize_window()
	{
		RECT client_rect{};
		GetClientRect(get_handle(), &client_rect);
		//The window's client region is 0.
		//Put off this operation for now.
		if (((client_rect.right - client_rect.left) == 0) || ((client_rect.bottom - client_rect.top) == 0))
		{
			return;
		}

		if (m_dxgi_swap_chain)
		{
			destroy_sized_resources();
			resize_swapchain();
		}
		else
		{
			create_swapchain();
		}
		create_sized_resources();
	}

	void main_window::graphics_init_device_independent()
	{
		using namespace winrt;
		UINT dxgi_flags{};
#ifdef _DEBUG
		dxgi_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		com_ptr<IDXGIFactory2> dxgi_factory;
		check_hresult(CreateDXGIFactory2(dxgi_flags, IID_PPV_ARGS(dxgi_factory.put())));

		com_ptr<ID2D1Factory1> d2d1_factory;
		D2D1_FACTORY_OPTIONS opts{};
#ifdef _DEBUG
		opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
		check_hresult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, opts, d2d1_factory.put()));

		com_ptr<IUnknown> dwrite_factory_unk;
		com_ptr<IDWriteFactory> dwrite_factory;
		check_hresult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), dwrite_factory_unk.put()));

		m_dxgi_factory = dxgi_factory;
		m_d2d1_factory = d2d1_factory;
		m_dwrite_factory = dwrite_factory;
	}

	void main_window::graphics_init_device_dependent()
	{
		create_devices();
	}

	bool main_window::on_create(const CREATESTRUCTW &)
	{
		bool succeeded = true;
		try
		{
			graphics_init_device_independent();
			graphics_init_device_dependent();
			//We put the creation of the size dependent resources until
			//WM_SIZE.
		}
		catch (winrt::hresult_error &he)
		{
			debug_helper::writeln_debugger(L"Graphics initialisation failed: {:x}.", he.code().value);
			succeeded = false;
		}
		catch (...)
		{
			debug_helper::writeln_debugger(L"Graphics initialisation failed with an unknown exception.");
			succeeded = false;
		}

		return succeeded;
	}

	bool main_window::on_close()
	{
		return true;
	}

	void main_window::on_destroy()
	{
		PostQuitMessage(0);
	}

	void main_window::on_size(uint16_t op, uint16_t, uint16_t)
	{
		try
		{
			if (op == SIZE_MINIMIZED)
			{
				//Don't do anything for now, this can be used to pause rendering.
				return;
			}
			if (op == SIZE_RESTORED || op == SIZE_MAXIMIZED)
			{
				resize_window();
			}
		}
		catch (winrt::hresult_error &he)
		{
			debug_helper::writeln_debugger(L"Exception in WM_SIZE: {:x}", he.code().value);
			__fastfail(he.code().value);
		}
		catch (...)
		{
			debug_helper::writeln_debugger(L"Unhandled exception in WM_SIZE.");
			__fastfail(static_cast<unsigned int>(-1));
		}
	}

	void main_window::on_paint(HDC, const PAINTSTRUCT &)
	{
		m_d2d1_device_context->BeginDraw();
		m_d2d1_device_context->Clear(D2D1::ColorF(D2D1::ColorF::Green));

		auto result = m_d2d1_device_context->EndDraw();
		if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
		{
			//Handle device lost;
		}
		winrt::check_hresult(result);
		winrt::check_hresult(m_dxgi_swap_chain->Present(0, 0));
	}

	LRESULT main_window::my_event_handler(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		//IMPORTANT
		//If WM_NCCREATE is overridden, be sure to call DefWindowProc.
		switch (msg)
		{
		case WM_CREATE:
		{
			bool create_result = on_create(*reinterpret_cast<CREATESTRUCTW *>(lparam));
			return create_result == true ? 0 : -1;
		}
		case WM_CLOSE:
		{
			if (on_close())
			{
				DestroyWindow(get_handle());
			}
			return 0;
		}
		case WM_DESTROY:
		{
			on_destroy();
			return 0;
		}
		case WM_SIZE:
		{
			on_size(static_cast<uint16_t>(wparam), static_cast<uint16_t>(LOWORD(lparam)), static_cast<uint16_t>(HIWORD(lparam)));
			return 0;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			auto dc = BeginPaint(get_handle(), &ps);
			on_paint(dc, ps);
			EndPaint(get_handle(), &ps);
			return 0;
		}
		default:
		{
			return DefWindowProcW(get_handle(), msg, wparam, lparam);
		}
		}
	}

	LRESULT main_window::my_window_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) try
	{
		main_window *my_ptr = main_window::ptr_from_hwnd(wnd);
		if (my_ptr != nullptr)
		{
			if (msg == WM_NCDESTROY)
			{
				LRESULT res = my_ptr->my_event_handler(msg, wparam, lparam);

				std::unique_ptr<main_window> window_ptr;
				//Take ownership of the window class.
				//We are destroying it now.
				window_ptr.reset(my_ptr);
				//Remove the pointer from the window user data.
				//This will mean any potential stragglers will
				//just go to DefWindowProc.
				SetWindowLongPtrW(wnd, GWLP_USERDATA, 0);

				return res;
			}

			return my_ptr->my_event_handler(msg, wparam, lparam);
		}
		return DefWindowProcW(wnd, msg, wparam, lparam);
	}
	catch (...)
	{
		debug_helper::writeln_debugger(L"Uncaught exception in window procedure.");
		__fastfail(static_cast<unsigned int>(-1));
		//This should never be hit, but the compiler complains.
		return DefWindowProcW(wnd, msg, wparam, lparam);
	}

	LRESULT main_window::my_hook_proc(int hook_code, WPARAM wparam, LPARAM lparam)
	{
		if (hook_code != HCBT_CREATEWND)
		{
			return CallNextHookEx(main_window::s_create_hook, hook_code, wparam, lparam);
		}

		HWND handle = reinterpret_cast<HWND>(wparam);
		CBT_CREATEWNDW &create_struct = *(reinterpret_cast<CBT_CREATEWNDW *>(lparam));

		//Set the handle in the class here and place the pointer to the class
		//in the window's user data.
		//This simplifies the window procedure.
		main_window *my_ptr = reinterpret_cast<main_window *>(create_struct.lpcs->lpCreateParams);
		my_ptr->set_handle(handle);
		SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create_struct.lpcs->lpCreateParams));

		auto result = CallNextHookEx(main_window::s_create_hook, hook_code, wparam, lparam);

		//We do not need to do anything more with this hook.
		//Unhook this for better performance.
		UnhookWindowsHookEx(main_window::s_create_hook);
		main_window::s_create_hook = nullptr;

		return result;
	}

	main_window *main_window::ptr_from_hwnd(HWND wnd)
	{
		//GetWindowLongPtr doesn't reset the last error.
		//This is so we can tell the difference between
		//a 0 because no class has been set, and a 0 because
		//an error has occured.
		SetLastError(ERROR_SUCCESS);

		main_window *my_ptr = reinterpret_cast<main_window *>(GetWindowLongPtrW(wnd, GWLP_USERDATA));
		if (!my_ptr)
		{
			auto last_error = GetLastError();
			_ASSERTE(last_error == ERROR_SUCCESS);
			if (last_error)
			{
				debug_helper::writeln_debugger(L"GetWindowLongPtrW failed: {}", last_error);
				__fastfail(last_error);
			}
		}

		return my_ptr;
	}

	void main_window::set_handle(HWND wnd)
	{
		//This is a call once function.
		_ASSERTE(m_handle == nullptr);

		m_handle = wnd;
	}

	HWND main_window::get_handle() const
	{
		return m_handle;
	}
}