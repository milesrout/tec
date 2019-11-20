// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#ifndef TRILLEK_COMMON_VFS_VFS_HPP
#define TRILLEK_COMMON_VFS_VFS_HPP

#include "types.hpp"
#include "fs.hpp"
#include "shared_any.hpp"
#include "unique_any.hpp"
#include "stream.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <typeindex>
#include <vector>
#include <variant>

#include "type_debug.hpp"

namespace tec::vfs {

/* A virtual path is a file path within the Trillek VFS. Files on the Trillek
 * VFS have ASCII filenames including their file paths.
 * 
 * Virtual paths are always absolute, there is no such thing as a 'relative
 * path' in the Trillek VFS.
 */
class virtual_path final {
public:
	virtual_path(const char* p);
	virtual_path(std::string const& p);

	[[nodiscard]] virtual_path operator/(virtual_path const& other) const;
	[[nodiscard]] bool operator==(virtual_path const& other) const;
	[[nodiscard]] bool operator<(virtual_path const& other) const;

	std::optional<virtual_path> matches(virtual_path const& other) const;

	/* A path is a directory if and only if it ends with a /. This is an
	 * invariant that we maintain throughout the entire VFS.
	 * All paths start with a /, which is another invariant.
	 */
	bool is_directory() const;

	/* Return the directory component of a file path (everything up to and
	 * including the final /). This is an error if the path is a directory.
	 */
	std::string directory() const;

	/* Return the file component of a file path (everything after and including
	 * the final /). This is an error if the path is a directory.
	 */
	std::string file() const;

	operator std::string() const;
private:
	std::string _path;
};

class virtual_file_system;

/* A virtual_directory is a kind of resource that can be mounted to a virtual
 * filesystem that acts like a directory. It's not possible to tell directly
 * from the VFS interface that items mounted in virtual directories haven't
 * been individually mounted.
 *
 * Virtual directories can be backed by real directories on disk, an in-memory
 * storage (like a ramdisk) or an archive (readonly transparent access to the
 * contents).
 *
 * The contents of virtual directories are mounted lazily to the VFS when
 * requested.
 */
class virtual_directory final {
public:

	/* A base class for different types of backing storage for a virtual
	 * directory.
	 */
	class backing_storage {
	public:
		virtual ~backing_storage() = default;
		virtual std::optional<shared_any>
			lookup(virtual_path const&, std::string ext) = 0;
	};

	/* A onetime_directory is a virtual_directory that hosts only a single
	 * item, used when individual objects are mounted at specific locations.
	 */
	class oneitem_directory : public backing_storage {
	public:
		template <typename T>
		oneitem_directory(virtual_path const& filename, T&& file)
			: filename{filename}, file{std::forward<T>(file)} {}
		virtual ~oneitem_directory() = default;

		std::optional<shared_any> lookup(virtual_path const& name, std::string ext) override {
			if (name == filename) {
				return std::optional{file};
			}
			return std::nullopt;
		}
	private:
		std::string filename;
		shared_any file;
	};

	/* A real_directory is backing storage for a virtual_directory using a real
	 * directory on the real filesystem. Modifications to this directory are
	 * synchronised to disk.
	 */
	class real_directory : public backing_storage {
	public:
		real_directory(virtual_file_system& vfs, fs::path const& path)
			: vfs(vfs), path(fs::canonical(path))
		{
			if (!fs::is_directory(path)) {
				throw std::runtime_error{"Must be a directory"};
			}
		}
		virtual ~real_directory() = default;

		/* Return true if 'path' is a prefix of 'other'. This doesn't
		 * canonicalise 'other'.
		 */
		bool is_within(fs::path other) {
			auto pair = std::mismatch(other.begin(), other.end(), path.begin(), path.end());
			return pair.second == path.end();
		}

		std::optional<shared_any> lookup(virtual_path const& name, std::string ext) override;
	private:
		std::map<virtual_path, shared_any> resources;
		virtual_file_system& vfs;
		fs::path path;
	};

	/* An inmemory_directory is backing storage for a virtual_directory using RAM.
	 * Modifications to this directory are ephemeral.
	 */
	class inmemory_directory : public backing_storage {
	public:
		virtual ~inmemory_directory() = default;
		std::optional<shared_any> lookup(virtual_path const& name, std::string ext) override {
			return std::nullopt;
		}
	private:
		std::map<virtual_path, shared_any> _files;
	};

	// /* An archive_directory is backing storage for a virtual_directory using an
	//  * archive on disk. This directory is not modifiable.
	//  */
	// class archive_directory : public backing_storage {
	//
	// };

	template <typename T>
	std::optional<std::shared_ptr<T>> lookup(virtual_path const& name, std::string ext) {
		std::optional<shared_any> file = _backing_storage->lookup(name, ext);
		if (!file) {
			return std::nullopt;
		}

		try {
			return std::optional{shared_any_base_cast<T>(*file)};
		} catch (std::bad_any_cast const&) {
			return std::nullopt;
		}
	}

	virtual_directory(std::unique_ptr<backing_storage> bs)
		: _backing_storage(std::move(bs)) {}
private:
	std::unique_ptr<backing_storage> _backing_storage;
};

class virtual_file_system final {
public:
	// tag type
	struct create_if_not_exists_t {};
	static const constexpr virtual_file_system::create_if_not_exists_t create_if_not_exists{};

	/* A mount_point_handle is an object that is used to give access to the VFS
	 * in the most limited possible way. Rather than giving extension handlers
	 * a non-const reference to a virtual_file_system, we give them only a
	 * mount_point_handle, which can be used to do only one thing: mount
	 * *something* at a particular, already-established location.
	 *
	 * In other words, the only thing you can do with a mount_point_handle is
	 * call 'mount', which takes a shared_any (which can be constructed from a
	 * totally arbitrary type).
	 */
	struct mount_point_handle {
		mount_point_handle() : flag(ATOMIC_FLAG_INIT) {}
		virtual ~mount_point_handle() = default;
		void mount(shared_any resource) {
			if (flag.test_and_set())
				throw std::runtime_error{"Only one resource may be mounted using a mount_point_handle"};
			do_mount(resource);
		}
	protected:
		virtual void do_mount(shared_any resource) = 0;
	private:
		std::atomic_flag flag;
	};

	struct realdir_mount_point_handle : public mount_point_handle {
	protected:
		void do_mount(shared_any res) override {
			/* We don't actually mount the resource, we just put it on this
			 * handle, then the real_directory can grab it from this handle and
			 * store it properly. */
			resource = std::optional{res};
		}
	private:
		friend class virtual_directory::real_directory;
		realdir_mount_point_handle() : mount_point_handle() {}
		std::optional<shared_any> resource;
	};

	struct oneitem_mount_point_handle : public mount_point_handle {
	protected:
		void do_mount(shared_any resource) override {
			auto dirname = mount_point.directory();
			auto filename = mount_point.file();
			virtual_directory::oneitem_directory oneitem_dir{std::move(filename), std::move(resource)};
			virtual_directory virtdir{std::make_unique<virtual_directory::oneitem_directory>(std::move(oneitem_dir))};
			vfs.do_mount(std::move(dirname), std::move(virtdir));
		}
	private:
		friend class virtual_file_system;
		oneitem_mount_point_handle(virtual_file_system& vfs, virtual_path mount_point)
			: mount_point_handle(), vfs(vfs), mount_point(mount_point)
		{
		}
		virtual_file_system& vfs;
		virtual_path mount_point;
	};

	/* A mount_point_handle_directory is like a mount_point_handle, but slightly more flexible. While a mount_point_handle may only mount exactly one resource 
	 */
	struct mount_point_handle_directory {
		void mount_directory(virtual_directory vd) {
			if (flag.test_and_set())
				throw std::runtime_error{"Only one directory may be mounted using a mount_point_handle_directory"};
			vfs.do_mount(mount_point, std::move(vd));
		}
	private:
		friend class virtual_file_system;
		mount_point_handle_directory(virtual_file_system& vfs, virtual_path mount_point)
			: vfs(vfs), mount_point(mount_point), flag(ATOMIC_FLAG_INIT)
		{}
		virtual_file_system& vfs;
		virtual_path mount_point;
		std::atomic_flag flag;
	};

	/* The mount point entirely determines whether we mount as a directory or
	 * as a file.  In theory, we could for example mount archives as archive
	 * files with ``mount("/foo", "$HOME/foo.zip")`` and also mount them
	 * transparently as directories with ``mount("/bar/", "$HOME/bar.zip")``.
	 * In practice, anything that can be mounted as a directory can only be
	 * mounted as a directory.
	 */
	void mount(virtual_path const& mount_point, fs::path const&);
	void mount(virtual_path const& mount_point, fs::path const&, create_if_not_exists_t);

	template <typename StreamType>
	using extension_handler = bool(
		std::string,           // ext: the extension we are handling
		virtual_path const&,   // mp: the mount point
		StreamType,            // file: the file for the object being mounted
		mount_point_handle&,    // mp_handle: a handle for vfs/mp
		std::optional<std::function<void()>>
							   // next: a function to command the extension
							   // handling machinery to call the extension
							   // handler we are overriding.
	);

	template <typename StreamType>
	using single_extension_handler = bool(
		std::string,           // ext: the extension we are handling
		virtual_path const&,   // mp: the mount point
		StreamType,            // file: the file for the object being mounted
		mount_point_handle&    // mp_handle: a handle for vfs/mp
	);

	using extension_handler_t = extension_handler<std::iostream&>;
	using readonly_extension_handler_t = extension_handler<std::istream&>;
	// TODO: Add support for extension handlers that gain ownership of the
	// file, but can't be overridden. The stream is moved into the function.
	//
	// This is the sort of code that makes me appreciate Rust.
	using single_extension_handler_t = single_extension_handler<std::iostream>;
	using single_readonly_extension_handler_t = single_extension_handler<std::istream>;

	/* Register a file extension for mounting purposes. The registered function
	 * will be called when a file with the given extension is directly mounted
	 * as a file or directory.
	 *
	 * The most recently registered handler for a given extension will be
	 * called and will have the option to invoke previously-registered handlers
	 * by calling the function that is its final argument.
	 */
	template <typename T>
	void register_extension(std::string ext, std::function<extension_handler_t> ext_handler) {
		extension_handlers[ext].push_back(ext_handler);
		type_ext_map[std::type_index(typeid(T))] = ext;
	}

	//void register_extension(std::string ext, std::function<extension_handler_t> f);
	//void register_extension(std::string ext, std::function<readonly_extension_handler_t> f);
	//void register_extension(std::string ext, std::function<single_extension_handler_t> f);
	//void register_extension(std::string ext, std::function<single_readonly_extension_handler_t> f);

	/* Try to load a value from a virtual path of a given type.
	 */
	template <typename T>
	std::optional<std::shared_ptr<T>> try_load(virtual_path const& path)
	{
		std::string ext;
		ext = type_ext_map.at(std::type_index(typeid(T)));
		for (auto& [mount_point, dir] : util::reversed(mounts)) {
			if (auto const& rest_of_path = mount_point.matches(path); rest_of_path) {
				return dir.template lookup<T>(rest_of_path.value(), ext);
			}
		}
		return std::nullopt;
	}

	/* Load a value from a virtual path of a given type.
	 */
	template <typename T>
	std::shared_ptr<T> load(virtual_path const& path)
	{
		auto optional_ptr = try_load<T>(path);
		if (!optional_ptr)
			throw std::runtime_error{"No such file or directory (A)"};
		return optional_ptr.value();
	}
private:
	friend class oneitem_mount_point_handle;
	friend class realdir_mount_point_handle;
	friend class mount_point_handle_directory;
	friend class virtual_directory::real_directory;
	// helper functions
	void do_mount_directory(virtual_path const& mount_point, fs::path const&);
	void do_mount_directory(virtual_path const& mount_point, fs::path const&, create_if_not_exists_t);
	void do_mount_file(virtual_path const& mount_point, fs::path const&);
	void do_mount_file(virtual_path const& mount_point, fs::path const&, create_if_not_exists_t);
	void do_extension_mount(virtual_path const& mount_point, mount_point_handle&, fs::path const&);

	void do_mount(virtual_path mp, virtual_directory vd) {
		mounts.emplace_back(std::move(mp), std::move(vd));
	}

	// Order matters, latter mounts override earlier (bind mounts)
	std::vector<std::pair<virtual_path, virtual_directory>> mounts;

	// This is the order we look at them.
	template <typename ext_handler>
	using ext_handler_map = std::map<std::string, std::vector<std::function<ext_handler>>>;

	std::map<std::type_index, std::string> type_ext_map;

	ext_handler_map<single_extension_handler_t> single_extension_handlers;
	ext_handler_map<single_readonly_extension_handler_t> single_readonly_extension_handlers;
	ext_handler_map<extension_handler_t> extension_handlers;
	ext_handler_map<readonly_extension_handler_t> readonly_extension_handlers;
};

}

#endif
