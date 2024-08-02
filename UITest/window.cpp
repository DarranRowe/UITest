#include "window.h"
#include <application.hpp>
#include <application_helper.hpp>
#include <application_dispatcher_queue.hpp>
#include <application_dispatcher_queue_projection.hpp>

namespace windowing
{
	main_window::main_window(HINSTANCE inst) : my_base(inst)
	{
	}

	main_window *main_window::create(HINSTANCE inst)
	{
		using namespace std;
		using namespace application::helper;

		main_window *ptr = nullptr;
		try
		{
			//We are not using unique_ptr here because of the requirements for
			//being able to access the default constructor.
			//The function is exception safe.
			ptr = new main_window(inst);

			auto icon = reinterpret_cast<HICON>(LoadImageW(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
			//GetSystemMetrics is ok here, since it defaults to our process' default DPI.
			auto sm_icon = reinterpret_cast<HICON>(LoadImageW(nullptr, IDI_APPLICATION, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
			auto cursor = reinterpret_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED));

			if (!ptr->register_class(wstring_view(main_window::class_name), wstring_view{}, 0, 0, my_base::window_proc, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), cursor, icon, sm_icon))
			{
				delete ptr;
				DWORD last_error = GetLastError();

				writeln_debugger(L"Class registration failed. Last error: {}.", last_error);
				return nullptr;
			}

			if (!ptr->create_window(wstring_view(main_window::class_name), wstring_view{ L"Test Window" }, WS_OVERLAPPEDWINDOW, 0, { CW_USEDEFAULT, CW_USEDEFAULT }, { CW_USEDEFAULT, CW_USEDEFAULT }))
			{
				delete ptr;
				return nullptr;
			}
		}
		catch (...)
		{
			delete ptr;

			writeln_debugger(L"Unexpected exception caught.");
			throw;
		}

		return ptr;
	}

	draw_interface::draw_interface *main_window::get_draw_interface() const
	{
		_ASSERTE(m_draw_interface);

		return m_draw_interface.get();
	}

	bool main_window::on_create(const CREATESTRUCTW &)
	{
		bool succeeded = true;
		application::application_system_dispatcher_queue queue_control;
		auto queue_id = queue_control.create_background_dispatcher_queue();
		m_my_queue = application::projection::application_system_dispatcher_queue_access::get_thread_dispatcher_queue();

		m_timer_queue = application::projection::application_system_dispatcher_queue_access::get_background_dispatcher_queue(queue_id);

		m_timer = m_timer_queue.CreateTimer();

		winrt::Windows::Foundation::TimeSpan ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>{1. / 60});
		m_timer.Interval(ts);

		m_timer_tick_revoker = m_timer.Tick(winrt::auto_revoke, [this](auto &&, auto &&)
			{
				m_my_queue.TryEnqueue([this]()
					{
						m_draw_interface->update_frame();
					});
			});

		m_timer.Start();

		m_draw_interface = std::make_unique<draw_interface::draw_interface>(get_handle());
		m_draw_interface->init_device_independent_resources();
		m_draw_interface->init_device_dependent_resources();

		//We don't initialise the sized resources here.
		//This will happen in WM_SIZE.

		return succeeded;
	}

	void main_window::on_close()
	{
		m_timer.Stop();
		PostMessageW(get_handle(), WM_USER + 10, 0, 0);
	}

	void main_window::on_destroy()
	{
		m_draw_interface->cleanup_sized_resources();
		m_draw_interface->cleanup_device_dependent_resources();
		m_draw_interface->cleanup_device_independent_resources();
	}

	void main_window::on_size(resize_type type, int32_t, int32_t)
	{
		if (m_draw_interface == nullptr)
		{
			return;
		}

		if (type == resize_type::minimized)
		{
			m_draw_interface->resize_hide();
		}
		else
		{
			RECT client_rect{};
			
			GetClientRect(get_handle(), &client_rect);
			
			SIZEL dimentions{client_rect.right - client_rect.left, client_rect.bottom - client_rect.top};

			m_draw_interface->resize(dimentions);
		}
	}

	void main_window::on_paint(const PAINTSTRUCT &)
	{
	}

	bool main_window::on_erasebkgnd(HDC)
	{
		return true;
	}

	void main_window::on_deferquit()
	{
		DestroyWindow(get_handle());
	}

	std::pair<LRESULT, bool> main_window::process_window_messages(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		bool handled = false;
		LRESULT result = 0;

		switch (msg)
		{
		case WM_DEFERQUIT:
		{
			on_deferquit();
			handled = true;
			break;
		}
		}

		return { result, handled };
	}

	LRESULT main_window::message_handler(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		auto [result, handled] = process_window_messages(msg, wparam, lparam);
		if (handled)
		{
			return result;
		}

		return simple_default_message_handler(msg, wparam, lparam);
	}
}