// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#ifndef TRILLEK_COMMON_VFS_STREAMBUF_HPP
#define TRILLEK_COMMON_VFS_STREAMBUF_HPP

#include "types.hpp"
#include "fs.hpp"
#include "shared_any.hpp"

#include <filesystem>
#include <iostream>
#include <streambuf>

namespace tec::vfs {

// /* A tec::vfs::basic_zipbuf is like a std::basic_filebuf, but rather than being
//  * backed by a real file on disk, it's backed by a location in a ZIP archive.
//  */
// template <typename CharT, typename Traits=std::char_traits<CharT>>
// class basic_zipbuf : public std::basic_streambuf<CharT, Traits> {
// 	// to be implemented
// }

// /* A tec::vfs::fstream is much like a std::fstream, but rather than being
//  * backed by a std::filebuf, it's backed by either a filebuf or a stringbuf.
//  */
// class fstream : public std::iostream {
// public:
// 	using char_type = char;
// 	using traits_type = std::char_traits<char>;
// 	using int_type = traits::int_type;
// 	using pos_type = traits::pos_type;
// 	using off_type = traits::off_type;
// protected:
// 	// constructors (standard)
// 	fstream();
// 	fstream(const fstream& rhs) = delete;
// 	fstream(fstream&& rhs);
// 
// 	// constructors (file-like)
// 	explicit fstream(const char* s, ios_base::openmode mode=ios_base::in|ios_base::out);
// 	explicit fstream(const std::string& s, ios_base::openmode mode=ios_base::in|ios_base::out);
// 
// 	// constructors (string-like)
// 	explicit fstream(ios_base::openmode which=ios_base::in|ios_base::out);
// 	explicit fstream(const std::string& str, ios_base::openmode which=ios_base::in|ios_base::out);
// 
// 	// move/copy/swap operators
// 	fstream& operator=(const fstream& rhs) = delete;
// 	fstream& operator=(fstream&& rhs);
// 	void swap(fstream& rhs);
// 
// 	// member functions (standard)
// 	std::streambuf* rdbuf() const;
// 
// 	// member functions (file-like)
// 	bool is_open() const;
// 	void open(const char *s, ios_base::openmode mode=ios_base::in|ios_base::out);
// 	void open(const std::string& s, ios_base::openmode mode=ios_base::in|ios_base::out);
// 	void close();
// 
// 	// member functions (string-like)
// 	std::string str() const;
// 	void str(const std::string& s);
// private:
// 	std::variant<std::filebuf, std::stringbuf> sb;
// };

// /* A tec::vfs::basic_ifstream is much like a std::basic_ifstream, but rather
//  * than being backed by a std::basic_filebuf, it's backed by our virtual
//  * backing storage.
//  */
// template <typename CharT, typename Traits = std::char_traits<CharT>>
// class basic_ifstream : public std::basic_istream<CharT, Traits> {
// };
// 
// using fstream = basic_fstream<char>;
// using ifstream = basic_ifstream<char>;

}

#endif
