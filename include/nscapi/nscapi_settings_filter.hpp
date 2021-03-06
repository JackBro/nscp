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

#include <map>
#include <string>

#include <nscapi/nscapi_settings_helper.hpp>
#include <nscapi/nscapi_settings_proxy.hpp>
#include <nscapi/nscapi_settings_object.hpp>
#include <nscapi/nscapi_protobuf_types.hpp>
#include <nscapi/nscapi_helper.hpp>

#include <nscapi/dll_defines.hpp>

namespace nscapi {
	namespace settings_filters {
		struct NSCAPI_EXPORT filter_object {
			bool debug;
			bool escape_html;

			std::string syntax_top;
			std::string syntax_detail;
			std::string target;
			std::string syntax_ok;
			std::string syntax_empty;
			std::string filter_string;
			std::string filter_ok;
			std::string filter_warn;
			std::string filter_crit;
			std::string perf_data;
			std::string perf_config;
			NSCAPI::nagiosReturn severity;
			std::string command;
			boost::optional<boost::posix_time::time_duration> max_age;
			std::string target_id;
			std::string source_id;
			std::string timeout_msg;

			filter_object(std::string syntax_top, std::string syntax_detail, std::string target)
				: debug(false)
				, escape_html(false)
				, syntax_top(syntax_top)
				, syntax_detail(syntax_detail)
				, target(target)
				, severity(-1) {}

			filter_object(const filter_object &other)
				: debug(other.debug)
				, escape_html(other.escape_html)
				, syntax_top(other.syntax_top)
				, syntax_detail(other.syntax_detail)
				, target(other.target)
				, syntax_ok(other.syntax_ok)
				, syntax_empty(other.syntax_empty)
				, filter_string(other.filter_string)
				, filter_ok(other.filter_ok)
				, filter_warn(other.filter_warn)
				, filter_crit(other.filter_crit)
				, perf_data(other.perf_data)
				, perf_config(other.perf_config)
				, severity(other.severity) 
				, command(other.command)
				, max_age(other.max_age)
				, target_id(other.target_id)
				, source_id(other.source_id)
				, timeout_msg(other.timeout_msg)
			{}


			std::string to_string() const {
				std::stringstream ss;
				ss << "{TODO}";
				return ss.str();
			}

			void set_severity(std::string severity_) {
				severity = nscapi::plugin_helper::translateReturn(severity_);
			}

			inline boost::posix_time::time_duration parse_time(std::string time) {
				std::string::size_type p = time.find_first_of("sSmMhHdDwW");
				if (p == std::string::npos)
					return boost::posix_time::seconds(boost::lexical_cast<long>(time));
				long value = boost::lexical_cast<long>(time.substr(0, p));
				if ((time[p] == 's') || (time[p] == 'S'))
					return boost::posix_time::seconds(value);
				else if ((time[p] == 'm') || (time[p] == 'M'))
					return boost::posix_time::minutes(value);
				else if ((time[p] == 'h') || (time[p] == 'H'))
					return boost::posix_time::hours(value);
				else if ((time[p] == 'd') || (time[p] == 'D'))
					return boost::posix_time::hours(value * 24);
				else if ((time[p] == 'w') || (time[p] == 'W'))
					return boost::posix_time::hours(value * 24 * 7);
				return boost::posix_time::seconds(value);
			}

			void set_max_age(std::string age) {
				if (age != "none" && age != "infinite" && age != "false" && age != "off")
					max_age = parse_time(age);
			}

			void read_object(nscapi::settings_helper::path_extension &path, const bool is_default);
			void apply_parent(const filter_object &parent);
		};
	}
}