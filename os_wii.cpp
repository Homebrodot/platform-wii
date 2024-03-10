/**************************************************************************/
/*  os_wii.cpp                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                               HOMEBRODOT                               */
/**************************************************************************/
/* Copyright (c) 2023-present Homebrodot contributors.                    */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "os_wii.h"

//#include "drivers/gx/rasterizer_gx.h"
#include "main/main.h"

#include <errno.h>
#include <fat.h>
#include <gccore.h>
#include <network.h>
#include <ogc/lwp_watchdog.h> // ticks_to_microseconds
#include <time.h>
#include <wiiuse/wpad.h>

#include "core/os/file_access.h"

#include "drivers/audio/audio_driver_ogc.h"
#include "dir_access_wii.h"
#include "file_access_wii.h"
#include "ip_wii.h"

static uint64_t _clock_start = 0;
static void _setup_clock() {
	_clock_start = SYS_Time();
}

void OS_Wii::initialize_core() {
	// TODO: DevKitPro crash handler?

	if (!fatInitDefault()) {
		printerr("fatInitdefault failed during OS_Wii::initialize_core(). File system functions may not work.");
	}

	FileAccess::make_default<FileAccessWii>(FileAccess::ACCESS_RESOURCES);
	FileAccess::make_default<FileAccessWii>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessWii>(FileAccess::ACCESS_FILESYSTEM);
	//FileAccessBufferedFA<FileAccessWii>::make_default();
	DirAccess::make_default<DirAccessWii>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessWii>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessWii>(DirAccess::ACCESS_FILESYSTEM);

	if (net_init() < 0) {
		ERR_PRINT("net_init() failed! Networking may not function!");
	}
	IP_Wii::make_default();

	_setup_clock();
}

Error OS_Wii::initialize(const VideoMode &p_desired, int p_video_driver, int p_audio_driver) {
	main_loop = NULL;

//	RasterizerGX::make_current();

	video_driver_index = p_video_driver;

	visual_server = memnew(VisualServerRaster);
	//if (get_render_thread_mode() != RENDER_THREAD_UNSAFE) {
	//	visual_server = memnew(VisualServerWrapMT(visual_server, false));
	//}

	visual_server->init();

	AudioDriverManager::initialize(p_audio_driver);

	input = memnew(InputDefault);

	return OK;
}

void OS_Wii::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
	input->set_main_loop(p_main_loop);
}

void OS_Wii::delete_main_loop() {
	if (main_loop)
		memdelete(main_loop);
	main_loop = NULL;
}

void OS_Wii::finalize() {
	if (main_loop)
		memdelete(main_loop);
	main_loop = NULL;

	visual_server->finish();
	memdelete(visual_server);

	memdelete(input);
}

void OS_Wii::finalize_core() {
	net_deinit();
}

bool OS_Wii::_check_internal_feature_support(const String &p_feature) {
	return false;
}

//#define _break(...) printf(__VA_ARGS__);while(1){WPAD_ScanPads();u32 pressed = WPAD_ButtonsDown(0);if(pressed & WPAD_BUTTON_HOME)break;VIDEO_WaitVSync();}
#define _break(...) printf(__VA_ARGS__)
void OS_Wii::run() {
	_break("running!\n");
	force_quit = false;

	if (!main_loop)
		return;

	main_loop->init();

	_break("main loop init!\n");

	while (!force_quit) {
		// TODO: Process input events

		if (Main::iteration())
			break;
	}

	main_loop->finish();
}

void OS_Wii::set_video_mode(const VideoMode &p_video_mode, int p_screen) {
	video_mode = p_video_mode;
}
OS::VideoMode OS_Wii::get_video_mode(int p_screen) const {
	return video_mode;
}

void OS_Wii::get_fullscreen_mode_list(List<VideoMode> *p_list, int p_screen) const {
	p_list->push_back(video_mode);
}

Size2 OS_Wii::get_window_size() const {
	return Size2(video_mode.width, video_mode.height);
}

Error OS_Wii::execute(const String &p_path, const List<String> &p_arguments, bool p_blocking, ProcessID *r_child_id, String *r_pipe, int *r_exitcode, bool read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	return ERR_UNAVAILABLE; //TODO: Fix or remove
}

Error OS_Wii::kill(const OS::ProcessID &p_pid) {
	return ERR_UNAVAILABLE; //TODO: Fix or remove
}

bool OS_Wii::is_process_running(const ProcessID &p_pid) const {
	return false;
}

OS::Date OS_Wii::get_date(bool utc) const {
	time_t t = time(NULL);
	struct tm *lt;
	if (utc)
		lt = gmtime(&t);
	else
		lt = localtime(&t);
	Date ret;
	ret.year = 1900 + lt->tm_year;
	// Index starting at 1 to match OS_Unix::get_date
	//   and Windows SYSTEMTIME and tm_mon follows the typical structure
	//   of 0-11, noted here: http://www.cplusplus.com/reference/ctime/tm/
	ret.month = (Month)(lt->tm_mon + 1);
	ret.day = lt->tm_mday;
	ret.weekday = (Weekday)lt->tm_wday;
	ret.dst = lt->tm_isdst;

	return ret;
}

OS::Time OS_Wii::get_time(bool utc) const {
	time_t t = time(NULL);
	struct tm *lt;
	if (utc)
		lt = gmtime(&t);
	else
		lt = localtime(&t);
	Time ret;
	ret.hour = lt->tm_hour;
	ret.min = lt->tm_min;
	ret.sec = lt->tm_sec;
	get_time_zone_info();
	return ret;
}

OS::TimeZoneInfo OS_Wii::get_time_zone_info() const {
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	char name[16];
	strftime(name, 16, "%Z", lt);
	name[15] = 0;
	TimeZoneInfo ret;
	ret.name = name;

	char bias_buf[16];
	strftime(bias_buf, 16, "%z", lt);
	int bias;
	bias_buf[15] = 0;
	sscanf(bias_buf, "%d", &bias);

	// convert from ISO 8601 (1 minute=1, 1 hour=100) to minutes
	int hour = (int)bias / 100;
	int minutes = bias % 100;
	if (bias < 0)
		ret.bias = hour * 60 - minutes;
	else
		ret.bias = hour * 60 + minutes;

	return ret;
}

void OS_Wii::delay_usec(uint32_t p_usec) const {
	struct timespec rem = { static_cast<time_t>(p_usec / 1000000), (static_cast<long>(p_usec) % 1000000) * 1000 };
	while (nanosleep(&rem, &rem) == EINTR) {
	}
}

uint64_t OS_Wii::get_ticks_usec() const {
	uint64_t longtime = SYS_Time();
	longtime -= _clock_start;

	return ticks_to_microsecs(longtime);
}

bool OS_Wii::can_draw() const {
	return true;
}

OS_Wii::OS_Wii() {
	video_mode.width = 640;
	video_mode.height = 480;
	video_mode.fullscreen = true;
	video_mode.resizable = false;

	AudioDriverManager::add_driver(&wii_audio_driver);
}

OS_Wii::~OS_Wii() {
}