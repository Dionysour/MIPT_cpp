#include <memory>

template<size_t storage_size>
class StackStorage {
 public:
  StackStorage() {
    static char arr[storage_size];
    array_ = reinterpret_cast<void*>(arr);
    free_memory_ = array_;
    left_bytes_ = storage_size;
  }

  StackStorage(const StackStorage<storage_size>& storage) = delete;

  template<typename T>
  void* allocate(size_t num) {
    if (std::align(alignof(T[num]),
                   sizeof(T[num]),
                   free_memory_,
                   left_bytes_)) {
      void* result = free_memory_;
      free_memory_ = static_cast<char*>(free_memory_) + sizeof(T[num]);
      left_bytes_ -= sizeof(T[num]);
      return result;
    }
    throw std::bad_alloc();
  }

  void deallocate([[maybe_unused]] void* ptr, [[maybe_unused]] size_t num) {}

 private:
  void* array_;
  void* free_memory_;
  size_t left_bytes_;
};

template<typename T, size_t storage_size>
class StackAllocator {
 public:
  using value_type = T;
  using pointer = T*;
  using storage_type = StackStorage<storage_size>;

  StackAllocator(storage_type& storage) {
    storage_ = &storage;
  }

  template<typename U>
  StackAllocator(const StackAllocator<U, storage_size>& alloc) {
    storage_ = alloc.get_storage();
  }

  StackAllocator& operator=(const StackAllocator& alloc) {
    storage_ = alloc.storage_;
    return *this;
  }

  pointer allocate(size_t num) {
    pointer result =
        reinterpret_cast<pointer>(storage_->template allocate<value_type>(num));
    return result;
  }

  using propagate_on_container_copy_assignment = std::true_type;

  template<typename U>
  struct rebind {
    using other = StackAllocator<U, storage_size>;
  };

  void deallocate(pointer ptr, size_t num) {
    storage_->deallocate(ptr, num);
  }

  storage_type* get_storage() const {
    return storage_;
  }

 private:
  storage_type* storage_;
};

template<typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct BaseNode;
  struct Node;

  using base_node_alloc = typename Allocator::template rebind<BaseNode>::other;
  using base_node_alloc_traits = std::allocator_traits<base_node_alloc>;
  using node_alloc = typename Allocator::template rebind<Node>::other;
  using node_alloc_traits = std::allocator_traits<node_alloc>;
 public:
  template<bool is_const>
  class Iterator;

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using allocator_type = Allocator;
  using alloc_traits = std::allocator_traits<allocator_type>;

  List(const allocator_type& alloc = Allocator()) : alloc_(alloc),
                                                    base_node_alloc_(alloc_),
                                                    node_alloc_(alloc_) {
    fake_node_ = base_node_alloc_traits::allocate(base_node_alloc_, 1);
    base_node_alloc_traits::construct(base_node_alloc_,
                                      fake_node_,
                                      fake_node_,
                                      fake_node_);
    size_ = 0;
  }

  List(size_t num, const allocator_type& alloc = Allocator()) : List(alloc) {
    for (size_t i = 0; i < num; ++i) {
      try {
        push_back();
      } catch (...) {
        BaseNode* cur_ptr = fake_node_->next_ptr;
        for (size_t j = 0; j < i; ++j) {
          BaseNode* next_ptr = cur_ptr->next_ptr;
          node_alloc_traits::destroy(node_alloc_,
                                     static_cast<Node*>(cur_ptr));
          node_alloc_traits::deallocate(node_alloc_,
                                        static_cast<Node*>(cur_ptr), 1);
          cur_ptr = next_ptr;
        }
        size_ = 0;
        throw;
      }
    }
  }

  List(size_t num, const T& item, const allocator_type& alloc = Allocator())
      : List(alloc) {
    for (size_t i = 0; i < num; ++i) {
      try {
        push_back(item);
      } catch (...) {
        BaseNode* cur_ptr = fake_node_->next_ptr;
        for (size_t j = 0; j < i; ++j) {
          BaseNode* next_ptr = cur_ptr->next_ptr;
          node_alloc_traits::destroy(node_alloc_,
                                     static_cast<Node*>(cur_ptr));
          node_alloc_traits::deallocate(node_alloc_,
                                        static_cast<Node*>(cur_ptr), 1);
          cur_ptr = next_ptr;
        }
        size_ = 0;
        throw;
      }
    }
  }

  List(const List& other) :
      alloc_(alloc_traits::select_on_container_copy_construction(other.alloc_)),
      base_node_alloc_(base_node_alloc_traits::select_on_container_copy_construction(other.base_node_alloc_)),
      node_alloc_(node_alloc_traits::select_on_container_copy_construction(other.node_alloc_)) {
    fake_node_ = base_node_alloc_traits::allocate(base_node_alloc_, 1);
    base_node_alloc_traits::construct(base_node_alloc_,
                                      fake_node_,
                                      fake_node_,
                                      fake_node_);
    size_ = 0;
    auto other_it = other.cbegin();
    for (size_t i = 0; i < other.size_; ++i) {
      try {
        push_back(*other_it);
      } catch (...) {
        BaseNode* cur_ptr = fake_node_->next_ptr;
        for (size_t j = 0; j < i; ++j) {
          BaseNode* next_ptr = cur_ptr->next_ptr;
          node_alloc_traits::destroy(node_alloc_,
                                     static_cast<Node*>(cur_ptr));
          node_alloc_traits::deallocate(node_alloc_,
                                        static_cast<Node*>(cur_ptr), 1);
          cur_ptr = next_ptr;
        }
        size_ = 0;
        base_node_alloc_traits::destroy(base_node_alloc_, fake_node_);
        base_node_alloc_traits::deallocate(base_node_alloc_, fake_node_, 1);
        size_ = 0;
        throw;
      }
      ++other_it;
    }
  }

  List(const List& other, const allocator_type& alloc) :
      alloc_(alloc),
      base_node_alloc_(base_node_alloc(alloc)),
      node_alloc_(node_alloc(alloc)) {
    fake_node_ = base_node_alloc_traits::allocate(base_node_alloc_, 1);
    base_node_alloc_traits::construct(base_node_alloc_,
                                      fake_node_,
                                      fake_node_,
                                      fake_node_);
    size_ = 0;
    auto other_it = other.cbegin();
    for (size_t i = 0; i < other.size_; ++i) {
      try {
        push_back(*other_it);
      } catch (...) {
        BaseNode* cur_ptr = fake_node_->next_ptr;
        for (size_t j = 0; j < i; ++j) {
          BaseNode* next_ptr = cur_ptr->next_ptr;
          node_alloc_traits::destroy(node_alloc_,
                                     static_cast<Node*>(cur_ptr));
          node_alloc_traits::deallocate(node_alloc_,
                                        static_cast<Node*>(cur_ptr), 1);
          cur_ptr = next_ptr;
        }
        size_ = 0;
        base_node_alloc_traits::destroy(base_node_alloc_, fake_node_);
        base_node_alloc_traits::deallocate(base_node_alloc_, fake_node_, 1);
        size_ = 0;
        throw;
      }
      ++other_it;
    }
  }

  void swap(List& other) {
    std::swap(fake_node_, other.fake_node_);
    std::swap(alloc_, other.alloc_);
    std::swap(base_node_alloc_, other.base_node_alloc_);
    std::swap(node_alloc_, other.node_alloc_);
    std::swap(size_, other.size_);
  }

  List& operator=(const List& other) {
    List tmp(other, select_on_container_copy_assignment(other.alloc_));
    swap(tmp);
    return *this;
  }

  iterator begin() {
    return iterator(fake_node_->next_ptr);
  }

  iterator end() {
    return iterator(fake_node_);
  }

  const_iterator begin() const { return cbegin(); }

  const_iterator end() const { return cend(); }

  const_iterator cbegin() const {
    return const_iterator(fake_node_->next_ptr);
  }

  const_iterator cend() const {
    return const_iterator(fake_node_);
  }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rbegin() const { return crbegin(); }

  const_reverse_iterator rend() const { return crend(); }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  size_t size() const {
    return size_;
  }

  void push_back(const T& item) {
    insert(end(), item);
  }

  void push_back() {
    insert(end());
  }

  void push_front(const T& item) {
    insert(begin(), item);
  }

  void insert(const_iterator iter, const T& item) {
    Node* new_node = node_alloc_traits::allocate(node_alloc_, 1);
    try {
      node_alloc_traits::construct(node_alloc_, new_node, iter.get_node_ptr(),
                                   iter.get_node_ptr()->prev_ptr, item);
    } catch (...) {
      node_alloc_traits::deallocate(node_alloc_, new_node, 1);
      throw;
    }
    iter.get_node_ptr()->prev_ptr->next_ptr = new_node;
    iter.get_node_ptr()->prev_ptr = new_node;
    size_ += 1;
  }

  void insert(const_iterator iter) {
    Node* new_node = node_alloc_traits::allocate(node_alloc_, 1);
    try {
      node_alloc_traits::construct(node_alloc_, new_node, iter.get_node_ptr(),
                                   iter.get_node_ptr()->prev_ptr);
    } catch (...) {
      node_alloc_traits::deallocate(node_alloc_, new_node, 1);
      throw;
    }
    iter.get_node_ptr()->prev_ptr->next_ptr = new_node;
    iter.get_node_ptr()->prev_ptr = new_node;
    size_ += 1;
  }

  void pop_back() {
    erase(--end());
  }

  void pop_front() {
    erase(begin());
  }

  void erase(const_iterator iter) {
    auto prev = iter.get_node_ptr()->prev_ptr;
    auto next = iter.get_node_ptr()->next_ptr;
    next->prev_ptr = prev;
    prev->next_ptr = next;
    node_alloc_traits::destroy(node_alloc_,
                               static_cast<Node*>(iter.get_node_ptr()));
    node_alloc_traits::deallocate(node_alloc_,
                                  static_cast<Node*>(iter.get_node_ptr()), 1);
    size_ -= 1;
  }

  allocator_type get_allocator() const {
    return alloc_;
  }

  ~List() {
    auto sz = size_;
    for (size_t i = 0; i < sz; ++i) {
      pop_back();
    }
    base_node_alloc_traits::destroy(base_node_alloc_, fake_node_);
    base_node_alloc_traits::deallocate(base_node_alloc_, fake_node_, 1);
  };

 private:
  struct BaseNode {
    BaseNode() = default;

    BaseNode(BaseNode* n_ptr, BaseNode* p_ptr) {
      next_ptr = n_ptr;
      prev_ptr = p_ptr;
    }

    BaseNode* next_ptr = nullptr;
    BaseNode* prev_ptr = nullptr;
  };

  struct Node : public BaseNode {
    Node() = delete;

    Node(BaseNode* n_ptr, BaseNode* p_ptr, const T& item)
        : BaseNode(n_ptr, p_ptr), value(item) {}

    Node(BaseNode* n_ptr, BaseNode* p_ptr)
        : BaseNode(n_ptr, p_ptr) {}
    T value;
  };

  allocator_type select_on_container_copy_assignment(const allocator_type& alloc) {
    if constexpr(alloc_traits::propagate_on_container_copy_assignment::value) {
      return alloc;
    } else {
      return alloc_traits::select_on_container_copy_construction(alloc);
    }
  }

  allocator_type alloc_;
  base_node_alloc base_node_alloc_;
  node_alloc node_alloc_;
  BaseNode* fake_node_;
  size_t size_;
};

template<typename T, typename Alloc>
template<bool is_const>
class List<T, Alloc>::Iterator {
 public:
  using value_type = std::conditional_t<is_const, const T, T>;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = int;

  explicit Iterator(BaseNode* ptr) {
    node_ = ptr;
  }

  operator Iterator<true>() {
    Iterator<true> const_iter(node_);
    return const_iter;
  };

  BaseNode* get_node_ptr() {
    return node_;
  }

  reference operator*() {
    return static_cast<Node*>(node_)->value;
  }

  pointer operator->() {
    return &static_cast<Node*>(node_)->value;
  }

  Iterator& operator++() {
    node_ = node_->next_ptr;
    return *this;
  }

  Iterator operator++(int) {
    Iterator result(node_);
    node_ = node_->next_ptr;
    return result;
  }

  Iterator& operator--() {
    node_ = node_->prev_ptr;
    return *this;
  }

  bool operator==(const Iterator& other) {
    return node_ == other.node_;
  }

  bool operator!=(const Iterator& other) {
    return node_ != other.node_;
  }

 private:
  BaseNode* node_;
};
