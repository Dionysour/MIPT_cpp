#include <cstring>
#include <iostream>

class String {
 public:
  String() : string_(new char[kZeroCapacity]){};

  String(const char* cstring)
      : size_(strlen(cstring)), string_(new char[size_]), capacity_(size_) {
    memcpy(string_, cstring, size_);
  }

  explicit String(size_t size, char ch = '0')
      : size_(size), string_(new char[size]), capacity_(size) {
    memset(string_, ch, size);
  }

  String(char ch) : String(1, ch) {}

  String(const String& string)
      : size_(string.size_),
        string_(new char[string.size_]),
        capacity_(string.size_) {
    memcpy(string_, string.string_, size_);
  }

  void swap(String& string) {
    std::swap(string_, string.string_);
    std::swap(size_, string.size_);
    std::swap(capacity_, string.capacity_);
  }

  String& operator=(const String& string) {
    String copy = string;
    swap(copy);
    return *this;
  }

  ~String() { delete[] string_; }

  char& operator[](size_t index) { return string_[index]; }

  char operator[](size_t index) const { return string_[index]; }

  size_t length() const { return size_; }

  void push_back(char ch) {
    if (size_ == capacity_) {
      reallocation(capacity_ * 2);
    }
    string_[size_] = ch;
    size_ += 1;
  }

  void pop_back() {
    if (size_ < capacity_ / kDecreaseCapacityRatio && size_ > kMinSize) {
      reallocation(capacity_ / 2);
    }
    size_ -= 1;
  }

  char& front() { return string_[0]; }

  char front() const { return string_[0]; }

  char& back() { return string_[size_ - 1]; }

  char back() const { return string_[size_ - 1]; }

  String& operator+=(const String& str2) {
    if (size_ + str2.size_ > capacity_) {
      reallocation((size_ + str2.size_) * 2);
    }
    memcpy(string_ + size_, str2.string_, str2.size_);
    size_ += str2.size_;
    return *this;
  }

  size_t find(const String& substring) const {
    size_t subindex;
    for (size_t index = 0; index < size_; ++index) {
      subindex = 0;
      while (subindex + index < size_ &&
             string_[index + subindex] == substring[subindex]) {
        if (subindex == substring.size_ - 1) {
          return index;
        }
        ++subindex;
      }
    }
    return size_;
  }

  size_t rfind(const String& substring) const {
    size_t subindex;
    for (ssize_t index = static_cast<ssize_t>(size_ - 1); index > -1; --index) {
      subindex = 0;
      while (subindex + index < size_ &&
             string_[index + subindex] == substring[subindex]) {
        if (subindex == substring.size_ - 1) {
          return index;
        }
        ++subindex;
      }
    }
    return size_;
  }

  String substr(size_t start, size_t count) const {
    String result = String(count);
    memcpy(result.string_, string_ + start, count);
    return result;
  }

  bool empty() const { return size_ == 0; }

  bool is_equal(const String& second) const {
    if (length() == second.length()) {
      for (size_t index = 0; index < length(); ++index) {
        if (operator[](index) != second[index]) {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  void clear() {
    while (!empty()) {
      pop_back();
    }
  }

 private:
  void reallocation(size_t capacity) {
    char* new_str = new char[capacity];
    memcpy(new_str, string_, size_);
    delete[] string_;
    string_ = new_str;
    capacity_ = capacity;
  }

  static const size_t kZeroCapacity = 20;
  static const size_t kMinSize = 10;
  static const size_t kDecreaseCapacityRatio = 4;
  size_t size_ = 0;
  char* string_ = nullptr;
  size_t capacity_ = kZeroCapacity;
};

String operator+(const String& str1, const String& str2) {
  String result = str1;
  result += str2;
  return result;
}

std::ostream& operator<<(std::ostream& out, const String& str) {
  for (size_t index = 0; index < str.length(); ++index) {
    out << str[index];
  }
  return out;
}

std::istream& operator>>(std::istream& in, String& str) {
  char ch;
  str.clear();
  while (EOF != (ch = static_cast<char>(in.get())) &&
         !static_cast<bool>(std::isspace(ch))) {
    str.push_back(ch);
  }
  return in;
}

bool operator==(const String& first, const String& second) {
  bool result = first.is_equal(second);
  return result;
}
