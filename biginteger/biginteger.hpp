#include <iostream>
#include <string>
#include <vector>

class BigInteger;
class Rational;

BigInteger operator*(const BigInteger& first, const BigInteger& second);
BigInteger operator/(const BigInteger& first, const BigInteger& second);
bool operator<(const BigInteger& first, const BigInteger& second);
bool operator>(const BigInteger& first, const BigInteger& second);
bool operator==(const BigInteger& first, const BigInteger& second);
bool operator<=(const BigInteger& first, const BigInteger& second);
bool operator>=(const BigInteger& first, const BigInteger& second);
bool operator!=(const BigInteger& first, const BigInteger& second);

class BigInteger {
 public:
  BigInteger() = default;

  BigInteger(int integer) {
    if (integer >= 0) {
      is_positive_ = true;
    } else {
      is_positive_ = false;
      integer = abs(integer);
    }
    if (integer == 0) {
      data_.push_back(0);
    }
    while (integer > 0) {
      data_.push_back(static_cast<int8_t>(integer % kBase));
      integer /= kBase;
    }
  }

  std::string toString() const {
    std::string result;
    if (!is_positive_) {
      result.push_back('-');
    }
    for (size_t index = 0; index < data_.size(); ++index) {
      result.push_back(
          static_cast<char>(data_[data_.size() - 1 - index] + '0'));
    }
    return result;
  }

  bool isPositive() const { return is_positive_; }

  bool isSmallerWithoutSign(const BigInteger& number) const {
    if (data_.size() < number.data_.size()) {
      return true;
    }
    if (data_.size() > number.data_.size()) {
      return false;
    }
    for (ssize_t index = static_cast<ssize_t>(data_.size() - 1); index > -1;
         --index) {
      if (data_[index] < number.data_[index]) {
        return true;
      }
      if (data_[index] > number.data_[index]) {
        return false;
      }
    }
    return false;
  }

  void setByString(const std::string& str) {
    data_.clear();
    is_positive_ = true;
    size_t str_size = str.size();
    if (str[0] == '-') {
      is_positive_ = false;
      str_size -= 1;
    }
    for (size_t index = 0; index < str_size; ++index) {
      data_.push_back(static_cast<int8_t>(str[str.size() - 1 - index] - '0'));
    }
  }

  BigInteger& operator+=(const BigInteger& number) {
    while (number.data_.size() > data_.size()) {
      data_.push_back(0);
    }
    if (is_positive_ == number.is_positive_) {
      sameSgnSum(number);
      return *this;
    }
    if (!number.is_positive_) {
      is_positive_ = false;
      if (*this <= number) {
        bigMinusSmall(number);
        is_positive_ = true;
        return *this;
      }
      smallMinusBig(number);
      return *this;
    }
    is_positive_ = true;
    if (*this >= number) {
      bigMinusSmall(number);
      if (data_.size() != 1 || data_[0] != 0) {
        is_positive_ = false;
      }
      return *this;
    }
    smallMinusBig(number);
    return *this;
  }

  BigInteger& operator-=(const BigInteger& number) {
    is_positive_ = !is_positive_;
    *this += number;
    is_positive_ = !is_positive_;
    return *this;
  }

  BigInteger& operator*=(const BigInteger& number) {
    BigInteger result;
    int8_t buffer = 0;
    for (size_t counter = 0; counter < data_.size() + number.data_.size();
         ++counter) {
      result.data_.push_back(0);
    }
    for (size_t first_index = 0; first_index < data_.size(); ++first_index) {
      for (size_t second_index = 0; second_index < number.data_.size();
           ++second_index) {
        buffer = data_[first_index] * number.data_[second_index] +
                 result.data_[first_index + second_index] + buffer;
        result.data_[first_index + second_index] = buffer % kBase;
        buffer /= kBase;
      }
      if (buffer > 0) {
        result.data_[first_index + number.data_.size()] = buffer;
      }
      buffer = 0;
    }
    result.is_positive_ = (is_positive_ == number.is_positive_);
    *this = result;
    deleteZeros();
    correctMinusZero();
    return *this;
  }

  void shiftExponent(int power) {
    if (power > 0) {
      for (size_t counter = 0; counter < static_cast<size_t>(power);
           ++counter) {
        data_.push_back(0);
      }
      for (size_t index = 0; index < data_.size() - power; ++index) {
        std::swap(data_[data_.size() - 1 - index],
                  data_[data_.size() - 1 - power - index]);
      }
    } else if (power < 0) {
      power = -power;
      for (size_t counter = 0; counter < static_cast<size_t>(power);
           ++counter) {
        data_[counter] = 0;
      }
      for (size_t index = power; index < data_.size(); ++index) {
        std::swap(data_[index - power], data_[index]);
      }
      deleteZeros();
    }
  }

  void reverse() {
    for (size_t index = 0; index < data_.size() / 2; ++index) {
      std::swap(data_[index], data_[data_.size() - 1 - index]);
    }
  }

  BigInteger& operator/=(const BigInteger& number) {
    BigInteger result;
    if (this == &number) {
      *this = 1;
      return *this;
    }
    if (data_.size() < number.data_.size()) {
      result = 0;
      *this = result;
      return *this;
    }
    int8_t buffer = 0;
    size_t shift = 0;
    size_t initial_size = data_.size();
    for (size_t counter = 0; counter < initial_size - number.data_.size() + 1;
         ++counter) {
      shift = initial_size - number.data_.size() - counter;
      while (isDivisibleWithShift(number, shift)) {
        minusWithShift(number, shift);
        buffer += 1;
      }
      result.data_.push_back(buffer);
      buffer = 0;
    }
    result.is_positive_ = is_positive_ == number.is_positive_;
    *this = result;
    reverse();
    deleteZeros();
    correctMinusZero();
    return *this;
  }

  BigInteger& operator%=(const BigInteger& number) {
    if (this == &number) {
      *this = 0;
      return *this;
    }
    if (data_.size() < number.data_.size()) {
      return *this;
    }
    size_t shift = 0;
    size_t initial_size = data_.size();
    for (size_t counter = 0; counter < initial_size - number.data_.size() + 1;
         ++counter) {
      shift = initial_size - number.data_.size() - counter;
      while (isDivisibleWithShift(number, shift)) {
        minusWithShift(number, shift);
      }
    }
    deleteZeros();
    correctMinusZero();
    return *this;
  }

  explicit operator bool() const { return *this != 0; }

  BigInteger& operator++() {
    *this += 1;
    return *this;
  }

  BigInteger operator++(int) {
    BigInteger result = *this;
    ++*this;
    return result;
  }

  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }

  BigInteger operator--(int) {
    BigInteger result = *this;
    --*this;
    return result;
  }

  BigInteger operator-() {
    BigInteger result = *this;
    result.is_positive_ = !is_positive_;
    result.correctMinusZero();
    return result;
  }

  void changeSgn() {
    is_positive_ = !is_positive_;
    correctMinusZero();
  }

 private:
  void sameSgnSum(const BigInteger& number) {
    int8_t buffer = 0;
    for (size_t index = 0; index < number.data_.size(); ++index) {
      buffer += data_[index] + number.data_[index];
      data_[index] = buffer % kBase;
      buffer /= kBase;
    }
    for (size_t index = number.data_.size(); index < data_.size(); ++index) {
      buffer += data_[index];
      data_[index] = buffer % kBase;
      buffer /= kBase;
    }
    if (buffer > 0) {
      data_.push_back(buffer);
    }
    correctMinusZero();
  }

  void bigMinusSmall(const BigInteger& number) {
    int8_t buffer = 0;
    for (size_t index = 0; index < number.data_.size(); ++index) {
      buffer += data_[index] - number.data_[index];
      if (buffer < 0) {
        data_[index] = buffer + kBase;
        buffer = -1;
      } else {
        data_[index] = buffer;
        buffer = 0;
      }
    }
    for (size_t index = number.data_.size(); index < data_.size(); ++index) {
      buffer += data_[index];
      if (buffer < 0) {
        data_[index] = buffer + kBase;
        buffer = -1;
      } else {
        data_[index] = buffer;
        buffer = 0;
      }
    }
    deleteZeros();
    correctMinusZero();
  }

  void smallMinusBig(const BigInteger& number) {
    int8_t buffer = 0;
    for (size_t index = 0; index < data_.size(); ++index) {
      buffer += number.data_[index] - data_[index];
      if (buffer < 0) {
        data_[index] = buffer + kBase;
        buffer = -1;
      } else {
        data_[index] = buffer;
        buffer = 0;
      }
    }
    deleteZeros();
    correctMinusZero();
  }

  size_t deleteZeros() {
    size_t counter = 0;
    while (data_.back() == 0 && data_.size() != 1) {
      data_.pop_back();
      counter += 1;
    }
    return counter;
  }

  bool isDivisibleWithShift(const BigInteger& number, size_t shift) {
    if (data_.size() < number.data_.size() + shift) {
      return false;
    }
    if (data_.size() > number.data_.size() + shift) {
      return true;
    }
    for (size_t counter = 0; counter < number.data_.size(); ++counter) {
      if (data_[number.data_.size() + shift - counter - 1] <
          number.data_[number.data_.size() - counter - 1]) {
        return false;
      }
      if (data_[number.data_.size() + shift - counter - 1] >
          number.data_[number.data_.size() - counter - 1]) {
        return true;
      }
    }
    return true;
  }

  void minusWithShift(const BigInteger& number, size_t shift) {
    int8_t buffer = 0;
    for (size_t index = 0; index < number.data_.size(); ++index) {
      buffer += data_[index + shift] - number.data_[index];
      if (buffer < 0) {
        data_[index + shift] = buffer + kBase;
        buffer = -1;
      } else {
        data_[index + shift] = buffer;
        buffer = 0;
      }
    }
    for (size_t index = number.data_.size() + shift; index < data_.size();
         ++index) {
      buffer += data_[index];
      if (buffer < 0) {
        data_[index] = buffer + kBase;
        buffer = -1;
      } else {
        data_[index] = buffer;
        buffer = 0;
      }
    }
    deleteZeros();
  }

  void correctMinusZero() {
    if (data_.size() == 1 && data_[0] == 0) {
      is_positive_ = true;
    }
  }

  std::vector<int8_t> data_;
  bool is_positive_ = true;
  static const int8_t kBase = 10;
};

bool operator<(const BigInteger& first, const BigInteger& second) {
  if (first.isPositive() != second.isPositive()) {
    return !first.isPositive();
  }
  if (first.isPositive()) {
    return first.isSmallerWithoutSign(second);
  }
  if (!first.isPositive()) {
    return !first.isSmallerWithoutSign(second);
  }
  return false;
}

bool operator>(const BigInteger& first, const BigInteger& second) {
  return second < first;
}

bool operator<=(const BigInteger& first, const BigInteger& second) {
  return !(second < first);
}

bool operator>=(const BigInteger& first, const BigInteger& second) {
  return !(first < second);
}

bool operator==(const BigInteger& first, const BigInteger& second) {
  return !((first < second) || (second < first));
}

bool operator!=(const BigInteger& first, const BigInteger& second) {
  return (first < second) || (second < first);
}

BigInteger operator+(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result += second;
  return result;
}

BigInteger operator-(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result -= second;
  return result;
}

BigInteger operator*(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result *= second;
  return result;
}

BigInteger operator/(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result /= second;
  return result;
}

BigInteger operator%(const BigInteger& first, const BigInteger& second) {
  BigInteger result = first;
  result %= second;
  return result;
}

std::ostream& operator<<(std::ostream& out, const BigInteger& bigint) {
  out << bigint.toString();
  return out;
}

std::istream& operator>>(std::istream& in, BigInteger& bigint) {
  std::string str;
  in >> str;
  bigint.setByString(str);
  return in;
}

BigInteger Gcd(BigInteger first, BigInteger second) {
  while (second) {
    first %= second;
    std::swap(first, second);
  }
  if (!first.isPositive()) {
    first.changeSgn();
    return first;
  }
  return first;
}

class Rational {
 public:
  Rational() = default;

  Rational(BigInteger big_int) : nominator_(big_int) {}

  Rational(int integer) : nominator_(integer) {}

  Rational& operator*=(const Rational& number) {
    nominator_ *= number.nominator_;
    denominator_ *= number.denominator_;
    squeeze();
    return *this;
  }

  Rational& operator/=(const Rational& number) {
    nominator_ *= number.denominator_;
    denominator_ *= number.nominator_;
    if (!denominator_.isPositive()) {
      denominator_.changeSgn();
      nominator_.changeSgn();
    }
    squeeze();
    return *this;
  }

  Rational& operator-=(const Rational& number) {
    BigInteger sec_nom = number.nominator_ * denominator_;
    denominator_ *= number.denominator_;
    nominator_ *= number.denominator_;
    nominator_ -= sec_nom;
    squeeze();
    return *this;
  }

  Rational& operator+=(const Rational& number) {
    BigInteger sec_nom = number.nominator_ * denominator_;
    denominator_ *= number.denominator_;
    nominator_ *= number.denominator_;
    nominator_ += sec_nom;
    squeeze();
    return *this;
  }

  std::string toString() const {
    std::string result;
    result += nominator_.toString();
    if (denominator_ != 1) {
      result += '/';
      result += denominator_.toString();
    }
    return result;
  }

  Rational operator-() {
    Rational result = *this;
    result.nominator_.changeSgn();
    return result;
  }

  const BigInteger& getNom() const { return nominator_; }

  const BigInteger& getDenominator() const { return denominator_; }

  std::string asDecimal(size_t precision = 0) {
    std::string result;
    BigInteger nom = nominator_;
    if (!nom.isPositive()) {
      result += '-';
    }
    result += (nom / denominator_).toString();
    nom %= denominator_;
    if (!nom.isPositive()) {
      nom.changeSgn();
    }
    if (precision != 0) {
      result += '.';
    }
    for (size_t counter = 0; counter < precision; ++counter) {
      nom.shiftExponent(1);
      result += (nom / denominator_).toString();
      nom %= denominator_;
    }
    return result;
  }

  explicit operator double() {
    double result;
    result = stod(asDecimal(kDoublePrecision));
    return result;
  }

 private:
  void squeeze() {
    BigInteger gcd;
    gcd = ::Gcd(nominator_, denominator_);
    nominator_ /= gcd;
    denominator_ /= gcd;
  }

  BigInteger nominator_ = 0;
  BigInteger denominator_ = 1;
  static const size_t kDoublePrecision = 18;
};

Rational operator+(const Rational& first, const Rational& second) {
  Rational result = first;
  result += second;
  return result;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational result = first;
  result -= second;
  return result;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational result = first;
  result *= second;
  return result;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational result = first;
  result /= second;
  return result;
}

bool operator<(const Rational& first, const Rational& second) {
  bool result = first.getNom() * second.getDenominator() <
                second.getNom() * first.getDenominator();
  return result;
}

bool operator>(const Rational& first, const Rational& second) {
  return second < first;
}

bool operator<=(const Rational& first, const Rational& second) {
  return !(second < first);
}

bool operator>=(const Rational& first, const Rational& second) {
  return !(first < second);
}

bool operator==(const Rational& first, const Rational& second) {
  return !((first < second) || (second < first));
}

bool operator!=(const Rational& first, const Rational& second) {
  return ((first < second) || (second < first));
}
