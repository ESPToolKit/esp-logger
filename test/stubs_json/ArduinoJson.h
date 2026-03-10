#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#define ARDUINOJSON_VERSION_MAJOR 7

namespace ArduinoJson {

namespace detail {

inline std::string quote(const std::string &value) {
	std::string quoted;
	quoted.reserve(value.size() + 2);
	quoted.push_back('"');
	for (char ch : value) {
		if (ch == '"' || ch == '\\') {
			quoted.push_back('\\');
		}
		quoted.push_back(ch);
	}
	quoted.push_back('"');
	return quoted;
}

inline std::string indentSubsequentLines(const std::string &text, const std::string &indent) {
	std::string result;
	result.reserve(text.size() + indent.size());

	for (char ch : text) {
		result.push_back(ch);
		if (ch == '\n') {
			result += indent;
		}
	}

	return result;
}

} // namespace detail

class JsonVariantConst {
  public:
	JsonVariantConst() = default;
	JsonVariantConst(const char *value)
	    : _compact(detail::quote(value != nullptr ? value : "")), _pretty(_compact) {
	}
	JsonVariantConst(int value) : _compact(std::to_string(value)), _pretty(_compact) {
	}
	JsonVariantConst(bool value) : _compact(value ? "true" : "false"), _pretty(_compact) {
	}

	static JsonVariantConst fromRaw(std::string compact, std::string pretty) {
		return JsonVariantConst(std::move(compact), std::move(pretty), RawTag{});
	}

	const std::string &compact() const {
		return _compact;
	}
	const std::string &pretty() const {
		return _pretty;
	}

  private:
	struct RawTag {};

	JsonVariantConst(std::string compact, std::string pretty, RawTag)
	    : _compact(std::move(compact)), _pretty(std::move(pretty)) {
	}

	std::string _compact;
	std::string _pretty;
};

class JsonDocument {
  public:
	class MemberProxy {
	  public:
		MemberProxy(JsonDocument &owner, std::string key) : _owner(owner), _key(std::move(key)) {
		}

		MemberProxy &operator=(const char *value) {
			_owner.set(_key, JsonVariantConst(value));
			return *this;
		}

		MemberProxy &operator=(int value) {
			_owner.set(_key, JsonVariantConst(value));
			return *this;
		}

		MemberProxy &operator=(bool value) {
			_owner.set(_key, JsonVariantConst(value));
			return *this;
		}

		MemberProxy &operator=(JsonVariantConst value) {
			_owner.set(_key, std::move(value));
			return *this;
		}

	  private:
		JsonDocument &_owner;
		std::string _key;
	};

	MemberProxy operator[](const char *key) {
		return MemberProxy(*this, key != nullptr ? key : "");
	}

	template <typename T> T as() const;

  private:
	friend class MemberProxy;

	void set(const std::string &key, JsonVariantConst value) {
		for (auto &member : _members) {
			if (member.first == key) {
				member.second = std::move(value);
				return;
			}
		}

		_members.emplace_back(key, std::move(value));
	}

	JsonVariantConst toVariant() const {
		std::string compact = "{";
		std::string pretty = "{";

		if (!_members.empty()) {
			pretty.push_back('\n');
		}

		for (size_t index = 0; index < _members.size(); ++index) {
			const auto &member = _members[index];
			const bool isLast = index + 1 == _members.size();

			compact += detail::quote(member.first);
			compact += ':';
			compact += member.second.compact();

			pretty += "  ";
			pretty += detail::quote(member.first);
			pretty += ": ";
			pretty += detail::indentSubsequentLines(member.second.pretty(), "  ");

			if (!isLast) {
				compact.push_back(',');
				pretty += ",\n";
			}
		}

		if (!_members.empty()) {
			pretty.push_back('\n');
		}

		compact.push_back('}');
		pretty.push_back('}');

		return JsonVariantConst::fromRaw(std::move(compact), std::move(pretty));
	}

	std::vector<std::pair<std::string, JsonVariantConst>> _members;
};

template <> inline JsonVariantConst JsonDocument::as<JsonVariantConst>() const {
	return toVariant();
}

inline size_t measureJson(JsonVariantConst source) {
	return source.compact().size();
}

inline size_t measureJsonPretty(JsonVariantConst source) {
	return source.pretty().size();
}

inline size_t serializeJson(JsonVariantConst source, void *buffer, size_t bufferSize) {
	if (buffer == nullptr || bufferSize == 0) {
		return 0;
	}

	const std::string &text = source.compact();
	if (text.empty() || bufferSize <= text.size()) {
		return 0;
	}

	std::memcpy(buffer, text.data(), text.size());
	static_cast<char *>(buffer)[text.size()] = '\0';
	return text.size();
}

inline size_t serializeJsonPretty(JsonVariantConst source, void *buffer, size_t bufferSize) {
	if (buffer == nullptr || bufferSize == 0) {
		return 0;
	}

	const std::string &text = source.pretty();
	if (text.empty() || bufferSize <= text.size()) {
		return 0;
	}

	std::memcpy(buffer, text.data(), text.size());
	static_cast<char *>(buffer)[text.size()] = '\0';
	return text.size();
}

} // namespace ArduinoJson
