#include "window.h"
#include "application.hpp"
#include "debug_helper.h"

namespace windowing
{
	main_window::main_window(HINSTANCE inst) : my_base(inst)
	{
	}

	main_window *main_window::create(HINSTANCE inst)
	{
		using namespace std;

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

				debug_helper::writeln_debugger(L"Class registration failed. Last error: {}.", last_error);
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

			debug_helper::writeln_debugger(L"Unexpected exception caught.");
			throw;
		}

		return ptr;
	}

	bool main_window::callback(const MSG &m)
	{
		return true;
	}

	bool main_window::on_create(const CREATESTRUCTW &)
	{
		bool succeeded = true;

		auto ci = application::application::try_get_current_instance();
		if (ci.has_value())
		{
			auto cii = ci.value();

			auto t = cii.get_for_thread();

			add_message_callback(make_message_callback(&main_window::callback, this), 10);
		}

		return succeeded;
	}

	void main_window::on_close()
	{
		DestroyWindow(get_handle());
	}

	void main_window::on_destroy()
	{
		PostQuitMessage(0);
	}

	void main_window::on_size(resize_type, int32_t, int32_t)
	{
	}

	LRESULT main_window::message_handler(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		return simple_default_message_handler(msg, wparam, lparam);
	}
}