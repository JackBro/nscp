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

#include <map>
#include <vector>
#include <ostream>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/assign.hpp>
#include <boost/optional.hpp>
#include <boost/date_time.hpp>

#include <nscapi/nscapi_protobuf_functions.hpp>
#include <nscapi/nscapi_core_helper.hpp>
#include <nscapi/nscapi_helper_singleton.hpp>
#include <nscapi/macros.hpp>
#include <nscapi/nscapi_helper.hpp>

#include <parsers/expression/expression.hpp>

#include <nscapi/nscapi_settings_helper.hpp>

#include "SimpleFileWriter.h"

namespace sh = nscapi::settings_helper;

void build_syntax(parsers::simple_expression &parser, std::string &syntax, SimpleFileWriter::index_lookup_type &index);

struct simple_string_functor {
	std::string value;
	simple_string_functor(std::string value) : value(value) {}
	simple_string_functor(const simple_string_functor &other) : value(other.value) {}
	const simple_string_functor& operator=(const simple_string_functor &other) {
		value = other.value;
		return *this;
	}
	std::string operator() (const config_object&, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &) {
		return value;
	}
};
struct header_host_functor {
	std::string operator() (const config_object&, const std::string channel, const Plugin::Common::Header &hdr, const Plugin::QueryResponseMessage::Response &) {
		std::string sender = hdr.sender_id();
		BOOST_FOREACH(const Plugin::Common::Host &h, hdr.hosts()) {
			if (h.id() == sender)
				return h.host();
		}
		return sender;
	}
};
struct payload_command_functor {
	std::string operator() (const config_object&, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &payload) {
		return payload.command();
	}
};
struct channel_functor {
	std::string operator() (const config_object&, const std::string channel, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &) {
		return channel;
	}
};
struct payload_alias_functor {
	std::string operator() (const config_object&, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &payload) {
		return payload.alias();
	}
};
struct payload_message_functor {
	std::string operator() (const config_object&, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &payload) {
		std::string ret;
		BOOST_FOREACH(Plugin::QueryResponseMessage::Response::Line l, payload.lines())
			ret += l.message();
		return ret;
	}
};
struct payload_result_functor {
	std::string operator() (const config_object&, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &payload) {
		return nscapi::plugin_helper::translateReturn(nscapi::protobuf::functions::gbp_to_nagios_status(payload.result()));
	}
};
struct payload_result_nr_functor {
	std::string operator() (const config_object&, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &payload) {
		return strEx::s::xtos(nscapi::protobuf::functions::gbp_to_nagios_status(payload.result()));
	}
};
struct payload_alias_or_command_functor {
	std::string operator() (const config_object&, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &payload) {
		if (payload.has_alias())
			return payload.alias();
		return payload.command();
	}
};

struct epoch_functor {
	std::string operator() (const config_object&, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &payload) {
		boost::posix_time::ptime time_t_epoch(boost::gregorian::date(1970, 1, 1));
		boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
		boost::posix_time::time_duration diff = now - time_t_epoch;
		return strEx::s::xtos(diff.total_seconds());
	}
};

struct time_functor {
	std::string operator() (const config_object& config, const std::string, const Plugin::Common::Header &, const Plugin::QueryResponseMessage::Response &payload) {
		std::stringstream ss;
		boost::posix_time::time_facet *facet = new boost::posix_time::time_facet(config.time_format.c_str());
		ss.imbue(std::locale(std::cout.getloc(), facet));
		ss << boost::posix_time::second_clock::local_time();
		return ss.str();
	}
};

std::string simple_string_fun(std::string key) {
	return key;
}
bool SimpleFileWriter::loadModuleEx(std::string alias, NSCAPI::moduleLoadMode) {
	std::string syntax;
	std::string syntax_host;
	std::string syntax_service;
	std::string channel;
	try {
		sh::settings_registry settings(get_settings_proxy());

		settings.set_alias(alias, "writers/file");

		settings.alias().add_path_to_settings()
			("FILE WRITER", "Section for simple file writer module (SimpleFileWriter.dll).")

			;

		settings.alias().add_key_to_settings()
			("syntax", sh::string_key(&syntax, "${alias-or-command} ${result} ${message}"),
				"MESSAGE SYNTAX", "The syntax of the message to write to the line.\nCan be any arbitrary string as well as include any of the following special keywords:"
				"${command} = The command name, ${host} the host, ${channel} the recieving channel, ${alias} the alias for the command, ${alias-or-command} = alias if set otherweise command, ${message} = the message data (no escape), ${result} or ${result_number} = The result status (number), ${epoch} = seconds since unix epoch, ${time} = time using time-format.")

			("service-syntax", sh::string_key(&syntax_service),
				"SERVICE MESSAGE SYNTAX", "The syntax of the message to write to the line.\nCan be any arbitrary string as well as include any of the following special keywords:"
				"${command} = The command name, ${host} the host, ${channel} the recieving channel, ${alias} the alias for the command, ${alias-or-command} = alias if set otherweise command, ${message} = the message data (no escape), ${result} or ${result_number} = The result status (number), ${epoch} = seconds since unix epoch, ${time} = time using time-format.")

			("host-syntax", sh::string_key(&syntax_host),
				"HOST MESSAGE SYNTAX", "The syntax of the message to write to the line.\nCan be any arbitrary string as well as include any of the following special keywords:"
				"${command} = The command name, ${host} the host, ${channel} the recieving channel, ${alias} the alias for the command, ${alias-or-command} = alias if set otherweise command, ${message} = the message data (no escape), ${result} or ${result_number} = The result status (number), ${epoch} = seconds since unix epoch, ${time} = time using time-format.")

			("file", sh::path_key(&filename_, "output.txt"),
				"FILE TO WRITE TO", "The filename to write output to.")

			("channel", sh::string_key(&channel, "FILE"),
				"CHANNEL", "The channel to listen to.")

			("time-syntax", sh::string_key(&config_.time_format, "%Y-%m-%d %H:%M:%S"),
				"TIME SYNTAX", "The date format using strftime format flags. This is the time of writing the message as messages currently does not have a source time.")

			;

		settings.register_all();
		settings.notify();

		nscapi::core_helper core(get_core(), get_id());
		core.register_channel(channel);

		if (syntax_host.empty()) {
			syntax_host = syntax;
		}
		if (syntax_service.empty()) {
			syntax_service = syntax;
		}
		parsers::simple_expression parser;
		parsers::simple_expression::result_type result_host, result_service;
		build_syntax(parser, syntax_host, syntax_host_lookup_);
		build_syntax(parser, syntax_service, syntax_service_lookup_);

	} catch (nscapi::nscapi_exception &e) {
		NSC_LOG_ERROR_EXR("Failed to register command: ", e);
		return false;
	} catch (std::exception &e) {
		NSC_LOG_ERROR_EXR("load", e);
		return false;
	} catch (...) {
		NSC_LOG_ERROR_EX("load");
		return false;
	}
	return true;
}


void build_syntax(parsers::simple_expression &parser, std::string &syntax, SimpleFileWriter::index_lookup_type &index) {
	parsers::simple_expression::result_type result;
	if (!parser.parse(syntax, result)) {
		NSC_LOG_ERROR_STD("Failed to parse syntax: " + syntax)
	}
	BOOST_FOREACH(parsers::simple_expression::entry &e, result) {
		if (!e.is_variable) {
			index.push_back(simple_string_functor(e.name));
		} else if (e.name == "command") {
			index.push_back(payload_command_functor());
		} else if (e.name == "host") {
			index.push_back(header_host_functor());
		} else if (e.name == "channel") {
			index.push_back(channel_functor());
		} else if (e.name == "alias") {
			index.push_back(payload_alias_functor());
		} else if (e.name == "alias-or-command") {
			index.push_back(payload_alias_or_command_functor());
		} else if (e.name == "message") {
			index.push_back(payload_message_functor());
		} else if (e.name == "result") {
			index.push_back(payload_result_functor());
		} else if (e.name == "result_number") {
			index.push_back(payload_result_nr_functor());
		} else if (e.name == "epoch") {
			index.push_back(epoch_functor());
		} else if (e.name == "time") {
			index.push_back(time_functor());
		} else {
			NSC_LOG_ERROR_STD("Invalid index: " + e.name);
		}
	}
}

void SimpleFileWriter::handleNotification(const std::string &, const Plugin::QueryResponseMessage::Response &request, Plugin::SubmitResponseMessage::Response *response, const Plugin::SubmitRequestMessage &request_message) {
	std::string key;

	if ((request.has_alias() && !request.alias().empty()) || (request.has_command() && !request.command().empty()) ) {
		BOOST_FOREACH(index_lookup_function &f, syntax_service_lookup_) {
			key += f(config_, request.command(), request_message.header(), request);
		}
	} else {
		BOOST_FOREACH(index_lookup_function &f, syntax_host_lookup_) {
			key += f(config_, request.command(), request_message.header(), request);
		}
	}
	std::string data = request.SerializeAsString();
	{
		boost::unique_lock<boost::shared_mutex> lock(cache_mutex_);
		if (!lock) {
			nscapi::protobuf::functions::append_simple_submit_response_payload(response, request.command(), Plugin::Common_Result_StatusCodeType_STATUS_ERROR, "Failed to get lock");
			return;
		}
		std::ofstream out;
		out.open(filename_.c_str(), std::ios::out | std::ios::app);
		out << key << std::endl;
	}
	nscapi::protobuf::functions::append_simple_submit_response_payload(response, request.command(), Plugin::Common_Result_StatusCodeType_STATUS_OK, "message has been written");
}