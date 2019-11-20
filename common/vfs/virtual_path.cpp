// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#include "vfs.hpp"

namespace tec::vfs {

virtual_path::virtual_path(const char *p) : _path(p) {
	if (p[0] != '/') {
		std::cerr << "Invalid path: " << p << std::endl;
		throw std::runtime_error{"Virtual paths are always absolute"};
	}
}

virtual_path::virtual_path(std::string const& p) : _path(p) {
	if (p[0] != '/') {
		std::cerr << "Invalid path: " << p << std::endl;
		throw std::runtime_error{"Virtual paths are always absolute"};
	}
}


[[nodiscard]] virtual_path virtual_path::operator/(virtual_path const& other) const {
	if (!is_directory())
		throw std::runtime_error{"Can't create subpaths of non-directories"};

	// The other path will begin with a / (class invariant), so we should
	// remove it before we concatenate the paths.
	return _path + other._path.substr(1);
}

[[nodiscard]] bool virtual_path::operator==(virtual_path const& other) const {
	return _path == other._path;
}

[[nodiscard]] bool virtual_path::operator<(virtual_path const& other) const {
	return _path < other._path;
}

std::optional<virtual_path> virtual_path::matches(virtual_path const& other) const {
	if (!is_directory())
		throw std::runtime_error{"Can't match against non-directory paths"};

	auto idx = other._path.find(_path);
	if (idx != 0)
		return std::nullopt;

	// Include the initial slash
	return other._path.substr(_path.length() - 1);
}

/* A path is a directory if and only if it ends with a /. This is an
 * invariant that we maintain throughout the entire VFS.
 * All paths start with a /, which is another invariant.
 */
bool virtual_path::is_directory() const {
	return _path.back() == '/';
}

/* Return the directory component of a file path (everything up to and
 * including the final /). This is an error if the path is a directory.
 */
std::string virtual_path::directory() const {
	return _path.substr(0, _path.find_last_of('/') + 1);
}

/* Return the file component of a file path (everything after and including
 * the final /). This is an error if the path is a directory.
 */
std::string virtual_path::file() const {
	return _path.substr(_path.find_last_of('/'));
}

virtual_path::operator std::string() const {
	return _path;
}
}
