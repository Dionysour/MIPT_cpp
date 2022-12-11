#include <iterator>

template <typename Iterator>
class ReverseIterator;

template <typename T>
class Deque {
 private:
 public:
  template <bool is_const>
  class Iterator;
  using const_iterator = Iterator<true>;                           // NOLINT
  using iterator = Iterator<false>;                                // NOLINT
  using reverse_iterator = ReverseIterator<iterator>;              // NOLINT
  using const_reverse_iterator = ReverseIterator<const_iterator>;  // NOLINT

  Deque() {
    deque_size_ = 0;
    batches_ = 1;
    front_capacity_ = kBatchSize / 2;
    back_capacity_ = kBatchSize / 2;
    pointers_ = new T*[batches_];
    try {
      pointers_[0] = reinterpret_cast<T*>(new uint8_t[kBatchSize * sizeof(T)]);
    } catch (...) {
      delete[] pointers_;
      throw;
    }
  }

  explicit Deque(size_t capacity, const T& item = T()) {
    construct_raw_memory(capacity);
    for (auto iter = begin(); iter != end(); ++iter) {
      try {
        new (iter.return_pointer()) T(item);
      } catch (...) {
        for (auto sec_iter = begin(); sec_iter != iter; ++sec_iter) {
          iter->~T();
        }
        for (size_t i = 0; i < batches_; ++i) {
          delete[] reinterpret_cast<uint8_t*>(pointers_[i]);
        }
        delete[] pointers_;
        throw;
      }
    }
  }

  Deque(const Deque& deq) {
    batches_ = deq.batches_;
    front_capacity_ = deq.front_capacity_;
    back_capacity_ = deq.back_capacity_;
    deque_size_ = deq.deque_size_;
    pointers_ = new T*[batches_];
    for (size_t i = 0; i < batches_; ++i) {
      try {
        pointers_[i] =
            reinterpret_cast<T*>(new uint8_t[kBatchSize * sizeof(T)]);
      } catch (...) {
        for (size_t j = 0; j < i; ++j) {
          delete[] reinterpret_cast<uint8_t*>(pointers_[j]);
        }
        delete[] pointers_;
        throw;
      }
    }
    copy(deq);
  }

  void swap(Deque& deq1, Deque& deq2) {
    std::swap(deq1.pointers_, deq2.pointers_);
    std::swap(deq1.batches_, deq2.batches_);
    std::swap(deq1.front_capacity_, deq2.front_capacity_);
    std::swap(deq1.back_capacity_, deq2.back_capacity_);
    std::swap(deq1.deque_size_, deq2.deque_size_);
  }

  Deque& operator=(const Deque& deq) {
    Deque new_deq = deq;
    swap(*this, new_deq);
    return *this;
  }

  iterator begin() {
    iterator iter(pointers_ + (front_capacity_ / kBatchSize),
                  front_capacity_ % kBatchSize);
    return iter;
  }

  iterator end() {
    iterator iter(pointers_ + ((front_capacity_ + deque_size_) / kBatchSize),
                  (front_capacity_ + deque_size_) % kBatchSize);
    return iter;
  }

  const_iterator begin() const { return cbegin(); }

  const_iterator end() const { return cend(); }

  const_iterator cbegin() const {
    const_iterator iter(pointers_ + (front_capacity_ / kBatchSize),
                        front_capacity_ % kBatchSize);
    return iter;
  }

  const_iterator cend() const {
    const_iterator iter(
        pointers_ + ((front_capacity_ + deque_size_) / kBatchSize),
        (front_capacity_ + deque_size_) % kBatchSize);
    return iter;
  }

  reverse_iterator rbegin() { return ReverseIterator<iterator>(end()); }

  reverse_iterator rend() { return ReverseIterator<iterator>(begin()); }

  const_reverse_iterator crbegin() const {
    return ReverseIterator<const_iterator>(cend());
  }

  const_reverse_iterator crend() const {
    return ReverseIterator<const_iterator>(cbegin());
  }

  size_t size() const { return deque_size_; }

  void push_back(const T& item) {
    T** old_pointers = nullptr;
    if (back_capacity_ == 0) {
      old_pointers = reserve_back(batches_ * 2);
    }
    try {
      new ((end()).return_pointer()) T(item);
    } catch (...) {
      if (old_pointers != nullptr) {
        batches_ /= 2;
        for (size_t i = batches_; i < batches_ * 2; ++i) {
          delete[] reinterpret_cast<uint8_t*>(pointers_[i]);
        }
        delete[] pointers_;
        pointers_ = old_pointers;
        back_capacity_ = 0;
      }
      throw;
    }
    delete[] old_pointers;
    --back_capacity_;
    ++deque_size_;
  }

  void push_front(const T& item) {
    T** old_pointers = nullptr;
    if (front_capacity_ == 0) {
      old_pointers = reserve_front(batches_ * 2);
    }
    try {
      new ((--begin()).return_pointer()) T(item);
    } catch (...) {
      if (old_pointers != nullptr) {
        batches_ /= 2;
        for (size_t i = batches_ + 1; i < batches_ * 2 + 1; ++i) {
          delete[] reinterpret_cast<uint8_t*>(pointers_[batches_ * 2 - i]);
        }
        delete[] pointers_;
        pointers_ = old_pointers;
        front_capacity_ = 0;
      }
      throw;
    }
    delete[] old_pointers;
    --front_capacity_;
    ++deque_size_;
  }

  T& operator[](size_t index) { return *(begin() + index); }

  const T& operator[](size_t index) const { return *(cbegin() + index); }

  void pop_front() {
    begin()->~T();
    --deque_size_;
    ++front_capacity_;
  }

  void pop_back() {
    (--end())->~T();
    --deque_size_;
    ++back_capacity_;
  }

  T& at(size_t index) {
    if (index >= deque_size_) {
      throw std::out_of_range("index");
    }
    return operator[](index);
  }

  const T& at(size_t index) const {
    if (index >= deque_size_) {
      throw std::out_of_range("index");
    }
    return operator[](index);
  }

  void insert(iterator iter, const T& item) {
    if (iter == end()) {
      push_back(item);
    } else if (iter == begin()) {
      push_front(item);
    } else {
      T* iter_pointer = iter.operator->();
      push_back(item);
      for (auto it = end() - 1; it.operator->() != iter_pointer; --it) {
        std::swap(*it, *(it - 1));
      }
    }
  }

  void erase(iterator iter) {
    for (; iter != end() - 1; ++iter) {
      std::swap(*iter, *(iter + 1));
    }
    pop_back();
  }

  ~Deque() {
    for ([[maybe_unused]] auto it : *this) {
      it.~T();
    }
    for (size_t i = 0; i < batches_; ++i) {
      delete[] reinterpret_cast<uint8_t*>(pointers_[i]);
    }
    delete[] pointers_;
  }

 private:
  void copy(const Deque& deq) {
    for (size_t i = 0; i < deque_size_; ++i) {
      try {
        new (&(operator[](i))) T(deq[i]);
      } catch (...) {
        for (size_t j = 0; j < i; ++j) {
          operator[](j).~T();
        }
        for (size_t k = 0; k < batches_; ++k) {
          delete[] reinterpret_cast<uint8_t*>(pointers_[i]);
        }
        delete[] pointers_;
        throw;
      }
    }
  }

  void construct_raw_memory(size_t capacity) {
    deque_size_ = capacity;
    batches_ = deque_size_ / kBatchSize + 1;
    front_capacity_ = (kBatchSize * batches_ - deque_size_) / 2;
    back_capacity_ = kBatchSize * batches_ - deque_size_ - front_capacity_;
    pointers_ = new T*[batches_];
    for (size_t i = 0; i < batches_; ++i) {
      try {
        pointers_[i] =
            reinterpret_cast<T*>(new uint8_t[kBatchSize * sizeof(T)]);
      } catch (...) {
        for (size_t j = 0; j < i; ++j) {
          delete[] reinterpret_cast<uint8_t*>(pointers_[j]);
        }
        delete[] pointers_;
        throw;
      }
    }
  }

  T** reserve_front(size_t new_batches) {
    T** new_pointers = new T*[new_batches];
    for (size_t i = 1; i < batches_ + 1; ++i) {
      new_pointers[new_batches - i] = pointers_[batches_ - i];
    }
    for (size_t i = batches_ + 1; i < new_batches + 1; ++i) {
      try {
        new_pointers[new_batches - i] =
            reinterpret_cast<T*>(new uint8_t[kBatchSize * sizeof(T)]);
      } catch (...) {
        for (size_t j = batches_ + 1; j < i; ++j) {
          delete[] reinterpret_cast<uint8_t*>(new_pointers[new_batches - j]);
        }
        delete[] new_pointers;
        throw;
      }
    }
    T** old_pointers = pointers_;
    pointers_ = new_pointers;
    front_capacity_ += (new_batches - batches_) * kBatchSize;
    batches_ = new_batches;
    return old_pointers;
  }

  T** reserve_back(size_t new_batches) {
    T** new_pointers = new T*[new_batches];
    for (size_t i = 0; i < batches_; ++i) {
      new_pointers[i] = pointers_[i];
    }
    for (size_t i = batches_; i < new_batches; ++i) {
      try {
        new_pointers[i] =
            reinterpret_cast<T*>(new uint8_t[kBatchSize * sizeof(T)]);
      } catch (...) {
        for (size_t j = batches_; j < i; ++j) {
          delete[] reinterpret_cast<uint8_t*>(new_pointers[j]);
        }
        delete[] new_pointers;
        throw;
      }
    }
    T** old_pointers = pointers_;
    pointers_ = new_pointers;
    back_capacity_ += (new_batches - batches_) * kBatchSize;
    batches_ = new_batches;
    return old_pointers;
  }

  size_t deque_size_;
  T** pointers_;
  size_t front_capacity_;
  size_t back_capacity_;
  size_t batches_;
  static const size_t kBatchSize = 50;
};

template <typename T>
template <bool is_const>
class Deque<T>::Iterator {
 public:
  using value_type = std::conditional_t<is_const, const T, T>;  // NOLINT
  using reference = value_type&;                                // NOLINT
  using pointer = value_type*;                                  // NOLINT
  using difference_type = int;                                  // NOLINT
  using iterator_category = std::random_access_iterator_tag;    // NOLINT

  Iterator(T** batch_ptr, size_t index) {
    batch_ptr_ = batch_ptr;
    index_ = index;
  }

  reference operator*() const { return (*batch_ptr_)[index_]; }
  pointer operator->() const { return *batch_ptr_ + index_; }
  pointer return_pointer() const { return *batch_ptr_ + index_; };

  operator Deque<T>::const_iterator() {
    Deque<T>::const_iterator result(batch_ptr_, index_);
    return result;
  }

  Iterator& operator++() { return *this += 1; }

  Iterator operator++(int) {
    auto res = *this;
    this->operator++();
    return res;
  }

  Iterator& operator+=(difference_type diff) {
    if (diff >= 0) {
      batch_ptr_ += (index_ + diff) / kBatchSize;
      index_ = (index_ + diff) % kBatchSize;
      return *this;
    }
    return *this -= -diff;
  }

  Iterator& operator--() { return *this -= 1; }

  Iterator operator--(int) {
    auto res = *this;
    this->operator--();
    return res;
  }

  Iterator& operator-=(difference_type diff) {
    if (diff >= 0) {
      batch_ptr_ -= (kBatchSize - 1 - index_ + diff) / kBatchSize;
      index_ = kBatchSize - 1 - ((kBatchSize - 1 - index_ + diff) % kBatchSize);
      return *this;
    }
    return *this += -diff;
  }

  Iterator operator+(difference_type diff) const {
    Iterator result = *this;
    result += diff;
    return result;
  }

  Iterator operator-(difference_type diff) const {
    Iterator result = *this;
    result -= diff;
    return result;
  }

  difference_type operator-(const Iterator& other) const {
    return kBatchSize * (batch_ptr_ - other.batch_ptr_) + index_ - other.index_;
  }

  bool operator<(const Iterator& other) const {
    if (batch_ptr_ < other.batch_ptr_) {
      return true;
    }
    if (batch_ptr_ > other.batch_ptr_) {
      return false;
    }
    return index_ < other.index_;
  }

  bool operator>(const Iterator& other) const { return other < *this; }

  bool operator==(const Iterator& other) const {
    return !(*this < other || other < *this);
  }

  bool operator!=(const Iterator& other) const {
    return *this < other || other < *this;
  }

  bool operator<=(const Iterator& other) const { return !(other < *this); }

  bool operator>=(const Iterator& other) const { return !(*this < other); }

 private:
  T** batch_ptr_;
  size_t index_;
};

template <typename Iterator>
class ReverseIterator {
 public:
  using value_type = typename Iterator::value_type;                // NOLINT
  using reference = typename Iterator::reference;                  // NOLINT
  using pointer = typename Iterator::pointer;                      // NOLINT
  using difference_type = typename Iterator::difference_type;      // NOLINT
  using iterator_category = typename Iterator::iterator_category;  // NOLINT

  explicit ReverseIterator(Iterator iter) : iter_(iter) {}

  reference operator*() const { return *(iter_ - 1); }
  pointer operator->() const { return (iter_ - 1).operator->(); }

  ReverseIterator& operator++() {
    --iter_;
    return *this;
  }

  ReverseIterator operator++(int) {
    ReverseIterator res = *this;
    this->operator++();
    return res;
  }

  ReverseIterator& operator--() {
    ++iter_;
    return *this;
  }

  ReverseIterator operator--(int) {
    ReverseIterator res = *this;
    this->operator--();
    return res;
  }

  ReverseIterator& operator+=(difference_type diff) {
    iter_ -= diff;
    return *this;
  }

  ReverseIterator& operator-=(difference_type diff) {
    iter_ += diff;
    return *this;
  }

  ReverseIterator operator+(difference_type diff) const {
    return ReverseIterator<Iterator>(iter_ - diff);
  }

  ReverseIterator operator-(difference_type diff) const {
    return ReverseIterator<Iterator>(iter_ + diff);
  }

  difference_type operator-(const ReverseIterator& other) const {
    return (other.iter_ - iter_);
  }

  bool operator==(const ReverseIterator& rhs) const {
    return iter_ == rhs.iter_;
  }

  bool operator!=(const ReverseIterator& rhs) const { return !(rhs == *this); }

  bool operator<(const ReverseIterator& rhs) const { return rhs.iter_ < iter_; }

  bool operator>(const ReverseIterator& rhs) const { return rhs < *this; }

  bool operator<=(const ReverseIterator& rhs) const { return !(rhs < *this); }

  bool operator>=(const ReverseIterator& rhs) const { return !(*this < rhs); }

 private:
  Iterator iter_;
};
