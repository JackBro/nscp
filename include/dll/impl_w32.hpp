/*
 * Copyright 2004-2016 The NSClient++ Authors - https://nsclient.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

#include <error.hpp>

namespace dll {
	namespace win32 {
		class impl : public boost::noncopyable {
		private:
			HMODULE handle_;
			boost::filesystem::path module_;
		public:
			impl(boost::filesystem::path module) : module_(module), handle_(NULL) {
				if (!boost::filesystem::is_regular(module_)) {
					module_ = fix_module_name(module_);
				}
			}
			static boost::filesystem::path fix_module_name(boost::filesystem::path module) {
				if (boost::filesystem::is_regular(module))
					return module;
				/* this one (below) is wrong I think */
				boost::filesystem::path mod = module / get_extension();
				if (boost::filesystem::is_regular(mod))
					return mod;
				mod = boost::filesystem::path(module.string() + get_extension());
				if (boost::filesystem::is_regular(mod))
					return mod;
				return module;
			}

			static std::string get_extension() {
				return ".dll";
			}

			static bool is_module(std::string file) {
				return boost::ends_with(file, get_extension());
			}

			void load_library() {
				if (handle_ != NULL)
					unload_library();
				handle_ = LoadLibrary(module_.native().c_str());
				if (handle_ == NULL)
					throw dll_exception("Could not load library: " + utf8::cvt<std::string>(error::lookup::last_error()) + ": " + module_.filename().string());
			}
			LPVOID load_proc(std::string name) {
				if (handle_ == NULL)
					throw dll_exception("Failed to load process since module is not loaded: " + module_.filename().string());
				LPVOID ep = GetProcAddress(handle_, name.c_str());
				return ep;
			}

			void unload_library() {
				if (handle_ == NULL)
					return;
				FreeLibrary(handle_);
				handle_ = NULL;
			}
			bool is_loaded() const { return handle_ != NULL; }
			boost::filesystem::path get_file() const { return module_; }
			std::string get_filename() const { return module_.filename().string(); }
			std::string get_module_name() {
				std::string ext = ".dll";
				std::string::size_type l = ext.length();
				std::string fn = get_filename();
				if ((fn.length() > l) && (fn.substr(fn.size() - l) == ext))
					return fn.substr(0, fn.size() - l);
				return fn;
			}
		};
	}
}