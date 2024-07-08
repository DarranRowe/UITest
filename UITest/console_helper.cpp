#include "framework.h"

namespace debug_helper
{
	void connect_console()
	{
		auto d_in_handle = GetStdHandle(STD_INPUT_HANDLE);
		auto d_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		auto d_err_handle = GetStdHandle(STD_ERROR_HANDLE);

		AllocConsole();

		FILE *dummy = nullptr;
		if (d_in_handle == nullptr)
		{
			freopen_s(&dummy, "CONIN$", "r", stdin);
		}
		if (d_out_handle == nullptr)
		{
			freopen_s(&dummy, "CONOUT$", "w", stdout);
		}
		if (d_err_handle == nullptr)
		{
			freopen_s(&dummy, "CONOUT$", "w", stderr);
		}
	}
}