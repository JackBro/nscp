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

#include <parsers/filter/modern_filter.hpp>
#include <nscapi/nscapi_helper_singleton.hpp>
#include <nscapi/macros.hpp>

namespace modern_filter {
	bool error_handler_impl::has_errors() const {
		return !error.empty();
	}
	void error_handler_impl::log_error(const std::string error_) {
		NSC_DEBUG_MSG_STD(error_);
		error = error_;
	}
	void error_handler_impl::log_warning(const std::string error) {
		NSC_DEBUG_MSG_STD(error);
	}
	void error_handler_impl::log_debug(const std::string error) {
		NSC_DEBUG_MSG_STD(error);
	}
	std::string error_handler_impl::get_errors() const {
		return error;
	}

	bool error_handler_impl::is_debug() const {
		return debug_;
	}
	void error_handler_impl::set_debug(bool debug) {
		debug_ = debug;
	}
}