/**************************************************************************/
/*  file_access_wii.h                                                     */
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
#ifndef FILE_ACCESS_WII_H
#define FILE_ACCESS_WII_H

#include "core/os/file_access.h"

#include <stdio.h>

class FileAccessWii : public FileAccess {
protected:
	FILE *fp;
	int flags;
	mutable Error last_error;
	String path;
	String path_src;

	void check_errors() const;

public:
	virtual Error _open(const String &p_path, int p_mode_flags); // Open file
	virtual void close(); // Close file
	virtual bool is_open() const; // Is file open?

	virtual String get_path() const;
	virtual String get_path_absolute() const;

	virtual void seek(uint64_t p_position); // Seek to position in file
	virtual void seek_end(int64_t p_position = 0); // Seek to position from end of file
	virtual uint64_t get_position() const; // Get current file position
	virtual uint64_t get_len() const; // Get size of file

	virtual bool eof_reached() const; // Reached end of file?

	virtual uint8_t get_8() const; // Read a byte
	virtual uint64_t get_buffer(uint8_t *p_dst, int p_length) const; // Read a buffer of bytes

	virtual Error get_error() const; // Get last error

	virtual void flush(); // Flush buffer to file
	virtual void store_8(uint8_t p_dest); // Store byte
	virtual void store_buffer(const uint8_t *p_src, int p_length); // Store a buffer of bytes

	virtual bool file_exists(const String &p_path); // Does file at p_path exist?

	virtual uint64_t _get_modified_time(const String &p_file);
	virtual uint32_t _get_unix_permissions(const String &p_file);
	virtual Error _set_unix_permissions(const String &p_file, uint32_t p_permissions);

	FileAccessWii();
	~FileAccessWii();
};

#endif // FILE_ACCESS_WII_H
