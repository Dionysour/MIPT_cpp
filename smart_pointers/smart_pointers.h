#ifndef SHARED__SHARED_PTR_H_
#define SHARED__SHARED_PTR_H_

#include <memory>

template <typename T>
class SharedPtr {
 public:
  SharedPtr() : control_block_(nullptr), ptr_(nullptr) {}

  template<typename Y>
  explicit SharedPtr(Y* ptr) :
      control_block_(new PointerControlBlock<Y, std::default_delete<Y>, std::allocator<Y>>
                         (1, 0, ptr, std::default_delete<Y>(), std::allocator<Y>())), ptr_(ptr) {};

  template<typename Y, typename Deleter>
  explicit SharedPtr(Y* ptr, Deleter deleter) :
      control_block_(new PointerControlBlock<Y, Deleter, std::allocator<Y>>
                         (1, 0, ptr, deleter, std::allocator<Y>())), ptr_(ptr) {};

  template<typename Y, typename Deleter, typename Alloc>
  explicit SharedPtr(Y* ptr, Deleter deleter, Alloc alloc) : ptr_(ptr) {
    using ControlBlock = PointerControlBlock<Y, Deleter, Alloc>;
    using alloc_traits = std::allocator_traits<Alloc>;
    typename alloc_traits::template rebind_alloc<ControlBlock> block_alloc = alloc;
    control_block_ = block_alloc.allocate(1);
    new(control_block_) ControlBlock(1, 0, ptr, deleter, alloc);
  };

  template <typename Y>
  SharedPtr(const SharedPtr<Y>& other) {
    if (!other.get_cb()) {
      control_block_ = nullptr;
      ptr_ = nullptr;
      return;
    }
    control_block_ = reinterpret_cast<BaseControlBlock*>(other.get_cb());
    control_block_->shared_count += 1;
    ptr_ = other.get();
  }

  SharedPtr(const SharedPtr& other) {
    if (!other.get_cb()) {
      control_block_ = nullptr;
      ptr_ = nullptr;
      return;
    }
    control_block_ = other.control_block_;
    control_block_->shared_count += 1;
    ptr_ = other.ptr_;
  }

  template<typename Y>
  SharedPtr(SharedPtr<Y>&& other) noexcept {
    control_block_ = reinterpret_cast<BaseControlBlock*>(other.get_cb());
    control_block_->shared_count += 1;
    ptr_ = other.get();
    other.reset();
  }

  SharedPtr(SharedPtr&& other) noexcept {
    control_block_ = other.control_block_;
    ptr_ = other.ptr_;
    other.control_block_ = nullptr;
    other.ptr_ = nullptr;
  }

  template<typename Y>
  SharedPtr& operator=(const SharedPtr<Y>& other) {
    auto buf = SharedPtr(other);
    swap(buf);
    return *this;
  }

  SharedPtr& operator=(const SharedPtr& other) {
    auto buf = SharedPtr(other);
    swap(buf);
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) noexcept {
    auto buf = SharedPtr(std::move(other));
    swap(buf);
    return *this;
  }

  template<typename Y>
  SharedPtr& operator=(SharedPtr<Y>&& other) noexcept {
    auto buf = SharedPtr(std::move(other));
    swap(buf);
    return *this;
  }

  T* operator->() const { return ptr_; }

  T& operator*() const { return *ptr_; }

  T* get() const { return ptr_; }

  void reset() {
    if (!control_block_) { return; }
    control_block_->shared_count -= 1;
    if (control_block_->shared_count == 0) {
      control_block_->destroy_obj();
      if (control_block_->weak_count == 0) {
        control_block_->deallocate_this();
      }
    }
    control_block_ = nullptr;
    ptr_ = nullptr;
  }

  template<typename Y>
  void reset(Y* ptr) {
    reset();
    *this = std::move(SharedPtr(ptr));
  }

  void swap(SharedPtr& other) {
    std::swap(control_block_, other.control_block_);
    std::swap(ptr_, other.ptr_);
  }

  size_t use_count() const {
    return control_block_->shared_count;
  }

  ~SharedPtr() {
    if (!control_block_) { return; }
    control_block_->shared_count -= 1;
    if (control_block_->shared_count == 0) {
      control_block_->destroy_obj();
      if (control_block_->weak_count == 0) {
        control_block_->deallocate_this();
      }
    }
  }

  struct BaseControlBlock {
    size_t shared_count;
    size_t weak_count;

    BaseControlBlock(size_t shared_c, size_t weak_c) : shared_count(shared_c),
                                                       weak_count(weak_c) {}

    template<typename Y>
    BaseControlBlock& operator=(typename SharedPtr<Y>::BaseControlBlock other) {

    }

    virtual void destroy_obj() = 0;
    virtual void deallocate_this() = 0;

    virtual T* get() = 0;
  };

  template <typename Y, typename Alloc>
  struct MakeSharedControlBlock : BaseControlBlock {
    Y obj;
    Alloc alloc;

    template<typename... Args>
    MakeSharedControlBlock(size_t shared_c, size_t weak_c, Alloc allocator, Args&&... args) :
        BaseControlBlock(shared_c, weak_c), obj(std::forward<Args>(args)...), alloc(allocator) {};

    void destroy_obj() override {
      std::allocator_traits<Alloc>::destroy(alloc, &obj);
    }

    void deallocate_this() override {
      using alloc_traits = std::allocator_traits<Alloc>;
      typename alloc_traits::template rebind_alloc<MakeSharedControlBlock> block_alloc = alloc;
      block_alloc.deallocate(this, 1);
    }

    T* get() override {
      return &obj;
    }
  };

  template <typename Y, typename Deleter, typename Alloc>
  struct PointerControlBlock : BaseControlBlock {
    Y* ptr;
    Deleter deleter;
    Alloc alloc;

    PointerControlBlock(size_t shared_c, size_t weak_c, Y* pointer,
                        Deleter del = Deleter(), Alloc allocator = Alloc()) :
        BaseControlBlock(shared_c, weak_c), ptr(pointer), deleter(del),
        alloc(allocator) {}

    void destroy_obj() override {
      deleter(ptr);
    }

    void deallocate_this() override {
      using alloc_traits = std::allocator_traits<Alloc>;
      typename alloc_traits::template rebind_alloc<PointerControlBlock> block_alloc = alloc;
      block_alloc.deallocate(this, 1);
    }

    T* get() override {
      return ptr;
    }
  };

  BaseControlBlock* get_cb() const {
    return control_block_;
  }

 private:
  SharedPtr(typename SharedPtr::BaseControlBlock* control_block) {
    control_block_ = control_block;
    ptr_ = control_block_->get();
  }

  template <typename Y, typename Alloc, typename... Args>
  friend SharedPtr<Y> allocateShared(Alloc, Args&&...);

  template <typename Y>
  friend class WeakPtr;

  BaseControlBlock* control_block_;
  T* ptr_;
};

template<typename Y, typename Alloc, typename... Args>
SharedPtr<Y> allocateShared(Alloc alloc, Args&&... args) {
  using alloc_traits = std::allocator_traits<Alloc>;
  using control_block_type = typename SharedPtr<Y>::template MakeSharedControlBlock<Y, Alloc>;

  typename alloc_traits::template rebind_alloc<control_block_type> block_alloc = alloc;
  auto control_block = block_alloc.allocate(1);
  alloc_traits::construct(alloc, control_block, 1, 0, alloc, std::forward<Args>(args)...);
  auto base_control_block = dynamic_cast<typename SharedPtr<Y>::BaseControlBlock*>(control_block);
  return SharedPtr<Y>(base_control_block);
}

template<typename Y, typename... Args>
SharedPtr<Y> makeShared(Args&&... args) {
  return allocateShared<Y, std::allocator<Y>>(std::allocator<Y>(), std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {
 public:
  WeakPtr() : control_block_(nullptr) {}

  template <typename Y>
  WeakPtr(const SharedPtr<Y>& shared) {
    control_block_ = reinterpret_cast<BaseControlBlock*>(shared.control_block_);
    control_block_->weak_count += 1;
  }

  WeakPtr(const WeakPtr& other) {
    control_block_ = other.control_block_;
    control_block_->weak_count += 1;
  }

  template<typename Y>
  WeakPtr(const WeakPtr<Y>& other) {
    control_block_ = reinterpret_cast<BaseControlBlock*>(other.get_cb());
    control_block_->weak_count += 1;
  }

  template<typename Y>
  WeakPtr& operator=(const WeakPtr<Y>& other) {
    WeakPtr buf = other;
    swap(buf);
    return *this;
  }

  template<typename Y>
  WeakPtr& operator=(WeakPtr<Y>&& other) {
    WeakPtr buf = std::move(other);
    swap(buf);
    return *this;
  }

  template<typename Y>
  WeakPtr& operator=(const SharedPtr<Y>& other) {
    WeakPtr buf = other;
    swap(buf);
    return *this;
  }

  template<typename Y>
  WeakPtr& operator=(SharedPtr<Y>&& other) {
    WeakPtr buf = std::move(other);
    swap(buf);
    return *this;
  }

  void swap(WeakPtr& other) {
    std::swap(control_block_, other.control_block_);
  }

  SharedPtr<T> lock() const {
    control_block_->shared_count += 1;
    return SharedPtr<T>(control_block_);
  }

  size_t use_count() const {
    return control_block_->shared_count;
  }

  bool expired() const {
    return control_block_->shared_count == 0;
  }

  void reset() {
    if (!control_block_) { return; }
    control_block_->weak_count -= 1;
    if (control_block_->shared_count == 0 and control_block_->weak_count == 0) {
      control_block_->deallocate_this();
    }
    control_block_ = nullptr;
  }

  ~WeakPtr() {
    if (!control_block_) { return; }
    control_block_->weak_count -= 1;
    if (control_block_->shared_count == 0 and control_block_->weak_count == 0) {
      control_block_->deallocate_this();
    }
  }

  using BaseControlBlock = typename SharedPtr<T>::BaseControlBlock;

  BaseControlBlock* get_cb() const {
    return control_block_;
  }
 private:
  BaseControlBlock* control_block_;
};

#endif //SHARED__SHARED_PTR_H_
