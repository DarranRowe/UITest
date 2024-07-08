#pragma once
#include "framework.h"

namespace window
{
	class main_window
	{
	public:
		~main_window();

		HWND get_handle() const;
		static main_window *create_window(HINSTANCE);
	private:
		bool on_create(const CREATESTRUCTW &);
		bool on_close();
		void on_destroy();
		void on_size(uint16_t, uint16_t, uint16_t);
		void on_paint(HDC, const PAINTSTRUCT &);

		//There are three levels of resources.
		//Device independent resources are those that
		//exist regardless of the D3D11 Device.
		//Device dependent resources are those that
		//depend on a device.
		//Finally there are size dependent resources.
		//These are resources that depend upon the
		//size of the window.
		void recreate_dxgi_factory();
		void create_devices();
		void resize_window();

		void destroy_sized_resources();
		void resize_swapchain();
		void create_swapchain();
		void create_sized_resources();
		
		void graphics_init_device_independent();
		void graphics_init_device_dependent();

		main_window();
		main_window(const main_window &) = delete;
		main_window(main_window &&) = delete;
		main_window &operator=(const main_window &) = delete;
		main_window &operator=(main_window &&) = delete;

		LRESULT my_event_handler(UINT, WPARAM, LPARAM);
		void set_handle(HWND);

		static bool register_class(HINSTANCE);

		static LRESULT CALLBACK my_window_proc(HWND, UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK my_hook_proc(int, WPARAM, LPARAM);
		static main_window *ptr_from_hwnd(HWND);
		inline static HHOOK s_create_hook = nullptr;
		inline static wchar_t s_class_name[] = L"Main Window Class";

		HWND m_handle{};

		winrt::Windows::UI::Composition::CompositionTarget m_composition_target{ nullptr };
		winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };

		winrt::com_ptr<IDXGIFactory2> m_dxgi_factory;
		winrt::com_ptr<IDXGIAdapter> m_dxgi_adapter;
		winrt::com_ptr<IDXGISwapChain> m_dxgi_swap_chain;
		winrt::com_ptr<ID3D11Device> m_d3d11_device;
		winrt::com_ptr<ID3D11DeviceContext> m_d3d11_device_context;
		D3D_FEATURE_LEVEL m_feature_level{};
		winrt::com_ptr<ID3D11Texture2D> m_d3d11_render_target;
		winrt::com_ptr<ID2D1Factory1> m_d2d1_factory;
		winrt::com_ptr<ID2D1Device> m_d2d1_device;
		winrt::com_ptr<ID2D1DeviceContext> m_d2d1_device_context;
		winrt::com_ptr<ID2D1Bitmap> m_d2d1_render_target;
		winrt::com_ptr<IDWriteFactory> m_dwrite_factory;
	};
}