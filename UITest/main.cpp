#include "framework.h"
#include <application.hpp>
#include <application_helper.hpp>
#include <apartment.hpp>
#include <application_dispatcher_queue.hpp>
#include "window.h"

static application::apartment s_main_apartment{ application::winrt };
static application::application_system_dispatcher_queue s_app_dispatcher_queue{};

int protected_main(HINSTANCE inst, int cmd_show)
{
	int main_result = 0;
	application::application main_application;
	auto app_thread = main_application.get_for_thread();
	s_app_dispatcher_queue.create_dispatcher_queue_on_thread();
	windowing::main_window *main_window_ptr = windowing::main_window::create(inst);

	if (main_window_ptr)
	{
		main_window_ptr->show_window_cmd(cmd_show);
		main_window_ptr->update_window();

		main_result = app_thread.run_message_pump();
	}

	return main_result;
}

int WINAPI wWinMain(_In_ HINSTANCE inst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int cmd_show)
{
	using namespace application::helper;
	try
	{
		return protected_main(inst, cmd_show);
	}
	catch (...)
	{
		writeln_debugger(L"Uncaught exception in wWinMain.");
	}

	return -1;
}