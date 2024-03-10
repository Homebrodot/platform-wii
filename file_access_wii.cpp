/**************************************************************************/
/*  file_access_wii.cpp                                                   */
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

#include "file_access_wii.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

FileAccessWii::FileAccessWii() :
		fp(NULL), last_error(OK), flags(0) {
}

FileAccessWii::~FileAccessWii() {
	close();
}

void FileAccessWii::check_errors() const {
	ERR_FAIL_COND_MSG(!fp, "File must be opened before use.");

	if (feof(fp)) {
		last_error = ERR_FILE_EOF;
	}
}

Error FileAccessWii::_open(const String &p_path, int p_mode_flags) {
	if (fp)
		fclose(fp);
	fp = NULL;

	path_src = p_path;
	path = fix_path(p_path);

	const char *mode;

	switch (p_mode_flags) {
		case READ:
			mode = "rb";
			break;
		case WRITE:
			mode = "wb";
			break;
		case READ_WRITE:
			mode = "rb+";
			break;
		case WRITE_READ:
			mode = "wb+";
			break;
		default:
			return ERR_INVALID_PARAMETER;
	}

	struct stat st;
	int err = stat(path.utf8().get_data(), &st);
	if (!err) {
		switch (st.st_mode & S_IFMT) {
			case S_IFLNK:
			case S_IFREG:
				break;
			default:
				return ERR_FILE_CANT_OPEN;
		}
	}

	fp = fopen(path.utf8().get_data(), mode);

	if (fp == NULL) {
		switch (errno) {
			case ENOENT: {
				last_error = ERR_FILE_NOT_FOUND;
			} break;
			default: {
				last_error = ERR_FILE_CANT_OPEN;
			} break;
		}
		return last_error;
	}

	// Set close on exec to avoid leaking it to subprocesses.
	int fd = fileno(fp);

	if (fd != -1) {
		int opts = fcntl(fd, F_GETFD);
		fcntl(fd, F_SETFD, opts | FD_CLOEXEC);
	}

	last_error = OK;
	flags = p_mode_flags;
	return OK;
}

void FileAccessWii::close() {
	if (!fp)
		return;

	fclose(fp);
	fp = NULL;
}

bool FileAccessWii::is_open() const {
	return (fp != NULL);
}

String FileAccessWii::get_path() const {
	return path_src;
}

String FileAccessWii::get_path_absolute() const {
	return path;
}

void FileAccessWii::seek(uint64_t p_position) {
	ERR_FAIL_COND_MSG(!fp, "File must be opened before use.");

	last_error = OK;
	if (fseek(fp, p_position, SEEK_SET))
		check_errors();
}

void FileAccessWii::seek_end(int64_t p_position) {
	ERR_FAIL_COND_MSG(!fp, "File must be opened before use.");

	if (fseek(fp, p_position, SEEK_END))
		check_errors();
}

uint64_t FileAccessWii::get_position() const {
	ERR_FAIL_COND_V_MSG(!fp, 0, "File must be opened before use.");

	long pos = ftell(fp);
	if (pos < 0) {
		check_errors();
		ERR_FAIL_V(0);
	}
	return pos;
}

uint64_t FileAccessWii::get_len() const {
	ERR_FAIL_COND_V_MSG(!fp, 0, "File must be opened before use.");

	long pos = ftell(fp);
	ERR_FAIL_COND_V(pos < 0, 0);
	ERR_FAIL_COND_V(fseek(fp, 0, SEEK_END), 0);
	long size = ftell(fp);
	ERR_FAIL_COND_V(size < 0, 0);
	ERR_FAIL_COND_V(fseek(fp, pos, SEEK_SET), 0);

	return size;
}

bool FileAccessWii::eof_reached() const {
	return last_error == ERR_FILE_EOF;
}

uint8_t FileAccessWii::get_8() const {
	ERR_FAIL_COND_V_MSG(!fp, 0, "File must be opened before use.");
	uint8_t b;
	if (fread(&b, 1, 1, fp) == 0) {
		check_errors();
		b = '\0';
	}
	return b;
}

uint64_t FileAccessWii::get_buffer(uint8_t *p_dst, int p_length) const {
	ERR_FAIL_COND_V_MSG(!fp, -1, "File must be opened before use.");
	int read = fread(p_dst, 1, p_length, fp);
	check_errors();
	return read;
}

Error FileAccessWii::get_error() const {
	return last_error;
}

void FileAccessWii::flush() {
	ERR_FAIL_COND_MSG(!fp, "File must be opened before use.");
	fflush(fp);
}

void FileAccessWii::store_8(uint8_t p_dest) {
	ERR_FAIL_COND_MSG(!fp, "File must be opened before use.");
	ERR_FAIL_COND(fwrite(&p_dest, 1, 1, fp) != 1);
}

void FileAccessWii::store_buffer(const uint8_t *p_src, int p_length) {
	ERR_FAIL_COND_MSG(!fp, "File must be opened before use.");
	ERR_FAIL_COND(!p_src);
	ERR_FAIL_COND((int)fwrite(p_src, 1, p_length, fp) != p_length);
}

bool FileAccessWii::file_exists(const String &p_path) {
	int err;
	struct stat st;
	String filename = fix_path(p_path);

	// Does the name exist at all?
	err = stat(filename.utf8().get_data(), &st);
	if (err)
		return false;

	// See if we have access to the file
	if (access(filename.utf8().get_data(), F_OK))
		return false;

	// See if this is a regular file
	switch (st.st_mode & S_IFMT) {
		case S_IFLNK:
		case S_IFREG:
			return true;
		default:
			return false;
	}
}

uint64_t FileAccessWii::_get_modified_time(const String &p_file) {
	String file = fix_path(p_file);
	struct stat flags;
	int err = stat(file.utf8().get_data(), &flags);

	if (!err) {
		return flags.st_mtime;
	} else {
		ERR_FAIL_V_MSG(0, "Failed to get modified time for: " + p_file + ".");
	};
}

uint32_t FileAccessWii::_get_unix_permissions(const String &p_file) {
	String file = fix_path(p_file);
	struct stat flags;
	int err = stat(file.utf8().get_data(), &flags);

	if (!err) {
		return flags.st_mode & 0x7FF; //only permissions
	} else {
		ERR_FAIL_V_MSG(0, "Failed to get unix permissions for: " + p_file + ".");
	};
}

Error FileAccessWii::_set_unix_permissions(const String &p_file, uint32_t p_permissions) {
	String file = fix_path(p_file);

	int err = chmod(file.utf8().get_data(), p_permissions);
	if (!err) {
		return OK;
	}

	return FAILED;
}
