#pragma once
#include "window.hpp"
#include "framework.h"
#include "draw_interface.h"

namespace windowing
{
	struct meh {};

	class main_window : public window_t<main_window>
	{
	public:
		using quit_process_policy = window_quit_process_t;
		using default_nccreate_policy = window_default_nccreate_t;
		using mouse_track_policy = window_mouse_track_t;
		using ncmouse_track_policy = window_ncmouse_track_t;

		using my_base = window_t<main_window>;
		static main_window *create(HINSTANCE);

		draw_interface::draw_interface *get_draw_interface() const;
	protected:
		bool on_create(const CREATESTRUCTW &);
		void on_close();
		void on_destroy();
		void on_size(resize_type, int32_t, int32_t);

		void on_paint(const PAINTSTRUCT &);
		bool on_erasebkgnd(HDC);

		using my_base::simple_default_message_handler;
		std::pair<LRESULT, bool> process_window_messages(UINT, WPARAM, LPARAM);
		LRESULT message_handler(UINT, WPARAM, LPARAM);

		void on_deferquit();

		inline static wchar_t class_name[] = L"Main Window Class";
		constexpr inline static UINT WM_DEFERQUIT = WM_USER + 10;
	private:
		//Needed for window_t to access message_handler.
		friend class my_base;

		explicit main_window(HINSTANCE);

		main_window() = delete;
		main_window(const main_window &) = delete;
		main_window(main_window &&) = delete;
		main_window &operator=(const main_window &) = delete;
		main_window &operator=(main_window &&) = delete;

		std::unique_ptr<draw_interface::draw_interface> m_draw_interface;
		winrt::Windows::System::DispatcherQueue m_my_queue{ nullptr };
		winrt::Windows::System::DispatcherQueue m_timer_queue{ nullptr };
		winrt::Windows::System::DispatcherQueueTimer m_timer{ nullptr };
		winrt::Windows::System::DispatcherQueueTimer::Tick_revoker m_timer_tick_revoker{};
	};
}