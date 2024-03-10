/**************************************************************************/
/*  dir_access_wii.h                                                      */
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

#pragma once
#ifndef DIR_ACCESS_WII_H
#define DIR_ACCESS_WII_H

#include "core/os/dir_access.h"

#include <dirent.h>

class DirAccessWii : public DirAccess {
	DIR *dir_stream;

	String current_dir;
	bool _cisdir;
	bool _cishidden;

protected:
	virtual String fix_unicode_name(const char *p_name) const { return String::utf8(p_name); }

public:
	virtual Error list_dir_begin();
	virtual String get_next();
	virtual bool current_is_dir() const;
	virtual bool current_is_hidden() const;

	virtual void list_dir_end();

	virtual int get_drive_count();
	virtual String get_drive(int p_drive);

	virtual Error change_dir(String p_dir);
	virtual String get_current_dir();
	virtual Error make_dir(String p_dir);

	virtual bool file_exists(String p_file);
	virtual bool dir_exists(String p_dir);
	virtual uint64_t get_space_left();

	virtual uint64_t get_modified_time(String p_file);

	virtual Error rename(String p_from, String p_to);
	virtual Error remove(String p_name);

	virtual bool is_link(String p_file) { return false; }
	virtual String read_link(String p_file) { return p_file; }
	virtual Error create_link(String p_source, String p_target) { return FAILED; }

	virtual String get_filesystem_type() const;

	DirAccessWii();
	virtual ~DirAccessWii();
};

#endif // DIR_ACCESS_WII_H
