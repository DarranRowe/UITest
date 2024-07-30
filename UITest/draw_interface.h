#pragma once

#include "framework.h"

namespace draw_interface
{
	enum class init_state
	{
		uninit,
		device_independent,
		device_dependent,
		sized,
		fail,
		lost
	};

	class draw_interface
	{
	public:
		explicit draw_interface(HWND) noexcept;
		draw_interface(HWND, const winrt::Windows::UI::Composition::Compositor &) noexcept;

		winrt::Windows::UI::Composition::Compositor get_compositor() const;
		void change_compositor(const winrt::Windows::UI::Composition::Compositor &);

		void init_device_independent_resources();
		void cleanup_device_independent_resources();

		void init_device_dependent_resources();
		void cleanup_device_dependent_resources();

		void init_sized_resources(const SIZEL &);
		void cleanup_sized_resources();
		void resize(const SIZEL &);
		void resize_hide();

		void handle_device_lost();
		void reset();

		bool is_failed() const;
		bool is_device_lost() const;

		void update_frame();

	private:
		draw_interface() = delete;

		void init_factories();
		void cleanup_factories();
		//This is only a simple example.
		//The target that we initialise is only
		//the lower target.
		void init_composition_target();
		void cleanup_composition_target();

		void init_dxgi();
		void init_d3d11();
		void init_d2d1();
		void cleanup_dxgi();
		void cleanup_d3d11();
		void cleanup_d2d1();

		void create_render_targets();
		void create_swapchain(const SIZEL &);
		void create_composition_objects(const SIZEL &);

		void cleanup_render_targets();
		void cleanup_swap_chain();
		void cleanup_composition_objects();
		void resize_swap_chain(const SIZEL &);
		void set_render_targets();
		void resize_composition_objects(const SIZEL &);

		//DXGI interfaces.
		//We start off with the highest version and then
		//step backwars once we know the maximum version
		//we need to use.
		winrt::com_ptr<IDXGIFactory7> m_dxgi_factory;
		winrt::com_ptr<IDXGIAdapter4> m_dxgi_adapter;
		winrt::com_ptr<IDXGISwapChain4> m_dxgi_swapchain;

		//D3D11
		winrt::com_ptr<ID3D11Device5> m_d3d11_device;
		winrt::com_ptr<ID3D11DeviceContext4> m_d3d11_devicecontext;
		D3D_FEATURE_LEVEL m_d3d_feature_level{};
		winrt::com_ptr<ID3D11Texture2D1> m_d3d11_render_target;

		//D2D1
		winrt::com_ptr<ID2D1Factory8> m_d2d1_factory;
		winrt::com_ptr<ID2D1Device7> m_d2d1_device;
		winrt::com_ptr<ID2D1DeviceContext7> m_d2d1_decivecontext;
		winrt::com_ptr<ID2D1Bitmap1> m_d2d1_render_target;

		//DWrite
		winrt::com_ptr<IDWriteFactory7> m_dwrite_factory;

		//Composition
		winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
		winrt::Windows::UI::Composition::CompositionTarget m_composition_target{ nullptr };
		winrt::Windows::UI::Composition::Visual m_root_visual{ nullptr };
		winrt::Windows::UI::Composition::Visual m_sc_visual{ nullptr };

		init_state m_init_state = init_state::uninit;
		HWND m_target_window{};
	};
}