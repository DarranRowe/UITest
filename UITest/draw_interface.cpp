#include "draw_interface.h"

#include <application_helper.hpp>

#include <windows.ui.composition.interop.h>

namespace draw_interface
{
	draw_interface::draw_interface(HWND target_window) noexcept : m_target_window{ target_window }, m_compositor{}
	{}

	draw_interface::draw_interface(HWND target_window, const winrt::Windows::UI::Composition::Compositor &compositor) noexcept : m_target_window{ target_window }, m_compositor{ compositor }
	{
	}

	winrt::Windows::UI::Composition::Compositor draw_interface::get_compositor() const
	{
		return m_compositor;
	}

	void draw_interface::change_compositor(const winrt::Windows::UI::Composition::Compositor &compositor)
	{
		_ASSERTE(m_init_state == init_state::uninit);
		if (m_init_state != init_state::uninit)
		{
			__fastfail(FAST_FAIL_UNEXPECTED_CALL);
		}

		_ASSERTE(compositor != nullptr);
		if (compositor == nullptr)
		{
			__fastfail(FAST_FAIL_INVALID_ARG);
		}

		m_compositor = compositor;
	}

	void draw_interface::init_device_independent_resources()
	{
		try
		{
			_ASSERTE(m_init_state == init_state::uninit);

			init_factories();
			init_composition_target();

			m_init_state = init_state::device_independent;
		}
		catch (...)
		{
			m_init_state = init_state::fail;
			throw;
		}
	}

	void draw_interface::cleanup_device_independent_resources()
	{
		try
		{
			_ASSERTE(m_init_state == init_state::device_independent);
			m_init_state = init_state::uninit;
			cleanup_factories();
			cleanup_composition_target();
		}
		catch (...)
		{
			m_init_state = init_state::fail;
			throw;
		}
	}

	void draw_interface::init_device_dependent_resources()
	{
		try
		{
			_ASSERTE(m_init_state == init_state::device_independent);

			init_dxgi();
			init_d3d11();
			init_d2d1();

			m_init_state = init_state::device_dependent;
		}
		catch (...)
		{
			m_init_state = init_state::fail;
			throw;
		}
	}

	void draw_interface::cleanup_device_dependent_resources()
	{
		try
		{
			_ASSERTE(m_init_state == init_state::device_dependent);
			m_init_state = init_state::device_independent;

			cleanup_d2d1();
			cleanup_d3d11();
			cleanup_dxgi();
		}
		catch (...)
		{
			m_init_state = init_state::fail;
			throw;
		}
	}

	void draw_interface::init_sized_resources(const SIZEL &dimentions)
	{
		try
		{
			_ASSERTE(m_init_state == init_state::device_dependent);

			m_init_state = init_state::sized;

			create_swapchain(dimentions);
			create_render_targets();
			create_composition_objects(dimentions);
		}
		catch (...)
		{
			m_init_state = init_state::fail;
			throw;
		}
	}

	void draw_interface::cleanup_sized_resources()
	{
		try
		{
			_ASSERTE(m_init_state == init_state::sized);
			m_init_state = init_state::device_dependent;

			cleanup_composition_objects();
			cleanup_render_targets();
			cleanup_swap_chain();
		}
		catch (...)
		{
			m_init_state = init_state::fail;
			throw;
		}
	}

	void draw_interface::resize(const SIZEL &dimentions)
	{
		try
		{
			SIZEL dimentions_cache = dimentions;
			dimentions_cache.cx = dimentions_cache.cx >= 8 ? dimentions_cache.cx : 8;
			dimentions_cache.cy = dimentions_cache.cy >= 8 ? dimentions_cache.cy : 8;
			//It is possible to resize the window when the this object is not in the correct state.
			//We want to just exit in this case.
			if (!(m_init_state == init_state::sized || m_init_state == init_state::device_dependent))
			{
				return;
			}

			if (!m_visible)
			{
				m_visible = true;
			}

			if (m_init_state == init_state::device_dependent)
			{
				init_sized_resources(dimentions_cache);
			}
			else
			{
				cleanup_render_targets();
				resize_swap_chain(dimentions_cache);
				create_render_targets();
				resize_composition_objects(dimentions_cache);
			}
			set_render_targets();
		}
		catch (...)
		{
			m_init_state = init_state::fail;
			throw;
		}
	}

	void draw_interface::resize_hide()
	{
		m_visible = false;
	}

	void draw_interface::handle_device_lost()
	{

	}

	void draw_interface::reset()
	{
		if (m_init_state != init_state::fail)
		{
			application::helper::writeln_debugger(L"Reset should only be called when there was a failure.");
		}

		m_sc_visual = nullptr;
		m_root_visual = nullptr;
		m_d2d1_render_target = nullptr;
		m_d3d11_render_target = nullptr;
		m_dxgi_swapchain = nullptr;
		m_d2d1_decivecontext = nullptr;
		m_d2d1_device = nullptr;
		m_d3d11_devicecontext = nullptr;
		m_d3d11_device = nullptr;
		m_d3d_feature_level = {};
		m_dxgi_adapter = nullptr;
		m_dwrite_factory = nullptr;
		m_d2d1_factory = nullptr;
		m_composition_target = nullptr;
		m_dxgi_factory = nullptr;
		m_visible = false;

		application::helper::writeln_debugger(L"Drawing interface reset.");
		m_init_state = init_state::uninit;
	}

	void draw_interface::update_frame()
	{
		if (m_visible)
		{
			m_d2d1_decivecontext->BeginDraw();

			m_d2d1_decivecontext->Clear(D2D1::ColorF(D2D1::ColorF::HotPink));

			m_d2d1_decivecontext->EndDraw();
			m_dxgi_swapchain->Present(1, 0);
		}
	}

	bool draw_interface::is_failed() const
	{
		return m_init_state == init_state::fail;
	}

	bool draw_interface::is_device_lost() const
	{
		return m_init_state == init_state::lost;
	}

	void draw_interface::init_factories()
	{
		using namespace winrt;

		com_ptr<IDXGIFactory> dxgi_fact;

		UINT dxgi_flags = 0;
#ifdef _DEBUG
		dxgi_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
		check_hresult(CreateDXGIFactory2(dxgi_flags, IID_PPV_ARGS(dxgi_fact.put())));

		com_ptr<ID2D1Factory> d2d1_fact;

		D2D1_FACTORY_OPTIONS opts{};
#ifdef _DEBUG
		opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

		check_hresult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, opts, d2d1_fact.put()));

		com_ptr<IUnknown> dwrite_fact;

		check_hresult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), dwrite_fact.put()));

		m_dxgi_factory = dxgi_fact.as<IDXGIFactory7>();
		m_d2d1_factory = d2d1_fact.as<ID2D1Factory8>();
		m_dwrite_factory = dwrite_fact.as<IDWriteFactory7>();
	}

	void draw_interface::cleanup_factories()
	{
		m_dwrite_factory = nullptr;
		m_d2d1_factory = nullptr;
		m_dxgi_factory = nullptr;
	}

	void draw_interface::init_composition_target()
	{
		using namespace winrt;
		auto compositor_interop = m_compositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();

		winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget desktop_target{ nullptr };

		check_hresult(compositor_interop->CreateDesktopWindowTarget(m_target_window, FALSE, reinterpret_cast<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget **>(winrt::put_abi(desktop_target))));

		m_composition_target = desktop_target;
	}

	void draw_interface::cleanup_composition_target()
	{
		m_composition_target = nullptr;
	}

	void draw_interface::init_dxgi()
	{
		//Gets the adapter to use for output.
		using namespace winrt;

		com_ptr<IDXGIAdapter1> dxgi_adapt;

		//This obtains the first GPU.
		check_hresult(m_dxgi_factory->EnumAdapters1(0, dxgi_adapt.put()));

		m_dxgi_adapter = dxgi_adapt.as<IDXGIAdapter4>();
	}

	void draw_interface::init_d3d11()
	{
		//Creates everything else except the render target texture.
		using namespace winrt;

		//BGRA support required for D2D.
		UINT d3d_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
		d3d_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL feature_levels[]{
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};

		D3D_FEATURE_LEVEL d3d_fl{};
		com_ptr<ID3D11Device> d3d_device;
		com_ptr<ID3D11DeviceContext> d3d_devicectx;

		check_hresult(D3D11CreateDevice(m_dxgi_adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, d3d_flags, feature_levels, ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, d3d_device.put(), &d3d_fl, d3d_devicectx.put()));

		m_d3d_feature_level = d3d_fl;
		m_d3d11_device = d3d_device.as<ID3D11Device5>();
		m_d3d11_devicecontext = d3d_devicectx.as<ID3D11DeviceContext4>();
	}

	void draw_interface::init_d2d1()
	{
		//Creates everything else except the render target bitmap.
		using namespace winrt;

		com_ptr<ID2D1Device> d2d_device;
		com_ptr<ID2D1DeviceContext> d2d_devicectx;
		com_ptr<IDXGIDevice> dxgi_device = m_d3d11_device.as<IDXGIDevice>();
		
		check_hresult(m_d2d1_factory->CreateDevice(dxgi_device.get(), d2d_device.put()));
		check_hresult(d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d_devicectx.put()));

		m_d2d1_device = d2d_device.as<ID2D1Device7>();
		m_d2d1_decivecontext = d2d_devicectx.as<ID2D1DeviceContext7>();
	}

	void draw_interface::cleanup_dxgi()
	{
		m_dxgi_adapter = nullptr;
	}

	void draw_interface::cleanup_d3d11()
	{
		m_d3d_feature_level = {};
		m_d3d11_devicecontext = nullptr;
		m_d3d11_device = nullptr;
	}

	void draw_interface::cleanup_d2d1()
	{
		m_d2d1_decivecontext = nullptr;
		m_d2d1_device = nullptr;
	}

	void draw_interface::create_render_targets()
	{
		//This creates the ID3D11Texture2D.
		//This creates the ID2D1Bitmap.
		using namespace winrt;

		com_ptr<ID3D11Texture2D> back_buffer;
		check_hresult(m_dxgi_swapchain->GetBuffer(0, IID_PPV_ARGS(back_buffer.put())));

		com_ptr<IDXGISurface> back_buffer_surface = back_buffer.as<IDXGISurface>();

		D2D1_BITMAP_PROPERTIES1 bps{};
		com_ptr<ID2D1Bitmap1> back_buffer_bitmap;
		bps = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
		m_d2d1_decivecontext->CreateBitmapFromDxgiSurface(back_buffer_surface.get(), bps, back_buffer_bitmap.put());

		m_d3d11_render_target = back_buffer.as<ID3D11Texture2D1>();
		m_d2d1_render_target = back_buffer_bitmap.as<ID2D1Bitmap1>();
	}

	void draw_interface::create_swapchain(const SIZEL &dimentions)
	{
		using namespace winrt;
		//This creates the IDXGISwapChain.
		DXGI_SWAP_CHAIN_DESC1 scd{};
		scd.Width = dimentions.cx;
		scd.Height = dimentions.cy;
		scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		scd.SampleDesc = { 1,0 };
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 2;
		scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		scd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
		
		com_ptr<IDXGISwapChain1> dxgi_sc;
		check_hresult(m_dxgi_factory->CreateSwapChainForComposition(m_d3d11_device.get(), &scd, nullptr, dxgi_sc.put()));

		m_dxgi_swapchain = dxgi_sc.as<IDXGISwapChain4>();
	}

	void draw_interface::create_composition_objects(const SIZEL &dimentions)
	{
		//This creates the WUC.ContainerVisual
		//and the WUC.SpriteVisual for the swap chain.
		using namespace winrt;

		auto container = m_compositor.CreateContainerVisual();

		winrt::Windows::UI::Composition::ICompositionSurface swap_chain_surface{ nullptr };
		auto compositor_interop = m_compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
		check_hresult(compositor_interop->CreateCompositionSurfaceForSwapChain(m_dxgi_swapchain.get(), reinterpret_cast<ABI::Windows::UI::Composition::ICompositionSurface **>(put_abi(swap_chain_surface))));

		auto swap_chain_brush = m_compositor.CreateSurfaceBrush(swap_chain_surface);
		swap_chain_brush.Stretch(winrt::Windows::UI::Composition::CompositionStretch::None);
		auto swap_chain_visual = m_compositor.CreateSpriteVisual();
		swap_chain_visual.Brush(swap_chain_brush);

		m_root_visual = container;
		m_sc_visual = swap_chain_visual;
		container.Children().InsertAtBottom(swap_chain_visual);
		m_composition_target.Root(container);

		auto float_dimentions = winrt::Windows::Foundation::Numerics::float2{ static_cast<float>(dimentions.cx), static_cast<float>(dimentions.cy) };
		m_sc_visual.Size(float_dimentions);
	}

	void draw_interface::cleanup_render_targets()
	{
		m_d3d11_devicecontext->ClearState();
		m_d2d1_decivecontext->SetTarget(nullptr);

		m_d2d1_render_target = nullptr;
		m_d3d11_render_target = nullptr;
	}

	void draw_interface::cleanup_swap_chain()
	{
		m_dxgi_swapchain = nullptr;
	}

	void draw_interface::cleanup_composition_objects()
	{
		m_sc_visual = nullptr;
		m_root_visual = nullptr;
		m_composition_target.Root(nullptr);
	}

	void draw_interface::resize_swap_chain(const SIZEL &dimentions)
	{
		//This resizes the swapchain.
		using namespace winrt;

		check_hresult(m_dxgi_swapchain->ResizeBuffers(0, dimentions.cx, dimentions.cy, DXGI_FORMAT_UNKNOWN, 0));
	}

	void draw_interface::set_render_targets()
	{
		m_d2d1_decivecontext->SetTarget(m_d2d1_render_target.get());
	}

	void draw_interface::resize_composition_objects(const SIZEL &dimentions)
	{
		using namespace winrt;
		auto float_dimentions = winrt::Windows::Foundation::Numerics::float2{ static_cast<float>(dimentions.cx), static_cast<float>(dimentions.cy) };

		m_sc_visual.Size(float_dimentions);
	}
}