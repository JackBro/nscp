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

#include <file/file_finder.hpp>

namespace file {

	class finder {
		template <class finder_function>
		void recursive_scan(std::wstring dir, boost::wregex  pattern, int current_level, int max_level, finder_function & f, error_reporter * errors) {
			if ((max_level != -1) && (current_level > max_level))
				return;

			boost::filesystem::path path = dir;
			if (path.is_directory()) {
				// scan and parse all sub folders.
				boost::filesystem::wdirectory_iterator end_itr; // default construction yields past-the-end
				for ( boost::filesystem::wdirectory_iterator itr( path ); itr != end_itr; ++itr ) {
					recursive_scan<finder_function>(itr->path().leaf(), pattern, current_level+1, max_level, f, errors, debug);
					//if ( !is_directory(itr->status()) ) {}
				}
			} else if (!path.is_regular_file()) {
				// Match against pattern
				if (regex_match(path.string(), pattern)) {
					NSC_DEBUG_MSG_STD(_T("Matched: ") + path.string());
					f(file_finder_data(wfd, path.string(), errors));
				}
			} else {
				error->(...);
			}
			WIN32_FIND_DATA wfd;

			DWORD fileAttr = GetFileAttributes(dir.c_str());
			NSC_DEBUG_MSG_STD(_T("Input is: ") + dir + _T(" / ") + strEx::ihextos(fileAttr));

			if (!file_helpers::checks::is_directory(fileAttr)) {
				NSC_DEBUG_MSG_STD(_T("Found a file dont do recursive scan: ") + dir);
				// It is a file check it an return (dont check recursivly)
				pattern_type single_path = split_path_ex(dir);
				NSC_DEBUG_MSG_STD(_T("Path is: ") + single_path.first);
				HANDLE hFind = FindFirstFile(dir.c_str(), &wfd);
				if (hFind != INVALID_HANDLE_VALUE) {
					f(file_finder_data(wfd, single_path.first, errors));
					FindClose(hFind);
				}
				return;
			std::wstring file_pattern = dir + _T("\\") + pattern;
			NSC_DEBUG_MSG_STD(_T("File pattern: ") + file_pattern);
			HANDLE hFind = FindFirstFile(file_pattern.c_str(), &wfd);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (!f(file_finder_data(wfd, dir, errors)))
						break;
				} while (FindNextFile(hFind, &wfd));
				FindClose(hFind);
			}
			std::wstring dir_pattern = dir + _T("\\*.*");
			NSC_DEBUG_MSG_STD(_T("File pattern: ") + dir_pattern);
			hFind = FindFirstFile(dir_pattern.c_str(), &wfd);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (file_helpers::checks::is_directory(wfd.dwFileAttributes)) {
						if ( (wcscmp(wfd.cFileName, _T(".")) != 0) && (wcscmp(wfd.cFileName, _T("..")) != 0) )
							recursive_scan<finder_function>(dir + _T("\\") + wfd.cFileName, pattern, current_level+1, max_level, f, errors, debug);
					}
				} while (FindNextFile(hFind, &wfd));
				FindClose(hFind);
			}
		}

	};



}