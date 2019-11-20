// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#include "vfs.hpp"

#include <iostream>
#include <fstream>

namespace tec::vfs {

// TODO: move to utility header
std::string quoted(std::string str, char c) {
	std::stringstream ss;
	ss << std::quoted(str, '\'');
	std::string s;
	ss >> s;
	return s;
}

// mount /foo/ ./dir1       -> mount the directory directly
// mount /foo/ ./assets.zip -> mount archive as a directory transparently
// mount /foo  ./dir1       -> MEANINGLESS!
// mount /foo  ./asset.png  -> mount png as an image resource named "foo"

void virtual_file_system::mount(virtual_path const& mount_point, fs::path const& real_path) {
	if (!fs::exists(real_path)) {
		throw std::runtime_error{"No such file or directory (B)"};
	}

	if (mount_point.is_directory()) {
		this->do_mount_directory(mount_point, real_path);
	} else {
		this->do_mount_file(mount_point, real_path);
	}
}

void virtual_file_system::mount(virtual_path const& mount_point, fs::path const& real_path, create_if_not_exists_t) {
	if (mount_point.is_directory()) {
		this->do_mount_directory(mount_point, real_path, create_if_not_exists);
	} else {
		this->do_mount_file(mount_point, real_path, create_if_not_exists);
	}
}

void virtual_file_system::do_mount_directory(virtual_path const& mount_point, fs::path const& real_path, create_if_not_exists_t) {
	if (!fs::exists(real_path)) {
		fs::create_directories(real_path);
	}
	this->do_mount_directory(mount_point, real_path);
}

void virtual_file_system::do_mount_directory(virtual_path const& mount_point, fs::path const& real_path) {
	if (fs::is_directory(real_path)) {
		// The directory needs to be able to look up extensions by types.
		virtual_directory::real_directory realdir{*this, real_path};
		virtual_directory virtdir{std::make_unique<virtual_directory::real_directory>(realdir)};
		this->do_mount(mount_point, std::move(virtdir));
	}
	else {
		oneitem_mount_point_handle omp_handle{*this, mount_point};
		mount_point_handle& mp_handle = omp_handle;
		this->do_extension_mount(mount_point, mp_handle, real_path);
	}
}

void virtual_file_system::do_mount_file(virtual_path const& mount_point, fs::path const& real_path, create_if_not_exists_t) {
	if (!fs::exists(real_path)) {
		// In the future we'll want to create the file, so that e.g. we can
		// create empty logfiles by mounting them. However, we have to do this
		// *after* extension handling so that the extension handler can create
		// the file. This is all a bit tricky so I'm just not going to bother
		// with it for now. It's easier with directories because there's only
		// one way to create a directory.
		//
		// Actually, that's not true: directories should use extension handlers
		// too with create_if_not_exists, because if you mount a virtual
		// directory path to a nonexistent real file path ending with ".zip",
		// we should create an archive there. People will just have to remember
		// that their plugins can create zipbombs!
		throw std::runtime_error("No such file " + quoted(real_path, '\''));
	}
	this->do_mount_file(mount_point, real_path);
}

void virtual_file_system::do_mount_file(virtual_path const& mount_point, fs::path const& real_path) {
	if (fs::is_directory(real_path)) {
		throw std::runtime_error("Cannot mount a directory to a non-directory path");
	}

	oneitem_mount_point_handle omp_handle{*this, mount_point};
	mount_point_handle& mp_handle = omp_handle;
	this->do_extension_mount(mount_point, mp_handle, real_path);
}


/* mount_point is passed directly to the mount_point_handle, so it should be
 * the exact point in the virtual file system where the resource should
 * ultimately be mounted, **not** including the 'file' component.
 * i.e. mount_point should end in /.
 */
void virtual_file_system::do_extension_mount(virtual_path const& mount_point, mount_point_handle& mp_handle, fs::path const& real_path) {
	std::string ext = real_path.extension();
	std::string stem = real_path.stem();

	// Resources should be added to the VFS with a name and type.
	// The name in this case should be $MOUNT_POINT/$STEM.
	// The type should be determined by the extension.

	if (extension_handlers.count(ext) == 0) {
		throw std::runtime_error("Unknown file type");
	}

	// Construct a chain of handlers, where each handler gets passed a function
	// next, where next invokes the next handler in the chain, and so on. the
	// final handler in the chain gets nullopt for next.
	std::optional<std::function<void()>> previous_handler = std::nullopt;
	std::fstream file{real_path};
	for (auto const& ext_handler : extension_handlers[ext]) {
		previous_handler = [=, &mp_handle, &file]{
			bool result = ext_handler(ext, mount_point, file, mp_handle, previous_handler);
			if (!result) {
				throw std::runtime_error{"What sort of error should this be?"};
			}
		};
	}

	// This is guaranteed to be present, because extension_handlers.count(ext) != 0.
	previous_handler.value()();
}


std::optional<shared_any> virtual_directory::real_directory::lookup(
		virtual_path const& name, std::string ext)
{
	if (resources.find(name) != resources.end()) {
		return resources[name];
	}
	// Search the directory for everything named 'bob' with an
	// extension that is registered, then load them all.
	// Take off the initial /. 
	fs::path filename = (path / std::string{name}.substr(1)).replace_extension(ext);
	fs::path canonical = fs::canonical(filename);
	if (!is_within(canonical)) {
		throw std::runtime_error{"File must be within this directory"};
	}

	if (fs::exists(canonical)) {
		virtual_file_system::realdir_mount_point_handle rmp_handle{};
		virtual_file_system::mount_point_handle& mp_handle = rmp_handle;
		vfs.do_extension_mount(name, mp_handle, canonical);
		if (rmp_handle.resource) {
			resources[name] = *rmp_handle.resource;
		}
		return rmp_handle.resource;
	}
	return std::nullopt;
}

}
