#pragma once

#include <Windows.h>
#include <string>
#include <format>

namespace debug_helper
{
	template <typename... _Types>
	void write_debugger(std::format_string<_Types...> _Fmt, _Types &&... _Args)
	{
		if (IsDebuggerPresent())
		{
			auto fmt_string = std::format(_Fmt, std::forward<_Types>(_Args)...);
			OutputDebugStringA(fmt_string.c_str());
		}
	}

	template <typename... _Types>
	void writeln_debugger(std::format_string<_Types...> _Fmt, _Types &&... _Args)
	{
		if (IsDebuggerPresent())
		{
			auto fmt_string = std::format(_Fmt, std::forward<_Types>(_Args)...);
			fmt_string += "\r\n";
			OutputDebugStringA(fmt_string.c_str());
		}
	}

	template <typename... _Types>
	void write_debugger(std::wformat_string<_Types...> _Fmt, _Types &&... _Args)
	{
		if (IsDebuggerPresent())
		{
			auto fmt_string = std::format(_Fmt, std::forward<_Types>(_Args)...);
			OutputDebugStringW(fmt_string.c_str());
		}
	}

	template <typename... _Types>
	void writeln_debugger(std::wformat_string<_Types...> _Fmt, _Types &&... _Args)
	{
		if (IsDebuggerPresent())
		{
			auto fmt_string = std::format(_Fmt, std::forward<_Types>(_Args)...);
			fmt_string += L"\r\n";
			OutputDebugStringW(fmt_string.c_str());
		}
	}

	template <typename... _Types>
	int write_log(FILE *output, std::format_string<_Types...> _Fmt, _Types &&... _Args)
	{
		auto fmt_string = std::format(_Fmt, std::forward<_Types>(_Args)...);
		return fputs(fmt_string.c_str(), output);
	}

	template <typename... _Types>
	int writeln_log(FILE *output, std::format_string<_Types...> _Fmt, _Types &&... _Args)
	{
		auto fmt_string = std::format(_Fmt, std::forward<_Types>(_Args)...);
		fmt_string += "\n";
		return fputs(fmt_string.c_str(), output);
	}

	template <typename... _Types>
	int write_log(FILE *output, std::wformat_string<_Types...> _Fmt, _Types &&... _Args)
	{
		auto fmt_string = std::format(_Fmt, std::forward<_Types>(_Args)...);
		return fputws(fmt_string.c_str(), output);
	}

	template <typename... _Types>
	int writeln_log(FILE *output, std::wformat_string<_Types...> _Fmt, _Types &&... _Args)
	{
		auto fmt_string = std::format(_Fmt, std::forward<_Types>(_Args)...);
		fmt_string += L"\n";
		return fputws(fmt_string.c_str(), output);
	}

	void connect_console();
}