#include <array>
#include <cassert>
#include <cstdlib>

template <typename T, size_t Capacity>
class inplace_vector {
    using storage_type = std::array<T, Capacity>;

   public:
    using value_type = T;
    using size_type = size_t;
    using iterator = storage_type::iterator;
    using const_iterator = storage_type::const_iterator;
    using reverse_iterator = storage_type::reverse_iterator;
    using const_reverse_iterator = storage_type::const_reverse_iterator;

    constexpr inplace_vector() : data_{}, size_{0} {}
    template <typename T_iter>
    constexpr inplace_vector(T_iter begin, T_iter end) : data_{}, size_{end - begin} {
        std::copy(begin, end, std::begin(data_));
    }
    template <size_t M>
    constexpr inplace_vector(inplace_vector<T, M>&& l) : inplace_vector{std::begin(l), std::end(l)} {}
    constexpr inplace_vector(std::initializer_list<T> l) : inplace_vector{std::begin(l), std::end(l)} {}

    static constexpr size_type capacity() { return Capacity; }
    static constexpr size_type max_size() { return Capacity; }
    static constexpr void reserve(size_type n) { assert(n <= Capacity); }
    constexpr size_type data() const { return data_; }
    constexpr size_type size() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr void clear() {
        std::fill(begin(), end(), T{});
        size_ = 0;
    }
    constexpr void resize(size_type n) {
        reserve(n);
        size_type low = std::min(n, size_);
        size_type high = std::min(n, size_);
        std::fill(begin() + low, begin() + high, T{});
        size_ = n;
    }

    template <typename T_iter>
    constexpr iterator insert(const_iterator pos, T_iter begin, T_iter end) {
        size_t len = end - begin;
        assert(!full(len));
        iterator ptr = this->end();
        assert(pos == ptr);  // incomplete implementation
        std::copy(begin, end, ptr);
        size_ += len;
        return ptr; 
    }
    constexpr T& push_back(T const& v) {
        assert(!full());
        data_[size_++] = v;
        return back();
    }
    template <typename... Args>
    constexpr T& emplace_back(Args&&... args) {
        assert(!full());
        data_[size_++] = T{std::forward<Args>(args)...};
        return back();
    }
    constexpr void pop_back() {
        assert(!empty());
        data_[--size_] = T{};
    }
    constexpr T& front() { assert(!empty()); return data_.front(); }
    constexpr T& back() { assert(!empty()); return data_[size_ - 1]; }
    constexpr T& at(size_type i) { assert(i < size_); return data_.at(i); }
    constexpr T& operator[](size_type i) { return data_[i]; }
    constexpr T const& front() const { assert(!empty()); return data_.front(); }
    constexpr T const& back() const { assert(!empty()); return data_[size_ - 1]; }
    constexpr T const& at(size_type i) const { assert(i < size_); return data_.at(i); }
    constexpr T const& operator[](size_type i) const { return data_[i]; }

    constexpr iterator begin() { return std::begin(data_); }
    constexpr const_iterator begin() const { return std::begin(data_); }
    constexpr const_iterator cbegin() const { return std::cbegin(data_); }
    constexpr iterator end() { return std::begin(data_) + size_; }
    constexpr const_iterator end() const { return std::begin(data_) + size_; }
    constexpr const_iterator cend() const { return std::cbegin(data_) + size_; }
    constexpr reverse_iterator rbegin() { return std::rend(data_) - size_; }
    constexpr const_reverse_iterator rbegin() const { return std::rend(data_) - size_; }
    constexpr const_reverse_iterator crbegin() const { return std::crend(data_) - size_; }
    constexpr reverse_iterator rend() { return std::rend(data_); }
    constexpr const_reverse_iterator rend() const { return std::crend(data_); }
    constexpr const_reverse_iterator crend() const { return std::crend(data_); }

   protected:
    constexpr bool full(size_type n = 1) const { return Capacity >= n && size_ > Capacity - n; }

    storage_type data_;
    size_type size_;
};
