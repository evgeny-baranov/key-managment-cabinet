#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

class String {
 public:
  String() = default;
  String(const char* cstr) : value_(cstr != nullptr ? cstr : "") {}
  String(const std::string& s) : value_(s) {}

  String& operator=(const char* cstr) {
    value_ = (cstr != nullptr ? cstr : "");
    return *this;
  }

  String& operator=(const std::string& s) {
    value_ = s;
    return *this;
  }

  int length() const {
    return static_cast<int>(value_.size());
  }

  char charAt(int index) const {
    if (index < 0 || index >= length()) {
      return '\0';
    }
    return value_[static_cast<std::size_t>(index)];
  }

  String substring(int start, int end) const {
    int safeStart = std::max(0, start);
    int safeEnd = std::max(safeStart, std::min(end, length()));
    return String(value_.substr(static_cast<std::size_t>(safeStart), static_cast<std::size_t>(safeEnd - safeStart)));
  }

  bool startsWith(const String& prefix) const {
    if (prefix.length() > length()) {
      return false;
    }
    return value_.compare(0, static_cast<std::size_t>(prefix.length()), prefix.value_) == 0;
  }

  bool startsWith(const char* prefix) const {
    return startsWith(String(prefix));
  }

  void trim() {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    auto beginIt = std::find_if(value_.begin(), value_.end(), notSpace);
    auto endIt = std::find_if(value_.rbegin(), value_.rend(), notSpace).base();

    if (beginIt >= endIt) {
      value_.clear();
      return;
    }

    value_ = std::string(beginIt, endIt);
  }

  void toUpperCase() {
    std::transform(value_.begin(), value_.end(), value_.begin(), [](unsigned char ch) {
      return static_cast<char>(std::toupper(ch));
    });
  }

  const char* c_str() const {
    return value_.c_str();
  }

  String& operator+=(char ch) {
    value_.push_back(ch);
    return *this;
  }

  String& operator+=(const String& rhs) {
    value_ += rhs.value_;
    return *this;
  }

  String& operator+=(const char* rhs) {
    value_ += (rhs != nullptr ? rhs : "");
    return *this;
  }

  friend bool operator==(const String& lhs, const String& rhs) {
    return lhs.value_ == rhs.value_;
  }

  friend bool operator!=(const String& lhs, const String& rhs) {
    return !(lhs == rhs);
  }

  friend bool operator==(const String& lhs, const char* rhs) {
    return lhs.value_ == (rhs != nullptr ? rhs : "");
  }

  friend bool operator==(const char* lhs, const String& rhs) {
    return rhs == lhs;
  }

  friend bool operator!=(const String& lhs, const char* rhs) {
    return !(lhs == rhs);
  }

  friend bool operator!=(const char* lhs, const String& rhs) {
    return !(lhs == rhs);
  }

  friend String operator+(const String& lhs, const String& rhs) {
    return String(lhs.value_ + rhs.value_);
  }

  friend String operator+(const String& lhs, const char* rhs) {
    return String(lhs.value_ + std::string(rhs != nullptr ? rhs : ""));
  }

  friend String operator+(const char* lhs, const String& rhs) {
    return String(std::string(lhs != nullptr ? lhs : "") + rhs.value_);
  }

 private:
  std::string value_;
};

constexpr int HIGH = 1;
constexpr int LOW = 0;

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline unsigned long millis() {
  static const auto start = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  return static_cast<unsigned long>(
    std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count()
  );
}

inline bool isPrintable(int c) {
  return std::isprint(static_cast<unsigned char>(c)) != 0;
}