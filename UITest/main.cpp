#include "framework.h"
#include "debug_helper.h"
#include "window.h"

#include <DispatcherQueue.h>

struct apartment_init
{
	apartment_init()
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}
};

static apartment_init s_init_apartment;
static winrt::Windows::System::DispatcherQueueController s_dispatcher_queue_controller{ nullptr };

int protected_main(HINSTANCE inst, int cmd_show)
{
	winrt::check_hresult(CreateDispatcherQueueController(DispatcherQueueOptions{ sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_NONE }, reinterpret_cast<PDISPATCHERQUEUECONTROLLER *>(winrt::put_abi(s_dispatcher_queue_controller))));

	window::main_window *my_window = window::main_window::create_window(inst);

	if (!my_window)
	{
		return -1;
	}

	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return static_cast<int>(msg.wParam);
}

int WINAPI wWinMain(_In_ HINSTANCE inst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int cmd_show)
{
	try
	{
		return protected_main(inst, cmd_show);
	}
	catch (...)
	{
		debug_helper::writeln_debugger(L"Uncaught exception in wWinMain.");
	}

	return -1;
}