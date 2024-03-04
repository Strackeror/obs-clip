/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <windows.h>
#include <filesystem>
#include <format>
#include <thread>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

void restart_replay_buffer()
{
	obs_log(LOG_INFO, "Restarting replay buffer");
	while (obs_frontend_replay_buffer_active()) {
		Sleep(5000);
		obs_log(LOG_INFO, "Trying to stop replay buffer");
		obs_frontend_replay_buffer_stop();
	}

	while (!obs_frontend_replay_buffer_active()) {
		Sleep(10 * 1000);
		obs_log(LOG_INFO, "Trying to start replay buffer");
		obs_frontend_replay_buffer_start();
	}
}

void rename_replay()
{
	try {
		auto window = GetForegroundWindow();
		unsigned long pid;
		if (GetWindowThreadProcessId(window, &pid) == 0)
			return;
		obs_log(LOG_INFO, std::format("process id:{}", pid).c_str());
		auto handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
					  false, pid);
		if (!handle)
			return;

		WCHAR buf[1024] = {};
		DWORD bufSize = 1024;
		if (!QueryFullProcessImageName(handle, 0, buf, &bufSize)) {
			return;
		}
		auto process_name = std::filesystem::path{buf}
					    .filename()
					    .replace_extension();
		obs_log(LOG_INFO,
			std::format("process name:{}", process_name.string())
				.c_str());

		std::filesystem::path path = obs_frontend_get_last_replay();
		auto new_path = path;
		new_path.replace_filename(
			path.filename().replace_extension().string() + "-" +
			process_name.string() + path.extension().string());
		obs_log(LOG_INFO,
			std::format("replay name: {}, new replay name: {}",
				    path.string(), new_path.string())
				.c_str());
		std::filesystem::rename(path, new_path);
	} catch (std::exception &e) {
		obs_log(LOG_ERROR, e.what());
	}
}

void event_callback(obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED) {
		std::thread(rename_replay).detach();
	}
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT umsg, WPARAM w, LPARAM l)
{
	if (umsg == WM_POWERBROADCAST && w == PBT_APMRESUMEAUTOMATIC) {
		obs_log(LOG_INFO, "Wakeup from sleep detected");
		std::thread(restart_replay_buffer).detach();
	}
	return DefWindowProc(hwnd, umsg, w, l);
}

void load_thread()
{
	const WCHAR *name = TEXT("OBS SLEEP");
	WNDCLASS wc = {
		.lpfnWndProc = window_proc,
		.lpszClassName = name,
	};
	RegisterClass(&wc);

	HWND hWin = CreateWindow(name, TEXT(""), 0, 0, 0, 0, 0, NULL, NULL,
				 NULL, 0);

	MSG msg = {};
	while (true) {
		BOOL ret = GetMessage(&msg, hWin, 0, 0);
		if (ret == 0) {
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

extern "C" {
bool obs_module_load(void)
{
	obs_log(LOG_INFO, "OBS restart replay on wakeup loaded");
	std::thread{load_thread}.detach();
	obs_frontend_add_event_callback(event_callback, nullptr);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "OBS restart replay on wakeup unloaded");
}
}
