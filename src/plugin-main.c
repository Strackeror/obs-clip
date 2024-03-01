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

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

struct {
	uint32_t hotkey_id;
} data;


void restart_replay_buffer() {
	obs_frontend_replay_buffer_stop();
	Sleep(5000);
	obs_frontend_replay_buffer_start();
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT umsg, WPARAM w, LPARAM l)
{
	if (umsg == WM_POWERBROADCAST && w == PBT_APMRESUMEAUTOMATIC) {
		obs_log(LOG_INFO, "Wakeup from sleep detected");
		if (obs_frontend_replay_buffer_active()) {
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&restart_replay_buffer, 0, 0, NULL);
		}
	}
	return DefWindowProc(hwnd, umsg, w, l);
}

void load_thread()
{
	const WCHAR* name = TEXT("OBS SLEEP");
	WNDCLASS wc = {
		.lpfnWndProc = window_proc,
		.lpszClassName = name,
	};
	RegisterClass(&wc);

	HWND hWin = CreateWindow(name, TEXT(""), 0, 0, 0, 0, 0, NULL, NULL, NULL, 0);
	
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

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "OBS restart replay on wakeup loaded");
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&load_thread, 0, 0, NULL);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "OBS restart replay on wakeup unloaded");
}
